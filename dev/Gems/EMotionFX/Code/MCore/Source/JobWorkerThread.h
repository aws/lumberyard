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


namespace MCore
{
    // forward declarations
    class JobManager;
    class Job;


    /**
     * The job worker thread, which is responsible for acquiring jobs from the queue and executing them.
     * Each core will have such thread running.
     */
    class MCORE_API JobWorkerThread
        : public MemoryObject
    {
        MCORE_MEMORYOBJECTCATEGORY(JobWorkerThread, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM);
    public:
        static JobWorkerThread* Create(JobManager* jobManager, uint32 threadID);

        void MainFunction();
        void Exit();                    // tell it to stop processing jobs and exit
        void WaitUntilCompleted();      // wait until we exited the thread
        void DestroyFinishedItems();    // destroy all the jobs and lists that are finished

    private:
        MCore::Array<Job*>  mFinishedJobs;
        JobManager*         mJobManager;
        Thread              mThread;
        uint32              mThreadID;
        bool                mMustExit;

        JobWorkerThread(JobManager* jobManager, uint32 threadID);
        ~JobWorkerThread();

        void AddFinishedJob(Job* job);  // register a finished job
    };
}   // namespace MCore
