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
#ifndef _EFFECTS_RENDERNODES_LIGHTNINGNODE_H_
#define _EFFECTS_RENDERNODES_LIGHTNINGNODE_H_
#pragma once

#include <GameEffectSystem/IGameRenderNode.h>

struct SLightningParams;
struct SLightningStats;

class CLightningRenderNode
    : public IRenderNode
{
private:
    typedef SVF_P3F_C4B_T2F SLightningVertex;

    class CTriStrip
    {
    private:
        typedef std::vector<SLightningVertex> TVertexArray;
        typedef std::vector<uint16> TIndexArray;

    public:
        void Reset();
        void PushVertex(SLightningVertex v);
        void Branch();
        void Draw(const SRendParams& renderParams, const SRenderingPassInfo& passInfo, IRenderer* pRenderer,
            CRenderObject* pRenderObject, _smart_ptr<IMaterial>  pMaterial, float distanceToCamera);
        void Clear();

        void AddStats(SLightningStats* pStats) const;

    private:
        TVertexArray m_vertices;
        TIndexArray m_indices;
        int m_firstVertex;
    };

    struct SPointData
    {
        std::vector<Vec3> m_points;
        std::vector<Vec3> m_velocity;
        std::vector<Vec3> m_fuzzyPoints;
    };

    class CSegment
    {
    public:
        void Create(const SLightningParams& desc, SPointData* m_pointData, int _parentSegmentIdx, int _parentPointIdx,
            Vec3 _origin, Vec3 _destany, float _duration, float _intensity);
        void Update(const SLightningParams& desc);
        void Draw(const SLightningParams& desc, const SPointData& pointData, CTriStrip* strip, Vec3 cameraPosition,
            float deviationMult);
        bool IsDone(const SLightningParams& desc);

        int GetNumPoints() const { return m_numFuzzyPoints; }
        Vec3 GetPoint(const SLightningParams& desc, const SPointData& pointData, int idx, float deviationMult) const;

        void SetOrigin(Vec3 _origin);
        void SetDestany(Vec3 _destany);

        int GetParentSegmentIdx() const { return m_parentSegmentIdx; }
        int GetParentPointIdx() const { return m_parentPointIdx; }
        void DecrementParentIdx() { --m_parentSegmentIdx; }

    private:
        int m_firstPoint;
        int m_numPoints;
        int m_firstFuzzyPoint;
        int m_numFuzzyPoints;

        Vec3 m_origin;
        Vec3 m_destany;

        float m_duration;
        float m_time;
        float m_intensity;

        int m_parentSegmentIdx;
        int m_parentPointIdx;
    };

    typedef std::vector<CSegment> TSegments;

public:
    CLightningRenderNode();
    ~CLightningRenderNode() override;

    // IRenderNode
    EERType GetRenderNodeType() override;
    const char* GetEntityClassName() const override;
    const char* GetName() const override;
    Vec3 GetPos(bool bWorldOnly = true) const override;
    void Render(const struct SRendParams& rParam, const SRenderingPassInfo& passInfo) override;
    IPhysicalEntity* GetPhysics() const override;
    void SetPhysics(IPhysicalEntity*) override;
    void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    _smart_ptr<IMaterial>  GetMaterial(Vec3* pHitPos = 0) override;
    _smart_ptr<IMaterial>  GetMaterialOverride() override;
    float GetMaxViewDist() override;
    void Precache() override;
    void GetMemoryUsage(ICrySizer* pSizer) const override;
    const AABB GetBBox() const override;
    void FillBBox(AABB& aabb) override;
    void SetBBox(const AABB& WSBBox) override;
    void OffsetPosition(const Vec3& delta) override {}
    bool IsAllocatedOutsideOf3DEngineDLL() override;

    void Reset();
    float TriggerSpark();
    void SetLightningParams(const SLightningParams* pDescriptor);
    void SetEmiterPosition(Vec3 emiterPosition);
    void SetReceiverPosition(Vec3 receiverPosition);
    void SetSparkDeviationMult(float deviationMult);

    void AddStats(SLightningStats* pStats) const;

private:
    void Update();
    void Draw(CTriStrip* strip, Vec3 cameraPosition);
    void CreateSegment(Vec3 originPosition, int parentIdx, int parentPointIdx, float duration, int level);
    void PopSegment();

    CTriStrip m_triStrip;
    TSegments m_segments;
    Vec3 m_emmitterPosition;
    Vec3 m_receiverPosition;
    SPointData m_pointData;
    float m_deviationMult;

    const SLightningParams* m_pLightningDesc;
    _smart_ptr<IMaterial> m_pMaterial;

    mutable AABB m_aabb;
    mutable bool m_dirtyBBox;
};

#endif // CRYINCLUDE_GAMEDLL_EFFECTS_RENDERNODES_LIGHTNINGNODE_H
