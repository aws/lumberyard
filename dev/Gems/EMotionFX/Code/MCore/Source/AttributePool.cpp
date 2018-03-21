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
#include "AttributePool.h"
#include "Attribute.h"
#include "AttributeFactory.h"


namespace MCore
{
    //--------------------------------------------------------------------------------------------------
    // class AttributePool::SubPool
    //--------------------------------------------------------------------------------------------------

    // constructor
    AttributePool::SubPool::SubPool()
        : mAttributeData(nullptr)
        , mNumAttributes(0)
        , mAttributeSize(0)
    {
    }


    // destructor
    AttributePool::SubPool::~SubPool()
    {
        MCore::AlignedFree(mAttributeData);
        mAttributeData = nullptr;
    }


    //--------------------------------------------------------------------------------------------------
    // class AttributePool::Pool
    //--------------------------------------------------------------------------------------------------

    // constructor
    AttributePool::Pool::Pool(uint32 typeID)
    {
        mFreeList.SetMemoryCategory(MCORE_MEMCATEGORY_ATTRIBUTEPOOL);
        mSubPools.SetMemoryCategory(MCORE_MEMCATEGORY_ATTRIBUTEPOOL);
        mPoolType           = POOLTYPE_DYNAMIC;
        mAttributeData      = nullptr;
        mNumAttributes      = 0;
        mNumUsedAttributes  = 0;
        mSubPoolSize        = 0;

        // create the temp attribute
        const uint32 attribIndex = GetAttributeFactory().FindAttributeIndexByType(typeID);
        if (attribIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogError("MCore::AttributePool::Pool() - The attribute type you are trying to register hasn't been registered to the Attribute Factory yet, please do so first!");
            MCORE_ASSERT(false);
            return;
        }

        mAttribute = GetAttributeFactory().GetRegisteredAttribute(attribIndex);
        mAttributeSize = AZ_SIZE_ALIGN(mAttribute->GetClassSize(), 16);
    }


    // destructor
    AttributePool::Pool::~Pool()
    {
        if (mPoolType == POOLTYPE_STATIC)
        {
            MCore::AlignedFree(mAttributeData);
            mAttributeData = nullptr;
            mFreeList.Clear();
        }
        else
        if (mPoolType == POOLTYPE_DYNAMIC)
        {
            MCORE_ASSERT(mAttributeData == nullptr);

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
    // class AttributePool
    //--------------------------------------------------------------------------------------------------

    // constructor
    AttributePool::AttributePool()
    {
        mPools.Init(25);
    }


    // destructor
    AttributePool::~AttributePool()
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
    void AttributePool::RegisterTypeWithoutLock(uint32 attributeTypeID, uint32 numAttribs, EPoolType poolType, uint32 subPoolSize)
    {
        // check if we use numAttribs==0 with a static pool, as that isn't allowed of course
        if (poolType == POOLTYPE_STATIC && numAttribs == 0)
        {
            MCore::LogError("MCore::AttributePool::RegisterAttributeType() - The number of attributes cannot be 0 when using a static pool. Please set the dynamic parameter to true, or increase the value of numAttribs.");
            MCORE_ASSERT(false);
            return;
        }

        // check if we already registered for this attribute type
        const uint32 typeID = attributeTypeID;
        if (mPools.Contains(typeID))
        {
            MCore::LogWarning("MCore::AttributePool::RegisterAttributeType() - The attribute with type (%d or 0x%x) has already been registered. This register call with (numAttribs=%d) will be ignored now.", typeID, typeID, numAttribs);
            return;
        }

        // create the subpool for the attribute type
        Pool* pool = new Pool(typeID);
        if (pool->mAttribute == nullptr)
        {
            return;
        }

        pool->mNumAttributes    = numAttribs;
        pool->mPoolType         = poolType;
        pool->mSubPoolSize      = subPoolSize;

        // if we have a static pool
        if (poolType == POOLTYPE_STATIC)
        {
            pool->mAttributeData    = (uint8*)MCore::AlignedAllocate(numAttribs * pool->mAttributeSize, pool->mAttribute->GetMemoryAlignment(), MCORE_MEMCATEGORY_ATTRIBUTEPOOL);  // alloc space

            // call the constructors on each of the attributes
            // also init the free list
            pool->mFreeList.ResizeFast(numAttribs);
            for (uint32 i = 0; i < numAttribs; ++i)
            {
                //MCore::MemCopy(pool->mAttributeData + i*pool->mAttributeSize, pool->mAttribute, pool->mAttributeSize);    // try to simulate a constructor by cloning the newly created temp attribute memory
                pool->mFreeList[i] = (void*)(pool->mAttributeData + i * pool->mAttributeSize);
            }
        }
        else // if we have a dynamic pool
        if (poolType == POOLTYPE_DYNAMIC)
        {
            pool->mSubPools.Reserve(32);

            SubPool* subPool = new SubPool();
            subPool->mAttributeData = (uint8*)MCore::AlignedAllocate(numAttribs * pool->mAttributeSize, pool->mAttribute->GetMemoryAlignment(), MCORE_MEMCATEGORY_ATTRIBUTEPOOL);  // alloc space
            subPool->mNumAttributes = numAttribs;
            subPool->mAttributeSize = pool->mAttributeSize;

            // call the constructors on each of the attributes
            // also init the free list
            pool->mFreeList.ResizeFast(numAttribs);
            for (uint32 i = 0; i < numAttribs; ++i)
            {
                //MCore::MemCopy(subPool->mAttributeData + i*pool->mAttributeSize, pool->mAttribute, pool->mAttributeSize); // try to simulate a constructor by cloning the newly created temp attribute memory
                pool->mFreeList[i] = (void*)(subPool->mAttributeData + i * pool->mAttributeSize);
            }

            pool->mSubPools.Add(subPool);
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
        }
        // add the new subpool to the hash table
        mPools.Add(typeID, pool);
    }


    // return a new attribute
    Attribute* AttributePool::RequestNewWithoutLock(uint32 typeID)
    {
        // find the attribute subpool
        Pool* pool = nullptr;
        if (mPools.GetValue(typeID, &pool) == false)
        {
            //MCore::LogInfo("MCore::AttributePool::RequestNewAttribute() - The attribute type (%d or 0x%x) you are trying to request has not been registered yet, auto registering a dynamic pool of size 1024.", typeID, typeID);
            RegisterTypeWithoutLock(typeID, 1024, POOLTYPE_DYNAMIC, 1024);
            mPools.GetValue(typeID, &pool);
        }

        // if there is are free items left
        if (pool->mFreeList.GetLength() > 0)
        {
            // get the attribute
            Attribute* result = (Attribute*)pool->mFreeList.GetLast();
            pool->mFreeList.RemoveLast(); // remove it from the free list
            pool->mNumUsedAttributes++;
            //MCore::MemCopy(result, pool->mAttribute, pool->mAttributeSize);   // try to simulate a constructor by cloning the newly created temp attribute memory
            pool->mAttribute->CreateInstance(result); // call the constructor
            return result;
        }

        // we have no more free attributes left
        if (pool->mPoolType == POOLTYPE_DYNAMIC) // we're dynamic, so we can just create new ones
        {
            const uint32 numAttribs = pool->mSubPoolSize;

            pool->mNumAttributes += numAttribs;

            SubPool* subPool = new SubPool();
            subPool->mAttributeData = (uint8*)MCore::AlignedAllocate(numAttribs * pool->mAttributeSize, pool->mAttribute->GetMemoryAlignment(), MCORE_MEMCATEGORY_ATTRIBUTEPOOL);  // alloc space
            subPool->mNumAttributes = numAttribs;
            subPool->mAttributeSize = pool->mAttributeSize;

            // call the constructors on each of the attributes
            // also init the free list
            const uint32 startIndex = pool->mFreeList.GetLength();
            //pool->mFreeList.Reserve( numAttribs * 2 );
            if (pool->mFreeList.GetMaxLength() < pool->mNumAttributes)
            {
                pool->mFreeList.Reserve(pool->mNumAttributes);
            }

            pool->mFreeList.ResizeFast(startIndex + numAttribs);
            for (uint32 i = 0; i < numAttribs; ++i)
            {
                //MCore::MemCopy(subPool->mAttributeData + i*pool->mAttributeSize, pool->mAttribute, pool->mAttributeSize); // try to simulate a constructor by cloning the newly created temp attribute memory
                pool->mFreeList[i + startIndex] = (void*)(subPool->mAttributeData + i * pool->mAttributeSize);
            }

            pool->mSubPools.Add(subPool);

            Attribute* result = (Attribute*)pool->mFreeList.GetLast();
            pool->mFreeList.RemoveLast(); // remove it from the free list
            pool->mNumUsedAttributes++;
            pool->mAttribute->CreateInstance(result); // call the constructor
            return result;
        }
        else // we are static and ran out of free attributes
        if (pool->mPoolType == POOLTYPE_STATIC)
        {
            MCore::LogError("MCore::AttributePool::RequestNewAttribute() - There are no free attributes in the static pool for attributes of type %d or 0x%x. Please increase the size of the pool or make it dynamic when calling RegisterAttributeType.", typeID, typeID);
            MCORE_ASSERT(false); // we ran out of free attributes
            return nullptr;
        }
        else
        {
            MCORE_ASSERT(false); // unhandled pool type
            return nullptr;
        }
    }


    // free a given attribute
    void AttributePool::FreeWithoutLock(Attribute* attribute)
    {
        if (attribute == nullptr)
        {
            return;
        }

        const uint32 typeID = attribute->GetType();

        // find the attribute subpool
        Pool* pool;
        if (mPools.GetValue(typeID, &pool) == false)
        {
            MCore::LogWarning("MCore::AttributePool::FreeAttribute() - The attribute type (%d or 0x%x) you are trying to free has not been registered yet, please use RegisterAttributeType first.", typeID, typeID);
            MCORE_ASSERT(false); // please call RegisterAttributeType first to register the given attribute type
            return;
        }

        // add it back to the free list
        attribute->~Attribute(); // call the destructor
        pool->mFreeList.Add(attribute);
        pool->mNumUsedAttributes--;
    }


    // register new (thread safe)
    void AttributePool::RegisterType(uint32 attributeTypeID, uint32 numAttribs, EPoolType poolType, uint32 subPoolSize)
    {
        LockGuard guard(mMutex);
        RegisterTypeWithoutLock(attributeTypeID, numAttribs, poolType, subPoolSize);
    }


    // request new (thread safe)
    Attribute* AttributePool::RequestNew(uint32 typeID)
    {
        LockGuard guard(mMutex);
        return RequestNewWithoutLock(typeID);
    }


    // free (thread safe)
    void AttributePool::Free(Attribute* attribute)
    {
        LockGuard guard(mMutex);
        FreeWithoutLock(attribute);
    }



    // log the memory stats
    void AttributePool::LogMemoryStats()
    {
        Lock();
        LogInfo("MCore::AttributePool::LogMemoryStats() - Logging attribute pool info");

        uint32 totalAttribMemUsage = 0;
        uint32 totalUsedAttribMemUsage = 0;
        uint32 totalMemUsage = 0;
        uint32 totalNumAttributes = 0;
        uint32 totalNumUsedAttributes = 0;
        uint32 poolIndex = 0;

        const uint32 numElements = mPools.GetNumTableElements();
        for (uint32 t = 0; t < numElements; ++t)
        {
            const uint32 numEntries = mPools.GetNumEntries(t);
            for (uint32 e = 0; e < numEntries; ++e)
            {
                const Pool* pool = mPools.GetEntry(t, e).GetValue();

                const uint32 numFree = pool->mFreeList.GetLength();
                uint32 numUsed = pool->mNumUsedAttributes;
                uint32 memUsage = 0;
                uint32 usedMemUsage = 0;

                if (pool->mPoolType == POOLTYPE_STATIC)
                {
                    if (pool->mNumAttributes > 0)
                    {
                        memUsage = pool->mNumAttributes * pool->mAttributeSize;
                        usedMemUsage = numUsed * pool->mAttributeSize;
                    }
                }
                else
                if (pool->mPoolType == POOLTYPE_DYNAMIC)
                {
                    if (pool->mNumAttributes > 0)
                    {
                        memUsage = pool->mNumAttributes * pool->mAttributeSize;
                        usedMemUsage = numUsed * pool->mAttributeSize;
                    }
                }

                totalAttribMemUsage += memUsage;
                totalUsedAttribMemUsage += usedMemUsage;
                totalMemUsage += memUsage;
                totalMemUsage += sizeof(Pool);
                totalMemUsage += pool->mFreeList.CalcMemoryUsage(false);

                LogInfo("Pool #%d:", poolIndex);
                if (pool->mPoolType == POOLTYPE_STATIC)
                {
                    LogInfo("   - Attribute Type: %s", pool->mAttribute->GetTypeString());
                }
                else
                if (pool->mPoolType == POOLTYPE_DYNAMIC)
                {
                    LogInfo("   - Attribute Type: %s", pool->mAttribute->GetTypeString());
                    LogInfo("   - Num SubPools:   %d", pool->mSubPools.GetLength());
                }

                LogInfo("   - Num Attributes: %d", pool->mNumAttributes);
                LogInfo("   - Num Free:       %d", numFree);
                LogInfo("   - Num Used:       %d", numUsed);
                LogInfo("   - PoolType:       %s", (pool->mPoolType == POOLTYPE_STATIC) ? "Static" : "Dynamic");
                LogInfo("   - Attribute Mem:  %d bytes (%d k)", memUsage, memUsage / 1000);

                totalNumAttributes += pool->mNumAttributes;
                totalNumUsedAttributes += numUsed;

                poolIndex++;
            }
        }

        LogInfo("Attribute Pool Totals:");
        LogInfo("   - Num Pools:           %d", poolIndex);
        LogInfo("   - Num Attributes:      %d", totalNumAttributes);
        LogInfo("   - Num Used Attributes: %d", totalNumUsedAttributes);
        LogInfo("   - Total Attribute Mem: %d (%d k)", totalAttribMemUsage, totalAttribMemUsage / 1000);
        LogInfo("   - Used Attribute Mem:  %d (%d k)", totalUsedAttribMemUsage, totalUsedAttribMemUsage / 1000);
        LogInfo("   - Total Mem Usage:     %d (%d k)", totalMemUsage, totalMemUsage / 1000);
        Unlock();
    }


    // wait with execution until we can set the lock
    void AttributePool::Lock()
    {
        mMutex.Lock();
    }


    // release the lock again
    void AttributePool::Unlock()
    {
        mMutex.Unlock();
    }
}   // namespace MCore
