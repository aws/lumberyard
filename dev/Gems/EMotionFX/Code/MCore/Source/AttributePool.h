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
#include "HashTable.h"
#include "Array.h"
#include "LogManager.h"
#include "MultiThreadManager.h"


namespace MCore
{
    // forward declarations
    class Attribute;

    /**
     *
     *
     */
    class MCORE_API AttributePool
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributePool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTEPOOL)

    public:
        enum EPoolType
        {
            POOLTYPE_STATIC,
            POOLTYPE_DYNAMIC
        };

        AttributePool();
        ~AttributePool();

        // thread-safe
        void RegisterType(uint32 attributeTypeID, uint32 numAttribs = 1024, EPoolType poolType = POOLTYPE_DYNAMIC, uint32 subPoolSize = 1024);
        Attribute* RequestNew(uint32 typeID);
        void Free(Attribute* attribute);

        // lock free (not thread-safe)
        void RegisterTypeWithoutLock(uint32 attributeTypeID, uint32 numAttribs = 1024, EPoolType poolType = POOLTYPE_DYNAMIC, uint32 subPoolSize = 1024);
        Attribute* RequestNewWithoutLock(uint32 typeID);
        void FreeWithoutLock(Attribute* attribute);

        // misc
        void LogMemoryStats();  // internally locks and unlocks

        // threading
        void Lock();
        void Unlock();

    private:
        class MCORE_API SubPool
        {
            MCORE_MEMORYOBJECTCATEGORY(AttributePool::SubPool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTEPOOL)

        public:
            SubPool();
            ~SubPool();

            uint8*              mAttributeData;
            uint32              mNumAttributes;
            uint32              mAttributeSize;
        };


        class MCORE_API Pool
        {
            MCORE_MEMORYOBJECTCATEGORY(AttributePool::Pool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTEPOOL)

        public:
            Pool(uint32 typeID);
            ~Pool();

            uint8*              mAttributeData;
            Attribute*          mAttribute;
            uint32              mAttributeSize;
            uint32              mNumAttributes;
            uint32              mNumUsedAttributes;
            uint32              mSubPoolSize;
            Array<void*>        mFreeList;
            Array<SubPool*>     mSubPools;
            EPoolType           mPoolType;
        };

        HashTable<uint32, Pool*>    mPools;
        Mutex                       mMutex;
    };
}   // namespace MCore
