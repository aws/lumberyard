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
#include "Job.h"
#include "MCoreSystem.h"
#include "JobManager.h"
#include "JobPool.h"


namespace MCore
{
    // constructor
    Job::Job()
    {
        mJobFunction    = nullptr;
        mCustomData     = nullptr;
        mNext           = nullptr;
        mPrev           = nullptr;
        mListIndex      = MCORE_INVALIDINDEX32;
        mTypeID         = MCORE_INVALIDINDEX32;
        mThreadIndex    = MCORE_INVALIDINDEX32;
    }


    // destructor
    Job::~Job()
    {
    }


    // create a new job at a given memory location (used by the JobPool pooling system)
    Job* Job::Create(void* memLocation)
    {
        return new(memLocation) Job();
    }



    // create a job based on given parameters
    Job* Job::CreateWithoutLock(const JobFunction& func, uint32 jobTypeID, void* customData)
    {
        Job* result = GetJobManager().GetJobPool().RequestNewWithoutLock();

        result->SetJobFunction(func);
        result->SetJobTypeID(jobTypeID);
        result->SetCustomData(customData);

        return result;
    }


    // create a new job from the pool
    Job* Job::CreateWithoutLock()
    {
        return GetJobManager().GetJobPool().RequestNewWithoutLock();
    }


    // create a new job from the pool
    Job* Job::Create()
    {
        return GetJobManager().GetJobPool().RequestNew();
    }


    // create a job based on given parameters
    Job* Job::Create(const JobFunction& func, uint32 jobTypeID, void* customData)
    {
        Job* result = GetJobManager().GetJobPool().RequestNew();

        result->SetJobFunction(func);
        result->SetJobTypeID(jobTypeID);
        result->SetCustomData(customData);

        return result;
    }


    // destroy the job
    void Job::Destroy()
    {
        GetJobManager().GetJobPool().Free(this);
    }


    // destroy the job
    void Job::DestroyWithoutLock()
    {
        GetJobManager().GetJobPool().FreeWithoutLock(this);
    }


    // set the thread index
    void Job::SetThreadIndex(uint32 index)
    {
        mThreadIndex = index;
    }


    // execute the job
    void Job::Execute() const
    {
        if (!mJobFunction)
        {
            return;
        }

        mJobFunction(this);
    }
}   // namespace MCore
