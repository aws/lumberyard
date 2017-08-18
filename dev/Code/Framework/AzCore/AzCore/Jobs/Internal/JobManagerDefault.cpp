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

#if defined(AZCORE_JOBS_IMPL_DEFAULT)

#include <AzCore/Jobs/Internal/JobManagerDefault.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/Internal/JobNotify.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/lock.h>


using namespace AZ;
using namespace Internal;

JobManagerDefault::JobManagerDefault()
{
    m_isKillingThreads = false;
    m_isAsynchronous = false;
}

JobManagerDefault::JobManagerDefault(const JobManagerDesc& desc)
{
    m_isKillingThreads = false;

    //need to set this first, before creating threads, which will check it
    m_isAsynchronous = (desc.m_workerThreads.size() > 0);

    // Create all worker threads.
    for (unsigned int iThread = 0; iThread < desc.m_workerThreads.size(); ++iThread)
    {
        AddThread(desc.m_workerThreads[iThread]);
    }
}

JobManagerDefault::~JobManagerDefault()
{
    KillThreads();
}

void JobManagerDefault::AddPendingJob(Job* job)
{
    AZ_Assert(job->GetDependentCount() == 0, ("Job has a non-zero ready count, it should not be being added yet"));

    if (IsAsynchronous())
    {
        //lock and push the job onto the queue
        AZStd::lock_guard<AZStd::mutex> lock(m_queueMutex);
        m_jobQueue.push(job);

        //wake up a worker thread to process the job
        m_condVar.notify_one(); //notify can only be called while holding the mutex
    }
    else
    {
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_queueMutex);
            m_jobQueue.push(job);
        }

        //no workers, so must process the jobs right now
        ThreadInfo* info = GetCurrentThreadInfo();
        if (!info->m_currentJob)  //unless we're already processing
        {
            ProcessJobsSynchronous(info, NULL, NULL);
        }
    }
}

void JobManagerDefault::SuspendJobUntilReady(Job* job)
{
    ThreadInfo* info = GetCurrentThreadInfo();
    AZ_Assert(info->m_currentJob == job, ("Can't suspend a job which isn't currently running"));

    info->m_currentJob = NULL; //clear current job

    if (IsAsynchronous())
    {
        ProcessJobsAssist(info, job, NULL);
    }
    else
    {
        ProcessJobsSynchronous(info, job, NULL);
    }

    info->m_currentJob = job; //restore current job
}

void JobManagerDefault::NotifySuspendedJobReady(Job* job)
{
    (void)job;
    //thread on which the job is suspended may be sleeping, we must wake up all waiting threads to make sure we get
    //the right one
    AZStd::unique_lock<AZStd::mutex> lock(m_queueMutex);
    m_condVar.notify_all();//notify can only be called while holding the mutex
}

void JobManagerDefault::StartJobAndAssistUntilComplete(Job* job)
{
    ThreadInfo* info = GetCurrentThreadInfo();
    AZ_Assert(!info->m_isWorker, "Can't assist using a worker thread");
    AZ_Assert(!info->m_currentJob, "Can't assist when a job is already processing on this thread");

    //We require the notify job, because the other job may auto-delete when it is complete.
    AZStd::atomic<bool> notifyFlag(false);
    Internal::JobNotify notifyJob(&notifyFlag, job->GetContext());
    job->SetDependent(&notifyJob);
    notifyJob.Start();
    job->Start();

    //the processing functions will return when the notify flag has been set
    if (IsAsynchronous())
    {
        ProcessJobsAssist(info, NULL, &notifyFlag);
    }
    else
    {
        ProcessJobsSynchronous(info, NULL, &notifyFlag);
    }

    AZ_Assert(notifyFlag.load(AZStd::memory_order_acquire), "");
}

Job* JobManagerDefault::GetCurrentJob()
{
    ThreadInfo* info = GetCurrentThreadInfo();
    return info->m_currentJob;
}

void JobManagerDefault::ProcessJobsWorker(ThreadInfo* info)
{
    ProcessJobsInternal(info, NULL, NULL);
}

void JobManagerDefault::ProcessJobsAssist(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    ProcessJobsInternal(info, suspendedJob, notifyFlag);
}

void JobManagerDefault::ProcessJobsInternal(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    AZ_Assert(IsAsynchronous(), "ProcessJobs is only to be used when we have worker threads (can be called on non-workers too though)");

    while (true)
    {
        //pop a job from the queue, blocking until one is available
        Job* job;
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queueMutex);

            //we must check this before blocking, and while we have acquired the lock, to ensure we don't miss a
            //notification
            if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
            {
                return;
            }

            while (m_jobQueue.empty())
            {
                m_condVar.wait(lock);

                //check if wakeup was due to suspended job being ready
                if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                    (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
                {
                    return;
                }

                if (m_isKillingThreads)
                {
                    return;
                }
            }

            job = m_jobQueue.front();
            m_jobQueue.pop();
        }

        info->m_currentJob = job;
        Process(job);
        info->m_currentJob = NULL;

        //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore
    }
}

void JobManagerDefault::ProcessJobsSynchronous(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    while (!m_jobQueue.empty())
    {
        Job* job = m_jobQueue.front();
        m_jobQueue.pop();

        info->m_currentJob = job;
        Process(job);
        info->m_currentJob = NULL;

        //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore

        //check if suspended job is now ready
        if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
            (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
        {
            return;
        }
    }
}

JobManagerDefault::ThreadInfo* JobManagerDefault::GetCurrentThreadInfo()
{
    //we could use tls to speed this up, but it's not available on all platforms
    AZStd::lock_guard<AZStd::mutex> lock(m_threadsMutex);
    AZStd::thread::id thisId = AZStd::this_thread::get_id();
    for (unsigned int i = 0; i < m_threads.size(); ++i)
    {
        if (m_threads[i]->m_threadId == thisId)
        {
            return m_threads[i];
        }
    }

    //must be a new user thread, allocate a new info
    ThreadInfo* info = aznew ThreadInfo;
    info->m_threadId = thisId;
    info->m_isWorker = false;
    info->m_currentJob = NULL;
    m_threads.push_back(info);

    return info;
}

void JobManagerDefault::AddThread(const JobManagerThreadDesc& desc)
{
    ThreadInfo* info = aznew ThreadInfo;

    AZStd::thread_desc threadDesc;
    threadDesc.m_name = "AZ JobManager worker thread";
    threadDesc.m_cpuId = desc.m_cpuId;
    threadDesc.m_priority = desc.m_priority;
    if (desc.m_stackSize != 0)
    {
        threadDesc.m_stackSize = desc.m_stackSize;
    }
    info->m_thread = AZStd::thread(AZStd::bind(&JobManagerDefault::ProcessJobsWorker, this, info), &threadDesc);
    info->m_threadId = info->m_thread.get_id();
    info->m_isWorker = true;
    info->m_currentJob = NULL;

    m_threads.push_back(info);
}

void JobManagerDefault::KillThreads()
{
    if (!m_threads.empty())
    {
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_queueMutex);
            m_isKillingThreads = true;
            //note that using notify_all is not sufficient. We need to ensure all threads get a wakeup event, but
            //notify_all will only wake the threads currently sleeping
            for (unsigned int i = 0; i < m_threads.size(); ++i)
            {
                m_condVar.notify_one();
            }
        }

        for (unsigned int i = 0; i < m_threads.size(); ++i)
        {
            if (m_threads[i]->m_isWorker)
            {
                m_threads[i]->m_thread.join();
            }
        }

        for (unsigned int i = 0; i < m_threads.size(); ++i)
        {
            delete m_threads[i];
        }

        m_threads.clear();
    }
}

#endif // defined(AZCORE_JOBS_IMPL_DEFAULT)

#endif // #ifndef AZ_UNITY_BUILD