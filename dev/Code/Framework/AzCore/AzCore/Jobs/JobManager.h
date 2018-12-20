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
#ifndef AZCORE_JOBS_JOBMANAGER_H
#define AZCORE_JOBS_JOBMANAGER_H 1

#include <AzCore/base.h>
#include <AzCore/Jobs/JobManagerDesc.h>
#include <AzCore/Memory/Memory.h>

// Other job manager implementation could be chosen here on a per-platform basis.  The work stealing implementation requires thread local storage and atomics (pointer-sized, 32-bit)
#if defined(AZ_THREAD_LOCAL)
#   define AZCORE_JOBS_IMPL_WORK_STEALING
#   include <AzCore/Jobs/Internal/JobManagerWorkStealing.h>
#else
#   error Thread local storage support required for JobManagerWorkStealing
#endif

//#define JOBMANAGER_ENABLE_STATS

namespace AZ
{
    class Job;
    namespace Internal {
        class JobNotify;
    }

    /**
     * JobManager continuously runs jobs as they become available. To create and run jobs use the Job class, JobManager
     * should only be used to manage the threads which run the jobs.
     *
     * The number of JobThreads created is controlled by JobManagerDesc.
     * Note: as a debugging tool, one can temporarily force a JobContext to run all jobs synchronously by altering the provided JobManagerDesc to contain zero JobManagerThreadDesc.
     * This is best done done on a per-JobContext basis.  The default JobContect is created by the JobManagerComponent.
     */
    class JobManager
    {
    public:
        /**
        * Value returned by GetWorkerThreadId if the calling thread is not a worker thread in this JobContext.
        */
        static const AZ::u32 InvalidWorkerThreadId = Internal::JobManagerBase::InvalidWorkerThreadId;

        AZ_CLASS_ALLOCATOR_DECL

        JobManager(const JobManagerDesc& desc);

        ~JobManager();

        /// Check if we have multiple threads for parallel processing.
        AZ_FORCE_INLINE bool    IsAsynchronous() const { return m_impl.IsAsynchronous(); }

        /**
         * Clears all accumulated statistics, should only really be called when system is idle to ensure consistent
         * results.
         */
        void ClearStats() { m_impl.ClearStats(); }

        /**
         * Dumps accumulated statistics, should only really be called when system is idle to ensure consistent results.
         */
        void PrintStats() { m_impl.PrintStats(); }

        /**
         * Optionally call this to collect garbage from the work stealing deques, it absolutely *MUST* be only called
         * when the system is idle. The garbage is bounded, to 100% of the deque memory, so don't call it at all if
         * there is any doubt.
         */
        void CollectGarbage() { m_impl.CollectGarbage(); }

        /**
         * Returns the job which is currently running in this thread, if this is not a worker thread then NULL is
         * returned.
         */
        Job* GetCurrentJob() { return m_impl.GetCurrentJob(); }

        /// Returns number of active worker threads.
        AZ::u32 GetNumWorkerThreads() const { return m_impl.GetNumWorkerThreads(); }

        /// Returns 0 based worker index (for legacy Job compatibility)
        AZ::u32 GetWorkerThreadId() const { return m_impl.GetWorkerThreadId(); }

    private:
        //non-copyable
        JobManager(const JobManager& manager);
        JobManager& operator=(const JobManager& manager);

        //called internally by Job class when it becomes available to run
        friend class Job;
        friend class Internal::JobNotify;
        AZ_FORCE_INLINE void AddPendingJob(Job* job) { m_impl.AddPendingJob(job); }

        //called internally by Job class to suspend itself until child jobs are complete
        AZ_FORCE_INLINE void SuspendJobUntilReady(Job* job) { m_impl.SuspendJobUntilReady(job); }

        //called internally by Job class to start a job and then assist in processing until it is complete
        AZ_FORCE_INLINE void StartJobAndAssistUntilComplete(Job* job) { m_impl.StartJobAndAssistUntilComplete(job); }

#if defined(AZCORE_JOBS_IMPL_WORK_STEALING)
        Internal::JobManagerWorkStealing m_impl;
#elif defined(AZCORE_JOBS_IMPL_DEFAULT)
        Internal::JobManagerDefault m_impl;
#elif defined(AZCORE_JOBS_IMPL_SYNCHRONOUS)
        Internal::JobManagerSynchronous m_impl;
#endif
    };
}

#endif
#pragma once
