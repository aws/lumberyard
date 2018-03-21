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

#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLEEMITTER_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLEEMITTER_H
#pragma once

#include "ParticleEffect.h"
#include "ParticleEnviron.h"
#include "ParticleContainer.h"
#include "ParticleSubEmitter.h"
#include "ParticleManager.h"

#undef PlaySound

class CParticle;

namespace AZ
{
    class LegacyJobExecutor;
}

//////////////////////////////////////////////////////////////////////////
// A top-level emitter system, interfacing to 3D engine
class CParticleEmitter
    : public IParticleEmitter
    , public CParticleSource
{
public:

    CParticleEmitter(const IParticleEffect* pEffect, const QuatTS& loc, uint32 uEmitterFlags = 0, const SpawnParams* pSpawnParams = NULL);
    ~CParticleEmitter();

    //////////////////////////////////////////////////////////////////////////
    // IRenderNode implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void ReleaseNode(bool bImmediate)
    {
        Register(false, bImmediate);
        Kill();
    }
    virtual EERType GetRenderNodeType()
    {
        return eERType_ParticleEmitter;
    }

    virtual char const* GetName() const
    {
        return m_pTopEffect->GetName();
    }
    virtual char const* GetEntityClassName() const
    {
        return "ParticleEmitter";
    }
    virtual string GetDebugString(char type = 0) const;

    virtual Vec3 GetPos(bool bWorldOnly = true) const
    {
        return GetLocation().t;
    }

    virtual const AABB GetBBox() const
    {
        return m_bbWorld;
    }
    virtual void SetBBox(const AABB& WSBBox)
    {
        m_bbWorld = WSBBox;
    }
    virtual void FillBBox(AABB& aabb)
    {
        aabb = GetBBox();
    }

    virtual void GetLocalBounds(AABB& bbox);

    virtual float GetMaxViewDist();

    virtual void SetMatrix(Matrix34 const& mat)
    {
        if (mat.IsValid())
        {
            SetLocation(QuatTS(mat));
        }
    }

    void SetMaterial(_smart_ptr<IMaterial> pMaterial) override
    {
        m_pMaterial = pMaterial;
    }
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    virtual _smart_ptr<IMaterial> GetMaterialOverride()
    {
        return m_pMaterial;
    }

    virtual IPhysicalEntity* GetPhysics() const
    {
        return 0;
    }
    virtual void SetPhysics(IPhysicalEntity*)
    {
    }

    virtual void Render(SRendParams const& rParam, const SRenderingPassInfo& passInfo);

    virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent const& event);
    virtual void OnPhysAreaChange()
    {
        m_PhysEnviron.m_nNonUniformFlags &= ~EFF_LOADED;
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    //////////////////////////////////////////////////////////////////////////
    // IParticleEmitter implementation.
    //////////////////////////////////////////////////////////////////////////
    void SetEffect(IParticleEffect const* pEffect) override;
    const IParticleEffect* GetEffect() const override
    {
        return m_pTopEffect.get();
    };

    void SetLocation(const QuatTS& loc) override;
    QuatTS GetLocationQuat() const override; //Eric@conffx
    ParticleTarget const& GetTarget() const
    {
        return m_Target;
    }
    void SetTarget(ParticleTarget const& target) override
    {
        if ((int)target.bPriority >= (int)m_Target.bPriority)
        {
            if (target.bTarget != m_Target.bTarget || target.vTarget != m_Target.vTarget)
            {
                InvalidateStaticBounds();
            }
            m_Target = target;
        }
    }
    void SetSpawnParams(SpawnParams const& spawnParams, GeomRef geom = GeomRef()) override;
    void GetSpawnParams(SpawnParams& spawnParams) const override
    {
        spawnParams = m_SpawnParams;
    }
    bool IsAlive() const override;
    bool IsInstant() const override;
    virtual bool IsImmortal() const;
    void Prime() override;
    void Activate(bool bActive) override;
    void Kill() override;
    void Restart() override;
    void Update() override;
    void EmitParticle(const EmitParticleData* pData = NULL) override;

    void SetEntity(IEntity* pEntity, int nSlot) override;
    virtual void OffsetPosition(const Vec3& delta);
    bool UpdateStreamableComponents(float fImportance, Matrix34A&  objMatrix, IRenderNode* pRenderNode, float fEntDistance, bool bFullUpdate, int nLod) override;
    EntityId GetAttachedEntityId() override
    {
        return m_nEntityId;
    }
    int GetAttachedEntitySlot() override
    {
        return m_nEntitySlot;
    }
    const uint32& GetEmitterFlags() const override
    {
        return m_nEmitterFlags;
    }
    void SetEmitterFlags(uint32 flags) override;

    bool GetPreviewMode() const override;
    //////////////////////////////////////////////////////////////////////////
    // Other methods.
    //////////////////////////////////////////////////////////////////////////
    float GetEmitterAge() const override //Eric@conffx
    {
        return m_fAge;
    }
    float GetRelativeAge(float fStartAge = 0.f) const override
    {
        if (min(m_fAge - fStartAge, m_fResetAge - fStartAge) <= 0.f)
        {
            return 0.f;
        }
        return div_min(m_fAge - fStartAge, m_fResetAge - fStartAge, 1.f);
    }
    float GetAliveParticleCount() const override;
    bool IsActive() const    // Has particles
    {
        return m_fAge <= m_fDeathAge;
    }


    const SpawnParams& GetSpawnParams() const
    {
        return m_SpawnParams;
    }
    void SetEmitGeom(GeomRef const& geom);
    void UpdateEmitCountScale();
    float GetNearestDistance(const Vec3& vPos, float fBoundsScale) const;

    void SerializeState(TSerialize ser);
    void Register(bool b, bool bImmediate = false);
    ILINE float GetEmitCountScale() const
    {
        return m_fEmitCountScale;
    }

    void RefreshEffect(bool recreateContainer = false);

    void UpdateEffects();

    void UpdateState();
    void UpdateResetAge();

    void CreateIndirectEmitters(CParticleSource* pSource, CParticleContainer* pCont);

    SPhysEnviron const& GetPhysEnviron() const
    {
        return m_PhysEnviron;
    }
    SVisEnviron const& GetVisEnviron() const
    {
        return m_VisEnviron;
    }
    void OnVisAreaDeleted(IVisArea* pVisArea)
    {
        m_VisEnviron.OnVisAreaDeleted(pVisArea);
    }

    void GetDynamicBounds(AABB& bb) const
    {
        bb.Reset();
        for_all_ptrs (const CParticleContainer, c, m_Containers)
        bb.Add(c->GetDynamicBounds());
    }
    ILINE uint32 GetEnvFlags() const
    {
        return m_nEnvFlags & CParticleManager::Instance()->GetAllowedEnvironmentFlags();
    }
    void AddEnvFlags(uint32 nFlags)
    {
        m_nEnvFlags |= nFlags;
    }

    float GetParticleScale() const
    {
        // Somewhat obscure. But top-level emitters spawned from entities,
        // and not attached to other objects, should apply the entity scale to their particles.
        if (!GetEmitGeom())
        {
            return m_SpawnParams.fSizeScale * GetLocation().s;
        }
        else
        {
            return m_SpawnParams.fSizeScale;
        }
    }

#ifdef SHARED_GEOM
    SInstancingInfo* GetInstancingInfo()
    {
        return m_InstInfos.push_back();
    }
#endif

    void InvalidateStaticBounds()
    {
        m_bbWorld.Reset();
        for_all_ptrs (CParticleContainer, c, m_Containers)
        {
            float fStableTime = c->InvalidateStaticBounds();
            m_fBoundsStableAge = max(m_fBoundsStableAge, fStableTime);
        }
    }
    void RenderDebugInfo();
    IEntity* GetEntity() const;
    void UpdateFromEntity();
    uint32 IsIndependent() const
    {
        return m_nEmitterFlags & ePEF_Independent;
    }
    bool NeedSerialize() const
    {
        return (m_nEmitterFlags & ePEF_Independent) && !(m_nEmitterFlags & ePEF_TemporaryEffect);
    }
    float TimeNotRendered() const
    {
        return GetAge() - m_fAgeLastRendered;
    }

    void GetCounts(SParticleCounts& counts) const
    {
        for_all_ptrs (const CParticleContainer, c, m_Containers)
        {
            c->GetCounts(counts);
        }
    }
    void GetAndClearCounts(SParticleCounts& counts)
    {
        FUNCTION_PROFILER_SYS(PARTICLE);
        for_all_ptrs (CParticleContainer, c, m_Containers)
        {
            c->GetCounts(counts);
            c->ClearCounts();
        }
    }

    ParticleList<CParticleContainer> const& GetContainers() const
    {
        return m_Containers;
    }

    bool IsEditSelected() const;
    void AddUpdateParticlesJob();
    void SyncUpdateParticlesJob();
    void UpdateAllParticlesJob();

    void Reset()
    {
        Register(false);

        // Free unneeded memory.
        m_Containers.clear();

        // Release and remove external geom refs.
        SEmitGeom::Release();
        SEmitGeom::operator= (SEmitGeom());
    }

    void SetUpdateParticlesJobState(AZ::LegacyJobExecutor* pJobExecutor)
    {
        m_pJobExecutorParticles = pJobExecutor;
    }

    void GetEmitterBounds(Vec3& totalBounds) override
    {
        for (ParticleList<CParticleContainer>::iterator container(m_Containers); container != m_Containers.end(); ++container)
        {
            Vec3 currentBounds(0.f);
            container->GetContainerBounds(currentBounds);
            totalBounds.CheckMax(currentBounds);
        }
    }

private:

    // Internal emitter flags, extend EParticleEmitterFlags
    enum EFlags
    {
        ePEF_HasPhysics                     = BIT(6),
        ePEF_HasTarget                      = BIT(7),
        ePEF_HasAttachment                  = BIT(8),
        ePEF_NeedsEntityUpdate              = BIT(9),
        ePEF_Registered                     = BIT(10),
    };

    AZ::LegacyJobExecutor*                          m_pJobExecutorParticles;

    // Constant values, effect-related.
    _smart_ptr<CParticleEffect>                     m_pTopEffect;
    _smart_ptr<IMaterial>                           m_pMaterial;            // Override material for this emitter.

    // Cache values derived from the main effect.
    float                                           m_fMaxParticleSize;

    SpawnParams                                     m_SpawnParams;          // External settings modifying emission.
    float                                           m_fEmitCountScale;      // Composite particle count scale.

    ParticleList<CParticleContainer>                m_Containers;
    uint32                                          m_nEnvFlags;            // Union of environment flags affecting emitters.
    uint32                                          m_nRenObjFlags;         // Union of render feature flags.
    ParticleTarget                                  m_Target;               // Target set from external source.

    AABB                                            m_bbWorld;              // World bbox.

    float                                           m_fAgeLastRendered;
    float                                           m_fBoundsStableAge;     // Next age at which bounds stable.
    float                                           m_fResetAge;            // Age to purge unseen particles.
    float                                           m_fStateChangeAge;      // Next age at which a container's state changes.
    float                                           m_fDeathAge;            // Age when all containers (particles) dead.

    // Entity connection params.
    int                                             m_nEntityId;
    int                                             m_nEntitySlot;

    uint32                                          m_nEmitterFlags;

    SPhysEnviron                                    m_PhysEnviron;          // Common physical environment (uniform forces only) for emitter.
    SVisEnviron                                     m_VisEnviron;
    
    bool                                            m_isPrimed;             // Indicates that Prime() was called on this frame.

#ifdef SHARED_GEOM
    ParticleList<SInstancingInfo>                   m_InstInfos;            // For geom particle rendering.
#endif // SHARED_GEOM

    // Functions.
    void ResetUnseen();
    void AllocEmitters();
    void UpdateContainers();
    void UpdateTimes(float fAgeAdjust = 0.f);

    // bInEditor indicates that the particle is being tweaked in the editor and prevents indirect particles from being deleted when they are active
    void AddEffect(CParticleContainer* pParentContainer, const CParticleEffect* pEffect, bool bUpdate = true, bool bInEditor = false, CLodInfo* startLod = nullptr, CLodInfo* endLod = nullptr);
    CParticleContainer* AddContainer(CParticleContainer* pParentContainer, const CParticleEffect* pEffect, CLodInfo* startLod = nullptr, CLodInfo* endLod = nullptr);
};

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLEEMITTER_H
