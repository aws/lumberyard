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
#include "JobQueue.h"


namespace MCore
{
    class MCORE_API JobList
    {
        friend class JobListPool;
        MCORE_MEMORYOBJECTCATEGORY(JobList, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM)
    public:
        static JobList* Create();
        static JobList* CreateWithoutLock();
        void Destroy();
        void DestroyWithoutLock();

        void Clear();
        void AddJob(Job* job);
        void AddSyncPoint();
        void AddSyncPointWithoutLock();

        Job* GetNextJob();
        uint32 GetNumJobs() const;
        uint32 GetNumFinishedJobs() const;
        uint32 GetNumStartedJobs() const;
        uint32 GetSyncPointIndex() const;
        uint32 GetIndex() const;
        uint32 GetNumWaiting() const;
        uint32 FindFirstSyncJobIndex() const;

        void IncreaseNumStartedJobs();
        void IncreaseNumFinishedJobs();
        bool GetIsFinished() const;
        void TriggerFinishedEvent();
        void WaitUntilFinished();
        void SetSyncPointIndex(int32 index);
        void SetIndex(uint32 id);

    private:
        JobQueue            mJobQueue;
        AtomicUInt32        mNumTotalJobs;      // includes sync points
        AtomicUInt32        mNumFinishedJobs;
        AtomicUInt32        mNumStartedJobs;
        AtomicUInt32        mSyncPointIndex;
        AtomicUInt32        mNumWaiting;
        ConditionVariable   mFinishedCV;
        Mutex               mFinishedMutex;
        uint32              mIndex;

        JobList();
        ~JobList();

        static JobList* Create(void* memLocation);
    };
}   // namespace MCore
