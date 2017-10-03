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

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/Array.h>
#include <MCore/Source/HashTable.h>
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraphObjectData;
    class AnimGraphInstance;

    /**
     *
     *
     */
    class EMFX_API AnimGraphObjectDataPool
    {
        friend class AnimGraphObjectData;
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphObjectDataPool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL)

    public:
        enum EPoolType
        {
            POOLTYPE_STATIC,
            POOLTYPE_DYNAMIC
        };

        AnimGraphObjectDataPool();
        ~AnimGraphObjectDataPool();

        // main methods
        void Register(uint32 objectTypeID, uint32 numInitialObjectDatas = 256, EPoolType poolType = POOLTYPE_DYNAMIC, uint32 subPoolSize = 1024);

        // thread safe
        AnimGraphObjectData* RequestNew(uint32 objectTypeID, AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
        void Free(AnimGraphObjectData* objectData);
        void UnregisterObjectType(uint32 objectTypeID);

        // lockfree (not thread safe without manual locks)
        void RegisterWithoutLock(uint32 objectTypeID, uint32 numInitialObjectDatas = 256, EPoolType poolType = POOLTYPE_DYNAMIC, uint32 subPoolSize = 1024);
        AnimGraphObjectData* RequestNewWithoutLock(uint32 objectTypeID, AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
        void FreeWithoutLock(AnimGraphObjectData* objectData);
        void UnregisterObjectTypeWithoutLock(uint32 objectTypeID);

        // misc
        void LogMemoryStats();  // internally locks and unlocks
        void Shrink();          // shrink the memory usage

        // manual locking and unlocking for threading
        void Lock();
        void Unlock();


    private:
        class EMFX_API SubPool
        {
            friend class AnimGraphObject;
            MCORE_MEMORYOBJECTCATEGORY(SubPool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL)

        public:
            SubPool();
            ~SubPool();

            uint8*      mData;
            uint32      mNumObjectDatas;
            uint32      mObjectDataSize;
            uint32      mNumInUse;
        };

        struct EMFX_API MemLocation
        {
            void*       mAddress;
            SubPool*    mSubPool;
        };

        class EMFX_API Pool
        {
            MCORE_MEMORYOBJECTCATEGORY(Pool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL)

        public:
            Pool(uint32 typeID);
            ~Pool();

            uint8*                      mData;
            AnimGraphObjectData*       mObjectData;
            uint32                      mObjectDataSize;
            uint32                      mNumObjectDatas;
            uint32                      mNumUsedObjectDatas;
            uint32                      mSubPoolSize;
            uint32                      mTypeID;
            MCore::Array<MemLocation>   mFreeList;
            MCore::Array<SubPool*>      mSubPools;
            EPoolType                   mPoolType;
        };

        MCore::HashTable<uint32, Pool*>     mPools;
        MCore::MutexRecursive               mLock;

        void ShrinkPool(Pool* pool);
    };
}   // namespace EMotionFX
