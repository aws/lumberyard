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
#ifndef AZCORE_JOBS_INTERNAL_JOBMANAGER_SYNCHRONOUS_H
#define AZCORE_JOBS_INTERNAL_JOBMANAGER_SYNCHRONOUS_H 1

// Inluded from JobManager.h
#if defined(AZCORE_JOBS_IMPL_SYNCHRONOUS)

#include <AzCore/Jobs/Internal/JobManagerBase.h>
#include <AzCore/Jobs/JobManagerDesc.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/std/containers/queue.h>

namespace AZ
{
    class Job;

    namespace Internal
    {
        /**
         * A synchronous implementation of a job manager, all jobs are ran immediately when they are added, does not
         * use any threads or synchronization primitives. Useful as a quick implementation when the target platform
         * is single core anyway.
         */
        class JobManagerSynchronous
            : public JobManagerBase
        {
        public:
            JobManagerSynchronous(const JobManagerDesc& desc);
            ~JobManagerSynchronous();

            AZ_FORCE_INLINE bool IsAsynchronous() const { return false; }

            void AddPendingJob(Job* job);

            void SuspendJobUntilReady(Job* job);

            void NotifySuspendedJobReady(Job* job);

            void StartJobAndAssistUntilComplete(Job* job);

            void ClearStats() {}
            void PrintStats() {}

            void CollectGarbage() { }

            Job* GetCurrentJob() { return m_currentJob; }

            unsigned int GetNumWorkerThreads() const { return 1; }

        private:
            typedef AZStd::queue<Job*> JobQueue;

            JobQueue m_jobQueue;
            Job* m_currentJob;

            void ProcessJobs(Job* suspendedJob);
        };
    }
}
#endif // #if defined(AZCORE_JOBS_IMPL_SYNCHRONOUS)

#endif
#pragma once