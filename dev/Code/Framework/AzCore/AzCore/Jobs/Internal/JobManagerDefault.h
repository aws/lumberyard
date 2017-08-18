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
#ifndef AZCORE_JOBS_INTERNAL_JOBMANAGER_DEFAULT_H
#define AZCORE_JOBS_INTERNAL_JOBMANAGER_DEFAULT_H 1

// Inluded from JobManager.h

#if defined(AZCORE_JOBS_IMPL_DEFAULT)

#include <AzCore/Jobs/Internal/JobManagerBase.h>
#include <AzCore/Jobs/JobManagerDesc.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>

namespace AZ
{
    class Job;

    namespace Internal
    {
        /**
         * A reference implementation of a job manager, uses only locks and condition variables.
         */
        class JobManagerDefault
            : public JobManagerBase
        {
        public:
            JobManagerDefault();
            JobManagerDefault(const JobManagerDesc& desc);
            ~JobManagerDefault();

            AZ_FORCE_INLINE bool IsAsynchronous() const { return m_isAsynchronous; }

            void AddPendingJob(Job* job);

            void SuspendJobUntilReady(Job* job);

            void NotifySuspendedJobReady(Job* job);

            void StartJobAndAssistUntilComplete(Job* job);

            void ClearStats() { }
            void PrintStats() { }

            void CollectGarbage() { }

            Job* GetCurrentJob();

            unsigned int GetNumWorkerThreads() const
            {
                return static_cast<unsigned int>(m_threads.size());
            }

        protected:
            typedef AZStd::queue<Job*, AZStd::deque<Job*> > JobQueue;

            JobQueue m_jobQueue;
            AZStd::mutex m_queueMutex;
            AZStd::condition_variable m_condVar;

            struct ThreadInfo
            {
                AZ_CLASS_ALLOCATOR(ThreadInfo, ThreadPoolAllocator, 0)

                AZStd::thread::id m_threadId;
                bool m_isWorker;
                AZStd::thread m_thread; //valid for workers only, we own the workers
                Job* m_currentJob; //job which is currently processing on this thread
            };

            void ProcessJobsWorker(ThreadInfo* info);
            void ProcessJobsAssist(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag);
            void ProcessJobsInternal(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag);
            void ProcessJobsSynchronous(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag);
            void AddThread(const JobManagerThreadDesc& desc);
            void KillThreads();
            ThreadInfo* GetCurrentThreadInfo();

            typedef AZStd::vector<ThreadInfo*> ThreadList;
            ThreadList m_threads;
            AZStd::mutex m_threadsMutex;

            bool m_isAsynchronous;
            bool m_isKillingThreads;
        };
    }
}

#endif // defined(AZCORE_JOBS_IMPL_DEFAULT)

#endif
#pragma once