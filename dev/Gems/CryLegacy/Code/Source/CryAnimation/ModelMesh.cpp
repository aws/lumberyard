/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

//  Contains:
//  Implementation of all software skinning & morphing functions

#include "CryLegacy_precompiled.h"
#include <IRenderAuxGeom.h>
#include "ModelMesh.h"
#include "QTangent.h"
#include "LoaderCHR.h"

CModelMesh::CModelMesh()
{
    m_iThreadMeshAccessCounter = 0;
    m_vRenderMeshOffset = Vec3(ZERO);
    m_pIRenderMesh = 0;
    m_faceCount = 0;
    m_geometricMeanFaceArea = 0.0f;
    m_pIDefaultMaterial = g_pI3DEngine->GetMaterialManager()->GetDefaultMaterial(); // the material physical game id that will be used as default for this character
}

uint32 CModelMesh::InitMesh(CMesh* pMesh, CNodeCGF* pMeshNode, _smart_ptr<IMaterial> pMaterial, CSkinningInfo* pSkinningInfo, const char* szFilePath, uint32 nLOD)
{
    m_vRenderMeshOffset = pMeshNode ? pMeshNode->worldTM.GetTranslation() : Vec3(ZERO);
    if (pMaterial)
    {
        m_pIDefaultMaterial = pMaterial;
    }
    else
    {
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, szFilePath, "No material in lod %d", nLOD);
    }

    if (pMesh)
    {
        m_geometricMeanFaceArea = pMesh->m_geometricMeanFaceArea;
        m_faceCount = pMesh->GetIndexCount() / 3;

        uint32 numIntFaces = pSkinningInfo->m_arrIntFaces.size();
        uint32 numExtFaces = pMesh->GetIndexCount() / 3;
        if (numExtFaces != numIntFaces)
        {
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, szFilePath, "external and internal face-amount is different. Probably caused by degenerated faces");
        }

        PrepareMesh(pMesh);
    }

    m_softwareMesh.GetVertexFrames().Create(*pSkinningInfo, -m_vRenderMeshOffset);

    if (!Console::GetInst().ca_StreamCHR && pMesh)
    {
        m_pIRenderMesh = CreateRenderMesh(pMesh, szFilePath, nLOD, true);
        PrepareRenderChunks(*pMesh, m_arrRenderChunks);
    }

    return 1;
}

_smart_ptr<IRenderMesh> CModelMesh::InitRenderMeshAsync(CMesh* pMesh, const char* szFilePath, int nLod, DynArray<RChunk>& arrNewRenderChunks)
{
    PrepareMesh(pMesh);

    _smart_ptr<IRenderMesh> pNewRenderMesh = CreateRenderMesh(pMesh, szFilePath, nLod, false);
    PrepareRenderChunks(*pMesh, arrNewRenderChunks);

    return pNewRenderMesh;
}

uint32 CModelMesh::InitRenderMeshSync(DynArray<RChunk>& arrNewRenderChunks, _smart_ptr<IRenderMesh> pNewRenderMesh)
{
    m_arrRenderChunks.swap(arrNewRenderChunks);
    m_pIRenderMesh = pNewRenderMesh;

    return 1;
}

uint32 CModelMesh::IsVBufferValid()
{
    if (m_pIRenderMesh == 0)
    {
        return 0;
    }
    m_pIRenderMesh->LockForThreadAccess();
    uint32 numIndices     = m_pIRenderMesh->GetIndicesCount();
    uint32 numVertices    = m_pIRenderMesh->GetVerticesCount();
    vtx_idx* pIndices = m_pIRenderMesh->GetIndexPtr(FSL_READ);
    if (pIndices == 0)
    {
        return 0;
    }
    int32       nPositionStride;
    uint8*  pPositions          = m_pIRenderMesh->GetPosPtr(nPositionStride, FSL_READ);
    if (pPositions == 0)
    {
        return 0;
    }
    int32       nSkinningStride;
    uint8* pSkinningInfo       = m_pIRenderMesh->GetHWSkinPtr(nSkinningStride, FSL_READ);  //pointer to weights and bone-id
    if (pSkinningInfo == 0)
    {
        return 0;
    }
    ++m_iThreadMeshAccessCounter;


    m_pIRenderMesh->UnLockForThreadAccess();
    --m_iThreadMeshAccessCounter;
    if (m_iThreadMeshAccessCounter == 0)
    {
        m_pIRenderMesh->UnlockStream(VSF_GENERAL);
        m_pIRenderMesh->UnlockStream(VSF_HWSKIN_INFO);
    }

    return 1;
}

void CModelMesh::AbortStream()
{
    if (m_stream.pStreamer)
    {
        m_stream.pStreamer->AbortLoad();
    }
}

ClosestTri CModelMesh::GetAttachmentTriangle(const Vec3& RMWPosition, const JointIdType* const pRemapTable)
{
    ClosestTri cf;
    if (m_pIRenderMesh == 0)
    {
        return cf;
    }
    m_pIRenderMesh->LockForThreadAccess();
    uint32  numIndices          =   m_pIRenderMesh->GetIndicesCount();
    uint32  numVertices         = m_pIRenderMesh->GetVerticesCount();
    vtx_idx* pIndices               = m_pIRenderMesh->GetIndexPtr(FSL_READ);
    if (pIndices == 0)
    {
        return cf;
    }
    int32       nPositionStride;
    uint8*  pPositions          = m_pIRenderMesh->GetPosPtr(nPositionStride, FSL_READ);
    if (pPositions == 0)
    {
        return cf;
    }
    int32       nSkinningStride;
    uint8* pSkinningInfo       = m_pIRenderMesh->GetHWSkinPtr(nSkinningStride, FSL_READ, false);  //pointer to weights and bone-id
    if (pSkinningInfo == 0)
    {
        return cf;
    }
    ++m_iThreadMeshAccessCounter;

    f32 distance = 99999999.0f;
    uint32 foundPos(0);
    for (uint32 d = 0; d < numIndices; d += 3)
    {
        Vec3 v0 = *(Vec3*)(pPositions + pIndices[d + 0] * nPositionStride);
        Vec3 v1 = *(Vec3*)(pPositions + pIndices[d + 1] * nPositionStride);
        Vec3 v2 = *(Vec3*)(pPositions + pIndices[d + 2] * nPositionStride);
        Vec3 TriMiddle = (v0 + v1 + v2) / 3.0f + m_vRenderMeshOffset;
        f32 sqdist = (TriMiddle - RMWPosition) | (TriMiddle - RMWPosition);
        if  (distance > sqdist)
        {
            distance = sqdist, foundPos = d;
        }
    }

    for (uint32 t = 0; t < 3; t++)
    {
        uint32 e = pIndices[ foundPos + t ];
        Vec3        hwPosition  = *(Vec3*)(pPositions + e * nPositionStride);
        uint16* hwBoneIDs   =  ((SVF_W4B_I4S*)(pSkinningInfo + e * nSkinningStride))->indices;
        ColorB  hwWeights       = *(ColorB*)&((SVF_W4B_I4S*)(pSkinningInfo + e * nSkinningStride))->weights;
        cf.v[t].m_attTriPos  = hwPosition + m_vRenderMeshOffset;
        cf.v[t].m_attWeights[0] = hwWeights[0] / 255.0f;
        cf.v[t].m_attWeights[1] = hwWeights[1] / 255.0f;
        cf.v[t].m_attWeights[2] = hwWeights[2] / 255.0f;
        cf.v[t].m_attWeights[3] = hwWeights[3] / 255.0f;
        if (pRemapTable)
        {
            cf.v[t].m_attJointIDs[0] = pRemapTable[hwBoneIDs[0]];
            cf.v[t].m_attJointIDs[1] = pRemapTable[hwBoneIDs[1]];
            cf.v[t].m_attJointIDs[2] = pRemapTable[hwBoneIDs[2]];
            cf.v[t].m_attJointIDs[3] = pRemapTable[hwBoneIDs[3]];
        }
        else
        {
            cf.v[t].m_attJointIDs[0] = hwBoneIDs[0];
            cf.v[t].m_attJointIDs[1] = hwBoneIDs[1];
            cf.v[t].m_attJointIDs[2] = hwBoneIDs[2];
            cf.v[t].m_attJointIDs[3] = hwBoneIDs[3];
        }
    }

    m_pIRenderMesh->UnLockForThreadAccess();
    --m_iThreadMeshAccessCounter;
    if (m_iThreadMeshAccessCounter == 0)
    {
        m_pIRenderMesh->UnlockStream(VSF_GENERAL);
        m_pIRenderMesh->UnlockStream(VSF_HWSKIN_INFO);
    }

    return cf;
}



uint32 CModelMesh::InitSWSkinBuffer()
{
    bool valid = m_softwareMesh.IsInitialized();
    if (!valid && m_pIRenderMesh)
    {
        m_pIRenderMesh->LockForThreadAccess();
        ++m_iThreadMeshAccessCounter;

        valid = m_softwareMesh.Create(*m_pIRenderMesh, m_arrRenderChunks, m_vRenderMeshOffset);
        m_pIRenderMesh->UnLockForThreadAccess();
        --m_iThreadMeshAccessCounter;
        if (m_iThreadMeshAccessCounter == 0)
        {
            m_pIRenderMesh->UnlockStream(VSF_GENERAL);
            m_pIRenderMesh->UnlockStream(VSF_TANGENTS);
            m_pIRenderMesh->UnlockStream(VSF_HWSKIN_INFO);
        }
    }
    return valid;
}


size_t CModelMesh::SizeOfModelMesh() const
{
    uint32 nSize = sizeof(CModelMesh);
    uint32 numRenderChunks = m_arrRenderChunks.size();
    nSize += m_arrRenderChunks.get_alloc_size();

    return nSize;
}


void CModelMesh::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_arrRenderChunks);
}

void CModelMesh::PrepareMesh(CMesh* pMesh)
{
    //HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-HACK-
    //fix this in the exporter
    uint32 numMeshSubsets = pMesh->m_subsets.size();
    for (uint32 i = 0; i < numMeshSubsets; i++)
    {
        pMesh->m_subsets[i].FixRanges(pMesh->m_pIndices);
    }

    // TEMP: Convert tangent frame from matrix to quaternion based.
    // This code should go into RC and not be performed at loading time!
    if (pMesh->m_pTangents && !pMesh->m_pQTangents)
    {
        pMesh->m_pQTangents = (SMeshQTangents*)pMesh->m_pTangents;
        MeshTangentsFrameToQTangents(
            pMesh->m_pTangents, sizeof(pMesh->m_pTangents[0]), pMesh->GetVertexCount(),
            pMesh->m_pQTangents, sizeof(pMesh->m_pQTangents[0]));
    }
}

void CModelMesh::PrepareRenderChunks(CMesh& mesh, DynArray<RChunk>& renderChunks)
{
#if defined(SUBDIVISION_ACC_ENGINE)
    DynArray<SMeshSubset>& subsets = mesh.m_bIsSubdivisionMesh ? mesh.m_subdSubsets : mesh.m_subsets;
#else
    DynArray<SMeshSubset>& subsets = mesh.m_subsets;
#endif

    const uint subsetCount = uint(subsets.size());
    renderChunks.resize(subsetCount);
    for (uint i = 0; i < subsetCount; i++)
    {
        renderChunks[i].m_nFirstIndexId = subsets[i].nFirstIndexId;
        renderChunks[i].m_nNumIndices = subsets[i].nNumIndices;
    }

    m_softwareMesh.Create(mesh, renderChunks, m_vRenderMeshOffset);
}

_smart_ptr<IRenderMesh> CModelMesh::CreateRenderMesh(CMesh* pMesh, const char* szFilePath, int nLod, bool bCreateDeviceMesh)
{
    //////////////////////////////////////////////////////////////////////////
    // Create the RenderMesh
    //////////////////////////////////////////////////////////////////////////

    ERenderMeshType eRMType = eRMT_Static;

#if defined(WIN32) || defined(WIN64)
    eRMType = eRMT_Dynamic;
#endif

    _smart_ptr<IRenderMesh> pRenderMesh =   g_pIRenderer->CreateRenderMesh("Character", szFilePath, NULL, eRMType);
    assert(pRenderMesh != 0);
    if (pRenderMesh == 0)
    {
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, szFilePath, "Failed to create render mesh for lod %d", nLod);
        return 0;
    }

    uint32 nFlags = FSM_VERTEX_VELOCITY;
    if (bCreateDeviceMesh)
    {
        nFlags |= FSM_CREATE_DEVICE_MESH;
    }
#ifdef MESH_TESSELLATION_ENGINE
    nFlags |= FSM_ENABLE_NORMALSTREAM;
#endif
    pRenderMesh->SetMesh(*pMesh, 0, nFlags, true);

    return pRenderMesh;
}
