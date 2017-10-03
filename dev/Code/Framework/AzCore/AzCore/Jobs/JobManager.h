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

// The work stealing imlementation is the fastest. It requires thread local storage and atomics, needs platform to support xchg 128 bit
#if defined(AZ_THREAD_LOCAL) && (defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE_OSX)) // ACCEPTED_USE
#   define AZCORE_JOBS_IMPL_WORK_STEALING
#elif defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
// Thread local storage was only added to iOS in Xcode 8, and ARM processors do not support xchg128 bit instructions, so we can't use the work stealing implementation.
#   define AZCORE_JOBS_IMPL_DEFAULT
#else
#   define AZCORE_JOBS_IMPL_SYNCHRONOUS
#endif

//#define JOBMANAGER_ENABLE_STATS

#if defined(AZCORE_JOBS_IMPL_WORK_STEALING)
    #include <AzCore/Jobs/Internal/JobManagerWorkStealing.h>
#elif defined(AZCORE_JOBS_IMPL_DEFAULT)
    #include <AzCore/Jobs/Internal/JobManagerDefault.h>
#elif defined(AZCORE_JOBS_IMPL_SYNCHRONOUS)
    #include <AzCore/Jobs/Internal/JobManagerSynchronous.h>
#endif

namespace AZ
{
    class Job;
    namespace Internal {
        class JobNotify;
    }

    /**
     * JobManager continuously runs jobs as they become available. To create and run jobs use the Job class, JobManager
     * should only be used to manage the threads which run the jobs.
     */
    class JobManager
    {
    public:
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
        unsigned int GetNumWorkerThreads() const    { return m_impl.GetNumWorkerThreads(); }

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

        //called internally by Job class when a suspended job is now ready to be resumed
        AZ_FORCE_INLINE void NotifySuspendedJobReady(Job* job) { m_impl.NotifySuspendedJobReady(job); }

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
