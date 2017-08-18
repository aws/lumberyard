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

#ifdef AZCORE_JOBS_IMPL_WORK_STEALING

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/Internal/JobNotify.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/functional.h>

#ifdef JOBMANAGER_ENABLE_STATS
#   include <stdio.h>
#endif // JOBMANAGER_ENABLE_STATS

using namespace AZ;
using namespace Internal;


AZ_THREAD_LOCAL JobManagerWorkStealing::ThreadInfo* JobManagerWorkStealing::m_currentThreadInfo = NULL;

//AZ_CLASS_ALLOCATOR_IMPL(JobManagerWorkStealing,SystemAllocator,0)

JobManagerWorkStealing::JobManagerWorkStealing(const JobManagerDesc& desc)
{
    m_isQuit = false;

    m_numAvailableWorkers = 0;

    //need to set this first, before creating threads, which will check it
    m_isAsynchronous = (desc.m_workerThreads.size() > 0);

    // Create all worker threads.
    for (unsigned int iThread = 0; iThread < desc.m_workerThreads.size(); ++iThread)
    {
        AddThread(desc.m_workerThreads[iThread]);
    }

    //allow workers to begin processing after they have all been created, needed to wait since they may access each
    //others queues
    m_initSemaphore.release(static_cast<unsigned int>(desc.m_workerThreads.size()));
}

JobManagerWorkStealing::~JobManagerWorkStealing()
{
    KillThreads();
}

void JobManagerWorkStealing::AddPendingJob(Job* job)
{
    AZ_Assert(job->GetDependentCount() == 0, ("Job has a non-zero ready count, it should not be being added yet"));

    ThreadInfo* info = m_currentThreadInfo;
    if (info && info->m_isWorker)
    {
        //current thread is a worker, push to the local queue
        info->m_pendingJobs.local_push_bottom(job);
#ifdef JOBMANAGER_ENABLE_STATS
        ++info->m_jobsForked;
#endif
        // if there are threads asleep wake one up
        ActivateWorker();
    }
    else
    {
        //current thread is not a worker thread, push to the global queue
        {
            AZStd::lock_guard<GlobalQueueMutexType> lock(m_globalJobQueueMutex);
            m_globalJobQueue.push(job);
        }
        if (IsAsynchronous())
        {
            ActivateWorker();
        }
        else
        {
            //no workers, so must process the jobs right now
            if (!m_currentThreadInfo)  //unless we're already processing
            {
                ProcessJobsSynchronous(GetCurrentThreadInfo(), NULL, NULL);
            }
        }
    }
}

void JobManagerWorkStealing::SuspendJobUntilReady(Job* job)
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

void JobManagerWorkStealing::NotifySuspendedJobReady(Job* job)
{
    (void)job;
    //Nothing to do here, a thread which has a suspended job waiting will not go to sleep, so we don't need to send
    //any wake up events or anything. This means is possible for a thread to spin while waiting for a child to
    //complete if there are no other jobs available, but this should be fairly rare.
}

void JobManagerWorkStealing::StartJobAndAssistUntilComplete(Job* job)
{
    ThreadInfo* info = GetCurrentThreadInfo();
    AZ_Assert(!m_currentThreadInfo, "This thread is already assisting, you should use regular child jobs instead");
    AZ_Assert(!info->m_isWorker, "Can't assist using a worker thread");
    AZ_Assert(!info->m_currentJob, "Can't assist when a job is already processing on this thread");

    //We require the notify job, because the other job may auto-delete when it is complete.
    AZStd::atomic<bool> notifyFlag(false);
    Internal::JobNotify notifyJob(&notifyFlag, job->GetContext());
    job->SetDependent(&notifyJob);
    notifyJob.Start();
    job->Start();

    //the processing functions will return when the empty job dependent count has reached 1
    if (IsAsynchronous())
    {
        ProcessJobsAssist(info, NULL, &notifyFlag);
    }
    else
    {
        ProcessJobsSynchronous(info, NULL, &notifyFlag);
    }

    AZ_Assert(!m_currentThreadInfo, "");
    AZ_Assert(notifyFlag.load(AZStd::memory_order_acquire), "");
}

void JobManagerWorkStealing::ClearStats()
{
#ifdef JOBMANAGER_ENABLE_STATS
    for (unsigned int i = 0; i < m_threads.size(); ++i)
    {
        ThreadInfo* info = m_threads[i];
        info->m_globalJobs = 0;
        info->m_jobsForked = 0;
        info->m_jobsDone = 0;
        info->m_jobsStolen = 0;
        info->m_jobTime = 0;
        info->m_stealTime = 0;
    }
#endif
}

void JobManagerWorkStealing::PrintStats()
{
#ifdef JOBMANAGER_ENABLE_STATS
    char str[256];
    OutputDebugString("===================================================\n");
    OutputDebugString("Job System Stats:\n");
    OutputDebugString("Thread   Global jobs    Forks/dependents   Jobs done   Jobs stolen    Job time (ms)  Steal time (ms)  Total time (ms)\n");
    OutputDebugString("------   -------------  -----------------  ----------  ------------   -------------  ---------------  ---------------\n");
    for (unsigned int i = 0; i < m_threads.size(); ++i)
    {
        ThreadInfo* info = m_threads[i];
        double jobTime = 1000.0f * static_cast<double>(info->m_jobTime) / AZStd::GetTimeTicksPerSecond();
        double stealTime = 1000.0f * static_cast<double>(info->m_stealTime) / AZStd::GetTimeTicksPerSecond();
        azsnprintf(str, AZ_ARRAY_SIZE(str),  " %d:        %5d          %5d           %5d         %5d            %3.2f           %3.2f         %3.2f\n",
            i, info->m_globalJobs, info->m_jobsForked, info->m_jobsDone, info->m_jobsStolen, jobTime, stealTime, jobTime + stealTime);
        OutputDebugString(str);
        printf(str);
    }
#endif
}

void JobManagerWorkStealing::CollectGarbage()
{
    //spin until all worker threads are sleeping, that's the only safe time to collect the garbage
    while (m_numAvailableWorkers.load(AZStd::memory_order_acquire) == m_workerThreads.size())
    {
    }

    for (unsigned int i = 0; i < m_workerThreads.size(); ++i)
    {
        m_workerThreads[i]->m_pendingJobs.collect_garbage();
    }
}

void JobManagerWorkStealing::ProcessJobsWorker(ThreadInfo* info)
{
    //block until all workers are created, we don't want to steal from a thread that hasn't been created yet
    m_initSemaphore.acquire();

    //setup thread-local storage
    m_currentThreadInfo = info;

    ProcessJobsInternal(info, NULL, NULL);

    m_currentThreadInfo = NULL;
}

void JobManagerWorkStealing::ProcessJobsAssist(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    ThreadInfo* oldInfo = m_currentThreadInfo;
    m_currentThreadInfo = info;

    ProcessJobsInternal(info, suspendedJob, notifyFlag);

    m_currentThreadInfo = oldInfo; //restore previous ThreadInfo, necessary as must be NULL when returning to user code
}

void JobManagerWorkStealing::ProcessJobsInternal(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    AZ_Assert(IsAsynchronous(), "ProcessJobs is only to be used when we have worker threads (can be called on non-workers too though)");

    //get thread local job queue
    WorkQueue* pendingJobs = info->m_isWorker ? &info->m_pendingJobs : NULL;
    unsigned int victim = 0;

    while (true)
    {
        //check if suspended job is ready, before we try to get a new job
        if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
            (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
        {
            return;
        }

        //Try to get initial job.
        Job* job = NULL;
        {
            //only sleep if this thread is a worker and does not have a suspended job
            if (info->m_isWorker && !suspendedJob && m_globalJobQueue.empty())
            {
                if (m_isQuit)
                {
                    return;
                }

                bool wasAvailable = info->m_isAvailable.exchange(true, AZStd::memory_order_acq_rel);
                (void)wasAvailable;
                AZ_Assert(!wasAvailable, "available flag should have been false as we are processing jobs!");

                //going to sleep, increment the sleep counter. This is not synchronized properly, but it doesn't have to be
                // since we are only using it as a guess to optimize some stuff
                m_numAvailableWorkers.fetch_add(1, AZStd::memory_order_acq_rel);

                //block, quit thread if we get a kill event
                info->m_waitEvent.acquire();

                if (m_isQuit)
                {
                    return;
                }
            }

            //check if suspended job is ready, before we try to get a new job
            if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
            {
                return;
            }

            {
                AZStd::lock_guard<GlobalQueueMutexType> lock(m_globalJobQueueMutex);
                if (!m_globalJobQueue.empty())
                {
                    job = m_globalJobQueue.front();
                    m_globalJobQueue.pop();
#ifdef JOBMANAGER_ENABLE_STATS
                    ++info->m_globalJobs;
#endif
                }
            }
        }

        if (!job)
        {
            //nothing on the global queue, try to pop from the local queue
            if (pendingJobs && !pendingJobs->local_pop_bottom(&job))
            {
                job = NULL;
            }
        }

        bool isTerminated = false;
        while (!isTerminated)
        {
#ifdef JOBMANAGER_ENABLE_STATS
            AZStd::sys_time_t jobStartTime = AZStd::GetTimeNowTicks();
#endif
            //run current job and jobs from the local queue until it is empty
            while (job)
            {
                info->m_currentJob = job;
                Process(job);
                info->m_currentJob = NULL;

                //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore
#ifdef JOBMANAGER_ENABLE_STATS
                ++info->m_jobsDone;
#endif
                //check if our suspended job is ready, before we try running a new job
                if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                    (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
                {
                    return;
                }

                //pop a new job from the local queue
                if (pendingJobs && pendingJobs->local_pop_bottom(&job))
                {
                    //not necessary, just an optimization - wakeup sleeping threads, there's work to be done
                    ActivateWorker();
                }
                else
                {
                    job = NULL;
                }
            }

#ifdef JOBMANAGER_ENABLE_STATS
            AZStd::sys_time_t jobEndTime = AZStd::GetTimeNowTicks();
            info->m_jobTime += jobEndTime - jobStartTime;
#endif

            //steal a job from another threads queue
            unsigned int numStealAttempts = 0;
            const unsigned int maxStealAttempts = (unsigned int)m_workerThreads.size() * 3; //try every thread a few times before giving up
            while (!job)
            {
                //check if our suspended job is ready, before we try stealing a new job
                if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                    (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
                {
                    return;
                }

                //select a victim thread, using the same victim as the previous successful steal if possible
                WorkQueue* victimQueue = &m_workerThreads[victim]->m_pendingJobs;

                //attempt the steal
                if (victimQueue->steal_top(&job))
                {
                    //success, continue with the stolen job
#ifdef JOBMANAGER_ENABLE_STATS
                    ++info->m_jobsStolen;
#endif
                    break;
                }
                else
                {
                    job = NULL;
                }

                //steal failed, choose a new victim for next time
                ++victim;
                if ((victim < m_workerThreads.size()) && (m_workerThreads[victim] == info))  //don't steal from ourselves
                {
                    ++victim;
                }
                if (victim >= m_workerThreads.size())
                {
                    victim = 0;
                }

                ++numStealAttempts;
                if (numStealAttempts > maxStealAttempts)
                {
                    //Time to give up, it's likely all the local queues are empty. Note that this does not mean all the jobs
                    // are done, some jobs may be in progress, or we may have had terrible luck with our steals. There may be
                    // more jobs coming, another worker could create many new jobs right now. But the only way this thread
                    // will get a new job is from the global queue or by a steal, so we're going to sleep until a new job is
                    // queued.
                    // The important thing to note is that all jobs will be processed, even if this thread goes to sleep while
                    // jobs are pending.

                    job = NULL;
                    isTerminated = true;
                    break;
                }
            }

#ifdef JOBMANAGER_ENABLE_STATS
            info->m_stealTime += AZStd::GetTimeNowTicks() - jobEndTime;
#endif
        }
    }
}

void JobManagerWorkStealing::ProcessJobsSynchronous(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    AZ_Assert(!IsAsynchronous(), "ProcessJobsSynchronous should not be used when we have worker threads");

    ThreadInfo* oldInfo = m_currentThreadInfo;
    m_currentThreadInfo = info;

    while (!m_globalJobQueue.empty())
    {
        Job* job = m_globalJobQueue.front();
        m_globalJobQueue.pop();

        info->m_currentJob = job;
        Process(job);
        info->m_currentJob = NULL;

        //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore
#ifdef JOBMANAGER_ENABLE_STATS
        ++info->m_jobsDone;
#endif

        if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
            (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
        {
            return;
        }
    }

    m_currentThreadInfo = oldInfo; //restore previous ThreadInfo, necessary as must be NULL when returning to user code
}

JobManagerWorkStealing::ThreadInfo* JobManagerWorkStealing::GetCurrentThreadInfo()
{
    ThreadInfo* info = m_currentThreadInfo;
    if (!info)
    {
        //Search existing thread list for this thread.
        //We only set m_currentThreadInfo for user threads when jobs are processing on the user thread, as otherwise
        //it's impossible to clean it up as we don't control the lifetime of the user thread

        AZStd::lock_guard<AZStd::mutex> lock(m_threadsMutex);

        AZStd::thread::id thisId = AZStd::this_thread::get_id();
        for (unsigned int i = 0; i < m_threads.size(); ++i)
        {
            if (m_threads[i]->m_threadId == thisId)
            {
                info = m_threads[i];
                break;
            }
        }

        if (!info)
        {
            info = aznew ThreadInfo;
            info->m_isWorker = false;
            info->m_threadId = AZStd::this_thread::get_id();
            info->m_currentJob = nullptr;

#ifdef JOBMANAGER_ENABLE_STATS
            info->m_globalJobs = 0;
            info->m_jobsForked = 0;
            info->m_jobsDone = 0;
            info->m_jobsStolen = 0;
            info->m_jobTime = 0;
            info->m_stealTime = 0;
#endif
            m_threads.push_back(info);
        }
    }

    return info;
}

void JobManagerWorkStealing::AddThread(const JobManagerThreadDesc& desc)
{
    ThreadInfo* info = aznew ThreadInfo;

    info->m_isWorker = true;
    info->m_currentJob = nullptr;
    info->m_isAvailable = false;

    AZStd::thread_desc threadDesc;
    threadDesc.m_name = "AZ JobManager worker thread";
    threadDesc.m_cpuId = desc.m_cpuId;
    threadDesc.m_priority = desc.m_priority;
    if (desc.m_stackSize != 0)
    {
        threadDesc.m_stackSize = desc.m_stackSize;
    }
    info->m_thread = AZStd::thread(AZStd::bind(&JobManagerWorkStealing::ProcessJobsWorker, this, info), &threadDesc);
    info->m_threadId = info->m_thread.get_id();

#ifdef JOBMANAGER_ENABLE_STATS
    info->m_globalJobs = 0;
    info->m_jobsForked = 0;
    info->m_jobsDone = 0;
    info->m_jobsStolen = 0;
    info->m_jobTime = 0;
    info->m_stealTime = 0;
#endif

    {
        AZStd::lock_guard<AZStd::mutex> lock(m_threadsMutex);
        m_threads.push_back(info);
        m_workerThreads.push_back(info);
    }
}

void JobManagerWorkStealing::KillThreads()
{
    if (!m_threads.empty())
    {
        //kill worker threads
        if (!m_workerThreads.empty())
        {
            m_isQuit = true;

            for (unsigned int i = 0; i < m_workerThreads.size(); ++i)
            {
                m_workerThreads[i]->m_waitEvent.release();
                m_workerThreads[i]->m_thread.join();
            }
        }

        //cleanup all threads
        for (unsigned int i = 0; i < m_threads.size(); ++i)
        {
            delete m_threads[i];
        }

        m_threads.clear();
        m_workerThreads.clear();
    }
}

inline void JobManagerWorkStealing::ActivateWorker()
{
    // find an available worker thread (we do it brute force because the number of threads is small)
    if (m_numAvailableWorkers.load(AZStd::memory_order_acquire) > 0)
    {
        for (size_t i = 0; i < m_workerThreads.size(); ++i)
        {
            ThreadInfo* info = m_workerThreads[i];
            if (info->m_isAvailable.exchange(false, AZStd::memory_order_acq_rel) == true)
            {
                // decrement number of available workers
                m_numAvailableWorkers.fetch_sub(1, AZStd::memory_order_acq_rel);
                // resume the thread execution
                info->m_waitEvent.release();
                break;
            }
        }
    }
}

#endif // AZCORE_JOBS_IMPL_WORK_STEALING

#endif // #ifndef AZ_UNITY_BUILD