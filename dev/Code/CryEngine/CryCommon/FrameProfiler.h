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

#ifndef CRYINCLUDE_CRYCOMMON_FRAMEPROFILER_H
#define CRYINCLUDE_CRYCOMMON_FRAMEPROFILER_H
#pragma once

#include <AzCore/Debug/Profiler.h>

#define PROFILE_HISTORY_COUNT 64

#define SUBSYSTEM_DEFINES \
    X(PROFILE_ANY,          "Any")          \
    X(PROFILE_RENDERER,     "Renderer")     \
    X(PROFILE_3DENGINE,     "3DEngine")     \
    X(PROFILE_PARTICLE,     "Particle")     \
    X(PROFILE_AI,           "AI")           \
    X(PROFILE_ANIMATION,    "Animation")    \
    X(PROFILE_MOVIE,        "Movie")        \
    X(PROFILE_ENTITY,       "Entity")       \
    X(PROFILE_UI,           "UI")           \
    X(PROFILE_NETWORK,      "Network")      \
    X(PROFILE_PHYSICS,      "Physics")      \
    X(PROFILE_SCRIPT,       "Script")       \
    X(PROFILE_SCRIPT_CFUNC, "Script C Functions")  \
    X(PROFILE_AUDIO,        "Audio")        \
    X(PROFILE_EDITOR,       "Editor")       \
    X(PROFILE_SYSTEM,       "System")       \
    X(PROFILE_ACTION,       "Action")       \
    X(PROFILE_GAME,         "Game")         \
    X(PROFILE_INPUT,        "Input")        \
    X(PROFILE_SYNC,         "Sync")         \
    X(PROFILE_NETWORK_TRAFFIC, "Network Traffic") \
    X(PROFILE_DEVICE,       "Device")

#define X(Subsystem, SubsystemName) Subsystem,
enum EProfiledSubsystem
{
    SUBSYSTEM_DEFINES
    PROFILE_LAST_SUBSYSTEM
};
#undef X

inline const char* GetProfiledSubsystemName(EProfiledSubsystem subsystem)
{
#define X(Subsystem, SubsystemName) SubsystemName,
    static const char* ProfiledSubsystemNames[PROFILE_LAST_SUBSYSTEM] =
    {
        SUBSYSTEM_DEFINES
    };
#undef X
    return ProfiledSubsystemNames[subsystem];
}
static_assert(static_cast<AZ::Debug::ProfileCategory>(PROFILE_LAST_SUBSYSTEM) == AZ::Debug::ProfileCategory::LegacyLast, "Mismatched AZ and Legacy profile categories");
#include <platform.h>
#include <ISystem.h> // <> required for Interfuscator
#include <AzCore/Debug/EventTrace.h>

class CFrameProfiler;
class CFrameProfilerSection;
class CCustomProfilerSection;

//////////////////////////////////////////////////////////////////////////
/*! This callback will be called for Profiling Peaks.
 */
struct IFrameProfilePeakCallback
{
    virtual ~IFrameProfilePeakCallback(){}
    //! Called when peak is detected for this profiler.
    //! @param fPeakTime peak time in milliseconds.
    virtual void OnFrameProfilerPeak(CFrameProfiler* pProfiler, float fPeakTime) = 0;
};

struct SPeakRecord
{
    CFrameProfiler* pProfiler;
    float peakValue;
    float avarageValue;
    float variance;
    int pageFaults; // Number of page faults at this frame.
    int count;  // Number of times called for peak.
    float when; // when it added.
};

//////////////////////////////////////////////////////////////////////////
//! IFrameProfileSystem interface.
//! the system which does the gathering of stats
struct IFrameProfileSystem
{
    virtual ~IFrameProfileSystem(){}
    enum EDisplayQuantity
    {
        SELF_TIME,
        TOTAL_TIME,
        SELF_TIME_EXTENDED,
        TOTAL_TIME_EXTENDED,
        PEAK_TIME,
        SUBSYSTEM_INFO,
        COUNT_INFO,
        STANDARD_DEVIATION,
        ALLOCATED_MEMORY,
        ALLOCATED_MEMORY_BYTES,
        STALL_TIME,
        PEAKS_ONLY,
    };

    //! Reset all profiling data.
    virtual void Reset() = 0;
    //! Add new frame profiler.
    virtual void AddFrameProfiler(CFrameProfiler* pProfiler) = 0;
    //! Remove existing frame profiler.
    virtual void RemoveFrameProfiler(CFrameProfiler* pProfiler) = 0;
    //! Must be called at the start of the frame.
    virtual void StartFrame() = 0;
    //! Must be called at the end of the frame.
    virtual void EndFrame() = 0;
    //! Must be called when something quacks like the end of the frame.
    virtual void OnSliceAndSleep() = 0;

    //! Get number of registered frame profilers.
    virtual int GetProfilerCount() const = 0;
    //! Get frame profiler at specified index.
    //! @param index must be 0 <= index < GetProfileCount()
    virtual CFrameProfiler* GetProfiler(int index) const = 0;
    //! Get number of registered peak records.
    virtual int GetPeaksCount() const = 0;
    //! Get peak record at specified index.
    //! @param index must be 0 <= index < GetPeaksCount()
    virtual const SPeakRecord* GetPeak(int index) const = 0;
    //! Gets the bottom active section.
    virtual CFrameProfilerSection const* GetCurrentProfilerSection() = 0;

    //! get internal time lost to profiling
    virtual float GetLostFrameTimeMS() const = 0;

    virtual void Enable(bool bCollect, bool bDisplay) = 0;
    virtual void SetSubsystemFilter(bool bFilterSubsystem, EProfiledSubsystem subsystem) = 0;
    //! True if profiler is turned off (even if collection is paused).
    virtual bool IsEnabled() const = 0;
    //! True if profiler statistics is visible
    virtual bool IsVisible() const = 0;
    //! True if profiler must collect profiling data.
    virtual bool IsProfiling() const = 0;
    virtual void SetDisplayQuantity(EDisplayQuantity quantity) = 0;

    // For custom frame profilers.
    virtual void StartCustomSection(CCustomProfilerSection* pSection) = 0;
    virtual void EndCustomSection(CCustomProfilerSection* pSection) = 0;

    //! Register peak listener callback to be called when peak value is greater then this.
    virtual void AddPeaksListener(IFrameProfilePeakCallback* pPeakCallback) = 0;
    virtual void RemovePeaksListener(IFrameProfilePeakCallback* pPeakCallback) = 0;

    //! access to call stack string
    virtual const char* GetFullName(CFrameProfiler* pProfiler) = 0;
    virtual const char* GetModuleName(CFrameProfiler* pProfiler) = 0;
};


//////////////////////////////////////////////////////////////////////////
//! CFrameProfilerSamplesHistory provides information on history of sample values
//! for profiler counters.
//////////////////////////////////////////////////////////////////////////
template <class T, int TCount>
class CFrameProfilerSamplesHistory
{
public:
    CFrameProfilerSamplesHistory()
        :    m_nHistoryNext (0)
        , m_nHistoryCount(0) {}

    //! Add a new sample to history.
    void Add(T sample)
    {
        m_history[m_nHistoryNext] = sample;
        m_nHistoryNext = (m_nHistoryNext + TCount - 1) % TCount;
        if (m_nHistoryCount < TCount)
        {
            ++m_nHistoryCount;
        }
    }
    //! cleans up the data history
    void Clear()
    {
        m_nHistoryNext = 0;
        m_nHistoryCount = 0;
    }
    //! Get last sample value.
    T GetLast()
    {
        if (m_nHistoryCount)
        {
            return m_history[(m_nHistoryNext + 1) % TCount];
        }
        else
        {
            return 0;
        }
    }
    //! calculates average sample value for at most the given number of frames (less if so many unavailable)
    T GetAverage(int nCount = TCount)
    {
        if (m_nHistoryCount)
        {
            T fSum = 0;
            if (nCount > m_nHistoryCount)
            {
                nCount = m_nHistoryCount;
            }
            for (int i = 1; i <= nCount; ++i)
            {
                fSum += m_history[(m_nHistoryNext + i) % (sizeof(m_history) / sizeof((m_history)[0]))];
            }
            return fSum / nCount;
        }
        else
        {
            return 0;
        }
    }
    //! calculates average sample value for at most the given number of frames (less if so many unavailable),
    //! multiplied by the Poisson function
    T GetAveragePoisson(int nCount, float fMultiplier)
    {
        if (m_nHistoryCount)
        {
            float fSum = 0, fCurrMult = 1, fSumWeight = 0;
            if (nCount > m_nHistoryCount)
            {
                nCount = m_nHistoryCount;
            }
            for (int i = 1; i <= nCount; ++i)
            {
                fSum += m_history[(m_nHistoryNext + i) % TCount] * fCurrMult;
                fSumWeight += fCurrMult;
                fCurrMult *= fMultiplier;
            }
            return (T)(fSum / fSumWeight);
        }
        else
        {
            return 0;
        }
    }
    //! calculates Standard deviation of values.
    //! stdev = Sqrt( Sum((X-Xave)^2)/(n-1) )
    T GetStdDeviation(int nCount = TCount)
    {
        if (m_nHistoryCount > 1)    // standard deviation is undefined for only one element
        {
            T fAve = GetAverage(nCount);
            T fVal = 0;
            T fSumVariance = 0;
            if (nCount > m_nHistoryCount)
            {
                nCount = m_nHistoryCount;
            }
            for (int i = 1; i <= nCount; ++i)
            {
                fVal = m_history[(m_nHistoryNext + i) % (sizeof(m_history) / sizeof((m_history)[0]))];
                fSumVariance = (fVal - fAve) * (fVal - fAve); // (X-Xave)^2
            }
            return sqrtf(fSumVariance / (nCount - 1));
        }
        else
        {
            return 0;
        }
    }
    //! calculates max sample value for at most the given number of frames (less if so many unavailable)
    T GetMax(int nCount = TCount)
    {
        if (m_nHistoryCount)
        {
            T fMax = 0;
            if (nCount > m_nHistoryCount)
            {
                nCount = m_nHistoryCount;
            }
            for (int i = 1; i <= nCount; ++i)
            {
                T fCur = m_history[(m_nHistoryNext + i) % (sizeof(m_history) / sizeof((m_history)[0]))];
                if (i == 1 || fCur > fMax)
                {
                    fMax = fCur;
                }
            }
            return fMax;
        }
        else
        {
            return 0;
        }
    }
    //! calculates min sample value for at most the given number of frames (less if so many unavailable)
    T GetMin(int nCount = TCount)
    {
        if (m_nHistoryCount)
        {
            T fMin = 0;
            if (nCount > m_nHistoryCount)
            {
                nCount = m_nHistoryCount;
            }
            for (int i = 1; i <= nCount; ++i)
            {
                T fCur = m_history[(m_nHistoryNext + i) % (sizeof(m_history) / sizeof((m_history)[0]))];
                if (i == 1 || fCur < fMin)
                {
                    fMin = fCur;
                }
            }
            return fMin;
        }
        else
        {
            return 0;
        }
    }
protected:
    // the timer values for the last frames
    T m_history[TCount];
    // the current pointer in the timer history, decreases
    int m_nHistoryNext;
    // the currently collected samples in the timer history
    int m_nHistoryCount;
    // adds the entry to the timer history (current timer value)
};

//////////////////////////////////////////////////////////////////////////
class CFrameProfilerGraph
{
public:
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    std::vector<uint8> m_data;
};

//////////////////////////////////////////////////////////////////////////
class CFrameProfilerOfflineHistory
{
public:
    //! Self time in microseconds.
    std::vector<uint32> m_selfTime;
    //! Number of calls.
    std::vector<uint16> m_count;
};

//////////////////////////////////////////////////////////////////////////
//! CFrameProfiler is a single profiler counter with unique name and data.
//! Multiple Sections can be executed for this profiler, they all will be merged in this class.
//! CFrameProfileSection must reference pointer to instance of this counter, to collect the sampling data.
//!
class CFrameProfiler
{
public:
    ISystem* m_pISystem;
    const char* m_name;

    //! Total time spent in this counter including time of child profilers in current frame.
    int64 m_totalTime;
    //! Self frame time spent only in this counter (But includes recursive calls to same counter) in current frame.
    int64 m_selfTime;
    //! Latest frame ID
    uint64 m_latestFrame;
    //! Displayed quantity (interpolated or avarage).
    float m_displayedValue;
    //! How many times this profiler counter was executed.
    int m_count;
    //! How variant this value.
    float m_variance;
    //! peak from this frame (useful if count is >1).
    int64 m_peak;

    //! Current parent profiler in last frame.
    CFrameProfiler* m_pParent;
    //! Expended or collapsed displaying state.
    uint8 m_bExpended;
    uint8 m_bHaveChildren;
    uint8 m_subsystem;
    uint8 m_isStallProfiler;

    const char* m_stallCause;//additional information of stall cause

    //! Thread Id of this instance.
    threadID m_threadId;
    //! Linked list to other thread instances.
    CFrameProfiler* m_pNextThread;

    CFrameProfilerSamplesHistory<float, PROFILE_HISTORY_COUNT> m_totalTimeHistory;
    CFrameProfilerSamplesHistory<float, PROFILE_HISTORY_COUNT> m_selfTimeHistory;
    CFrameProfilerSamplesHistory<int, PROFILE_HISTORY_COUNT> m_countHistory;

    //! Graph data for this frame profiler.

    //! Graph associated with this profiler.
    CFrameProfilerGraph* m_pGraph;
    CFrameProfilerOfflineHistory* m_pOfflineHistory;

    bool const m_bAlwaysCollect;

    bool m_addedSelf = false;

    CFrameProfiler(ISystem* pSystem, const char* sCollectorName, EProfiledSubsystem subsystem = PROFILE_ANY, bool const bAlwaysCollect = false)
        :   m_pISystem(pSystem)
        ,   m_name(sCollectorName)
        ,   m_totalTime(0)
        ,   m_selfTime(0)
        ,   m_latestFrame(0)
        ,   m_displayedValue(0.0f)
        ,   m_count(0)
        ,   m_variance(0.0f)
        ,   m_peak(0)
        ,   m_pParent(NULL)
        ,   m_bExpended(0)
        ,   m_bHaveChildren(0)
        ,   m_subsystem((uint8)subsystem)
        ,   m_isStallProfiler(0)
        ,   m_stallCause(NULL)
        ,   m_threadId(0)
        ,   m_pNextThread(NULL)
        ,   m_pGraph(NULL)
        ,   m_pOfflineHistory(NULL)
        ,   m_bAlwaysCollect(bAlwaysCollect)
    {
        if (m_pISystem)
        {
            if (IFrameProfileSystem* const pFrameProfileSystem = m_pISystem->GetIProfileSystem())
            {
                pFrameProfileSystem->AddFrameProfiler(this);
                m_addedSelf = true;
            }
        }
    }

    ~CFrameProfiler()
    {
        // This is needed for when modules get unloaded at runtime.
        // this might happen very late (atexit) so it needs to only happen if we actually added ourself
        // at this point "m_pISystem" might actually be invalid.
        if (m_addedSelf && m_pISystem)
        {
            if (IFrameProfileSystem* const pFrameProfileSystem = m_pISystem->GetIProfileSystem())
            {
                pFrameProfileSystem->RemoveFrameProfiler(this);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////
//! CFrameProfilerSection is an auto class placed where code block need to be profiled.
//! Every time this object is constructed and destructed the time between constructor
//! and destructor is merged into the referenced CFrameProfiler instance.
//! For consoles, a faster non LHS triggering way is used
//! (but potentially problematic with many threads when profiling is enabled at a very
//! 'bad' time)
//!
class CFrameProfilerSection
{
public:
    int64 m_startTime;
    int64 m_excludeTime;
    CFrameProfiler* m_pFrameProfiler;
    CFrameProfilerSection* m_pParent;

    ILINE CFrameProfilerSection(CFrameProfiler* profiler)
    {
        if (gEnv && ((gEnv->bProfilerEnabled || profiler->m_bAlwaysCollect) && (gEnv->callbackStartSection)))
        {
            m_pFrameProfiler = profiler;
            gEnv->callbackStartSection(this);
        }
        else
        {
            m_pFrameProfiler = nullptr;
        }
    }
    ILINE ~CFrameProfilerSection()
    {
        if (m_pFrameProfiler)
        {
            gEnv->callbackEndSection(this);
        }
    }
};


//////////////////////////////////////////////////////////////////////////
//! CCustomProfilerSection is an auto class placed where any custom data need to be profiled.
//! Works similarly to CFrameProfilerSection, but records any custom data, instead of elapsed time.
//!
class CCustomProfilerSection
{
public:
    int* m_pValue;
    int m_excludeValue;
    CFrameProfiler* m_pFrameProfiler;
    CCustomProfilerSection* m_pParent;

    //! pValue pointer must remain valid until after calling destructor of this custom profiler section.
    ILINE CCustomProfilerSection(CFrameProfiler* profiler, int* pValue)
    {
        m_pValue = pValue;
        m_pFrameProfiler = profiler;
        if (profiler)
        {
            m_pFrameProfiler->m_pISystem->GetIProfileSystem()->StartCustomSection(this);
        }
    }
    ILINE ~CCustomProfilerSection()
    {
        if (m_pFrameProfiler)
        {
            m_pFrameProfiler->m_pISystem->GetIProfileSystem()->EndCustomSection(this);
        }
    }
} _ALIGN(32);

//USE_FRAME_PROFILER
#if defined(USE_FRAME_PROFILER)

//////////////////////////////////////////////////////////////////////////
//! Place this macro when you need to profile a function.
//!
//! void CTest::Func() {
//!   FUNCTION_PROFILER( GetISystem() );
//!   // function body will be profiled.
//! }
#define FUNCTION_PROFILER_LEGACYONLY(pISystem, subsystem)                                                               \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, __FUNC__, subsystem); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__));

#define FUNCTION_PROFILER(pISystem, subsystem)                                                               \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, __FUNC__, subsystem); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__)); \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

#define FUNCTION_PROFILER_FAST(pISystem, subsystem, bProfileEnabled)                                         \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, __FUNC__, subsystem); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__)); \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

#define FUNCTION_PROFILER_ALWAYS(pISystem, subsystem)                                                              \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, __FUNC__, subsystem, true); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__)); \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

//////////////////////////////////////////////////////////////////////////
//! Place this macro when you need to profile any code block.
//! {
//!     ... some code ...
//!   {
//!         FRAME_PROFILER( GetISystem(),"MyCode" );
//!     ... more code ... // This code will be profiled with counter named "MyCode"
//!     }
//! }
#define FRAME_PROFILER_LEGACYONLY(szProfilerName, pISystem, subsystem)                                                        \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, szProfilerName, subsystem); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__));

#define FRAME_PROFILER(szProfilerName, pISystem, subsystem)                                                        \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, szProfilerName, subsystem); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__)); \
    AZ_PROFILE_SCOPE(static_cast<AZ::Debug::ProfileCategory>(subsystem), szProfilerName);

//! Older version of FRAME_PROFILE macro
#define FRAME_PROFILER_FAST(szProfilerName, pISystem, subsystem, bProfileEnabled)                                  \
    PREFAST_SUPPRESS_WARNING(6246) static CFrameProfiler AZ_JOIN(staticFrameProfiler, __LINE__)(pISystem, szProfilerName, subsystem); \
    CFrameProfilerSection AZ_JOIN(frameProfilerSection, __LINE__)(&AZ_JOIN(staticFrameProfiler, __LINE__)); \
    AZ_PROFILE_SCOPE(static_cast<AZ::Debug::ProfileCategory>(subsystem), szProfilerName);

#else //#if !defined(USE_FRAME_PROFILER)

#define FUNCTION_PROFILER_LEGACYONLY(pISystem, subsystem)
#define FUNCTION_PROFILER(pISystem, subsystem) \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));
#define FUNCTION_PROFILER_FAST(pISystem, subsystem, bProfileEnabled) \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));
#define FUNCTION_PROFILER_ALWAYS(pISystem, subsystem) \
    AZ_PROFILE_FUNCTION(static_cast<AZ::Debug::ProfileCategory>(subsystem));

#define FRAME_PROFILER_LEGACYONLY(szProfilerName, pISystem, subsystem)
#define FRAME_PROFILER(szProfilerName, pISystem, subsystem) \
    AZ_PROFILE_SCOPE(static_cast<AZ::Debug::ProfileCategory>(subsystem), szProfilerName);
#define FRAME_PROFILER_FAST(szProfilerName, pISystem, subsystem, bProfileEnabled) \
    AZ_PROFILE_SCOPE(static_cast<AZ::Debug::ProfileCategory>(subsystem), szProfilerName);

#endif //USE_FRAME_PROFILER

#define FUNCTION_PROFILER_SYS(subsystem) \
    FUNCTION_PROFILER_FAST(gEnv->pSystem, PROFILE_##subsystem, m_bProfilerEnabled)


    #define STALL_PROFILER(cause)

#endif // CRYINCLUDE_CRYCOMMON_FRAMEPROFILER_H
