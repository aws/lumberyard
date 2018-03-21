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
#include <AzCore/std/parallel/conditional_variable.h>

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
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
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
                    lock_guard<shared_mutex> lock(m_access);
                    // now we have exclusive access
                    unsigned int currentValue = m_currentValue;                    
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
                EXPECT_TRUE(rwlock.try_lock());
                rwlock.unlock();

                rwlock.lock(); // get the exclusive lock
                // while exclusive lock is taken nobody else can get a lock
                EXPECT_FALSE(rwlock.try_lock());
                EXPECT_FALSE(rwlock.try_lock_shared());
                rwlock.unlock();

                // try shared lock
                EXPECT_TRUE(rwlock.try_lock_shared());
                rwlock.unlock_shared();

                rwlock.lock_shared(); // get the shared lock
                EXPECT_TRUE(rwlock.try_lock_shared()); // make sure we can have multiple shared locks
                rwlock.unlock_shared();
                rwlock.unlock_shared();
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

                EXPECT_EQ(100, m_currentValue);
                // Check for the range of the sums as we don't guarantee adding all numbers.
                EXPECT_TRUE(m_readSum[0] > 1000 && m_readSum[0] <= 5050);
                EXPECT_TRUE(m_readSum[1] > 1000 && m_readSum[1] <= 5050);
                EXPECT_TRUE(m_readSum[2] > 1000 && m_readSum[2] <= 5050);
                EXPECT_TRUE(m_readSum[3] > 1000 && m_readSum[3] <= 5050);
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
        AZStd::atomic_int i(0);
        AZStd::atomic_bool done(false);
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
        AZStd::atomic_int i(0);
        AZStd::atomic_bool done1(false);
        AZStd::atomic_bool done2(false);
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
        AZStd::atomic_int i(0);
        
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
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(waitThreads); ++threadIdx)
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
