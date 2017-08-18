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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Jobs/JobManager.h>

#if defined(AZCORE_JOBS_IMPL_SYNCHRONOUS)

#include <AzCore/Jobs/Job.h>

using namespace AZ;
using namespace Internal;

JobManagerSynchronous::JobManagerSynchronous(const JobManagerDesc& desc)
{
    (void)desc;
    AZ_Warning("", desc.m_workerThreads.size() == 0, "JobManagerSynchronous does not support any worker threads, they will be ignored");
    m_currentJob = NULL;
}

JobManagerSynchronous::~JobManagerSynchronous()
{
}

void JobManagerSynchronous::AddPendingJob(Job* job)
{
    AZ_Assert(job->GetDependentCount() == 0, ("Job has a non-zero ready count, it should not be being added yet"));

    m_jobQueue.push(job);

    //no workers, so must process the jobs right now
    if (!m_currentJob)  //unless we're already processing
    {
        ProcessJobs(NULL);
    }
}

void JobManagerSynchronous::SuspendJobUntilReady(Job* job)
{
    AZ_Assert(m_currentJob == job, ("Can't suspend a job which isn't currently running"));

    m_currentJob = NULL; //clear current job

    ProcessJobs(job);
    AZ_Assert(job->GetDependentCount() == 0, "Suspended job is still not ready, and we have finished processing")

    m_currentJob = job; //restore current job
}

void JobManagerSynchronous::NotifySuspendedJobReady(Job* job)
{
    (void)job;
}

void JobManagerSynchronous::StartJobAndAssistUntilComplete(Job* job)
{
    AZ_Assert(!m_currentJob, "Can't assist when a job is already processing on this thread");

    //there's no need to do any processing here, just starting the job will make it run immediately
    job->Start();
}

void JobManagerSynchronous::ProcessJobs(Job* suspendedJob)
{
    while (!m_jobQueue.empty())
    {
        Job* job = m_jobQueue.front();
        m_jobQueue.pop();

        m_currentJob = job;
        Process(job);
        m_currentJob = NULL;

        //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore

        //check if suspended job is now ready
        if (suspendedJob && (suspendedJob->GetDependentCount() == 0))
        {
            return;
        }
    }
}
#endif // AZCORE_JOBS_IMPL_SYNCHRONOUS

#endif // #ifndef AZ_UNITY_BUILD