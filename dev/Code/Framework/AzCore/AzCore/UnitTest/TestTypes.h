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

#if defined(HAVE_BENCHMARK)

#if defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif // clang

#include <benchmark/benchmark.h>

#if defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif // clang

#endif // HAVE_BENCHMARK

namespace UnitTest
{
    /**
    * Base class to share common allocator code between fixture types.
    */
    class AllocatorsBase
    {
        AZ::Debug::DrillerManager* m_drillerManager;
    public:
        void SetupAllocator()
        {
            m_drillerManager = AZ::Debug::DrillerManager::Create();
            m_drillerManager->Register(aznew AZ::Debug::MemoryDriller);

            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            AZ::Debug::AllocationRecords* records = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords();
            if (records)
            {
                records->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
            }
        }

        void TeardownAllocator()
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            AZ::Debug::DrillerManager::Destroy(m_drillerManager);
        }
    };

    /**
    * RAII wrapper of AllocatorBase.
    * The benefit of using this wrapper instead of AllocatorsTestFixture is that SetUp/TearDown of the allocator is managed
    * on construction/destruction, allowing member variables of derived classes to exist as value (and do heap allocation).
    */
    class ScopedAllocatorSetupFixture 
        : public ::testing::Test
        , AllocatorsBase
    {
    public:
        ScopedAllocatorSetupFixture() { SetupAllocator(); }
        ~ScopedAllocatorSetupFixture() { TeardownAllocator(); }
    };

    /**
    * Helper class to handle the boiler plate of setting up a test fixture that uses the system alloctors
    * If you wish to do additional setup and tear down be sure to call the base class SetUp first and TearDown
    * last.
    * By default memory tracking through driller is enabled.
    * Defaults to a heap size of 15 MB
    */

    class AllocatorsTestFixture
        : public ::testing::Test
        , public AllocatorsBase
    {
    public:
        //GTest interface
        void SetUp() override
        {
            SetupAllocator();
        }

        void TearDown() override
        {
            TeardownAllocator();
        }
    };

    //Legacy alias to avoid needing to modify tons of files
    //-- Do not use for new tests.
    using AllocatorsFixture = AllocatorsTestFixture;

#if defined(HAVE_BENCHMARK)
    /**
    * Helper class to handle the boiler plate of setting up a benchmark fixture that uses the system alloctors
    * If you wish to do additional setup and tear down be sure to call the base class SetUp first and TearDown
    * last.
    * By default memory tracking through driller is disabled.
    * Defaults to a heap size of 15 MB
    */
    class AllocatorsBenchmarkFixture
        : public ::benchmark::Fixture
        , public AllocatorsBase
    {
    public:
        //Benchmark interface
        void SetUp(::benchmark::State& st) override
        {
            AZ_UNUSED(st);
            SetupAllocator();
        }

        void TearDown(::benchmark::State& st) override
        {
            AZ_UNUSED(st);
            TeardownAllocator();
        }
    };
#endif

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
