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

#include "StdAfx.h"
#include "ParticleEmitter.h"
#include "ParticleContainer.h"
#include "ParticleSubEmitter.h"
#include "ICryAnimation.h"
#include "Particle.h"
#include "FogVolumeRenderNode.h"
#include "IThreadTask.h"
#include "STLGlobalAllocator.h"
#include "StringUtils.h"

#include "Components/IComponentRender.h"
#include "../CryCommon/IGPUParticleEngine.h"
#include "ParticleContainerGPU.h"


static const float fUNSEEN_EMITTER_RESET_TIME   = 5.f;      // Max time to wait before freeing unseen emitter resources
static const float fVELOCITY_SMOOTHING_TIME     = 0.125f;   // Interval to smooth computed entity velocity

/*
    Scheme for Emitter updating & bounding volume computation.

    Whenever possible, we update emitter particles only on viewing.
    However, the bounding volume for all particles must be precomputed,
    and passed to the renderer, for visibility. Thus, whenever possible,
    we precompute a Static Bounding Box, loosely estimating the possible
    travel of all particles, without actually updating them.

    Currently, Dynamic BBs are computed for emitters that perform physics,
    or are affected by non-uniform physical forces.
*/

static void EnsureGpuDataCreated(CParticleContainer* c)
{
    if (c->GetGPUData() == nullptr)
    {
        IRenderer* render = c->GetRenderer();
        if (render && render->GetGPUParticleEngine())
        {
            // create GPU instance if it doesn't exist
            c->SetGPUData(new CParticleContainerGPU());
            c->GetGPUData()->Initialize(c);
        }
    }
}


static void RenderContainer(CParticleContainer* c, SRendParams const& RenParams, SPartRenderParams PartParams, const SRenderingPassInfo& passInfo)
{
    if (c->IsGPUParticle() == false)
    {
        c->SetGPUData(nullptr);
        c->Render(RenParams, PartParams, passInfo);
    }
    else
    {
        EnsureGpuDataCreated(c);
        CParticleContainerGPU* gpuData = c->GetGPUData();
        if (gpuData)
        {
            gpuData->Render(RenParams, PartParams, passInfo);
        }
    }
}

AZ::LegacyJobExecutor* CParticleBatchDataManager::AddUpdateJob(CParticleEmitter *pEmitter)
{
    AZ::LegacyJobExecutor* pJobExecutor = GetJobExecutor();

    pJobExecutor->Reset();
    pJobExecutor->StartJob(
        [pEmitter]
        {
            pEmitter->UpdateAllParticlesJob();
        }
    );

    return pJobExecutor;
}

bool CParticleEmitter::IsEditSelected() const
{
    if (gEnv->IsEditing())
    {
        if (IEntity* pEntity = GetEntity())
        {
            if (IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>())
            {
                if (IRenderNode* pRenderNode = pRenderComponent->GetRenderNode())
                {
                    return (pRenderNode->GetRndFlags() & ERF_SELECTED) != 0;
                }
            }
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////
void CParticleEmitter::AddUpdateParticlesJob()
{
    FUNCTION_PROFILER_SYS(PARTICLE)

    if (!GetCVars()->e_ParticlesThread)
    {
        return;
    }

    // Queue background job for particle updates.
    assert(!m_pJobExecutorParticles);

    m_pJobExecutorParticles = CParticleManager::Instance()->AddUpdateJob(this);
}

void CParticleEmitter::UpdateAllParticlesJob()
{
    AZ_TRACE_METHOD();
    for_all_ptrs(CParticleContainer, pCont, m_Containers)
    {
        if (pCont->NeedJobUpdate())
        {
            pCont->UpdateParticles();
        }
        else
        {
            pCont->UpdateEmitters();
        }
    }
}

void CParticleEmitter::SyncUpdateParticlesJob()
{
    if (m_pJobExecutorParticles)
    {
        FUNCTION_PROFILER(gEnv->pSystem, PROFILE_PARTICLE);
        PARTICLE_LIGHT_SYNC_PROFILER();
        m_pJobExecutorParticles->WaitForCompletion();
        m_pJobExecutorParticles = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CParticleEmitter::GetMaterial(Vec3*)
{
    if (m_pMaterial)
    {
        return m_pMaterial;
    }
    if (!m_Containers.empty())
    {
        return m_Containers.front().GetParams().pMaterial;
    }
    return NULL;
}

float CParticleEmitter::GetMaxViewDist()
{
    // Max particles/radian, modified by emitter settings.
    return gEnv->pSystem->GetViewCamera().GetAngularResolution()
           * max(GetCVars()->e_ParticlesMinDrawPixels, 0.125f)
           * m_fMaxParticleSize
           * GetParticleScale()
           * GetViewDistanceMultiplier();
}

bool CParticleEmitter::IsAlive() const
{
    if (m_fAge <= m_fDeathAge)
    {
        return true;
    }

    if (GetSpawnParams().fPulsePeriod > 0.f)
    {
        // Between pulses.
        return true;
    }

    if (m_fAge < 0.f)
    {
        // Emitter paused.
        return true;
    }

    // Natural death.
    return false;
}

void CParticleEmitter::UpdateState()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    uint32 nEnvFlags = GetEnvFlags();
    Vec3 vPos = GetLocation().t;

    bool bUpdateBounds = (nEnvFlags & REN_ANY) && (
            m_bbWorld.IsReset() ||
            (nEnvFlags & EFF_DYNAMIC_BOUNDS) ||
        #ifndef _RELEASE
            (GetCVars()->e_ParticlesDebug & AlphaBit('d')) ||
        #endif
            m_fAge <= m_fBoundsStableAge
            );

    if (m_nEnvFlags & ENV_PHYS_AREA)
    {
        bool bCreateAreaChangeProxy = (m_pRNTmpData && m_pRNTmpData->nPhysAreaChangedProxyId == ~0);

        IF (bCreateAreaChangeProxy || !(m_PhysEnviron.m_nNonUniformFlags & EFF_LOADED), 0)
        {
            // For initial bounds computation, query the physical environment at the origin.
            m_PhysEnviron.GetPhysAreas(CParticleManager::Instance()->GetPhysEnviron(), AABB(vPos),
                Get3DEngine()->GetVisAreaFromPos(vPos) != NULL, m_nEnvFlags, true, this);

            // Get phys area changed proxy if not set yet
            if (bCreateAreaChangeProxy)
            {
                uint16 uPhysicsMask = 0;
                if (m_nEnvFlags & ENV_WATER)
                {
                    uPhysicsMask |= Area_Water;
                }
                if (m_nEnvFlags & ENV_WIND)
                {
                    uPhysicsMask |= Area_Air;
                }
                if (m_nEnvFlags & ENV_GRAVITY)
                {
                    uPhysicsMask |= Area_Gravity;
                }
                m_pRNTmpData->nPhysAreaChangedProxyId = Get3DEngine()->GetPhysicsAreaUpdates().CreateProxy(this, uPhysicsMask);
            }

            bUpdateBounds = true;
        }
    }

    bool bUpdateState = bUpdateBounds || m_fAge >= m_fStateChangeAge;
    if (bUpdateState)
    {
        m_fStateChangeAge = fHUGE;

        // Update states and bounds.
        if (!m_Containers.empty())
        {
            UpdateContainers();
        }
        else
        {
            AllocEmitters();
        }

        if (bUpdateBounds && !(m_nEmitterFlags & ePEF_Nowhere))
        {
            // Re-query environment to include full bounds, and water volumes for clipping.
            m_VisEnviron.Update(GetLocation().t, m_bbWorld);
            m_PhysEnviron.GetPhysAreas(CParticleManager::Instance()->GetPhysEnviron(), m_bbWorld,
                m_VisEnviron.OriginIndoors(), m_nEnvFlags | ENV_WATER, true, this);
        }
        // Update phys area changed proxy
        if (m_pRNTmpData && m_pRNTmpData->nPhysAreaChangedProxyId != ~0)
        {
            Get3DEngine()->GetPhysicsAreaUpdates().UpdateProxy(this, m_pRNTmpData->nPhysAreaChangedProxyId);
        }
    }
}

void CParticleEmitter::Activate(bool bActive)
{
    float fPrevAge = m_fAge;
    if (bActive)
    {
        Start(min(-m_fAge, 0.f));
    }
    else
    {
        Stop();
    }
    UpdateTimes(m_fAge - fPrevAge);
}

void CParticleEmitter::Restart()
{
    //clean all the containers.
    Reset();

    float fPrevAge = m_fAge;
    Start();
    UpdateTimes(m_fAge - fPrevAge);
}

void CParticleEmitter::UpdateTimes(float fAgeAdjust)
{
    if (m_Containers.empty() && m_pTopEffect)
    {
        // Set death age to effect maximum.
        m_fDeathAge = m_pTopEffect->CalcEffectLife(FMaxEffectLife().bAllChildren(1).fEmitterMaxLife(m_fStopAge).bParticleLife(1).bPreviewMode(GetPreviewMode()));
        m_fStateChangeAge = m_fAge;
    }
    else
    {
        m_fDeathAge = 0.f;
        m_fStateChangeAge = fHUGE;

        // Update subemitter timings, and compute exact death age.
        for_all_ptrs (CParticleContainer, c, m_Containers)
        {
            c->UpdateContainerLife(fAgeAdjust);
            m_fDeathAge = max(m_fDeathAge, c->GetContainerLife());
            m_fStateChangeAge = min(m_fStateChangeAge, c->GetContainerLife());
        }
    }

    m_fAgeLastRendered = min(m_fAgeLastRendered, m_fAge);
    UpdateResetAge();
}

void CParticleEmitter::Kill()
{
    Reset();
    if (m_fDeathAge >= 0.f)
    {
        CParticleSource::Kill();
        m_fDeathAge = -fHUGE;
    }
}

void CParticleEmitter::UpdateResetAge()
{
    m_fResetAge = m_fAge + fUNSEEN_EMITTER_RESET_TIME;
}

void CParticleEmitter::Prime()
{
    if (GetAge() >= GetStopAge())
    {
        return;
    }

    float fEqTime = m_pTopEffect->GetEquilibriumAge(true, GetPreviewMode());
    Start(-fEqTime);

    m_isPrimed = true;
}

void CParticleEmitter::UpdateEmitCountScale()
{
    m_fEmitCountScale = m_SpawnParams.fCountScale * GetCVars()->e_ParticlesLod;
    if (m_SpawnParams.bCountPerUnit)
    {
        // Count multiplied by geometry extent.
        m_fEmitCountScale *= GetEmitGeom().GetExtent(m_SpawnParams.eAttachForm);
        m_fEmitCountScale *= ScaleExtent(m_SpawnParams.eAttachForm, GetLocation().s);
    }
}

CParticleContainer* CParticleEmitter::AddContainer(CParticleContainer* pParentContainer, const CParticleEffect* pEffect, CLodInfo* startLod, CLodInfo* endLod)
{
    void* pMem = m_Containers.push_back_new();
    if (pMem)
    {
        return new(pMem) CParticleContainer(pParentContainer, this, pEffect, startLod, endLod);
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// CParticleEmitter implementation.
////////////////////////////////////////////////////////////////////////
CParticleEmitter::CParticleEmitter(const IParticleEffect* pEffect, const QuatTS& loc, uint32 uEmitterFlags, const SpawnParams* pSpawnParams)
    : m_fMaxParticleSize(0.f)
    , m_fEmitCountScale(1.f)
    , m_fDeathAge(0.f)
    , m_fStateChangeAge(0.f)
    , m_fAgeLastRendered(0.f)
    , m_fBoundsStableAge(0.f)
    , m_fResetAge(fUNSEEN_EMITTER_RESET_TIME)
    , m_nEmitterFlags(uEmitterFlags)
    , m_pJobExecutorParticles(nullptr)
    , m_isPrimed(false)
{
    m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP;

    m_nEntityId = 0;
    m_nEntitySlot = -1;
    m_nEnvFlags = 0;
    m_bbWorld.Reset();

    m_Loc = loc;
    if ((m_nEmitterFlags & ePEF_IgnoreRotation) != 0)
    {
        m_Loc.q.SetIdentity();
    }

    if (pSpawnParams)
    {
        m_SpawnParams = *pSpawnParams;
    }

    GetInstCount(GetRenderNodeType())++;

    SetEffect(pEffect);
}

//////////////////////////////////////////////////////////////////////////
CParticleEmitter::~CParticleEmitter()
{
    m_p3DEngine->FreeRenderNodeState(this); // Also does unregister entity.
    GetInstCount(GetRenderNodeType())--;
    GetEmitGeom().Release();
}

//////////////////////////////////////////////////////////////////////////
void CParticleEmitter::Register(bool b, bool bImmediate)
{
    if (!b)
    {
        if (m_nEmitterFlags & ePEF_Registered)
        {
            bImmediate ? m_p3DEngine->UnRegisterEntityDirect(this) : m_p3DEngine->UnRegisterEntityAsJob(this);
            m_nEmitterFlags &= ~ePEF_Registered;
        }
    }
    else
    {
        // Register top-level render node if applicable.
        if (m_nEmitterFlags & ePEF_Nowhere)
        {
            bImmediate ? m_p3DEngine->UnRegisterEntityDirect(this) : m_p3DEngine->UnRegisterEntityAsJob(this);
        }
        else if (!(m_nEmitterFlags & ePEF_Registered))
        {
            if (!m_bbWorld.IsReset())
            {
                // Register node.
                // Normally, use emitter's designated position for visibility.
                // However, if all bounds are computed dynamically, they might not contain the origin, so use bounds for visibility.
                if (m_bbWorld.IsContainPoint(GetPos()))
                {
                    SetRndFlags(ERF_REGISTER_BY_POSITION, true);
                }
                else
                {
                    SetRndFlags(ERF_REGISTER_BY_POSITION, false);
                }
                if (m_SpawnParams.bRegisterByBBox)
                {
                    SetRndFlags(ERF_REGISTER_BY_BBOX, true);
                }
                m_p3DEngine->RegisterEntity(this);
                m_nEmitterFlags |= ePEF_Registered;
            }
            else if (m_nEmitterFlags & ePEF_Registered)
            {
                bImmediate ? m_p3DEngine->UnRegisterEntityDirect(this) : m_p3DEngine->UnRegisterEntityAsJob(this);
                m_nEmitterFlags &= ~ePEF_Registered;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
string CParticleEmitter::GetDebugString(char type) const
{
    string s = GetName();
    if (type == 's')
    {
        // Serialization debugging.
        IEntity* pEntity = GetEntity();
        if (pEntity)
        {
            s += string().Format(" entity=%s slot=%d", pEntity->GetName(), m_nEntitySlot);
        }
        if (IsIndependent())
        {
            s += " indep";
        }
        if (IsActive())
        {
            s += " Active";
        }
        else if (IsAlive())
        {
            s += " Dormant";
        }
        else
        {
            s += " Dead";
        }
    }
    else
    {
        SParticleCounts counts;
        GetCounts(counts);
        s += string().Format(" E=%.0f P=%.0f R=%0.f", counts.EmittersActive, counts.ParticlesActive, counts.ParticlesRendered);
    }

    s += string().Format(" age=%.3f", GetAge());
    return s;
}

//////////////////////////////////////////////////////////////////////////
void CParticleEmitter::AddEffect(CParticleContainer* pParentContainer, const CParticleEffect* pEffect, bool bUpdate, bool bInEditor, CLodInfo* startLod, CLodInfo* endLod)
{
    CLodInfo* _startLod = startLod;
    CLodInfo* _endLod = endLod;

    if (pEffect->HasLevelOfDetail() && !(m_nEmitterFlags & ePEF_DisableLOD))
    {
        //We take the first active lod and give it as the end lod for the base particle.
        //This ensures that this particle will be faded out at the proper camera distance.
        for (int i = 0; i < pEffect->GetLevelOfDetailCount(); i++)
        {
            if ((pEffect->GetLevelOfDetail(i)->GetActive() || GetPreviewMode()) &&
                pEffect->GetLodParticle(pEffect->GetLevelOfDetail(i))->GetParticleParams().bEnabled)
            {
                _endLod = static_cast<CLodInfo*>(pEffect->GetLevelOfDetail(i));
                break;
            }
        }
    }

    CParticleContainer* pContainer = pParentContainer;

    // Add the effect if it is enabled and correct for the current spec, or if it is enabled and is located in
    // the preview window, where we want it to be visible regardless of config
    if (pEffect->IsActive() || pEffect->HasParams() && pEffect->GetParams().bEnabled && GetPreviewMode())
    {
        if (IsIndependent() && pEffect->GetParams().IsImmortal())
        {
#ifndef _RELEASE
            // Do not allow immortal effects on independent emitters.
            static std::set<const CParticleEffect*, std::less<const CParticleEffect*>, stl::STLGlobalAllocator<const CParticleEffect*> > s_setWarnings;
            if (s_setWarnings.insert(pEffect).second)
            {
                Warning("Ignoring spawning of immortal independent particle sub-effect %s",
                    (pEffect ? pEffect->GetFullName().c_str() : "[programmatic]"));
            }
#endif
        }
        else
        {
            CParticleContainer* pNext = 0;
            if (bUpdate)
            {
                // Look for existing container
                for_all_ptrs (CParticleContainer, c, m_Containers)
                {
                    if (!c->IsUsed())
                    {
                        // Prevent deletion of indirect particles when in editor mode when bInEditor is set. This happens only in the editor
                        // when parameters are being tweaked, and not in-game
                        if (c->GetEffect() == pEffect && ((c->IsIndirect() == !!pEffect->GetIndirectParent()) || bInEditor))
                        {
                            pContainer = c;
                            pContainer->SetUsed(true);
                            pContainer->OnEffectChange();
                            pContainer->InvalidateStaticBounds();

                            pContainer->SetEndLod(_endLod);
                            pContainer->SetStartLod(_startLod);

                            break;
                        }
                        if (!pNext)
                        {
                            pNext = c;
                        }
                    }
                }
            }
            if (pContainer == pParentContainer)
            {
                // Add new container
                if (void* pMem = pNext ? m_Containers.insert_new(pNext) : m_Containers.push_back_new())
                {
                    pContainer = new(pMem) CParticleContainer(pParentContainer, this, pEffect, _startLod, _endLod);
                }
                else
                {
                    return;
                }
            }
        }
    }

    // Recurse effect tree.
    if (pEffect)
    {
        for (int i = 0, n = pEffect->GetChildCount(); i < n; i++)
        {
            if (CParticleEffect const* pChild = static_cast<const CParticleEffect*>(pEffect->GetChild(i)))
            {
                AddEffect(pContainer, pChild, bUpdate, bInEditor, _startLod, _endLod);
            }
        }
    }
    //Add lod effects only if this effect is the top effect. The children lod effects will be added through 
    //the top effect's lod effect since they are the children of that lod effect.
    if (pEffect->HasLevelOfDetail() && !(m_nEmitterFlags & ePEF_DisableLOD) && pEffect->GetLevelOfDetail(0)->GetTopParticle() == pEffect)
    {
        for (int i = 0; i < pEffect->GetLevelOfDetailCount(); i++)
        {
            //If The particle is inactive, we can ignore it.
            if ((pEffect->GetLevelOfDetail(i)->GetActive() == false && !GetPreviewMode()) ||
                pEffect->GetLodParticle(pEffect->GetLevelOfDetail(i))->GetParticleParams().bEnabled == false)
            {
                continue;
            }

            _endLod = nullptr;
            _startLod = static_cast<CLodInfo*>(pEffect->GetLevelOfDetail(i));

            //Find the next active lod.
            for (int u = i + 1; u < pEffect->GetLevelOfDetailCount(); u++)
            {
                if ((pEffect->GetLevelOfDetail(u)->GetActive() || GetPreviewMode()) && pEffect &&
                    pEffect->GetLodParticle(pEffect->GetLevelOfDetail(u))->GetParticleParams().bEnabled)
                {
                    _endLod = static_cast<CLodInfo*>(pEffect->GetLevelOfDetail(u));
                    break;
                }
            }

            AddEffect(pParentContainer, static_cast<const CParticleEffect*>(pEffect->GetLodParticle(pEffect->GetLevelOfDetail(i))), bUpdate, bInEditor, _startLod, _endLod);
		}
	}
}

void CParticleEmitter::RefreshEffect(bool recreateContainer)
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    if (!m_pTopEffect)
    {
        return;
    }

    m_pTopEffect->LoadResources(true);

    if (recreateContainer)
    {
        m_Containers.clear();
    }

    if (m_Containers.empty())
    {
        // Create new containers
        AddEffect(0, m_pTopEffect, false);
    }
    else
    {
        // Make sure the particle update isn't running anymore when we update an effect
        SyncUpdateParticlesJob();

        // Update existing containers
        for_all_ptrs (CParticleContainer, c, m_Containers)
        {
            c->SetUsed(false);
        }

        AddEffect(0, m_pTopEffect, true, gEnv->IsEditing());

        // Remove unused containers, in reverse order, children before parents
        for_rev_all_ptrs (CParticleContainer, c, m_Containers)
        {
            if (!c->IsUsed())
            {
                m_Containers.erase(c);
            }
        }
    }

    m_nEnvFlags = 0;
    m_nRenObjFlags = 0;
    m_fMaxParticleSize = 0.f;

    for_all_ptrs (CParticleContainer, c, m_Containers)
    {
        m_nEnvFlags |= c->GetEnvironmentFlags();
        m_nRenObjFlags |= c->GetParams().nRenObjFlags.On;
        m_fMaxParticleSize = max(m_fMaxParticleSize, c->GetParams().GetMaxParticleSize() * c->GetParams().fViewDistanceAdjust);
        if (c->GetParams().fMinPixels)
        {
            m_fMaxParticleSize = fHUGE;
        }
    }

    m_bbWorld.Reset();
    m_VisEnviron.Invalidate();
    m_PhysEnviron.Clear();

    // Force state update.
    m_fStateChangeAge = -fHUGE;
    m_fDeathAge = fHUGE;
    m_nEmitterFlags |= ePEF_NeedsEntityUpdate;
}

void CParticleEmitter::SetEffect(IParticleEffect const* pEffect)
{
    if (m_pTopEffect != pEffect)
    {
        Reset();
        m_pTopEffect = &non_const(*static_cast<CParticleEffect const*>(pEffect));
        RefreshEffect();
    }
}

void CParticleEmitter::CreateIndirectEmitters(CParticleSource* pSource, CParticleContainer* pCont)
{
    for_all_ptrs (CParticleContainer, pChild, m_Containers)
    {
        if (pChild->GetParent() == pCont)
        {
            // Found an indirect container for this emitter.
            if (!pChild->AddEmitter(pSource))
            {
                break;
            }
        }
    }
}

void CParticleEmitter::SetLocation(const QuatTS& loc)
{
    QuatTS temp = loc;
    if ((m_nEmitterFlags & ePEF_NotAttached) != 0)
    {
        return;
    }

    if ((m_nEmitterFlags & ePEF_IgnoreRotation) != 0)
    {
        temp.q.SetIdentity();
    }

    if (!QuatTS::IsEquivalent(GetLocation(), temp, 0.0045f, 1e-5f))
    {
        InvalidateStaticBounds();
        m_VisEnviron.Invalidate();
    }

    if (!(m_nEmitterFlags & ePEF_HasPhysics))
    {
        // Infer velocity from movement, smoothing over time.
        if (float fStep = GetParticleTimer()->GetFrameTime())
        {
            Velocity3 vv;
            vv.FromDelta(QuatT(GetLocation().q, GetLocation().t), QuatT(temp.q, temp.t), max(fStep, fVELOCITY_SMOOTHING_TIME));
            m_Vel += vv;
        }
    }

    m_Loc = temp;
}

void CParticleEmitter::ResetUnseen()
{
    if (!(GetEnvFlags() & (EFF_DYNAMIC_BOUNDS | EFF_ANY)))
    {
        // Can remove all emitter structures.
        m_Containers.clear();
    }
    else
    {
        // Some containers require update every frame; reset the others.
        for_rev_all_ptrs (CParticleContainer, c, m_Containers)
        if (!c->NeedsDynamicBounds())
        {
            c->Reset();
        }
    }
}

void CParticleEmitter::AllocEmitters()
{
    // (Re)alloc cleared emitters as needed.
    if (m_Containers.empty())
    {
        SyncUpdateParticlesJob();

        AddEffect(0, m_pTopEffect, false);
        UpdateContainers();
    }
}

void CParticleEmitter::UpdateContainers()
{
    AABB bbPrev = m_bbWorld;
    m_bbWorld.Reset();
    m_fDeathAge = 0.f;
    for_all_ptrs (CParticleContainer, c, m_Containers)
    {
        c->UpdateState();
        float fContainerLife = c->GetContainerLife();
        m_fDeathAge = max(m_fDeathAge, fContainerLife);
        if (fContainerLife > m_fAge)
        {
            m_fStateChangeAge = min(m_fStateChangeAge, fContainerLife);
        }

        if (c->GetEnvironmentFlags() & REN_ANY)
        {
            m_bbWorld.Add(c->GetBounds());
        }

        if (m_isPrimed)
        {
            EnsureGpuDataCreated(c);
            CParticleContainerGPU* gpuData = c->GetGPUData();
            if (gpuData)
            {
                gpuData->Prime(m_fAge);
            }
        }
    }
    if (!IsEquivalent(bbPrev, m_bbWorld))
    {
        Register(false);
    }
}

bool CParticleEmitter::IsInstant() const
{
    return GetStopAge() + GetSpawnParams().fPulsePeriod == 0.f;
}

bool CParticleEmitter::IsImmortal() const
{
    return GetSpawnParams().fPulsePeriod > 0.f || m_pTopEffect->CalcEffectLife(FMaxEffectLife().bAllChildren(1).fEmitterMaxLife(fHUGE).bParticleLife(1).bPreviewMode(GetPreviewMode())) > fHUGE * 0.5f;
}

void CParticleEmitter::SetEmitGeom(GeomRef const& geom)
{
    SEmitGeom emit_new(geom, m_SpawnParams.eAttachType);
    if (emit_new != GetEmitGeom())
    {
        emit_new.AddRef();
        GetEmitGeom().Release();
        static_cast<SEmitGeom&>(*this) = emit_new;
        InvalidateStaticBounds();
    }

    if (m_SpawnParams.eAttachType)
    {
        m_nEmitterFlags |= ePEF_HasAttachment;
        GetEmitGeom().GetExtent(m_SpawnParams.eAttachForm);
    }
    else
    {
        m_nEmitterFlags &= ~ePEF_HasAttachment;
    }
}

void CParticleEmitter::SetSpawnParams(SpawnParams const& spawnParams, GeomRef geom)
{
    if (ZeroIsHuge(spawnParams.fPulsePeriod) < 0.1f)
    {
        Warning("Particle emitter (effect %s) PulsePeriod %g too low to be useful",
            GetName(), spawnParams.fPulsePeriod);
    }
    if (spawnParams.eAttachType != m_SpawnParams.eAttachType ||
        spawnParams.fSizeScale != m_SpawnParams.fSizeScale ||
        spawnParams.fSpeedScale != m_SpawnParams.fSpeedScale)
    {
        InvalidateStaticBounds();
    }

    m_SpawnParams = spawnParams;
    SetEmitGeom(geom);
}


void CParticleEmitter::SetEmitterFlags(uint32 flags)
{
    m_nEmitterFlags = flags;

    if ((m_nEmitterFlags & ePEF_IgnoreRotation) != 0)
    {
        m_Loc.q.SetIdentity();
    }
}

IEntity* CParticleEmitter::GetEntity() const
{
    if (m_nEntityId)
    {
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_nEntityId);
        assert(pEntity);
        return pEntity;
    }
    return NULL;
}

bool GetPhysicalVelocity(Velocity3& Vel, IEntity* pEnt, const Vec3& vPos)
{
    if (pEnt)
    {
        if (IPhysicalEntity* pPhysEnt = pEnt->GetPhysics())
        {
            pe_status_dynamics dyn;
            if (pPhysEnt->GetStatus(&dyn))
            {
                Vel.vLin = dyn.v;
                Vel.vRot = dyn.w;
                Vel.vLin = Vel.VelocityAt(vPos - dyn.centerOfMass);
                return true;
            }
        }
        if (pEnt = pEnt->GetParent())
        {
            return GetPhysicalVelocity(Vel, pEnt, vPos);
        }
    }
    return false;
}

void CParticleEmitter::OnEntityEvent(IEntity* pEntity, SEntityEvent const& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_ATTACH_THIS:
    case ENTITY_EVENT_DETACH_THIS:
    case ENTITY_EVENT_LINK:
    case ENTITY_EVENT_DELINK:
    case ENTITY_EVENT_ENABLE_PHYSICS:
    case ENTITY_EVENT_PHYSICS_CHANGE_STATE:
        m_nEmitterFlags |= ePEF_NeedsEntityUpdate;
        m_Vel = ZERO;
        break;
    }
}

void CParticleEmitter::UpdateFromEntity()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    m_nEmitterFlags &= ~(ePEF_HasPhysics | ePEF_HasTarget | ePEF_HasAttachment);

    // Get emitter entity.
    IEntity* pEntity = GetEntity();

    // Set external target.
    if (!m_Target.bPriority)
    {
        ParticleTarget target;
        if (pEntity)
        {
            for (IEntityLink* pLink = pEntity->GetEntityLinks(); pLink; pLink = pLink->next)
            {
                if (!_strnicmp(pLink->name, "Target", 6) || !_strnicmp(pLink->name, "Target-", 7))
                {
                    if (IEntity* pTarget = gEnv->pEntitySystem->GetEntity(pLink->entityId))
                    {
                        target.bTarget = true;
                        target.vTarget = pTarget->GetPos();

                        Velocity3 Vel(ZERO);
                        GetPhysicalVelocity(Vel, pTarget, GetLocation().t);
                        target.vVelocity = Vel.vLin;

                        AABB bb;
                        pTarget->GetLocalBounds(bb);
                        target.fRadius = max(bb.min.len(), bb.max.len());
                        m_nEmitterFlags |= ePEF_HasTarget;

                        if (target.bTarget != m_Target.bTarget || target.vTarget != m_Target.vTarget)
                        {
                            InvalidateStaticBounds();
                        }
                        m_Target = target;

                        break;
                    }
                }
            }
        }
    }

    bool bShadows = (GetEnvFlags() & REN_CAST_SHADOWS) != 0;

    // Get entity of attached parent.
    if (pEntity)
    {
        if (!(pEntity->GetFlags() & ENTITY_FLAG_CASTSHADOW))
        {
            bShadows = false;
        }

        if (pEntity->GetPhysics())
        {
            m_nEmitterFlags |= ePEF_HasPhysics;
        }

        if (pEntity->GetParent())
        {
            pEntity = pEntity->GetParent();
        }

        if (pEntity->GetPhysics())
        {
            m_nEmitterFlags |= ePEF_HasPhysics;
        }

        if (m_SpawnParams.eAttachType != GeomType_None)
        {
            // If entity attached, find attached physics and geometry on parent.
            GeomRef geom;

            int nStart = m_nEntitySlot < 0 ? 0 : m_nEntitySlot;
            int nEnd = m_nEntitySlot < 0 ?  pEntity->GetSlotCount() : m_nEntitySlot + 1;
            for (int nSlot = nStart; nSlot < nEnd; nSlot++)
            {
                SEntitySlotInfo slotInfo;
                if (pEntity->GetSlotInfo(nSlot, slotInfo))
                {
                    geom.m_pStatObj = slotInfo.pStatObj;
                    geom.m_pChar = slotInfo.pCharacter;

                    if (geom)
                    {
                        if (slotInfo.pWorldTM)
                        {
                            SetMatrix(*slotInfo.pWorldTM);
                        }
                        break;
                    }
                }
            }

            geom.m_pPhysEnt = pEntity->GetPhysics();

            SetEmitGeom(geom);
        }
    }

    if (bShadows)
    {
        SetRndFlags(ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, true);
    }
    else
    {
        SetRndFlags(ERF_CASTSHADOWMAPS, false);
    }
}

void CParticleEmitter::GetLocalBounds(AABB& bbox)
{
    if (!m_bbWorld.IsReset())
    {
        bbox.min = m_bbWorld.min - GetLocation().t;
        bbox.max = m_bbWorld.max - GetLocation().t;
    }
    else
    {
        bbox.min = bbox.max = Vec3(0);
    }
}

void CParticleEmitter::Update()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    m_fAge += GetParticleTimer()->GetFrameTime() * m_SpawnParams.fTimeScale + m_SpawnParams.fStepValue;

    if (m_SpawnParams.fPulsePeriod > 0.f && m_fAge < m_fStopAge)
    {
        // Apply external pulsing (restart).
        if (m_fAge >= m_SpawnParams.fPulsePeriod)
        {
            float fPulseAge = m_fAge - fmodf(m_fAge, m_SpawnParams.fPulsePeriod);
            m_fAge -= fPulseAge;
            m_fStopAge -= fPulseAge;
            UpdateTimes(-fPulseAge);
        }
    }
    else if (m_fAge > m_fDeathAge)
    {
        return;
    }

    #ifdef SHARED_GEOM
    m_InstInfos.clear();
    #endif

    if (m_nEmitterFlags & (ePEF_NeedsEntityUpdate | ePEF_HasTarget | ePEF_HasAttachment))
    {
        m_nEmitterFlags &= ~ePEF_NeedsEntityUpdate;
        UpdateFromEntity();
    }

    if (m_fAge > m_fResetAge)
    {
        ResetUnseen();
        m_fResetAge = fHUGE;
    }

    // Update velocity
    Velocity3 Vel;
    if ((m_nEmitterFlags & ePEF_HasPhysics) && GetPhysicalVelocity(Vel, GetEntity(), GetLocation().t))
    {
        // Get velocity from physics.
        m_Vel = Vel;
    }
    else
    {
        // Decay velocity, using approx exponential decay, so we reach zero soon.
        float fDecay = max(1.f - GetParticleTimer()->GetFrameTime() / fVELOCITY_SMOOTHING_TIME, 0.f);
        m_Vel *= fDecay;
    }

    if (!(m_nEmitterFlags & ePEF_Nowhere))
    {
        //Only bind our matrix to the camera if we aren't in a previewer window (e.g. Particle Editor/Mannequin).  Otherwise, the particles don't preview correctly.
        if ((m_nEnvFlags & REN_BIND_CAMERA) && gEnv->pSystem)
        {
            SetMatrix(gEnv->pSystem->GetViewCamera().GetMatrix());
        }
    }

    UpdateEmitCountScale();

    // Update emitter state and bounds.
    UpdateState();

    m_isPrimed = false;
}

void CParticleEmitter::UpdateEffects()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    for_all_ptrs (CParticleContainer, c, m_Containers)
    {
        if (c->GetEnvironmentFlags() & EFF_ANY)
        {
            c->UpdateEffects();
        }
    }
}

void CParticleEmitter::EmitParticle(const EmitParticleData* pData)
{
    AllocEmitters();
    if (!m_Containers.empty())
    {
        m_Containers.front().EmitParticle(pData);
        UpdateResetAge();
        m_fStateChangeAge = m_fAge;
    }
}

void CParticleEmitter::SetEntity(IEntity* pEntity, int nSlot)
{
    m_nEntityId = pEntity ? pEntity->GetId() : 0;
    m_nEntitySlot = nSlot;
    m_nEmitterFlags |= ePEF_NeedsEntityUpdate;

    for_all_ptrs (CParticleContainer, c, m_Containers)
    if (CParticleSubEmitter* e = c->GetDirectEmitter())
    {
        e->ResetLoc();
    }
}

void CParticleEmitter::Render(SRendParams const& RenParams, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_SYS(PARTICLE);
    PARTICLE_LIGHT_PROFILER();

    IF (!passInfo.RenderParticles(), 0)
    {
        return;
    }

    IF (passInfo.IsRecursivePass() && (m_nEnvFlags & REN_BIND_CAMERA), 0)
    {
        // Render only in main camera.
        return;
    }

    if (!IsActive())
    {
        return;
    }

    AllocEmitters();

    // Check combined emitter flags.
    uint32 nRenFlags = REN_SPRITE | REN_GEOMETRY;
    uint32 nRenRequiredFlags = 0;

    if (passInfo.IsShadowPass())
    {
        nRenRequiredFlags |= REN_CAST_SHADOWS;
    }
    else if (!passInfo.IsAuxWindow())
    {
        nRenFlags |= REN_DECAL;
        if (!passInfo.IsRecursivePass())
        {
            nRenFlags |= REN_LIGHTS;
        }
    }

    uint32 nEnvFlags = m_nEnvFlags & CParticleManager::Instance()->GetAllowedEnvironmentFlags();
    nRenFlags &= nEnvFlags;

    if (!nRenFlags)
    {
        return;
    }

    uint32 nRenRequiredCheckFlags = nRenRequiredFlags & ~nRenFlags;
    if ((nRenRequiredFlags & nEnvFlags) != nRenRequiredFlags)
    {
        return;
    }

    if (!passInfo.IsShadowPass())
    {
        m_fAgeLastRendered = GetAge();
        UpdateResetAge();
    }

    SPartRenderParams PartParams;
    ZeroStruct(PartParams);

    ColorF fogVolumeContrib;
    CFogVolumeRenderNode::TraceFogVolumes(GetPos(), fogVolumeContrib, passInfo);
    PartParams.m_nFogVolumeContribIdx = GetRenderer()->PushFogVolumeContribution(fogVolumeContrib, passInfo);

    // Compute camera distance with top effect's SortBoundsScale.
    PartParams.m_fMainBoundsScale = m_pTopEffect->IsEnabled() ? +m_pTopEffect->GetParams().fSortBoundsScale : 1.f;
    if (PartParams.m_fMainBoundsScale == 1.f && RenParams.fDistance > 0.f)
    {
        // Use 3DEngine-computed BB distance.
        PartParams.m_fCamDistance = RenParams.fDistance;
    }
    else
    {
        // Compute with custom bounds scale and adjustment for camera inside BB.
        PartParams.m_fCamDistance = GetNearestDistance(passInfo.GetCamera().GetPosition(), PartParams.m_fMainBoundsScale);
    }

    // Compute max allowed res.
    PartParams.m_fMaxAngularDensity = passInfo.GetCamera().GetAngularResolution() * max(GetCVars()->e_ParticlesMinDrawPixels, 0.125f);
    PartParams.m_nRenObjFlags = CParticleManager::Instance()->GetRenderFlags();

    if (!passInfo.IsAuxWindow())
    {
        PartParams.m_nDeferredLightVolumeId = m_p3DEngine->m_LightVolumesMgr.RegisterVolume(GetPos(), m_bbWorld.GetRadius() * 0.5f, RenParams.nClipVolumeStencilRef, passInfo);
    }
    else
    {
        // Emitter in preview window, etc
        PartParams.m_nRenObjFlags.SetState(-1, FOB_GLOBAL_ILLUMINATION | FOB_PARTICLE_SHADOWS | FOB_SOFT_PARTICLE | FOB_MOTION_BLUR);
        CParticleManager::Instance()->CreateLightShader();
    }

    if (!PartParams.m_nDeferredLightVolumeId)
    {
        PartParams.m_nRenObjFlags.SetState(-1, FOB_LIGHTVOLUME);
    }
    if (RenParams.m_pVisArea && !RenParams.m_pVisArea->IsAffectedByOutLights())
    {
        PartParams.m_nRenObjFlags.SetState(-1, FOB_IN_DOORS | FOB_PARTICLE_SHADOWS);
    }
    if (RenParams.dwFObjFlags & FOB_NEAREST)
    {
        PartParams.m_nRenObjFlags.SetState(-1, FOB_SOFT_PARTICLE);
    }

    if (RenParams.m_pVisArea) //&& RenParams.m_pVisArea->IsIgnoringGI()) - ignore GI not used in most vis areas unfortunately, disable GI for any particle inside a vis area
    {
        PartParams.m_nRenObjFlags.SetState(-1, FOB_GLOBAL_ILLUMINATION);
    }

    if (passInfo.IsAuxWindow())
    {
        // Stand-alone rendering.
        CParticleManager::Instance()->PrepareForRender(passInfo);
    }

    PartParams.m_nRenFlags = nRenFlags;
    int nThreadJobs = 0;
    if (!RenParams.mRenderFirstContainer)
    {
        for_all_ptrs(CParticleContainer, c, m_Containers)
        {
            m_Containers.PrefetchLink(m_Containers.next(c));
            PrefetchLine(m_Containers.next(c), 0);
            uint32 nContainerFlags = c->GetEnvironmentFlags();
            if (nContainerFlags & nRenFlags)
            {
                if ((nContainerFlags & nRenRequiredCheckFlags) == nRenRequiredFlags)
                {
                    if (GetAge() <= c->GetContainerLife())
                    {
                        RenderContainer(c, RenParams, PartParams, passInfo);
                        nThreadJobs += c->NeedJobUpdate();
                    }
                }
            }
        }
    }
    else
    {
        if (m_pTopEffect && m_pTopEffect->IsEnabled() && !m_Containers.empty())
        {
            CParticleContainer* selectedContainer = m_Containers.begin();
            
            uint32 nContainerFlags = selectedContainer->GetEnvironmentFlags();
            if (nContainerFlags & nRenFlags)
            {
                if ((nContainerFlags & nRenRequiredCheckFlags) == nRenRequiredFlags)
                {
                    if (GetAge() <= selectedContainer->GetContainerLife())
                    {
                        RenderContainer(selectedContainer, RenParams, PartParams, passInfo);
                        nThreadJobs += selectedContainer->NeedJobUpdate();
                    }
                }
            }
        }
    }


    if (passInfo.IsAuxWindow())
    {
        // Stand-alone rendering. Submit render object.
        CParticleManager::Instance()->FinishParticleRenderTasks(passInfo);
        RenderDebugInfo();
    }
    else if (nThreadJobs && !m_pJobExecutorParticles)
    {
        // Schedule new emitter update, for containers first rendered this frame.
        AddUpdateParticlesJob();
    }
}

float CParticleEmitter::GetNearestDistance(const Vec3& vPos, float fBoundsScale) const
{
    if (fBoundsScale == 0.f)
    {
        return (vPos - m_Loc.t).GetLength();
    }

    AABB bb;
    float fAbsScale = abs(fBoundsScale);
    if (fAbsScale == 1.f)
    {
        bb = m_bbWorld;
    }
    else
    {
        // Scale toward origin
        bb.min = Lerp(m_Loc.t, m_bbWorld.min, fAbsScale);
        bb.max = Lerp(m_Loc.t, m_bbWorld.max, fAbsScale);
    }
    bb.min -= vPos;
    bb.max -= vPos;

    if (fBoundsScale < 0.f)
    {
        // Find furthest point.
        Vec3 vFar;
        vFar.x = max(abs(bb.min.x), abs(bb.max.x));
        vFar.y = max(abs(bb.min.y), abs(bb.max.y));
        vFar.z = max(abs(bb.min.z), abs(bb.max.z));
        return vFar.GetLength();
    }

    // Find nearest point.
    Vec3 vNear(ZERO);
    vNear.CheckMin(bb.max);
    vNear.CheckMax(bb.min);
    float fDistanceSq = vNear.GetLengthSquared();

    if (fDistanceSq > 0.f)
    {
        return sqrt_tpl(fDistanceSq);
    }
    else
    {
        // When point inside bounds, return negative distance, for stable sorting.
        vNear = -bb.min;
        vNear.CheckMin(bb.max);
        float fMinDist = min(vNear.x, min(vNear.y, vNear.z));
        return -fMinDist;
    }
}

// Disable printf argument verification since it is generated at runtime
#if defined(__GNUC__)
#if __GNUC__ >= 4 && __GNUC__MINOR__ < 7
    #pragma GCC diagnostic ignored "-Wformat-security"
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif
#endif

void CParticleEmitter::RenderDebugInfo()
{
    if (TimeNotRendered() < 0.25f)
    {
        IRenderAuxGeom* pRenAux = GetRenderer()->GetIRenderAuxGeom();
        pRenAux->SetRenderFlags(SAuxGeomRenderFlags());

        bool bSelected = IsEditSelected();
        if (bSelected || (GetCVars()->e_ParticlesDebug & AlphaBit('b')))
        {
            AABB const& bb = GetBBox();
            if (bb.IsReset())
            {
                return;
            }

            AABB bbDyn;
            GetDynamicBounds(bbDyn);

            CCamera const& cam = GetRenderer()->GetCamera();
            ColorF color(1, 1, 1, 1);

            // Compute label position, in bb clipped to camera.
            Vec3 vPos = GetLocation().t;
            Vec3 vLabel = vPos;

            // Clip to cam frustum.
            float fBorder = 1.f;
            for (int i = 0; i < FRUSTUM_PLANES; i++)
            {
                Plane const& pl = *cam.GetFrustumPlane(i);
                float f = pl.DistFromPlane(vLabel) + fBorder;
                if (f > 0.f)
                {
                    vLabel -= pl.n * f;
                }
            }

            Vec3 vDist = vLabel - cam.GetPosition();
            vLabel += (cam.GetViewdir() * (vDist * cam.GetViewdir()) - vDist) * 0.1f;
            vLabel.CheckMax(bbDyn.min);
            vLabel.CheckMin(bbDyn.max);

            SParticleCounts counts;
            GetCounts(counts);
            float fPixToScreen = 1.f / ((float)GetRenderer()->GetWidth() * (float)GetRenderer()->GetHeight());

            if (!bSelected)
            {
                // Randomize hue by effect.
                color = CryStringUtils::CalculateHash(GetEffect()->GetName());
                color.r = color.r * 0.5f + 0.5f;
                color.g = color.g * 0.5f + 0.5f;
                color.b = color.b * 0.5f + 0.5f;

                // Modulate alpha by screen-space bounds extents.
                int aiOut[4];
                cam.CalcScreenBounds(aiOut, &bbDyn, GetRenderer()->GetWidth(), GetRenderer()->GetHeight());
                float fCoverage = (aiOut[2] - aiOut[0]) * (aiOut[3] - aiOut[1]) * fPixToScreen;

                // And by pixel and particle counts.
                float fWeight = sqrt_fast_tpl(fCoverage * counts.ParticlesRendered * counts.PixelsRendered * fPixToScreen);

                color.a = clamp_tpl(fWeight, 0.2f, 1.f);
            }

            stack_string sLabel = GetName();
            sLabel += stack_string().Format(" P=%.0f F=%.3f S/D=",
                    counts.ParticlesRendered,
                    counts.PixelsRendered * fPixToScreen);
            if (counts.DynamicBoundsVolume > 0.f)
            {
                sLabel += stack_string().Format("%.2f", counts.StaticBoundsVolume / counts.DynamicBoundsVolume);
                if (counts.ErrorBoundsVolume > 0.f)
                {
                    sLabel += stack_string().Format(" E/D=%3f", counts.ErrorBoundsVolume / counts.DynamicBoundsVolume);
                }
            }
            else
            {
                sLabel += "~0";
            }

            if (counts.ParticlesCollideTest)
            {
                sLabel += stack_string().Format(" Col=%.0f/%.0f", counts.ParticlesCollideHit, counts.ParticlesCollideTest);
            }
            if (counts.ParticlesClip)
            {
                sLabel += stack_string().Format(" Clip=%.0f", counts.ParticlesClip);
            }

            ColorF colLabel = color;
            if (!bb.ContainsBox(bbDyn))
            {
                // Tint red.
                colLabel.r = 1.f;
                colLabel.g *= 0.5f;
                colLabel.b *= 0.5f;
                colLabel.a = (color.a + 1.f) * 0.5f;
            }
            else if (m_nEnvFlags & EFF_DYNAMIC_BOUNDS)
            {
                // Tint yellow.
                colLabel.r = 1.f;
                colLabel.g = 1.f;
                colLabel.b *= 0.5f;
            }
            GetRenderer()->DrawLabelEx(vLabel, 1.5f, (float*)&colLabel, true, true, sLabel);

            // Compare static and dynamic boxes.
            if (!bbDyn.IsReset())
            {
                if (bbDyn.min != bb.min || bbDyn.max != bb.max)
                {
                    // Separate dynamic BB.
                    // Color outlying points bright red, draw connecting lines.
                    ColorF colorGood = color * 0.8f;
                    ColorF colorBad = colLabel;
                    colorBad.a = colorGood.a;
                    Vec3 vStat[8], vDyn[8];
                    ColorB clrDyn[8];
                    for (int i = 0; i < 8; i++)
                    {
                        vStat[i] = Vec3(i & 1 ? bb.max.x : bb.min.x, i & 2 ? bb.max.y : bb.min.y, i & 4 ? bb.max.z : bb.min.z);
                        vDyn[i] = Vec3(i & 1 ? bbDyn.max.x : bbDyn.min.x, i & 2 ? bbDyn.max.y : bbDyn.min.y, i & 4 ? bbDyn.max.z : bbDyn.min.z);
                        clrDyn[i] = bb.IsContainPoint(vDyn[i]) ? colorGood : colorBad;
                        pRenAux->DrawLine(vStat[i], color, vDyn[i], clrDyn[i]);
                    }

                    // Draw dyn bb.
                    if (bb.ContainsBox(bbDyn))
                    {
                        pRenAux->DrawAABB(bbDyn, false, colorGood, eBBD_Faceted);
                    }
                    else
                    {
                        pRenAux->DrawLine(vDyn[0], clrDyn[0], vDyn[1], clrDyn[1]);
                        pRenAux->DrawLine(vDyn[0], clrDyn[0], vDyn[2], clrDyn[2]);
                        pRenAux->DrawLine(vDyn[0], clrDyn[0], vDyn[4], clrDyn[4]);

                        pRenAux->DrawLine(vDyn[1], clrDyn[1], vDyn[3], clrDyn[3]);
                        pRenAux->DrawLine(vDyn[1], clrDyn[1], vDyn[5], clrDyn[5]);

                        pRenAux->DrawLine(vDyn[2], clrDyn[2], vDyn[3], clrDyn[3]);
                        pRenAux->DrawLine(vDyn[2], clrDyn[2], vDyn[6], clrDyn[6]);

                        pRenAux->DrawLine(vDyn[3], clrDyn[3], vDyn[7], clrDyn[7]);

                        pRenAux->DrawLine(vDyn[4], clrDyn[4], vDyn[5], clrDyn[5]);
                        pRenAux->DrawLine(vDyn[4], clrDyn[4], vDyn[6], clrDyn[6]);

                        pRenAux->DrawLine(vDyn[5], clrDyn[5], vDyn[7], clrDyn[7]);

                        pRenAux->DrawLine(vDyn[6], clrDyn[6], vDyn[7], clrDyn[7]);
                    }
                }
            }

            pRenAux->DrawAABB(bb, false, color, eBBD_Faceted);
        }

        // draw emitter transform
#if 0
        Matrix34 orient(this->m_Loc.q);
        float len = 0.1f;
        float thick = 0.005f;
        pRenAux->DrawCylinder(Vec3_Zero + orient.GetColumn0() * (len * 0.5f), orient.GetColumn0(), thick, len, ColorB(0xff, 0, 0));
        pRenAux->DrawCylinder(Vec3_Zero + orient.GetColumn1() * (len * 0.5f), orient.GetColumn1(), thick, len, ColorB(0, 0xff, 0));
        pRenAux->DrawCylinder(Vec3_Zero + orient.GetColumn2() * (len * 0.5f), orient.GetColumn2(), thick, len, ColorB(0, 0, 0xff));
#endif


        if (bSelected)
        {
            // Draw emission volume(s)
            IRenderAuxGeom* auxRenderer = GetRenderer()->GetIRenderAuxGeom();

            int c = 0;
            for_all_ptrs (const CParticleContainer, pCont, m_Containers)
            {
                if (!pCont->IsIndirect())
                {
                    c++;
                    ColorB color(c & 1 ? 255 : 128, c & 2 ? 255 : 128, c & 4 ? 255 : 128, 128);

                    const ParticleParams& params = pCont->GetEffect()->GetParams();
                    auxRenderer->DrawAABB(params.GetEmitOffsetBounds(), Matrix34(GetLocation()), false, color, eBBD_Faceted);
                }
            }
        }

#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
        // Render refraction partial resolve bounding boxes
        static ICVar* pRefractionPartialResolvesDebugCVar = gEnv->pConsole->GetCVar("r_RefractionPartialResolvesDebug");
        if (pRefractionPartialResolvesDebugCVar && pRefractionPartialResolvesDebugCVar->GetIVal() == eRPR_DEBUG_VIEW_3D_BOUNDS)
        {
            if (IRenderAuxGeom* pAuxRenderer = gEnv->pRenderer->GetIRenderAuxGeom())
            {
                SAuxGeomRenderFlags oldRenderFlags = pAuxRenderer->GetRenderFlags();

                SAuxGeomRenderFlags newRenderFlags;
                newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
                newRenderFlags.SetAlphaBlendMode(e_AlphaBlended);
                pAuxRenderer->SetRenderFlags(newRenderFlags);

                // Render all bounding boxes for containers that have refractive particles
                for_all_ptrs (CParticleContainer, pContainer, m_Containers)
                {
                    if (pContainer->WasRenderedPrevFrame())
                    {
                        const ResourceParticleParams& params = pContainer->GetParams();

                        _smart_ptr<IMaterial> pMatToUse = params.pMaterial;
                        if (!pMatToUse)
                        {
                            pMatToUse = CParticleManager::Instance()->GetLightShader(params.UseTextureMirror());
                        }

                        if (pMatToUse)
                        {
                            IShader* pShader = pMatToUse->GetShaderItem().m_pShader;
                            if (pShader && (pShader->GetFlags() & EF_REFRACTIVE))
                            {
                                const AABB& aabb = pContainer->GetBounds();
                                const bool bSolid = true;
                                const ColorB solidColor(64, 64, 255, 64);
                                pAuxRenderer->DrawAABB(aabb, bSolid, solidColor, eBBD_Faceted);

                                const ColorB wireframeColor(255, 0, 0, 255);
                                pAuxRenderer->DrawAABB(aabb, !bSolid, wireframeColor, eBBD_Faceted);
                            }
                        }
                    }

                    // Set previous Aux render flags back again
                    pAuxRenderer->SetRenderFlags(oldRenderFlags);
                }
            }
        }
#endif
    }
}
#if defined(__GNUC__)
#if __GNUC__ >= 4 && __GNUC__MINOR__ < 7
    #pragma GCC diagnostic error "-Wformat-security"
#else
    #pragma GCC diagnostic pop
#endif
#endif

void CParticleEmitter::SerializeState(TSerialize ser)
{
    // Time values.
    ser.Value("Age", m_fAge);
    ser.Value("StopAge", m_fStopAge);

    // Spawn params.
    ser.Value("SizeScale", m_SpawnParams.fSizeScale);
    ser.Value("SpeedScale", m_SpawnParams.fSpeedScale);
    ser.Value("TimeScale", m_SpawnParams.fTimeScale);
    ser.Value("CountScale", m_SpawnParams.fCountScale);
    ser.Value("CountPerUnit", m_SpawnParams.bCountPerUnit);
    ser.Value("PulsePeriod", m_SpawnParams.fPulsePeriod);
}

void CParticleEmitter::GetMemoryUsage(ICrySizer* pSizer) const
{
    m_Containers.GetMemoryUsage(pSizer);
    m_PhysEnviron.GetMemoryUsage(pSizer);
}

bool CParticleEmitter::UpdateStreamableComponents(float fImportance, Matrix34A& objMatrix, IRenderNode* pRenderNode, float fEntDistance, bool bFullUpdate, int nLod)
{
    FUNCTION_PROFILER_3DENGINE;

    IRenderer* pRenderer = GetRenderer();
    for_all_ptrs (const CParticleContainer, c, m_Containers)
    {
        ResourceParticleParams const& params = c->GetParams();
        if (params.pStatObj)
        {
            static_cast<CStatObj*>(params.pStatObj.get())->UpdateStreamableComponents(fImportance, objMatrix, bFullUpdate, nLod);
        }

        if (params.nEnvFlags & REN_SPRITE)
        {
            if (ITexture* pTexture = pRenderer->EF_GetTextureByID(params.nTexId))
            {
                const float minMipFactor = sqr(fEntDistance) / (params.GetFullTextureArea() + 1e-6f);
                pRenderer->EF_PrecacheResource(pTexture, minMipFactor, 0.f, bFullUpdate ? FPR_SINGLE_FRAME_PRIORITY_UPDATE : 0, GetObjManager()->GetUpdateStreamingPrioriryRoundId());
            }
        }

        if (CMatInfo* pMatInfo = (CMatInfo*)params.pMaterial.get())
        {
            pMatInfo->PrecacheMaterial(fEntDistance, NULL, bFullUpdate);
        }
    }

    return true;
}

float CParticleEmitter::GetAliveParticleCount() const
{
    SParticleCounts counts;
    GetCounts(counts);
    return counts.ParticlesActive;
}

void CParticleEmitter::OffsetPosition(const Vec3& delta)
{
    SMoveState::OffsetPosition(delta);
    if (m_pRNTmpData)
    {
        m_pRNTmpData->OffsetPosition(delta);
    }
    m_Target.vTarget += delta;
    m_bbWorld.Move(delta);

    for_all_ptrs (CParticleContainer, c, m_Containers)
    c->OffsetPosition(delta);
}

QuatTS CParticleEmitter::GetLocationQuat() const
{
    return GetLocation();
}

bool CParticleEmitter::GetPreviewMode() const
{
    // We assume not being in the world indicates that the effect is located in the preview window
    const bool previewMode = 0 != (m_nEmitterFlags & ePEF_Nowhere);
    return previewMode;
}
