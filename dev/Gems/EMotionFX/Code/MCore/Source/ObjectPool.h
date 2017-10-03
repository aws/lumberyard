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
#include "LogManager.h"
#include "Array.h"
#include <mutex>

namespace MCore
{
    /**
     * The object pool template is here to reuse objects, preventing you to new and delete objects all the time.
     * To initialize the pool use the Resize function first, to set how many objects of this type are stored inside the pool.
     * After that inside your main loop use the AcquireObject function to grab a new object pointer, instead of newing an object.
     * When you can delete the object, use the ReleaseObject function instead of deleting it.
     * The pool can grow if you tell the Acquire function to allow this. It will only grow when all objects inside the pool are already in use and you
     * try to acquire a new one.
     */
    template <class T>
    class ObjectPool
    {
        MCORE_MEMORYOBJECTCATEGORY(ObjectPool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_OBJECTPOOL);
    public:
        // constructor and destructor
        ObjectPool();
        virtual ~ObjectPool();

        // init and shutdown
        void SetMemoryCategory(uint32 categoryID);
        void Reserve(uint32 numObjects);
        void Resize(uint32 numObjects);
        void Release(); // automatically called in destructor
        void SetTriggerEvents(bool triggerEvents)   { mTriggerEvents = triggerEvents;}
        bool GetTriggerEvents() const               { return mTriggerEvents; }

        // main functions
        T* AcquireObject(bool autoResize = true);     // acquire a new object
        void ReleaseObject(T* object);              // free the object that was acquired before

        // misc
        uint32 GetNumObjects() const        { return mObjects.GetLength(); }
        uint32 GetNumFreeObjects() const    { return mFreeList.GetLength(); }
        uint32 GetNumUsedObjects() const    { return mUsedList.GetLength(); }
        T* GetObject(uint32 index) const    { return mObjects[index]; }

    protected:
        MCore::Array<T*>    mObjects;
        MCore::Array<T*>    mFreeList;
        MCore::Array<T*>    mUsedList;
        std::mutex          mLock;
        bool                mTriggerEvents;

        void AddNewObjects(uint32 numObjects);
        void RemoveObjects(uint32 numObjects);

        // events
        virtual void OnCreateObject(T* object) {}
        virtual void OnDeleteObject(T* object) {}
    };

    // include the inline code
#include "ObjectPool.inl"
}   // namespace MCore
