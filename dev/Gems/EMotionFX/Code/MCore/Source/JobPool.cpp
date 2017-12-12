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
#include "JobPool.h"
#include "Job.h"
#include "LogManager.h"


namespace MCore
{
    //--------------------------------------------------------------------------------------------------
    // class JobPool::SubPool
    //--------------------------------------------------------------------------------------------------

    // constructor
    JobPool::SubPool::SubPool()
        : mData(nullptr)
        , mNumJobs(0)
    {
    }


    // destructor
    JobPool::SubPool::~SubPool()
    {
        MCore::Free(mData);
        mData = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    // class JobPool::Pool
    //--------------------------------------------------------------------------------------------------

    // constructor
    JobPool::Pool::Pool()
    {
        mFreeList.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);
        mSubPools.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);
        mPoolType           = POOLTYPE_DYNAMIC;
        mData               = nullptr;
        mNumJobs            = 0;
        mNumUsedJobs        = 0;
        mSubPoolSize        = 0;
    }


    // destructor
    JobPool::Pool::~Pool()
    {
        if (mPoolType == POOLTYPE_STATIC)
        {
            MCore::Free(mData);
            mData = nullptr;
            mFreeList.Clear();
        }
        else
        if (mPoolType == POOLTYPE_DYNAMIC)
        {
            MCORE_ASSERT(mData == nullptr);

            // delete all subpools
            const uint32 numSubPools = mSubPools.GetLength();
            for (uint32 s = 0; s < numSubPools; ++s)
            {
                delete mSubPools[s];
            }
            mSubPools.Clear();

            mFreeList.Clear();
        }
        else
        {
            MCORE_ASSERT(false);
        }
    }



    //--------------------------------------------------------------------------------------------------
    // class JobPool
    //--------------------------------------------------------------------------------------------------

    // constructor
    JobPool::JobPool()
    {
        mPool = nullptr;
    }


    // destructor
    JobPool::~JobPool()
    {
        if (mPool && mPool->mNumUsedJobs > 0)
        {
            LogError("MCore::JobPool::~JobPool() - There are still %d unfreed jobs, please use the Free function in the JobPool to free them, just like you would delete the object.", mPool->mNumUsedJobs);
        }

        delete mPool;
    }


    // init the pool
    void JobPool::Init(uint32 numInitialJobs, EPoolType poolType, uint32 subPoolSize)
    {
        if (mPool)
        {
            LogWarning("MCore::JobPool::Init() - We have already initialized the pool, ignoring new init call.");
            return;
        }

        // check if we use numInitialJobs==0 with a static pool, as that isn't allowed of course
        if (poolType == POOLTYPE_STATIC && numInitialJobs == 0)
        {
            LogError("MCore::JobPool::Init() - The number of initial jobs cannot be 0 when using a static pool. Please set the dynamic parameter to true, or increase the value of numInitialJobs.");
            MCORE_ASSERT(false);
            return;
        }

        // create the subpool
        mPool = new Pool();
        mPool->mNumJobs         = numInitialJobs;
        mPool->mPoolType        = poolType;
        mPool->mSubPoolSize     = subPoolSize;

        // if we have a static pool
        if (poolType == POOLTYPE_STATIC)
        {
            mPool->mData    = (uint8*)MCore::Allocate(numInitialJobs * sizeof(Job), MCORE_MEMCATEGORY_JOBSYSTEM);// alloc space
            mPool->mFreeList.ResizeFast(numInitialJobs);
            for (uint32 i = 0; i < numInitialJobs; ++i)
            {
                void* memLocation = (void*)(mPool->mData + i * sizeof(Job));
                mPool->mFreeList[i] = memLocation;
            }
        }
        else // if we have a dynamic pool
        if (poolType == POOLTYPE_DYNAMIC)
        {
            mPool->mSubPools.Reserve(32);

            SubPool* subPool = new SubPool();
            subPool->mData              = (uint8*)MCore::Allocate(numInitialJobs * sizeof(Job), MCORE_MEMCATEGORY_JOBSYSTEM);// alloc space
            subPool->mNumJobs           = numInitialJobs;

            mPool->mFreeList.ResizeFast(numInitialJobs);
            for (uint32 i = 0; i < numInitialJobs; ++i)
            {
                mPool->mFreeList[i] = (void*)(subPool->mData + i * sizeof(Job));
            }

            mPool->mSubPools.Add(subPool);
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
        }
    }


    // return a new job
    Job* JobPool::RequestNewWithoutLock()
    {
        // check if we already initialized
        if (mPool == nullptr)
        {
            //LogInfo("MCore::JobPool::RequestNew() - We have not yet initialized the pool, initializing it to a dynamic pool");
            Init();
        }

        // if there is are free items left
        if (mPool->mFreeList.GetLength() > 0)
        {
            Job* result = Job::Create(mPool->mFreeList.GetLast());
            mPool->mFreeList.RemoveLast(); // remove it from the free list
            mPool->mNumUsedJobs++;
            return result;
        }

        // we have no more free attributes left
        if (mPool->mPoolType == POOLTYPE_DYNAMIC) // we're dynamic, so we can just create new ones
        {
            const uint32 numJobs = mPool->mSubPoolSize;
            mPool->mNumJobs += numJobs;

            SubPool* subPool = new SubPool();
            subPool->mData      = (uint8*)MCore::Allocate(numJobs * sizeof(Job), MCORE_MEMCATEGORY_JOBSYSTEM);// alloc space
            subPool->mNumJobs   = numJobs;

            const uint32 startIndex = mPool->mFreeList.GetLength();
            if (mPool->mFreeList.GetMaxLength() < mPool->mNumJobs)
            {
                mPool->mFreeList.Reserve(mPool->mNumJobs);
            }

            mPool->mFreeList.ResizeFast(startIndex + numJobs);
            for (uint32 i = 0; i < numJobs; ++i)
            {
                void* memAddress = (void*)(subPool->mData + i * sizeof(Job));
                mPool->mFreeList[i + startIndex] = memAddress;
            }

            mPool->mSubPools.Add(subPool);

            Job* result = Job::Create(mPool->mFreeList.GetLast());
            mPool->mFreeList.RemoveLast(); // remove it from the free list
            mPool->mNumUsedJobs++;
            return result;
        }
        else // we are static and ran out of free attributes
        if (mPool->mPoolType == POOLTYPE_STATIC)
        {
            LogError("MCore::JobPool::RequestNew() - There are no free jobs in the static pool. Please increase the size of the pool or make it dynamic when calling Init.");
            MCORE_ASSERT(false); // we ran out of free motion instances
            return nullptr;
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
            return nullptr;
        }
    }


    // free the job
    void JobPool::FreeWithoutLock(Job* job)
    {
        if (job == nullptr)
        {
            return;
        }

        if (mPool == nullptr)
        {
            LogError("MCore::JobPool::Free() - The pool has not yet been initialized, please call Init first.");
            MCORE_ASSERT(false);
            return;
        }

        // add it back to the free list
        job->~Job(); // call the destructor
        mPool->mFreeList.Add(job);
        mPool->mNumUsedJobs--;
    }


    // log the memory stats
    void JobPool::LogMemoryStats()
    {
        LockGuard lock(mLock);
        LogInfo("MCore::JobPool::LogMemoryStats() - Logging motion instance pool info");

        const uint32 numFree    = mPool->mFreeList.GetLength();
        uint32 numUsed          = mPool->mNumUsedJobs;
        uint32 memUsage         = 0;
        uint32 usedMemUsage     = 0;
        uint32 totalMemUsage    = 0;
        uint32 totalUsedInstancesMemUsage = 0;

        if (mPool->mPoolType == POOLTYPE_STATIC)
        {
            if (mPool->mNumJobs > 0)
            {
                memUsage = mPool->mNumJobs * sizeof(Job);
                usedMemUsage = numUsed * sizeof(Job);
            }
        }
        else
        if (mPool->mPoolType == POOLTYPE_DYNAMIC)
        {
            if (mPool->mNumJobs > 0)
            {
                memUsage = mPool->mNumJobs * sizeof(Job);
                usedMemUsage = numUsed * sizeof(Job);
            }
        }

        totalUsedInstancesMemUsage  += usedMemUsage;
        totalMemUsage += memUsage;
        totalMemUsage += sizeof(Pool);
        totalMemUsage += sizeof(uint32) * mPool->mFreeList.GetLength();

        LogInfo("Pool:");
        if (mPool->mPoolType == POOLTYPE_DYNAMIC)
        {
            LogInfo("   - Num SubPools:    %d", mPool->mSubPools.GetLength());
        }

        LogInfo("   - Num Jobs:              %d", mPool->mNumJobs);
        LogInfo("   - Num Free:              %d", numFree);
        LogInfo("   - Num Used:              %d", numUsed);
        LogInfo("   - PoolType:              %s", (mPool->mPoolType == POOLTYPE_STATIC) ? "Static" : "Dynamic");
        LogInfo("   - Total Mem:             %d bytes (%d k)", memUsage, memUsage / 1000);
        LogInfo("   - Used Mem:              %d (%d k)", totalUsedInstancesMemUsage, totalUsedInstancesMemUsage / 1000);
        LogInfo("   - Total Mem Usage:       %d (%d k)", totalMemUsage, totalMemUsage / 1000);
    }


    // request a new one including lock
    Job* JobPool::RequestNew()
    {
        LockGuard lock(mLock);
        Job* result = RequestNewWithoutLock();
        return result;
    }


    // free including lock
    void JobPool::Free(Job* job)
    {
        LockGuard lock(mLock);
        FreeWithoutLock(job);
    }


    // lock the pool
    void JobPool::Lock()
    {
        mLock.Lock();
    }


    // unlock the pool
    void JobPool::Unlock()
    {
        mLock.Unlock();
    }
}   // namespace MCore
