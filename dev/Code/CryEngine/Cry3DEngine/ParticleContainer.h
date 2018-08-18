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

#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLECONTAINER_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLECONTAINER_H
#pragma once

#include "ParticleEffect.h"
#include "ParticleEnviron.h"
#include "ParticleUtils.h"
#include "ParticleList.h"
#include "Particle.h"
#include "CREParticle.h"

class CParticleEmitter;
class CParticleSubEmitter;
class CParticleContainerGPU;
struct SParticleVertexContext;
struct SContainerCounts;

typedef ParticleList<CParticle>::Node TParticleElem;


//////////////////////////////////////////////////////////////////////////
struct SParticleUpdateContext
{
    float                       fUpdateTime;
    uint32                  nEnvFlags;
    Vec3                        vEmitBox;
    Vec3                        vEmitScale;
    AABB*                       pbbDynamicBounds;
    bool                        bHasTarget;

    // Iteration support.
    float                       fMaxLinearDeviation;
    float                       fMaxLinearStepTime;
    float                       fMinStepTime;

    // Sorting support.
    Vec3                        vMainCamPos;
    float                       fCenterCamDist;
    int                         nSortQuality;
    bool                        b3DRotation;

    // Updated per emitter.
    float                       fDensityAdjust;

    // Lod support
    CLodInfo*                   startLodInfo;
    CLodInfo*                   endLodInfo;
    float                       lodBlend; //Value between 0 and 1 that represents the alpha value. If 0, the particle should not emit anything.

    struct SSortElem
    {
        const CParticle*    pPart;
        float                           fDist;

        bool operator < (const SSortElem& r) const
        { return fDist < r.fDist; }
    };
    Array<SSortElem>        aParticleSort;

    struct SSpaceLoop
    {
        Vec3                            vCenter, vSize;
        Vec3                            vScaledAxes[3];
    };
    SSpaceLoop SpaceLoop;
} _ALIGN(16);

//////////////////////////////////////////////////////////////////////////
// Particle rendering helper params.
struct SPartRenderParams
{
    float                                   m_fCamDistance;
    float                                   m_fMainBoundsScale;
    float                                   m_fMaxAngularDensity;
    uint16                              m_nFogVolumeContribIdx;
    uint32                              m_nRenFlags;
    TrinaryFlags<uint32>    m_nRenObjFlags;
    uint16                              m_nDeferredLightVolumeId;
} _ALIGN(16);


//////////////////////////////////////////////////////////////////////////
// Contains, updates, and renders a list of particles, of a single effect

class CParticleContainer
    : public IParticleVertexCreator
    , public Cry3DEngineBase
{
public:
    CParticleContainer(CParticleContainer* pParent, CParticleEmitter* pMain, CParticleEffect const* pEffect, CLodInfo* startLod, CLodInfo* endLod);
    CParticleContainer(CParticleContainer* pParent, CParticleEmitter* pMain, CParticleEffect const* pEffect);
    ~CParticleContainer();

    // Associated structures.
    ILINE const CParticleEffect* GetEffect() const                      { return m_pEffect; }
    ILINE CParticleEmitter& GetMain()                                   { return *m_pMainEmitter; }
    ILINE const CParticleEmitter& GetMain() const                       { return *m_pMainEmitter; }
    ILINE CParticleContainerGPU* GetGPUData()                           { return m_pGPUData; }
    ILINE ResourceParticleParams const& GetParams() const               { return *m_pParams; }

    float GetAge() const;

    void SetGPUData(CParticleContainerGPU* pGPUData);

    ILINE bool IsGPUParticle() const
    {
        return m_pParams->eEmitterType == ParticleParams::EEmitterType::GPU;
    }
    ILINE bool IsIndirect() const
    {
        return m_pParentContainer != 0;
    }
    ILINE CParticleSubEmitter* GetDirectEmitter()
    {
        if (!m_pParentContainer && !m_Emitters.empty())
        {
            assert(m_Emitters.size() == 1);
            return &m_Emitters.front();
        }
        return NULL;
    }

    // If indirect container.
    ILINE CParticleContainer* GetParent() const
    {
        return m_pParentContainer;
    }

    void Reset();

    CParticleSubEmitter* AddEmitter(CParticleSource* pSource);
    CParticle* AddParticle(SParticleUpdateContext& context, const CParticle& part);

    void UpdateParticles(const CCamera* overrideCamera = nullptr);
    void UpdateEmitters(SParticleUpdateContext* pEmitContext = NULL);
    void UpdateEffects();
    void EmitParticle(const EmitParticleData* pData);
    uint16 GetNextEmitterSequence()
    {
        return m_nEmitterSequence++;
    }
    void SyncUpdateParticles(const CCamera* overrideCamera = nullptr);
    float InvalidateStaticBounds();
    void Render(SRendParams const& RenParams, SPartRenderParams const& PRParams, const SRenderingPassInfo& passInfo);
    void RenderGeometry(const SRendParams& RenParams, const SRenderingPassInfo& passInfo);
    void RenderDecals(const SRenderingPassInfo& passInfo);
    void RenderLights(const SRendParams& RenParams, const SRenderingPassInfo& passInfo);

    // Bounds functions.
    void UpdateState();
    void SetDynamicBounds();

    uint32 NeedsDynamicBounds() const
    {
    #ifndef _RELEASE
        if (GetCVars()->e_ParticlesDebug & AlphaBit('d'))
        {
            // Force all dynamic bounds computation and culling.
            return EFF_DYNAMIC_BOUNDS;
        }
    #endif
        return (m_nEnvFlags | m_nChildFlags) & EFF_DYNAMIC_BOUNDS;
    }
    bool StaticBoundsStable() const
    {
        return GetAge() >= m_fAgeStaticBoundsStable;
    }
    AABB const& GetBounds() const
    {
        return m_bbWorld;
    }
    AABB const& GetStaticBounds() const
    {
        return m_bbWorldStat;
    }
    AABB const& GetDynamicBounds() const
    {
        return m_bbWorldDyn;
    }
    uint32 GetEnvironmentFlags() const;
    size_t GetNumParticles() const
    {
        return m_Particles.size();
    }
    uint32 GetChildFlags() const
    {
        return m_nChildFlags;
    }
    float GetContainerLife() const
    {
        return m_fContainerLife;
    }
    void UpdateContainerLife(float fAgeAdjust = 0.f);
    bool GetTarget(ParticleTarget& target, const CParticleSubEmitter* pSubEmitter) const;
    float GetTimeToUpdate() const
    {
        return GetAge() - m_fAgeLastUpdate;
    }

    // Temp functions to update edited effects.
    inline bool IsUsed() const
    {
        return m_bUsed;
    }
    void SetUsed(bool used);

    int GetHistorySteps() const
    {
        return m_nHistorySteps;
    }
    uint32 NeedsCollisionInfo() const
    {
        return GetEnvironmentFlags() & ENV_COLLIDE_INFO;
    }
    float GetMaxParticleFullLife() const
    {
        return m_fMaxParticleFullLife;
    }
    void OnEffectChange();
    void ComputeUpdateContext(SParticleUpdateContext& context, float fUpdateTime);

    // Stat/profile functions.
    SContainerCounts& GetCounts()
    {
        return m_Counts;
    }
    void GetCounts(SParticleCounts& counts) const;
    void ClearCounts()
    {
        ZeroStruct(m_Counts);
    }
    void GetMemoryUsage(ICrySizer* pSizer) const;

    //////////////////////////////////////////////////////////////////////////
    // IParticleVertexCreator methods
    virtual void ComputeVertices(const SCameraInfo& camInfo, CREParticle* pRE, uint64 uVertexFlags, float fMaxPixels);

    // Other methods.
    bool NeedsUpdate() const;

    // functions for particle generate and culling of vertices directly into Video Memory
    int CullParticles(SParticleVertexContext& Context, int& nVertices, int& nIndices, uint8 auParticleAlpha[]);
    void WriteVerticesToVMEM(SParticleVertexContext& Context, SRenderVertices& RenderVerts, const uint8 auParticleAlpha[]);

    void SetNeedJobUpdate(uint8 n)
    {
        m_nNeedJobUpdate = n;
    }
    uint8 NeedJobUpdate() const
    {
        return m_nNeedJobUpdate;
    }
    bool WasRenderedPrevFrame() const
    {
        return m_nNeedJobUpdate > 0;
    }
    int GetEmitterCount() const
    {
        return m_Emitters.size();
    }
    void OffsetPosition(const Vec3& delta);

    CLodInfo*   GetStartLod();
    void    SetEndLod(CLodInfo* lod);
    void    SetStartLod(CLodInfo* lod);
    
    float ComputeLodBlend(float distanceToCamera, float updateTime);

    void GetContainerBounds(Vec3& totalBounds);

private:
    const ResourceParticleParams*                               m_pParams;                  // Pointer to particle params (effect or code).
    _smart_ptr<CParticleEffect>                                 m_pEffect;                  // Particle effect used for this emitter.
    uint32                                                      m_nEnvFlags;
    uint32                                                      m_nChildFlags;              // Summary of rendering/environment info for child containers.

    ParticleList<CParticleSubEmitter>                           m_Emitters;                 // All emitters into this container.
    ParticleList<EmitParticleData>                              m_DeferredEmitParticles;    // Data for client EmitParticles calls, deferred till next update.
    ParticleList< _smart_ptr<IStatObj> >                        m_ExternalStatObjs;         // Any StatObjs from EmitParticle; released on destruction.

    uint16                                                      m_nHistorySteps;
    uint16                                                      m_nEmitterSequence;

    ParticleList<CParticle>                                     m_Particles;

    // Last time when emitter updated, and static bounds validated.
    float                                                       m_fAgeLastUpdate;
    float                                                       m_fAgeStaticBoundsStable;
    float                                                       m_fContainerLife;

    // Final bounding volume for rendering. Also separate static & dynamic volumes for sub-computation.
    AABB                                                        m_bbWorld, m_bbWorldStat, m_bbWorldDyn;

    bool                                                        m_bUsed;                    // Temp var used during param editing.
    uint8                                                       m_nNeedJobUpdate;           // Mark container as needing sprite rendering this frame.
                                                                                            // Can be set to >1 to prime for threaded update next frame.

    // Associated structures.
    CParticleContainerGPU*                                      m_pGPUData;                 // Pointer to GPU specific particle data
    CParticleContainer*                                         m_pParentContainer;         // Parent container, if indirect.
    CParticleEmitter*                                           m_pMainEmitter;             // Emitter owning this container.
    float                                                       m_fMaxParticleFullLife;     // Cached value indicating max update time necessary.
    
    SContainerCounts                                            m_Counts;                   // Counts for stats.

    //Level of Detail
    _smart_ptr<CLodInfo> m_StartLodInfoPtr; //This lod specifies when this container will start emitting particles
    _smart_ptr<CLodInfo> m_EndLodInfoPtr; //This lod specifies when this container will end emitting particles
    float m_LodBlend;
    float m_LodOverlap;

    // Bounds functions.
    bool NeedsExtendedBounds() const
    {
        // Bounds need extending on movement unless bounds always restricted relative to emitter.
        return m_pParams->eMoveRelEmitter < (m_pParams->GetTailSteps() ? ParticleParams::EMoveRelative::YesWithTail : ParticleParams::EMoveRelative::Yes) && !m_pParams->bSpaceLoop;
    }

    void ComputeStaticBounds(AABB& bb, bool bWithSize = true, float fMaxLife = fHUGE);
    float GetEmitterLife() const;
    int GetMaxParticleCount(const SParticleUpdateContext& context) const;
    void UpdateParticleStates(SParticleUpdateContext& context);
    void SetScreenBounds(const CCamera& cam, uint8 aScreenBounds[4]);
};

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLECONTAINER_H
