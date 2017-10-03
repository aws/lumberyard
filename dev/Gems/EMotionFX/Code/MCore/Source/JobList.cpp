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
#include "JobList.h"
#include "JobManager.h"
#include "Job.h"
#include "LogManager.h"


namespace MCore
{
    // constructor
    JobList::JobList()
    {
        mIndex = MCORE_INVALIDINDEX32;
        mSyncPointIndex.SetValue(MCORE_INVALIDINDEX32);
    }


    // destructor
    JobList::~JobList()
    {
        Clear();
    }


    // create the list at a given memory location
    JobList* JobList::Create(void* memLocation)
    {
        return new(memLocation) JobList();
    }


    // create without a lock
    JobList* JobList::CreateWithoutLock()
    {
        return GetJobManager().GetJobListPool().RequestNewWithoutLock();
    }


    // create with lock
    JobList* JobList::Create()
    {
        return GetJobManager().GetJobListPool().RequestNew();
    }


    // destroy the job list
    void JobList::Destroy()
    {
        GetJobManager().GetJobListPool().Free(this);
    }


    // destroy without lock
    void JobList::DestroyWithoutLock()
    {
        GetJobManager().GetJobListPool().FreeWithoutLock(this);
    }


    // add a job
    void JobList::AddJob(Job* job)
    {
        // TODO: make this thread safe?
        if (job)
        {
            job->SetListIndex(mJobQueue.GetNumJobs());
            mJobQueue.AddJob(job);
        }
        mNumTotalJobs.Increment();
    }


    // get the next job
    Job* JobList::GetNextJob()
    {
        return mJobQueue.GetNextJob();
    }


    // clear the list
    void JobList::Clear()
    {
        mJobQueue.Clear(false);
    }

    // get the number of remaining jobs in the list
    uint32 JobList::GetNumJobs() const
    {
        return mJobQueue.GetNumJobs();
    }


    uint32 JobList::GetNumStartedJobs() const
    {
        return mNumStartedJobs.GetValue();
    }


    // increase the number of finished jobs
    void JobList::IncreaseNumFinishedJobs()
    {
        mNumFinishedJobs.Increment();
    }


    // increase the number of started jobs
    void JobList::IncreaseNumStartedJobs()
    {
        mNumStartedJobs.Increment();
    }


    // check if this list is finished
    bool JobList::GetIsFinished() const
    {
        return (mNumFinishedJobs.GetValue() == mNumTotalJobs.GetValue());
    }


    // trigger the finished event
    void JobList::TriggerFinishedEvent()
    {
        //mFinishedEvent.NotifyAll();
        mFinishedCV.NotifyAll();
    }


    // wait until this listl is finished
    void JobList::WaitUntilFinished()
    {
        mNumWaiting.Increment();
        mFinishedCV.Wait(mFinishedMutex, [this] { return GetIsFinished();
            });
        mNumWaiting.Decrement();
    }


    // set the sync point index
    void JobList::SetSyncPointIndex(int32 index)
    {
        mSyncPointIndex.SetValue(index);
    }


    // get the number of finished jobs
    uint32 JobList::GetNumFinishedJobs() const
    {
        return mNumFinishedJobs.GetValue();
    }


    // get the current sync point index
    uint32 JobList::GetSyncPointIndex() const
    {
        return mSyncPointIndex.GetValue();
    }


    // get the index
    uint32 JobList::GetIndex() const
    {
        return mIndex;
    }


    // set the index
    void JobList::SetIndex(uint32 index)
    {
        mIndex = index;
    }


    // add a sync point
    void JobList::AddSyncPoint()
    {
        Job* syncJob = Job::Create(nullptr, Job::JOBTYPE_SYNC);
        AddJob(syncJob);
    }


    // add a sync point
    void JobList::AddSyncPointWithoutLock()
    {
        Job* syncJob = Job::CreateWithoutLock(nullptr, Job::JOBTYPE_SYNC);
        AddJob(syncJob);
    }


    uint32 JobList::GetNumWaiting() const
    {
        return mNumWaiting.GetValue();
    }


    // get the first sync job index
    uint32 JobList::FindFirstSyncJobIndex() const
    {
        return mJobQueue.FindFirstSyncJobIndex();
    }
}   // namespace MCore
