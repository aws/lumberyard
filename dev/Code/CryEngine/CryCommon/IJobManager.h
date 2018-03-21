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

// Description : JobManager interface


#ifndef CRYINCLUDE_CRYCOMMON_IJOBMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IJOBMANAGER_H
#pragma once

#include <ITimer.h>
#include <CryArray.h>
#include <CryThread.h>
#include <ISystem.h>

#include <AzCore/Debug/Profiler.h>

// ==============================================================================
// Job manager settings
// ==============================================================================
//enable to obtain stats of spu usage each frame
#define JOBMANAGER_SUPPORT_FRAMEPROFILER

// collect per job informations about dispatch, start, stop and sync times
#if !defined(DEDICATED_SERVER)
#define JOBMANAGER_SUPPORT_PROFILING
#endif


// disable features which cost performance
#if !defined(USE_FRAME_PROFILER) || defined(MOBILE)
#undef JOBMANAGER_SUPPORT_FRAMEPROFILER
#undef JOBMANAGER_SUPPORT_PROFILING
#endif

// Use of 128-bit atomic operations in the JobManager is unsupported on ARM processors: https://issues.labcollab.net/browse/LMBR-1674
// All job manager threads use _InterlockedCompareExchange128 every frame to check for new jobs, and all attempts to make this atomic
// for ARM processors have thus far failed, so we must disable the job system. Long term we hope to move all existing jobs to use the
// AZ::JobManager system, the default implementation of which does not rely on the cmpxchg128 instruction that is not provided by arm
// processors (although the faster job-stealing implementation does require both a cmpxchg128 instruction and thread local storage).
#if defined(INTERLOCKED_COMPARE_EXCHANGE_128_NOT_SUPPORTED)
#define JOBMANAGER_DISABLED 1
#else
#define JOBMANAGER_DISABLED 0
#endif

// ==============================================================================
// forward declarations
// ==============================================================================
struct ILog;


// implementation of mutex/condition variable
// used in the job manager for yield waiting in corner cases
// like waiting for a job to finish/jobqueue full and so on
class SJobFinishedConditionVariable
{
public:
    SJobFinishedConditionVariable()
        : m_nRefCounter(0)
        , m_pOwner(NULL)
    {
        m_nFinished = 1;
    }

    void Acquire()
    {
        //wait for completion of the condition
        m_Notify.Lock();
        while (m_nFinished == 0)
        {
            m_CondNotify.Wait(m_Notify);
        }
        m_Notify.Unlock();
    }

    void Release()
    {
        m_Notify.Lock();
        m_nFinished = 1;
        m_CondNotify.Notify();
        m_Notify.Unlock();
    }

    void SetRunning()
    {
        m_nFinished = 0;
    }

    bool IsRunning()
    {
        return m_nFinished == 0;
    }

    void SetStopped()
    {
        m_nFinished = 1;
    }

    void SetOwner(volatile const void* pOwner)
    {
        if (m_pOwner != NULL)
        {
            __debugbreak();
        }
        m_pOwner = pOwner;
    }

    void ClearOwner(volatile const void* pOwner)
    {
        if (m_pOwner != pOwner)
        {
            __debugbreak();
        }

        m_pOwner = NULL;
    }

    bool AddRef(volatile const void* pOwner)
    {
        if (m_pOwner != pOwner)
        {
            return false;
        }

        ++m_nRefCounter;
        return true;
    }
    uint32 DecRef(volatile const void* pOwner)
    {
        if (m_pOwner != pOwner)
        {
            __debugbreak();
        }

        return --m_nRefCounter;
    }

    bool HasOwner() const { return m_pOwner != NULL; }

private:
    CryMutex                            m_Notify;
    CryConditionVariable    m_CondNotify;
    volatile uint32             m_nFinished;
    volatile uint32             m_nRefCounter;
    volatile const void*    m_pOwner;
};


///////////////////////////////////////////////////////////////////////////////
//forward declarations
namespace JobManager {
    class CJobBase;
}
namespace JobManager {
    class CJobDelegator;
}
namespace JobManager {
    struct SProdConsQueueBase;
}
namespace JobManager {
    namespace detail {
        struct SJobQueueSlotState;
    }
}


///////////////////////////////////////////////////////////////////////////////
// Wrapper for Vector4 Uint
_MS_ALIGN(16) struct SVEC4_UINT
{
#if defined(ANDROID) || defined(IOS) || defined(APPLETV)
    uint32 v[4];
#else
    __m128 v;
#endif
} _ALIGN(16);


// ==============================================================================
// global values
// ==============================================================================
// front end values
namespace JobManager
{
    /////////////////////////////////////////////////////////////////////////////
    // priority settings used for jobs
    enum TPriorityLevel
    {
        eHighPriority           = 0,
        eRegularPriority    = 1,
        eLowPriority            = 2,
        eStreamPriority     = 3,
        eNumPriorityLevel = 4
    };

    /////////////////////////////////////////////////////////////////////////////
    // Stores worker utilization
    // One instance per Worker
    // One instance per cache line to avoid thread synchronization issues
    _MS_ALIGN(128) struct SWorkerStats
    {
        unsigned int nExecutionPeriod;  // Accumulated job execution period in micro seconds
        unsigned int nNumJobsExecuted;  // Number of jobs executed
    } _ALIGN(128);


    /////////////////////////////////////////////////////////////////////////////
    // type to reprensent a semaphore handle of the jobmanager
    typedef uint16 TSemaphoreHandle;

    /////////////////////////////////////////////////////////////////////////////
    // magic value to reprensent an invalid job handle
    enum
    {
        INVALID_JOB_HANDLE = ((unsigned int)-1)
    };

    /////////////////////////////////////////////////////////////////////////////
    // BackEnd Type
    enum EBackEndType
    {
        eBET_Thread,
        eBET_Fallback,
        eBET_Blocking
    };

    namespace Fiber
    {
        // The alignment of the fibertask stack (currently set to 128 kb)
        enum
        {
            FIBERTASK_ALIGNMENT = 128U << 10
        };
        // The number of switches the fibertask records (default: 32)
        enum
        {
            FIBERTASK_RECORD_SWITCHES = 32U
        };
    } // namespace JobManager::Fiber
} // namespace JobManager


// ==============================================================================
// Used structures of the JobManager
// ==============================================================================
namespace JobManager
{
    struct SJobStringHandle
    {
        const char* cpString;           //points into repository, must be first element
        unsigned int strLen;            //string length
        int jobId;                              //index (also acts as id) of job
        uint32 nJobInvokerIdx;      //index of the jobInvoker (used to job switching in the prod/con queue)

        inline bool operator==(const SJobStringHandle& crOther) const;
        inline bool operator<(const SJobStringHandle& crOther) const;
    };

    //handle retrieved by name for job invocation
    typedef SJobStringHandle* TJobHandle;

    bool SJobStringHandle::operator==(const SJobStringHandle& crOther) const
    {
        return strcmp(cpString, crOther.cpString) == 0;
    }

    bool SJobStringHandle::operator<(const SJobStringHandle& crOther) const
    {
        return strcmp(cpString, crOther.cpString) < 0;
    }

    // struct to collect profiling informations about job invocations and sync times
    struct SJobProfilingData
    {
        typedef CTimeValue TimeValueT;

        TimeValueT nStartTime;
        TimeValueT nEndTime;

        TimeValueT nWaitBegin;
        TimeValueT nWaitEnd;

        TJobHandle jobHandle;
        threadID nThreadId;
        uint32 nWorkerThread;
    } _ALIGN(16);

    struct SJobProfilingDataContainer
    {
        enum
        {
            nCapturedFrames = 4
        };
        enum
        {
            nCapturedEntriesPerFrame = 4096
        };
        uint32 nFrameIdx;
        uint32 nProfilingDataCounter[nCapturedFrames];

        ILINE uint32 GetFillFrameIdx() { return nFrameIdx % nCapturedFrames; }
        ILINE uint32 GetPrevFrameIdx() { return (nFrameIdx - 1) % nCapturedFrames; }
        ILINE uint32 GetRenderFrameIdx() { return (nFrameIdx - 2) % nCapturedFrames; }
        ILINE uint32 GetPrevRenderFrameIdx() { return (nFrameIdx - 3) % nCapturedFrames; }

        SJobProfilingData m_DymmyProfilingData _ALIGN(128);
        SJobProfilingData arrJobProfilingData[nCapturedFrames][nCapturedEntriesPerFrame];
    } _ALIGN(128);

    // special combination of a volatile spinning variable combined with a semaphore
    // which is used if the running state is not yet set to finish during waiting
    struct SJobSyncVariable
    {
        SJobSyncVariable();

        bool IsRunning() const volatile;

        // interface, should only be used by the job manager or the job state classes
        void Wait() volatile;
        void SetRunning() volatile;
        bool SetStopped() volatile;

    private:
        friend class CJobManager;

        // union used to combine the semaphore and the running state in a single word
        union SyncVar
        {
            volatile uint32 wordValue;
            struct
            {
                uint16 nRunningCounter;
                TSemaphoreHandle semaphoreHandle;
            };
        };

        // members
        SyncVar syncVar;        // sync-variable which contain the running state or the used semaphore
#if defined(PLATFORM_64BIT)
        char padding[4];
#endif
    };


    //condition variable like struct to be used for polling if a job has been finished
    struct SJobStateBase
    {
    public:
        ILINE bool IsRunning() const        {   return syncVar.IsRunning(); }
        ILINE void SetRunning()
        {
            syncVar.SetRunning();
        }
        ILINE bool SetStopped()
        {
            return syncVar.SetStopped();
        }
        ILINE void Wait()
        {
            syncVar.Wait();
        }

    private:
        friend class CJobManager;

        SJobSyncVariable syncVar;
    };

    // for speed use 16 byte aligned job state
    struct SJobState
        : SJobStateBase
    {
        // when doing profiling, intercept the SetRunning() and SetStopped() functions for profiling informations

        ILINE SJobState()
            : m_pFollowUpJob(NULL)
#if defined(JOBMANAGER_SUPPORT_PROFILING)
            , nProfilerIndex(~0)
#endif
        {}

        ILINE void SetRunning();
        void SetStopped();

        ILINE void RegisterPostJob(CJobBase* pFollowUpJob) { m_pFollowUpJob = pFollowUpJob; }

#if defined(JOBMANAGER_SUPPORT_PROFILING)
        uint16 nProfilerIndex;
#endif

        CJobBase* m_pFollowUpJob;
    } _ALIGN(16);

    // Stores worker utilization stats for a frame
    class CWorkerFrameStats
    {
    public:
        struct SWorkerStats
        {
            float  nUtilPerc;                       // Utilization percentage this frame [0.f..100.f]
            uint32 nExecutionPeriod;        // Total execution time on this worker in usec
            uint32 nNumJobsExecuted;        // Number of jobs executed contributing to nExecutionPeriod
        };

    public:
        ILINE CWorkerFrameStats(uint8 workerNum)
            : numWorkers(workerNum)
        {
            workerStats = new SWorkerStats[workerNum];
            Reset();
        }

        ILINE ~CWorkerFrameStats()
        {
            delete[] workerStats;
        }

        ILINE void Reset()
        {
            for (int i = 0; i < numWorkers; ++i)
            {
                workerStats[i].nUtilPerc = 0.f;
                workerStats[i].nExecutionPeriod = 0;
                workerStats[i].nNumJobsExecuted = 0;
            }

            nSamplePeriod = 0;
        }

    public:
        uint8                   numWorkers;
        uint32              nSamplePeriod;
        SWorkerStats* workerStats;
    };

    struct SWorkerFrameStatsSummary
    {
        uint32 nSamplePeriod;                     // Worker sample period
        uint32 nTotalExecutionPeriod;       // Total execution time on this worker in usec
        uint32 nTotalNumJobsExecuted;       // Total number of jobs executed contributing to nExecutionPeriod
        float  nAvgUtilPerc;                        // Average worker utilization [0.f ... 100.f]
        uint8    nNumActiveWorkers;             // Active number of workers for this frame
    };

    //per frame and job specific
    struct SJobFrameStats
    {
        unsigned int usec;              // last frame's job time in microseconds
        unsigned int count;             // number of calls this frame
        const char* cpName;             // job name

        unsigned int usecLast;      // last but one frames Job time in microseconds (written and accumulated by Thread)

        inline SJobFrameStats();
        inline SJobFrameStats(const char* cpJobName);

        inline void Reset();
        inline void operator=(const SJobFrameStats& crFrom);
        inline bool operator<(const SJobFrameStats& crOther) const;

        static bool sort_lexical (const JobManager::SJobFrameStats& i, const JobManager::SJobFrameStats& j)
        {
            return strcmp(i.cpName, j.cpName) < 0;
        }

        static bool sort_time_high_to_low (const JobManager::SJobFrameStats& i, const JobManager::SJobFrameStats& j)
        {
            return i.usec > j.usec;
        }

        static bool sort_time_low_to_high (const JobManager::SJobFrameStats& i, const JobManager::SJobFrameStats& j)
        {
            return i.usec < j.usec;
        }
    } _ALIGN(16);

    _MS_ALIGN(16) struct SJobFrameStatsSummary
    {
        unsigned int nTotalExecutionTime;                   // Total execution time of all jobs in microseconds
        unsigned int nNumJobsExecuted;                      // Total Number of job executions this frame
        unsigned int nNumIndividualJobsExecuted;    // Total Number of individual jobs
    } _ALIGN(16);

    SJobFrameStats::SJobFrameStats(const char* cpJobName)
        : usec(0)
        , count(0)
        , cpName(cpJobName)
        , usecLast(0)
    {}

    SJobFrameStats::SJobFrameStats()
        : cpName("Uninitialized")
        , usec(0)
        , count(0)
        , usecLast(0)
    {}

    void SJobFrameStats::operator=(const SJobFrameStats& crFrom)
    {
        usec = crFrom.usec;
        count = crFrom.count;
        cpName = crFrom.cpName;
        usecLast = crFrom.usecLast;
    }

    void SJobFrameStats::Reset()
    {
        usecLast = usec;
        usec = count = 0;
    }

    bool SJobFrameStats::operator<(const SJobFrameStats& crOther) const
    {
        //sort from large to small
        return usec > crOther.usec;
    }

    // struct to represent a packet for the consumer producer queue
    _MS_ALIGN(16) struct SAddPacketData
    {
        JobManager::SJobState*  pJobState;
        unsigned short profilerIndex;
        unsigned char nInvokerIndex; // to keep the struct 16 byte big, we use a index into the info block
    } _ALIGN(16);

    // delegator function, takes a pointer to a params structure
    // and does the decomposition of the parameters, then calls the
    // job entry function
    typedef void (* Invoker)(void*);

    //info block transferred first for each job
    ///we have to transfer the info block, parameter block, input memory needed for execution
    _MS_ALIGN(128) struct SInfoBlock
    {
        inline void* operator new(std::size_t)
        {
            size_t allocated = 0;
            return CryMalloc(sizeof(SInfoBlock), allocated, 128);
        }
        inline void operator delete(void* p)
        {
            CryFree(p, 128);
        }

        // struct to represent the state of a SInfoBlock
        struct SInfoBlockState
        {
            // union of requiered members and uin32 to allow easy atomic operations
            union
            {
                struct
                {
                    TSemaphoreHandle    nSemaphoreHandle;           // handle for the conditon variable to use in case some thread is waiting
                    uint16                      nRoundID;                           // number of rounds over the job queue, to prevent a race condition with SInfoBlock reuse (we have 15 bits, thus 32k full job queue iterations are safe)
                };
                volatile LONG nValue;                                           // value for easy Compare and Swap
            };

            void IsInUse(uint32 nRoundIdToCheckOrg, bool& rWait, bool& rRetry, uint32 nMaxValue) volatile const
            {
                uint32 nRoundIdToCheck = nRoundIdToCheckOrg & 0x7FFF; // clamp nRoundID to 15 bit
                uint32 nLoadedRoundID = nRoundID;
                uint32 nNextLoadedRoundID = ((nLoadedRoundID + 1) & 0x7FFF);
                nNextLoadedRoundID = nNextLoadedRoundID >= nMaxValue ? 0 : nNextLoadedRoundID;

                if (nLoadedRoundID == nRoundIdToCheck) // job slot free
                {
                    rWait = false;
                    rRetry = false;
                }
                else if (nNextLoadedRoundID == nRoundIdToCheck)   // job slot need to be worked on by worker
                {
                    rWait = true;
                    rRetry = false;
                }
                else // producer was suspended long enough that the worker overtook it
                {
                    rWait = false;
                    rRetry = true;
                }
            }
        };

        // data needed to run the job and it's functionality
        volatile SInfoBlockState                jobState;                           // state of the SInfoBlock in job queue, should only be modified by access functions
        Invoker                                                 jobInvoker;                     // callback function to job invoker (extracts parameters and calls entry function)
        union                                                                                                   //external job state address /shared with address of prod-cons queue
        {
            JobManager::SJobState*                  pJobState;
            JobManager::SProdConsQueueBase* pQueue;
        };
        JobManager::SInfoBlock*                 pNext;                              // single linked list for fallback jobs in the case the queue is full, and a worker wants to push new work

        // per job settings like cache size and so on
        unsigned char frameProfIndex;                           // index of SJobFrameStats*
        unsigned char nflags;
        unsigned char paramSize;                                    // size in total of parameter block in 16 byte units
        unsigned char jobId;                                            // corresponding job ID, needs to track jobs

        // could also use a union, but this solution is hopefully more clear
#if defined(JOBMANAGER_SUPPORT_PROFILING)
        unsigned short profilerIndex;                           // index for the job system profiler
#endif

        // bits used for nflags
        static const unsigned int scHasQueue                            = 0x4;

        // size of the SInfoBlock struct and how much memory we have to store parameters
#if defined(PLATFORM_64BIT)
        static const unsigned int scSizeOfSJobQueueEntry            = 512;
        static const unsigned int scSizeOfJobQueueEntryHeader   = 64; // Please adjust when adding/removing members, keep as a multiple of 16
        static const unsigned int scAvailParamSize                      = scSizeOfSJobQueueEntry - scSizeOfJobQueueEntryHeader;
#else
        static const unsigned int scSizeOfSJobQueueEntry            = 384;
        static const unsigned int scSizeOfJobQueueEntryHeader   = 32; // Please adjust when adding/removing members, keep as a multiple of 16
        static const unsigned int scAvailParamSize                      = scSizeOfSJobQueueEntry - scSizeOfJobQueueEntryHeader;
#endif


        //parameter data are enclosed within to save a cache miss
        _MS_ALIGN(16) unsigned char paramData[scAvailParamSize] _ALIGN(16); //is 16 byte aligned, make sure it is kept aligned

        ILINE void AssignMembersTo(SInfoBlock* const pDest) const
        {
            SVEC4_UINT* const __restrict pDst = (SVEC4_UINT* const __restrict)(pDest);
            SVEC4_UINT* const __restrict pSrc = (SVEC4_UINT* const __restrict)(this);

            pDest->jobInvoker = jobInvoker;
            pDest->pJobState = pJobState;
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];

#if defined(PLATFORM_64BIT) // copy more data, since the block itself is bigger
            pDst[3] = pSrc[3];
            pDst[4] = pSrc[4];
#endif
        }

        ILINE bool HasQueue() const
        {
            return (nflags & (unsigned char)scHasQueue) != 0;
        }

        ILINE void SetJobState(JobManager::SJobState* _pJobState)
        {
            assert(!HasQueue());
            pJobState = _pJobState;
        }

        ILINE JobManager::SJobState* GetJobState() const
        {
            assert(!HasQueue());
            return pJobState;
        }

        ILINE JobManager::SProdConsQueueBase* GetQueue() const
        {
            assert(HasQueue());
            return pQueue;
        }

        ILINE unsigned char* const __restrict GetParamAddress()
        {
            assert(!HasQueue());
            return &paramData[0];
        }

        // functions to access jobstate of SInfoBlock
        ILINE void IsInUse(uint32 nRoundId, bool& rWait, bool& rRetry, uint32 nMaxValue) const { return jobState.IsInUse(nRoundId, rWait, rRetry, nMaxValue); }

        void Release(uint32 nMaxValue);
        void Wait(uint32 nRoundID, uint32 nMaxValue);
    } _ALIGN(128);


    // state information for the fill and empty pointers of the job queue
    struct SJobQueuePos
    {
        JobManager::SInfoBlock*                 jobQueue[JobManager::eNumPriorityLevel];                                    // base of job queue per priority level
        JobManager::detail::SJobQueueSlotState* jobQueueStates[JobManager::eNumPriorityLevel];      // base of job queue per priority level
        volatile uint64                                 index;                                                                                                      // index use for job slot publishing each priority level is encoded in this single value

        // util function to increase counter for their priority level only
        static const uint64 eBitsInIndex = sizeof(uint64) * CHAR_BIT;
        static const uint64 eBitsPerPriorityLevel = eBitsInIndex / JobManager::eNumPriorityLevel;
        static const uint64 eMaskStreamPriority     = (1ULL << eBitsPerPriorityLevel) - 1ULL;
        static const uint64 eMaskLowPriority            = (((1ULL << eBitsPerPriorityLevel) << eBitsPerPriorityLevel) - 1ULL) & ~eMaskStreamPriority;
        static const uint64 eMaskRegularPriority    = ((((1ULL << eBitsPerPriorityLevel) << eBitsPerPriorityLevel) << eBitsPerPriorityLevel) - 1ULL) & ~(eMaskStreamPriority | eMaskLowPriority);
        static const uint64 eMaskHighPriority           = (((((1ULL << eBitsPerPriorityLevel) << eBitsPerPriorityLevel) << eBitsPerPriorityLevel) << eBitsPerPriorityLevel) - 1ULL) & ~(eMaskRegularPriority | eMaskStreamPriority | eMaskLowPriority);

        ILINE static uint64 ExtractIndex(uint64 currentIndex, uint32 nPriorityLevel)
        {
            switch (nPriorityLevel)
            {
            case eHighPriority:
                return ((((currentIndex & eMaskHighPriority) >> eBitsPerPriorityLevel) >> eBitsPerPriorityLevel) >> eBitsPerPriorityLevel);
            case eRegularPriority:
                return (((currentIndex & eMaskRegularPriority) >> eBitsPerPriorityLevel) >> eBitsPerPriorityLevel);
            case eLowPriority:
                return ((currentIndex & eMaskLowPriority) >> eBitsPerPriorityLevel);
            case eStreamPriority:
                return (currentIndex & eMaskStreamPriority);
            default:
                return ~0;
            }
        }

        ILINE static bool IncreasePullIndex(uint64 currentPullIndex, uint64 currentPushIndex, uint64& rNewPullIndex, uint32& rPriorityLevel,
            uint32 nJobQueueSizeHighPriority, uint32 nJobQueueSizeRegularPriority, uint32 nJobQueueSizeLowPriority, uint32 nJobQueueSizeStreamPriority)
        {
            uint32 nPushExtractedHighPriority           = static_cast<uint32>(ExtractIndex(currentPushIndex, eHighPriority));
            uint32 nPushExtractedRegularPriority    = static_cast<uint32>(ExtractIndex(currentPushIndex, eRegularPriority));
            uint32 nPushExtractedLowPriority            = static_cast<uint32>(ExtractIndex(currentPushIndex, eLowPriority));
            uint32 nPushExtractedStreamPriority     = static_cast<uint32>(ExtractIndex(currentPushIndex, eStreamPriority));

            uint32 nPullExtractedHighPriority           = static_cast<uint32>(ExtractIndex(currentPullIndex, eHighPriority));
            uint32 nPullExtractedRegularPriority    = static_cast<uint32>(ExtractIndex(currentPullIndex, eRegularPriority));
            uint32 nPullExtractedLowPriority            = static_cast<uint32>(ExtractIndex(currentPullIndex, eLowPriority));
            uint32 nPullExtractedStreamPriority     = static_cast<uint32>(ExtractIndex(currentPullIndex, eStreamPriority));

            uint32 nRoundPushHighPriority               = nPushExtractedHighPriority / nJobQueueSizeHighPriority;
            uint32 nRoundPushRegularPriority        = nPushExtractedRegularPriority / nJobQueueSizeRegularPriority;
            uint32 nRoundPushLowPriority                = nPushExtractedLowPriority / nJobQueueSizeLowPriority;
            uint32 nRoundPushStreamPriority         = nPushExtractedStreamPriority / nJobQueueSizeStreamPriority;

            uint32 nRoundPullHighPriority               = nPullExtractedHighPriority / nJobQueueSizeHighPriority;
            uint32 nRoundPullRegularPriority        = nPullExtractedRegularPriority / nJobQueueSizeRegularPriority;
            uint32 nRoundPullLowPriority                = nPullExtractedLowPriority / nJobQueueSizeLowPriority;
            uint32 nRoundPullStreamPriority         = nPullExtractedStreamPriority / nJobQueueSizeStreamPriority;

            uint32 nPushJobSlotHighPriority         = nPushExtractedHighPriority & (nJobQueueSizeHighPriority - 1);
            uint32 nPushJobSlotRegularPriority  = nPushExtractedRegularPriority & (nJobQueueSizeRegularPriority - 1);
            uint32 nPushJobSlotLowPriority          = nPushExtractedLowPriority & (nJobQueueSizeLowPriority - 1);
            uint32 nPushJobSlotStreamPriority       = nPushExtractedStreamPriority & (nJobQueueSizeStreamPriority - 1);

            uint32 nPullJobSlotHighPriority         = nPullExtractedHighPriority & (nJobQueueSizeHighPriority - 1);
            uint32 nPullJobSlotRegularPriority  = nPullExtractedRegularPriority & (nJobQueueSizeRegularPriority - 1);
            uint32 nPullJobSlotLowPriority          = nPullExtractedLowPriority & (nJobQueueSizeLowPriority - 1);
            uint32 nPullJobSlotStreamPriority       = nPullExtractedStreamPriority & (nJobQueueSizeStreamPriority - 1);

            //NOTE: if this shows in the profiler, find a lockless variant of it
            if (((nRoundPushHighPriority == nRoundPullHighPriority) && nPullJobSlotHighPriority < nPushJobSlotHighPriority) ||
                ((nRoundPushHighPriority != nRoundPullHighPriority) && nPullJobSlotHighPriority >= nPushJobSlotHighPriority))
            {
                rPriorityLevel = eHighPriority;
                rNewPullIndex = IncreaseIndex(currentPullIndex, eHighPriority);
            }
            else if (((nRoundPushRegularPriority == nRoundPullRegularPriority) && nPullJobSlotRegularPriority < nPushJobSlotRegularPriority) ||
                     ((nRoundPushRegularPriority != nRoundPullRegularPriority) && nPullJobSlotRegularPriority >= nPushJobSlotRegularPriority))
            {
                rPriorityLevel = eRegularPriority;
                rNewPullIndex = IncreaseIndex(currentPullIndex, eRegularPriority);
            }
            else if (((nRoundPushLowPriority == nRoundPullLowPriority) && nPullJobSlotLowPriority < nPushJobSlotLowPriority) ||
                     ((nRoundPushLowPriority != nRoundPullLowPriority) && nPullJobSlotLowPriority >= nPushJobSlotLowPriority))
            {
                rPriorityLevel = eLowPriority;
                rNewPullIndex = IncreaseIndex(currentPullIndex, eLowPriority);
            }
            else if (((nRoundPushStreamPriority == nRoundPullStreamPriority) && nPullJobSlotStreamPriority < nPushJobSlotStreamPriority) ||
                     ((nRoundPushStreamPriority != nRoundPullStreamPriority) && nPullJobSlotStreamPriority >= nPushJobSlotStreamPriority))
            {
                rPriorityLevel = eStreamPriority;
                rNewPullIndex = IncreaseIndex(currentPullIndex, eStreamPriority);
            }
            else
            {
                rNewPullIndex = ~0;
                return false;
            }

            return true;
        }

        ILINE static uint64 IncreasePushIndex(uint64 currentPushIndex, uint32 nPriorityLevel)
        {
            return IncreaseIndex(currentPushIndex, nPriorityLevel);
        }

        ILINE static uint64 IncreaseIndex(uint64 currentIndex, uint32 nPriorityLevel)
        {
            uint64 nIncrease = 1ULL;
            uint64 nMask = 0;
            switch (nPriorityLevel)
            {
            case eHighPriority:
                nIncrease <<= eBitsPerPriorityLevel;
                nIncrease <<= eBitsPerPriorityLevel;
                nIncrease <<= eBitsPerPriorityLevel;
                nMask = eMaskHighPriority;
                break;
            case eRegularPriority:
                nIncrease <<= eBitsPerPriorityLevel;
                nIncrease <<= eBitsPerPriorityLevel;
                nMask = eMaskRegularPriority;
                break;
            case eLowPriority:
                nIncrease <<= eBitsPerPriorityLevel;
                nMask = eMaskLowPriority;
                break;
            case eStreamPriority:
                nMask = eMaskStreamPriority;
                break;
            }

            // increase counter while preventing overflow
            uint64 nCurrentValue                    = currentIndex & nMask;                         // extract all bits for this priority only
            uint64 nCurrentValueCleared     = currentIndex & ~nMask;                        // extract all bits of other priorities (they shouldn't change)
            nCurrentValue += nIncrease;                                                                             // increase value by one (already at the right bit position)
            nCurrentValue &= nMask;                                                                                     // mask out again to handle overflow
            uint64 nNewCurrentValue = nCurrentValueCleared | nCurrentValue;     // add new value to other priorities
            return nNewCurrentValue;
        }
    } _ALIGN(16);

    //struct covers DMA data setup common to all jobs and packets
    class CCommonDMABase
    {
    public:
        CCommonDMABase();
        void SetJobParamData(void* pParamData);
        const void* GetJobParamData() const;
        uint16 GetProfilingDataIndex() const;
        // in case of persistent job objects, we have to reset the profiling data
        void ForceUpdateOfProfilingDataIndex();

    protected:
        void*   m_pParamData;                   //parameter struct, not initialized to 0 for performance reason
        uint16 nProfilerIndex;          // index handle for Jobmanager Profiling Data
    };

    //delegation class for each job
    class CJobDelegator
        : public CCommonDMABase
    {
    public:

        typedef volatile CJobBase* TDepJob;

        CJobDelegator();
        void RunJob(const JobManager::TJobHandle cJobHandle);
        void RegisterQueue(const JobManager::SProdConsQueueBase* const cpQueue);
        JobManager::SProdConsQueueBase* GetQueue() const;

        void RegisterJobState(JobManager::SJobState* __restrict pJobState);

        //return a copy since it will get overwritten
        void SetParamDataSize(const unsigned int cParamDataSize);
        const unsigned int GetParamDataSize() const;
        void SetCurrentThreadId(const threadID cID);
        const threadID GetCurrentThreadId() const;

        JobManager::SJobState* GetJobState() const;
        void SetRunning();

        ILINE void SetDelegator(Invoker pGenericDelecator)
        {
            m_pGenericDelecator = pGenericDelecator;
        }

        Invoker GetGenericDelegator()
        {
            return m_pGenericDelecator;
        }

        unsigned int GetPriorityLevel() const { return m_nPrioritylevel; }
        bool IsBlocking() const { return m_bIsBlocking; };

        void SetPriorityLevel(unsigned int nPrioritylevel) { m_nPrioritylevel = nPrioritylevel; }
        void SetBlocking() { m_bIsBlocking = true; }

    protected:
        JobManager::SJobState* m_pJobState;                  // extern job state
        const JobManager::SProdConsQueueBase*   m_pQueue;   // consumer/producer queue
        unsigned int m_nPrioritylevel;                          // enum represent the priority level to use
        bool m_bIsBlocking;                                                 // if true, then the job could block on a mutex/sleep
        unsigned int        m_ParamDataSize;                        // sizeof parameter struct
        threadID        m_CurThreadID;                          // current thread id
        Invoker m_pGenericDelecator;
    };

    //base class for jobs
    class CJobBase
    {
    public:
        ILINE CJobBase()
            : m_pJobProgramData(NULL)
        {
        }

        ILINE void RegisterJobState(JobManager::SJobState* __restrict pJobState) volatile
        {
            ((CJobBase*)this)->m_JobDelegator.RegisterJobState(pJobState);
        }

        ILINE void RegisterQueue(const JobManager::SProdConsQueueBase* const cpQueue) volatile
        {
            ((CJobBase*)this)->m_JobDelegator.RegisterQueue(cpQueue);
        }
        ILINE void Run()
        {
            m_JobDelegator.RunJob(m_pJobProgramData);
        }

        ILINE const JobManager::TJobHandle GetJobProgramData()
        {
            return m_pJobProgramData;
        }


        ILINE void SetCurrentThreadId(const threadID cID)
        {
            ((CJobBase*)this)->m_JobDelegator.SetCurrentThreadId(cID);
        }

        ILINE const threadID GetCurrentThreadId() const
        {
            return ((CJobBase*)this)->m_JobDelegator.GetCurrentThreadId();
        }

        ILINE CJobDelegator* GetJobDelegator()
        {
            return &m_JobDelegator;
        }

    protected:
        CJobDelegator                                   m_JobDelegator;             //delegation implementation, all calls to job manager are going through it
        JobManager::TJobHandle              m_pJobProgramData;      //handle to program data to run

        //sets the job program data
        ILINE void SetJobProgramData(const JobManager::TJobHandle pJobProgramData)
        {
            assert(pJobProgramData);
            m_pJobProgramData = pJobProgramData;
        }
    };
} // namespace JobManager


// ==============================================================================
// Interface of the JobManager
// ==============================================================================

// Create a Producer consumer queue for a job type
#define PROD_CONS_QUEUE_TYPE(name, size) JobManager::CProdConsQueue < name, (size) >

// Declaration for the InvokeOnLinkedStacked util function
extern "C"
{
void InvokeOnLinkedStack(void (* proc)(void*), void* arg, void* stack, size_t stackSize);
} // extern "C"

namespace JobManager
{
    class IWorkerBackEndProfiler;

    // base class for the various backends the jobmanager supports
    struct IBackend
    {
        // <interfuscator:shuffle>
        virtual ~IBackend(){}

        virtual bool Init(uint32 nSysMaxWorker) = 0;
        virtual bool ShutDown() = 0;
        virtual void Update() = 0;

        virtual void AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle, JobManager::SInfoBlock& rInfoBlock) = 0;
        virtual uint32 GetNumWorkerThreads() const = 0;
        // </interfuscator:shuffle>

#if defined(JOBMANAGER_SUPPORT_FRAMEPROFILER)
        virtual IWorkerBackEndProfiler* GetBackEndWorkerProfiler() const = 0;
#endif
    };

    // singleton managing the job queues
    struct IJobManager
    {
        // <interfuscator:shuffle>
        // === front end interface ===
        virtual ~IJobManager(){}

        virtual void Init(uint32 nSysMaxWorker) = 0;

        // adds a job
        virtual void AddJob(JobManager::CJobDelegator& RESTRICT_REFERENCE crJob, const JobManager::TJobHandle cJobHandle) = 0;

        // wait for a job, preempt the calling thread if the job is not done yet
        virtual const bool WaitForJob(JobManager::SJobState& rJobState) const = 0;

        // obtain job handle from name
        virtual const JobManager::TJobHandle GetJobHandle(const char* cpJobName, const unsigned int cStrLen, JobManager::Invoker pInvoker) = 0;
        virtual const JobManager::TJobHandle GetJobHandle(const char* cpJobName, JobManager::Invoker pInvoker) = 0;

        // shuts down the job manager
        virtual void ShutDown() = 0;

        virtual JobManager::IBackend* GetBackEnd(JobManager::EBackEndType backEndType) = 0;

        virtual bool InvokeAsJob(const char* cJobHandle) const = 0;
        virtual bool InvokeAsJob(const JobManager::TJobHandle cJobHandle) const = 0;

        virtual void SetJobFilter(const char* pFilter) = 0;
        virtual void SetJobSystemEnabled(int nEnable) = 0;

        // If the calling thread is a Worker Thread, returns the Worker Thread ID, otherwise returns 0xFFFFFFFF
        // (Worker Thread ID is a index ranging 0 to GetNumWorkerThreads()-1)
        virtual uint32 GetWorkerThreadId() const = 0;

        virtual JobManager::SJobProfilingData* GetProfilingData(uint16 nProfilerIndex) = 0;
        virtual uint16 ReserveProfilingData() = 0;

        virtual void Update(int nJobSystemProfiler) = 0;
        virtual void PushProfilingMarker(const char* pName) = 0;
        virtual void PopProfilingMarker() = 0;


        virtual uint32 GetNumWorkerThreads() const = 0;

        // get a free semaphore handle from the Job Manager pool
        virtual JobManager::TSemaphoreHandle AllocateSemaphore(volatile const void* pOwner) = 0;

        // return a semaphore handle to the Job Manager pool
        virtual void DeallocateSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner) = 0;

        // increase the refcounter of a semaphore, but only if it is > 0, else returns false
        virtual bool AddRefSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner) = 0;

        // get the actual semaphore object for aquire/release calls
        virtual SJobFinishedConditionVariable* GetSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner) = 0;

        virtual void DumpJobList() = 0;

        virtual void SetFrameStartTime(const CTimeValue& rFrameStartTime) = 0;
    };

    /////////////////////////////////////////////////////////////////////////////
    // Util function to get the worker thread id in a job, returns 0xFFFFFFFF otherwise
    // (Worker Thread ID is a index ranging 0 to GetNumWorkerThreads()-1)
    ILINE uint32 GetWorkerThreadId()
    {
        uint32 nWorkerThreadID = gEnv->GetJobManager()->GetWorkerThreadId();
        return nWorkerThreadID == ~0 ? ~0 : (nWorkerThreadID & ~0x40000000);
    }

    /////////////////////////////////////////////////////////////////////////////
    // util function to find out if a call comes from the mainthread or from a worker thread
    ILINE bool IsWorkerThread()
    {
        return gEnv->GetJobManager()->GetWorkerThreadId() != ~0;
    }

    /////////////////////////////////////////////////////////////////////////////
    // util function to find out if a call comes from the mainthread or from a worker thread
    ILINE bool IsBlockingWorkerThread()
    {
        return (gEnv->GetJobManager()->GetWorkerThreadId() != ~0) && ((gEnv->GetJobManager()->GetWorkerThreadId() & 0x40000000) != 0);
    }


    /////////////////////////////////////////////////////////////////////////////
    // util function to check if a specific job should really run as job
    ILINE bool InvokeAsJob(const char* pJobName)
    {
        // We're earlying out here to avoid v-table lookup.
#if JOBMANAGER_DISABLED
        return false;
#elif defined(_RELEASE)
        return true; // in release alwasy assume to execute the job
#else
        return gEnv->pJobManager->InvokeAsJob(pJobName);
#endif
    }

    /////////////////////////////////////////////////////////////////////////////
    ILINE void SJobState::SetRunning()
    {
        SJobStateBase::SetRunning();
    }

    /////////////////////////////////////////////////////////////////////////////
    inline void SJobState::SetStopped()
    {
        CJobBase* pFollowUpJob = m_pFollowUpJob;    // take this pointer early as the job state could be freed by another thread
        bool bStopped = SJobStateBase::SetStopped();

        // start post job if set
        if (pFollowUpJob != NULL && bStopped)
        {
            pFollowUpJob->Run();
        }
    }

    // ==============================================================================
    // Interface of the Producer/Consumer Queue for JobManager
    // ==============================================================================
    //  producer - consumer queue
    //
    //  - all implemented ILINE using a template:
    //      - ring buffer size (num of elements)
    //      - instance type of job
    //      - param type of job
    //  - Factory with macro instantiating the queue (knowing the exact names for a job)
    //  - queue consists of:
    //      - ring buffer consists of 1 instance of param type of job
    //      - for atomic test on finished and push pointer update, the queue is 128 byte aligned and push, pull ptr and
    //              DMA job state are lying within that first 128 byte
    //      - volatile push (only modified by producer) /pull (only modified by consumer) pointer, point to ring buffer, both equal in the beginning
    //      - job instance (create with def. ctor)
    //      - DMA job state (running, finished)
    //      - AddPacket - method to add a packet
    //          - wait on current push/pull if any space is available
    //          - need atomically test if a job is running and update the push pointer, if job is finished, start new job
    //      - Finished method, returns push==pull
    //
    //  Job Producer side:
    //      - provide RegisterProdConsumerQueue - method, set flags accordingly
    //  Job side:
    //      - check if it has  a prod/consumer queue
    //      - if it is part of queue, it must obtain DMA address for job state and of push/pull (static offset to pointer)
    //      - a flag tells if job state is job state or queue
    //      - lock is obtained when current push/pull ptr is obtained, snooping therefore enabled
    //      - before FlushCacheComplete, get next parameter packet if multiple packets needs to be processed
    //      - if all packets were processed, try to write updated pull pointer and set finished state
    //              if it fails, push pointer was updated during processing time, in that case get lock again and with it the new push pointer
    //      - loop till the lock and finished state was updated successfully
    //      - no HandleCallback method, only 1 JOb is permitted to run with queue
    //      - no write back to external job state, always just one inner packet loop
    struct SProdConsQueueBase
    {
        // ---- don't move the state and push ptr to another place in this struct ----
        // ---- they are updated together with an atomic operation
        SJobSyncVariable        m_QueueRunningState;                // waiting state of the queue
        void* m_pPush;                                                                  //push pointer, current ptr to push packets into (written by Producer
        // ---------------------------------------------------------------------------

        volatile void* m_pPull;                                         // pull pointer, current ptr to pull packets from (written by Consumer)
        uint32 m_PullIncrement _ALIGN(16);                  // increment of pull
        uint32 m_AddPacketDataOffset;                               // offset of additional data relative to push ptr
        INT_PTR m_RingBufferStart;                                  // start of ring buffer
        INT_PTR m_RingBufferEnd;                                        // end of ring buffer
        SJobFinishedConditionVariable*              m_pQueueFullSemaphore;          // Semaphore used for yield-waiting when the queue is full

        SProdConsQueueBase();
    };

    // util functions namespace for the producer consumer queue
    /////////////////////////////////////////////////////////////////////////////////
    namespace ProdConsQueueBase
    {
        inline INT_PTR MarkPullPtrThatWeAreWaitingOnASemaphore(INT_PTR nOrgValue) { assert((nOrgValue & 1) == 0); return nOrgValue | 1; }
        inline bool IsPullPtrMarked(INT_PTR nOrgValue) { return (nOrgValue & 1) == 1; }
    }

    /////////////////////////////////////////////////////////////////////////////////
    template <class TJobType, unsigned int Size>
    class CProdConsQueue
        : public SProdConsQueueBase
    {
    public:
        CProdConsQueue();                                                               //default ctor
        ~CProdConsQueue();

        template <class TAnotherJobType>
        void AddPacket
        (
            const typename TJobType::packet& crPacket,
            JobManager::TPriorityLevel nPriorityLevel,
            TAnotherJobType* pJobParam,
            bool differentJob = true
        );  //adds a new parameter packet with different job type (job invocation)
        void AddPacket
        (
            const typename TJobType::packet& crPacket,
            JobManager::TPriorityLevel nPriorityLevel = JobManager::eRegularPriority
        );  //adds a new parameter packet (job invocation)
        void WaitFinished();                                        // wait til all current jobs have been finished and been processed
        bool IsEmpty();                                                 // returns true if queue is empty

    private:
        //initializes queue
        void Init(const unsigned int cPacketSize);

        //get incremented ptr, takes care of wrapping
        void* const GetIncrementedPointer();

        void* m_pRingBuffer _ALIGN(128);                                                                //the ring buffer
        TJobType m_JobInstance;                                                         //job instance
        int m_Initialized;
    } _ALIGN(128);//align for DMA speed and cache line reservation


    // Interface to track BackEnd worker utilisation and job execution timings
    class IWorkerBackEndProfiler
    {
    public:
        enum EJobSortOrder
        {
            eJobSortOrder_NoSort,
            eJobSortOrder_TimeHighToLow,
            eJobSortOrder_TimeLowToHigh,
            eJobSortOrder_Lexical,
        };

    public:
        typedef DynArray<JobManager::SJobFrameStats> TJobFrameStatsContainer;

    public:
        virtual ~IWorkerBackEndProfiler(){; }

        virtual void Init(const uint16 numWorkers) = 0;

        // Update the profiler at the beginning of the sample period
        virtual void Update() = 0;
        virtual void Update(const uint32 curTimeSample) = 0;

        // Register a job with the profiler
        virtual void RegisterJob(const uint32 jobId, const char* jobName) = 0;

        // Record execution information for a registered job
        virtual void RecordJob(const uint16 profileIndex, const uint8 workerId, const uint32 jobId, const uint32 runTimeMicroSec) = 0;

        // Get worker frame stats
        virtual void GetFrameStats(JobManager::CWorkerFrameStats& rStats) const = 0;
        virtual void GetFrameStats(TJobFrameStatsContainer& rJobStats, EJobSortOrder jobSortOrder) const = 0;
        virtual void GetFrameStats(JobManager::CWorkerFrameStats& rStats, TJobFrameStatsContainer& rJobStats, EJobSortOrder jobSortOrder) const = 0;

        // Get worker frame stats summary
        virtual void GetFrameStatsSummary(SWorkerFrameStatsSummary& rStats) const = 0;
        virtual void GetFrameStatsSummary(SJobFrameStatsSummary& rStats) const = 0;

        // Returns the index of the active multi-buffered profile data
        virtual uint16 GetProfileIndex() const = 0;

        // Get the number of workers tracked
        virtual uint32 GetNumWorkers() const = 0;

    public:
        // Returns a microsecond sample
        static uint32 GetTimeSample()
        {
            return static_cast<uint32>(gEnv->pTimer->GetAsyncTime().GetMicroSecondsAsInt64());
        }
    };
}// namespace JobManager

// interface to create the JobManager
// needed here for calls from Producer Consumer queue
extern "C"
{
DLL_EXPORT JobManager::IJobManager* GetJobManagerInterface();
}

// ==============================================================================
// Implementation of Producer Consumer functions
// currently adding jobs to a producer/consumer queue is not supported
// ==============================================================================
template <class TJobType, unsigned int Size>
ILINE JobManager::CProdConsQueue<TJobType, Size>::~CProdConsQueue()
{
    if (m_pRingBuffer)
    {
        CryModuleMemalignFree(m_pRingBuffer);
    }
}

///////////////////////////////////////////////////////////////////////////////
ILINE JobManager::SProdConsQueueBase::SProdConsQueueBase()
    : m_pPush(NULL)
    , m_pPull(NULL)
    , m_PullIncrement(0)
    , m_AddPacketDataOffset(0)
    , m_RingBufferStart(0)
    ,  m_RingBufferEnd(0)
    , m_pQueueFullSemaphore(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
ILINE JobManager::CProdConsQueue<TJobType, Size>::CProdConsQueue()
    : m_Initialized(0)
    , m_pRingBuffer(NULL)
{
    assert(Size > 2);
    m_JobInstance.RegisterQueue(this);
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
ILINE void JobManager::CProdConsQueue<TJobType, Size>::Init(const uint32 cPacketSize)
{
    ScopedSwitchToGlobalHeap heapSwitch;
    assert((cPacketSize & 15) == 0);
    m_AddPacketDataOffset = cPacketSize;
    m_PullIncrement             = m_AddPacketDataOffset + sizeof(SAddPacketData);
    m_pRingBuffer                   =  CryModuleMemalign(Size * m_PullIncrement, 128);

    assert(m_pRingBuffer);
    m_pPush = m_pRingBuffer;
    m_pPull = m_pRingBuffer;
    m_RingBufferStart   = (INT_PTR)m_pRingBuffer;
    m_RingBufferEnd     = m_RingBufferStart + Size * m_PullIncrement;
    m_Initialized           = 1;
    ((TJobType*)&m_JobInstance)->SetParamDataSize(cPacketSize);
    m_pQueueFullSemaphore = NULL;
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
ILINE void JobManager::CProdConsQueue<TJobType, Size>::WaitFinished()
{
    m_QueueRunningState.Wait();
    // ensure that the pull ptr is set right
    assert(m_pPull == m_pPush);
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
ILINE bool JobManager::CProdConsQueue<TJobType, Size>::IsEmpty()
{
    return (INT_PTR)m_pPush == (INT_PTR)m_pPull;
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
ILINE void* const JobManager::CProdConsQueue<TJobType, Size>::GetIncrementedPointer()
{
    //returns branch free the incremented wrapped aware param pointer
    INT_PTR cNextPtr = (INT_PTR)m_pPush + m_PullIncrement;
#if defined(PLATFORM_64BIT)
    if ((INT_PTR)cNextPtr >= (INT_PTR)m_RingBufferEnd)
    {
        cNextPtr = (INT_PTR)m_RingBufferStart;
    }
    return (void*)cNextPtr;
#else
    const unsigned int cNextPtrMask = (unsigned int)(((int)(cNextPtr - m_RingBufferEnd)) >> 31);
    return (void*)(cNextPtr & cNextPtrMask | m_RingBufferStart & ~cNextPtrMask);
#endif
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
inline void JobManager::CProdConsQueue<TJobType, Size>::AddPacket
(
    const typename TJobType::packet& crPacket,
    JobManager::TPriorityLevel nPriorityLevel
)
{
    AddPacket<TJobType>(crPacket, nPriorityLevel, (TJobType*)&m_JobInstance, false);
}

///////////////////////////////////////////////////////////////////////////////
template <class TJobType, unsigned int Size>
template <class TAnotherJobType>
inline void JobManager::CProdConsQueue<TJobType, Size>::AddPacket
(
    const typename TJobType::packet& crPacket,
    JobManager::TPriorityLevel nPriorityLevel,
    TAnotherJobType* pJobParam,
    bool differentJob
)
{
    const uint32 cPacketSize = crPacket.GetPacketSize();
    IF (m_Initialized == 0, 0)
    {
        Init(cPacketSize);
    }

    assert(m_RingBufferEnd == m_RingBufferStart + Size * (cPacketSize + sizeof(SAddPacketData)));

    const void* const cpCurPush = m_pPush;

    // don't overtake the pull ptr

    SJobSyncVariable curQueueRunningState = m_QueueRunningState;
    IF ((cpCurPush == m_pPull) && (curQueueRunningState.IsRunning()), 0)
    {
        INT_PTR nPushPtr = (INT_PTR)cpCurPush;
        INT_PTR markedPushPtr = JobManager::ProdConsQueueBase::MarkPullPtrThatWeAreWaitingOnASemaphore(nPushPtr);
        TSemaphoreHandle nSemaphoreHandle = gEnv->pJobManager->AllocateSemaphore(this);
        m_pQueueFullSemaphore = gEnv->pJobManager->GetSemaphore(nSemaphoreHandle, this);

        bool bNeedSemaphoreWait = false;
#if defined(PLATFORM_64BIT) // for 64 bit, we need to atomicly swap 128 bit
        int64 compareValue[2] = { *alias_cast<int64*>(&curQueueRunningState), (int64)nPushPtr};
        _InterlockedCompareExchange128((volatile int64*)this, (int64)markedPushPtr, *alias_cast<int64*>(&curQueueRunningState), compareValue);
        // make sure nobody set the state to stopped in the meantime
        bNeedSemaphoreWait = (compareValue[0] == *alias_cast<int64*>(&curQueueRunningState) && compareValue[1] == (int64)nPushPtr);
#else
        // union used to construct 64 bit value for atomic updates
        union T64BitValue
        {
            int64 doubleWord;
            struct
            {
                uint32 word0;
                uint32 word1;
            };
        };

        // job system is process the queue right now, we need an atomic update
        T64BitValue compareValue;
        compareValue.word0 = *alias_cast<uint32*>(&curQueueRunningState);
        compareValue.word1 = (uint32)nPushPtr;

        T64BitValue exchangeValue;
        exchangeValue.word0 = *alias_cast<uint32*>(&curQueueRunningState);
        exchangeValue.word1 = (uint32)markedPushPtr;

        T64BitValue resultValue;
        resultValue.doubleWord = CryInterlockedCompareExchange64((volatile int64*)this, exchangeValue.doubleWord, compareValue.doubleWord);

        bNeedSemaphoreWait = (resultValue.word0 == *alias_cast<uint32*>(&curQueueRunningState) && resultValue.word1 == nPushPtr);
#endif

        // C-A-S successful, now we need to do a yield-wait
        IF (bNeedSemaphoreWait, 0)
        {
            m_pQueueFullSemaphore->Acquire();
        }
        else
        {
            m_pQueueFullSemaphore->SetStopped();
        }

        gEnv->pJobManager->DeallocateSemaphore(nSemaphoreHandle, this);
        m_pQueueFullSemaphore = NULL;
    }

    if (crPacket.GetJobStateAddress())
    {
        JobManager::SJobState* pJobState = reinterpret_cast<JobManager::SJobState*>(crPacket.GetJobStateAddress());
        pJobState->SetRunning();
    }

    //get incremented push pointer and check if there is a slot to push it into
    void* const cpNextPushPtr = GetIncrementedPointer();

    const SVEC4_UINT* __restrict pPacketCont = crPacket.GetPacketCont();
    SVEC4_UINT* __restrict pPushCont                = (SVEC4_UINT*)cpCurPush;

    //copy packet data
    const uint32 cIters = cPacketSize >> 4;
    for (uint32 i = 0; i < cIters; ++i)
    {
        pPushCont[i] = pPacketCont[i];
    }


    // setup addpacket data for Jobs
    SAddPacketData* const __restrict pAddPacketData = (SAddPacketData*)((unsigned char*)cpCurPush + m_AddPacketDataOffset);
    pAddPacketData->pJobState = crPacket.GetJobStateAddress();

#if defined(JOBMANAGER_SUPPORT_PROFILING)
    SJobProfilingData* pJobProfilingData = gEnv->GetJobManager()->GetProfilingData(crPacket.GetProfilerIndex());
    pJobProfilingData->jobHandle = pJobParam->GetJobProgramData();
    pAddPacketData->profilerIndex = crPacket.GetProfilerIndex();
    if (pAddPacketData->pJobState) // also store profilerindex in syncvar, so be able to record wait times
    {
        pAddPacketData->pJobState->nProfilerIndex = pAddPacketData->profilerIndex;
    }
#endif

    // set invoker, for the case the the job changes within the queue
    pAddPacketData->nInvokerIndex = pJobParam->GetProgramHandle()->nJobInvokerIdx;

    // new job queue, or empty job queue
    if (!m_QueueRunningState.IsRunning())
    {
        m_pPush = cpNextPushPtr;//make visible
        m_QueueRunningState.SetRunning();

        pJobParam->RegisterQueue(this);
        pJobParam->SetParamDataSize(((TJobType*)&m_JobInstance)->GetParamDataSize());
        pJobParam->SetPriorityLevel(nPriorityLevel);
        pJobParam->Run();

        return;
    }

    bool bAtomicSwapSuccessfull = false;
    JobManager::SJobSyncVariable newSyncVar;
    newSyncVar.SetRunning();
#if defined(PLATFORM_64BIT) // for 64 bit, we need to atomicly swap 128 bit
    int64 compareValue[2] = { *alias_cast<int64*>(&newSyncVar), (int64)cpCurPush};
    _InterlockedCompareExchange128((volatile int64*)this, (int64)cpNextPushPtr, *alias_cast<int64*>(&newSyncVar), compareValue);
    // make sure nobody set the state to stopped in the meantime
    bAtomicSwapSuccessfull = (compareValue[0] > 0);
#else
    // union used to construct 64 bit value for atomic updates
    union T64BitValue
    {
        int64 doubleWord;
        struct
        {
            uint32 word0;
            uint32 word1;
        };
    };

    // job system is process the queue right now, we need an atomic update
    T64BitValue compareValue;
    compareValue.word0 = *alias_cast<uint32*>(&newSyncVar);
    compareValue.word1 = (uint32)(TRUNCATE_PTR)cpCurPush;

    T64BitValue exchangeValue;
    exchangeValue.word0 = *alias_cast<uint32*>(&newSyncVar);
    exchangeValue.word1 = (uint32)(TRUNCATE_PTR)cpNextPushPtr;

    T64BitValue resultValue;
    resultValue.doubleWord = CryInterlockedCompareExchange64((volatile int64*)this, exchangeValue.doubleWord, compareValue.doubleWord);

    bAtomicSwapSuccessfull = (resultValue.word0 > 0);
#endif

    // job system finished in the meantime, next to issue a new job
    if (!bAtomicSwapSuccessfull)
    {
        m_pPush = cpNextPushPtr;//make visible
        m_QueueRunningState.SetRunning();

        pJobParam->RegisterQueue(this);
        pJobParam->SetParamDataSize(((TJobType*)&m_JobInstance)->GetParamDataSize());
        pJobParam->SetPriorityLevel(nPriorityLevel);
        pJobParam->Run();
    }
}

// ==============================================================================
// CCommonDMABase Function implementations
// ==============================================================================
inline JobManager::CCommonDMABase::CCommonDMABase()
{
    ForceUpdateOfProfilingDataIndex();
}

///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CCommonDMABase::ForceUpdateOfProfilingDataIndex()
{
#if defined(JOBMANAGER_SUPPORT_PROFILING)
    nProfilerIndex = gEnv->GetJobManager()->ReserveProfilingData();
#endif
}

///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CCommonDMABase::SetJobParamData(void* pParamData)
{
    m_pParamData = pParamData;
}

///////////////////////////////////////////////////////////////////////////////
inline const void* JobManager::CCommonDMABase::GetJobParamData() const
{
    return m_pParamData;
}

///////////////////////////////////////////////////////////////////////////////
inline uint16 JobManager::CCommonDMABase::GetProfilingDataIndex() const
{
    return nProfilerIndex;
}

// ==============================================================================
// Job Delegator Function implementations
// ==============================================================================
ILINE JobManager::CJobDelegator::CJobDelegator()
    : m_ParamDataSize(0)
    , m_CurThreadID(THREADID_NULL)
{
    m_pQueue = NULL;
    m_pJobState = NULL;
    m_nPrioritylevel = JobManager::eRegularPriority;
    m_bIsBlocking = false;
}

///////////////////////////////////////////////////////////////////////////////
ILINE void JobManager::CJobDelegator::RunJob(const JobManager::TJobHandle cJobHandle)
{
    gEnv->GetJobManager()->AddJob(*static_cast<CJobDelegator*>(this), cJobHandle);
}

///////////////////////////////////////////////////////////////////////////////
ILINE void JobManager::CJobDelegator::RegisterQueue(const JobManager::SProdConsQueueBase* const cpQueue)
{
    m_pQueue = cpQueue;
}

///////////////////////////////////////////////////////////////////////////////
ILINE JobManager::SProdConsQueueBase* JobManager::CJobDelegator::GetQueue() const
{
    return const_cast<JobManager::SProdConsQueueBase*>(m_pQueue);
}

///////////////////////////////////////////////////////////////////////////////
ILINE void JobManager::CJobDelegator::RegisterJobState(JobManager::SJobState* __restrict pJobState)
{
    assert(pJobState);
    m_pJobState = pJobState;
#if defined(JOBMANAGER_SUPPORT_PROFILING)
    pJobState->nProfilerIndex = this->GetProfilingDataIndex();
#endif
}

///////////////////////////////////////////////////////////////////////////////
ILINE void JobManager::CJobDelegator::SetParamDataSize(const unsigned int cParamDataSize)
{
    m_ParamDataSize = cParamDataSize;
}

///////////////////////////////////////////////////////////////////////////////
ILINE const unsigned int JobManager::CJobDelegator::GetParamDataSize() const
{
    return m_ParamDataSize;
}

///////////////////////////////////////////////////////////////////////////////
ILINE void JobManager::CJobDelegator::SetCurrentThreadId(const threadID cID)
{
    m_CurThreadID = cID;
}

///////////////////////////////////////////////////////////////////////////////
ILINE const threadID JobManager::CJobDelegator::GetCurrentThreadId() const
{
    return m_CurThreadID;
}

///////////////////////////////////////////////////////////////////////////////
ILINE JobManager::SJobState* JobManager::CJobDelegator::GetJobState() const
{
    return m_pJobState;
}

///////////////////////////////////////////////////////////////////////////////
ILINE void JobManager::CJobDelegator::SetRunning()
{
    m_pJobState->SetRunning();
}


// ==============================================================================
// Implementation of SJObSyncVariable functions
// ==============================================================================
inline JobManager::SJobSyncVariable::SJobSyncVariable()
{
    syncVar.wordValue = 0;
#if defined(PLATFORM_64BIT)
    padding[0] = padding[1] = padding[2] = padding[3]  = 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////////
inline bool JobManager::SJobSyncVariable::IsRunning() const volatile
{
    return syncVar.nRunningCounter != 0;
}

/////////////////////////////////////////////////////////////////////////////////
inline void JobManager::SJobSyncVariable::Wait() volatile
{
    if (syncVar.wordValue == 0)
    {
        return;
    }
    AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::System);

    IJobManager* pJobManager = gEnv->GetJobManager();

    SyncVar currentValue;
    SyncVar newValue;
    SyncVar resValue;
    TSemaphoreHandle semaphoreHandle = gEnv->GetJobManager()->AllocateSemaphore(this);

retry:
    // volatile read
    currentValue.wordValue = syncVar.wordValue;

    if (currentValue.wordValue != 0)
    {
        // there is already a semaphore, wait for this one
        if (currentValue.semaphoreHandle)
        {
            // prevent the following race condition: Thread A Gets the semaphore and waits, then Thread B fails the earlier C-A-S
            // thus will wait for the same semaphore, but Thread A already release the semaphore and now another Thread (could even be A again)
            // is using the same semaphore to wait for the job (executed by thread B) which waits for the samephore -> Deadlock
            // this works since the jobmanager semphore function internally are using locks
            // so if the following line succeeds, we got the lock before the release and have increased the use counter
            // if not, it means that thread A release the semaphore, which also means thread B doesn't have to wait anymore

            if (pJobManager->AddRefSemaphore(currentValue.semaphoreHandle, this))
            {
                pJobManager->GetSemaphore(currentValue.semaphoreHandle, this)->Acquire();
                pJobManager->DeallocateSemaphore(currentValue.semaphoreHandle, this);
            }
        }
        else // no semaphore found
        {
            newValue = currentValue;
            newValue.semaphoreHandle = semaphoreHandle;
            resValue.wordValue = CryInterlockedCompareExchange((volatile LONG*)&syncVar.wordValue, newValue.wordValue, currentValue.wordValue);

            // four case are now possible:
            //a) job has finished -> we only need to free our semaphore
            //b) we succeeded setting our semaphore, thus wait for it
            //c) another waiter has set it's sempahore wait for the other one
            //d) another thread has increased/decreased the num runner variable, we need to do the run again

            if (resValue.wordValue != 0) // case a), do nothing
            {
                if (resValue.wordValue == currentValue.wordValue) // case b)
                {
                    pJobManager->GetSemaphore(semaphoreHandle, this)->Acquire();
                }
                else // C-A-S not succeeded, check how to proceed
                {
                    if (resValue.semaphoreHandle) // case c)
                    {
                        // prevent the following race condition: Thread A Gets the semaphore and waits, then Thread B fails the earlier C-A-S
                        // thus will wait for the same semaphore, but Thread A already release the semaphore and now another Thread (could even be A again)
                        // is using the same semaphore to wait for the job (executed by thread B) which waits for the samephore -> Deadlock
                        // this works since the jobmanager semphore function internally are using locks
                        // so if the following line succeeds, we got the lock before the release and have increased the use counter
                        // if not, it means that thread A release the semaphore, which also means thread B doesn't have to wait anymore

                        if (pJobManager->AddRefSemaphore(resValue.semaphoreHandle, this))
                        {
                            pJobManager->GetSemaphore(resValue.semaphoreHandle, this)->Acquire();
                            pJobManager->DeallocateSemaphore(resValue.semaphoreHandle, this);
                        }
                    }
                    else // case d
                    {
                        goto retry;
                    }
                }
            }
        }
    }

    // mark an old semaphore as stopped, in case we didn't use use  (if we did use it, this is a nop)
    pJobManager->GetSemaphore(semaphoreHandle, this)->SetStopped();
    pJobManager->DeallocateSemaphore(semaphoreHandle, this);
}

/////////////////////////////////////////////////////////////////////////////////
inline void JobManager::SJobSyncVariable::SetRunning() volatile
{
    AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::System);

    SyncVar currentValue;
    SyncVar newValue;

    do
    {
        // volatile read
        currentValue.wordValue = syncVar.wordValue;

        newValue = currentValue;
        newValue.nRunningCounter += 1;
    } while (CryInterlockedCompareExchange((volatile LONG*)&syncVar.wordValue, newValue.wordValue, currentValue.wordValue) != currentValue.wordValue);
}

/////////////////////////////////////////////////////////////////////////////////
inline bool JobManager::SJobSyncVariable::SetStopped() volatile
{
    AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::System);

    SyncVar currentValue;
    SyncVar newValue;
    SyncVar resValue;

    // first use atomic operations to decrease the running counter ("--syncVar.nRunningCounter")
    do
    {
        // volatile read
        currentValue.wordValue = syncVar.wordValue;

        newValue = currentValue;
        assert(newValue.nRunningCounter > 0);
        newValue.nRunningCounter -= 1;

        resValue.wordValue = CryInterlockedCompareExchange((volatile LONG*)&syncVar.wordValue, newValue.wordValue, currentValue.wordValue);
    } while (resValue.wordValue != currentValue.wordValue);

    // we now successfull decreased our counter, check if we need to signal a semaphore
    if (newValue.nRunningCounter > 0) // return if we still have other jobs on this syncvar
    {
        return false;
    }

    // do we need to release a semaphore
    if (currentValue.semaphoreHandle)
    {
        // try to set the running counter to 0 atomically
        do
        {
            // volatile read
            currentValue.wordValue = syncVar.wordValue;
            newValue = currentValue;
            newValue.semaphoreHandle = 0;

            // another thread increased the running counter again, don't call semaphore release in this case
            if (currentValue.nRunningCounter)
            {
                return false;
            }
        } while (CryInterlockedCompareExchange((volatile LONG*)&syncVar.wordValue, newValue.wordValue, currentValue.wordValue) != currentValue.wordValue);
        // set the running successfull to 0, now we can release the semaphore
        gEnv->GetJobManager()->GetSemaphore(resValue.semaphoreHandle, this)->Release();
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// SInfoBlock State functions
/////////////////////////////////////////////////////////////////////////////////
inline void JobManager::SInfoBlock::Release(uint32 nMaxValue)
{
    SInfoBlockState currentInfoBlockState;
    SInfoBlockState newInfoBlockState;
    SInfoBlockState resultInfoBlockState;

    // use atomic operations to set the running flag
    do
    {
        // volatile read
        currentInfoBlockState.nValue = *(const_cast<volatile LONG*>(&jobState.nValue));

        newInfoBlockState.nSemaphoreHandle = 0;
        newInfoBlockState.nRoundID = (currentInfoBlockState.nRoundID + 1) & 0x7FFF;
        newInfoBlockState.nRoundID = newInfoBlockState.nRoundID >= nMaxValue ? 0 : newInfoBlockState.nRoundID;

        resultInfoBlockState.nValue = CryInterlockedCompareExchange((volatile LONG*)&jobState.nValue, newInfoBlockState.nValue, currentInfoBlockState.nValue);
    } while (resultInfoBlockState.nValue != currentInfoBlockState.nValue);

    // do we need to release a semaphore
    if (currentInfoBlockState.nSemaphoreHandle)
    {
        gEnv->GetJobManager()->GetSemaphore(currentInfoBlockState.nSemaphoreHandle, this)->Release();
    }
}

/////////////////////////////////////////////////////////////////////////////////
inline void JobManager::SInfoBlock::Wait(uint32 nRoundID, uint32 nMaxValue)
{
    bool bWait = false;
    bool bRetry = false;
    jobState.IsInUse(nRoundID, bWait, bRetry, nMaxValue);
    if (bWait == false)
    {
        return;
    }

    AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::System);

    JobManager::IJobManager* pJobManager = gEnv->GetJobManager();

    // get a semaphore to wait on
    TSemaphoreHandle semaphoreHandle = pJobManager->AllocateSemaphore(this);

    // try to set semaphore (info block could have finished in the meantime, or another thread has set the semaphore
    SInfoBlockState currentInfoBlockState;
    SInfoBlockState newInfoBlockState;
    SInfoBlockState resultInfoBlockState;

    currentInfoBlockState.nValue = *(const_cast<volatile LONG*>(&jobState.nValue));

    // make sure the
    currentInfoBlockState.IsInUse(nRoundID, bWait, bRetry, nMaxValue);
    if (bWait == true)
    {
        // is a semaphore already set
        if (currentInfoBlockState.nSemaphoreHandle != 0)
        {
            if (pJobManager->AddRefSemaphore(currentInfoBlockState.nSemaphoreHandle, this))
            {
                pJobManager->GetSemaphore(currentInfoBlockState.nSemaphoreHandle, this)->Acquire();
                pJobManager->DeallocateSemaphore(currentInfoBlockState.nSemaphoreHandle, this);
            }
        }
        else
        {
            // try to set the semaphore
            newInfoBlockState.nRoundID = currentInfoBlockState.nRoundID;
            newInfoBlockState.nSemaphoreHandle = semaphoreHandle;

            resultInfoBlockState.nValue = CryInterlockedCompareExchange((volatile LONG*)&jobState.nValue, newInfoBlockState.nValue, currentInfoBlockState.nValue);


            // three case are now possible:
            //a) job has finished -> we only need to free our semaphore
            //b) we succeeded setting our semaphore, thus wait for it
            //c) another waiter has set it's sempahore wait for the other one
            resultInfoBlockState.IsInUse(nRoundID, bWait, bRetry, nMaxValue);
            if (bWait == true)  // case a) do nothing
            {
                if (resultInfoBlockState.nValue == currentInfoBlockState.nValue) // case b)
                {
                    pJobManager->GetSemaphore(semaphoreHandle, this)->Acquire();
                }
                else // case c)
                {
                    if (pJobManager->AddRefSemaphore(resultInfoBlockState.nSemaphoreHandle, this))
                    {
                        pJobManager->GetSemaphore(resultInfoBlockState.nSemaphoreHandle, this)->Acquire();
                        pJobManager->DeallocateSemaphore(resultInfoBlockState.nSemaphoreHandle, this);
                    }
                }
            }
        }
    }

    pJobManager->GetSemaphore(semaphoreHandle, this)->SetStopped();
    pJobManager->DeallocateSemaphore(semaphoreHandle, this);
}

#endif // CRYINCLUDE_CRYCOMMON_IJOBMANAGER_H
