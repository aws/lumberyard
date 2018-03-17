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
#include "JobWorkerThread.h"
#include "LogManager.h"
#include "JobManager.h"
#include "Job.h"
#include "JobList.h"

#include <chrono>

namespace MCore
{
    // constructor
    JobWorkerThread::JobWorkerThread(JobManager* jobManager, uint32 threadID)
    {
        mJobManager = jobManager;
        mThreadID   = threadID;
        mMustExit   = false;

        // reserve mem
        mFinishedJobs.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);
        mFinishedJobs.Reserve(4096);

        // init the thread
        AZStd::function<void()> func = AZStd::bind(&MCore::JobWorkerThread::MainFunction, this);
        mThread.Init(func);
    }


    // destructor
    JobWorkerThread::~JobWorkerThread()
    {
        DestroyFinishedItems();
    }


    // create a new thread
    JobWorkerThread* JobWorkerThread::Create(JobManager* jobManager, uint32 threadID)
    {
        return new JobWorkerThread(jobManager, threadID);
    }


    // inform that we want to exit
    void JobWorkerThread::Exit()
    {
        mMustExit = true;
    }


    // destroy all the jobs and lists that are finished
    void JobWorkerThread::DestroyFinishedItems()
    {
        // clear jobs
        for (uint32 i = 0; i < mFinishedJobs.GetLength(); ++i)
        {
            mFinishedJobs[i]->DestroyWithoutLock();
        }
        mFinishedJobs.Clear(false);
    }


    // wait until we have completed
    void JobWorkerThread::WaitUntilCompleted()
    {
        mThread.Join();
    }


    // add a finished job
    void JobWorkerThread::AddFinishedJob(Job* job)
    {
        mFinishedJobs.Add(job);
    }


    // the main function
    void JobWorkerThread::MainFunction()
    {
        //MCore::LogInfo("MCore::JobWorkerThread::MainFunction() - Entering thread with ID %d", mThreadID);

        // wait until we are ready to consume jobs
        mJobManager->GetStartJobCondition().Wait();

        // while we want to keep running this thread
        while (mMustExit == false)
        {
            // yield to other threads
            AZStd::this_thread::yield();

            // get the next job to execute
            JobList*    jobList = nullptr;
            Job*        job     = nullptr;
            mJobManager->GetNextJob(&job, &jobList);

            // if there is a job to execute, do it
            if (job)
            {
                job->SetThreadIndex(mThreadID);
                job->Execute();
                AddFinishedJob(job);
                jobList->IncreaseNumFinishedJobs();
            }
        }

        //MCore::LogInfo("MCore::JobWorkerThread::MainFunction() - Exiting thread with ID %d", mThreadID);
    }
}   // namespace MCore
