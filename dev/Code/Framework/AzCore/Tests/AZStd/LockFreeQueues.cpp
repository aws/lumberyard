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

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/containers/lock_free_queue.h>
#include <AzCore/std/parallel/containers/lock_free_stamped_queue.h>
#include <AzCore/std/parallel/containers/work_stealing_queue.h>
#include <AzCore/std/functional.h>

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    class LockFreeQueue
        : public AllocatorsFixture
    {
    protected:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 5000;
#else
        static const int NUM_ITERATIONS = 100000;
#endif
    public:
        template <class Q>
        void Push(Q* queue)
        {
            for (int i = 0; i < NUM_ITERATIONS; ++i)
            {
                queue->push(i);
            }
        }

        template <class Q>
        void Pop(Q* queue)
        {
            int expected = 0;
            while (expected < NUM_ITERATIONS)
            {
                int value = NUM_ITERATIONS;
                if (queue->pop(&value))
                {
                    if (value == expected)
                    {
                        ++m_counter;
                    }
                    ++expected;
                }
            }
        }

        atomic<int> m_counter;
    };


    TEST_F(LockFreeQueue, LockFreeQueue)
    {
        lock_free_queue<int, MyLockFreeAllocator> queue;
        int result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        queue.push(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        {
            m_counter = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeQueue::Push<decltype(queue)>, this, &queue));
            AZStd::thread thread1(AZStd::bind(&LockFreeQueue::Pop<decltype(queue)>, this, &queue));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_counter == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }

    TEST_F(LockFreeQueue, LockFreeStampedQueue)
    {
        lock_free_stamped_queue<int, MyLockFreeAllocator> queue;

        int result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        queue.push(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        {
            m_counter = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeQueue::Push<decltype(queue)>, this, &queue));
            AZStd::thread thread1(AZStd::bind(&LockFreeQueue::Pop<decltype(queue)>, this, &queue));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_counter == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }

    /**
     * work_stealing_queue container test.
     */
    class WorkStealingQueue
        : public AllocatorsFixture
    {
    public:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 5000;
#else
        static const int NUM_ITERATIONS = 50000;
#endif
        void PushAndPop(work_stealing_queue<int>* queue)
        {
            //push
            for (int i = 1; i <= NUM_ITERATIONS; ++i)
            {
                queue->local_push_bottom(i);
                m_total.fetch_add(i);
            }
            //pop
            while (m_numPopped < NUM_ITERATIONS)
            {
                int value = 0;
                if (queue->local_pop_bottom(&value))
                {
                    m_total.fetch_sub(value);
                    ++m_numPopped;
                }
            }
        }
        void Steal(work_stealing_queue<int>* queue)
        {
            while (m_numPopped < NUM_ITERATIONS)
            {
                int value = 0;
                if (queue->steal_top(&value))
                {
                    m_total.fetch_sub(value);
                    ++m_numPopped;
                }
            }
        }
        
        atomic<int> m_total;
        atomic<int> m_numPopped;
    };

    TEST_F(WorkStealingQueue, Test)
    {
        work_stealing_queue<int> queue;

        int result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.local_pop_bottom(&result));

        queue.local_push_bottom(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.local_pop_bottom(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.local_pop_bottom(&result));

        queue.local_push_bottom(20);
        queue.local_push_bottom(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.local_pop_bottom(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.local_pop_bottom(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.local_pop_bottom(&result));

        AZ_TEST_ASSERT(!queue.steal_top(&result));
        queue.local_push_bottom(20);
        queue.local_push_bottom(30);
        queue.local_push_bottom(40);
        queue.local_push_bottom(50);
        AZ_TEST_ASSERT(queue.steal_top(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.local_pop_bottom(&result));
        AZ_TEST_ASSERT(result == 50);
        AZ_TEST_ASSERT(queue.steal_top(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.steal_top(&result));
        AZ_TEST_ASSERT(result == 40);
        AZ_TEST_ASSERT(!queue.steal_top(&result));

        {
            m_total = 0;
            m_numPopped = 0;
            AZStd::thread thread0(AZStd::bind(&WorkStealingQueue::PushAndPop, this, &queue));
            AZStd::thread thread1(AZStd::bind(&WorkStealingQueue::Steal, this, &queue));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(m_numPopped == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }
}
