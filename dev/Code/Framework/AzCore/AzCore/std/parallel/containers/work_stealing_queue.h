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
#ifndef AZSTD_PARALLEL_CONTAINERS_WORK_STEALING_QUEUE_H
#define AZSTD_PARALLEL_CONTAINERS_WORK_STEALING_QUEUE_H 1

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/std/parallel/lock.h>

namespace AZStd
{
    /**
     * A queue used for work stealing, it provides fast push/pop at the bottom of the queue for a single thread only,
     * and any thread can steal from the top of the queue. Implementation is lock-free and wait-free. The queue is
     * unbounded, it will resize itself as necessary. The CollectGarbage method should be called to clean up memory
     * discarded by resizes, this can only be done when no other push/pop/steal operations are in progress.
     */
#if 1
    template<typename T, class Allocator = AZStd::allocator>
    class work_stealing_queue
    {
        //This implementation is based on the C# implementation from Joe Duffy:
        // http://www.bluebytesoftware.com/blog/2008/08/12/BuildingACustomThreadPoolSeriesPart2AWorkStealingQueue.aspx
        //It requires an interlocked operation to pop a job, even from the local queue. Steals always require a lock.
        //Despite these issues, it has the nice feature that it actually works correctly, unlike the waste-of-time,
        //piece-of-crap version below.
    public:
        typedef AZStd::spin_mutex MutexType;

        work_stealing_queue()
        {
            m_array.store(create_array(32), memory_order_release);
            m_top.store(0, memory_order_release);
            m_bottom.store(0, memory_order_release);
        }

        ~work_stealing_queue()
        {
            collect_garbage();
            destroy_array(m_array.load(memory_order_acquire));
        }

        bool empty() const
        {
            int top = m_top.load(memory_order_acquire);  //order is important, must read m_top before m_bottom
            int bottom = m_bottom.load(memory_order_acquire);
            return bottom <= top;
        }

        unsigned int size() const
        {
            int top = m_top.load(memory_order_acquire);  //order is important, must read m_top before m_bottom
            int bottom = m_bottom.load(memory_order_acquire);
            return bottom - top;
        }

        void local_push_bottom(const T& item)
        {
            int oldBottom = m_bottom.load(memory_order_acquire);
            int oldTop = m_top.load(memory_order_acquire);
            CircularArray* currentArray = m_array.load(memory_order_acquire);
            int size = oldBottom - oldTop;
            if (size < currentArray->GetCapacity() - 1)
            {
                currentArray->SetItem(oldBottom, item);
                m_bottom.store(oldBottom + 1, memory_order_release);
            }
            else
            {
                lock_guard<MutexType> lock(m_foreignLock);

                //check if resize still needed
                oldTop = m_top.load(memory_order_acquire);
                size = oldBottom - oldTop;

                if (size >= currentArray->GetCapacity() - 1)
                {
                    CircularArray* newArray = create_array(currentArray->GetCapacity() * 2);
                    for (int i = 0; i < currentArray->GetCapacity(); ++i)
                    {
                        newArray->SetItem(i, currentArray->GetItem(i + oldTop));
                    }
                    m_garbageArrays.push_back(currentArray);
                    currentArray = newArray;

                    m_array.store(newArray, memory_order_release);
                    m_top.store(0, memory_order_release);
                    m_bottom.store(size, memory_order_release);
                    oldBottom = size;
                }

                currentArray->SetItem(oldBottom, item);
                m_bottom.store(oldBottom + 1, memory_order_release);
            }
        }

        bool local_pop_bottom(T* item_out)
        {
            int oldBottom = m_bottom.load(memory_order_acquire);
            int oldTop = m_top.load(memory_order_acquire);
            if (oldTop >= oldBottom)
            {
                return false;
            }

            --oldBottom;
            m_bottom.exchange(oldBottom, memory_order_acq_rel);

            oldTop = m_top.load(memory_order_acquire);
            if (oldTop <= oldBottom)
            {
                *item_out = m_array.load(memory_order_acquire)->GetItem(oldBottom);
                return true;
            }
            else
            {
                lock_guard<MutexType> lock(m_foreignLock);

                oldTop = m_top.load(memory_order_acquire);
                if (oldTop <= oldBottom)
                {
                    *item_out = m_array.load(memory_order_acquire)->GetItem(oldBottom);
                    return true;
                }
                else
                {
                    m_bottom.store(oldBottom + 1, memory_order_release);
                    return false;
                }
            }
        }

        bool steal_top(T* item_out)
        {
            lock_guard<MutexType> lock(m_foreignLock);

            int oldTop = m_top.load(memory_order_acquire);
            m_top.exchange(oldTop + 1, memory_order_acq_rel);

            int oldBottom = m_bottom.load(memory_order_acquire);
            if (oldTop < oldBottom)
            {
                *item_out = m_array.load(memory_order_acquire)->GetItem(oldTop);
                return true;
            }
            else
            {
                m_top.store(oldTop, memory_order_release);
                return false;
            }
        }

        /**
         * Clean up any memory discarded by resize operations. This function can only be called when there are no
         * other push/pop/steal operations in progress.
         */
        void collect_garbage()
        {
            for (unsigned int i = 0; i < m_garbageArrays.size(); ++i)
            {
                destroy_array(m_garbageArrays[i]);
            }
            m_garbageArrays.clear();
        }

    private:
        class CircularArray
        {
        public:
            CircularArray(int capacity)
                : m_capacity(capacity)
            {
                AZ_Assert((capacity & (capacity - 1)) == 0, "Capacity must be a power of 2");
                m_array.resize(capacity);
                m_mask = capacity - 1;
            }
            void SetItem(int i, const T& item) { m_array[i & m_mask] = item; }
            const T& GetItem(int i)            { return m_array[i & m_mask]; }
            int GetCapacity() const            { return m_capacity; }
        private:
            vector<T, Allocator> m_array;
            int m_capacity;
            int m_mask;
        };

        CircularArray* create_array(int capacity)
        {
            return new(m_allocator.allocate(sizeof(CircularArray), alignment_of<CircularArray>::value))
                   CircularArray(capacity);
        }

        void destroy_array(CircularArray* array)
        {
            array->~CircularArray();
            m_allocator.deallocate(array, sizeof(CircularArray), alignment_of<CircularArray>::value);
        }

        atomic<int> m_bottom;
        atomic<int> m_top;
        atomic<CircularArray*> m_array;
        MutexType m_foreignLock;
        vector<CircularArray*, Allocator> m_garbageArrays;
        Allocator m_allocator;
    };

#else

    template<typename T, class Allocator = AZStd::allocator>
    class work_stealing_queue
    {
        //Implementation based on Chapter 16.5, The Art of Multiprocessor Programming by Herlihy & Shavitt.
        //Has the nice feature that local pushes/pops do not need any interlocked operations when the queue is non-empty,
        //some other implementations have a CAS for every local pop.
        //NOTE: It doesn't work! It passes my tests, but in practice we get extremely rare crashes, leading me to think
        //      there is a very subtle memory ordering issue here.
    public:
        work_stealing_queue()
        {
            m_array.store(create_array(32), memory_order_release);
            m_top.store(0, memory_order_release);
            m_bottom.store(0, memory_order_release);
        }

        ~work_stealing_queue()
        {
            collect_garbage();
            destroy_array(m_array.load(memory_order_acquire));
        }

        bool empty() const
        {
            int top = m_top.load(memory_order_acquire);  //order is important, must read m_top before m_bottom
            int bottom = m_bottom.load(memory_order_acquire);
            return bottom <= top;
        }

        unsigned int size() const
        {
            int top = m_top.load(memory_order_acquire);  //order is important, must read m_top before m_bottom
            int bottom = m_bottom.load(memory_order_acquire);
            return bottom - top;
        }

        void local_push_bottom(const T& item)
        {
            int oldBottom = m_bottom.load(memory_order_acquire);
            int oldTop = m_top.load(memory_order_acquire);
            CircularArray* currentArray = m_array.load(memory_order_acquire);
            int size = oldBottom - oldTop;
            if (size >= currentArray->GetCapacity() - 1)
            {
                CircularArray* newArray = create_array(currentArray->GetCapacity() * 2);
                for (int i = oldTop; i < oldBottom; ++i)
                {
                    newArray->SetItem(i, currentArray->GetItem(i));
                }
                m_garbageArrays.push_back(currentArray);
                currentArray = newArray;
                m_array.store(newArray, memory_order_release);
            }
            currentArray->SetItem(oldBottom, item);
            m_bottom.store(oldBottom + 1, memory_order_release);
        }

        bool local_pop_bottom(T* item_out)
        {
            m_bottom.store(m_bottom.load(memory_order_acquire) - 1, memory_order_release);
            int oldTop = m_top.load(memory_order_acquire);
            int newTop = oldTop + 1;
            int size = m_bottom.load(memory_order_acquire) - oldTop;
            if (size < 0)
            {
                m_bottom.store(oldTop, memory_order_release);
                return false;
            }
            *item_out = m_array.load(memory_order_acquire)->GetItem(m_bottom.load(memory_order_acquire));
            if (size > 0)
            {
                return true;
            }
            bool isSuccess = true;
            int expectedTop = oldTop;
            if (!m_top.compare_exchange_strong(expectedTop, newTop, memory_order_acq_rel, memory_order_acquire))
            {
                isSuccess = false;
            }
            m_bottom.store(oldTop + 1, memory_order_release);
            return isSuccess;
        }

        bool steal_top(T* item_out)
        {
            int oldTop = m_top.load(memory_order_acquire);
            int newTop = oldTop + 1;
            int oldBottom = m_bottom.load(memory_order_acquire);
            int size = oldBottom - oldTop;
            if (size <= 0)
            {
                return false;
            }
            *item_out = m_array.load(memory_order_acquire)->GetItem(oldTop);
            if (m_top.compare_exchange_strong(oldTop, newTop, memory_order_acq_rel, memory_order_acquire))  //could possibly be weak
            {
                return true;
            }
            return false;
        }

        /**
         * Clean up any memory discarded by resize operations. This function can only be called when there are no
         * other push/pop/steal operations in progress.
         */
        void collect_garbage()
        {
            for (unsigned int i = 0; i < m_garbageArrays.size(); ++i)
            {
                destroy_array(m_garbageArrays[i]);
            }
            m_garbageArrays.clear();
        }

    private:
        class CircularArray
        {
        public:
            CircularArray(int capacity)
                : m_capacity(capacity)
            {
                AZ_Assert((capacity & (capacity - 1)) == 0, "Capacity must be a power of 2");
                m_array.resize(capacity);
                m_mask = capacity - 1;
            }
            void SetItem(int i, const T& item) { m_array[i & m_mask] = item; }
            const T& GetItem(int i)            { return m_array[i & m_mask]; }
            int GetCapacity() const            { return m_capacity; }
        private:
            vector<T, Allocator> m_array;
            int m_capacity;
            int m_mask;
        };

        CircularArray* create_array(int capacity)
        {
            return new(m_allocator.allocate(sizeof(CircularArray), alignment_of<CircularArray>::value))
                   CircularArray(capacity);
        }

        void destroy_array(CircularArray* array)
        {
            array->~CircularArray();
            m_allocator.deallocate(array, sizeof(CircularArray), alignment_of<CircularArray>::value);
        }

        atomic<int> m_bottom;
        atomic<int> m_top;
        atomic<CircularArray*> m_array;
        vector<CircularArray*, Allocator> m_garbageArrays;
        Allocator m_allocator;
    };
#endif
}

#endif
#pragma once