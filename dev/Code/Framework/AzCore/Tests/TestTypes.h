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
#ifndef AZCORE_UNITTEST_USERTYPES_H
#define AZCORE_UNITTEST_USERTYPES_H

#include <AzCore/base.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/AllocationRecords.h>

namespace UnitTest
{
    static const bool EnableLeakTracking = false;

    class AllocatorsFixture
        : public ::testing::Test
    {
        void* m_memBlock;
        AZ::Debug::DrillerManager* m_drillerManager;
        bool m_useMemoryDriller = true;
        unsigned int m_heapSizeMB = 15;
    public:
        AllocatorsFixture(unsigned int heapSizeMB = 15, bool isMemoryDriller = true)
            : m_memBlock(nullptr)
            , m_heapSizeMB(heapSizeMB)
            , m_useMemoryDriller(isMemoryDriller)
        {
            
        }

        virtual ~AllocatorsFixture()
        {
            // The allocator & memory block itself can't be destroyed in TearDown, because GTest hasn't actually destructed
            // the test class yet. This can leave containers and other structures pointing to garbage memory, resulting in
            // crashes. We have to free memory last, after absolutely everything else.
            if (m_memBlock)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
                DebugAlignFree(m_memBlock);
            }
        }

        void SetUp() override
        {
            if (m_useMemoryDriller)
            {
                m_drillerManager = AZ::Debug::DrillerManager::Create();
                m_drillerManager->Register(aznew AZ::Debug::MemoryDriller);
            }
            else
            {
                m_drillerManager = nullptr;
            }

            AZ::SystemAllocator::Descriptor desc;
            desc.m_heap.m_numMemoryBlocks = 1;
            desc.m_heap.m_memoryBlocksByteSize[0] = m_heapSizeMB * 1024 * 1024;
            m_memBlock = DebugAlignAlloc(desc.m_heap.m_memoryBlocksByteSize[0], desc.m_heap.m_memoryBlockAlignment);
            desc.m_heap.m_memoryBlocks[0] = m_memBlock;

            if (EnableLeakTracking)
            {
                desc.m_allocationRecords = true;
                desc.m_stackRecordLevels = 5;
            }

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);

            if (EnableLeakTracking)
            {
                AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
                if (records)
                {
                    records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
                }
            }
        }

        void TearDown() override
        {
            if (m_drillerManager)
            {
                AZ::Debug::DrillerManager::Destroy(m_drillerManager);
            }
        }
    };

    class DLLTestVirtualClass
    {
    public:
        DLLTestVirtualClass()
            : m_data(1) {}
        virtual ~DLLTestVirtualClass() {}

        int m_data;
    };

    template <AZ::u32 size, AZ::u8 instance, size_t alignment = 16>
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC >= 1900
    struct alignas(alignment) CreationCounter
#else
    struct CreationCounter
#endif
    {
        AZ_TYPE_INFO(CreationCounter, "{E9E35486-4366-4066-86E5-1A8CEB44198B}");
        int test[size / sizeof(int)];

        static int s_count;
        static int s_copied;
        static int s_moved;
        CreationCounter(int def = 0)
        {
            ++s_count;
            test[0] = def;
        }

        CreationCounter(AZStd::initializer_list<int> il)
        {
            ++s_count;
            if (il.size() > 0)
            {
                test[0] = *il.begin();
            }
        }
        CreationCounter(const CreationCounter& rhs)
            : CreationCounter()
        {
            memcpy(test, rhs.test, AZ_ARRAY_SIZE(test));
            ++s_copied;
        }
        CreationCounter(CreationCounter&& rhs)
            : CreationCounter()
        {
            memmove(test, rhs.test, AZ_ARRAY_SIZE(test));
            ++s_moved;
        }

        ~CreationCounter()
        {
            --s_count;
        }

        const int& val() const { return test[0]; }
        int& val() { return test[0]; }

        static void Reset()
        {
            s_count = 0;
            s_copied = 0;
            s_moved = 0;
        }
    };

    template <AZ::u32 size, AZ::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::s_count = 0;
    template <AZ::u32 size, AZ::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::s_copied = 0;
    template <AZ::u32 size, AZ::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::s_moved = 0;
}

#endif // AZCORE_UNITTEST_USERTYPES_H
