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

#ifndef CRYINCLUDE_CRY3DENGINE_ROADRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_ROADRENDERNODE_H
#pragma once

struct RoadRenderNodeCompileInfo;

class CRoadRenderNode
    : public IRoadRenderNode
    , public Cry3DEngineBase
{
public:
    CRoadRenderNode();
    virtual ~CRoadRenderNode();

    // IRoadRenderNode implementation
    virtual void SetVertices(const Vec3* pVerts, int nVertsNum, float fTexCoordBegin, float fTexCoordEnd, float fTexCoordBeginGlobal, float fTexCoordEndGlobal);
    virtual void SetVertices(const AZStd::vector<AZ::Vector3>& pVerts, const AZ::Transform& transform, float fTexCoordBegin, float fTexCoordEnd, float fTexCoordBeginGlobal, float fTexCoordEndGlobal);

    virtual void SetSortPriority(uint8 sortPrio);
    virtual void SetIgnoreTerrainHoles(bool bVal);
    virtual void SetPhysicalize(bool bVal)
    {
        if (m_bPhysicalize != bVal)
        {
            ScheduleRebuild();
            m_bPhysicalize = bVal;
        }
    }

    // IRenderNode implementation
    virtual const char* GetEntityClassName(void) const { return "RoadObjectClass"; }
    virtual const char* GetName(void) const { return "RoadObjectName"; }
    virtual Vec3 GetPos(bool) const;
    virtual void Render(const SRendParams& RendParams, const SRenderingPassInfo& passInfo);

    virtual IPhysicalEntity* GetPhysics(void) const { return m_pPhysEnt; }
    virtual void SetPhysics(IPhysicalEntity* pPhys) { m_pPhysEnt = pPhys; }

    void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual EERType GetRenderNodeType();
    virtual struct IRenderMesh* GetRenderMesh(int nLod) { return nLod == 0 ? m_pRenderMesh.get() : NULL; }
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);
    virtual void OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo);
    virtual void GetClipPlanes(Plane* pPlanes, int nPlanesNum, int nVertId);
    virtual void GetTexCoordInfo(float* pTexCoordInfo);
    virtual uint8 GetSortPriority() { return m_sortPrio; }

    virtual void SetLayerId(uint16 nLayerId) { m_nLayerId = nLayerId; }
    virtual uint16 GetLayerId() { return m_nLayerId; }

    static void ClipTriangle(CPolygonClipContext& clipContext, PodArray<Vec3>& lstVerts, PodArray<vtx_idx>& lstInds, PodArray<vtx_idx>& lstClippedInds, int nStartIdxId, Plane* pPlanes);
    using IRenderNode::Physicalize;
    virtual void Dephysicalize(bool bKeepIfReferenced = false);

    // Returns true if the rebuild was successful
    bool Compile();
    void DoDeferredCompile();
    void NotifyCompileFinished();

    void ScheduleRebuild();
    void OnTerrainChanged();

    _smart_ptr<IRenderMesh> m_pRenderMesh;
    _smart_ptr<IMaterial>       m_pMaterial;
    PodArray<Vec3> m_arrVerts;
    float m_arrTexCoors[2];
    float m_arrTexCoorsGlobal[2];

    uint8 m_sortPrio;
    bool m_bIgnoreTerrainHoles;
    bool m_bPhysicalize;

    IPhysicalEntity* m_pPhysEnt;

    AABB m_WSBBox;

    uint16 m_nLayerId;

private:
    void MakeRenderMesh();

    RoadRenderNodeCompileInfo* m_pCompileInfo;

    // tmp buffers used during road mesh creation
    PodArray<SVF_P3F_C4B_T2S> m_lstVerticesMerged;
    PodArray<vtx_idx> m_lstIndicesMerged;
    PodArray<SPipTangents> m_lstTangMerged;

    PodArray<Vec3> m_lstVerts;
    PodArray<vtx_idx> m_lstIndices;
    PodArray<vtx_idx> m_lstClippedIndices;

    PodArray<SPipTangents> m_lstTang;
    PodArray<SVF_P3F_C4B_T2S> m_lstVertices;

    CPolygonClipContext m_tmpClipContext;
};
#endif // CRYINCLUDE_CRY3DENGINE_ROADRENDERNODE_H

