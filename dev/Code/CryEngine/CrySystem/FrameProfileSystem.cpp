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
#include "FrameProfileSystem.h"

#include <ILog.h>
#include <IRenderer.h>
#include <StlUtils.h>
#include <IConsole.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <IThreadTask.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <IStatoscope.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

#include "Sampler.h"
#include "CryThread.h"
#include "Timer.h"

#if defined(LINUX)
#include "platform.h"
#endif


#ifdef WIN32
#include <psapi.h>
static HMODULE hPsapiModule;
typedef BOOL (WINAPI * FUNC_GetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
static FUNC_GetProcessMemoryInfo pfGetProcessMemoryInfo;
static bool m_bNoPsapiDll;
#endif

extern int CryMemoryGetAllocatedSize();

#ifdef USE_FRAME_PROFILER

#define MAX_PEAK_PROFILERS 30
#define MAX_ABSOLUTEPEAK_PROFILERS 100
//! Peak tolerance in milliseconds.
#define PEAK_TOLERANCE 10.0f

//////////////////////////////////////////////////////////////////////////
// CFrameProfilerSystem static variable.
//////////////////////////////////////////////////////////////////////////
CFrameProfileSystem* CFrameProfileSystem::s_pFrameProfileSystem = 0;

CryCriticalSection CFrameProfileSystem::m_staticProfilersLock;

//////////////////////////////////////////////////////////////////////////
// FrameProfilerSystem Implementation.
//////////////////////////////////////////////////////////////////////////

int CFrameProfileSystem::profile_callstack = 0;
int CFrameProfileSystem::profile_log = 0;
threadID CFrameProfileSystem::s_nFilterThreadId = 0;

//////////////////////////////////////////////////////////////////////////
CFrameProfileSystem::CFrameProfileSystem()
    : m_nCurSample(-1)
    , m_ProfilerThreads(GetCurrentThreadId())
{
    s_pFrameProfileSystem = this;
#ifdef WIN32
    hPsapiModule = NULL;
    pfGetProcessMemoryInfo = NULL;
    m_bNoPsapiDll = false;
#endif

    s_nFilterThreadId = 0;

    // Allocate space for 1024 profilers.
    m_profilers.reserve(1024);
    m_pCurrentCustomSection = 0;
    m_bEnabled = false;
    m_totalProfileTime = 0;
    m_frameStartTime = 0;
    m_frameTime = 0;
    m_callOverheadTime = 0;
    m_nCallOverheadTotal = 0;
    m_nCallOverheadRemainder = 0;
    m_nCallOverheadCalls = 0;

    m_frameSecAvg = 0.f;
    m_frameLostSecAvg = 0.f;
    m_frameOverheadSecAvg = 0.f;
    m_pRenderer = 0;
    m_displayQuantity = SELF_TIME;

    m_bCollect = false;
    m_nThreadSupport = 0;
    m_bDisplay = false;
    m_bDisplayMemoryInfo = false;
    m_bLogMemoryInfo = false;

    m_peakTolerance = PEAK_TOLERANCE;

    m_pGraphProfiler = 0;

    m_timeGraphCurrentPos = 0;
    m_bCollectionPaused = false;
    m_bDrawGraph = false;

    m_selectedRow = -1;

    m_bEnableHistograms = false;
    m_histogramsMaxPos = 200;
    m_histogramsHeight = 16;
    m_histogramsCurrPos = 0;

    m_bSubsystemFilterEnabled = false;
    m_subsystemFilter = PROFILE_RENDERER;
    m_maxProfileCount = 999;
    m_histogramScale = 100;

    m_bDisplayedProfilersValid = false;
    m_bNetworkProfiling = false;
    m_bMemoryProfiling = true;
    //m_pProfilers = &m_netTrafficProfilers;
    m_pProfilers = &m_profilers;

    m_nLastPageFaultCount = 0;
    m_nPagesFaultsLastFrame = 0;
    m_bPageFaultsGraph = false;
    m_nPagesFaultsPerSec = 0;

    m_pSampler = new CSampler;

    //////////////////////////////////////////////////////////////////////////
    // Initialize subsystems list.
    memset(m_subsystems, 0, sizeof(m_subsystems));

    for (uint32_t i = 0; i < PROFILE_LAST_SUBSYSTEM; ++i)
    {
        m_subsystems[i].name = GetProfiledSubsystemName((EProfiledSubsystem)i);
    }

    for (int i = 0; i < sizeof(m_subsystems) / sizeof(m_subsystems[0]); i++)
    {
        m_subsystems[i].budgetTime = 66.6f;
        m_subsystems[i].maxTime = 0.0f;
    }

    m_subsystems[PROFILE_AI].budgetTime = 5.0f;
    m_subsystems[PROFILE_ACTION].budgetTime = 3.0f;
    m_subsystems[PROFILE_GAME].budgetTime = 2.0f;

    m_peakDisplayDuration = 8.0f;
    m_offset = 0.0f;

    m_bRenderAdditionalSubsystems = false;
};

//////////////////////////////////////////////////////////////////////////
CFrameProfileSystem::~CFrameProfileSystem()
{
    g_bProfilerEnabled = false;
    delete m_pSampler;

    // Delete graphs for all frame profilers.
#ifdef WIN32
    if (hPsapiModule)
    {
        ::FreeLibrary(hPsapiModule);
    }
    hPsapiModule = NULL;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Init(ISystem* pSystem, int nThreadSupport)
{
    m_pSystem = pSystem;
    m_nThreadSupport = nThreadSupport;

    gEnv->callbackStartSection = &StartProfilerSection;
    gEnv->callbackEndSection = &EndProfilerSection;

    REGISTER_CVAR(profile_callstack, 0, 0, "Logs all Call Stacks of the selected profiler function for one frame");
    REGISTER_CVAR(profile_log, 0, 0, "Logs profiler output");
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Done()
{
    Profilers::const_iterator Iter(m_profilers.begin());
    Profilers::const_iterator IterEnd(m_profilers.end());
    CFrameProfiler* pFrameProfiler = NULL;

    for (; Iter != IterEnd; ++Iter)
    {
        pFrameProfiler = *Iter;
        SAFE_DELETE(pFrameProfiler->m_pGraph);
        SAFE_DELETE(pFrameProfiler->m_pOfflineHistory);

        // When this static object gets destroyed its dtor checks against m_pISystem.
        // We need to make sure to NULL it here so we do not access a dangling pointer during shutdown!
        pFrameProfiler->m_pISystem = NULL;
    }

    Iter = m_netTrafficProfilers.begin();
    IterEnd = m_netTrafficProfilers.end();

    for (; Iter != IterEnd; ++Iter)
    {
        pFrameProfiler = *Iter;
        SAFE_DELETE(pFrameProfiler->m_pGraph);
        SAFE_DELETE(pFrameProfiler->m_pOfflineHistory);

        // When this static object gets destroyed its dtor checks against m_pISystem.
        // We need to make sure to NULL it here so we do not access a dangling pointer during shutdown!
        pFrameProfiler->m_pISystem = NULL;
    }

    stl::free_container(m_profilers);
    stl::free_container(m_netTrafficProfilers);
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetProfiling(bool on, bool display, char* prefix, ISystem* pSystem)
{
    Enable(on, display);
    m_pPrefix = prefix;
    if (on && m_nCurSample < 0)
    {
        m_nCurSample = 0;
        CryLogAlways("Profiling data started (%s), prefix = \"%s\"", display ? "display only" : "tracing", prefix);

        m_frameTimeOfflineHistory.m_selfTime.reserve(1000);
        m_frameTimeOfflineHistory.m_count.reserve(1000);
    }
    else if (!on && m_nCurSample >= 0)
    {
        CryLogAlways("Profiling data finished");
        {
#ifdef WIN32
            // find the "frameprofileXX" filename for the file
            char outfilename[32] = "frameprofile.dat";
            // while there is such file already
            for (int i = 0; (GetFileAttributes (outfilename) != INVALID_FILE_ATTRIBUTES) && i < 1000; ++i)
            {
                azsprintf(outfilename, "frameprofile%02d.dat", i);
            }

            FILE* f = nullptr;
            azfopen(&f, outfilename, "wb");
            if (!f)
            {
                CryFatalError("Could not write profiling data to file!");
            }
            else
            {
                int i;
                // Find out how many profilers was active.
                int numProf = 0;

                for (i = 0; i < (int)m_pProfilers->size(); i++)
                {
                    CFrameProfiler* pProfiler = (*m_pProfilers)[i];
                    if (pProfiler && pProfiler->m_pOfflineHistory)
                    {
                        numProf++;
                    }
                }

                fwrite("FPROFDAT", 8, 1, f);                // magic header, for what its worth
                int version = 2;                            // bump this if any of the format below changes
                fwrite(&version, sizeof(int), 1, f);

                int numSamples = m_nCurSample;
                fwrite(&numSamples, sizeof(int), 1, f);   // number of samples per group (everything little endian)
                int mapsize = numProf + 1; // Plus 1 global.
                fwrite(&mapsize, sizeof(int), 1, f);

                // Write global profiler.
                fwrite("__frametime", strlen("__frametime") + 1, 1, f);
                int len = (int)m_frameTimeOfflineHistory.m_selfTime.size();
                assert(len == numSamples);
                for (i = 0; i < len; i++)
                {
                    fwrite(&m_frameTimeOfflineHistory.m_selfTime[i], 1, sizeof(int),   f);
                    fwrite(&m_frameTimeOfflineHistory.m_count[i], 1, sizeof(short), f);
                }
                ;

                // Write other profilers.
                for (i = 0; i < (int)m_pProfilers->size(); i++)
                {
                    CFrameProfiler* pProfiler = (*m_pProfilers)[i];
                    if (!pProfiler || !pProfiler->m_pOfflineHistory)
                    {
                        continue;
                    }

                    const char* name = GetFullName(pProfiler);
                    //int slen = strlen(name)+1;
                    fwrite(name, strlen(name) + 1, 1, f);

                    len = (int)pProfiler->m_pOfflineHistory->m_selfTime.size();
                    assert(len == numSamples);
                    for (int i = 0; i < len; i++)
                    {
                        fwrite(&pProfiler->m_pOfflineHistory->m_selfTime[i], 1, sizeof(int),   f);
                        fwrite(&pProfiler->m_pOfflineHistory->m_count[i], 1, sizeof(short), f);
                    }
                    ;

                    // Delete offline data, from profiler.
                    SAFE_DELETE(pProfiler->m_pOfflineHistory);
                }
                ;
                fclose(f);
                CryLogAlways("Profiling data saved to file '%s'", outfilename);
            };
#endif
        };
        m_frameTimeOfflineHistory.m_selfTime.clear();
        m_frameTimeOfflineHistory.m_count.clear();
        m_nCurSample = -1;
    }
    ;
};

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Enable(bool bCollect, bool bDisplay)
{
    if (m_bEnabled != bCollect)
    {
#ifdef WIN32
        if (bCollect)
        {
            CryLogAlways("SetThreadAffinityMask to %d", (int)1);
            if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0)
            {
                CryLogAlways("SetThreadAffinityMask Failed");
            }
        }
        else
        {
            DWORD_PTR mask1, mask2;
            GetProcessAffinityMask(GetCurrentProcess(), &mask1, &mask2);
            CryLogAlways("SetThreadAffinityMask to %d", (int)mask1);
            if (SetThreadAffinityMask(GetCurrentThread(), mask1) == 0)
            {
                CryLogAlways("SetThreadAffinityMask Failed");
            }
            if (m_bCollectionPaused)
            {
                UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
            }
        }
#endif


        Reset();
    }

    m_bEnabled = bCollect;
    m_bDisplay = bDisplay;
    m_bDisplayedProfilersValid = false;

    // add ourselves to the input system
    UpdateInputSystemStatus();
}

void CFrameProfileSystem::EnableHistograms(bool bEnableHistograms)
{
    m_bEnableHistograms = bEnableHistograms;
    m_bDisplayedProfilersValid = false;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartSampling(int nMaxSamples)
{
    if (m_pSampler)
    {
        m_pSampler->SetMaxSamples(nMaxSamples);
        m_pSampler->Start();
    }
}

//////////////////////////////////////////////////////////////////////////
CFrameProfiler* CFrameProfileSystem::GetProfiler(int index) const
{
    assert(index >= 0 && index < (int)m_profilers.size());
    return m_profilers[index];
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::Reset()
{
    //gEnv->callbackStartSection = &StartProfilerSection;
    //gEnv->callbackEndSection = &EndProfilerSection;

    m_absolutepeaks.clear();
    m_ProfilerThreads.Reset(m_pSystem);
    m_pCurrentCustomSection = 0;
    m_totalProfileTime = 0;
    m_frameStartTime = 0;
    m_frameTime = 0;
    m_callOverheadTime = 0;
    m_nCallOverheadTotal = 0;
    m_nCallOverheadRemainder = 0;
    m_nCallOverheadCalls = 0;
    m_frameSecAvg = 0.f;
    m_frameLostSecAvg = 0.f;
    m_frameOverheadSecAvg = 0.f;
    m_bCollectionPaused = false;

    int i;
    // Iterate over all profilers update their history and reset them.
    for (i = 0; i < (int)m_profilers.size(); i++)
    {
        CFrameProfiler* pProfiler = m_profilers[i];
        // Reset profiler.
        if (pProfiler)
        {
            pProfiler->m_totalTimeHistory.Clear();
            pProfiler->m_selfTimeHistory.Clear();
            pProfiler->m_countHistory.Clear();
            pProfiler->m_totalTime = 0;
            pProfiler->m_selfTime = 0;
            pProfiler->m_count = 0;
            pProfiler->m_displayedValue = 0;
            pProfiler->m_variance = 0;
            pProfiler->m_peak = 0;
        }
    }
    // Iterate over all profilers update their history and reset them.
    for (i = 0; i < (int)m_netTrafficProfilers.size(); i++)
    {
        CFrameProfiler* pProfiler = m_netTrafficProfilers[i];
        if (pProfiler)
        {
            // Reset profiler.
            pProfiler->m_totalTimeHistory.Clear();
            pProfiler->m_selfTimeHistory.Clear();
            pProfiler->m_countHistory.Clear();
            pProfiler->m_totalTime = 0;
            pProfiler->m_selfTime = 0;
            pProfiler->m_count = 0;
            pProfiler->m_peak = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddFrameProfiler(CFrameProfiler* pProfiler)
{
    CryAutoCriticalSection lock(m_profilersLock);
    ScopedSwitchToGlobalHeap useGlobalHeap;

    assert(pProfiler);
    if (!pProfiler)
    {
        return;
    }

    // Set default thread id.
    pProfiler->m_threadId = GetMainThreadId();
    if ((EProfiledSubsystem)pProfiler->m_subsystem == PROFILE_NETWORK_TRAFFIC)
    {
        m_netTrafficProfilers.push_back(pProfiler);
    }
    else
    {
        m_profilers.push_back(pProfiler);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RemoveFrameProfiler(CFrameProfiler* pProfiler)
{
    CryAutoCriticalSection lock(m_profilersLock);

    assert(pProfiler);
    if (!pProfiler)
    {
        return;
    }

    SAFE_DELETE(pProfiler->m_pGraph);
    SAFE_DELETE(pProfiler->m_pOfflineHistory);

    // When this static object gets destroyed its dtor checks against m_pISystem.
    // We need to make sure to NULL it here so we do not access a dangling pointer during shutdown!
    pProfiler->m_pISystem = NULL;

    if ((EProfiledSubsystem)pProfiler->m_subsystem == PROFILE_NETWORK_TRAFFIC)
    {
        stl::find_and_erase(m_netTrafficProfilers, pProfiler);
    }
    else
    {
        stl::find_and_erase(m_profilers, pProfiler);
    }
}

static int nMAX_THREADED_PROFILERS = 256;

void CFrameProfileSystem::SProfilerThreads::Reset(ISystem* pSystem)
{
    m_aThreadStacks[0].pProfilerSection = 0;
    if (!m_pReservedProfilers)
    {
        ScopedSwitchToGlobalHeap useGlobalHeap;

        // Allocate reserved profilers;
        for (int i = 0; i < nMAX_THREADED_PROFILERS; i++)
        {
            CFrameProfiler* pProf = new CFrameProfiler(pSystem, "");
            pProf->m_pNextThread = m_pReservedProfilers;
            m_pReservedProfilers = pProf;
        }
    }
}

CFrameProfiler* CFrameProfileSystem::SProfilerThreads::NewThreadProfiler(CFrameProfiler* pMainProfiler, TThreadId nThreadId)
{
    // Create new profiler for thread.
    CryWriteLock(&m_lock);
    if (!m_pReservedProfilers)
    {
        CryReleaseWriteLock(&m_lock);
        return 0;
    }
    CFrameProfiler* pProfiler = m_pReservedProfilers;
    m_pReservedProfilers = pProfiler->m_pNextThread;

    // Init.
    memset(pProfiler, 0, sizeof(*pProfiler));
    pProfiler->m_name = pMainProfiler->m_name;
    pProfiler->m_subsystem = pMainProfiler->m_subsystem;
    pProfiler->m_pISystem = pMainProfiler->m_pISystem;
    pProfiler->m_threadId = nThreadId;

    // Insert in frame profiler list.
    pProfiler->m_pNextThread = pMainProfiler->m_pNextThread;
    pMainProfiler->m_pNextThread = pProfiler;

    CryReleaseWriteLock(&m_lock);
    return pProfiler;
}


//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartProfilerSection(CFrameProfilerSection* pSection)
{
    //if (CFrameProfileSystem::s_pFrameProfileSystem->m_bCollectionPaused)
    //  return;

    // Find thread instance to collect profiles in.
    TThreadId nThreadId = GetCurrentThreadId();
    if (nThreadId != s_pFrameProfileSystem->GetMainThreadId())
    {
        if (!s_pFrameProfileSystem->m_nThreadSupport)
        {
            pSection->m_pFrameProfiler = NULL;
            return;
        }

        pSection->m_pFrameProfiler = s_pFrameProfileSystem->m_ProfilerThreads.GetThreadProfiler(pSection->m_pFrameProfiler, nThreadId);
        if (!pSection->m_pFrameProfiler)
        {
            return;
        }
    }

    // Push section on stack for current thread.
    s_pFrameProfileSystem->m_ProfilerThreads.PushSection(pSection, nThreadId);
    pSection->m_excludeTime = 0;
    pSection->m_startTime = CryGetTicks();
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndProfilerSection(CFrameProfilerSection* pSection)
{
    AccumulateProfilerSection(pSection);

    // Not in a SLICE_AND_SLEEP here, account for call overhead.
    if (pSection->m_pParent)
    {
        pSection->m_pParent->m_excludeTime += s_pFrameProfileSystem->m_callOverheadTime;
    }

    s_pFrameProfileSystem->m_ProfilerThreads.PopSection(pSection, pSection->m_pFrameProfiler->m_threadId);
}

void CFrameProfileSystem::AccumulateProfilerSection(CFrameProfilerSection* pSection)
{
    //if (CFrameProfileSystem::s_pFrameProfileSystem->m_bCollectionPaused)
    //  return;

    int64 endTime = CryGetTicks();

    CFrameProfiler* pProfiler = pSection->m_pFrameProfiler;
    if (!pProfiler)
    {
        return;
    }

    assert(GetCurrentThreadId() == pProfiler->m_threadId);

    int64 totalTime = endTime - pSection->m_startTime;
    int64 selfTime = totalTime - pSection->m_excludeTime;
    if (totalTime < 0)
    {
        totalTime = 0;
    }
    if (selfTime < 0)
    {
        selfTime = 0;
    }

    if (s_nFilterThreadId && GetCurrentThreadId() != s_nFilterThreadId)
    {
        selfTime = 0;
        totalTime = 0;
        pProfiler->m_count = 0;
    }

    pProfiler->m_count++;
    pProfiler->m_selfTime += selfTime;
    pProfiler->m_totalTime += totalTime;
    pProfiler->m_peak = max(pProfiler->m_peak, selfTime);

    if (pSection->m_pParent)
    {
        // If we have parent, add this counter total time (but not call overhead time, because
        // if this is a SLICE_AND_SLEEP instead of a real end of frame, we might not have entered
        // the section this frame and we certainly haven't left it) to parent exclude time.
        pSection->m_pParent->m_excludeTime += totalTime;
        if (!pProfiler->m_pParent && pSection->m_pParent->m_pFrameProfiler)
        {
            pSection->m_pParent->m_pFrameProfiler->m_bHaveChildren = 1;
            pProfiler->m_pParent = pSection->m_pParent->m_pFrameProfiler;
        }
    }
    else
    {
        pProfiler->m_pParent = 0;
    }

    if (profile_callstack)
    {
        if (pProfiler == s_pFrameProfileSystem->m_pGraphProfiler)
        {
            float fMillis = 1000.f * gEnv->pTimer->TicksToSeconds(totalTime);
            CryLogAlways("Function Profiler: %s  (time=%.2fms)", s_pFrameProfileSystem->GetFullName(pSection->m_pFrameProfiler), fMillis);
            GetISystem()->debug_LogCallStack();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartMemoryProfilerSection(CFrameProfilerSection* pSection)
{
    CryAutoCriticalSection lock(m_staticProfilersLock);

    // Find thread instance to collect profiles in.
    TThreadId nThreadId = GetCurrentThreadId();
    pSection->m_pFrameProfiler = s_pFrameProfileSystem->m_ProfilerThreads.GetThreadProfiler(pSection->m_pFrameProfiler, nThreadId);
    if (pSection->m_pFrameProfiler)
    {
        // Push section on stack for current thread.
        s_pFrameProfileSystem->m_ProfilerThreads.PushSection(pSection, nThreadId);
        pSection->m_excludeTime = 0;
        pSection->m_startTime = CryMemoryGetAllocatedSize();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndMemoryProfilerSection(CFrameProfilerSection* pSection)
{
    CryAutoCriticalSection lock(m_staticProfilersLock);

    int64 endTime = CryMemoryGetAllocatedSize();
    TThreadId nThreadId = GetCurrentThreadId();
    CFrameProfiler* pProfiler = pSection->m_pFrameProfiler;
    if (!pProfiler)
    {
        return;
    }

    assert(nThreadId == pProfiler->m_threadId);

    int64 totalTime = endTime - pSection->m_startTime;
    int64 selfTime = totalTime - pSection->m_excludeTime;
    if (totalTime < 0)
    {
        totalTime = 0;
    }
    if (selfTime < 0)
    {
        selfTime = 0;
    }

    if (s_nFilterThreadId && nThreadId != s_nFilterThreadId)
    {
        selfTime = 0;
        totalTime = 0;
        pProfiler->m_count = 0;
    }

    // Ignore allocation functions.
    if (0 == azstricmp(pProfiler->m_name, "CryMalloc") ||
        0 == azstricmp(pProfiler->m_name, "CryRealloc") ||
        0 == azstricmp(pProfiler->m_name, "CryFree"))
    {
        selfTime = 0;
        totalTime = 0;
    }

    pProfiler->m_count++;
    pProfiler->m_selfTime += selfTime;
    pProfiler->m_totalTime += totalTime;
    pProfiler->m_peak = max(pProfiler->m_peak, selfTime);

    s_pFrameProfileSystem->m_ProfilerThreads.PopSection(pSection, pProfiler->m_threadId);
    if (pSection->m_pParent)
    {
        // If we have parent, add this counter total time to parent exclude time.
        pSection->m_pParent->m_excludeTime += totalTime;
        if (!pProfiler->m_pParent && pSection->m_pParent->m_pFrameProfiler)
        {
            pSection->m_pParent->m_pFrameProfiler->m_bHaveChildren = 1;
            pProfiler->m_pParent = pSection->m_pParent->m_pFrameProfiler;
        }
    }
    else
    {
        pProfiler->m_pParent = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
CFrameProfilerSection const* CFrameProfileSystem::GetCurrentProfilerSection()
{
    // Return current (main-thread) profile section.
    return m_ProfilerThreads.GetMainSection();
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartCustomSection(CCustomProfilerSection* pSection)
{
    if (!m_bNetworkProfiling)
    {
        return;
    }

    pSection->m_excludeValue = 0;
    pSection->m_pParent = m_pCurrentCustomSection;
    m_pCurrentCustomSection = pSection;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndCustomSection(CCustomProfilerSection* pSection)
{
    if (!m_bNetworkProfiling || m_bCollectionPaused)
    {
        return;
    }

    int total = *pSection->m_pValue;
    int self = total - pSection->m_excludeValue;

    CFrameProfiler* pProfiler = pSection->m_pFrameProfiler;
    pProfiler->m_count++;
    pProfiler->m_selfTime += self;
    pProfiler->m_totalTime += total;
    pProfiler->m_peak = max(pProfiler->m_peak, (int64)self);

    m_pCurrentCustomSection = pSection->m_pParent;
    if (m_pCurrentCustomSection)
    {
        // If we have parent, add this counter total time to parent exclude time.
        m_pCurrentCustomSection->m_pFrameProfiler->m_bHaveChildren = 1;
        m_pCurrentCustomSection->m_excludeValue += total;
        pProfiler->m_pParent = m_pCurrentCustomSection->m_pFrameProfiler;
    }
    else
    {
        pProfiler->m_pParent = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::StartFrame()
{
    m_bCollect = m_bEnabled && !m_bCollectionPaused;

    if (m_bCollect)
    {
        m_ProfilerThreads.Reset(m_pSystem);
        m_pCurrentCustomSection = 0;
        m_frameStartTime = CryGetTicks();

        // Accumulate overhead measurement, in running average
        static const int nPROFILE_CALLS_PER_FRAME = 16;
        static CFrameProfiler staticFrameProfiler(GetISystem(), "CallOverhead", PROFILE_SYSTEM);

        int64 nTicks = CryGetTicks();
        for (int n = 0; n < nPROFILE_CALLS_PER_FRAME; n++)
        {
            CFrameProfilerSection frameProfilerSection(&staticFrameProfiler);
        }
        nTicks = CryGetTicks() - nTicks;

        m_nCallOverheadTotal += nTicks;
        m_nCallOverheadCalls += nPROFILE_CALLS_PER_FRAME;

        if (m_nCallOverheadCalls > 1024)
        {
            m_nCallOverheadTotal = m_nCallOverheadTotal * 1024 / m_nCallOverheadCalls;
            m_nCallOverheadCalls = 1024;
        }

        // Compute per-call overhead, dithering error into remainder for accurate average over time.
        m_callOverheadTime = (m_nCallOverheadTotal + m_nCallOverheadRemainder + m_nCallOverheadCalls / 2) / m_nCallOverheadCalls;
        m_nCallOverheadRemainder += m_nCallOverheadTotal - m_callOverheadTime * m_nCallOverheadCalls;
    }
    g_bProfilerEnabled = m_bCollect;
}

//////////////////////////////////////////////////////////////////////////
float CFrameProfileSystem::TranslateToDisplayValue(int64 val)
{
    if (!m_bNetworkProfiling && !m_bMemoryProfiling)
    {
        return gEnv->pTimer->TicksToSeconds(val) * 1000;
    }
    else if (m_displayQuantity == ALLOCATED_MEMORY)
    {
        return (float)(val >> 10); // In Kilobytes
    }
    else if (m_displayQuantity == ALLOCATED_MEMORY_BYTES)
    {
        return (float)val; // In bytes
    }
    else if (m_bNetworkProfiling)
    {
        return (float)val;
    }
    return (float)val;
}

const char* CFrameProfileSystem::GetFullName(CFrameProfiler* pProfiler)
{
    static char sNameBuffer[256];
    sprintf_s(sNameBuffer, sizeof(sNameBuffer), pProfiler->m_name);

#if !defined(_MSC_VER) // __FUNCTION__ only contains classname on MSVC, for other function we have __PRETTY_FUNCTION__, so we need to strip return / argument types
    {
        char* pEnd = (char*)strchr(sNameBuffer, '(');
        if (pEnd)
        {
            *pEnd = 0;
            while (*(pEnd) != ' ' && *(pEnd) != '*' && pEnd != (sNameBuffer - 1))
            {
                --pEnd;
            }
            memmove(sNameBuffer, pEnd + 1, &sNameBuffer[sizeof(sNameBuffer)] - (pEnd + 1));
        }
    }
#endif

    if (pProfiler->m_threadId == GetMainThreadId())
    {
        return sNameBuffer;
    }

    // Add thread name.
    static char sFullName[256];
    if (!pProfiler->m_pNextThread || m_nThreadSupport > 1)
    {
        const char* sThreadName = CryThreadGetName(pProfiler->m_threadId);
        if (sThreadName)
        {
            azsnprintf(sFullName, sizeof(sFullName), "%s @%s", sNameBuffer, sThreadName);
        }
        else
        {
            azsnprintf(sFullName, sizeof(sFullName), "%s @%" PRI_THREADID "", sNameBuffer, pProfiler->m_threadId);
        }
    }
    else
    {
        int nThreads = 1;
        while (pProfiler->m_pNextThread)
        {
            pProfiler = pProfiler->m_pNextThread;
            nThreads++;
        }
        azsnprintf(sFullName, sizeof(sFullName), "%s @(%d threads)", sNameBuffer, nThreads);
    }

    sFullName[sizeof(sFullName) - 1] = 0;
    return sFullName;
}

//////////////////////////////////////////////////////////////////////////
const char* CFrameProfileSystem::GetModuleName(CFrameProfiler* pProfiler)
{
    return m_subsystems[pProfiler->m_subsystem].name;
}

const char* CFrameProfileSystem::GetModuleName(int num) const
{
    assert(num >= 0 && num < PROFILE_LAST_SUBSYSTEM);
    if (num < GetModuleCount())
    {
        return m_subsystems[num].name;
    }
    else
    {
        return 0;
    }
}

const int CFrameProfileSystem::GetModuleCount(void) const
{
    return (int)PROFILE_LAST_SUBSYSTEM;
}

const float CFrameProfileSystem::GetOverBudgetRatio(int modulenumber) const
{
    assert(modulenumber >= 0 && modulenumber < PROFILE_LAST_SUBSYSTEM);
    if (modulenumber < GetModuleCount() && m_subsystems[modulenumber].totalAnalized)
    {
        return (float)m_subsystems[modulenumber].totalOverBudget / (float)m_subsystems[modulenumber].totalAnalized;
    }
    else
    {
        return 0.0f;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EndFrame()
{
    if (m_pSampler)
    {
        m_pSampler->Update();
    }

    if (!m_bEnabled)
    {
        m_totalProfileTime = 0;
    }

    if (!m_bEnabled && !m_bNetworkProfiling)
    {
        static ICVar* pDisplayInfo = NULL;

        if (pDisplayInfo == NULL)
        {
            pDisplayInfo = gEnv->pConsole->GetCVar("r_DisplayInfo");
            assert(pDisplayInfo != NULL);
        }

        // We want single subsystems to be able to still sample profile data even if the Profiler is disabled.
        // This is needed for r_DisplayInfo 2 for instance as it displays subsystems' performance data.
        // Note: This is currently meant to be as light weight as possible and to not run the entire Profiler.
        // Therefore we're ignoring overhead accumulated in StartFrame for instance.
        // Feel free to add that if it proves necessary.
        if (pDisplayInfo != NULL && pDisplayInfo->GetIVal() > 1)
        {
            CryAutoCriticalSection lock(m_profilersLock);

            // Update profilers that are set to always update.
            Profilers::const_iterator Iter(m_pProfilers->begin());
            Profilers::const_iterator const IterEnd(m_pProfilers->end());

            for (; Iter != IterEnd; ++Iter)
            {
                CFrameProfiler* const pProfiler = (*Iter);

                if (pProfiler != NULL && pProfiler->m_bAlwaysCollect)
                {
                    float const fSelfTime = TranslateToDisplayValue(pProfiler->m_selfTime);
                    pProfiler->m_selfTimeHistory.Add(fSelfTime);

                    // Reset profiler.
                    pProfiler->m_totalTime  = 0;
                    pProfiler->m_selfTime   = 0;
                    pProfiler->m_count  = 0;
                }
            }
        }

        return;
    }

#if defined(WIN32)

    const AzFramework::InputChannel* inputChannelScrollLock = AzFramework::InputChannelRequests::FindInputChannel(AzFramework::InputDeviceKeyboard::Key::WindowsSystemScrollLock);
    bool bPaused = (inputChannelScrollLock ? inputChannelScrollLock->IsActive() : false);

    // Will pause or resume collection.
    if (bPaused != m_bCollectionPaused)
    {
        if (bPaused)
        {
            // Must be paused.
            UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
        }
        else
        {
            // Must be resumed.
            UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
        }
    }
    if (m_bCollectionPaused != bPaused)
    {
        m_bDisplayedProfilersValid = false;
    }
    m_bCollectionPaused = bPaused;

#endif

    if (m_bCollectionPaused || (!m_bCollect && !m_bNetworkProfiling))
    {
        return;
    }

    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

    float smoothTime = gEnv->pTimer->TicksToSeconds(m_totalProfileTime);
    float smoothFactor = 1.f - gEnv->pTimer->GetProfileFrameBlending(&smoothTime);

    int64 endTime = CryGetTicks();
    m_frameTime = endTime - m_frameStartTime;
    m_totalProfileTime += m_frameTime;

    if (m_bSubsystemFilterEnabled)
    {
        CryAutoCriticalSection lock(m_profilersLock);

        Profilers::const_iterator Iter(m_pProfilers->begin());
        Profilers::const_iterator const IterEnd(m_pProfilers->end());

        for (; Iter != IterEnd; ++Iter)
        {
            CFrameProfiler* const pProfiler = (*Iter);

            if (pProfiler != NULL && pProfiler->m_subsystem != (uint8)m_subsystemFilter)
            {
                pProfiler->m_totalTime = 0;
                pProfiler->m_selfTime = 0;
                pProfiler->m_count = 0;
                pProfiler->m_variance = 0;
                pProfiler->m_peak = 0;
            }
        }
    }

    if (gEnv->pStatoscope)
    {
        gEnv->pStatoscope->SetCurrentProfilerRecords(m_pProfilers);
    }

    //////////////////////////////////////////////////////////////////////////
    // Lets see how many page faults we got.
    //////////////////////////////////////////////////////////////////////////
#if defined(WIN32) || defined(WIN64)

    // PSAPI is not supported on window9x
    // so, don't use it
    if (!m_bNoPsapiDll)
    {
        // Load psapi dll.
        if (!pfGetProcessMemoryInfo)
        {
            hPsapiModule = ::LoadLibraryA("psapi.dll");
            if (hPsapiModule)
            {
                pfGetProcessMemoryInfo = (FUNC_GetProcessMemoryInfo)(::GetProcAddress(hPsapiModule, "GetProcessMemoryInfo"));
            }
            else
            {
                m_bNoPsapiDll = true;
            }
        }
        if (pfGetProcessMemoryInfo)
        {
            PROCESS_MEMORY_COUNTERS pc;
            pfGetProcessMemoryInfo(GetCurrentProcess(), &pc, sizeof(pc));
            m_nPagesFaultsLastFrame = (int)(pc.PageFaultCount - m_nLastPageFaultCount);
            m_nLastPageFaultCount = pc.PageFaultCount;
            static float fLastPFTime = 0;
            static int nPFCounter = 0;
            nPFCounter += m_nPagesFaultsLastFrame;
            float fCurr = gEnv->pTimer->TicksToSeconds(endTime);
            if ((fCurr - fLastPFTime) >= 1.f)
            {
                fLastPFTime = fCurr;
                m_nPagesFaultsPerSec = nPFCounter;
                nPFCounter = 0;
            }
        }
    }
#endif
    //////////////////////////////////////////////////////////////////////////


    static ICVar* pVarMode = m_pSystem->GetIConsole()->GetCVar("profile_weighting");
    int nWeightMode = pVarMode ? pVarMode->GetIVal() : 0;

    int64 selfAccountedTime = 0;

    float fPeakTolerance = m_peakTolerance;
    if (m_bMemoryProfiling)
    {
        fPeakTolerance = 0;
    }

    // Iterate over all profilers update their history and reset them.
    int nProfileCalls = 0;

    CryAutoCriticalSection lock(m_profilersLock);

    Profilers::const_iterator Iter(m_pProfilers->begin());
    Profilers::const_iterator const IterEnd(m_pProfilers->end());

    for (; Iter != IterEnd; ++Iter)
    {
        CFrameProfiler* const pProfiler = (*Iter);

        // Skip this profiler if its filtered out.
        if (pProfiler != NULL && (!m_bSubsystemFilterEnabled || (m_bSubsystemFilterEnabled && pProfiler->m_subsystem == (uint8)m_subsystemFilter)))
        {
            //filter stall profilers
            if ((int)m_displayQuantity != STALL_TIME && pProfiler->m_isStallProfiler ||
                (int)m_displayQuantity == STALL_TIME && !pProfiler->m_isStallProfiler)
            {
                continue;
            }

            if (pProfiler->m_threadId == GetMainThreadId())
            {
                selfAccountedTime += pProfiler->m_selfTime;
                nProfileCalls += pProfiler->m_count;
            }

            float aveValue = 0;
            float currentValue = 0;

            if (m_nThreadSupport == 1 && pProfiler->m_threadId == m_ProfilerThreads.GetMainThreadId())
            {
                // Combine all non-main thread stats into 1
                if (CFrameProfiler* pThread = pProfiler->m_pNextThread)
                {
                    for (CFrameProfiler* pOtherThread = pThread->m_pNextThread; pOtherThread; pOtherThread = pOtherThread->m_pNextThread)
                    {
                        pThread->m_count += pOtherThread->m_count;
                        pOtherThread->m_count = 0;
                        pThread->m_selfTime += pOtherThread->m_selfTime;
                        pOtherThread->m_selfTime = 0;
                        pThread->m_totalTime += pOtherThread->m_totalTime;
                        pOtherThread->m_totalTime = 0;
                        pOtherThread->m_displayedValue = 0;
                    }
                }
            }

            float fDisplay_TotalTime = TranslateToDisplayValue(pProfiler->m_totalTime);
            float fDisplay_SelfTime = TranslateToDisplayValue(pProfiler->m_selfTime);

            bool bEnablePeaks = nWeightMode < 3;

            switch ((int)m_displayQuantity)
            {
            case SELF_TIME:
            case PEAK_TIME:
            case COUNT_INFO:
            case STALL_TIME:
                currentValue = fDisplay_SelfTime;
                aveValue = pProfiler->m_selfTimeHistory.GetAverage();
                break;
            case TOTAL_TIME:
                currentValue = fDisplay_TotalTime;
                aveValue = pProfiler->m_totalTimeHistory.GetAverage();
                break;
            case SELF_TIME_EXTENDED:
                currentValue = fDisplay_SelfTime;
                aveValue = pProfiler->m_selfTimeHistory.GetAverage();
                bEnablePeaks = false;
                break;
            case TOTAL_TIME_EXTENDED:
                currentValue = fDisplay_TotalTime;
                aveValue = pProfiler->m_totalTimeHistory.GetAverage();
                bEnablePeaks = false;
                break;
            case SUBSYSTEM_INFO:
                currentValue = (float)pProfiler->m_count;
                aveValue = pProfiler->m_selfTimeHistory.GetAverage();
                if (pProfiler->m_subsystem < PROFILE_LAST_SUBSYSTEM)
                {
                    m_subsystems[pProfiler->m_subsystem].selfTime += aveValue;
                }
                break;
            case STANDARD_DEVIATION:
                // Standard Deviation.
                aveValue = pProfiler->m_selfTimeHistory.GetStdDeviation();
                aveValue *= 100.0f;
                currentValue = aveValue;
                break;

            case ALLOCATED_MEMORY:
                //currentValue = pProfiler->m_selfTime / 1024.0f;
                //FIX this
                //fDisplay_SelfTime = fDisplay_SelfTime;
                //fDisplay_TotalTime = fDisplay_TotalTime;
                currentValue = pProfiler->m_selfTimeHistory.GetMax();
                aveValue = pProfiler->m_selfTimeHistory.GetAverage();
                break;
            }
            ;

            if ((SUBSYSTEM_INFO != m_displayQuantity) && (GetAdditionalSubsystems()))
            {
                float faveValue = pProfiler->m_selfTimeHistory.GetAverage();
                if (pProfiler->m_subsystem < PROFILE_LAST_SUBSYSTEM)
                {
                    m_subsystems[pProfiler->m_subsystem].selfTime += faveValue;
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // Records Peaks.
            uint64 frameID = gEnv->pRenderer->GetFrameID(false);
            if (bEnablePeaks)
            {
                float prevValue = pProfiler->m_selfTimeHistory.GetLast();
                float peakValue = fDisplay_SelfTime;

                if (pProfiler->m_latestFrame != frameID - 1)
                {
                    prevValue = 0.0f;
                }

                if ((peakValue - prevValue) > fPeakTolerance)
                {
                    SPeakRecord peak;
                    peak.pProfiler = pProfiler;
                    peak.peakValue = peakValue;
                    peak.avarageValue = pProfiler->m_selfTimeHistory.GetAverage();
                    peak.count = pProfiler->m_count;
                    peak.pageFaults = m_nPagesFaultsLastFrame;
                    peak.when = gEnv->pTimer->TicksToSeconds(m_totalProfileTime);
                    AddPeak(peak);

                    // Call peak callbacks.
                    if (!m_peakCallbacks.empty())
                    {
                        for (int n = 0; n < (int)m_peakCallbacks.size(); n++)
                        {
                            m_peakCallbacks[n]->OnFrameProfilerPeak(pProfiler, peakValue);
                        }
                    }
                }
            }
            //////////////////////////////////////////////////////////////////////////

            pProfiler->m_latestFrame = frameID;
            pProfiler->m_totalTimeHistory.Add(fDisplay_TotalTime);
            pProfiler->m_selfTimeHistory.Add(fDisplay_SelfTime);
            pProfiler->m_countHistory.Add(pProfiler->m_count);

            pProfiler->m_displayedValue = pProfiler->m_displayedValue * smoothFactor + currentValue * (1.0f - smoothFactor);
            float variance = square(currentValue - aveValue);
            pProfiler->m_variance = pProfiler->m_variance * smoothFactor + variance * (1.0f - smoothFactor);

            if (m_bMemoryProfiling)
            {
                pProfiler->m_displayedValue = currentValue;
            }

            if (m_bEnableHistograms)
            {
                if (!pProfiler->m_pGraph)
                {
                    // Create graph.
                    pProfiler->m_pGraph = new CFrameProfilerGraph;
                }
                // Update values in histogram graph.
                if (m_histogramsMaxPos != pProfiler->m_pGraph->m_data.size())
                {
                    pProfiler->m_pGraph->m_width = m_histogramsMaxPos;
                    pProfiler->m_pGraph->m_height = m_histogramsHeight;
                    pProfiler->m_pGraph->m_data.resize(m_histogramsMaxPos);
                }
                float millis;
                if (m_displayQuantity == TOTAL_TIME || m_displayQuantity == TOTAL_TIME_EXTENDED)
                {
                    millis = m_histogramScale * pProfiler->m_totalTimeHistory.GetLast();
                }
                else
                {
                    millis = m_histogramScale * pProfiler->m_selfTimeHistory.GetLast();
                }
                if (millis < 0)
                {
                    millis = 0;
                }
                if (millis > 255)
                {
                    millis = 255;
                }
                pProfiler->m_pGraph->m_data[m_histogramsCurrPos] = 255 - FtoI(millis); // must use ftoi.
            }

            if (m_nCurSample >= 0)
            {
                UpdateOfflineHistory(pProfiler);
            }

            // Reset profiler.
            pProfiler->m_totalTime = 0;
            pProfiler->m_selfTime = 0;
            pProfiler->m_peak = 0;
            pProfiler->m_count = 0;
        }
    }

    for (int i = 0; i < PROFILE_LAST_SUBSYSTEM; i++)
    {
        m_subsystems[i].totalAnalized++;
        if (m_subsystems[i].selfTime > m_subsystems[i].budgetTime)
        {
            m_subsystems[i].totalOverBudget++;
        }

        m_subsystems[i].maxTime = (float)fsel(m_subsystems[i].maxTime - m_subsystems[i].selfTime, m_subsystems[i].maxTime, m_subsystems[i].selfTime);
    }

    float frameSec = gEnv->pTimer->TicksToSeconds(m_frameTime);
    m_frameSecAvg = m_frameSecAvg * smoothFactor + frameSec * (1.0f - smoothFactor);

    float frameLostSec = gEnv->pTimer->TicksToSeconds(m_frameTime - selfAccountedTime);
    m_frameLostSecAvg = m_frameLostSecAvg * smoothFactor + frameLostSec * (1.0f - smoothFactor);

    float overheadtime = gEnv->pTimer->TicksToSeconds(nProfileCalls * m_nCallOverheadTotal / m_nCallOverheadCalls);
    m_frameOverheadSecAvg = m_frameOverheadSecAvg * smoothFactor + overheadtime * (1.0f - smoothFactor);

    if (m_nCurSample >= 0)
    {
        // Keep offline global time history.
        m_frameTimeOfflineHistory.m_selfTime.push_back(FtoI(frameSec * 1e6f));
        m_frameTimeOfflineHistory.m_count.push_back(1);
        m_nCurSample++;
    }
    //AdvanceFrame( m_pSystem );

    // Reset profile callstack var.
    profile_callstack = 0;
}

void CFrameProfileSystem::OnSliceAndSleep()
{
    m_ProfilerThreads.OnEnterSliceAndSleep(GetCurrentThreadId());
    EndFrame();
    m_ProfilerThreads.OnLeaveSliceAndSleep(GetCurrentThreadId());
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::UpdateOfflineHistory(CFrameProfiler* pProfiler)
{
    if (!pProfiler->m_pOfflineHistory)
    {
        pProfiler->m_pOfflineHistory = new CFrameProfilerOfflineHistory;
        pProfiler->m_pOfflineHistory->m_count.reserve(1000 + m_nCurSample * 2);
        pProfiler->m_pOfflineHistory->m_selfTime.reserve(1000 + m_nCurSample * 2);
    }
    int prevCont = (int)pProfiler->m_pOfflineHistory->m_selfTime.size();
    int newCount = m_nCurSample + 1;
    pProfiler->m_pOfflineHistory->m_selfTime.resize(newCount);
    pProfiler->m_pOfflineHistory->m_count.resize(newCount);

    uint32 micros = FtoI(gEnv->pTimer->TicksToSeconds(pProfiler->m_selfTime) * 1e6f);
    uint16 count = pProfiler->m_count;
    for (int i = prevCont; i < newCount; i++)
    {
        pProfiler->m_pOfflineHistory->m_selfTime[i] = micros;
        pProfiler->m_pOfflineHistory->m_count[i] = count;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddPeak(SPeakRecord& peak)
{
    // Add peak.
    if (m_peaks.size() > MAX_PEAK_PROFILERS)
    {
        m_peaks.pop_back();
    }

    if (gEnv->IsDedicated())
    {
        m_pSystem->GetILog()->Log("Peak: name:'%s' val:%.2f avg:%.2f cnt:%d",
            (const char*)GetFullName(peak.pProfiler),
            peak.peakValue,
            peak.avarageValue,
            peak.count);
    }

    /*
    // Check to see if this function is already a peak.
    for (int i = 0; i < (int)m_peaks.size(); i++)
    {
        if (m_peaks[i].pProfiler == peak.pProfiler)
        {
            m_peaks.erase( m_peaks.begin()+i );
            break;
        }
    }
    */
    m_peaks.insert(m_peaks.begin(), peak);

    // Add to absolute value

    for (int i = 0; i < (int)m_absolutepeaks.size(); i++)
    {
        if (m_absolutepeaks[i].pProfiler == peak.pProfiler)
        {
            if (m_absolutepeaks[i].peakValue < peak.peakValue)
            {
                m_absolutepeaks.erase(m_absolutepeaks.begin() + i);
                break;
            }
            else
            {
                return;
            }
        }
    }

    bool bInserted = false;
    for (size_t i = 0; i < m_absolutepeaks.size(); ++i)
    {
        if (m_absolutepeaks[i].peakValue < peak.peakValue)
        {
            m_absolutepeaks[i] = peak;
            bInserted = true;
            break;
        }
    }

    if (!bInserted && m_absolutepeaks.size() < MAX_ABSOLUTEPEAK_PROFILERS)
    {
        m_absolutepeaks.push_back(peak);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetDisplayQuantity(EDisplayQuantity quantity)
{
    m_displayQuantity = quantity;
    m_bDisplayedProfilersValid = false;
    if (m_displayQuantity == SELF_TIME_EXTENDED || m_displayQuantity == TOTAL_TIME_EXTENDED)
    {
        EnableHistograms(true);
    }
    else
    {
        EnableHistograms(false);
    }

    if (m_displayQuantity == ALLOCATED_MEMORY || m_displayQuantity == ALLOCATED_MEMORY_BYTES)
    {
        m_bMemoryProfiling = true;
        gEnv->callbackStartSection = &StartMemoryProfilerSection;
        gEnv->callbackEndSection = &EndMemoryProfilerSection;
    }
    else
    {
        gEnv->callbackStartSection = &StartProfilerSection;
        gEnv->callbackEndSection = &EndProfilerSection;
        m_bMemoryProfiling = false;
    }
};

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetSubsystemFilter(bool bFilterSubsystem, EProfiledSubsystem subsystem)
{
    m_bSubsystemFilterEnabled = bFilterSubsystem;
    m_subsystemFilter = subsystem;
    m_bDisplayedProfilersValid = false;
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetSubsystemFilterThread(const char* sFilterThreadName)
{
    if (sFilterThreadName[0] != 0)
    {
        s_nFilterThreadId = GetISystem()->GetIThreadTaskManager()->GetThreadByName(sFilterThreadName);
    }
    else
    {
        s_nFilterThreadId = 0;
    }
    if (s_nFilterThreadId)
    {
        m_filterThreadName = sFilterThreadName;
    }
    else
    {
        m_filterThreadName = "";
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::SetSubsystemFilter(const char* szFilterName)
{
    bool bFound = false;
    for (int i = 0; i < PROFILE_LAST_SUBSYSTEM; i++)
    {
        if (!m_subsystems[i].name)
        {
            continue;
        }
        if (azstricmp(m_subsystems[i].name, szFilterName) == 0)
        {
            SetSubsystemFilter(true, (EProfiledSubsystem)i);
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        // Check for count limit.
        int nCount = atoi(szFilterName);
        if (nCount > 0)
        {
            m_maxProfileCount = nCount;
        }
        else
        {
            SetSubsystemFilter(false, PROFILE_ANY);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::AddPeaksListener(IFrameProfilePeakCallback* pPeakCallback)
{
    ScopedSwitchToGlobalHeap useGlobalHeap;

    // Only add one time.
    stl::push_back_unique(m_peakCallbacks, pPeakCallback);
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::RemovePeaksListener(IFrameProfilePeakCallback* pPeakCallback)
{
    stl::find_and_erase(m_peakCallbacks, pPeakCallback);
}

//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::EnableMemoryProfile(bool bEnable)
{
    if (bEnable != m_bDisplayMemoryInfo)
    {
        m_bLogMemoryInfo = true;
    }
    m_bDisplayMemoryInfo = bEnable;
}

//////////////////////////////////////////////////////////////////////////
bool CFrameProfileSystem::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
{
    bool ret = false;

    //only react to keyboard input
    if (AzFramework::InputDeviceKeyboard::IsKeyboardDevice(inputChannel.GetInputDevice().GetInputDeviceId()))
    {
        //and only if the key was just pressed
        if (inputChannel.IsStateBegan())
        {
            const AzFramework::InputChannelId& channelId = inputChannel.GetInputChannelId();
            if (channelId == AzFramework::InputDeviceKeyboard::Key::WindowsSystemScrollLock)
            {
                // toggle collection pause
                m_bCollectionPaused = !m_bCollectionPaused;
                m_bDisplayedProfilersValid = false;

                {
                    CryAutoCriticalSection lock(m_profilersLock);

                    for (Profilers::iterator it = m_pProfilers->begin(); m_pProfilers->end() != it; ++it)
                    {
                        (*it)->m_selfTimeHistory.Clear();
                    }
                }

                UpdateInputSystemStatus();
                ret = true;
            }
            else if (channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown ||
                     channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp)
            {
                // are we going up or down?
                int32 off = (channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp) ? -1 : 1;

                //if the collection has been paused ...
                if (m_bCollectionPaused)
                {
                    // ... allow selecting the row ...
                    m_selectedRow += off;
                    m_offset = (m_selectedRow - 30) * ROW_SIZE;

                    m_pGraphProfiler = GetSelectedProfiler();
                }
                else
                {
                    // ... otherwise scroll the whole display
                    m_offset += off * ROW_SIZE;
                }

                // never allow negative offsets
                m_offset = (float)fsel(m_offset, m_offset, 0.0f);
                ret = true;
            }
            else if (channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft ||
                     channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight)
            {
                if (m_bCollectionPaused)
                {
                    // expand the selected profiler
                    m_bDisplayedProfilersValid = false;
                    if (m_pGraphProfiler)
                    {
                        m_pGraphProfiler->m_bExpended = (channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight);
                    }

                    ret = true;
                }
            }
        }
    }

    return ret;
}


//////////////////////////////////////////////////////////////////////////
void CFrameProfileSystem::UpdateInputSystemStatus()
{
    ScopedSwitchToGlobalHeap globalHeap;

    // Disconnect from receiving input events
    AzFramework::InputChannelEventListener::Disconnect();

    // Add ourself if needed
    if (m_bEnabled)
    {
        m_currentInputPriority = m_bCollectionPaused ?
                                 AzFramework::InputChannelEventListener::GetPriorityDebug() :
                                 AzFramework::InputChannelEventListener::GetPriorityUI();
        AzFramework::InputChannelEventListener::Connect();
    }
}


#else

// dummy functions if no frame profiler is used
CFrameProfiler* CFrameProfileSystem::GetProfiler(int index) const {return NULL; }
void CFrameProfileSystem::StartFrame() {}
void CFrameProfileSystem::EndFrame() {}
void CFrameProfileSystem::OnSliceAndSleep() {}
void CFrameProfileSystem::AddPeaksListener(IFrameProfilePeakCallback* pPeakCallback) {}
void CFrameProfileSystem::RemovePeaksListener(IFrameProfilePeakCallback* pPeakCallback) {}
void CFrameProfileSystem::AddFrameProfiler(CFrameProfiler* pProfiler){}
void CFrameProfileSystem::RemoveFrameProfiler(CFrameProfiler* pProfiler){}
void CFrameProfileSystem::Enable(bool bCollect, bool bDisplay) {}
#endif




