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

#include "UserTypes.h"

#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/allocator_concurrent_static.h>


using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    static constexpr size_t s_allocatorCapacity = 1024;
    static constexpr size_t s_numberThreads = 4;

    template <typename AllocatorType>
    class ConcurrentAllocatorTestFixture
        : public AllocatorsTestFixture
    {
    protected:
        using this_type = ConcurrentAllocatorTestFixture<AllocatorType>;
        using allocator_type = AllocatorType;
    };

    struct NodeType
    {
        int m_number;
    };

    using AllocatorTypes = ::testing::Types<
        AZStd::static_pool_concurrent_allocator<NodeType, s_allocatorCapacity>
    >;
    TYPED_TEST_CASE(ConcurrentAllocatorTestFixture, AllocatorTypes);

    TYPED_TEST(ConcurrentAllocatorTestFixture, Name)
    {
        const char name[] = "My test allocator";
        allocator_type myalloc(name);
        EXPECT_EQ(0, strcmp(myalloc.get_name(), name));
        {
            const char newName[] = "My new test allocator";
            myalloc.set_name(newName);
            EXPECT_EQ(0, strcmp(myalloc.get_name(), newName));
            EXPECT_EQ(sizeof(allocator_type::value_type) * s_allocatorCapacity, myalloc.get_max_size());
        }
    }

    TYPED_TEST(ConcurrentAllocatorTestFixture, AllocateDeallocate)
    {
        allocator_type myalloc;

        EXPECT_EQ(0, myalloc.get_allocated_size());
        allocator_type::pointer_type data = myalloc.allocate();
        EXPECT_NE(nullptr, data);
        EXPECT_EQ(sizeof(allocator_type::value_type), myalloc.get_allocated_size());
        EXPECT_EQ(sizeof(allocator_type::value_type) * (s_allocatorCapacity - 1), myalloc.get_max_size());
        myalloc.deallocate(data);
        EXPECT_EQ(0, myalloc.get_allocated_size());
        EXPECT_EQ(sizeof(allocator_type::value_type) * s_allocatorCapacity, myalloc.get_max_size());
    }

    TYPED_TEST(ConcurrentAllocatorTestFixture, MultipleAllocateDeallocate)
    {
        allocator_type myalloc;

        // Allocate N (6) and free half (evens)
        constexpr size_t dataSize = 6; // keep this number even
        allocator_type::pointer_type data[dataSize];
        AZStd::set<allocator_type::pointer_type> dataSet; // to test for uniqueness
        for (size_t i = 0; i < dataSize; ++i)
        {
            data[i] = myalloc.allocate();
            EXPECT_NE(nullptr, data[i]);
            dataSet.insert(data[i]);
        }
        EXPECT_EQ(dataSize, dataSet.size());
        dataSet.clear();
        EXPECT_EQ(sizeof(allocator_type::value_type) * dataSize, myalloc.get_allocated_size());
        EXPECT_EQ((s_allocatorCapacity - dataSize) * sizeof(allocator_type::value_type), myalloc.get_max_size());
        for (size_t i = 0; i < dataSize; i += 2)
        {
            myalloc.deallocate(data[i]);
        }
        EXPECT_EQ(sizeof(allocator_type::value_type) * (dataSize / 2), myalloc.get_allocated_size());
        EXPECT_EQ((s_allocatorCapacity - dataSize / 2) * sizeof(allocator_type::value_type), myalloc.get_max_size());
        for (size_t i = 1; i < dataSize; i += 2)
        {
            myalloc.deallocate(data[i]);
        }
        EXPECT_EQ(0, myalloc.get_allocated_size());
        EXPECT_EQ(s_allocatorCapacity * sizeof(allocator_type::value_type), myalloc.get_max_size());
    }

    TYPED_TEST(ConcurrentAllocatorTestFixture, ConcurrentAllocateoDeallocate)
    {
        allocator_type myalloc;

        AZStd::atomic<int> failures{ 0 };
        AZStd::array<AZStd::thread, s_numberThreads> threads;
        for (size_t i = 0; i < s_numberThreads; ++i)
        {
            threads[i] = AZStd::thread([&myalloc, &failures]
            {
                // We have 4 threads, each thread can allocate at most s_allocatorCapacity/s_numberThreads values.
                // The amount of iterations do not affect since each thread will free all the values before the next
                // iteration
                constexpr size_t numIterations = 100;
                constexpr size_t numValues = s_allocatorCapacity / s_numberThreads;
                AZStd::array<allocator_type::pointer_type, numValues> allocations;
                for (int iter = 0; iter < numIterations; ++iter)
                {
                    // allocate
                    for (int i = 0; i < numValues; ++i)
                    {
                        allocations[i] = myalloc.allocate();
                        if (!allocations[i])
                        {
                            ++failures;
                        }
                    }
                    // deallocate
                    for (int i = 0; i < numValues; ++i)
                    {
                        myalloc.deallocate(allocations[i]);
                        allocations[i] = nullptr;
                    }
                }
            });
        }
        for (size_t i = 0; i < s_numberThreads; ++i)
        {
            threads[i].join();
        }
        EXPECT_EQ(0, failures);
        EXPECT_EQ(0, myalloc.get_allocated_size());
    }

    using StaticPoolConcurrentAllocatorTestFixture = AllocatorsTestFixture;

    TEST(StaticPoolConcurrentAllocatorTestFixture, Aligment)
    {
        // static pool allocator
        // Generally we can't use more then 16 byte alignment on the stack.
        // Some platforms might fail. Which is ok, higher alignment should be handled by US. Or not on the stack.
        const int dataAlignment = 16;

        typedef aligned_storage<sizeof(int), dataAlignment>::type aligned_int_type;
        typedef AZStd::static_pool_concurrent_allocator<aligned_int_type, s_allocatorCapacity> aligned_int_node_pool_type;
        aligned_int_node_pool_type myaligned_pool;
        aligned_int_type* aligned_data = reinterpret_cast<aligned_int_type*>(myaligned_pool.allocate(sizeof(aligned_int_type), dataAlignment));

        EXPECT_NE(nullptr, aligned_data);
        EXPECT_EQ(0, ((AZStd::size_t)aligned_data & (dataAlignment - 1)));
        EXPECT_EQ((s_allocatorCapacity - 1) * sizeof(aligned_int_type), myaligned_pool.get_max_size());
        EXPECT_EQ(sizeof(aligned_int_type), myaligned_pool.get_allocated_size());

        myaligned_pool.deallocate(aligned_data, sizeof(aligned_int_type), dataAlignment); // Make sure we free what we have allocated.
    }
}
