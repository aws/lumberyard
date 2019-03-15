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

#ifndef CRYINCLUDE_CRY3DENGINE_VEGETATION_H
#define CRYINCLUDE_CRY3DENGINE_VEGETATION_H
#pragma once

#include "ObjMan.h"

class CDeformableNode;

#define VEGETATION_CONV_FACTOR 64.f

template <class T>
class PodArrayAABB
    : public PodArray<T>
{
public:
    AABB m_aabbBox;
};

// Warning: Average outdoor level has about 200.000 objects of this class allocated - so keep it small
class CVegetation
    : public IVegetation
    , public Cry3DEngineBase
{
public:
    Vec3 m_vPos;
    IPhysicalEntity* m_pPhysEnt;
    CDeformableNode* m_pDeformable;
    PodArrayAABB<CRenderObject::SInstanceData>* m_pInstancingInfo;
    int  m_nObjectTypeIndex;
    byte  m_ucAngle;
    byte  m_ucSunDotTerrain;
    byte  m_ucScale;
    byte  m_boxExtends[6];
    byte  m_ucRadius;
    byte  m_ucAngleX;
    byte  m_ucAngleY;
    byte m_bApplyPhys;

    static float g_scBoxDecomprTable[256] _ALIGN(128);

    CVegetation();
    virtual ~CVegetation();
    void SetStatObjGroupIndex(int nVegetationGroupIndex);

    void CheckCreateDeformable();

    int GetStatObjGroupId() const { return m_nObjectTypeIndex; }
    const char* GetEntityClassName(void) const { return "Vegetation"; }
    Vec3 GetPos(bool bWorldOnly = true) const;
    virtual float GetScale(void) const { return (1.f / VEGETATION_CONV_FACTOR) * m_ucScale; }
    void SetScale(float fScale) { m_ucScale = (uint8)SATURATEB(fScale * VEGETATION_CONV_FACTOR); }
    virtual const char* GetName() const override;
    virtual CLodValue ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo) override;
    bool CanExecuteRenderAsJob() override;
    virtual void Render(const SRendParams& RendParams, const SRenderingPassInfo& passInfo){ assert(0); }
    void Render(const SRenderingPassInfo& passInfo, const CLodValue& lodValue, SSectorTextureSet* pTerrainTexInfo, const SRendItemSorter& rendItemSorter) const;
    IPhysicalEntity* GetPhysics(void) const { return m_pPhysEnt; }
    IRenderMesh*       GetRenderMesh(int nLod);
    void SetPhysics(IPhysicalEntity* pPhysEnt) { m_pPhysEnt = pPhysEnt; }
    void SetMaterial(_smart_ptr<IMaterial>) override {}
    _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    _smart_ptr<IMaterial> GetMaterialOverride();
    void SetMatrix(const Matrix34& mat);
    virtual void Physicalize(bool bInstant = false);
    bool PhysicalizeFoliage(bool bPhysicalize = true, int iSource = 0, int nSlot = 0);
    virtual IRenderNode* Clone() const;
    IPhysicalEntity* GetBranchPhys(int idx, int nSlot = 0);
    IFoliage* GetFoliage(int nSlot = 0);
    bool IsBreakable() { pe_params_part pp; pp.ipart = 0; return m_pPhysEnt && m_pPhysEnt->GetParams(&pp) && pp.idmatBreakable >= 0; }
    void AddBending(Vec3 const& v);
    virtual float GetMaxViewDist();
    IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false);
    virtual EERType GetRenderNodeType();
    virtual void Dephysicalize(bool bKeepIfReferenced = false);
    void Dematerialize();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const AABB GetBBox() const;
    virtual void FillBBox(AABB& aabb);
    virtual void SetBBox(const AABB& WSBBox);
    virtual void OffsetPosition(const Vec3& delta);
    const float GetRadius() const;
    void UpdateRndFlags();
    int GetStatObjGroupSize() const;
    StatInstGroup& GetStatObjGroup() const;

    IStatObj* GetStatObj();

    float GetZAngle() const;
    AABB CalcBBox();
    void CalcMatrix(Matrix34A& tm, int* pTransFags = NULL);
    virtual uint8 GetMaterialLayers() const;
    //  float GetLodForDistance(float fDistance);
    void UpdateSunDotTerrain();
    void Init();
    void ShutDown();
    void OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo);
    void UpdateBending();
    static void InitVegDecomprTable();
    virtual bool GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
    float GetFirstLodDistance() const override;

    // Avoid C4266 virtual function is hidden because that SetScale(float fScale) is also defined in IVegetation: https://msdn.microsoft.com/en-us/library/4b76ty10.aspx
    void SetScale(const Vec3& scale) override
    {
        IRenderNode::SetScale(scale);
    }

    ILINE void SetBoxExtends(byte* pBoxExtends, byte* pRadius, const AABB& RESTRICT_REFERENCE aabb, const Vec3& RESTRICT_REFERENCE vPos)
    {
        const float fRatio = (255.f / VEGETATION_CONV_FACTOR);
        Vec3 v0 = aabb.max - vPos;
        pBoxExtends[0] = (byte)SATURATEB(v0.x * fRatio + 1.f);
        pBoxExtends[1] = (byte)SATURATEB(v0.y * fRatio + 1.f);
        pBoxExtends[2] = (byte)SATURATEB(v0.z * fRatio + 1.f);
        Vec3 v1 = vPos - aabb.min;
        pBoxExtends[3] = (byte)SATURATEB(v1.x * fRatio + 1.f);
        pBoxExtends[4] = (byte)SATURATEB(v1.y * fRatio + 1.f);
        pBoxExtends[5] = (byte)SATURATEB(v1.z * fRatio + 1.f);
        *pRadius = (byte)SATURATEB(max(v0.GetLength(), v1.GetLength()) * fRatio + 1.f);
    }

    ILINE void FillBBoxFromExtends(AABB& aabb, const byte* const __restrict pBoxExtends, const Vec3& RESTRICT_REFERENCE vPos) const
    {
        const float* const __restrict cpDecompTable = g_scBoxDecomprTable;
        const float cData0 = cpDecompTable[pBoxExtends[0]];
        const float cData1 = cpDecompTable[pBoxExtends[1]];
        const float cData2 = cpDecompTable[pBoxExtends[2]];
        const float cData3 = cpDecompTable[pBoxExtends[3]];
        const float cData4 = cpDecompTable[pBoxExtends[4]];
        const float cData5 = cpDecompTable[pBoxExtends[5]];
        aabb.max.x = vPos.x + cData0;
        aabb.max.y = vPos.y + cData1;
        aabb.max.z = vPos.z + cData2;
        aabb.min.x = vPos.x - cData3;
        aabb.min.y = vPos.y - cData4;
        aabb.min.z = vPos.z - cData5;
    }

    ILINE void FillBBox_NonVirtual(AABB& aabb) const
    {
        FillBBoxFromExtends(aabb, m_boxExtends, m_vPos);
    }
};

#endif // CRYINCLUDE_CRY3DENGINE_VEGETATION_H
