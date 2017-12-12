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

#pragma once

// include required headers
#include "StandardHeaders.h"
#include "MultiThreadManager.h"
#include "MemoryObject.h"
#include "JobQueue.h"
#include "JobPool.h"
#include "JobListPool.h"


namespace MCore
{
    // include required headers
    class JobWorkerThread;


    /**
     * The multithreading job manager.
     * This is responsible for executing and scheduling jobs accross multiple cores.
     */
    class MCORE_API JobManager
        : public MemoryObject
    {
        MCORE_MEMORYOBJECTCATEGORY(JobManager, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM);
    public:
        static JobManager* Create();                    // init on hardware max num threads, does not accept jobs automatically
        static JobManager* Create(uint32 numThreads);   // init on number of threads, does not accept jobs automatically

        void SetNumThreads(uint32 numThreads);          // you have to call StartAcceptingJobs afterwards again
        uint32 GetNumThreads() const;
        void StartAcceptingJobs();
        void RemoveAllThreads();

        void AddJobList(JobList* jobList);
        void AddSyncPoint();
        void GetNextJob(Job** outJob, JobList** outJobList);
        //void WaitUntilJobsAvailable();
        void WaitUntilAllJobsCompleted();

        void DestroyJobsLists();
        void DestroyFinishedThreadItems();

        void StopProcessingJobs();  // requires an Init again to restart
        bool GetIsRunning() const;  // check if we are currently run job threads
        bool GetIsStopped() const;  // opposite of IsRunning

        void Lock();
        void Unlock();
        void NextFrame();

        ConditionEvent& GetStartJobCondition();
        ConditionEvent& GetAllCompletedEvent();

        MCORE_INLINE JobPool& GetJobPool()                          { return mJobPool; }
        MCORE_INLINE const JobPool& GetJobPool() const              { return mJobPool; }

        MCORE_INLINE JobListPool& GetJobListPool()                  { return mJobListPool; }
        MCORE_INLINE const JobListPool& GetJobListPool() const      { return mJobListPool; }

    private:
        Array<JobWorkerThread*>     mThreads;
        Array<JobList*>             mJobLists;
        Array<JobList*>             mFinishedJobLists;
        ConditionEvent              mStartJobCondition;
        //ConditionEvent                mJobsAvailableEvent;
        ConditionEvent              mAllCompletedEvent;
        JobPool                     mJobPool;
        JobListPool                 mJobListPool;
        MutexRecursive              mMutex;

        JobManager();
        JobManager(uint32 numThreads);
        ~JobManager();

        void Init(uint32 numThreads);
    };
}   // namespace MCore
