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

// Description : Manage particle effects and emitters


#include "StdAfx.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ParticleContainer.h"
#include "ParticleMemory.h"
#include "3dEngine.h"

#include <StringUtils.h>
#include <IStatoscope.h>
#include <CryProfileMarker.h>
#include <IZLibCompressor.h>

#define LIBRARY_PATH            "Libs/"
#define EFFECTS_SUBPATH     LIBRARY_PATH "Particles/"
#define LEVEL_PATH              "Levels/"

using namespace minigui;


//////////////////////////////////////////////////////////////////////////
// Single direct entry point for particle clients.
IParticleManager* CreateParticleManager(bool bEnable)
{
    return new CParticleManager(bEnable);
}

//////////////////////////////////////////////////////////////////////////
// CParticleBatchDataManager implementation

void CParticleBatchDataManager::PrepareForRender(const SRenderingPassInfo& passInfo)
{
    if (passInfo.GetRecursiveLevel() == 0)
    {
        m_ParticlesToScene[passInfo.ThreadID()].resize(0);
    }
    m_ParticlesJobStart[passInfo.ThreadID()][passInfo.GetRecursiveLevel()] = m_ParticlesToScene[passInfo.ThreadID()].size();
}
void CParticleBatchDataManager::FinishParticleRenderTasks(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_PARTICLE);
    PARTICLE_LIGHT_PROFILER();
    AZ_TRACE_METHOD();

    // make sure no job/task runs anymore
    SyncAllUpdateParticlesJobs();

    int nJobStart = m_ParticlesJobStart[passInfo.ThreadID()][passInfo.GetRecursiveLevel()];
    if (m_ParticlesToScene[passInfo.ThreadID()].size() > nJobStart)
    {
        GetRenderer()->EF_AddMultipleParticlesToScene(&m_ParticlesToScene[passInfo.ThreadID()][nJobStart], m_ParticlesToScene[passInfo.ThreadID()].size() - nJobStart, passInfo);
    }
}

void CParticleBatchDataManager::GetMemoryUsage(ICrySizer* pSizer) const
{
    for (int t = 0; t < RT_COMMAND_BUF_COUNT; ++t)
    {
        pSizer->AddObject(m_ParticlesToScene[t]);
    }
}

void CParticleBatchDataManager::ResetData()
{
    for (int t = 0; t < RT_COMMAND_BUF_COUNT; ++t)
    {
        stl::free_container(m_ParticlesToScene[t]);
    }

    for (int i = 0, end = m_UpdateParticleJobExecutors.size(); i < end; ++i)
    {
        delete m_UpdateParticleJobExecutors[i];
    }

    stl::free_container(m_UpdateParticleJobExecutors);
    m_nUsedStates = 0;
}

void CParticleBatchDataManager::SyncAllUpdateParticlesJobs()
{
    AZ_TRACE_METHOD();
    for (int i = 0; i < m_nUsedStates; ++i)
    {
        m_UpdateParticleJobExecutors[i]->WaitForCompletion();
    }
    m_nUsedStates = 0;
}

//////////////////////////////////////////////////////////////////////////
ITimer* g_pParticleTimer = NULL;

//////////////////////////////////////////////////////////////////////////
CParticleManager::CParticleManager(bool bEnable)
{
    m_bEnabled = bEnable;
    m_bRegisteredListener = false;
#ifdef bEVENT_TIMINGS
    m_iEventSwitch = 1;
    m_timeThreshold = 0.f;
#endif
    m_pWidget = NULL;
    g_pParticleTimer = gEnv->pTimer;
    m_pLastDefaultParams = &GetDefaultParams();

    if (GetPhysicalWorld())
    {
        GetPhysicalWorld()->AddEventClient(EventPhysAreaChange::id, &StaticOnPhysAreaChange, 0);

        REGISTER_COMMAND("e_ParticleListEmitters", &CmdParticleListEmitters, 0, "Writes all emitters to log");
        REGISTER_COMMAND("e_ParticleListEffects", &CmdParticleListEffects, 0, "Writes all effects used and counts to log");
        REGISTER_COMMAND("e_ParticleMemory", &CmdParticleMemory, 0, "Displays current particle memory usage");
    }

    // reset tracking data for dumping vertex/index pool usage
#if defined(PARTICLE_COLLECT_VERT_IND_POOL_USAGE)
    memset(m_arrVertexIndexPoolUsage, 0, sizeof(m_arrVertexIndexPoolUsage));
    m_bOutOfVertexIndexPoolMemory = false;
    m_nRequieredVertexPoolMemory = 0;
    m_nRequieredIndexPoolMemory = 0;
    m_nRendererParticleContainer = 0;
#endif

    m_nFrameTicks = 0;
    m_nFrameSyncTicks = 0;
    m_nActiveEmitter = 0;

    UpdateEngineData();
}

CParticleManager::~CParticleManager()
{
    if (Get3DEngine()->GetIVisAreaManager())
    {
        Get3DEngine()->GetIVisAreaManager()->RemoveListener(this);
    }
    GetPhysicalWorld()->RemoveEventClient(EventPhysAreaChange::id, &StaticOnPhysAreaChange, 0);
    Reset(false);

    // Destroy references to shaders.
    m_pPartLightShader = nullptr;
    m_partMirrorLightShader = nullptr;

    if (m_pWidget)
    {
        gEnv->pSystem->GetPerfHUD()->RemoveWidget(m_pWidget);
    }
}

struct SortEffectStats
{
    typedef CParticleManager::TEffectStats::value_type SEffectStatEntry;

    float SParticleCounts::* _pSortField;

    SortEffectStats(float SParticleCounts::* pSortField)
        : _pSortField(pSortField) {}

    bool operator()(const SEffectStatEntry& a, const SEffectStatEntry& b) const
    {
        return a.second.*_pSortField > b.second.*_pSortField;
    }
};

void CParticleManager::CollectEffectStats(TEffectStats& mapEffectStats, float SParticleCounts::* pSortField) const
{
    SParticleCounts countsTotal;
    for_all_ptrs (const CParticleEmitter, e, m_Emitters)
    {
        for_all_ptrs (const CParticleContainer, c, e->GetContainers())
        {
            SParticleCounts countsEmitter;
            c->GetCounts(countsEmitter);
            SParticleCounts& countsEffect = mapEffectStats[c->GetEffect()];
            AddArray(FloatArray(countsEffect), FloatArray(countsEmitter));
            AddArray(FloatArray(countsTotal), FloatArray(countsEmitter));
        }
    }

    // Add total to list.
    mapEffectStats[NULL] = countsTotal;

    // Re-sort by selected stat.
    std::sort(mapEffectStats.begin(), mapEffectStats.end(), SortEffectStats(pSortField));
}

void CParticleManager::GetCounts(SParticleCounts& counts)
{
    counts = m_GlobalCounts;
}

void CParticleManager::GetMemoryUsage(ICrySizer* pSizer) const
{
    {
        SIZER_COMPONENT_NAME(pSizer, "Effects");
        pSizer->AddObject(m_Effects);
        pSizer->AddObject(m_LoadedLibs);
    }
    {
        SIZER_COMPONENT_NAME(pSizer, "Emitters");
        pSizer->AddObject(m_Emitters);
    }
}

void CParticleManager::PrintParticleMemory()
{
    // Get stats and mem usage for all effects.
    ICrySizer* pSizer = GetSystem()->CreateSizer();
    SEffectCounts eCounts;

    pSizer->AddObject(m_LoadedLibs);
    pSizer->AddObject(m_Effects);

    for_container (TEffectsList, it, m_Effects)
    {
        it->second->GetEffectCounts(eCounts);
    }

    CryLogAlways("Particle effects: %d loaded, %d enabled, %d active, %" PRISIZE_T " KB, %" PRISIZE_T " allocs",
        eCounts.nLoaded, eCounts.nEnabled, eCounts.nActive, pSizer->GetTotalSize() >> 10, pSizer->GetObjectCount());

    // Get mem usage for all emitters.
    pSizer->Reset();
    pSizer->Add(*this);
    m_Emitters.GetMemoryUsage(pSizer);

    stl::SPoolMemoryUsage memParticle = ParticleObjectAllocator().GetTotalMemory();

    CryLogAlways("Particle heap: %d KB used, %d KB freed, %d KB unused",
        int(memParticle.nUsed >> 10), int(memParticle.nPoolFree() >> 10), int(memParticle.nNonPoolFree() >> 10));
    CryLogAlways("Non heap: %d KB", int(pSizer->GetTotalSize() - memParticle.nUsed) >> 10);

    pSizer->Release();
}

void CParticleManager::Reset(bool bIndependentOnly)
{
    m_bRegisteredListener = false;

    // make sure "no mat" and "simple" material are initialized before level starts
    CreateLightShader();

    for_all_ptrs (CParticleEmitter, e, m_Emitters)
    {
        if (!bIndependentOnly || e->IsIndependent())
        {
            EraseEmitter(e);
        }
    }

    if (!bIndependentOnly)
    {
        ResetData();
        m_nActiveEmitter = 0;
    }

    m_PhysEnv.Clear();

    // reset tracking data for dumping vertex/index pool usage
#if defined(PARTICLE_COLLECT_VERT_IND_POOL_USAGE)
    memset(m_arrVertexIndexPoolUsage, 0, sizeof(m_arrVertexIndexPoolUsage));
    m_bOutOfVertexIndexPoolMemory = false;
    m_nRequieredVertexPoolMemory = 0;
    m_nRequieredIndexPoolMemory = 0;
    m_nRendererParticleContainer = 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CParticleManager::ClearRenderResources(bool bForceClear)
{
    if (GetCVars()->e_ParticlesDebug & AlphaBit('m'))
    {
        PrintParticleMemory();
    }

    Reset(false);

    if (!GetCVars()->e_ParticlesPreload || bForceClear)
    {
        m_Effects.clear();
        m_LoadedLibs.clear();
        m_Emitters.clear();
        m_pDefaultEffect = 0;
        m_pLastDefaultParams = &GetDefaultParams();
    }

    CParticleEffect* pEffect = CParticleEffect::get_intrusive_list_root();
    while (pEffect)
    {
        pEffect->UnloadResources(false);
        pEffect = pEffect->m_next_intrusive;
    }

    stl::free_container(m_Effects);

    ParticleObjectAllocator().ResetUsage();

    m_pPartLightShader = nullptr;
    m_partMirrorLightShader = nullptr;
    m_PhysEnv.FreeMemory();
}

//////////////////////////////////////////////////////////////////////////
void CParticleManager::ClearDeferredReleaseResources()
{
    // make sure all particles etc are gone, only the emitters are keep alive
    for_all_ptrs (CParticleEmitter, e, m_Emitters)
    e->Reset();
}

//////////////////////////////////////////////////////////////////////////
void CParticleManager::AddEventListener(IParticleEffectListener* pListener)
{
    assert(pListener);

    if (pListener)
    {
        stl::push_back_unique(m_ListenersList, pListener);
    }
}

void CParticleManager::RemoveEventListener(IParticleEffectListener* pListener)
{
    assert(pListener);

    m_ListenersList.remove(pListener);
}

//////////////////////////////////////////////////////////////////////////
// Particle Effects.
//////////////////////////////////////////////////////////////////////////

void CParticleManager::SetDefaultEffect(const IParticleEffect* pEffect)
{
    m_pDefaultEffect = pEffect ? new CParticleEffect(static_cast<const CParticleEffect&>(*pEffect), true) : NULL;
    m_pLastDefaultParams = &GetDefaultParams();
    ReloadAllEffects();
}

const ParticleParams& CParticleManager::GetDefaultParams(ParticleParams::EInheritance eInheritance, int nVersion) const
{
    static ParticleParams s_paramsStandard;

    if (eInheritance == eInheritance.System)
    {
        if (m_pDefaultEffect)
        {
            if (const IParticleEffect* pEffect = m_pDefaultEffect->FindActiveEffect(nVersion))
            {
                return pEffect->GetParticleParams();
            }
        }
    }

    return s_paramsStandard;
}

IParticleEffect* CParticleManager::CreateEffect()
{
    if (!m_bEnabled)
    {
        return NULL;
    }
    CParticleEffect* effect = new CParticleEffect();

    //The particle created with this function is supposed to be renamed right after. Particle Editor is using it when add a new particle. 
    AZ_Assert(m_Effects.find(effect->GetName()) == m_Effects.end(), "The previous created effect wasn't renamed");
    m_Effects[effect->GetName()] = effect;
    return effect;
}

//////////////////////////////////////////////////////////////////////////
void CParticleManager::RenameEffect(CParticleEffect* pEffect, cstr sNewName)
{
    if (pEffect == nullptr)
    {
        AZ_Assert(false, "Trying to rename nullptr particle effect");
        return;
    }
    cstr effectName = pEffect->GetName();
    TEffectsList::iterator it = m_Effects.find(effectName);
    if (it != m_Effects.end())
    {
        pEffect->AddRef();
        m_Effects.erase(effectName);
        if (*sNewName)
        {
            m_Effects[ sNewName ] = pEffect;
        }
        pEffect->Release();
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleManager::DeleteEffect(IParticleEffect* pEffect)
{
    if (!pEffect)
    {
        return;
    }
    if (!pEffect->GetParent())
    {
        m_Effects.erase(pEffect->GetName());
    }
}

//////////////////////////////////////////////////////////////////////////
CParticleEffect* CParticleManager::FindLoadedEffect(cstr sEffectName)
{
    if (!m_bEnabled)
    {
        return NULL;
    }

    TEffectsList::iterator it = m_Effects.find(sEffectName);
    if (it != m_Effects.end())
    {
        return it->second;
    }

    // Find sub-effect
    if (const char* pDot = strchr(sEffectName, '.'))
    {
        if (pDot = strrchr(pDot + 1, '.'))
        {
            // 2nd separator found, after library name
            string sBase(sEffectName, pDot);
            if (CParticleEffect* pParent = FindLoadedEffect(sBase))
            {
                return pParent->FindChild(pDot + 1);
            }
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IParticleEffect* CParticleManager::FindEffect(cstr sEffectName, cstr sSource, bool bLoad)
{
    if (!m_bEnabled || !sEffectName || !*sEffectName)
    {
        return NULL;
    }

    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    CParticleEffect* pEffect = FindLoadedEffect(sEffectName);
    if (!pEffect)
    {
        if (CanAccessFiles(sEffectName))
        {
            if (cstr sDot = strchr(sEffectName, '.'))
            {
                stack_string sLibraryName(sEffectName, sDot);
                LoadLibrary(sLibraryName);

                // Try it again.
                pEffect = FindLoadedEffect(sEffectName);
            }

            if (!pEffect)
            {
                Warning("Particle effect not found: '%s'%s%s", sEffectName, *sSource ? " from " : "", sSource);
            }
        }

        if (!pEffect)
        {
            // Add an empty effect to avoid duplicate loads and warnings.
            pEffect = new CParticleEffect(sEffectName);
            m_Effects[ pEffect->GetName() ] = pEffect;
            return NULL;
        }
    }

    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Particles");
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleEffect, EMemStatContextFlags::MSF_Instance, "%s", sEffectName);

    assert(pEffect);
    if (pEffect->IsEnabled() || pEffect->GetChildCount())
    {
        if (bLoad)
        {
            pEffect->LoadResources(true, sSource);
        }
        return pEffect;
    }

    // Empty effect (either disabled params, or cached not found).
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
#ifdef bEVENT_TIMINGS
void CParticleManager::LogEvents()
{
    WriteLock lock(m_EventLock);
    if (GetCVars()->e_ParticlesDebug & AlphaBits('ed'))
    {
        if (!m_aEvents.size())
        {
            // Start timing this frame.
            AddEventTiming("*Frame", 0);
        }
        else
        {
            // Finish and log frame.
            m_aEvents[0].timeEnd = max(GetParticleTimer()->GetAsyncCurTime(), m_aEvents[0].timeStart + GetParticleTimer()->GetRealFrameTime());
            float timeFrameStart = m_aEvents[0].timeStart,
                  timeFrameTime = m_aEvents[0].timeEnd - m_aEvents[0].timeStart;

            if (timeFrameTime > 0.f)
            {
                static const int nWidth = 50;
                int nSumStart = m_aEvents.size();
                float timeTotal = 0.f;

                // Add to summary entries at end.
                for (int e = 0; e < nSumStart; e++)
                {
                    SEventTiming* pEvent = &m_aEvents[e];
                    if (pEvent->nContainerId)
                    {
                        timeTotal += pEvent->timeEnd - pEvent->timeStart;
                        int s;
                        for (s = nSumStart; s < m_aEvents.size(); s++)
                        {
                            SEventTiming* pSum = &m_aEvents[s];
                            if (pSum->sEvent == pEvent->sEvent && pSum->nThread == pEvent->nThread)
                            {
                                break;
                            }
                        }
                        if (s == m_aEvents.size())
                        {
                            // Add to summary.
                            SEventTiming* pSum = m_aEvents.push_back();
                            *pSum = m_aEvents[e];
                            pSum->nContainerId = 0;
                        }
                    }
                }

                // Check against time threshold.
                if ((GetCVars()->e_ParticlesDebug & AlphaBit('e')) || timeTotal > m_timeThreshold)
                {
                    // Increase threshold for next time.
                    m_timeThreshold = timeTotal * 1.1f;

                    for_array (c, m_aEvents)
                    {
                        SEventTiming* pEventC = &m_aEvents[c];
                        if (!pEventC->sEvent)
                        {
                            continue;
                        }

                        // Iterate unique threads.
                        for (int t = c; t < m_aEvents.size(); t++)
                        {
                            SEventTiming* pEventT = &m_aEvents[t];
                            if (!pEventT->sEvent)
                            {
                                continue;
                            }
                            if (pEventT->nContainerId != pEventC->nContainerId)
                            {
                                continue;
                            }

                            cstr sThread = CryThreadGetName(pEventT->nThread);
                            if (pEventT == pEventC)         // Main thread.
                            {
                                GetLog()->LogToFile("%*s %s(%X) @%s",
                                    nWidth, "", pEventC->pEffect ? pEventC->pEffect->GetName() : "", pEventC->nContainerId, sThread);
                            }
                            else
                            {
                                GetLog()->LogToFile("%*s   @%s", nWidth, "", sThread);
                            }

                            // Log event times.
                            for (int e = t; e < m_aEvents.size(); e++)
                            {
                                SEventTiming* pEvent = &m_aEvents[e];
                                if (!pEvent->sEvent)
                                {
                                    continue;
                                }
                                if (pEvent->nContainerId != pEventT->nContainerId || pEvent->nThread != pEventT->nThread)
                                {
                                    continue;
                                }

                                // Construct thread timeline.
                                char sGraph[nWidth + 1];
                                memset(sGraph, ' ', nWidth);
                                sGraph[nWidth] = 0;

                                int start_iter = pEventC->nContainerId ? e : 0;
                                int end_iter = pEventC->nContainerId ? e + 1 : nSumStart;
                                float timeStart = m_aEvents[e].timeStart,
                                      timeEnd = m_aEvents[e].timeEnd,
                                      timeTotal = 0.f;
                                for (int i = start_iter; i < end_iter; i++)
                                {
                                    SEventTiming* pEventI = &m_aEvents[i];
                                    if (pEventI->sEvent != pEvent->sEvent || pEventI->nThread != pEvent->nThread)
                                    {
                                        continue;
                                    }

                                    timeStart = min(timeStart, pEventI->timeStart);
                                    timeEnd = max(timeEnd, pEventI->timeEnd);
                                    timeTotal += pEventI->timeEnd - pEventI->timeStart;

                                    int nEventStart = int_round((pEventI->timeStart - timeFrameStart) * nWidth / timeFrameTime);
                                    int nEventEnd = int_round((pEventI->timeEnd - timeFrameStart) * nWidth / timeFrameTime);

                                    nEventStart = min(nEventStart, nWidth - 1);
                                    nEventEnd = min(max(nEventEnd, nEventStart + 1), nWidth);

                                    cstr sEvent = strrchr(pEventI->sEvent, ':');
                                    char cEvent = sEvent ? sEvent[1] : *pEventI->sEvent;
                                    memset(sGraph + nEventStart, cEvent, nEventEnd - nEventStart);
                                }

                                GetLog()->LogToFile("%s     %.3f-%.3f [%.3f] %s",
                                    sGraph,
                                    (timeStart - timeFrameStart) * 1000.f, (timeEnd - timeFrameStart) * 1000.f, timeTotal * 1000.f,
                                    pEvent->sEvent);

                                pEvent->sEvent = 0;
                            }
                        }
                    }
                }
            }

            // Clear log.
            GetCVars()->e_ParticlesDebug &= ~AlphaBit('e');
            m_aEvents.resize(0);
        }
    }
    else
    {
        m_aEvents.resize(0);
        m_timeThreshold = 0.f;
    }
    m_iEventSwitch *= -1;
}
#endif

void CParticleManager::OnFrameStart()
{
#ifdef bEVENT_TIMINGS
    LogEvents();
#endif

    if (IsRuntime())
    {
        ClearCachedLibraries();
    }

    // Update allowed particle features.
    UpdateEngineData();

    if (!m_bRegisteredListener && Get3DEngine()->GetIVisAreaManager())
    {
        Get3DEngine()->GetIVisAreaManager()->AddListener(this);
        m_bRegisteredListener = true;
    }
}

void CParticleManager::Update()
{
    CRYPROFILE_SCOPE_PROFILE_MARKER("CPartManager::Update");

    if (m_bEnabled && GetCVars()->e_Particles)
    {
        // move all stuff into its own scope to not measure physics time in the fiber
        FUNCTION_PROFILER_CONTAINER(0);
        PARTICLE_LIGHT_PROFILER();

        GetRenderer()->SyncComputeVerticesJobs();

        DumpAndResetVertexIndexPoolUsage();

        bool bStatoscopeEffectStats = false;
        bool bCollectCounts = (GetCVars()->e_ParticlesDebug & 1)
            || (GetCVars()->e_ParticlesProfile & 1)
            || m_pWidget && m_pWidget->ShouldUpdate()
            || gEnv->pStatoscope->RequiresParticleStats(bStatoscopeEffectStats);

        ZeroStruct(m_GlobalCounts);

        if (GetSystem()->IsPaused() || (GetCVars()->e_ParticlesDebug & AlphaBit('z')))
        {
            return;
        }

        uint32 nActiveEmitter = 0;

        // Check emitter states.
        for_all_ptrs (CParticleEmitter, e, m_Emitters)
        {
            if (bCollectCounts)
            {
                e->GetAndClearCounts(m_GlobalCounts);
            }

            e->SetUpdateParticlesJobState(NULL);

            bool bRenderedLastFrame = e->TimeNotRendered() == 0.f && e->GetAge() > 0.f;

            e->Update();

            if (e->IsActive())
            {
                // Has particles
                if (e->GetEnvFlags() & EFF_ANY)
                {
                    e->UpdateEffects();
                }
                if (e->GetEnvFlags() & REN_ANY)
                {
                    e->Register(true);
                }
                nActiveEmitter++;
                if (bRenderedLastFrame)
                {
                    e->AddUpdateParticlesJob();
                }
            }
            else if (e->IsAlive())
            {
                // Dormant
                if (e->GetEnvFlags() & REN_ANY)
                {
                    e->Reset();
                }
            }
            else
            {
                // Dead
                EraseEmitter(e);
            }
        }

        m_nActiveEmitter = nActiveEmitter;
    }
}

//////////////////////////////////////////////////////////////////////////
CParticleEmitter* CParticleManager::CreateEmitter(const QuatTS& loc, const IParticleEffect* pEffect, uint32 uEmitterFlags, const SpawnParams* pSpawnParams)
{
    PARTICLE_LIGHT_PROFILER();
    FUNCTION_PROFILER_SYS(PARTICLE);

    if (!pEffect)
    {
        return NULL;
    }

    void* pAlloc;
    if (static_cast<const CParticleEffect*>(pEffect)->GetEnvironFlags(true) & EFF_FORCE)
    {
        // Place emitters that create forces first in the list, so they are updated first.
        pAlloc = m_Emitters.push_front_new();
    }
    else
    {
        pAlloc = m_Emitters.push_back_new();
    }
    if (!pAlloc)
    {
        return NULL;
    }

    CParticleEmitter* pEmitter = new(pAlloc) CParticleEmitter(pEffect, loc, uEmitterFlags, pSpawnParams);
    pEmitter->IParticleEmitter::AddRef();
    pEmitter->Activate(true);

    for_container (TListenersList, itListener, m_ListenersList)
    {
        (*itListener)->OnCreateEmitter(pEmitter, loc, pEffect, uEmitterFlags);
    }
    return pEmitter;
}

IParticleEmitter* CParticleManager::CreateEmitter(const QuatTS& loc, const ParticleParams& Params, uint32 uEmitterFlags, const SpawnParams* pSpawnParams)
{
    PARTICLE_LIGHT_PROFILER();
    FUNCTION_PROFILER_SYS(PARTICLE);

    if (!m_bEnabled)
    {
        return NULL;
    }

    // Create temporary effect for custom params.
    CParticleEffect* pEffect = new CParticleEffect("(Temp)", Params);

    IParticleEmitter* pEmitter = CreateEmitter(loc, pEffect, uEmitterFlags | ePEF_TemporaryEffect, pSpawnParams);

    if (!pEmitter)
    {
        // If the emitter failed to be allocated, the effect will leak (as it won't be bound to the emitter). Free it.
        pEffect->Release();
    }

    return pEmitter;
}

//////////////////////////////////////////////////////////////////////////
void CParticleManager::EraseEmitter(CParticleEmitter* pEmitter)
{
    if (pEmitter)
    {
        // Free resources.
        pEmitter->Reset();

        if (pEmitter->GetRefCount() == 1 && !pEmitter->m_pOcNode)
        {
            // Free object itself if no other refs.
            for_container (TListenersList, itListener, m_ListenersList)
            {
                (*itListener)->OnDeleteEmitter(pEmitter);
            }
            m_Emitters.erase(pEmitter);
        }
    }
}

void CParticleManager::DeleteEmitter(IParticleEmitter* pEmitter)
{
    if (pEmitter)
    {
        pEmitter->Kill();
    }
}

void CParticleManager::DeleteEmitters(uint32 mask)
{
    for_all_ptrs (CParticleEmitter, e, m_Emitters)
    {
        if (e->GetEmitterFlags() & mask)
        {
            DeleteEmitter(e);
        }
    }
}

void CParticleManager::UpdateEmitters(IParticleEffect* pEffect, bool recreateContainer)
{
    // Update all emitters with this effect tree.
    for_all_ptrs (CParticleEmitter, e, m_Emitters)
    {
        for (IParticleEffect* pTest = pEffect; pTest; pTest = pTest->GetParent())
        {
            if (e->GetEffect() == pTest)
            {
                e->RefreshEffect(recreateContainer);
                break;
            }
        }
    }
}

void CParticleManager::ReloadAllEffects()
{
    // Reload all current effects
    for_container (TEffectsList, it, m_Effects)
    {
        it->second->Reload(true);
    }

    // Update all emitters.
    for_all_ptrs (CParticleEmitter, e, m_Emitters)
    {
        e->RefreshEffect();
    }
}

bool CParticleManager::CanAccessFiles(cstr sObject, cstr sSource) const
{
    if (!gEnv->pCryPak->CheckFileAccessDisabled(0, 0))
    {
        return true;
    }

    // Check file access override.
    if (GetCVars()->e_ParticlesAllowRuntimeLoad)
    {
        return true;
    }

    if (!sSource)
    {
        sSource = "";
    }
    Warning("Particle asset read attempted at runtime: %s%s%s",
        sObject, (*sSource ? " from " : ""), sSource);

    return false;
}

void CParticleManager::UpdateEngineData()
{
    m_nAllowedEnvironmentFlags = ~0;

    if (GetCVars())
    {
        // Update allowed particle features.
        if (GetCVars()->e_ParticlesObjectCollisions < 2)
        {
            m_nAllowedEnvironmentFlags &= ~ENV_DYNAMIC_ENT;
            if (GetCVars()->e_ParticlesObjectCollisions < 1)
            {
                m_nAllowedEnvironmentFlags &= ~ENV_STATIC_ENT;
            }
        }
        if (!GetCVars()->e_ParticlesLights || !GetCVars()->e_DynamicLights)
        {
            m_nAllowedEnvironmentFlags &= ~REN_LIGHTS;
        }
    }

    // Render object flags, using cvar convention of 1 = allowed, 2 = forced.
    m_RenderFlags.Clear();
    m_RenderFlags.SetState(GetCVars()->e_ParticlesAnimBlend - 1, OS_ANIM_BLEND);
    m_RenderFlags.SetState(GetCVars()->e_ParticlesMotionBlur - 1, FOB_MOTION_BLUR);
    m_RenderFlags.SetState(GetCVars()->e_ParticlesShadows - 1, FOB_PARTICLE_SHADOWS);
    m_RenderFlags.SetState(GetCVars()->e_ParticlesGI - 1, FOB_GLOBAL_ILLUMINATION);
    m_RenderFlags.SetState(GetCVars()->e_ParticlesSoftIntersect - 1, FOB_SOFT_PARTICLE);

    bool bParticleTesselation = false;
    GetRenderer()->EF_Query(EFQ_ParticlesTessellation, bParticleTesselation);
    m_RenderFlags.SetState(int(bParticleTesselation) - 1, FOB_ALLOW_TESSELLATION);

    if (m_pLastDefaultParams != &GetDefaultParams())
    {
        // Default effect or config spec changed.
        m_pLastDefaultParams = &GetDefaultParams();
        ReloadAllEffects();
    }
}

void CParticleManager::OnVisAreaDeleted(IVisArea* pVisArea)
{
    for_all_ptrs (CParticleEmitter, e, m_Emitters)
    e->OnVisAreaDeleted(pVisArea);
}

IParticleEmitter* CParticleManager::SerializeEmitter(TSerialize ser, IParticleEmitter* pIEmitter)
{
    ser.BeginGroup("Emitter");

    string sEffect;
    QuatTS qLoc;

    CParticleEmitter* pEmitter = static_cast<CParticleEmitter*>(pIEmitter);
    if (pEmitter)
    {
        if (const IParticleEffect* pEffect = pEmitter->GetEffect())
        {
            sEffect = pEffect->GetName();
        }
        qLoc = pEmitter->GetLocation();
    }

    ser.Value("Effect", sEffect);

    ser.Value("Pos", qLoc.t);
    ser.Value("Rot", qLoc.q);
    ser.Value("Scale", qLoc.s);

    if (ser.IsReading())
    {
        assert(!pEmitter);
        if (IParticleEffect* pEffect = FindEffect(sEffect))
        {
            pEmitter = CreateEmitter(qLoc, pEffect);
        }
    }

    if (pEmitter)
    {
        pEmitter->SerializeState(ser);
    }

    ser.EndGroup();

    return pEmitter;
}


//////////////////////////////////////////////////////////////////////////
class CLibPathIterator
{
public:

    CLibPathIterator(cstr sLevelPath = "")
        : sPath(*sLevelPath ? sLevelPath : LEVEL_PATH)
        , bDone(false)
    {}
    operator bool() const
    {
        return !bDone;
    }
    const string& operator *() const
    { return sPath; }

    void operator++()
    {
        if (sPath.empty())
        {
            bDone = true;
        }
        else
        {
            sPath = PathUtil::GetParentDirectory(sPath);
        }
    }

protected:
    string sPath;
    bool bDone;
};

//////////////////////////////////////////////////////////////////////////
bool CParticleManager::LoadPreloadLibList(const cstr filename, const bool bLoadResources)
{
    if (!m_bEnabled)
    {
        return false;
    }

    int nCount = 0;
    CCryFile file;
    if (file.Open(filename, "r"))
    {
        const size_t nLen = file.GetLength();
        string sAllText;
        if (nLen > 0)
        {
            std::vector<char> buffer;
            buffer.resize(nLen, '\n');
            file.ReadRaw(&buffer[0], nLen);
            sAllText.assign(&buffer[0], nLen);
        }

        int pos = 0;
        string sLine;
        while (!(sLine = sAllText.Tokenize("\r\n", pos)).empty())
        {
            if (LoadLibrary(sLine, NULL, bLoadResources))
            {
                nCount++;
            }
        }
    }
    return nCount > 0;
}

//////////////////////////////////////////////////////////////////////////
bool CParticleManager::LoadLibrary(cstr sParticlesLibrary, cstr sParticlesLibraryFile, bool bLoadResources)
{
    if (!m_bEnabled)
    {
        return false;
    }

    if (m_LoadedLibs[sParticlesLibrary])
    {
        // Already loaded.
        if (bLoadResources && CanAccessFiles(sParticlesLibrary))
        {
            // Iterate all fx in lib and load their resources.
            stack_string sPrefix = sParticlesLibrary;
            sPrefix += ".";
            for_container (TEffectsList, it, m_Effects)
            {
                CParticleEffect* pEffect = it->second;
                if (pEffect && azstrnicmp(pEffect->GetName(), sPrefix, sPrefix.size()) == 0)
                {
                    pEffect->LoadResources(true);
                }
            }
        }
        return true;
    }

    if (!CanAccessFiles(sParticlesLibrary))
    {
        return false;
    }

    if (strchr(sParticlesLibrary, '*'))
    {
        // Wildcard load.
        if (!sParticlesLibraryFile)
        {
            // Load libs from level-local and global paths.
            int nCount = 0;
            for (CLibPathIterator path(Get3DEngine()->GetLevelFilePath("")); path; ++path)
            {
                nCount += LoadLibrary(sParticlesLibrary, PathUtil::Make(*path, EFFECTS_SUBPATH), bLoadResources);
            }
            return nCount > 0;
        }
        else
        {
            // Load from specified path.
            string sLibPath = PathUtil::Make(sParticlesLibraryFile, sParticlesLibrary, "xml");
            ICryPak* pack = gEnv->pCryPak;
            _finddata_t fd;
            intptr_t handle = pack->FindFirst(sLibPath, &fd);
            int nCount = 0;
            if (handle >= 0)
            {
                do
                {
                    if (LoadLibrary(PathUtil::GetFileName(fd.name), PathUtil::Make(sParticlesLibraryFile, fd.name), bLoadResources))
                    {
                        nCount++;
                    }
                } while (pack->FindNext(handle, &fd) >= 0);
                pack->FindClose(handle);
            }
            return nCount > 0;
        }
    }
    else if (sParticlesLibrary[0] == '@')
    {
        // first we try to load the level specific levelSpecificFile - if it exists, the libraries it lists are added
        // to those from the default levelSpecificFile
        string sFilename = PathUtil::Make(sParticlesLibraryFile, sParticlesLibrary + 1, "txt");
        bool levelSpecificLoaded = LoadPreloadLibList(sFilename, bLoadResources);

        // also load the default package
        sFilename = PathUtil::Make(EFFECTS_SUBPATH, sParticlesLibrary + 1, "txt");
        bool globalFileLoaded = LoadPreloadLibList(sFilename, bLoadResources);
        return globalFileLoaded || levelSpecificLoaded;
    }
    else
    {
        XmlNodeRef libNode;
        if (sParticlesLibraryFile)
        {
            if (gEnv->pCryPak->IsFileExist(sParticlesLibraryFile))
            {
                // Load from specified location.
                libNode = GetISystem()->LoadXmlFromFile(sParticlesLibraryFile);
            }
        }
        else
        {
            libNode = ReadLibrary(sParticlesLibrary);
        }

        if (!libNode)
        {
            return false;
        }

        LoadLibrary(sParticlesLibrary, libNode, bLoadResources);
    }
    return true;
}

XmlNodeRef CParticleManager::ReadLibrary(cstr sParticlesLibrary)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "ParticleLibraries");
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleLibrary, 0, "Particle lib (%s)", sParticlesLibrary);

    string sLibSubPath = PathUtil::Make(EFFECTS_SUBPATH, sParticlesLibrary, "xml");
    if (GetCVars()->e_ParticlesUseLevelSpecificLibs)
    {
        // Look for library in level-specific, then general locations.
        for (CLibPathIterator path(Get3DEngine()->GetLevelFilePath("")); path; ++path)
        {
            if (XmlNodeRef nodeLib = GetISystem()->LoadXmlFromFile(PathUtil::Make(*path, sLibSubPath)))
            {
                return nodeLib;
            }
        }
        return NULL;
    }
    else
    {
        return GetISystem()->LoadXmlFromFile(sLibSubPath);
    }
}

static IXmlNode* FindEffectNode(IXmlNode* nodeParent, const stack_string& sEffectName)
{
    for (int i = 0; i < nodeParent->getChildCount(); i++)
    {
        IXmlNode* node = nodeParent->getChild(i);
        if (node->isTag("Particles"))
        {
            cstr sNodeName = node->getAttr("Name");
            if (sEffectName.compareNoCase(sNodeName) == 0)
            {
                return node;
            }
            size_t nNodeNameLen = strlen(sNodeName);
            if (sEffectName.length() > nNodeNameLen && sEffectName[nNodeNameLen] == '.' && strncmp(sEffectName, sNodeName, nNodeNameLen) == 0)
            {
                // Search sub-effects
                if (XmlNodeRef childsNode = node->findChild("Childs"))
                {
                    stack_string sChildName = sEffectName.substr(nNodeNameLen + 1);
                    if (IXmlNode* found = FindEffectNode(childsNode, sChildName))
                    {
                        return found;
                    }
                }
            }
        }
    }
    return NULL;
}

XmlNodeRef CParticleManager::ReadEffectNode(cstr sEffectName)
{
    // Read XML node for this effect from libraries
    if (cstr sDot = strchr(sEffectName, '.'))
    {
        stack_string sLibraryName(sEffectName, sDot);
        if (XmlNodeRef nodeLib = ReadLibrary(sLibraryName))
        {
            cstr sBaseName = sDot + 1;
            if (IXmlNode* found = FindEffectNode(nodeLib, sBaseName))
            {
                int nVersion = 0;
                cstr sSandboxVersion = "3.5";
                if (nodeLib->haveAttr("ParticleVersion"))
                {
                    nodeLib->getAttr("ParticleVersion", nVersion);
                    found->setAttr("ParticleVersion", nVersion);
                }
                if (nodeLib->haveAttr("SandboxVersion"))
                {
                    nodeLib->getAttr("SandboxVersion", nVersion);
                    found->setAttr("SandboxVersion", nVersion);
                }
                return found;
            }
        }
    }

    return NULL;
}

bool CParticleManager::LoadLibrary(cstr sParticlesLibrary, XmlNodeRef& libNode, bool bLoadResources)
{
    if (!m_bEnabled)
    {
        return false;
    }

    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "ParticleLibraries");
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleLibrary, 0, "Particle lib (%s)", sParticlesLibrary);

    CRY_DEFINE_ASSET_SCOPE("ParticleLibrary", sParticlesLibrary);

    m_LoadedLibs[sParticlesLibrary] = libNode;

    // Load special defaults effects, if created (no warning otherwise).
    if (!m_pDefaultEffect && azstricmp(sParticlesLibrary, "System") != 0 && gEnv->pCryPak->IsFileExist(EFFECTS_SUBPATH "System.xml"))
    {
        SetDefaultEffect("System.Default");
    }

    // Load all nodes from the particle libaray.
    for (int i = 0, count = libNode->getChildCount(); i < count; i++)
    {
        XmlNodeRef effectNode = libNode->getChild(i);
        if (effectNode->isTag("Particles"))
        {
            string sEffectName = sParticlesLibrary;
            sEffectName += ".";
            sEffectName += effectNode->getAttr("Name");

            // Load the full effect.
            LoadEffect(sEffectName, effectNode, bLoadResources);
        }
    }
    return true;
}

IParticleEffect* CParticleManager::LoadEffect(cstr sEffectName, XmlNodeRef& effectNode, bool bLoadResources, const cstr sSource)
{
    if (!m_bEnabled)
    {
        return NULL;
    }

    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_ParticleEffect, 0, "%s", sEffectName);

    CParticleEffect* pEffect = FindLoadedEffect(sEffectName);
    if (!pEffect)
    {
        pEffect = new CParticleEffect(sEffectName);
        m_Effects[pEffect->GetName()] = pEffect;
    }

    // Load effect params and all assets.
    pEffect->Serialize(effectNode, true, true);

    if (bLoadResources)
    {
        pEffect->LoadResources(true, sSource);
    }

    return pEffect;
}

void CParticleManager::ClearCachedLibraries()
{
    if (GetCVars()->e_ParticlesAllowRuntimeLoad)
    {
        return;
    }

    if (m_LoadedLibs.empty())
    {
        return;
    }

    if (GetCVars()->e_ParticlesDebug & AlphaBit('m'))
    {
        PrintParticleMemory();
    }

    for (TEffectsList::iterator it = m_Effects.begin(); it != m_Effects.end(); )
    {
        // Purge all unused effects.
        CParticleEffect* pEffect = it->second;
        bool fxInUse = (pEffect->NumRefs() > 1) || (pEffect->ResourcesLoaded(true));
        if (!fxInUse)
        {
            it = m_Effects.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_LoadedLibs.clear();

    if ((GetCVars()->e_ParticlesDebug & AlphaBit('m')) || GetCVars()->e_ParticlesDumpMemoryAfterMapLoad)
    {
        PrintParticleMemory();
    }
}

IParticleEffectIteratorPtr CParticleManager::GetEffectIterator()
{
    return new CParticleEffectIterator(this);
}

void CParticleManager::RenderDebugInfo()
{
    bool bRefractivePartialResolveDebugView = false;
#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
    static ICVar* pRefractionPartialResolvesDebugCVar = gEnv->pConsole->GetCVar("r_RefractionPartialResolvesDebug");
    if (pRefractionPartialResolvesDebugCVar && pRefractionPartialResolvesDebugCVar->GetIVal() == eRPR_DEBUG_VIEW_3D_BOUNDS)
    {
        bRefractivePartialResolveDebugView = true;
    }
#endif
    if ((GetCVars()->e_ParticlesDebug & AlphaBit('b')) || gEnv->IsEditing() || bRefractivePartialResolveDebugView)
    {
        // Debug particle BBs.
        FUNCTION_PROFILER_SYS(PARTICLE);
        for_all_ptrs (CParticleEmitter, e, m_Emitters)
        if (!(e->GetEmitterFlags() & ePEF_Nowhere))
        {
            e->RenderDebugInfo();
        }
    }
}

#ifdef bEVENT_TIMINGS

int CParticleManager::AddEventTiming(cstr sEvent, const CParticleContainer* pCont)
{
    if (!m_aEvents.size() && *sEvent != '*')
    {
        return -1;
    }

    SEventTiming* pEvent = m_aEvents.push_back();
    pEvent->nContainerId = (uint32)pCont;
    pEvent->pEffect = pCont ? pCont->GetEffect() : 0;
    pEvent->nThread = CryGetCurrentThreadId();
    pEvent->sEvent = sEvent;
    pEvent->timeStart = GetParticleTimer()->GetAsyncCurTime();

    return m_aEvents.size() - 1;
}

#endif

//////////////////////////////////////////////////////////////////////////
void CParticleManager::Serialize(TSerialize ser)
{
    ser.BeginGroup("ParticleEmitters");

    if (ser.IsWriting())
    {
        ListEmitters("before save");

        int nCount = 0;

        for_all_ptrs (CParticleEmitter, e, m_Emitters)
        {
            if (e->NeedSerialize())
            {
                SerializeEmitter(ser, e);
                ++nCount;
            }
        }
        ser.Value("Emitters", nCount);
    }
    else
    {
        ListEmitters("before load");

        // Clean up existing emitters.
        for_all_ptrs (CParticleEmitter, e, m_Emitters)
        {
            if (e->IsIndependent())
            {
                // Would be serialized.
                EraseEmitter(e);
            }
            else if (!e->IsAlive())
            {
                // No longer exists at new time.
                EraseEmitter(e);
            }
            else
            {
                // Ensure it starts again, in case expired.
                e = e;
            }
        }

        int nCount = 0;
        ser.Value("Emitters", nCount);
        while (nCount-- > 0)
        {
            SerializeEmitter(ser);
        }
    }
    ser.EndGroup();
}

void CParticleManager::PostSerialize(bool bReading)
{
    if (bReading)
    {
        ListEmitters("after load");
    }
}

void CParticleManager::ListEmitters(cstr sDesc, bool bForce)
{
    if (bForce || GetCVars()->e_ParticlesDebug & AlphaBit('l'))
    {
        // Count emitters.
        int anEmitters[3] = {0};

        // Log summary, and state of each emitter.
        CryLog("Emitters %s: time %.3f", sDesc, GetParticleTimer()->GetFrameStartTime().GetSeconds());
        for_all_ptrs (const CParticleEmitter, e, m_Emitters)
        {
            if (e->IsActive())
            {
                anEmitters[0]++;
            }
            else if (e->IsAlive())
            {
                anEmitters[1]++;
            }
            else
            {
                anEmitters[2]++;
            }
            CryLog(" %s", e->GetDebugString('s').c_str());
        }
        CryLog("Total: %d active, %d dormant, %d dead",
            anEmitters[0], anEmitters[1], anEmitters[2]);
    }
}

void CParticleManager::ListEffects()
{
    // Collect all container stats, sum into effects map.
    TEffectStats mapEffectStats;
    CollectEffectStats(mapEffectStats, &SParticleCounts::ParticlesActive);

    // Header for CSV-formatted effect list.
    CryLogAlways(
        "Effect, "
        "Ems ren, Ems act, Ems all, "
        "Parts ren, Parts act, Parts all, "
        "Fill ren, Fill act, "
        "Stat vol, Dyn vol, Err vol,"
        "Coll test, Coll hit, Clip, "
        "Reiter, Reject"
        );

    for_container (CParticleManager::TEffectStats, it, mapEffectStats)
    {
        SParticleCounts const& counts = it->second;
        float fPixToScreen = 1.f / ((float)GetRenderer()->GetWidth() * (float)GetRenderer()->GetHeight());
        CryLogAlways(
            "%s, "
            "%.0f, %.0f, %.0f, "
            "%.0f, %.0f, %.0f, "
            "%.3f, %.3f, "
            "%.0f, %.0f, %.2f, "
            "%.2f, %.2f, %.2f, "
            "%.0f, %.2f",
            it->first ? (cstr)it->first->GetFullName() : "TOTAL",
            counts.EmittersRendered, counts.EmittersActive, counts.EmittersAlloc,
            counts.ParticlesRendered, counts.ParticlesActive, counts.ParticlesAlloc,
            counts.PixelsRendered * fPixToScreen, counts.PixelsProcessed * fPixToScreen,
            counts.StaticBoundsVolume, counts.DynamicBoundsVolume, counts.ErrorBoundsVolume,
            counts.ParticlesCollideTest, counts.ParticlesCollideHit, counts.ParticlesClip,
            counts.ParticlesReiterate, counts.ParticlesReject
            );
    }
}

void CParticleManager::CreatePerfHUDWidget()
{
    if (!m_pWidget)
    {
        if (ICryPerfHUD* pPerfHUD = gEnv->pSystem->GetPerfHUD())
        {
            if (IMiniCtrl* pRenderMenu = pPerfHUD->GetMenu("Rendering"))
            {
                m_pWidget = new CParticleWidget(pRenderMenu, pPerfHUD, this);
            }
        }
    }
}

void CParticleManager::DumpAndResetVertexIndexPoolUsage()
{
#if defined(PARTICLE_COLLECT_VERT_IND_POOL_USAGE)
    // don't show the out of vertex data at all if the user wants it
    if (GetCVars()->e_ParticlesProfile == 2)
    {
        return;
    }

    const stl::SPoolMemoryUsage memParticles = ParticleObjectAllocator().GetTotalMemory();

    bool bOutOfMemory = m_bOutOfVertexIndexPoolMemory || ((GetCVars()->e_ParticlesPoolSize * 1024) - memParticles.nUsed) == 0;
    // dump information if we are out of memory or if dumping is enabled
    if (bOutOfMemory || GetCVars()->e_ParticlesProfile == 1)
    {
        float fTextSideOffset = 10.0f;
        float fTopOffset = 3.0f;
        float fTextSize = 1.4f;
        float fTextColorOutOfMemory[] = {1.0f, 0.0f, 0.0f, 1.0f};
        float fTextColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        float* pColor = bOutOfMemory ? fTextColorOutOfMemory : fTextColor;
        float fScreenPix = (float)(GetRenderer()->GetWidth() * GetRenderer()->GetHeight());


        SParticleCounts CurCounts;
        m_pPartManager->GetCounts(CurCounts);

        uint32 nParticleVertexBufferSize = 0;
        uint32 nParticleIndexBufferSize = 0;
        uint32 nMaxParticleContainer = 0;
        gEnv->pRenderer->EF_Query(EFQ_GetParticleVertexBufferSize, nParticleVertexBufferSize);
        gEnv->pRenderer->EF_Query(EFQ_GetParticleIndexBufferSize, nParticleIndexBufferSize);
        gEnv->pRenderer->EF_Query(EFQ_GetMaxParticleContainer, nMaxParticleContainer);

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "== Particle Profiler ==");
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "- Particle Object Pool (Size %4d KB)",
            GetCVars()->e_ParticlesPoolSize);
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tUsed %4d Max Used %4d Free %4d (KB)",
            uint(memParticles.nUsed >> 10), uint((memParticles.nUsed + memParticles.nPool)) >> 10, uint(memParticles.nFree() >> 10));
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "- Particle VertexBuffer (Size %3d KB * 2)",
            nParticleVertexBufferSize / 1024);
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tAvail. %3d Used %3d Free %3d",
            nParticleVertexBufferSize / (uint)sizeof(SVF_Particle), m_nRequieredVertexPoolMemory / (uint)sizeof(SVF_Particle),
            m_nRequieredVertexPoolMemory >= nParticleVertexBufferSize ? 0 : (nParticleVertexBufferSize - m_nRequieredVertexPoolMemory) / (uint)sizeof(SVF_Particle));
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "- Particle IndexBuffer (Size %3d KB * 2)",
            nParticleIndexBufferSize / 1024);
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tAvail. %3d Used %3d Free %3d",
            nParticleIndexBufferSize / (uint)sizeof(uint16), m_nRequieredIndexPoolMemory / (uint)sizeof(uint16),
            m_nRequieredIndexPoolMemory >= nParticleIndexBufferSize ? 0 : (nParticleIndexBufferSize - m_nRequieredIndexPoolMemory) / (uint)sizeof(uint16));
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "- Render Stats");
        fTopOffset += 18.0f;

        /*
                gEnv->pRenderer->Draw2dLabel( fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tContainer Rendered %3d (Limit %3d)",
                    m_nNumContainerToRender, nMaxParticleContainer);
                fTopOffset += 18.0f;

                gEnv->pRenderer->Draw2dLabel( fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tParticles Culled   %3d",
                    m_nParticlesCulled);
                fTopOffset += 18.0f;
        */

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "- Fill Rate (Particle Pixels per Screen Pixel)");
        fTopOffset += 18.0f;


        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tLimit    %3d", (int)GetCVars()->e_ParticlesMaxScreenFill);
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tProcessed %3d", (int)(CurCounts.PixelsProcessed / fScreenPix));
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "\tRendered  %3d", (int)(CurCounts.PixelsRendered / fScreenPix));
        fTopOffset += 18.0f;
        fTopOffset += 18.0f;

        gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "- Top %d ParticleContainer in Vertex/Index Pool", nVertexIndexPoolUsageEntries);
        fTopOffset += 18.0f;

        for (int i = 0; i < nVertexIndexPoolUsageEntries; ++i)
        {
            if (m_arrVertexIndexPoolUsage[i].nVertexMemory == 0)
            {
                break;
            }

            gEnv->pRenderer->Draw2dLabel(fTextSideOffset, fTopOffset, fTextSize, pColor, false, "Vert: %3d KB Ind: %3d KB Part: %3d %s", m_arrVertexIndexPoolUsage[i].nVertexMemory / 1024, m_arrVertexIndexPoolUsage[i].nIndexMemory / 1024, 0, m_arrVertexIndexPoolUsage[i].pContainerName);
            fTopOffset += 18.0f;
        }
    }
    memset(m_arrVertexIndexPoolUsage, 0, sizeof(m_arrVertexIndexPoolUsage));
    m_bOutOfVertexIndexPoolMemory = false;
    m_nRequieredVertexPoolMemory = 0;
    m_nRequieredIndexPoolMemory = 0;
    m_nRendererParticleContainer = 0;
#endif
}


//PerfHUD
CParticleWidget::CParticleWidget(IMiniCtrl* pParentMenu, ICryPerfHUD* pPerfHud, CParticleManager* pPartManager)
    : ICryPerfHUDWidget(eWidget_Particles)
{
    m_pPartMgr = pPartManager;

    m_pTable = pPerfHud->CreateTableMenuItem(pParentMenu, "Particles");

    pPerfHud->AddWidget(this);

    m_displayMode = PARTICLE_DISP_MODE_NONE;
}

CParticleWidget::~CParticleWidget()
{
}

void CParticleWidget::Enable(int mode)
{
    mode = min(mode, PARTICLE_DISP_MODE_NUM - 1);
    EPerfHUD_ParticleDisplayMode newMode = (EPerfHUD_ParticleDisplayMode)mode;

    if (m_displayMode != newMode)
    {
        m_pTable->RemoveColumns();
        m_pTable->AddColumn("Effect Name");

        switch (newMode)
        {
        case PARTICLE_DISP_MODE_PARTICLE:
        case PARTICLE_DISP_MODE_FILL:
            m_pTable->AddColumn("P. Rendered");
            m_pTable->AddColumn("P. Colliding");
            m_pTable->AddColumn("P. Active");
            m_pTable->AddColumn("P. Alloc");
            m_pTable->AddColumn("P. Fill");
            m_displayMode = newMode;
            break;

        case PARTICLE_DISP_MODE_EMITTER:
            m_pTable->AddColumn("E. Rendered");
            m_pTable->AddColumn("E. Colliding");
            m_pTable->AddColumn("E. Active");
            m_pTable->AddColumn("E. Alloc");
            m_displayMode = newMode;
            break;

        default:
            CryLogAlways("[Particle Stats] Attempting to set incorrect display mode set: %d", mode);
            break;
        }
    }

    m_pTable->Hide(false);
}


bool CParticleWidget::ShouldUpdate()
{
    return !m_pTable->IsHidden();
}

void CParticleWidget::Update()
{
    FUNCTION_PROFILER_SYS(PARTICLE);

    m_pTable->ClearTable();

    ColorB col(255, 255, 255, 255);

    CParticleManager::TEffectStats mapEffectStats;

    switch (m_displayMode)
    {
    case PARTICLE_DISP_MODE_PARTICLE:
        m_pPartMgr->CollectEffectStats(mapEffectStats, &SParticleCounts::ParticlesRendered);
        break;

    case PARTICLE_DISP_MODE_FILL:
        m_pPartMgr->CollectEffectStats(mapEffectStats, &SParticleCounts::PixelsRendered);
        break;

    case PARTICLE_DISP_MODE_EMITTER:
        m_pPartMgr->CollectEffectStats(mapEffectStats, &SParticleCounts::EmittersRendered);
        break;
    }

    if (mapEffectStats[NULL].ParticlesAlloc)
    {
        float fPixToScreen = 1.f / (gEnv->pRenderer->GetWidth() * gEnv->pRenderer->GetHeight());
        for_container (CParticleManager::TEffectStats, it, mapEffectStats)
        {
            m_pTable->AddData(0, col, "%s", it->first ? (cstr)it->first->GetFullName() : "TOTAL");

            SParticleCounts const& counts = it->second;
            if (m_displayMode == PARTICLE_DISP_MODE_EMITTER)
            {
                m_pTable->AddData(1, col, "%d", (int)counts.EmittersRendered);
                m_pTable->AddData(2, col, "%d", (int)counts.EmittersActive);
                m_pTable->AddData(3, col, "%d", (int)counts.EmittersAlloc);
            }
            else
            {
                m_pTable->AddData(1, col, "%d", (int)counts.ParticlesRendered);
                m_pTable->AddData(2, col, "%d", (int)counts.ParticlesActive);
                m_pTable->AddData(3, col, "%d", (int)counts.ParticlesAlloc);
                m_pTable->AddData(4, col, "%.2f", counts.PixelsRendered * fPixToScreen);
            }
        }
    }
}

CParticleManager::SEffectsKey::SEffectsKey(const cstr& sName)
{
    stack_string lowerName(sName);
    CryStringUtils::ToLowerInplace(const_cast<char*>(lowerName.c_str()));

    IZLibCompressor* pZLib = GetISystem()->GetIZLibCompressor();
    assert(pZLib);

    SMD5Context context;
    pZLib->MD5Init(&context);
    pZLib->MD5Update(&context, lowerName.c_str(), lowerName.size());
    pZLib->MD5Final(&context, c16);
}
