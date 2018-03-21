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
#include "AnimGraphObjectDataPool.h"
#include "AnimGraphObjectData.h"
#include "AnimGraphObject.h"
#include "AnimGraphObjectFactory.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"


namespace EMotionFX
{
    //--------------------------------------------------------------------------------------------------
    // class AnimGraphObjectDataPool::SubPool
    //--------------------------------------------------------------------------------------------------

    // constructor
    AnimGraphObjectDataPool::SubPool::SubPool()
        : mData(nullptr)
        , mNumObjectDatas(0)
        , mObjectDataSize(0)
        , mNumInUse(0)
    {
    }


    // destructor
    AnimGraphObjectDataPool::SubPool::~SubPool()
    {
        MCore::AlignedFree(mData);
        mData = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    // class AnimGraphObjectDataPool::Pool
    //--------------------------------------------------------------------------------------------------

    // constructor
    AnimGraphObjectDataPool::Pool::Pool(uint32 typeID)
    {
        mFreeList.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL);
        mSubPools.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL);
        mPoolType           = POOLTYPE_DYNAMIC;
        mData               = nullptr;
        mNumObjectDatas     = 0;
        mNumUsedObjectDatas = 0;
        mSubPoolSize        = 0;
        mTypeID             = typeID;

        mFreeList.Reserve(10000);

        // get the object factory and try to find the object for this type ID
        AnimGraphObjectFactory* objectFactory = GetAnimGraphManager().GetObjectFactory();
        const uint32 objectIndex = objectFactory->FindRegisteredObjectByTypeID(typeID);
        if (objectIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogError("EMotionFX::AnimGraphObjectDataPool::Pool() - The object type you are trying to register unique data for hasn't been registered to the Object Factory yet, please do so first!");
            MCORE_ASSERT(false);
            return;
        }

        // create the temp object data from the object
        mObjectData = objectFactory->GetRegisteredObject(objectIndex)->CreateObjectData();
        mObjectDataSize = AZ_SIZE_ALIGN(mObjectData->GetClassSize(), 16);
    }


    // destructor
    AnimGraphObjectDataPool::Pool::~Pool()
    {
        mObjectData->Destroy();

        if (mPoolType == POOLTYPE_STATIC)
        {
            MCore::AlignedFree(mData);
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
    // class AnimGraphObjectDataPool
    //--------------------------------------------------------------------------------------------------

    // constructor
    AnimGraphObjectDataPool::AnimGraphObjectDataPool()
    {
        mPools.Init(25);
    }


    // destructor
    AnimGraphObjectDataPool::~AnimGraphObjectDataPool()
    {
        // delete the hash table subpools
        const uint32 numTableElements = mPools.GetNumTableElements();
        for (uint32 t = 0; t < numTableElements; ++t)
        {
            const uint32 numEntries = mPools.GetNumEntries(t);
            for (uint32 e = 0; e < numEntries; ++e)
            {
                delete mPools.GetEntry(t, e).GetValue();
            }
        }

        // clear the table
        mPools.Clear();
    }



    // register a new type
    void AnimGraphObjectDataPool::RegisterWithoutLock(uint32 objectTypeID, uint32 numInitialObjectDatas, EPoolType poolType, uint32 subPoolSize)
    {
        // check if we use numInitialObjectDatas==0 with a static pool, as that isn't allowed of course
        if (poolType == POOLTYPE_STATIC && numInitialObjectDatas == 0)
        {
            MCore::LogError("EMotionFX::AnimGraphObjectDataPool::Register() - The number of initial object datas cannot be 0 when using a static pool. Please set the dynamic parameter to true, or increase the value of numInitialObjectDatas.");
            MCORE_ASSERT(false);
            return;
        }

        // check if we already registered for this type
        if (mPools.Contains(objectTypeID))
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectDataPool::Register() - The object with type (%d or 0x%x) has already been registered. This register call with (numInitialObjectDatas=%d) will be ignored now.", objectTypeID, objectTypeID, numInitialObjectDatas);
            return;
        }

        // create the pool
        Pool* pool = new Pool(objectTypeID);
        if (pool->mObjectData == nullptr)
        {
            return;
        }

        pool->mNumObjectDatas   = numInitialObjectDatas;
        pool->mPoolType         = poolType;
        pool->mSubPoolSize      = subPoolSize;

        // if we have a static pool
        if (poolType == POOLTYPE_STATIC)
        {
            pool->mData = (uint8*)MCore::AlignedAllocate(numInitialObjectDatas * pool->mObjectDataSize, pool->mObjectData->GetMemoryAlignment(), EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL); // alloc space

            pool->mFreeList.ResizeFast(numInitialObjectDatas);
            for (uint32 i = 0; i < numInitialObjectDatas; ++i)
            {
                void* memLocation = (void*)(pool->mData + i * pool->mObjectDataSize);
                ;
                pool->mFreeList[i].mAddress = memLocation;
                pool->mFreeList[i].mSubPool = nullptr;
            }
        }
        else // if we have a dynamic pool
        if (poolType == POOLTYPE_DYNAMIC)
        {
            pool->mSubPools.Reserve(32);

            SubPool* subPool = new SubPool();
            subPool->mData  = (uint8*)MCore::AlignedAllocate(numInitialObjectDatas * pool->mObjectDataSize, pool->mObjectData->GetMemoryAlignment(), EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL);// alloc space
            subPool->mNumObjectDatas = numInitialObjectDatas;
            subPool->mObjectDataSize = pool->mObjectDataSize;

            pool->mFreeList.ResizeFast(numInitialObjectDatas);
            for (uint32 i = 0; i < numInitialObjectDatas; ++i)
            {
                pool->mFreeList[i].mAddress = (void*)(subPool->mData + i * pool->mObjectDataSize);
                pool->mFreeList[i].mSubPool = subPool;
            }

            pool->mSubPools.Add(subPool);
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
        }
        // add the new subpool to the hash table
        mPools.Add(objectTypeID, pool);
    }


    // return a new object data
    AnimGraphObjectData* AnimGraphObjectDataPool::RequestNewWithoutLock(uint32 typeID, AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
    {
        // find the pool
        Pool* pool = nullptr;
        if (mPools.GetValue(typeID, &pool) == false)
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectDataPool::RequestNew() - The object type (%d or 0x%x) you are trying to request has not been registered yet, auto registering a dynamic pool of size 256.", typeID, typeID);
            RegisterWithoutLock(typeID, 256, POOLTYPE_DYNAMIC, 1024);
            mPools.GetValue(typeID, &pool);
        }

        // if there is are free items left
        if (pool->mFreeList.GetLength() > 0)
        {
            const MemLocation& location = pool->mFreeList.GetLast();
            AnimGraphObjectData* result = pool->mObjectData->Clone(location.mAddress, object, animGraphInstance);
            if (location.mSubPool)
            {
                location.mSubPool->mNumInUse++;
            }

            result->SetSubPool(location.mSubPool);
            pool->mFreeList.RemoveLast(); // remove it from the free list
            pool->mNumUsedObjectDatas++;
            return result;
        }

        // we have no more free items
        if (pool->mPoolType == POOLTYPE_DYNAMIC) // we're dynamic, so we can just create new ones
        {
            const uint32 numObjectDatas = pool->mSubPoolSize;
            pool->mNumObjectDatas += numObjectDatas;

            SubPool* subPool = new SubPool();
            subPool->mData  = (uint8*)MCore::AlignedAllocate(numObjectDatas * pool->mObjectDataSize, pool->mObjectData->GetMemoryAlignment(), EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL);// alloc space
            subPool->mNumObjectDatas = numObjectDatas;
            subPool->mObjectDataSize = pool->mObjectDataSize;

            const uint32 startIndex = pool->mFreeList.GetLength();
            pool->mFreeList.Reserve(pool->mNumObjectDatas + startIndex);

            pool->mFreeList.ResizeFast(startIndex + numObjectDatas);
            for (uint32 i = 0; i < numObjectDatas; ++i)
            {
                void* memAddress = (void*)(subPool->mData + i * pool->mObjectDataSize);
                pool->mFreeList[i + startIndex].mAddress = memAddress;
                pool->mFreeList[i + startIndex].mSubPool = subPool;
            }

            pool->mSubPools.Add(subPool);

            const MemLocation& location = pool->mFreeList.GetLast();
            AnimGraphObjectData* result = pool->mObjectData->Clone(location.mAddress, object, animGraphInstance);
            if (location.mSubPool)
            {
                location.mSubPool->mNumInUse++;
            }
            result->SetSubPool(location.mSubPool);
            pool->mFreeList.RemoveLast(); // remove it from the free list
            pool->mNumUsedObjectDatas++;
            return result;
        }
        else // we are static and ran out of free attributes
        if (pool->mPoolType == POOLTYPE_STATIC)
        {
            MCore::LogError("EMotionFX::AnimGraphObjectDataPool::RequestNew() - There are no free items in the static pool for object of type %d or 0x%x. Please increase the size of the pool or make it dynamic when calling Register.", typeID, typeID);
            MCORE_ASSERT(false); // we ran out of free items
            return nullptr;
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
            return nullptr;
        }
    }


    // free a given object data
    void AnimGraphObjectDataPool::FreeWithoutLock(AnimGraphObjectData* objectData)
    {
        if (objectData == nullptr)
        {
            return;
        }

        const uint32 typeID = objectData->GetObject()->GetType();

        // find the attribute subpool
        Pool* pool;
        if (mPools.GetValue(typeID, &pool) == false)
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectDataPool::Free() - The object data type (%d or 0x%x) you are trying to free has not been registered yet, please use Register first.", typeID, typeID);
            MCORE_ASSERT(false);
            return;
        }

        // add it back to the free list
        if (objectData->GetSubPool())
        {
            objectData->GetSubPool()->mNumInUse--;
        }

        pool->mFreeList.AddEmpty();
        pool->mFreeList.GetLast().mAddress = objectData;
        pool->mFreeList.GetLast().mSubPool = objectData->GetSubPool();
        pool->mNumUsedObjectDatas--;

        objectData->DecreaseReferenceCount();
        objectData->~AnimGraphObjectData(); // call the destructor
    }



    // register (thread safe)
    void AnimGraphObjectDataPool::Register(uint32 objectTypeID, uint32 numInitialObjectDatas, EPoolType poolType, uint32 subPoolSize)
    {
        Lock();
        RegisterWithoutLock(objectTypeID, numInitialObjectDatas, poolType, subPoolSize);
        Unlock();
    }


    // request a new one (thread safe)
    AnimGraphObjectData* AnimGraphObjectDataPool::RequestNew(uint32 typeID, AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
    {
        Lock();
        AnimGraphObjectData* result = RequestNewWithoutLock(typeID, object, animGraphInstance);
        Unlock();
        return result;
    }


    // free (thread safe)
    void AnimGraphObjectDataPool::Free(AnimGraphObjectData* objectData)
    {
        if (objectData == nullptr)
        {
            return;
        }

        Lock();
        FreeWithoutLock(objectData);
        Unlock();
    }


    void AnimGraphObjectDataPool::UnregisterObjectType(uint32 objectTypeID)
    {
        Lock();
        UnregisterObjectTypeWithoutLock(objectTypeID);
        Unlock();
    }


    // log the memory stats
    void AnimGraphObjectDataPool::LogMemoryStats()
    {
        Lock();
        MCore::LogInfo("EMotionFX::AnimGraphObjectDataPool::LogMemoryStats() - Logging object data pool info");

        uint32 totalObjectDataMemUsage = 0;
        uint32 totalUsedObjectDataMemUsage = 0;
        uint32 totalMemUsage = 0;
        uint32 poolIndex = 0;

        const uint32 numElements = mPools.GetNumTableElements();
        for (uint32 t = 0; t < numElements; ++t)
        {
            const uint32 numEntries = mPools.GetNumEntries(t);
            for (uint32 e = 0; e < numEntries; ++e)
            {
                const Pool* pool = mPools.GetEntry(t, e).GetValue();

                const uint32 numFree    = pool->mFreeList.GetLength();
                uint32 numUsed          = pool->mNumUsedObjectDatas;
                uint32 memUsage         = 0;
                uint32 usedMemUsage     = 0;

                if (pool->mPoolType == POOLTYPE_STATIC)
                {
                    if (pool->mNumObjectDatas > 0)
                    {
                        memUsage = pool->mNumObjectDatas * pool->mObjectData->GetClassSize();
                        usedMemUsage = numUsed * pool->mObjectData->GetClassSize();
                    }
                }
                else
                if (pool->mPoolType == POOLTYPE_DYNAMIC)
                {
                    if (pool->mNumObjectDatas > 0)
                    {
                        memUsage = pool->mNumObjectDatas * pool->mObjectData->GetClassSize();
                        usedMemUsage = numUsed * pool->mObjectData->GetClassSize();
                    }
                }

                totalObjectDataMemUsage += memUsage;
                totalUsedObjectDataMemUsage += usedMemUsage;
                totalMemUsage += memUsage;
                totalMemUsage += sizeof(Pool);
                totalMemUsage += pool->mFreeList.CalcMemoryUsage(false);

                MCore::LogInfo("Pool #%d:", poolIndex);
                if (pool->mPoolType == POOLTYPE_STATIC)
                {
                    MCore::LogInfo("   - Object Type:     %s", pool->mObjectData->GetObject()->GetTypeString());
                }
                else
                if (pool->mPoolType == POOLTYPE_DYNAMIC)
                {
                    MCore::LogInfo("   - Object Type:     %s", pool->mObjectData->GetObject()->GetTypeString());
                    MCore::LogInfo("   - Num SubPools:    %d", pool->mSubPools.GetLength());
                }

                MCore::LogInfo("   - Num ObjectDatas: %d", pool->mNumObjectDatas);
                MCore::LogInfo("   - Num Free:        %d", numFree);
                MCore::LogInfo("   - Num Used:        %d", numUsed);
                MCore::LogInfo("   - PoolType:        %s", (pool->mPoolType == POOLTYPE_STATIC) ? "Static" : "Dynamic");
                MCore::LogInfo("   - ObjectData Mem:  %d bytes (%d k)", memUsage, memUsage / 1000);

                poolIndex++;
            }
        }

        MCore::LogInfo("Object Data Pool Totals:");
        MCore::LogInfo("   - Num Pools:            %d", poolIndex);
        MCore::LogInfo("   - Total ObjectData Mem: %d (%d k)", totalObjectDataMemUsage, totalObjectDataMemUsage / 1000);
        MCore::LogInfo("   - Used ObjectData Mem:  %d (%d k)", totalUsedObjectDataMemUsage, totalUsedObjectDataMemUsage / 1000);
        MCore::LogInfo("   - Total Mem Usage:      %d (%d k)", totalMemUsage, totalMemUsage / 1000);
        Unlock();
    }


    // wait with execution until we can set the lock
    void AnimGraphObjectDataPool::Lock()
    {
        mLock.Lock();
    }


    // release the lock again
    void AnimGraphObjectDataPool::Unlock()
    {
        mLock.Unlock();
    }


    // unregister a given object type's data pool
    void AnimGraphObjectDataPool::UnregisterObjectTypeWithoutLock(uint32 objectTypeID)
    {
        // try to find the entry
        uint32 elementIndex = MCORE_INVALIDINDEX32;
        uint32 entryIndex   = MCORE_INVALIDINDEX32;
        if (mPools.FindEntry(objectTypeID, &elementIndex, &entryIndex))
        {
            Pool* pool = mPools.GetEntry(elementIndex, entryIndex).GetValue();
            mPools.Remove(objectTypeID);
            delete pool;
        }
        else
        {
            MCore::LogWarning("EMotionFX::AnimGraphObjectDataPool::UnregisterObjectType() - The object data type (%d or 0x%x) you are trying to free has not been registered yet, please use Register first.", objectTypeID, objectTypeID);
            return;
        }
    }


    // shrink the pool
    void AnimGraphObjectDataPool::Shrink()
    {
        Lock();

        // iterate over all table entries
        const uint32 numElements = mPools.GetNumTableElements();
        for (uint32 e = 0; e < numElements; ++e)
        {
            const uint32 numEntries = mPools.GetNumEntries(e);
            for (uint32 n = 0; n < numEntries; ++n)
            {
                Pool* pool = mPools.GetEntry(e, n).GetValue();
                ShrinkPool(pool);
            }
        }

        Unlock();
    }


    // shrink the pool
    void AnimGraphObjectDataPool::ShrinkPool(Pool* pool)
    {
        for (uint32 i = 0; i < pool->mSubPools.GetLength(); )
        {
            SubPool* subPool = pool->mSubPools[i];
            if (subPool->mNumInUse == 0)
            {
                // remove all free allocations
                for (uint32 a = 0; a < pool->mFreeList.GetLength(); )
                {
                    if (pool->mFreeList[a].mSubPool == subPool)
                    {
                        pool->mFreeList.Remove(a);
                    }
                    else
                    {
                        ++a;
                    }
                }
                pool->mNumObjectDatas -= subPool->mNumObjectDatas;

                pool->mSubPools.Remove(i);
                delete subPool;
            }
            else
            {
                ++i;
            }
        }

        pool->mSubPools.Shrink();
        if ((pool->mFreeList.GetMaxLength() - pool->mFreeList.GetLength()) > 2048)
        {
            pool->mFreeList.ReserveExact(pool->mFreeList.GetLength() + 2048);
        }
    }
}   // namespace MCore
