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

// Description : Create buffer, copy it into var memory, draw


#include "StdAfx.h"

#include "terrain.h"
#include "terrain_sector.h"
#include "ObjMan.h"
#include "terrain_water.h"
#include "CryThread.h"

#include <AzCore/Jobs/LegacyJobExecutor.h>

template<typename Type>
class InPlaceArray
{
public:
    InPlaceArray()
        : m_Data(nullptr)
        , m_Size(0)
        , m_Capacity(0)
    {}

    void Clear()
    {
        m_Size = 0;
    }

    void SetMemory(Type* memory, size_t capacity)
    {
        m_Data = memory;
        m_Size = 0;
        m_Capacity = capacity;
    }

    void Add(const Type& obj)
    {
        assert(m_Size < m_Capacity);
        if (m_Size == m_Capacity)
        {
            CryFatalError("ran out of space in array");
            return;
        }

        m_Data[m_Size++] = obj;
    }

    Type* GetElements()
    {
        return m_Data;
    }

    const Type* GetElements() const
    {
        return m_Data;
    }

    size_t Count() const
    {
        return m_Size;
    }

    size_t MemorySize() const
    {
        return m_Capacity * sizeof(Type);
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(GetElements(), MemorySize());
    }

private:
    Type* m_Data;
    size_t m_Size;
    size_t m_Capacity;
};

class InPlaceIndexBuffer
    : public InPlaceArray<vtx_idx>
{
public:
    inline void AddIndex(int x, int y, int step, int sectorSize)
    {
        Add(GetIndex(x, y, step, sectorSize));
    }

    static vtx_idx GetIndex(int x, int y, int step, int sectorSize)
    {
        vtx_idx step_x = x / step;
        vtx_idx step_y = y / step;
        vtx_idx step_ss = sectorSize / step;

        return step_x * (step_ss + 1) + step_y;
    }
};

class BuildMeshData
{
public:
    AZ::LegacyJobExecutor m_jobExecutor;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_Indices);
        pSizer->AddObject(m_Vertices);
    }

    // Align objects to 128 byte to provide each with an own cache line since they are updated in parallel
    InPlaceIndexBuffer m_Indices _ALIGN(128);
    InPlaceArray<SVF_P2S_N4B_C4B_T1F> m_Vertices _ALIGN(128);
};

CTerrainUpdateDispatcher::CTerrainUpdateDispatcher()
{
    m_pHeapStorage = CryGetIMemoryManager()->AllocPages(TempPoolSize);
    m_pHeap = CryGetIMemoryManager()->CreateGeneralMemoryHeap(m_pHeapStorage, TempPoolSize, "Terrain temp pool");
}

CTerrainUpdateDispatcher::~CTerrainUpdateDispatcher()
{
    SyncAllJobs(true, SRenderingPassInfo::CreateGeneralPassRenderingInfo(gEnv->p3DEngine->GetRenderingCamera()));
    if (m_queuedJobs.size() || m_arrRunningJobs.size())
    {
        CryFatalError(
            "CTerrainUpdateDispatcher::~CTerrainUpdateDispatcher(): instance still has jobs "
            "queued while being destructed!\n");
    }

    m_pHeap = NULL;
    CryGetIMemoryManager()->FreePages(m_pHeapStorage, TempPoolSize);
}

void CTerrainUpdateDispatcher::QueueJob(CTerrainNode* pNode, const SRenderingPassInfo& passInfo)
{
    if (pNode && !Contains(pNode))
    {
        if (!AddJob(pNode, true, passInfo))
        {
            // if job submission was unsuccessful, queue the terrain job
            m_queuedJobs.Add(pNode);
        }
    }
}

bool CTerrainUpdateDispatcher::AddJob(CTerrainNode* pNode, bool executeAsJob, const SRenderingPassInfo& passInfo)
{
    // 1U<<MML_NOT_SET will generate 0!
    if (pNode->m_QueuedLOD == MML_NOT_SET)
    {
        return true;
    }

    pNode->CheckLeafData();
    STerrainNodeLeafData& leafData = *pNode->GetLeafData();

    const unsigned alignPad = TARGET_DEFAULT_ALIGN;

    // Preallocate enough temp memory to prevent reallocations
    const int nStep = (1 << pNode->m_QueuedLOD) * CTerrain::GetHeightMapUnitSize();
    const int nSectorSize = CTerrain::GetSectorSize() << pNode->m_nTreeLevel;

    int nNumVerts = (nStep) ? ((nSectorSize / nStep) + 1) * ((nSectorSize / nStep) + 1) : 0;
    int nNumIdx = (nStep) ? (nSectorSize / nStep) * (nSectorSize / nStep) * 6 : 0;

    if (nNumVerts == 0 || nNumIdx == 0)
    {
        return true;
    }

    size_t allocSize = sizeof(BuildMeshData);
    allocSize += alignPad;

    allocSize += sizeof(vtx_idx) * nNumIdx;
    allocSize += alignPad;

    allocSize += sizeof(SVF_P2S_N4B_C4B_T1F) * nNumVerts;

    uint8* pTempData = (uint8*)m_pHeap->Memalign(__alignof(BuildMeshData), allocSize, "");
    if (pTempData)
    {
        BuildMeshData* meshData = new (pTempData)BuildMeshData();
        pTempData += sizeof(BuildMeshData);
        pTempData = (uint8*)((reinterpret_cast<uintptr_t>(pTempData) + alignPad) & ~uintptr_t(alignPad));

        meshData->m_Indices.SetMemory(reinterpret_cast<vtx_idx*>(pTempData), nNumIdx);
        pTempData += sizeof(vtx_idx) * nNumIdx;
        pTempData = (uint8*)((reinterpret_cast<uintptr_t>(pTempData) + alignPad) & ~uintptr_t(alignPad));

        meshData->m_Vertices.SetMemory(reinterpret_cast<SVF_P2S_N4B_C4B_T1F*>(pTempData), nNumVerts);

        pNode->m_MeshData = meshData;

        PodArray<CTerrainNode*>& lstNeighbourSectors = leafData.m_Neighbors;
        PodArray<uint8>& lstNeighbourLods = leafData.m_NeighborLods;
        lstNeighbourSectors.reserve(64U);
        lstNeighbourLods.PreAllocate(64U, 64U);

        // dont run async in case of editor or if we render into shadowmap
        executeAsJob &= !gEnv->IsEditor();
        executeAsJob &= !passInfo.IsShadowPass();

        if (executeAsJob)
        {
            ScopedSwitchToGlobalHeap useGlobalHeap;

            meshData->m_jobExecutor.Reset();
            meshData->m_jobExecutor.StartJob([pNode, passInfo]() { pNode->BuildIndices_Wrapper(passInfo); });
            meshData->m_jobExecutor.StartJob([pNode]() { pNode->BuildVertices_Wrapper(); });
        }
        else
        {
            pNode->BuildIndices_Wrapper(passInfo);
            pNode->BuildVertices_Wrapper();

            // Finish the render mesh update
            pNode->RenderSectorUpdate_Finish(passInfo);
            meshData->~BuildMeshData();
            m_pHeap->Free(meshData);
            pNode->m_MeshData = NULL;
            return true;
        }

        m_arrRunningJobs.Add(pNode);
        return true;
    }

    return false;
}


void CTerrainUpdateDispatcher::SyncAllJobs(bool bForceAll, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    uint32 nNothingQueued = 0;
    do
    {
        bool bQueued = m_queuedJobs.size() ? false : true;
        size_t i = 0, nEnd = m_queuedJobs.size();
        while (i < nEnd)
        {
            CTerrainNode* pNode = m_queuedJobs[i];
            if (AddJob(pNode, false, passInfo))
            {
                bQueued = true;
                m_queuedJobs[i] = m_queuedJobs.Last();
                m_queuedJobs.DeleteLast();
                --nEnd;
                continue;
            }
            ++i;
        }

        i = 0;
        nEnd = m_arrRunningJobs.size();
        while (i < nEnd)
        {
            CTerrainNode* pNode = m_arrRunningJobs[i];
            BuildMeshData* pTempStorage = pNode->m_MeshData;
            assert(pTempStorage);
            PREFAST_ASSUME(pTempStorage);

            if (!pTempStorage->m_jobExecutor.IsRunning())
            {
                // Finish the render mesh update
                pNode->RenderSectorUpdate_Finish(passInfo);
                pTempStorage->~BuildMeshData();
                m_pHeap->Free(pTempStorage);
                pNode->m_MeshData = NULL;

                // Remove from running list
                m_arrRunningJobs[i] = m_arrRunningJobs.Last();
                m_arrRunningJobs.DeleteLast();
                --nEnd;
                continue;
            }
            ++i;
        }

        if (m_arrRunningJobs.size() == 0 && !bQueued)
        {
            ++nNothingQueued;
        }
        if (!bForceAll && nNothingQueued > 4)
        {
            CryLogAlways("ERROR: not all terrain sector vertex/index update requests could be scheduled");
            break;
        }
    } while (m_queuedJobs.size() != 0 || m_arrRunningJobs.size() != 0);
}

void CTerrainUpdateDispatcher::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_pHeapStorage, TempPoolSize);
    pSizer->AddObject(m_arrRunningJobs);
    pSizer->AddObject(m_queuedJobs);
}

void CTerrainUpdateDispatcher::RemoveJob(CTerrainNode* pNode)
{
    int index = m_arrRunningJobs.Find(pNode);
    if (index >= 0)
    {
        m_arrRunningJobs.Delete(index);
        return;
    }

    index = m_queuedJobs.Find(pNode);
    if (index >= 0)
    {
        m_queuedJobs.Delete(index);
        return;
    }
}

PodArray<vtx_idx> CTerrainNode::m_SurfaceIndices[SurfaceTile::MaxSurfaceCount][4];

void CTerrainNode::ResetStaticData()
{
    for (int s = 0; s < SurfaceTile::MaxSurfaceCount; s++)
    {
        for (int p = 0; p < 4; p++)
        {
            stl::free_container(m_SurfaceIndices[s][p]);
        }
    }
}

void CTerrainNode::GetStaticMemoryUsage(ICrySizer* sizer)
{
    SIZER_COMPONENT_NAME(sizer, "StaticIndices");
    for (int i = 0; i < SurfaceTile::MaxSurfaceCount; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            sizer->AddObject(CTerrainNode::m_SurfaceIndices[i][j]);
        }
    }
}

void CTerrainNode::SetupTexGenParams(SSurfaceType* pSurfaceType, float* pOutParams, uint8 ucProjAxis, bool bOutdoor, float fTexGenScale)
{
    assert(pSurfaceType);

    if (pSurfaceType->fCustomMaxDistance > 0)
    {
        pSurfaceType->fMaxMatDistanceZ = pSurfaceType->fMaxMatDistanceXY = pSurfaceType->fCustomMaxDistance;
    }
    else
    {
        pSurfaceType->fMaxMatDistanceZ = float(GetFloatCVar(e_TerrainDetailMaterialsViewDistZ)  * Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);
        pSurfaceType->fMaxMatDistanceXY = float(GetFloatCVar(e_TerrainDetailMaterialsViewDistXY) * Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);
    }

    // setup projection direction
    if (ucProjAxis == 'X')
    {
        pOutParams[0] = 0;
        pOutParams[1] = pSurfaceType->fScale * fTexGenScale;
        pOutParams[2] = 0;
        pOutParams[3] = pSurfaceType->fMaxMatDistanceXY;

        pOutParams[4] = 0;
        pOutParams[5] = 0;
        pOutParams[6] = -pSurfaceType->fScale * fTexGenScale;
        pOutParams[7] = 0;
    }
    else if (ucProjAxis == 'Y')
    {
        pOutParams[0] = pSurfaceType->fScale * fTexGenScale;
        pOutParams[1] = 0;
        pOutParams[2] = 0;
        pOutParams[3] = pSurfaceType->fMaxMatDistanceXY;

        pOutParams[4] = 0;
        pOutParams[5] = 0;
        pOutParams[6] = -pSurfaceType->fScale * fTexGenScale;
        pOutParams[7] = 0;
    }
    else // Z
    {
        pOutParams[0] = pSurfaceType->fScale * fTexGenScale;
        pOutParams[1] = 0;
        pOutParams[2] = 0;
        pOutParams[3] = pSurfaceType->fMaxMatDistanceZ;

        pOutParams[4] = 0;
        pOutParams[5] = -pSurfaceType->fScale * fTexGenScale;
        pOutParams[6] = 0;
        pOutParams[7] = 0;
    }
}

void CTerrainNode::DrawArray(const SRenderingPassInfo& passInfo)
{
    SetupTexturing(passInfo);

    _smart_ptr<IRenderMesh>& pRenderMesh = GetLeafData()->m_pRenderMesh;

    // make renderobject
    CRenderObject* pTerrainRenderObject = GetIdentityCRenderObject(passInfo.ThreadID());
    if (!pTerrainRenderObject)
    {
        return;
    }

    pTerrainRenderObject->m_pRenderNode = 0;
    pTerrainRenderObject->m_fDistance = m_DistanceToCamera[passInfo.GetRecursiveLevel()];

    pTerrainRenderObject->m_II.m_Matrix.SetIdentity();
    Vec3 vOrigin(m_nOriginX, m_nOriginY, 0);
    pTerrainRenderObject->m_II.m_Matrix.SetTranslation(vOrigin);

    pRenderMesh->AddRenderElements(GetTerrain()->m_pTerrainEf, pTerrainRenderObject, passInfo, EFSLIST_GENERAL, 1);

    if (passInfo.RenderTerrainDetailMaterial() && !passInfo.IsShadowPass())
    {
        CRenderObject* pDetailObj = NULL;

        for (int i = 0; i < m_DetailLayers.Count(); i++)
        {
            if (!m_DetailLayers[i].surfaceType->HasMaterial() || !m_DetailLayers[i].HasRM())
            {
                continue;
            }

            if (!pDetailObj)
            {
                pDetailObj = GetIdentityCRenderObject(passInfo.ThreadID());
                if (!pDetailObj)
                {
                    return;
                }
                pDetailObj->m_pRenderNode = 0;
                pDetailObj->m_ObjFlags |= (((passInfo.IsGeneralPass()) ? FOB_NO_FOG : 0)); // enable fog on recursive rendering (per-vertex)
                pDetailObj->m_fDistance = m_DistanceToCamera[passInfo.GetRecursiveLevel()];
                pDetailObj->m_II.m_Matrix = pTerrainRenderObject->m_II.m_Matrix;
            }

            uint8 szProj[] = "XYZ";
            for (int p = 0; p < 3; p++)
            {
                if (SSurfaceType* pSurf = m_DetailLayers[i].surfaceType)
                {
                    if (_smart_ptr<IMaterial> pMat = pSurf->GetMaterialOfProjection(szProj[p]))
                    {
                        pSurf->fMaxMatDistanceZ = float(GetFloatCVar(e_TerrainDetailMaterialsViewDistZ) * Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);
                        pSurf->fMaxMatDistanceXY = float(GetFloatCVar(e_TerrainDetailMaterialsViewDistXY) * Get3DEngine()->m_fTerrainDetailMaterialsViewDistRatio);

                        if (m_DistanceToCamera[passInfo.GetRecursiveLevel()] < pSurf->GetMaxMaterialDistanceOfProjection(szProj[p]))
                        {
                            if (IRenderMesh* pMesh = m_DetailLayers[i].triplanarMeshes[p])
                            {
                                AABB aabb;
                                pMesh->GetBBox(aabb.min, aabb.max);
                                if (passInfo.GetCamera().IsAABBVisible_F(aabb))
                                {
                                    if (pMesh->GetVertexContainer() == pRenderMesh && pMesh->GetIndicesCount())
                                    {
                                        if (GetCVars()->e_TerrainBBoxes == 2)
                                        {
                                            GetRenderer()->GetIRenderAuxGeom()->DrawAABB(aabb, false, ColorB(255 * ((m_nTreeLevel & 1) > 0), 255 * ((m_nTreeLevel & 2) > 0), 0, 255), eBBD_Faceted);
                                        }

                                        if (pMat && pMat->GetShaderItem().m_pShader)
                                        {
                                            bool    isTerrainType = (pMat->GetShaderItem().m_pShader->GetShaderType() == eST_Terrain);
                                            if (isTerrainType)
                                            {
                                                pMesh->AddRenderElements(pMat, pDetailObj, passInfo, EFSLIST_TERRAINLAYER, 1);
                                            }
                                            else
                                            {
                                                gEnv->pLog->LogError("Terrain Layer - Incorrect Terrain Layer shader type - [%d] found in frame [%d]",
                                                    pMat->GetShaderItem().m_pShader->GetShaderType(), gEnv->pRenderer->GetFrameID());
                                            }
                                        }
                                        else
                                        {
                                            gEnv->pLog->LogError("Terrain Layer - Unassigned material or shader in frame [%d]", 
                                                gEnv->pRenderer->GetFrameID());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CTerrainNode::ReleaseHeightMapGeometry(bool bRecursive, const AABB* pBox)
{
    if (pBox && !Overlap::AABB_AABB(*pBox, GetBBox()))
    {
        return;
    }

    for (int i = 0; i < m_DetailLayers.Count(); i++)
    {
        m_DetailLayers[i].DeleteRenderMeshes(GetRenderer());
    }

    if (GetLeafData() && GetLeafData()->m_pRenderMesh)
    {
        _smart_ptr<IRenderMesh>& pRenderMesh = GetLeafData()->m_pRenderMesh;
        pRenderMesh = NULL;

        if (GetCVars()->e_TerrainLog == 1)
        {
            PrintMessage("RenderMesh unloaded %d", GetSecIndex());
        }
    }

    delete m_pLeafData;
    m_pLeafData = NULL;

    if (bRecursive && m_Children)
    {
        for (int i = 0; i < 4; i++)
        {
            m_Children[i].ReleaseHeightMapGeometry(bRecursive, pBox);
        }
    }
}

void CTerrainNode::ResetHeightMapGeometry(bool bRecursive, const AABB* pBox)
{
    if (pBox && !Overlap::AABB_AABB(*pBox, GetBBox()))
    {
        return;
    }

    if (m_pLeafData)
    {
        m_pLeafData->m_Neighbors.Clear();
    }

    if (bRecursive && m_Children)
    {
        for (int i = 0; i < 4; i++)
        {
            m_Children[i].ResetHeightMapGeometry(bRecursive, pBox);
        }
    }
}

// fill vertex buffer
void CTerrainNode::BuildVertices(int nStep)
{
    FUNCTION_PROFILER_3DENGINE;

    m_MeshData->m_Vertices.Clear();

    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;

    // keep often used variables on stack
    const int nOriginX = m_nOriginX;
    const int nOriginY = m_nOriginY;
    const int nTerrainSize = CTerrain::GetTerrainSize();
    const int iLookupRadius = 2 * CTerrain::GetHeightMapUnitSize();
    CTerrain*      pTerrain = GetTerrain();

    for (int x = nOriginX; x <= nOriginX + nSectorSize; x += nStep)
    {
        for (int y = nOriginY; y <= nOriginY + nSectorSize; y += nStep)
        {
            int _x = CLAMP(x, nOriginX, nOriginX + nSectorSize);
            int _y = CLAMP(y, nOriginY, nOriginY + nSectorSize);
            float _z = pTerrain->GetZ(_x, _y);

            SVF_P2S_N4B_C4B_T1F vert;

            vert.xy = CryHalf2((float)(_x - nOriginX), (float)(_y - nOriginY));
            vert.z = _z;

            // calculate surface normal
            float sx;
            if ((x + iLookupRadius) < nTerrainSize && x > iLookupRadius)
            {
                sx = pTerrain->GetZ(x + iLookupRadius, y) - pTerrain->GetZ(x - iLookupRadius, y);
            }
            else
            {
                sx = 0;
            }

            float sy;
            if ((y + iLookupRadius) < nTerrainSize && y > iLookupRadius)
            {
                sy = pTerrain->GetZ(x, y + iLookupRadius) - pTerrain->GetZ(x, y - iLookupRadius);
            }
            else
            {
                sy = 0;
            }

            Vec3 normal(-sx, -sy, iLookupRadius * 2.0f);
            normal.Normalize();

            ITerrain::SurfaceWeight weight = pTerrain->GetSurfaceWeight(x, y);
            if (weight.PrimaryId() == ITerrain::SurfaceWeight::Hole)
            { // in case of hole - try to find some valid surface type around
                for (int i = -nStep; i <= nStep && (weight.PrimaryId() == ITerrain::SurfaceWeight::Hole); i += nStep)
                {
                    for (int j = -nStep; j <= nStep && (weight.PrimaryId() == ITerrain::SurfaceWeight::Hole); j += nStep)
                    {
                        weight = pTerrain->GetSurfaceWeight(x + i, y + j);
                    }
                }
            }

            // Pack surface ids and first weight into color components
            vert.color.bcolor[0] = weight.Ids[0];
            vert.color.bcolor[1] = weight.Ids[1];
            vert.color.bcolor[2] = weight.Ids[2];
            vert.color.bcolor[3] = weight.Weights[0];
            SwapEndian(vert.color.dcolor, eLittleEndian);

            // Pack normal and second weight into color components.
            vert.normal.bcolor[0] = (byte)(normal[0] * 127.5f + 128.0f);
            vert.normal.bcolor[1] = (byte)(normal[1] * 127.5f + 128.0f);
            vert.normal.bcolor[2] = (byte)(normal[2] * 127.5f + 128.0f);
            vert.normal.bcolor[3] = weight.Weights[1];
            SwapEndian(vert.normal.dcolor, eLittleEndian);

            m_MeshData->m_Vertices.Add(vert);
        }
    }
}

namespace Util
{
    void AddNeighbourNode(PodArray<CTerrainNode*>* plstNeighbourSectors, CTerrainNode* pNode)
    {
        // todo: cache this list, it is always the same
        if (pNode && plstNeighbourSectors->Find(pNode) < 0)
        {
            plstNeighbourSectors->Add(pNode);
        }
    }
}

void CTerrainNode::AddIndexAliased(int _x, int _y, int _step, int nSectorSize, PodArray<CTerrainNode*>* plstNeighbourSectors, InPlaceIndexBuffer& indices, const SRenderingPassInfo& passInfo)
{
    int nAliasingX = 1, nAliasingY = 1;
    int nShiftX = 0, nShiftY = 0;

    CTerrain* pTerrain = GetTerrain();
    int nHeightMapUnitSize = CTerrain::GetHeightMapUnitSize();

    IF(_x && _x < nSectorSize && plstNeighbourSectors, true)
    {
        IF(_y == 0, false)
        {
            if (CTerrainNode* pNode = pTerrain->GetLeafNodeAt(m_nOriginX + _x, m_nOriginY + _y - _step))
            {
                int nAreaMML = pNode->GetAreaLOD(passInfo);
                if (nAreaMML != MML_NOT_SET)
                {
                    nAliasingX = nHeightMapUnitSize * (1 << nAreaMML);
                    nShiftX = nAliasingX / 4;
                }
                Util::AddNeighbourNode(plstNeighbourSectors, pNode);
            }
        }
        else IF(_y == nSectorSize, false)
        {
            if (CTerrainNode* pNode = pTerrain->GetLeafNodeAt(m_nOriginX + _x, m_nOriginY + _y + _step))
            {
                int nAreaMML = pNode->GetAreaLOD(passInfo);
                if (nAreaMML != MML_NOT_SET)
                {
                    nAliasingX = nHeightMapUnitSize * (1 << nAreaMML);
                    nShiftX = nAliasingX / 2;
                }
                Util::AddNeighbourNode(plstNeighbourSectors, pNode);
            }
        }
    }

    IF(_y && _y < nSectorSize && plstNeighbourSectors, true)
    {
        IF(_x == 0, false)
        {
            if (CTerrainNode* pNode = pTerrain->GetLeafNodeAt(m_nOriginX + _x - _step, m_nOriginY + _y))
            {
                int nAreaMML = pNode->GetAreaLOD(passInfo);
                if (nAreaMML != MML_NOT_SET)
                {
                    nAliasingY = nHeightMapUnitSize * (1 << nAreaMML);
                    nShiftY = nAliasingY / 4;
                }
                Util::AddNeighbourNode(plstNeighbourSectors, pNode);
            }
        }
        else IF(_x == nSectorSize, false)
        {
            if (CTerrainNode* pNode = pTerrain->GetLeafNodeAt(m_nOriginX + _x + _step, m_nOriginY + _y))
            {
                int nAreaMML = pNode->GetAreaLOD(passInfo);
                if (nAreaMML != MML_NOT_SET)
                {
                    nAliasingY = nHeightMapUnitSize * (1 << nAreaMML);
                    nShiftY = nAliasingY / 2;
                }
                Util::AddNeighbourNode(plstNeighbourSectors, pNode);
            }
        }
    }

    int XA = nAliasingX ? ((_x + nShiftX) / nAliasingX) * nAliasingX : _x;
    int YA = nAliasingY ? ((_y + nShiftY) / nAliasingY) * nAliasingY : _y;

    assert(XA >= 0 && XA <= nSectorSize);
    assert(YA >= 0 && YA <= nSectorSize);

    indices.AddIndex(XA, YA, _step, nSectorSize);
}


void CTerrainNode::BuildIndices(InPlaceIndexBuffer& indices, PodArray<CTerrainNode*>* pNeighbourSectors, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    // 1<<MML_NOT_SET will generate 0;
    if (m_QueuedLOD == MML_NOT_SET)
    {
        return;
    }

    int nStep = (1 << m_QueuedLOD) * CTerrain::GetHeightMapUnitSize();
    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;

    indices.Clear();

    CTerrain* pTerrain = GetTerrain();

    int nHalfStep = nStep / 2;

    // add non borders
    for (int x = 0; x < nSectorSize; x += nStep)
    {
        for (int y = 0; y < nSectorSize; y += nStep)
        {
            if (!m_bHasHoles || !pTerrain->IsHole(m_nOriginX + x + nHalfStep, m_nOriginY + y + nHalfStep))
            {
                if (pTerrain->IsMeshQuadFlipped(m_nOriginX + x, m_nOriginY + y, nStep))
                {
                    AddIndexAliased(x + nStep, y, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x + nStep, y + nStep, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x, y, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);

                    AddIndexAliased(x, y, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x + nStep, y + nStep, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x, y + nStep, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                }
                else
                {
                    AddIndexAliased(x, y, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x + nStep, y, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x, y + nStep, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);

                    AddIndexAliased(x + nStep, y, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x + nStep, y + nStep, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                    AddIndexAliased(x, y + nStep, nStep, nSectorSize, pNeighbourSectors, indices, passInfo);
                }
            }
        }
    }
}

// entry point
bool CTerrainNode::RenderSector(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;
    STerrainNodeLeafData* pLeafData = GetLeafData();

    if (!pLeafData)
    {
        return false;
    }

    // detect if any neighbors switched lod since previous frame
    bool bNeighbourChanged = false;

    if (!passInfo.IsShadowPass())
    {
        for (int i = 0; i < pLeafData->m_Neighbors.Count(); i++)
        {
            if (!pLeafData->m_Neighbors[i])
            {
                continue;
            }
            int nNeighbourNewMML = pLeafData->m_Neighbors[i]->GetAreaLOD(passInfo);
            if (nNeighbourNewMML == MML_NOT_SET)
            {
                continue;
            }
            if (nNeighbourNewMML != pLeafData->m_NeighborLods[i] && (nNeighbourNewMML > m_QueuedLOD || pLeafData->m_NeighborLods[i] > m_QueuedLOD))
            {
                bNeighbourChanged = true;
                break;
            }
        }
    }

    _smart_ptr<IRenderMesh>& pRenderMesh = GetLeafData()->m_pRenderMesh;

    bool bDetailLayersReady = passInfo.IsShadowPass() ||
        !m_DetailLayers.Count() ||
        !passInfo.RenderTerrainDetailMaterial();
    for (int i = 0; i < m_DetailLayers.Count() && !bDetailLayersReady; ++i)
    {
        bDetailLayersReady =
            m_DetailLayers[i].HasRM() ||
            m_DetailLayers[i].surfaceType ||
            !m_DetailLayers[i].surfaceType->HasMaterial();
    }

    if (pRenderMesh && GetCVars()->e_TerrainDrawThisSectorOnly < 2 && bDetailLayersReady)
    {
        if (passInfo.GetRecursiveLevel() || (m_CurrentLOD == m_QueuedLOD && !bNeighbourChanged))
        {
            DrawArray(passInfo);
            return true;
        }
    }

    if (passInfo.GetRecursiveLevel())
    {
        if (pRenderMesh)
        {
            return true;
        }
    }

    return false;
}
void CTerrainNode::RenderSectorUpdate_Finish(const SRenderingPassInfo& passInfo)
{
    assert(m_MeshData != NULL);
    if (passInfo.IsGeneralPass())
    {
        FRAME_PROFILER("Sync_UpdateTerrain", GetSystem(), PROFILE_3DENGINE);
        m_MeshData->m_jobExecutor.WaitForCompletion();
    }

    m_CurrentLOD = m_QueuedLOD;

    // This can legally be zero if we have a bunch of holes in the terrain that end up
    // resolving to a blank patch at lower LODs.
    if (m_MeshData->m_Indices.Count() == 0)
    {
        return;
    }

    STerrainNodeLeafData* pLeafData = GetLeafData();
    if (!pLeafData)
    {
        return;
    }

    _smart_ptr<IRenderMesh>& pRenderMesh = GetLeafData()->m_pRenderMesh;
    assert(m_MeshData->m_Vertices.Count() < 65536);

    pRenderMesh = GetRenderer()->CreateRenderMeshInitialized(
        m_MeshData->m_Vertices.GetElements(), m_MeshData->m_Vertices.Count(),
        eVF_P2S_N4B_C4B_T1F,
        m_MeshData->m_Indices.GetElements(), m_MeshData->m_Indices.Count(),
        prtTriangleList,
        "TerrainSector", "TerrainSector",
        eRMT_Dynamic, 1, 0, NULL, NULL, false, true, nullptr);

    AABB boxWS = GetBBox();
    pRenderMesh->SetBBox(boxWS.min, boxWS.max);

    if (GetCVars()->e_TerrainLog == 1)
    {
        PrintMessage("RenderMesh created %d", GetSecIndex());
    }

    pRenderMesh->SetChunk(GetTerrain()->m_pTerrainEf, 0, pRenderMesh->GetVerticesCount(), 0, m_MeshData->m_Indices.Count(), 1.0f, eVF_P2S_N4B_C4B_T1F, 0);

    // update detail layers indices
    if (passInfo.RenderTerrainDetailMaterial())
    {
        // build all indices
        GenerateIndicesForAllSurfaces(pRenderMesh, pLeafData->m_SurfaceAxisIndexCount, m_MeshData);

        uint8 szProj[] = "XYZ";
        for (int i = 0; i < m_DetailLayers.Count(); i++)
        {
            SSurfaceType* surfaceType = m_DetailLayers[i].surfaceType;

            if (surfaceType && surfaceType->ucThisSurfaceTypeId < ITerrain::SurfaceWeight::Undefined)
            {
                bool b3D = surfaceType->IsMaterial3D();
                for (int p = 0; p < 3; p++)
                {
                    if (b3D || surfaceType->GetMaterialOfProjection(szProj[p]))
                    {
                        int nProjId = b3D ? p : 3;
                        PodArray<vtx_idx>& lstIndices = m_SurfaceIndices[surfaceType->ucThisSurfaceTypeId][nProjId];

                        if (m_DetailLayers[i].triplanarMeshes[p] && (lstIndices.Count() != m_DetailLayers[i].triplanarMeshes[p]->GetIndicesCount()))
                        {
                            m_DetailLayers[i].triplanarMeshes[p] = NULL;
                        }

                        if (lstIndices.Count())
                        {
                            UpdateSurfaceRenderMeshes(pRenderMesh, surfaceType, m_DetailLayers[i].triplanarMeshes[p], p, lstIndices, "TerrainMaterialLayer", pLeafData->m_SurfaceAxisIndexCount[surfaceType->ucThisSurfaceTypeId][nProjId], passInfo);
                        }

                        if (!b3D)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    DrawArray(passInfo);
}

void CTerrainNode::BuildIndices_Wrapper(const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    BuildMeshData* meshData = m_MeshData;
    if (meshData->m_Indices.MemorySize() == 0)
    {
        return;
    }

    STerrainNodeLeafData* pLeafData = GetLeafData();
    PodArray<CTerrainNode*>& neighbors = pLeafData->m_Neighbors;
    PodArray<uint8>& neighborLods = pLeafData->m_NeighborLods;
    neighbors.Clear();
    BuildIndices(meshData->m_Indices, &neighbors, passInfo);

    if (neighbors.Count() > neighborLods.Count())
    {
        neighborLods.resize(neighbors.Count());
    }

    // remember neighbor LOD's
    for (int i = 0; i < neighbors.Count(); i++)
    {
        int nNeighbourMML = neighbors[i]->GetAreaLOD(passInfo);
        assert(0 <= nNeighbourMML && nNeighbourMML <= ((uint8)-1));
        neighborLods[i] = nNeighbourMML;
    }
}

void CTerrainNode::BuildVertices_Wrapper()
{
    AZ_TRACE_METHOD();
    // don't try to create terrain data if an allocation failed
    if (m_MeshData->m_Vertices.MemorySize() == 0)
    {
        return;
    }

    STerrainNodeLeafData* pLeafData = GetLeafData();
    if (!pLeafData)
    {
        return;
    }

    _smart_ptr<IRenderMesh>& pRenderMesh = pLeafData->m_pRenderMesh;

    // 1U<<MML_NOT_SET will generate zero
    if (m_QueuedLOD == MML_NOT_SET)
    {
        return;
    }

    int nStep = (1 << m_QueuedLOD) * CTerrain::GetHeightMapUnitSize();
    int nSectorSize = CTerrain::GetSectorSize() << m_nTreeLevel;
    assert(nStep && nStep <= nSectorSize);
    if (nStep > nSectorSize)
    {
        nStep = nSectorSize;
    }

    BuildVertices(nStep);
}

static int GetVecProjectId(const Vec3& vNorm)
{
    Vec3 vNormAbs = vNorm.abs();

    int nOpenId = 0;

    if (vNormAbs.x >= vNormAbs.y && vNormAbs.x >= vNormAbs.z)
    {
        nOpenId = 0;
    }
    else if (vNormAbs.y >= vNormAbs.x && vNormAbs.y >= vNormAbs.z)
    {
        nOpenId = 1;
    }
    else
    {
        nOpenId = 2;
    }

    return nOpenId;
}

void CTerrainNode::GenerateIndicesForAllSurfaces(IRenderMesh* mesh, int surfaceAxisIndexCount[SurfaceTile::MaxSurfaceCount][4], BuildMeshData* meshData)
{
    FUNCTION_PROFILER_3DENGINE;

    // Used to quickly iterate unique surfaces.
    static uint8 SurfaceCounts[SurfaceTile::MaxSurfaceCount] = { 0 };

    bool bSurfaceIs3D[SurfaceTile::MaxSurfaceCount];

    CTerrain& terrain = *GetTerrain();
    for (int surfaceId = 0; surfaceId < SurfaceTile::MaxSurfaceCount; surfaceId++)
    {
        for (int axis = 0; axis < 4; axis++)
        {
            m_SurfaceIndices[surfaceId][axis].Clear();
            surfaceAxisIndexCount[surfaceId][axis] = -1;
        }

        bSurfaceIs3D[surfaceId] = terrain.GetSurfaceTypes()[surfaceId].IsMaterial3D();
    }

    vtx_idx* sourceIndices = meshData->m_Indices.GetElements();
    int sourceIndexCount = meshData->m_Indices.Count();
    assert(sourceIndices && sourceIndexCount);

    byte* positions = reinterpret_cast<byte*>(meshData->m_Vertices.GetElements());

    int colorStride = sizeof(SVF_P2S_N4B_C4B_T1F);
    byte* colors = positions + offsetof(SVF_P2S_N4B_C4B_T1F, color.dcolor);

    int normalStride = sizeof(SVF_P2S_N4B_C4B_T1F);
    byte* normals = positions + offsetof(SVF_P2S_N4B_C4B_T1F, normal);

    int vertexCount = mesh->GetVerticesCount();

    for (int j = 0; j < sourceIndexCount; j += 3)
    {
        vtx_idx triangle[3] = { sourceIndices[j + 0], sourceIndices[j + 1], sourceIndices[j + 2] };

        // Ignore degenerate triangles
        if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[2] == triangle[0])
        {
            continue;
        }

        assert(triangle[0] < (unsigned)vertexCount && triangle[1] < (unsigned)vertexCount && triangle[2] < (unsigned)vertexCount);
        UCol& Color0 = *(UCol*)&colors[triangle[0] * colorStride];
        UCol& Color1 = *(UCol*)&colors[triangle[1] * colorStride];
        UCol& Color2 = *(UCol*)&colors[triangle[2] * colorStride];

        //
        // Assume every triangle can have all 9 surface ids present.
        //
        const int SurfaceIdCount = 9;
        const uint8 SurfaceIds[] =
        {
            Color0.r,
            Color0.g,
            Color0.b,

            Color1.r,
            Color1.g,
            Color1.b,

            Color2.r,
            Color2.g,
            Color2.b
        };

        for (int i = 0; i < SurfaceIdCount; ++i)
        {
            uint8 surfaceId = SurfaceIds[i];

            if (surfaceId == ITerrain::SurfaceWeight::Undefined)
            {
                continue;
            }

            // Ignore surfaces we already processed. Gate with table counter.
            if (SurfaceCounts[surfaceId]++ != 0)
            {
                continue;
            }

            int projectionAxis = 3;
            if (bSurfaceIs3D[surfaceId])
            {
                byte* normal = &normals[triangle[0] * normalStride];
                projectionAxis = GetVecProjectId(Vec3(
                    ((float)normal[0] - 127.5f),
                    ((float)normal[1] - 127.5f),
                    ((float)normal[2] - 127.5f)));
            }

            assert(projectionAxis >= 0 && projectionAxis < 4);
            m_SurfaceIndices[surfaceId][projectionAxis].AddList(triangle, 3);
        }

        // Reset counters for next triangle
        for (int i = 0; i < SurfaceIdCount; ++i)
        {
            SurfaceCounts[SurfaceIds[i]] = 0;
        }
    }
}

void CTerrainNode::UpdateSurfaceRenderMeshes(
    const _smart_ptr<IRenderMesh> pSrcRM,
    SSurfaceType* pSurface,
    _smart_ptr<IRenderMesh>& pMatRM,
    int nProjectionId,
    PodArray<vtx_idx>& lstIndices,
    const char* szComment,
    int nNonBorderIndicesCount,
    const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;

    ERenderMeshType eRMType = eRMT_Dynamic;

    // force new rendermesh if vertex container has changed. Vertex containers aren't thread
    // safe, but seems like maybe something relies on this behaviour since fixing it on RM side
    // causes flickering.
    if (!pMatRM || (pMatRM && pMatRM->GetVertexContainer() != pSrcRM))
    {
        pMatRM = GetRenderer()->CreateRenderMeshInitialized(
            NULL, 0, eVF_P2S_N4B_C4B_T1F, NULL, 0,
            prtTriangleList, szComment, szComment, eRMType, 1, 0, NULL, NULL, false, false);
    }

    uint8 szProj[] = "XYZ";

    pMatRM->LockForThreadAccess();
    pMatRM->SetVertexContainer(pSrcRM);

    assert(1 || nNonBorderIndicesCount >= 0);

    pMatRM->UpdateIndices(lstIndices.GetElements(), lstIndices.Count(), 0, 0u);
    pMatRM->SetChunk(pSurface->GetMaterialOfProjection(szProj[nProjectionId]), 0, pSrcRM->GetVerticesCount(), 0, lstIndices.Count(), 1.0f, eVF_P2S_N4B_C4B_T1F);

    assert(nProjectionId >= 0 && nProjectionId < 3);
    float* pParams = pSurface->arrRECustomData[nProjectionId];
    SetupTexGenParams(pSurface, pParams, szProj[nProjectionId], true);

    // set surface type
    if (pSurface->IsMaterial3D())
    {
        pParams[8] = (nProjectionId == 0);
        pParams[9] = (nProjectionId == 1);
        pParams[10] = (nProjectionId == 2);
    }
    else
    {
        pParams[8] = 1.f;
        pParams[9] = 1.f;
        pParams[10] = 1.f;
    }

    pParams[11] = pSurface->ucThisSurfaceTypeId;

    // set texgen offset
    Vec3 vCamPos = passInfo.GetCamera().GetPosition();

    pParams[12] = pParams[13] = pParams[14] = pParams[15] = 0;

    // for diffuse
    if (_smart_ptr<IMaterial> pMat = pSurface->GetMaterialOfProjection(szProj[nProjectionId]))
    {
        if (pMat->GetShaderItem().m_pShaderResources)
        {
            if (SEfResTexture* pTex = pMat->GetShaderItem().m_pShaderResources->GetTextureResource(EFTT_DIFFUSE))
            {
                float fScaleX = pTex->m_bUTile ? pTex->GetTiling(0) * pSurface->fScale : 1.f;
                float fScaleY = pTex->m_bVTile ? pTex->GetTiling(1) * pSurface->fScale : 1.f;

                pParams[12] = int(vCamPos.x * fScaleX) / fScaleX;
                pParams[13] = int(vCamPos.y * fScaleY) / fScaleY;
            }

            if (SEfResTexture* pTex = pMat->GetShaderItem().m_pShaderResources->GetTextureResource(EFTT_NORMALS))
            {
                float fScaleX = pTex->m_bUTile ? pTex->GetTiling(0) * pSurface->fScale : 1.f;
                float fScaleY = pTex->m_bVTile ? pTex->GetTiling(1) * pSurface->fScale : 1.f;

                pParams[14] = int(vCamPos.x * fScaleX) / fScaleX;
                pParams[15] = int(vCamPos.y * fScaleY) / fScaleY;
            }
        }
    }

    if (pMatRM->GetChunks().size() && pMatRM->GetChunks()[0].pRE)
    {
        pMatRM->GetChunks()[0].pRE->m_CustomData = pParams;
        pMatRM->GetChunks()[0].pRE->mfUpdateFlags(FCEF_DIRTY);
    }

    Vec3 vMin, vMax;
    pSrcRM->GetBBox(vMin, vMax);
    pMatRM->SetBBox(vMin, vMax);
    pMatRM->UnLockForThreadAccess();
}

STerrainNodeLeafData::~STerrainNodeLeafData()
{
    m_pRenderMesh = NULL;
}

