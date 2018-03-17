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

// include required headers
#include "JobManager.h"
#include "JobWorkerThread.h"
#include "JobList.h"
#include "Job.h"
#include "LogManager.h"


namespace MCore
{
    // constructor
    JobManager::JobManager()
    {
        mJobLists.Reserve(512);
        mJobLists.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);

        mFinishedJobLists.Reserve(512);
        mFinishedJobLists.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);

        const uint32 numThreads = MCore::Max<uint32>(1, AZStd::thread::hardware_concurrency());
        Init(numThreads);
    }


    // extended constructor
    JobManager::JobManager(uint32 numThreads)
    {
        mJobLists.Reserve(512);
        mJobLists.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);

        mFinishedJobLists.Reserve(512);
        mFinishedJobLists.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);

        //const uint32 realNumThreads = MCore::Max<uint32>( 1, numThreads );
        //Init( realNumThreads );

        Init(numThreads);
    }


    // destructor
    JobManager::~JobManager()
    {
        RemoveAllThreads();
    }


    // create
    JobManager* JobManager::Create()
    {
        return new JobManager();
    }


    // create and init on a given number of threads
    JobManager* JobManager::Create(uint32 numThreads)
    {
        return new JobManager(numThreads);
    }


    // remove all threads
    void JobManager::RemoveAllThreads()
    {
        // first wait until all jobs finished processing
        if (mThreads.GetLength() > 0)
        {
            WaitUntilAllJobsCompleted();
        }

        // try to exit and destroy the existing threads
        const uint32 numThreads = mThreads.GetLength();
        for (uint32 i = 0; i < numThreads; ++i)
        {
            mThreads[i]->Exit();
            mThreads[i]->WaitUntilCompleted();
        }

        // destroy all finished items
        DestroyFinishedThreadItems();

        // destroy the threads
        for (uint32 i = 0; i < numThreads; ++i)
        {
            MCore::Destroy(mThreads[i]);
        }
        mThreads.Clear();

        // destroy all jobs and lists
        DestroyJobsLists();

        // reset the conditions
        mStartJobCondition.Reset();
        //mJobsAvailableEvent.Reset();
        mAllCompletedEvent.NotifyAll();
    }


    // init the job manager
    void JobManager::Init(uint32 numThreads)
    {
        // remove existing threads
        RemoveAllThreads();
        /*
            // check if we use the MCore job system list processing
            // if not, don't create threads
            #if defined(MCORE_PLATFORM_WINDOWS) // gcc etc don't seem to recognise the std::function::target method, while it is part of the c++11 standard, maybe have to wait for compiler updates
                void (*const* ptr)(JobList*, bool, bool) = JobListExecuteFunc.target<void(*)(JobList*, bool, bool)>();
                if (ptr && *ptr != JobListExecuteMCoreJobSystem)
                    return;
            #endif
        */

        if (numThreads == 0)
        {
            return;
        }

        // create new ones
        mThreads.Reserve(numThreads);
        for (uint32 i = 0; i < numThreads; ++i)
        {
            JobWorkerThread* newThread = JobWorkerThread::Create(this, i);
            mThreads.Add(newThread);
        }
    }


    // set the number of threads
    void JobManager::SetNumThreads(uint32 numThreads)
    {
        Init(numThreads);
        StartAcceptingJobs();
    }


    // start accepting jobs
    void JobManager::StartAcceptingJobs()
    {
        mStartJobCondition.NotifyAll();
    }


    // get the start job condition
    ConditionEvent& JobManager::GetStartJobCondition()
    {
        return mStartJobCondition;
    }


    // add a job list to the joblist array
    void JobManager::AddJobList(JobList* jobList)
    {
        LockGuardRecursive lock(mMutex);
        if (jobList)
        {
            jobList->SetIndex(mJobLists.GetLength());
        }
        mJobLists.Add(jobList);
        //mJobsAvailableEvent.NotifyAll();
    }


    // get the number of threads
    uint32 JobManager::GetNumThreads() const
    {
        return mThreads.GetLength();
    }


    // get the next job to execute
    void JobManager::GetNextJob(Job** outJob, JobList** outJobList)
    {
        LockGuardRecursive lock(mMutex);

        // iterate over all job lists
        const uint32 numJobLists = mJobLists.GetLength();
        if (numJobLists == 0)
        {
            mAllCompletedEvent.NotifyAll();
        }
        else
        {
            mAllCompletedEvent.Reset();
        }

        // iterate over all job lists
        for (uint32 i = 0; i < mJobLists.GetLength(); )
        {
            JobList* jobList = mJobLists[i];

            // skip finished job lists
            if (jobList && jobList->GetIsFinished())
            {
                if (jobList->GetNumWaiting() == 0)
                {
                    mFinishedJobLists.Add(jobList);
                    mJobLists.Remove(i);
                }
                else
                {
                    //LogInfo("JobList %d (0x%x) is finished", jobList->GetIndex(), jobList);
                    jobList->TriggerFinishedEvent();
                    i++;
                }
                continue;
            }

            // if this is a sync point
            if (jobList == nullptr)
            {
                if (i == 0)
                {
                    mJobLists.RemoveFirst();
                }

                *outJob     = nullptr;
                *outJobList = nullptr;
                return;
            }

            // if we are syncing inside a joblist, go to the next joblist if we're not ready to continue inside this one yet
            uint32 syncPointIndex = jobList->GetSyncPointIndex();
            if (syncPointIndex != MCORE_INVALIDINDEX32 && jobList->GetNumFinishedJobs() <= syncPointIndex)
            {
                i++;
                continue;
            }

            // get the next job in the list
            Job* job = jobList->GetNextJob();
            if (job && job->GetJobTypeID() != Job::JOBTYPE_SYNC) // if it is a regular job (not a sync job)
            {
                *outJob     = job;
                *outJobList = jobList;
                jobList->SetSyncPointIndex(MCORE_INVALIDINDEX32);
                jobList->IncreaseNumStartedJobs();
                return;
            }
            else
            {
                *outJob     = job;
                *outJobList = jobList;
                if (job)
                {
                    jobList->SetSyncPointIndex(static_cast<int32>(jobList->GetNumStartedJobs()));
                }
            }

            // if we found a job return
            if (*outJob && *outJobList)
            {
                jobList->IncreaseNumStartedJobs();
                return;
            }

            i++;
        }

        // nothing to execute
        //mJobsAvailableEvent.Reset();
        *outJob     = nullptr;
        *outJobList = nullptr;
    }


    // add a sync point
    void JobManager::AddSyncPoint()
    {
        AddJobList(nullptr);
    }

    /*
    // wait for jobs to be available
    void JobManager::WaitUntilJobsAvailable()
    {
        mJobsAvailableEvent.WaitWithTimeout(10);
    }
    */

    // get the all completed event
    ConditionEvent& JobManager::GetAllCompletedEvent()
    {
        return mAllCompletedEvent;
    }


    // wait until all jobs got completed
    void JobManager::WaitUntilAllJobsCompleted()
    {
        mAllCompletedEvent.Wait();
    }


    // next frame in the job manager
    void JobManager::NextFrame()
    {
        // wait until all jobs are finished
        WaitUntilAllJobsCompleted();

        // lock and check that we are really done
        LockGuardRecursive lock(mMutex);

        // destroy the job lists
        DestroyJobsLists();

        // destroy the finished items (stored inside the threads)
        DestroyFinishedThreadItems();
    }


    // destroy the job lists
    void JobManager::DestroyJobsLists()
    {
        // destroy the open job lists
        const uint32 numJobLists = mJobLists.GetLength();
        for (uint32 i = 0; i < numJobLists; ++i)
        {
            if (mJobLists[i])
            {
                mJobLists[i]->DestroyWithoutLock();
            }
        }
        mJobLists.Clear(false);

        const uint32 numFinishedJobLists = mFinishedJobLists.GetLength();
        for (uint32 i = 0; i < numFinishedJobLists; ++i)
        {
            if (mFinishedJobLists[i])
            {
                mFinishedJobLists[i]->DestroyWithoutLock();
            }
        }
        mFinishedJobLists.Clear(false);
    }


    // destroy all finished thread items
    void JobManager::DestroyFinishedThreadItems()
    {
        // destroy the finished job lists
        for (uint32 i = 0; i < mThreads.GetLength(); ++i)
        {
            mThreads[i]->DestroyFinishedItems();
        }
    }


    // lock the manager
    void JobManager::Lock()
    {
        mMutex.Lock();
    }


    // unlock the manager
    void JobManager::Unlock()
    {
        mMutex.Unlock();
    }


    // stop processing jobs
    void JobManager::StopProcessingJobs()
    {
        RemoveAllThreads();
    }


    // check if we are running or not
    bool JobManager::GetIsRunning() const
    {
        return (mThreads.GetLength() > 0);
    }


    // check if we stopped or not
    bool JobManager::GetIsStopped() const
    {
        return (mThreads.GetLength() == 0);
    }
}   // namespace MCore
