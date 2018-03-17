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

#ifndef CRYINCLUDE_CRY3DENGINE_DEFORMABLENODE_H
#define CRYINCLUDE_CRY3DENGINE_DEFORMABLENODE_H
#pragma once

#include <AzCore/Jobs/LegacyJobExecutor.h>

struct SDeformableData;
struct SMMRMProjectile;

#include <IDeformableNode.h>
    
class CDeformableNode 
    : public IDeformableNode
{
    SDeformableData** m_pData;
    size_t m_nData;
    uint32 m_nFrameId;
    Vec3* m_wind;
    primitives::sphere* m_Colliders;
    int m_nColliders;
    SMMRMProjectile* m_Projectiles;
    int m_nProjectiles;
    AABB m_bbox;
    IGeneralMemoryHeap* m_pHeap;
    std::vector<CRenderChunk> m_renderChunks;
    _smart_ptr<IRenderMesh> m_renderMesh;
    size_t m_numVertices, m_numIndices;
    IStatObj* m_pStatObj;
    AZ::LegacyJobExecutor m_cullCompletionDeformableNode;
    AZ::LegacyJobExecutor m_updateCompletionDeformableNode;
    bool m_all_prepared : 1;

protected:

    void UpdateInternalDeform(SDeformableData* pData
        , CRenderObject* pRenderObject, const AABB& bbox
        , const SRenderingPassInfo& passInfo
        , _smart_ptr<IRenderMesh>&
        , strided_pointer<SVF_P3S_C4B_T2S>
        , strided_pointer<SPipTangents>
        , strided_pointer<Vec3>
        , vtx_idx* idxBuf
        , size_t& iv
        , size_t& ii);

    void ClearInstanceData();

    void ClearSimulationData();

    void CreateInstanceData(SDeformableData* pData, IStatObj* pStatObj);

    void BakeInternal(SDeformableData* pData, const Matrix34& worldTM);


public:

    CDeformableNode();

    ~CDeformableNode() override;

    void SetStatObj(IStatObj* pStatObj) override;

    void CreateDeformableSubObject(bool create, const Matrix34& worldTM, IGeneralMemoryHeap* pHeap) override;
    void Render(const struct SRendParams& rParams, const SRenderingPassInfo& passInfo, const AABB& aabb) override;
    void RenderInternalDeform(CRenderObject* pRenderObject
        , int nLod, const AABB& bbox, const SRenderingPassInfo& passInfo
        , const SRendItemSorter& rendItemSorter) override;

    void BakeDeform(const Matrix34& worldTM) override;

    bool HasDeformableData() const override { return m_nData != 0; }
};


#endif // CRYINCLUDE_CRY3DENGINE_DEFORMABLENODE_H
