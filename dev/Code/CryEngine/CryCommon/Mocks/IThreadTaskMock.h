#pragma once
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
#include <IThreadTask.h>

#pragma warning( push )
#pragma warning( disable : 4373 ) // previous versions of the compiler did not override when parameters only differed by const / volatile qualifiers

class ThreadTaskManagerMock
    : public IThreadTaskManager
{
public:
    MOCK_METHOD2(RegisterTask,
        void(IThreadTask * pTask, const SThreadTaskParams&options));
    MOCK_METHOD1(UnregisterTask,
        void(IThreadTask * pTask));
    MOCK_METHOD1(SetMaxThreadCount,
        void(int nMaxThreads));
    MOCK_METHOD1(CreateThreadsPool,
        ThreadPoolHandle(const ThreadPoolDesc&desc));
    MOCK_METHOD1(DestroyThreadsPool,
        const bool(const ThreadPoolHandle&handle));
    MOCK_CONST_METHOD2(GetThreadsPoolDesc,
        const bool(const ThreadPoolHandle handle, ThreadPoolDesc * pDesc));
    MOCK_METHOD2(SetThreadsPoolAffinity,
        const bool(const ThreadPoolHandle handle, const ThreadPoolAffinityMask AffinityMask));
    MOCK_METHOD2(SetThreadName,
        void(threadID dwThreadId, const char* sThreadName));
    MOCK_METHOD1(GetThreadName,
        const char*(threadID dwThreadId));
    MOCK_METHOD1(GetThreadByName,
        threadID(const char* sThreadName));
    MOCK_METHOD2(MarkThisThreadForDebugging,
        void(const char* name, bool bDump));
};

#pragma warning( pop )