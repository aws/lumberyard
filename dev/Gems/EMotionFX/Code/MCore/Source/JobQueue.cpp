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
#include "JobQueue.h"
#include "Job.h"
#include "LogManager.h"


namespace MCore
{
    // constructor
    JobQueue::JobQueue()
    {
        mNext       = nullptr;
        mPrev       = nullptr;
        mFirstJob   = nullptr;
        mLastJob    = nullptr;
    }


    // destructor
    JobQueue::~JobQueue()
    {
        Clear();
    }


    // add a job
    void JobQueue::AddJob(Job* job)
    {
        if (mFirstJob)
        {
            MCORE_ASSERT(mLastJob);
            job->mPrev      = mLastJob;
            mLastJob->mNext = job;
            job->mNext      = nullptr;
            mLastJob        = job;
        }
        else // first job we add
        {
            MCORE_ASSERT(mLastJob == nullptr);
            mFirstJob   = job;
            mLastJob    = job;
        }

        mNumJobs.Increment();
    }


    // remove a job
    void JobQueue::RemoveJob(Job* job)
    {
        if (job->mNext)
        {
            job->mNext->mPrev = job->mPrev;
        }
        else
        {
            mLastJob = job->mPrev;
        }

        if (job->mPrev)
        {
            job->mPrev->mNext = job->mNext;
        }
        else
        {
            mFirstJob = job->mNext;
        }

        mNumJobs.Decrement();
    }


    // get the next job to execute
    Job* JobQueue::GetNextJob()
    {
        Job* result = mFirstJob;
        if (result)
        {
            RemoveJob(result);
        }

        return result;
    }


    // remove all remaining jobs from memory
    void JobQueue::Clear(bool destroyJobs)
    {
        Job* job = GetNextJob();
        while (job)
        {
            if (destroyJobs)
            {
                job->Destroy();
            }
            job = GetNextJob();
        }
        MCORE_ASSERT(mFirstJob == nullptr);
        MCORE_ASSERT(mLastJob == nullptr);
    }


    // get the number of remaining jobs in the list
    uint32 JobQueue::GetNumJobs() const
    {
        return mNumJobs.GetValue();
    }


    // find the next sync job index
    uint32 JobQueue::FindFirstSyncJobIndex() const
    {
        uint32 result = 0;
        Job* curJob = mFirstJob;
        if (curJob == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        while (curJob)
        {
            if (curJob->GetJobTypeID() == Job::JOBTYPE_SYNC)
            {
                return result;
            }

            curJob = curJob->GetNextJob();
            result++;
        }

        return MCORE_INVALIDINDEX32;
    }
}   // namespace MCore
