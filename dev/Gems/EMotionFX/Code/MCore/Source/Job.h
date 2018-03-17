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
#include "MultiThreadManager.h"
#include "Array.h"


namespace MCore
{
    // a job function
    class Job;
    typedef AZStd::function<void(const Job* job)> JobFunction;

    class MCORE_API Job
    {
        friend class JobQueue;
        friend class JobPool;

        MCORE_MEMORYOBJECTCATEGORY(Job, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM)
    public:
        enum : uint32
        {
            JOBTYPE_SYNC    = 0,
            JOBTYPE_UNKNOWN = MCORE_INVALIDINDEX32
        };

        static Job* Create();
        static Job* Create(const JobFunction& func, uint32 jobTypeID = MCORE_INVALIDINDEX32, void* customData = nullptr);
        static Job* CreateWithoutLock();
        static Job* CreateWithoutLock(const JobFunction& func, uint32 jobTypeID = MCORE_INVALIDINDEX32, void* customData = nullptr);

        void Destroy();
        void DestroyWithoutLock();

        MCORE_INLINE void SetJobFunction(const JobFunction& jobFunction)    { mJobFunction = jobFunction; }
        MCORE_INLINE void SetJobTypeID(uint32 id)                           { mTypeID = id; }
        MCORE_INLINE void SetCustomData(void* data)                         { mCustomData = data; }
        MCORE_INLINE void SetListIndex(uint32 index)                        { mListIndex = index; }

        MCORE_INLINE uint32 GetJobTypeID() const                            { return mTypeID; }
        MCORE_INLINE void* GetCustomData() const                            { return mCustomData; }
        MCORE_INLINE const JobFunction& GetJobFunction() const              { return mJobFunction; }

        MCORE_INLINE uint32 GetThreadIndex() const                          { return mThreadIndex; }
        MCORE_INLINE uint32 GetListIndex() const                            { return mListIndex; }

        MCORE_INLINE Job* GetNextJob() const                                { return mNext; }
        MCORE_INLINE Job* GetPrevJob() const                                { return mPrev; }

        void SetThreadIndex(uint32 index);
        void Execute() const;

    private:
        JobFunction mJobFunction;
        Job*        mNext;
        Job*        mPrev;
        void*       mCustomData;
        uint32      mTypeID;
        uint32      mThreadIndex;
        uint32      mListIndex;

        Job();
        ~Job();

        static Job* Create(void* memLocation);
    };
}   // namespace MCore
