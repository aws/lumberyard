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

#ifndef CRYINCLUDE_CRYANIMATION_MODELMESH_H
#define CRYINCLUDE_CRYANIMATION_MODELMESH_H
#pragma once

#include <CGFContent.h>
#include "Vertex/VertexData.h"

class CSkin;
class CCharInstance;
class CAttachmentSKIN;
class CryCHRLoader;

struct ClosestTri
{
    struct AttSkinVertex
    {
        Vec3 m_attTriPos;
        JointIdType m_attJointIDs[4];
        f32 m_attWeights[4];
        uint32 m_vertexIdx;
        AttSkinVertex()
        {
            m_attTriPos = Vec3(ZERO);
            m_attJointIDs[0] = 0, m_attJointIDs[1] = 0, m_attJointIDs[2] = 0, m_attJointIDs[3] = 0;
            m_attWeights[0] = 0, m_attWeights[1] = 0, m_attWeights[2] = 0, m_attWeights[3] = 0;
            m_vertexIdx = -1;
        }
    };
    AttSkinVertex v[3];
};

struct RChunk
{
    uint32 m_nFirstIndexId;
    uint32 m_nNumIndices;
    void GetMemoryUsage(ICrySizer* pSizer) const
    {
    }
};

struct MeshStreamInfo
{
    MeshStreamInfo()
    {
        memset(this, 0, sizeof(*this));
    }

    uint32 nRoundIds[MAX_STREAM_PREDICTION_ZONES];
    uint32 nFrameId;
    uint32 nKeepResidentRefs;
    bool bHasMeshFile;
    bool bIsUrgent;
    CryCHRLoader* pStreamer;
};

//////////////////////////////////////////////////////////////////////////
//      This is the class to manage the mesh on the CPU-side:           //
//////////////////////////////////////////////////////////////////////////
class CModelMesh
{
public:
    CModelMesh();
    void AbortStream();

    uint32 InitMesh(CMesh* pMesh, CNodeCGF* pGFXNode, _smart_ptr<IMaterial> pMaterial, CSkinningInfo* pSkinningInfo, const char* szFilePath, uint32 nLOD);
    _smart_ptr<IRenderMesh> InitRenderMeshAsync(CMesh* pMesh, const char* szFilePath, int nLod, DynArray<RChunk>& arrNewRenderChunks);
    uint32 InitRenderMeshSync(DynArray<RChunk>& arrNewRenderChunks, _smart_ptr<IRenderMesh> pNewRenderMesh);

    uint32 IsVBufferValid();
    ClosestTri GetAttachmentTriangle(const Vec3& RMWPosition, const JointIdType* const pRemapTable);

    uint32 InitSWSkinBuffer();
    void DrawDebugInfo(CDefaultSkeleton* pCSkel, int nLOD, const Matrix34& rRenderMat34, int DebugMode, const _smart_ptr<IMaterial>& pMaterial,  CRenderObject* pObj, const SRendParams& RendParams, bool isGeneralPass, IRenderNode* pRenderNode, const AABB& aabb);

#ifdef EDITOR_PCDEBUGCODE
    void ExportModel(IRenderMesh* pIRenderMesh);
    void DrawWireframeStatic(const Matrix34& m34, uint32 color);
#endif

    size_t SizeOfModelMesh() const;
    void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    void PrepareMesh(CMesh* pMesh);
    void PrepareRenderChunks(CMesh& mesh, DynArray<RChunk>& renderChunks);
    _smart_ptr<IRenderMesh> CreateRenderMesh(CMesh* pMesh, const char* szFilePath, int nLod, bool bCreateDeviceMesh);

public:
    //////////////////////////////////////////////////////////////////////////
    // Member variables.
    //////////////////////////////////////////////////////////////////////////
    _smart_ptr<IRenderMesh> m_pIRenderMesh;
    _smart_ptr<IMaterial> m_pIDefaultMaterial;      // Default material for this model.

    Vec3 m_vRenderMeshOffset;
    volatile int m_iThreadMeshAccessCounter;
    DynArray<RChunk>    m_arrRenderChunks;          //always initialized in LoaderCHR.cpp

    MeshStreamInfo m_stream;

    // used for geometric mean lod system
    float m_geometricMeanFaceArea;
    int m_faceCount;

    CSoftwareMesh m_softwareMesh;
};

#endif // CRYINCLUDE_CRYANIMATION_MODELMESH_H
