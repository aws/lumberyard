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


// constructor
template <class T>
ObjectPool<T>::ObjectPool()
{
    mTriggerEvents = false;
}


// destructor
template <class T>
ObjectPool<T>::~ObjectPool()
{
    Release();
}


// link the internal arrays to a given memory category
template <class T>
void ObjectPool<T>::SetMemoryCategory(uint32 categoryID)
{
    mObjects.SetMemoryCategory(categoryID);
    mFreeList.SetMemoryCategory(categoryID);
    mUsedList.SetMemoryCategory(categoryID);
}


// release the pool objects
template <class T>
void ObjectPool<T>::Release()
{
    // clear the objects list
    const uint32 numObjects = mObjects.GetLength();
    for (uint32 i = 0; i < numObjects; ++i)
    {
        if (mTriggerEvents)
        {
            OnDeleteObject(mObjects[i]);
        }

        delete mObjects[i];
    }
    mObjects.Clear();

    // clear the free and used list
    mFreeList.Clear();
    mUsedList.Clear();
}


// reserve enough array elements to prevent reallocations of the internal arrays
template <class T>
void ObjectPool<T>::Reserve(uint32 numObjects)
{
    mObjects.Reserve(numObjects);
    mFreeList.Reserve(numObjects);
    mUsedList.Reserve(numObjects);
}


// resize the pool
template <class T>
void ObjectPool<T>::Resize(uint32 numObjects)
{
    std::unique_lock<std::mutex> lock(mLock);

    // if we stick to the same size, do nothing
    const uint32 oldSize = mObjects.GetLength();
    if (numObjects == oldSize)
    {
        return;
    }

    // if we need to add new objects
    if (oldSize < numObjects)
    {
        AddNewObjects(numObjects - oldSize);
    }
    else    // we need to remove existing objects
    {
        RemoveObjects(oldSize - numObjects);
    }
}



// add a given amount of new objects
template <class T>
void ObjectPool<T>::AddNewObjects(uint32 numObjects)
{
    // resize the objects array and reserve the free and used lists to prevent reallocs
    const uint32 oldLength = mObjects.GetLength();
    mObjects.Resize(oldLength + numObjects);
    mFreeList.Reserve(mObjects.GetLength());
    mUsedList.Reserve(mObjects.GetLength());

    // for all objects we need to add
    for (uint32 i = 0; i < numObjects; ++i)
    {
        // create the object and add it to the free list
        const uint32 newObjectIndex = oldLength + i;
        T* newObject = new T();
        mObjects[newObjectIndex] = newObject;
        mFreeList.Add(newObject);

        // trigger a creation event
        if (mTriggerEvents)
        {
            OnCreateObject(newObject);
        }
    }
}


// remove a given amount of objects
template <class T>
void ObjectPool<T>::RemoveObjects(uint32 numObjects)
{
    std::unique_lock<std::mutex> lock(mLock);

    // make sure we have enough objects that are unused
    MCORE_ASSERT(mFreeList.GetLength() >= numObjects);
    for (uint32 i = 0; i < numObjects; ++i)
    {
        // remove the object from the free list and object list
        const uint32 lastIndex = mFreeList.GetLength() - 1;
        T* object = mFreeList[lastIndex];
        mObjects.RemoveByValue(object);
        mFreeList.SwapRemove(mFreeList.GetLength() - 1);

        // trigger a delete event
        if (mTriggerEvents)
        {
            OnDeleteObject(object);
        }

        // delete the actual object
        delete object;
    }
}


// acquire an object
template <class T>
T* ObjectPool<T>::AcquireObject(bool autoResize)
{
    std::unique_lock<std::mutex> lock(mLock);

    // check if we have enough free items
    const uint32 numFree = mFreeList.GetLength();
    if (numFree == 0)
    {
        // if we are not allowed to grow this pool, return nullptr and show some warning
        if (autoResize == false)
        {
            LogWarning("MCore::ObjectPool<T>::AcquireObject - Object pool of size %d objects has no more free objects available!", sizeof(T));
            return nullptr;
        }
        else    // growing is allowed, and we need to do it now
        {
            AddNewObjects(10);      // add 10 new objects
            MCORE_ASSERT(mFreeList.GetLength() > 0);
        }
    }

    // remove it from the free list, add it to the used list
    T* result = mFreeList[mFreeList.GetLength() - 1];
    mFreeList.SwapRemove(mFreeList.GetLength() - 1);
    mUsedList.Add(result);

    // multithread unlock
    return result;
}


// release an object
template <class T>
void ObjectPool<T>::ReleaseObject(T* object)
{
    std::unique_lock<std::mutex> lock(mLock);

    // add it to the free list again
    mFreeList.Add(object);

    // try to remove it from the used list
    const bool removed = mUsedList.RemoveByValue(object);
    if (removed == false)   // if it fails this object did not belong to this pool
    {
        LogWarning("MCore::ObjectPool<T>::ReleaseObject - Object pool of size %d objects is trying to release an object that does not belong to this pool!", sizeof(T));
        MCORE_ASSERT(false);
    }
}
