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

#include "TestTypes.h"

#include <AzCore/IPC/SharedMemory.h>
#include <AzCore/IPC/DirectSocket.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>

using namespace AZ;

#if defined(AZ_PLATFORM_WINDOWS)

namespace UnitTest
{
    class IPCEvenOddSocket
    {
        const char*  m_target;
        bool         m_isEven;
        DirectSocket m_socket;
    public:
        IPCEvenOddSocket(const char* name, const char* target, bool isEven)
            : m_socket(name)
            , m_target(target)
            , m_isEven(isEven)
        {}

        void execute()
        {
            int send = m_isEven ? 0 : 1;
            int receive = m_isEven ? 1 : 0;
            char from[128];
            const int numOfElements = 100000;
            while (send < numOfElements || receive < numOfElements)
            {
                if (m_socket.SendTo(&send, sizeof(send), m_target))
                {
                    send += 2;
                }
                int lastReceive = receive;
                if (m_socket.Receive(&receive, sizeof(receive), from, AZ_ARRAY_SIZE(from)))
                {
                    AZ_TEST_ASSERT(strcmp(from, m_target) == 0);
                    AZ_TEST_ASSERT(lastReceive == receive);
                    if (m_isEven)
                    {
                        AZ_TEST_ASSERT(receive % 2 == 1);
                    }
                    else
                    {
                        AZ_TEST_ASSERT(receive % 2 == 0);
                    }
                    receive += 2;
                }
            }
        }
    };

    /**
     * Test InterProcessCommunication utils.
     */
    class IPC
        : public AllocatorsFixture
    {
    };

    TEST_F(IPC, Test)
    {
        // DirectSocket (we test inter thread, but functionality is the same).
        IPCEvenOddSocket even("Even", "Odd", true);
        IPCEvenOddSocket odd("Odd", "Even", false);

        AZStd::thread tEven(AZStd::bind(&IPCEvenOddSocket::execute, &even));
        AZStd::thread tOdd(AZStd::bind(&IPCEvenOddSocket::execute, &odd));
        tEven.join();
        tOdd.join();
    }

    TEST_F(IPC, SharedMemoryThrash)
    {
        AZ::SharedMemory sharedMem;
        AZ::SharedMemory::CreateResult result = sharedMem.Create("SharedMemoryUnitTest", sizeof(AZStd::thread_id), true);
        EXPECT_EQ(AZ::SharedMemory::CreatedNew, result);
        bool mapped = sharedMem.Map();
        EXPECT_TRUE(mapped);

        auto threadFunc = [&sharedMem]()
        {
            AZStd::thread_id* lastThreadId = static_cast<AZStd::thread_id*>(sharedMem.Data());

            for (int i = 0; i < 100000; ++i)
            {
                AZStd::lock_guard<decltype(sharedMem)> lock(sharedMem);
                *lastThreadId = AZStd::this_thread::get_id();
            }
        };

        AZStd::thread threads[8];
        for (size_t t = 0; t < AZ_ARRAY_SIZE(threads); ++t)
        {
            threads[t] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        bool unmapped = sharedMem.UnMap();
        EXPECT_TRUE(unmapped);
    }

#ifdef AZ_PLATFORM_WINDOWS
    TEST_F(IPC, SharedMemoryMutexAbandonment)
    {
        const char* sharedMemKey = "SharedMemoryUnitTest";

        auto threadFunc = [sharedMemKey]()
        {
            void* memBuffer = azmalloc(sizeof(AZ::SharedMemory));
            AZ::SharedMemory* sharedMem = new (memBuffer) AZ::SharedMemory();
            AZ::SharedMemory::CreateResult result = sharedMem->Create(sharedMemKey, sizeof(AZStd::thread_id), true);
            EXPECT_TRUE(result == AZ::SharedMemory::CreatedNew || result == AZ::SharedMemory::CreatedExisting);
            bool mapped = sharedMem->Map();
            EXPECT_TRUE(mapped);

            AZStd::thread_id* lastThreadId = static_cast<AZStd::thread_id*>(sharedMem->Data());

            for (int i = 0; i < 100000; ++i)
            {
                sharedMem->lock();
                *lastThreadId = AZStd::this_thread::get_id();
                if (i == (int)(*lastThreadId).m_id || sharedMem->IsLockAbandoned())
                {
                    // Simulate a crash/dead process by cleaning up the handles but not unlocking
                    HANDLE* mmapHandle = (HANDLE*)(reinterpret_cast<char*>(sharedMem) + 128);
                    HANDLE* mutexHandle = mmapHandle + 1;
                    bool closedMap = CloseHandle(*mmapHandle) != 0;
                    EXPECT_TRUE(closedMap);
                    bool closedMutex = CloseHandle(*mutexHandle) != 0;
                    EXPECT_TRUE(closedMutex); 
                    azfree(sharedMem); // avoid calling dtor
                    return;
                }
                sharedMem->unlock();
            }

            bool unmapped = sharedMem->UnMap();
            EXPECT_TRUE(unmapped);

            sharedMem->~SharedMemory();
            azfree(memBuffer);
        };

        AZStd::thread threads[8];
        for (size_t t = 0; t < AZ_ARRAY_SIZE(threads); ++t)
        {
            threads[t] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
#endif
}

#endif // AZ_PLATFORM_WINDOWS


