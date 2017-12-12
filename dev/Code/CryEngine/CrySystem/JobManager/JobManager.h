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

#ifndef CRYINCLUDE_CRYSYSTEM_JOBMANAGER_JOBMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_JOBMANAGER_JOBMANAGER_H
#pragma once


#define JOBSYSTEM_INVOKER_COUNT (128)

#include <platform.h>
#include <MultiThread.h>

#include <IJobManager.h>
#include "JobStructs.h"

#include <map>

#include <AzFramework/Input/Events/InputChannelEventListener.h>

///////////////////////////////////////////////////////////////////////////////
namespace JobManager
{
    /////////////////////////////////////////////////////////////////////////////
    // Util Functions to get access Thread local data (needs to be unique per worker thread)
    namespace detail {
        // function to manipulate the per thread fallback job freelist
        void PushToFallbackJobList(JobManager::SInfoBlock* pInfoBlock);
        JobManager::SInfoBlock* PopFromFallbackJobList();

        // functions to access the per thread worker thread id
        void SetWorkerThreadId(uint32 nWorkerThreadId);
        uint32 GetWorkerThreadId();
    } // namespace detail

    // Tracks CPU/PPU worker thread(s) utilization and job execution time per frame
    class CWorkerBackEndProfiler
        : public IWorkerBackEndProfiler
    {
    public:
        CWorkerBackEndProfiler();
        virtual ~CWorkerBackEndProfiler();

        virtual void Init(const uint16 numWorkers);

        // Update the profiler at the beginning of the sample period
        virtual void Update();
        virtual void Update(const uint32 curTimeSample);

        // Register a job with the profiler
        virtual void RegisterJob(const uint32 jobId, const char* jobName);

        // Record execution information for a registered job
        virtual void RecordJob(const uint16 profileIndex, const uint8 workerId, const uint32 jobId, const uint32 runTimeMicroSec);

        // Get worker frame stats for the JobManager::detail::eJOB_FRAME_STATS - 1 frame
        virtual void GetFrameStats(JobManager::CWorkerFrameStats& rStats) const;
        virtual void GetFrameStats(TJobFrameStatsContainer& rJobStats, IWorkerBackEndProfiler::EJobSortOrder jobSortOrder) const;
        virtual void GetFrameStats(JobManager::CWorkerFrameStats& rStats, TJobFrameStatsContainer& rJobStats, IWorkerBackEndProfiler::EJobSortOrder jobSortOrder) const;

        // Get worker frame stats summary
        virtual void GetFrameStatsSummary(SWorkerFrameStatsSummary& rStats) const;
        virtual void GetFrameStatsSummary(SJobFrameStatsSummary& rStats) const;

        // Returns the index of the active multi-buffered profile data
        virtual uint16 GetProfileIndex() const;

        // Get the number of workers tracked
        virtual uint32 GetNumWorkers() const;

    protected:
        void GetWorkerStats(const uint8 nBufferIndex, JobManager::CWorkerFrameStats& rWorkerStats) const;
        void GetJobStats(const uint8 nBufferIndex, TJobFrameStatsContainer& rJobStatsContainer, IWorkerBackEndProfiler::EJobSortOrder jobSortOrder) const;
        void ResetWorkerStats(const uint8 nBufferIndex, const uint32 curTimeSample);
        void ResetJobStats(const uint8 nBufferIndex);

    protected:
        struct SJobStatsInfo
        {
            JobManager::SJobFrameStats  m_pJobStats[JobManager::detail::eJOB_FRAME_STATS * JobManager::detail::eJOB_FRAME_STATS_MAX_SUPP_JOBS ];// Array of job stats (multi buffered)
        };

        struct SWorkerStatsInfo
        {
            uint32                                      m_nStartTime[JobManager::detail::eJOB_FRAME_STATS];// Start Time of sample period (multi buffered)
            uint32                                      m_nEndTime[JobManager::detail::eJOB_FRAME_STATS];   // End Time of sample period (multi buffered)
            uint16                                      m_nNumWorkers;// Number of workers tracked
            JobManager::SWorkerStats* m_pWorkerStats; // Array of worker stats for each worker (multi buffered)
        };

    protected:
        uint8                         m_nCurBufIndex;       // Current buffer index [0,(JobManager::detail::eJOB_FRAME_STATS-1)]
        SJobStatsInfo           m_JobStatsInfo;     // Information about all job activities
        SWorkerStatsInfo    m_WorkerStatsInfo;// Information about each worker's utilization
    };


    // singleton managing the job queues
    class CJobManager
        : public IJobManager
        , public AzFramework::InputChannelEventListener
    {
    public:
        // singleton stuff
        static CJobManager* Instance();

        //destructor
        virtual ~CJobManager()
        {
            delete m_pThreadBackEnd;
            delete m_pFallBackBackEnd;
        }

        virtual void Init(uint32 nSysMaxWorker);

        // wait for a job, preempt the calling thread if the job is not done yet
        virtual const bool WaitForJob(JobManager::SJobState& rJobState) const;

        //adds a job
        virtual void AddJob(JobManager::CJobDelegator& crJob, const JobManager::TJobHandle cJobHandle);

        //obtain job handle from name
        virtual const JobManager::TJobHandle GetJobHandle(const char* cpJobName, const uint32 cStrLen, JobManager::Invoker pInvoker);
        virtual const JobManager::TJobHandle GetJobHandle(const char* cpJobName, JobManager::Invoker pInvoker)
        {
            return GetJobHandle(cpJobName, strlen(cpJobName), pInvoker);
        }


        virtual JobManager::IBackend* GetBackEnd(JobManager::EBackEndType backEndType)
        {
            switch (backEndType)
            {
            case eBET_Thread:
                return m_pThreadBackEnd;
            case eBET_Blocking:
                return m_pBlockingBackEnd;
            case eBET_Fallback:
                return m_pFallBackBackEnd;
            default:
                CRY_ASSERT_MESSAGE(0, "Unsupported EBackEndType encountered.");
                __debugbreak();
                return 0;
            }
            ;

            return 0;
        }

        //shuts down job manager
        virtual void ShutDown();

        virtual bool InvokeAsJob(const char* cpJobName) const;
        virtual bool InvokeAsJob(const JobManager::TJobHandle cJobHandle) const;

        virtual void SetJobFilter(const char* pFilter)
        {
            m_pJobFilter = pFilter;
        }

        virtual void SetJobSystemEnabled(int nEnable)
        {
#if JOBMANAGER_DISABLED
            m_nJobSystemEnabled = 0;
#else
            m_nJobSystemEnabled = nEnable;
#endif // JOBMANAGER_DISABLED
        }

        virtual void PushProfilingMarker(const char* pName);
        virtual void PopProfilingMarker();

        // move to right place
        enum
        {
            nMarkerEntries = 1024
        };
        struct SMarker
        {
            typedef CryFixedStringT<64> TMarkerString;
            enum MarkerType
            {
                PUSH_MARKER, POP_MARKER
            };

            SMarker() {}
            SMarker(MarkerType _type, CTimeValue _time, bool _bIsMainThread)
                : type(_type)
                , time(_time)
                , bIsMainThread(_bIsMainThread) {}
            SMarker(MarkerType _type, const char* pName, CTimeValue _time, bool _bIsMainThread)
                : type(_type)
                , marker(pName)
                , time(_time)
                , bIsMainThread(_bIsMainThread) {}

            TMarkerString marker;
            MarkerType type;
            CTimeValue time;
            bool bIsMainThread;
        };
        uint32 m_nMainThreadMarkerIndex[SJobProfilingDataContainer::nCapturedFrames];
        uint32 m_nRenderThreadMarkerIndex[SJobProfilingDataContainer::nCapturedFrames];
        SMarker m_arrMainThreadMarker[SJobProfilingDataContainer::nCapturedFrames][nMarkerEntries];
        SMarker m_arrRenderThreadMarker[SJobProfilingDataContainer::nCapturedFrames][nMarkerEntries];
        // move to right place

        //copy the job parameter into the jobinfo  structure
        static void CopyJobParameter(const uint32 cJobParamSize, void* pDest,   const void* pSrc);

        uint32 GetWorkerThreadId() const;

        virtual JobManager::SJobProfilingData* GetProfilingData(uint16 nProfilerIndex);
        virtual uint16 ReserveProfilingData();

        void Update(int nJobSystemProfiler);

        virtual void SetFrameStartTime(const CTimeValue& rFrameStartTime);

        ColorB GetRegionColor(SMarker::TMarkerString marker);
        JobManager::Invoker GetJobInvoker(uint32 nIdx)
        {
            assert(nIdx < m_nJobInvokerIdx);
            assert(nIdx < JOBSYSTEM_INVOKER_COUNT);
            return m_arrJobInvokers[nIdx];
        }
        virtual uint32 GetNumWorkerThreads() const
        {
            return m_pThreadBackEnd ? m_pThreadBackEnd->GetNumWorkerThreads() : 0;
        }

        // get a free semaphore from the jobmanager pool
        virtual JobManager::TSemaphoreHandle AllocateSemaphore(volatile const void* pOwner);

        // return a semaphore to the jobmanager pool
        virtual void DeallocateSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner);

        // increase the refcounter of a semaphore, but only if it is > 0, else returns false
        virtual bool AddRefSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner);

        // 'allocate' a semaphore in the jobmanager, and return the index of it
        virtual SJobFinishedConditionVariable* GetSemaphore(JobManager::TSemaphoreHandle nSemaphoreHandle, volatile const void* pOwner);

        virtual void DumpJobList();

        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        void IncreaseRunJobs();
        void IncreaseRunFallbackJobs();

        void AddBlockingFallbackJob(JobManager::SInfoBlock* pInfoBlock, uint32 nWorkerThreadID);

        const char* GetJobName(JobManager::Invoker invoker);

    private:
        CryCriticalSection m_JobManagerLock;                                                        // lock to protect non-performance critical parts of the jobmanager
        JobManager::Invoker m_arrJobInvokers[JOBSYSTEM_INVOKER_COUNT]; // support 128 jobs for now
        uint32 m_nJobInvokerIdx;

        const char* m_pJobFilter;
        int m_nJobSystemEnabled;                                                            // should the job system be used
        int m_bJobSystemProfilerEnabled;                                            // should the job system profiler be enabled
        bool m_bJobSystemProfilerPaused;                                            // should the job system profiler be paused

        bool     m_Initialized;                                                                 //true if JobManager have been initialized

        IBackend*               m_pFallBackBackEnd;             // Backend for development, jobs are executed in their calling thread
	// Backend for regular jobs, available on PC
        IBackend*               m_pThreadBackEnd;
        IBackend*               m_pBlockingBackEnd;             // Backend for tasks which can block to prevent stalling regular jobs in this case

        uint16                  m_nJobIdCounter;                    // JobId counter for jobs dynamically allocated at runtime

        std::set<JobManager::SJobStringHandle> m_registeredJobs;

        enum
        {
            nSemaphorePoolSize = 16
        };
        SJobFinishedConditionVariable       m_JobSemaphorePool[nSemaphorePoolSize];
        uint32                                                  m_nCurrentSemaphoreIndex;

        // per frame counter for jobs run/fallback jobs
        uint32 m_nJobsRunCounter;
        uint32 m_nFallbackJobsRunCounter;

        JobManager::SInfoBlock**                                    m_pRegularWorkerFallbacks;
        uint32                                                                      m_nRegularWorkerThreads;

        bool                                                                            m_bSuspendWorkerForMP;
        CryMutex                                                                    m_SuspendWorkerForMPLock;
        CryConditionVariable                                            m_SuspendWorkerForMPCondion;
#if defined(JOBMANAGER_SUPPORT_PROFILING)
        SJobProfilingDataContainer m_profilingData;
        std::map<JobManager::SJobStringHandle, ColorB> m_JobColors;
        std::map<SMarker::TMarkerString, ColorB> m_RegionColors;
        CTimeValue m_FrameStartTime[SJobProfilingDataContainer::nCapturedFrames];

#endif

        // singleton stuff
        CJobManager();
        // disable copy and assignment
        CJobManager(const CJobManager&);
        CJobManager& operator= (const CJobManager&);
    } _ALIGN(128);
}//JobManager


///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CJobManager::CopyJobParameter(const uint32 cJobParamSize, void* pDestParam, const void* pSrcParam)
{
    assert(IsAligned(cJobParamSize, 16) && "JobParameter Size needs to be a multiple of 16");
    assert(cJobParamSize <= JobManager::SInfoBlock::scAvailParamSize && "JobParameter Size larger than available storage");
    memcpy(pDestParam, pSrcParam, cJobParamSize);
}

///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CJobManager::IncreaseRunJobs()
{
    CryInterlockedIncrement((int volatile*)&m_nJobsRunCounter);
}

///////////////////////////////////////////////////////////////////////////////
inline void JobManager::CJobManager::IncreaseRunFallbackJobs()
{
    CryInterlockedIncrement((int volatile*)&m_nFallbackJobsRunCounter);
}

#endif //__JOBMAN_SPU_H
