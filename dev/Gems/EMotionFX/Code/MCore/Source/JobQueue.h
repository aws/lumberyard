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
#include "Array.h"


namespace MCore
{
    // forward declarations
    class Job;

    /**
     * A FIFO job queue (not thread safe).
     * This queue is used by the job system and stores the list of jobs to execute.
     */
    class MCORE_API JobQueue
    {
        MCORE_MEMORYOBJECTCATEGORY(JobQueue, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM);
    public:
        JobQueue();
        ~JobQueue();

        Job* GetNextJob();          // get the first item in the queue and remove it from the queue
        void AddJob(Job* job);      // add to the back of the queue
        void Clear(bool destroyJobs = true);

        uint32 GetNumJobs() const;
        uint32 FindFirstSyncJobIndex() const;

    private:
        Job*            mNext;          // the next job in the list
        Job*            mPrev;          // the previous job in the list
        Job*            mFirstJob;      // the first job
        Job*            mLastJob;       // the last job
        AtomicUInt32    mNumJobs;       // the number of jobs

        void RemoveJob(Job* job);   // does not del from mem
    };
}   // namespace MCore
