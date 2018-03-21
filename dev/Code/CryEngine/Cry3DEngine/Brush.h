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

#ifndef CRYINCLUDE_CRY3DENGINE_BRUSH_H
#define CRYINCLUDE_CRY3DENGINE_BRUSH_H
#pragma once

#include "ObjMan.h"
#include "DeformableNode.h"

#if defined(LINUX)
#include "platform.h"
#endif

class CBrush
    :   public IBrush
    , public Cry3DEngineBase
{
    friend class COctreeNode;

public:
    CBrush();
    virtual ~CBrush();

    virtual const char* GetEntityClassName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual const char* GetName() const;
    virtual bool HasChanged();
    virtual void Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo);
    virtual CLodValue ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo) override;
    void Render(const CLodValue& lodValue, const SRenderingPassInfo& passInfo, const SSectorTextureSet* pTerrainTexInfo, AZ::LegacyJobExecutor* pJobExecutor, const SRendItemSorter& rendItemSorter);
    void Render_JobEntry(CRenderObject* pObjMain, const CLodValue lodValue, const SRenderingPassInfo& passInfo, SRendItemSorter rendItemSorter);

    virtual struct IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false);

    virtual bool GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
    float GetFirstLodDistance() const override { return m_pStatObj ? m_pStatObj->GetLodDistance() : FLT_MAX; }

    virtual void SetEntityStatObj(unsigned int nSlot, IStatObj* pStatObj, const Matrix34A* pMatrix = NULL);

    virtual IRenderNode* Clone() const;

    virtual void SetCollisionClassIndex(int tableIndex) { m_collisionClassIdx = tableIndex; }

    virtual void SetLayerId(uint16 nLayerId) { m_nLayerId = nLayerId; }
    virtual uint16 GetLayerId() { return m_nLayerId; }
    virtual struct IRenderMesh* GetRenderMesh(int nLod);

    virtual IPhysicalEntity* GetPhysics() const;
    virtual void SetPhysics(IPhysicalEntity* pPhys);
    static bool IsMatrixValid(const Matrix34& mat);
    virtual void Dephysicalize(bool bKeepIfReferenced = false);
    virtual void Physicalize(bool bInstant = false);
    void PhysicalizeOnHeap(IGeneralMemoryHeap* pHeap, bool bInstant = false);
    virtual bool PhysicalizeFoliage(bool bPhysicalize = true, int iSource = 0, int nSlot = 0);
    virtual IPhysicalEntity* GetBranchPhys(int idx, int nSlot = 0) { IFoliage* pFoliage = GetFoliage(nSlot); return pFoliage ? pFoliage->GetBranchPhysics(idx) : 0; }
    virtual struct IFoliage* GetFoliage(int nSlot = 0);

    //! Assign override material to this entity.
    void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual void CheckPhysicalized();

    virtual float GetMaxViewDist();

    virtual EERType GetRenderNodeType();

    void SetStatObj(IStatObj* pStatObj);

    void SetMatrix(const Matrix34& mat);
    const Matrix34& GetMatrix() const {return m_Matrix; }
    virtual void SetDrawLast(bool enable) { m_bDrawLast = enable; }
    bool GetDrawLast() const { return m_bDrawLast; }

    void Dematerialize();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    static PodArray<SExportedBrushMaterial> m_lstSExportedBrushMaterials;

    virtual const AABB GetBBox() const;
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);

    virtual bool CanExecuteRenderAsJob();

    //private:
    void CalcBBox();
    void UpdatePhysicalMaterials(int bThreadSafe = 0);

    void OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo);

    void UpdateExecuteAsPreProcessJobFlag();

    bool HasDeformableData() const { return m_pDeform != NULL; }

    Matrix34 m_Matrix;
    float m_fMatrixScale;
    IPhysicalEntity* m_pPhysEnt;

    //! Override material.
    _smart_ptr<IMaterial> m_pMaterial;

    uint16 m_collisionClassIdx;
    uint16 m_nLayerId;

    _smart_ptr<IStatObj> m_pStatObj;
    CDeformableNode* m_pDeform;

    bool m_bVehicleOnlyPhysics;

    bool m_bMerged;
    bool m_bExecuteAsPreprocessJob;
    bool m_bDrawLast;

    AABB m_WSBBox;
};

///////////////////////////////////////////////////////////////////////////////
inline const AABB CBrush::GetBBox() const
{
    return m_WSBBox;
}

#endif // CRYINCLUDE_CRY3DENGINE_BRUSH_H
