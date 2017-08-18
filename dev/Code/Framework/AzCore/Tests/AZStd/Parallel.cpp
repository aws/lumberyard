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

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/combinable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/shared_mutex.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/std/chrono/chrono.h>

#include <AzCore/Memory/SystemAllocator.h>

using namespace AZStd;
using namespace AZStd::placeholders;
using namespace UnitTestInternal;

namespace UnitTest
{
    /**
     * Atomic integral type test
     */
    template<typename T, typename BaseType>
    class AtomicIntegralTest
        : public ::testing::Test
    {
    public:
        void run()
        {
            T v1;       //default constructor
            T v2(10);   //init constructor
            v1 = 20;    //assignment operator
            //T v3(v2); //copy constructor, should be deleted, and should not compile
            //v1 = v2;  //assignment from another atomic, should be deleted, and should not compile
            AZ_TEST_ASSERT(v1 == 20);
            AZ_TEST_ASSERT(v2 == 10);
            BaseType x = v1; //cast operator
            AZ_TEST_ASSERT(x == 20);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
            AZ_TEST_ASSERT(v1.is_lock_free());
#else
            AZ_TEST_ASSERT(!v1.is_lock_free());
#endif

            v1.store(50);
            AZ_TEST_ASSERT(v1 == 50);
            v1.store(60, memory_order_relaxed);
            AZ_TEST_ASSERT(v1 == 60);
            v1.store(70, memory_order_release);
            AZ_TEST_ASSERT(v1 == 70);
            v1.store(80, memory_order_seq_cst);
            AZ_TEST_ASSERT(v1 == 80);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
            AZ_TEST_START_ASSERTTEST;
            v1.store(100, memory_order_consume);
            v1.store(100, memory_order_acquire);
            v1.store(100, memory_order_acq_rel);
            AZ_TEST_STOP_ASSERTTEST(3);
#endif // AZ_USE_STD_ATOMIC

            v1 = 42;
            AZ_TEST_ASSERT(v1.load() == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_consume) == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_acquire) == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == 42);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
            AZ_TEST_START_ASSERTTEST;
            v1.load(memory_order_release);
            v1.load(memory_order_acq_rel);
            AZ_TEST_STOP_ASSERTTEST(2);
#endif // AZ_USE_STD_ATOMIC

            v1.store(10, memory_order_relaxed);
            AZ_TEST_ASSERT(v1 == 10);
            v1.store(20, memory_order_release);
            AZ_TEST_ASSERT(v1 == 20);
            v1.store(30, memory_order_seq_cst);
            AZ_TEST_ASSERT(v1 == 30);

            v1 = 42;
            AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_consume) == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_acquire) == 42);
            AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == 42);

            AZ_TEST_ASSERT(v1.exchange(25) == 42);
            AZ_TEST_ASSERT(v1 == 25);

            BaseType expected = 100;
            AZ_TEST_ASSERT(!v1.compare_exchange_strong(expected, 50));
            AZ_TEST_ASSERT(expected == 25);
            AZ_TEST_ASSERT(v1 == 25);
            expected = 25;
            AZ_TEST_ASSERT(v1.compare_exchange_strong(expected, 50));
            AZ_TEST_ASSERT(expected == 25);
            AZ_TEST_ASSERT(v1 == 50);

            AZ_TEST_ASSERT(!v1.compare_exchange_weak(expected, 75));
            AZ_TEST_ASSERT(expected == 50);
            AZ_TEST_ASSERT(v1 == 50);
            expected = 50;
            while (!v1.compare_exchange_weak(expected, 75))
            {
            }
            AZ_TEST_ASSERT(expected == 50);
            AZ_TEST_ASSERT(v1 == 75);

            AZ_TEST_ASSERT(v1.fetch_add(10) == 75);
            AZ_TEST_ASSERT(v1 == 85);

            AZ_TEST_ASSERT(v1.fetch_sub(15) == 85);
            AZ_TEST_ASSERT(v1 == 70);

            v1 = 0x75;
            AZ_TEST_ASSERT(v1.fetch_and(0x0f) == 0x75);
            AZ_TEST_ASSERT(v1 == 0x05);

            AZ_TEST_ASSERT(v1.fetch_or(0x73) == 0x05);
            AZ_TEST_ASSERT(v1 == 0x77);

            AZ_TEST_ASSERT(v1.fetch_xor(0x11) == 0x77);
            AZ_TEST_ASSERT(v1 == 0x66);

            AZ_TEST_ASSERT(v1.fetch_add(1, memory_order_seq_cst) == 0x66);
            AZ_TEST_ASSERT(v1 == 0x67);

            AZ_TEST_ASSERT(v1.fetch_sub(1, memory_order_seq_cst) == 0x67);
            AZ_TEST_ASSERT(v1 == 0x66);

            v1 = 100;
            AZ_TEST_ASSERT((v1++) == 100);
            AZ_TEST_ASSERT(v1 == 101);

            AZ_TEST_ASSERT((++v1) == 102);
            AZ_TEST_ASSERT(v1 == 102);

            AZ_TEST_ASSERT((v1--) == 102);
            AZ_TEST_ASSERT(v1 == 101);

            AZ_TEST_ASSERT((--v1) == 100);
            AZ_TEST_ASSERT(v1 == 100);

            v1 = 0x50;
            AZ_TEST_ASSERT((v1 += 0x10) == 0x60);
            AZ_TEST_ASSERT(v1 == 0x60);

            AZ_TEST_ASSERT((v1 -= 0x01) == 0x5f);
            AZ_TEST_ASSERT(v1 == 0x5f);

            AZ_TEST_ASSERT((v1 &= 0x63) == 0x43);
            AZ_TEST_ASSERT(v1 == 0x43);

            AZ_TEST_ASSERT((v1 |= 0x74) == 0x77);
            AZ_TEST_ASSERT(v1 == 0x77);

            AZ_TEST_ASSERT((v1 ^= 0x68) == 0x1f);
            AZ_TEST_ASSERT(v1 == 0x1f);
        }
    };

    /**
     * Atomic boolean type test
     */
    TEST(AtomicBool, Test)
    {
        atomic_bool v1;       //default constructor
        atomic_bool v2(true);   //init constructor
        v1 = false;    //assignment operator
        //T v3(v2); //copy constructor, should be deleted, and should not compile
        //v1 = v2;  //assignment from another atomic, should be deleted, and should not compile
        AZ_TEST_ASSERT(v1 == false);
        AZ_TEST_ASSERT(v2 == true);
        bool x = v1; //cast operator
        AZ_TEST_ASSERT(x == false);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        AZ_TEST_ASSERT(v1.is_lock_free());
#else
        AZ_TEST_ASSERT(!v1.is_lock_free());
#endif

        v1.store(true);
        AZ_TEST_ASSERT(v1 == true);
        v1.store(false, memory_order_relaxed);
        AZ_TEST_ASSERT(v1 == false);
        v1.store(true, memory_order_release);
        AZ_TEST_ASSERT(v1 == true);
        v1.store(false, memory_order_seq_cst);
        AZ_TEST_ASSERT(v1 == false);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
        AZ_TEST_START_ASSERTTEST;
        v1.store(true, memory_order_consume);
        v1.store(true, memory_order_acquire);
        v1.store(true, memory_order_acq_rel);
        AZ_TEST_STOP_ASSERTTEST(3);
#endif // AZ_USE_STD_ATOMIC

        v1 = true;
        AZ_TEST_ASSERT(v1.load() == true);
        AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == true);
        AZ_TEST_ASSERT(v1.load(memory_order_consume) == true);
        AZ_TEST_ASSERT(v1.load(memory_order_acquire) == true);
        AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == true);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
        AZ_TEST_START_ASSERTTEST;
        v1.load(memory_order_release);
        v1.load(memory_order_acq_rel);
        AZ_TEST_STOP_ASSERTTEST(2);
#endif // AZ_USE_STD_ATOMIC

        v1.store(true, memory_order_relaxed);
        AZ_TEST_ASSERT(v1 == true);
        v1.store(false, memory_order_release);
        AZ_TEST_ASSERT(v1 == false);
        v1.store(true, memory_order_seq_cst);
        AZ_TEST_ASSERT(v1 == true);

        v1 = false;
        AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == false);
        AZ_TEST_ASSERT(v1.load(memory_order_consume) == false);
        AZ_TEST_ASSERT(v1.load(memory_order_acquire) == false);
        AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == false);

        AZ_TEST_ASSERT(v1.exchange(true) == false);
        AZ_TEST_ASSERT(v1 == true);

        v1 = false;
        bool expected = true;
        AZ_TEST_ASSERT(!v1.compare_exchange_strong(expected, false));
        AZ_TEST_ASSERT(expected == false);
        AZ_TEST_ASSERT(v1 == false);
        expected = false;
        AZ_TEST_ASSERT(v1.compare_exchange_strong(expected, true));
        AZ_TEST_ASSERT(expected == false);
        AZ_TEST_ASSERT(v1 == true);

        AZ_TEST_ASSERT(!v1.compare_exchange_weak(expected, false));
        AZ_TEST_ASSERT(expected == true);
        AZ_TEST_ASSERT(v1 == true);
        expected = true;
        while (!v1.compare_exchange_weak(expected, false))
        {
        }
        AZ_TEST_ASSERT(expected == true);
        AZ_TEST_ASSERT(v1 == false);
    }

    struct TestStruct
    {
#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
        // Currenly if we use int m_x, m_y; (as of 4.8) gcc issues __atomic_load_8,
        // which can be find in libatomic.a, but it's load() doesn't seem to function properly
        // and fails to load some values. Using unions temporary avoids using the __atomic_load_8
        // In general if this is not addressed soon, we should switch to our atomic implementation
        void set(int x, int y) { m_value.m_x = x; m_value.m_y = y; }
        bool operator==(const TestStruct& rhs) const { return (rhs.m_value.m_x == m_value.m_x) && (rhs.m_value.m_y == m_value.m_y); }
        union
        {
            struct
            {
                int m_x, m_y;
            } m_value;
            AZ::u64  m_compilerTrick;
        };
#else
        void set(int x, int y) { m_x = x; m_y = y; }
        bool operator==(const TestStruct& rhs) const { return (rhs.m_x == m_x) && (rhs.m_y == m_y); }
        int m_x, m_y;
#endif
    };

    TEST(AtomicGeneric, Test)
    {
        TestStruct s0;
        s0.set(0, 1);
        TestStruct s1;
        s1.set(2, 3);
        TestStruct s2;
        s2.set(4, 5);
        TestStruct s3;
        s3.set(6, 7);
        TestStruct s4;
        s4.set(8, 9);
        TestStruct s5;
        s5.set(10, 11);

        atomic<TestStruct> v1;       //default constructor
        atomic<TestStruct> v2(s0);   //init constructor
        v1 = s1;    //assignment operator
        //T v3(v2); //copy constructor, should be deleted, and should not compile
        //v1 = v2;  //assignment from another atomic, should be deleted, and should not compile
        AZ_TEST_ASSERT((TestStruct&)(v1) == s1);
        AZ_TEST_ASSERT((TestStruct&)(v2) == s0);
        TestStruct x = v1; //cast operator
        AZ_TEST_ASSERT(x == s1);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        AZ_TEST_ASSERT(v1.is_lock_free());
#else
        AZ_TEST_ASSERT(!v1.is_lock_free());
#endif

        v1.store(s0);
        AZ_TEST_ASSERT((TestStruct)(v1) == s0);
        v1.store(s1, memory_order_relaxed);
        AZ_TEST_ASSERT((TestStruct)(v1) == s1);
        v1.store(s2, memory_order_release);
        AZ_TEST_ASSERT((TestStruct)(v1) == s2);
        v1.store(s3, memory_order_seq_cst);
        AZ_TEST_ASSERT((TestStruct)(v1) == s3);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
        AZ_TEST_START_ASSERTTEST;
        v1.store(s4, memory_order_consume);
        v1.store(s4, memory_order_acquire);
        v1.store(s4, memory_order_acq_rel);
        AZ_TEST_STOP_ASSERTTEST(3);
#endif // AZ_USE_STD_ATOMIC

        v1 = s4;
        AZ_TEST_ASSERT(v1.load() == s4);
        AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == s4);
        AZ_TEST_ASSERT(v1.load(memory_order_consume) == s4);
        AZ_TEST_ASSERT(v1.load(memory_order_acquire) == s4);
        AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == s4);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
        AZ_TEST_START_ASSERTTEST;
        v1.load(memory_order_release);
        v1.load(memory_order_acq_rel);
        AZ_TEST_STOP_ASSERTTEST(2);
#endif // AZ_USE_STD_ATOMIC

        v1.store(s0, memory_order_relaxed);
        AZ_TEST_ASSERT((TestStruct)(v1) == s0);
        v1.store(s1, memory_order_release);
        AZ_TEST_ASSERT((TestStruct)(v1) == s1);
        v1.store(s2, memory_order_seq_cst);
        AZ_TEST_ASSERT((TestStruct)(v1) == s2);

        v1 = s0;
        AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == s0);
        AZ_TEST_ASSERT(v1.load(memory_order_consume) == s0);
        AZ_TEST_ASSERT(v1.load(memory_order_acquire) == s0);
        AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == s0);

        AZ_TEST_ASSERT(v1.exchange(s1) == s0);
        AZ_TEST_ASSERT((TestStruct)(v1) == s1);

        TestStruct expected = s2;
        AZ_TEST_ASSERT(!v1.compare_exchange_strong(expected, s3));
        AZ_TEST_ASSERT(expected == s1);
        AZ_TEST_ASSERT((TestStruct)(v1) == s1);
        expected = s1;
        AZ_TEST_ASSERT(v1.compare_exchange_strong(expected, s3));
        AZ_TEST_ASSERT(expected == s1);
        AZ_TEST_ASSERT((TestStruct)(v1) == s3);

        AZ_TEST_ASSERT(!v1.compare_exchange_weak(expected, s4));
        AZ_TEST_ASSERT(expected == s3);
        AZ_TEST_ASSERT((TestStruct)(v1) == s3);
        expected = s3;
        while (!v1.compare_exchange_weak(expected, s4))
        {
        }
        AZ_TEST_ASSERT(expected == s3);
        AZ_TEST_ASSERT((TestStruct)(v1) == s4);
    }

    /**
     * Atomic address type test
     */
    template<typename T>
    class AtomicGenericPointerTest
    {
    public:
        void run()
        {
            T dummy[6];
            T* p0 = &dummy[0];
            T* p1 = &dummy[1];
            T* p2 = &dummy[2];
            T* p3 = &dummy[3];
            T* p4 = &dummy[4];
            T* p5 = &dummy[5];

            atomic<T*> v1;       //default constructor
            atomic<T*> v2(p0);   //init constructor
            v1 = p1;    //assignment operator
            //T v3(v2); //copy constructor, should be deleted, and should not compile
            //v1 = v2;  //assignment from another atomic, should be deleted, and should not compile
            AZ_TEST_ASSERT(v1 == p1);
            AZ_TEST_ASSERT(v2 == p0);
            void* x = v1; //cast operator
            AZ_TEST_ASSERT(x == p1);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_PS4) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
            AZ_TEST_ASSERT(v1.is_lock_free());
#else
            AZ_TEST_ASSERT(!v1.is_lock_free());
#endif

            v1.store(p0);
            AZ_TEST_ASSERT(v1 == p0);
            v1.store(p1, memory_order_relaxed);
            AZ_TEST_ASSERT(v1 == p1);
            v1.store(p2, memory_order_release);
            AZ_TEST_ASSERT(v1 == p2);
            v1.store(p3, memory_order_seq_cst);
            AZ_TEST_ASSERT(v1 == p3);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
            AZ_TEST_START_ASSERTTEST;
            v1.store(p4, memory_order_consume);
            v1.store(p4, memory_order_acquire);
            v1.store(p4, memory_order_acq_rel);
            AZ_TEST_STOP_ASSERTTEST(3);
#endif // AZ_USE_STD_ATOMIC

            v1 = p4;
            AZ_TEST_ASSERT(v1.load() == p4);
            AZ_TEST_ASSERT(v1.load(memory_order_relaxed) == p4);
            AZ_TEST_ASSERT(v1.load(memory_order_consume) == p4);
            AZ_TEST_ASSERT(v1.load(memory_order_acquire) == p4);
            AZ_TEST_ASSERT(v1.load(memory_order_seq_cst) == p4);
#if !defined(AZ_USE_STD_ATOMIC) // we can't test std::atomic for asserts
            AZ_TEST_START_ASSERTTEST;
            v1.load(memory_order_release);
            v1.load(memory_order_acq_rel);
            AZ_TEST_STOP_ASSERTTEST(2);
#endif // AZ_USE_STD_ATOMIC

            v1 = p0;
            AZ_TEST_ASSERT(v1.exchange(p1) == p0);
            AZ_TEST_ASSERT(v1 == p1);

            T* expected = p2;
            AZ_TEST_ASSERT(!v1.compare_exchange_strong(expected, p3));
            AZ_TEST_ASSERT(expected == p1);
            AZ_TEST_ASSERT(v1 == p1);
            expected = p1;
            AZ_TEST_ASSERT(v1.compare_exchange_strong(expected, p3));
            AZ_TEST_ASSERT(expected == p1);
            AZ_TEST_ASSERT(v1 == p3);

            AZ_TEST_ASSERT(!v1.compare_exchange_weak(expected, p4));
            AZ_TEST_ASSERT(expected == p3);
            AZ_TEST_ASSERT(v1 == p3);
            expected = p3;
            while (!v1.compare_exchange_weak(expected, p4))
            {
            }
            AZ_TEST_ASSERT(expected == p3);
            AZ_TEST_ASSERT(v1 == p4);

            v1 = p0;
            AZ_TEST_ASSERT(v1.fetch_add(4) == p0);
            AZ_TEST_ASSERT(v1 == p4);

            AZ_TEST_ASSERT(v1.fetch_sub(3) == p4);
            AZ_TEST_ASSERT(v1 == p1);

            v1 = p0;
            AZ_TEST_ASSERT((v1++) == p0);
            AZ_TEST_ASSERT(v1 == p1);

            AZ_TEST_ASSERT((++v1) == p2);
            AZ_TEST_ASSERT(v1 == p2);

            AZ_TEST_ASSERT((v1--) == p2);
            AZ_TEST_ASSERT(v1 == p1);

            AZ_TEST_ASSERT((--v1) == p0);
            AZ_TEST_ASSERT(v1 == p0);

            v1 = p1;
            AZ_TEST_ASSERT((v1 += 4) == p5);
            AZ_TEST_ASSERT(v1 == p5);

            AZ_TEST_ASSERT((v1 -= 1) == p4);
            AZ_TEST_ASSERT(v1 == p4);
        }
    };

    TEST(AtomicFlag, Test)
    {
        atomic_flag flag;

        flag.clear();
        AZ_TEST_ASSERT(flag.test_and_set() == false);
        AZ_TEST_ASSERT(flag.test_and_set() == true);
        AZ_TEST_ASSERT(flag.test_and_set() == true);
    }

    typedef AtomicIntegralTest<atomic_char, char> AtomicChar;
    TEST_F(AtomicChar, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_uchar, unsigned char> AtomicUnsignedChar;
    TEST_F(AtomicUnsignedChar, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_short, short> AtomicShort;
    TEST_F(AtomicShort, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_ushort, unsigned short> AtomicUnsignedShort;
    TEST_F(AtomicUnsignedShort, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_int, int> AtomicInt;
    TEST_F(AtomicInt, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_uint, unsigned int> AtomicUnsignedInt;
    TEST_F(AtomicUnsignedInt, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_long, long> AtomicLong;
    TEST_F(AtomicLong, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_ulong, unsigned long> AtomicUnsignedLong;
    TEST_F(AtomicUnsignedLong, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_llong, long long> AtomicLongLong;
    TEST_F(AtomicLongLong, Test)
    {
        run();
    }

    typedef AtomicIntegralTest<atomic_ullong, unsigned long long> AtomicUnsignedLongLong;
    TEST_F(AtomicUnsignedLongLong, Test)
    {
        run();
    }
    //typedef AtomicIntegralTest<atomic_wchar_t, wchar_t> AtomicWCharTest;

    typedef AtomicIntegralTest<atomic<int>, int> AtomicGenericIntTest;
    TEST_F(AtomicGenericIntTest, Test)
    {
        run();
    }

    /**
     * Synchronization primitives test.
     */
    TEST(Parallel, Mutex)
    {
        mutex m;
        m.lock();
        m.unlock();
    }

    TEST(Parallel, RecursiveMutex)
    {

        recursive_mutex m1;
        m1.lock();
        AZ_TEST_ASSERT(m1.try_lock());  // we should be able to lock it from the same thread again...
        m1.unlock();
        m1.unlock();
        {
            mutex m2;
            lock_guard<mutex> l(m2);
        }
    }

    TEST(Parallel, Semaphore)
    {
        semaphore sema;
        sema.release(1);
        sema.acquire();
    }

    TEST(Parallel, BinarySemaphore)
    {
        binary_semaphore event;
        event.release();
        event.acquire();
    }

    TEST(Parallel, SpinMutex)
    {

        spin_mutex sm;
        sm.lock();
        sm.unlock();
    }

    /**
     * Thread test
     */
    class Parallel_Thread
        : public AllocatorsFixture
    {
        int m_data;
        int m_dataMax;

        static const int m_threadStackSize = 32 * 1024;
        thread_desc      m_desc[3];
        int m_numThreadDesc = 0;
    public:
        void Parallel_Thread::SetUp() override
        {
            AllocatorsFixture::SetUp();

#if defined(AZ_PLATFORM_WII)
            m_numThreadDesc = 3;
            for (int i = 0; i < m_numThreadDesc; ++i)
            {
                m_desc[i].m_stackSize = m_threadStackSize;
                AZStd::allocator a;
                char* stackMem = (char*)a.allocate(m_desc[i].m_stackSize, 16);
#ifdef AZ_PLATFORM_WII
                stackMem += m_threadStackSize;  // we need to point at the end of memory!
#endif
                m_desc[i].m_stack = stackMem;
            }
#endif
        }

        void Parallel_Thread::TearDown() override
        {
#if defined(AZ_PLATFORM_WII)
            for (int i = 0; i < m_numThreadDesc; ++i)
            {
                char* stackMem = (char*)m_desc[i].m_stack;
#ifdef AZ_PLATFORM_WII
                stackMem -= m_threadStackSize;  // we need to point at the end of memory!
#endif
                AZStd::allocator a;
                a.deallocate(stackMem, m_desc[i].m_stackSize, 16);
            }
#endif
            AllocatorsFixture::TearDown();
        }

        void increment_data()
        {
            while (m_data < m_dataMax)
            {
                m_data++;
            }
        }

        void sleep_thread(chrono::milliseconds time)
        {
            this_thread::sleep_for(time);
        }

        void do_nothing()
        {}

        void test_thread_id_for_default_constructed_thread_is_default_constructed_id()
        {
            AZStd::thread t;
            AZ_TEST_ASSERT(t.get_id() == AZStd::thread::id());
        }

        void test_thread_id_for_running_thread_is_not_default_constructed_id()
        {
            const thread_desc* desc = m_numThreadDesc ? &m_desc[0] : 0;
            AZStd::thread t(AZStd::bind(&Parallel_Thread::do_nothing, this), desc);
            AZ_TEST_ASSERT(t.get_id() != AZStd::thread::id());
            t.join();
        }

        void test_different_threads_have_different_ids()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            const thread_desc* desc2 = m_numThreadDesc ? &m_desc[1] : 0;
            AZStd::thread t(AZStd::bind(&Parallel_Thread::do_nothing, this), desc1);
            AZStd::thread t2(AZStd::bind(&Parallel_Thread::do_nothing, this), desc2);
            AZ_TEST_ASSERT(t.get_id() != t2.get_id());
            t.join();
            t2.join();
        }

        void test_thread_ids_have_a_total_order()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            const thread_desc* desc2 = m_numThreadDesc ? &m_desc[1] : 0;
            const thread_desc* desc3 = m_numThreadDesc ? &m_desc[2] : 0;

            AZStd::thread t(AZStd::bind(&Parallel_Thread::do_nothing, this), desc1);
            AZStd::thread t2(AZStd::bind(&Parallel_Thread::do_nothing, this), desc2);
            AZStd::thread t3(AZStd::bind(&Parallel_Thread::do_nothing, this), desc3);
            AZ_TEST_ASSERT(t.get_id() != t2.get_id());
            AZ_TEST_ASSERT(t.get_id() != t3.get_id());
            AZ_TEST_ASSERT(t2.get_id() != t3.get_id());

            AZ_TEST_ASSERT((t.get_id() < t2.get_id()) != (t2.get_id() < t.get_id()));
            AZ_TEST_ASSERT((t.get_id() < t3.get_id()) != (t3.get_id() < t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t3.get_id()) != (t3.get_id() < t2.get_id()));

            AZ_TEST_ASSERT((t.get_id() > t2.get_id()) != (t2.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t.get_id() > t3.get_id()) != (t3.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() > t3.get_id()) != (t3.get_id() > t2.get_id()));

            AZ_TEST_ASSERT((t.get_id() < t2.get_id()) == (t2.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t.get_id()) == (t.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t.get_id() < t3.get_id()) == (t3.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t.get_id()) == (t.get_id() > t3.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t3.get_id()) == (t3.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t2.get_id()) == (t2.get_id() > t3.get_id()));

            AZ_TEST_ASSERT((t.get_id() < t2.get_id()) == (t2.get_id() >= t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t.get_id()) == (t.get_id() >= t2.get_id()));
            AZ_TEST_ASSERT((t.get_id() < t3.get_id()) == (t3.get_id() >= t.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t.get_id()) == (t.get_id() >= t3.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t3.get_id()) == (t3.get_id() >= t2.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t2.get_id()) == (t2.get_id() >= t3.get_id()));

            AZ_TEST_ASSERT((t.get_id() <= t2.get_id()) == (t2.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() <= t.get_id()) == (t.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t.get_id() <= t3.get_id()) == (t3.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t3.get_id() <= t.get_id()) == (t.get_id() > t3.get_id()));
            AZ_TEST_ASSERT((t2.get_id() <= t3.get_id()) == (t3.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t3.get_id() <= t2.get_id()) == (t2.get_id() > t3.get_id()));

            if ((t.get_id() < t2.get_id()) && (t2.get_id() < t3.get_id()))
            {
                AZ_TEST_ASSERT(t.get_id() < t3.get_id());
            }
            else if ((t.get_id() < t3.get_id()) && (t3.get_id() < t2.get_id()))
            {
                AZ_TEST_ASSERT(t.get_id() < t2.get_id());
            }
            else if ((t2.get_id() < t3.get_id()) && (t3.get_id() < t.get_id()))
            {
                AZ_TEST_ASSERT(t2.get_id() < t.get_id());
            }
            else if ((t2.get_id() < t.get_id()) && (t.get_id() < t3.get_id()))
            {
                AZ_TEST_ASSERT(t2.get_id() < t3.get_id());
            }
            else if ((t3.get_id() < t.get_id()) && (t.get_id() < t2.get_id()))
            {
                AZ_TEST_ASSERT(t3.get_id() < t2.get_id());
            }
            else if ((t3.get_id() < t2.get_id()) && (t2.get_id() < t.get_id()))
            {
                AZ_TEST_ASSERT(t3.get_id() < t.get_id());
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }

            AZStd::thread::id default_id;

            AZ_TEST_ASSERT(default_id < t.get_id());
            AZ_TEST_ASSERT(default_id < t2.get_id());
            AZ_TEST_ASSERT(default_id < t3.get_id());

            AZ_TEST_ASSERT(default_id <= t.get_id());
            AZ_TEST_ASSERT(default_id <= t2.get_id());
            AZ_TEST_ASSERT(default_id <= t3.get_id());

            AZ_TEST_ASSERT(!(default_id > t.get_id()));
            AZ_TEST_ASSERT(!(default_id > t2.get_id()));
            AZ_TEST_ASSERT(!(default_id > t3.get_id()));

            AZ_TEST_ASSERT(!(default_id >= t.get_id()));
            AZ_TEST_ASSERT(!(default_id >= t2.get_id()));
            AZ_TEST_ASSERT(!(default_id >= t3.get_id()));

            t.join();
            t2.join();
            t3.join();
        }

        void get_thread_id(AZStd::thread::id* id)
        {
            *id = this_thread::get_id();
        }

        void test_thread_id_of_running_thread_returned_by_this_thread_get_id()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;

            AZStd::thread::id id;
            AZStd::thread t(AZStd::bind(&Parallel_Thread::get_thread_id, this, &id), desc1);
            AZStd::thread::id t_id = t.get_id();
            t.join();
            AZ_TEST_ASSERT(id == t_id);
        }


        class MfTest
        {
        public:
            mutable unsigned int m_hash;

            MfTest()
                : m_hash(0) {}

            int f0() { f1(17); return 0; }
            int g0() const { g1(17); return 0; }

            int f1(int a1) { m_hash = (m_hash * 17041 + a1) % 32768; return 0; }
            int g1(int a1) const { m_hash = (m_hash * 17041 + a1 * 2) % 32768; return 0; }

            int f2(int a1, int a2) { f1(a1); f1(a2); return 0; }
            int g2(int a1, int a2) const { g1(a1); g1(a2); return 0; }

            int f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); return 0; }
            int g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); return 0; }

            int f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); return 0; }
            int g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); return 0; }

            int f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); return 0; }
            int g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); return 0; }

            int f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); return 0; }
            int g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); return 0; }

            int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); return 0; }
            int g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); return 0; }

            int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); return 0; }
            int g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); return 0; }
        };

        void do_nothing_id(AZStd::thread::id* my_id)
        {
            *my_id = this_thread::get_id();
        }

        void test_move_on_construction()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            AZStd::thread::id the_id;
            AZStd::thread x;
            x = AZStd::thread(AZStd::bind(&Parallel_Thread::do_nothing_id, this, &the_id), desc1);
            AZStd::thread::id x_id = x.get_id();
            x.join();
            AZ_TEST_ASSERT(the_id == x_id);
        }

        AZStd::thread make_thread(AZStd::thread::id* the_id)
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            return AZStd::thread(AZStd::bind(&Parallel_Thread::do_nothing_id, this, the_id), desc1);
        }

        void test_move_from_function_return()
        {
            AZStd::thread::id the_id;
            AZStd::thread x;
            x = make_thread(&the_id);
            AZStd::thread::id x_id = x.get_id();
            x.join();
            AZ_TEST_ASSERT(the_id == x_id);
        }

        /*thread make_thread_return_lvalue(boost::thread::id* the_id)
        {
            thread t(&Parallel_Thread::do_nothing_id,this,the_id);
            return AZStd::move(t);
        }

        void test_move_from_function_return_lvalue()
        {
            thread::id the_id;
            thread x=make_thread_return_lvalue(&the_id);
            thread::id x_id=x.get_id();
            x.join();
            AZ_TEST_ASSERT(the_id==x_id);
        }

        void test_move_assign()
        {
            thread::id the_id;
            thread x(do_nothing_id,&the_id);
            thread y;
            y=AZStd::move(x);
            thread::id y_id=y.get_id();
            y.join();
            AZ_TEST_ASSERT(the_id==y_id);
        }*/

        void simple_thread()
        {
            m_data = 999;
        }

        void comparison_thread(AZStd::thread::id parent)
        {
            AZStd::thread::id const my_id = this_thread::get_id();

            AZ_TEST_ASSERT(my_id != parent);
            AZStd::thread::id const my_id2 = this_thread::get_id();
            AZ_TEST_ASSERT(my_id == my_id2);

            AZStd::thread::id const no_thread_id = AZStd::thread::id();
            AZ_TEST_ASSERT(my_id != no_thread_id);
        }

        void do_test_creation()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            m_data = 0;
            AZStd::thread t(AZStd::bind(&Parallel_Thread::simple_thread, this), desc1);
            t.join();
            AZ_TEST_ASSERT(m_data == 999);
        }

        void test_creation()
        {
            //timed_test(&do_test_creation, 1);
            do_test_creation();
        }

        void do_test_id_comparison()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            AZStd::thread::id self = this_thread::get_id();
            AZStd::thread thrd(AZStd::bind(&Parallel_Thread::comparison_thread, this, self), desc1);
            thrd.join();
        }

        void test_id_comparison()
        {
            //timed_test(&do_test_id_comparison, 1);
            do_test_id_comparison();
        }

        struct non_copyable_functor
        {
            unsigned value;

            non_copyable_functor()
                : value(0)
            {}

            void operator()()
            {
                value = 999;
            }
        private:
            non_copyable_functor(const non_copyable_functor&);
            non_copyable_functor& operator=(const non_copyable_functor&);
        };

        void do_test_creation_through_reference_wrapper()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            non_copyable_functor f;

            AZStd::thread thrd(AZStd::ref(f), desc1);
            thrd.join();
            AZ_TEST_ASSERT(f.value == 999);
        }

        void test_creation_through_reference_wrapper()
        {
            do_test_creation_through_reference_wrapper();
        }

        void test_swap()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;
            const thread_desc* desc2 = m_numThreadDesc ? &m_desc[1] : 0;
            AZStd::thread t(AZStd::bind(&Parallel_Thread::simple_thread, this), desc1);
            AZStd::thread t2(AZStd::bind(&Parallel_Thread::simple_thread, this), desc2);
            AZStd::thread::id id1 = t.get_id();
            AZStd::thread::id id2 = t2.get_id();

            t.swap(t2);
            AZ_TEST_ASSERT(t.get_id() == id2);
            AZ_TEST_ASSERT(t2.get_id() == id1);

            swap(t, t2);
            AZ_TEST_ASSERT(t.get_id() == id1);
            AZ_TEST_ASSERT(t2.get_id() == id2);

            t.detach();
            t2.detach();
        }

        void run()
        {
            const thread_desc* desc1 = m_numThreadDesc ? &m_desc[0] : 0;

            // We need to have at least one processor
            AZ_TEST_ASSERT(AZStd::thread::hardware_concurrency() >= 1);

            // Create thread to increment data till we need to
            m_data = 0;
            m_dataMax = 10;
            AZStd::thread tr(AZStd::bind(&Parallel_Thread::increment_data, this), desc1);
            tr.join();
            AZ_TEST_ASSERT(m_data == m_dataMax);

            m_data = 0;
            AZStd::thread trDel(make_delegate(this, &Parallel_Thread::increment_data), desc1);
            trDel.join();
            AZ_TEST_ASSERT(m_data == m_dataMax);

            chrono::system_clock::time_point startTime = chrono::system_clock::now();
            {
                AZStd::thread tr1(AZStd::bind(&Parallel_Thread::sleep_thread, this, chrono::milliseconds(100)), desc1);
                tr1.join();
            }
            chrono::microseconds sleepTime = chrono::system_clock::now() - startTime;
            //printf("\nSleeptime: %d Ms\n",(unsigned int)  ());
            // On Windows we use Sleep. Sleep is dependent on MM timers.
            // 99000 can be used only if we support 1 ms resolution timeGetDevCaps() and we set it timeBeginPeriod(1) timeEndPeriod(1)
            // We will need to drag mmsystem.h and we don't really need to test the OS jus our math.
            AZ_TEST_ASSERT(sleepTime.count() >= /*99000*/ 50000);

            //////////////////////////////////////////////////////////////////////////
            test_creation();
            test_id_comparison();
            test_creation_through_reference_wrapper();
            test_swap();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Thread id
            test_thread_id_for_default_constructed_thread_is_default_constructed_id();
            test_thread_id_for_running_thread_is_not_default_constructed_id();
            test_different_threads_have_different_ids();
            test_thread_ids_have_a_total_order();
            test_thread_id_of_running_thread_returned_by_this_thread_get_id();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Member function tests
            // 0
            {
                MfTest x;
                AZStd::function<void ()> func = AZStd::bind(&MfTest::f0, &x);
                AZStd::thread(func, desc1).join();
                func = AZStd::bind(&MfTest::f0, AZStd::ref(x));
                AZStd::thread(func, desc1).join();
                func = AZStd::bind(&MfTest::g0, &x);
                AZStd::thread(func, desc1).join();
                func = AZStd::bind(&MfTest::g0, x);
                AZStd::thread(func, desc1).join();
                func = AZStd::bind(&MfTest::g0, AZStd::ref(x));
                AZStd::thread(func, desc1).join();

                //// 1
                //thread( AZStd::bind(&MfTest::f1, &x, 1) , desc1).join();
                //thread( AZStd::bind(&MfTest::f1, AZStd::ref(x), 1) , desc1).join();
                //thread( AZStd::bind(&MfTest::g1, &x, 1) , desc1).join();
                //thread( AZStd::bind(&MfTest::g1, x, 1) , desc1).join();
                //thread( AZStd::bind(&MfTest::g1, AZStd::ref(x), 1) , desc1).join();

                //// 2
                //thread( AZStd::bind(&MfTest::f2, &x, 1, 2) , desc1).join();
                //thread( AZStd::bind(&MfTest::f2, AZStd::ref(x), 1, 2) , desc1).join();
                //thread( AZStd::bind(&MfTest::g2, &x, 1, 2) , desc1).join();
                //thread( AZStd::bind(&MfTest::g2, x, 1, 2) , desc1).join();
                //thread( AZStd::bind(&MfTest::g2, AZStd::ref(x), 1, 2) , desc1).join();

                //// 3
                //thread( AZStd::bind(&MfTest::f3, &x, 1, 2, 3) , desc1).join();
                //thread( AZStd::bind(&MfTest::f3, AZStd::ref(x), 1, 2, 3) , desc1).join();
                //thread( AZStd::bind(&MfTest::g3, &x, 1, 2, 3) , desc1).join();
                //thread( AZStd::bind(&MfTest::g3, x, 1, 2, 3) , desc1).join();
                //thread( AZStd::bind(&MfTest::g3, AZStd::ref(x), 1, 2, 3) , desc1).join();

                //// 4
                //thread( AZStd::bind(&MfTest::f4, &x, 1, 2, 3, 4) , desc1).join();
                //thread( AZStd::bind(&MfTest::f4, AZStd::ref(x), 1, 2, 3, 4) , desc1).join();
                //thread( AZStd::bind(&MfTest::g4, &x, 1, 2, 3, 4) , desc1).join();
                //thread( AZStd::bind(&MfTest::g4, x, 1, 2, 3, 4) , desc1).join();
                //thread( AZStd::bind(&MfTest::g4, AZStd::ref(x), 1, 2, 3, 4) , desc1).join();

                //// 5
                //thread( AZStd::bind(&MfTest::f5, &x, 1, 2, 3, 4, 5) , desc1).join();
                //thread( AZStd::bind(&MfTest::f5, AZStd::ref(x), 1, 2, 3, 4, 5) , desc1).join();
                //thread( AZStd::bind(&MfTest::g5, &x, 1, 2, 3, 4, 5) , desc1).join();
                //thread( AZStd::bind(&MfTest::g5, x, 1, 2, 3, 4, 5) , desc1).join();
                //thread( AZStd::bind(&MfTest::g5, AZStd::ref(x), 1, 2, 3, 4, 5) , desc1).join();

                //// 6
                //thread( AZStd::bind(&MfTest::f6, &x, 1, 2, 3, 4, 5, 6) , desc1).join();
                //thread( AZStd::bind(&MfTest::f6, AZStd::ref(x), 1, 2, 3, 4, 5, 6) , desc1).join();
                //thread( AZStd::bind(&MfTest::g6, &x, 1, 2, 3, 4, 5, 6) , desc1).join();
                //thread( AZStd::bind(&MfTest::g6, x, 1, 2, 3, 4, 5, 6) , desc1).join();
                //thread( AZStd::bind(&MfTest::g6, AZStd::ref(x), 1, 2, 3, 4, 5, 6) , desc1).join();

                //// 7
                //thread( AZStd::bind(&MfTest::f7, &x, 1, 2, 3, 4, 5, 6, 7), desc1).join();
                //thread( AZStd::bind(&MfTest::f7, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7), desc1).join();
                //thread( AZStd::bind(&MfTest::g7, &x, 1, 2, 3, 4, 5, 6, 7), desc1).join();
                //thread( AZStd::bind(&MfTest::g7, x, 1, 2, 3, 4, 5, 6, 7), desc1).join();
                //thread( AZStd::bind(&MfTest::g7, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7), desc1).join();

                //// 8
                //thread( AZStd::bind(&MfTest::f8, &x, 1, 2, 3, 4, 5, 6, 7, 8) , desc1).join();
                //thread( AZStd::bind(&MfTest::f8, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7, 8) , desc1).join();
                //thread( AZStd::bind(&MfTest::g8, &x, 1, 2, 3, 4, 5, 6, 7, 8) , desc1).join();
                //thread( AZStd::bind(&MfTest::g8, x, 1, 2, 3, 4, 5, 6, 7, 8) , desc1).join();
                //thread( AZStd::bind(&MfTest::g8, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7, 8) , desc1).join();

                AZ_TEST_ASSERT(x.m_hash == 1366);
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Move
            test_move_on_construction();

            test_move_from_function_return();
            //////////////////////////////////////////////////////////////////////////
        }
    };

    TEST_F(Parallel_Thread, Test)
    {
        run();
    }

    class Parallel_Combinable
        : public AllocatorsFixture
    {
    public:
        void run()
        {
            //initialize with default value
            {
                combinable<TestStruct> c;
                TestStruct& s = c.local();
                AZ_TEST_ASSERT(s.m_x == 42);
            }

            //detect first initialization
            {
                combinable<int> c;
                bool exists;
                int& v1 = c.local(exists);
                AZ_TEST_ASSERT(!exists);
                v1 = 42;

                int& v2 = c.local(exists);
                AZ_TEST_ASSERT(exists);
                AZ_TEST_ASSERT(v2 == 42);

                int& v3 = c.local();
                AZ_TEST_ASSERT(v3 == 42);
            }

            //custom initializer
            {
                combinable<int> c(&Initializer);
                AZ_TEST_ASSERT(c.local() == 43);
            }

            //clear
            {
                combinable<int> c(&Initializer);
                bool exists;
                int& v1 = c.local(exists);
                AZ_TEST_ASSERT(v1 == 43);
                AZ_TEST_ASSERT(!exists);
                v1 = 44;

                c.clear();
                int& v2 = c.local(exists);
                AZ_TEST_ASSERT(v2 == 43);
                AZ_TEST_ASSERT(!exists);
            }

            //copy constructor and assignment
            {
                combinable<int> c1, c2;
                int& v = c1.local();
                v = 45;

                combinable<int> c3(c1);
                AZ_TEST_ASSERT(c3.local() == 45);

                c2 = c1;
                AZ_TEST_ASSERT(c2.local() == 45);
            }

            //combine
            {
                combinable<int> c(&Initializer);

                //default value when no other values
                AZ_TEST_ASSERT(c.combine(plus<int>()) == 43);

                c.local() = 50;
                AZ_TEST_ASSERT(c.combine(plus<int>()) == 50);
            }

            //combine_each
            {
                combinable<int> c(&Initializer);

                m_numCombinerCalls = 0;
                c.combine_each(bind(&Parallel_Combinable::MyCombiner, this, _1));
                AZ_TEST_ASSERT(m_numCombinerCalls == 0);

                m_numCombinerCalls = 0;
                m_combinerTotal = 0;
                c.local() = 50;
                c.combine_each(bind(&Parallel_Combinable::MyCombiner, this, _1));
                AZ_TEST_ASSERT(m_numCombinerCalls == 1);
                AZ_TEST_ASSERT(m_combinerTotal == 50);
            }

            //multithread test
            {
                AZStd::thread_desc desc;
                desc.m_name = "Test Thread 1";
                AZStd::thread t1(bind(&Parallel_Combinable::MyThreadFunc, this, 0, 10), &desc);
                desc.m_name = "Test Thread 2";
                AZStd::thread t2(bind(&Parallel_Combinable::MyThreadFunc, this, 10, 20), &desc);
                desc.m_name = "Test Thread 3";
                AZStd::thread t3(bind(&Parallel_Combinable::MyThreadFunc, this, 20, 500), &desc);
                desc.m_name = "Test Thread 4";
                AZStd::thread t4(bind(&Parallel_Combinable::MyThreadFunc, this, 500, 510), &desc);
                desc.m_name = "Test Thread 5";
                AZStd::thread t5(bind(&Parallel_Combinable::MyThreadFunc, this, 510, 2001), &desc);

                t1.join();
                t2.join();
                t3.join();
                t4.join();
                t5.join();

                m_numCombinerCalls = 0;
                m_combinerTotal = 0;
                m_threadCombinable.combine_each(bind(&Parallel_Combinable::MyCombiner, this, _1));
                AZ_TEST_ASSERT(m_numCombinerCalls == 5);
                AZ_TEST_ASSERT(m_combinerTotal == 2001000);

                AZ_TEST_ASSERT(m_threadCombinable.combine(plus<int>()) == 2001000);

                m_threadCombinable.clear();
            }
        }

        static int Initializer()
        {
            return 43;
        }

        void MyThreadFunc(int start, int end)
        {
            int& v = m_threadCombinable.local();
            v = 0;
            for (int i = start; i < end; ++i)
            {
                v += i;
            }
        }

        void MyCombiner(int v)
        {
            ++m_numCombinerCalls;
            m_combinerTotal += v;
        }

        int m_numCombinerCalls;
        int m_combinerTotal;

        combinable<int> m_threadCombinable;

        struct TestStruct
        {
            TestStruct()
                : m_x(42) { }
            int m_x;
        };
    };

    TEST_F(Parallel_Combinable, Test)
    {
        run();
    }

    class Parallel_SharedMutex
        : public AllocatorsFixture
    {
    public:
        static const int s_numOfReaders = 4;
        shared_mutex m_access;
        unsigned int m_readSum[s_numOfReaders];
        unsigned int m_currentValue;

        void Reader(int index)
        {
            unsigned int lastCurrentValue = 0;
            while (true)
            {
                {
                    // get shared access
                    shared_lock<shared_mutex> lock(m_access);
                    // now we have shared access
                    if (lastCurrentValue != m_currentValue)
                    {
                        lastCurrentValue = m_currentValue;

                        m_readSum[index] += lastCurrentValue;

                        if (m_currentValue == 100)
                        {
                            break;
                        }
                    }
                }

                this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            }
        }

        void Writer()
        {
            while (m_currentValue < 100)
            {
                {
                    // get upgradeable access
                    upgrade_lock<shared_mutex> lock(m_access);

                    unsigned int currentValue = m_currentValue;

                    // get exclusive access
                    upgrade_to_unique_lock<shared_mutex> uniqueLock(lock);

                    // now we have exclusive access
                    m_currentValue = currentValue + 1;
                }

                this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }
        }

        void run()
        {
            // basic operations
            {
                shared_mutex rwlock;

                // try exclusive lock
                AZ_TEST_ASSERT(rwlock.try_lock() == true);
                rwlock.unlock();

                rwlock.lock(); // get the exclusive lock
                // while exclusive lock is taken nobody else can get a lock
                AZ_TEST_ASSERT(rwlock.try_lock() == false);
                AZ_TEST_ASSERT(rwlock.try_lock_shared() == false);
                AZ_TEST_ASSERT(rwlock.try_lock_upgrade() == false);
                rwlock.unlock();

                // try shared lock
                AZ_TEST_ASSERT(rwlock.try_lock_shared() == true);
                rwlock.unlock();

                rwlock.lock_shared(); // get the shared lock
                AZ_TEST_ASSERT(rwlock.try_lock_shared() == true); // make sure we can have multiple shared locks
                AZ_TEST_ASSERT(rwlock.try_lock_upgrade() == true); // we can get upgrade too
                rwlock.unlock_shared();
                rwlock.unlock_shared();
                rwlock.unlock_upgrade();

                // try upgrade lock
                AZ_TEST_ASSERT(rwlock.try_lock_upgrade() == true);
                AZ_TEST_ASSERT(rwlock.try_lock_upgrade() == false); // we can have only one upgrade lock at a time
                AZ_TEST_ASSERT(rwlock.try_lock_shared() == true); // shared is fine
                rwlock.unlock_shared();
                rwlock.unlock_upgrade();

                // lock upgrade lock
                rwlock.lock_upgrade();
                AZ_TEST_ASSERT(rwlock.try_lock_upgrade() == false);
                AZ_TEST_ASSERT(rwlock.try_unlock_upgrade_and_lock() == true); // upgrade to exclusive
            }

            // spin threads and run test validity of operations
            {
                m_currentValue = 0;
                memset(m_readSum, 0, sizeof(unsigned int) * AZ_ARRAY_SIZE(m_readSum));

                AZStd::thread_desc desc;
                desc.m_name = "Test Reader 1";
                AZStd::thread t1(bind(&Parallel_SharedMutex::Reader, this, 0), &desc);
                desc.m_name = "Test Reader 2";
                AZStd::thread t2(bind(&Parallel_SharedMutex::Reader, this, 1), &desc);
                desc.m_name = "Test Reader 3";
                AZStd::thread t3(bind(&Parallel_SharedMutex::Reader, this, 2), &desc);
                desc.m_name = "Test Reader 4";
                AZStd::thread t4(bind(&Parallel_SharedMutex::Reader, this, 3), &desc);
                desc.m_name = "Test Writer 1";
                AZStd::thread t5(bind(&Parallel_SharedMutex::Writer, this), &desc);
                desc.m_name = "Test Writer 2";
                AZStd::thread t6(bind(&Parallel_SharedMutex::Writer, this), &desc);

                t1.join();
                t2.join();
                t3.join();
                t4.join();
                t5.join();
                t6.join();

                AZ_TEST_ASSERT(m_currentValue == 100);
                // Check for the range of the sums as we don't guarantee adding all numbers.
                AZ_TEST_ASSERT(m_readSum[0] > 1000 && m_readSum[0] <= 5050);
                AZ_TEST_ASSERT(m_readSum[1] > 1000 && m_readSum[1] <= 5050);
                AZ_TEST_ASSERT(m_readSum[2] > 1000 && m_readSum[2] <= 5050);
                AZ_TEST_ASSERT(m_readSum[3] > 1000 && m_readSum[3] <= 5050);
            }
        }
    };

    TEST_F(Parallel_SharedMutex, Test)
    {
        run();
    }

    class ConditionVariable
        : public AllocatorsFixture
    {};
    
    TEST_F(ConditionVariable, NotifyOneSingleWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i = 0;
        AZStd::atomic_bool done = false;
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&]{ return i == 1; });
            EXPECT_EQ(1, i);
            done = true;
        };

        auto signal = [&]()
        {
            cv.notify_one();
            EXPECT_EQ(0, i);
            EXPECT_FALSE(done);

            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            i = 1;
            while (!done)
            {
                lock.unlock();
                cv.notify_one();
                lock.lock();
            }            
        };

        EXPECT_EQ(0, i);
        EXPECT_FALSE(done);

        AZStd::thread waitThread1(wait);
        AZStd::thread signalThread(signal);
        waitThread1.join();
        signalThread.join();

        EXPECT_EQ(1, i);
        EXPECT_TRUE(done);
    }

    TEST_F(ConditionVariable, NotifyOneMultipleWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i = 0;
        AZStd::atomic_bool done1 = false;
        AZStd::atomic_bool done2 = false;
        auto wait1 = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&] { return i == 1; });
            EXPECT_EQ(1, i);
            done1 = true;
        };

        auto wait2 = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&] { return i == 1; });
            EXPECT_EQ(1, i);
            done2 = true;
        };

        auto signal = [&]()
        {
            cv.notify_one();
            EXPECT_EQ(0, i);
            EXPECT_FALSE(done1);
            EXPECT_FALSE(done2);

            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            i = 1;
            while (!(done1 && done2))
            {
                lock.unlock();
                cv.notify_one();
                lock.lock();
            }
        };

        EXPECT_EQ(0, i);
        EXPECT_FALSE(done1);
        EXPECT_FALSE(done2);

        AZStd::thread waitThread1(wait1);
        AZStd::thread waitThread2(wait2);
        AZStd::thread signalThread(signal);
        waitThread1.join();
        waitThread2.join();
        signalThread.join();

        EXPECT_EQ(1, i);
        EXPECT_TRUE(done1);
        EXPECT_TRUE(done2);
    }

    TEST_F(ConditionVariable, NotifyAll)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i = 0;
        
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&] { return i == 1; });
        };

        auto signal = [&]()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            {
                AZStd::lock_guard<AZStd::mutex> lock(cv_mutex);
                i = 0;
            }
            cv.notify_all();
            EXPECT_EQ(0, i);

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));

            {
                AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
                i = 1;
            }
            cv.notify_all();
        };

        EXPECT_EQ(0, i);

        AZStd::thread waitThreads[8];
        for (int threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(waitThreads); ++threadIdx)
        {
            waitThreads[threadIdx] = AZStd::thread(wait);
        }
        AZStd::thread signalThread(signal);
        
        for (auto& thread : waitThreads)
        {
            thread.join();
        }
        signalThread.join();

        EXPECT_EQ(1, i);
    }
}
