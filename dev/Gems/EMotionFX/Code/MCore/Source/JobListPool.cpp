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
#include "JobListPool.h"
#include "JobList.h"
#include "LogManager.h"


namespace MCore
{
    //--------------------------------------------------------------------------------------------------
    // class JobPool::SubPool
    //--------------------------------------------------------------------------------------------------

    // constructor
    JobListPool::SubPool::SubPool()
        : mData(nullptr)
        , mNumJobLists(0)
    {
    }


    // destructor
    JobListPool::SubPool::~SubPool()
    {
        MCore::Free(mData);
        mData = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    // class JobPool::Pool
    //--------------------------------------------------------------------------------------------------

    // constructor
    JobListPool::Pool::Pool()
    {
        mFreeList.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);
        mSubPools.SetMemoryCategory(MCORE_MEMCATEGORY_JOBSYSTEM);
        mPoolType           = POOLTYPE_DYNAMIC;
        mData               = nullptr;
        mNumJobLists        = 0;
        mNumUsedJobLists    = 0;
        mSubPoolSize        = 0;
    }


    // destructor
    JobListPool::Pool::~Pool()
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
    JobListPool::JobListPool()
    {
        mPool = nullptr;
    }


    // destructor
    JobListPool::~JobListPool()
    {
        if (mPool && mPool->mNumUsedJobLists > 0)
        {
            LogError("MCore::JobListPool::~JobListPool() - There are still %d unfreed job lists, please use the Free function in the JobListPool to free them, just like you would delete the object.", mPool->mNumUsedJobLists);
        }

        delete mPool;
    }


    // init the pool
    void JobListPool::Init(uint32 numInitial, EPoolType poolType, uint32 subPoolSize)
    {
        if (mPool)
        {
            LogWarning("MCore::JobListPool::Init() - We have already initialized the pool, ignoring new init call.");
            return;
        }

        // check if we use numInitial==0 with a static pool, as that isn't allowed of course
        if (poolType == POOLTYPE_STATIC && numInitial == 0)
        {
            LogError("MCore::JobListPool::Init() - The number of initial job lists cannot be 0 when using a static pool. Please set the dynamic parameter to true, or increase the value of numInitial.");
            MCORE_ASSERT(false);
            return;
        }

        // create the subpool
        mPool = new Pool();
        mPool->mNumJobLists     = numInitial;
        mPool->mPoolType        = poolType;
        mPool->mSubPoolSize     = subPoolSize;

        // if we have a static pool
        if (poolType == POOLTYPE_STATIC)
        {
            mPool->mData    = (uint8*)MCore::Allocate(numInitial * sizeof(JobList), MCORE_MEMCATEGORY_JOBSYSTEM);// alloc space
            mPool->mFreeList.ResizeFast(numInitial);
            for (uint32 i = 0; i < numInitial; ++i)
            {
                void* memLocation = (void*)(mPool->mData + i * sizeof(JobList));
                mPool->mFreeList[i] = memLocation;
            }
        }
        else // if we have a dynamic pool
        if (poolType == POOLTYPE_DYNAMIC)
        {
            mPool->mSubPools.Reserve(32);

            SubPool* subPool = new SubPool();
            subPool->mData              = (uint8*)MCore::Allocate(numInitial * sizeof(JobList), MCORE_MEMCATEGORY_JOBSYSTEM);// alloc space
            subPool->mNumJobLists       = numInitial;

            mPool->mFreeList.ResizeFast(numInitial);
            for (uint32 i = 0; i < numInitial; ++i)
            {
                mPool->mFreeList[i] = (void*)(subPool->mData + i * sizeof(JobList));
            }

            mPool->mSubPools.Add(subPool);
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
        }
    }


    // return a new job
    JobList* JobListPool::RequestNewWithoutLock()
    {
        // check if we already initialized
        if (mPool == nullptr)
        {
            //LogInfo("MCore::JobListPool::RequestNew() - We have not yet initialized the pool, initializing it to a dynamic pool");
            Init();
        }

        // if there is are free items left
        if (mPool->mFreeList.GetLength() > 0)
        {
            JobList* result = JobList::Create(mPool->mFreeList.GetLast());
            mPool->mFreeList.RemoveLast(); // remove it from the free list
            mPool->mNumUsedJobLists++;
            return result;
        }

        // we have no more free attributes left
        if (mPool->mPoolType == POOLTYPE_DYNAMIC) // we're dynamic, so we can just create new ones
        {
            const uint32 numJobLists = mPool->mSubPoolSize;
            mPool->mNumJobLists += numJobLists;

            SubPool* subPool = new SubPool();
            subPool->mData          = (uint8*)MCore::Allocate(numJobLists * sizeof(JobList), MCORE_MEMCATEGORY_JOBSYSTEM);// alloc space
            subPool->mNumJobLists   = numJobLists;

            const uint32 startIndex = mPool->mFreeList.GetLength();
            if (mPool->mFreeList.GetMaxLength() < mPool->mNumJobLists)
            {
                mPool->mFreeList.Reserve(mPool->mNumJobLists);
            }

            mPool->mFreeList.ResizeFast(startIndex + numJobLists);
            for (uint32 i = 0; i < numJobLists; ++i)
            {
                void* memAddress = (void*)(subPool->mData + i * sizeof(JobList));
                mPool->mFreeList[i + startIndex] = memAddress;
            }

            mPool->mSubPools.Add(subPool);

            JobList* result = JobList::Create(mPool->mFreeList.GetLast());
            mPool->mFreeList.RemoveLast(); // remove it from the free list
            mPool->mNumUsedJobLists++;
            return result;
        }
        else // we are static and ran out of free attributes
        if (mPool->mPoolType == POOLTYPE_STATIC)
        {
            LogError("MCore::JobListPool::RequestNew() - There are no free joblists in the static pool. Please increase the size of the pool or make it dynamic when calling Init.");
            MCORE_ASSERT(false); // we ran out of free motion instances
            return nullptr;
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
            return nullptr;
        }
    }


    // free the joblist
    void JobListPool::FreeWithoutLock(JobList* jobList)
    {
        if (jobList == nullptr)
        {
            return;
        }

        if (mPool == nullptr)
        {
            LogError("MCore::JobListPool::Free() - The pool has not yet been initialized, please call Init first.");
            MCORE_ASSERT(false);
            return;
        }

        //MCORE_ASSERT( mPool->mFreeList.Contains(jobList) == false );

        // add it back to the free list
        jobList->~JobList(); // call the destructor
        mPool->mFreeList.Add(jobList);
        mPool->mNumUsedJobLists--;
    }


    // log the memory stats
    void JobListPool::LogMemoryStats()
    {
        LockGuard lock(mLock);
        LogInfo("MCore::JobListPool::LogMemoryStats() - Logging motion instance pool info");

        const uint32 numFree    = mPool->mFreeList.GetLength();
        uint32 numUsed          = mPool->mNumUsedJobLists;
        uint32 memUsage         = 0;
        uint32 usedMemUsage     = 0;
        uint32 totalMemUsage    = 0;
        uint32 totalUsedInstancesMemUsage = 0;

        if (mPool->mPoolType == POOLTYPE_STATIC)
        {
            if (mPool->mNumJobLists > 0)
            {
                memUsage = mPool->mNumJobLists * sizeof(JobList);
                usedMemUsage = numUsed * sizeof(JobList);
            }
        }
        else
        if (mPool->mPoolType == POOLTYPE_DYNAMIC)
        {
            if (mPool->mNumJobLists > 0)
            {
                memUsage = mPool->mNumJobLists * sizeof(JobList);
                usedMemUsage = numUsed * sizeof(JobList);
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

        LogInfo("   - Num JobLists:          %d", mPool->mNumJobLists);
        LogInfo("   - Num Free:              %d", numFree);
        LogInfo("   - Num Used:              %d", numUsed);
        LogInfo("   - PoolType:              %s", (mPool->mPoolType == POOLTYPE_STATIC) ? "Static" : "Dynamic");
        LogInfo("   - Total Mem:             %d bytes (%d k)", memUsage, memUsage / 1000);
        LogInfo("   - Used Mem:              %d (%d k)", totalUsedInstancesMemUsage, totalUsedInstancesMemUsage / 1000);
        LogInfo("   - Total Mem Usage:       %d (%d k)", totalMemUsage, totalMemUsage / 1000);
    }


    // request a new one including lock
    JobList* JobListPool::RequestNew()
    {
        LockGuard lock(mLock);
        JobList* result = RequestNewWithoutLock();
        return result;
    }


    // free including lock
    void JobListPool::Free(JobList* jobList)
    {
        LockGuard lock(mLock);
        FreeWithoutLock(jobList);
    }


    // lock the pool
    void JobListPool::Lock()
    {
        mLock.Lock();
    }


    // unlock the pool
    void JobListPool::Unlock()
    {
        mLock.Unlock();
    }
}   // namespace MCore
