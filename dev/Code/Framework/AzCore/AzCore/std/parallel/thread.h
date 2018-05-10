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

#include <AzCore/std/parallel/config.h>
#include <AzCore/std/allocator.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/chrono/types.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define THREAD_H_SECTION_1 1
#define THREAD_H_SECTION_2 2
#endif

namespace AZStd
{
    namespace Internal
    {
        template<typename T>
        struct thread_move_t
        {
            T& m_t;
            explicit thread_move_t(T& t)
                : m_t(t)   {}

            T& operator*() const  { return m_t; }
            T* operator->() const { return &m_t; }
        private:
            void operator=(thread_move_t&);
        };

        //      Once we start adding move semantics
        //      template<typename T>
        //      typename Utils::enable_if<AZStd::is_convertible<T&,Internal::thread_move_t<T> >, T >::type move(T& t)
        //      {
        //          return T(Internal::thread_move_t<T>(t));
        //      }
        //      template<typename T>
        //      Internal::thread_move_t<T> move(Internal::thread_move_t<T> t)
        //      {
        //          return t;
        //      }
    }
    // Extension

    struct thread_desc
    {
        // Default thread desc settings
        thread_desc()
            : m_stack(0)
            , m_stackSize(-1)
            , m_priority(-100000)
            , m_cpuId(-1)
            , m_isJoinable(true)
            , m_name("AZStd::thread")
        {}

        void*           m_stack;        ///< Stack memory pointer (required for Wii)

        /**
        *  Thread stack size.
        *  Default is -1, which means we will use the default stack size for each platform.
        */
        int             m_stackSize;

        /**
         *  Windows: One of the following values:
         *      THREAD_PRIORITY_IDLE
         *      THREAD_PRIORITY_LOWEST
         *      THREAD_PRIORITY_BELOW_NORMAL
         *      THREAD_PRIORITY_NORMAL  (This is the default)
         *      THREAD_PRIORITY_ABOVE_NORMAL
         *      THREAD_PRIORITY_TIME_CRITICAL
         */
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION THREAD_H_SECTION_1
#include AZ_RESTRICTED_FILE(thread_h, AZ_RESTRICTED_PLATFORM)
#endif
        int             m_priority;

        /**
         *  The CPU id that this thread will be running on, see \ref AZStd::thread_desc::m_cpuId.
         *  Windows: This parameter is ignored.
         *  On other platforms, this maps directly to the core number [0-n], default is 0
         */
        int             m_cpuId;

        bool            m_isJoinable;   ///< If we can join the thread.
        const char*    m_name;          ///< Debug thread name.
    };


    // 30.3.1
    class thread
    {
    public:
        // types:
        typedef thread_id id;
        typedef native_thread_handle_type native_handle_type;

        // construct/copy/destroy:
        thread();
        /**
         * \note thread_desc is AZStd extension.
         */
        template <class F>
        explicit thread(F f, const thread_desc* desc = 0);

        ~thread();

#ifdef AZ_HAS_RVALUE_REFS
        thread(thread&& rhs)
            : m_thread(rhs.m_thread)
        {
            rhs.m_thread = AZStd::thread().m_thread; // set default value
        }
        thread& operator=(thread&& rhs)
        {
            AZ_Assert(joinable() == false, "You must call detach or join before you delete/move over the current thread!");
            m_thread = rhs.m_thread;
            rhs.m_thread = AZStd::thread().m_thread; // set default value
            return *this;
        }
#endif
        // Till we fully have RVALUES
        template <class F>
        explicit thread(Internal::thread_move_t<F> f);
        thread(Internal::thread_move_t<thread> rhs);
        inline operator Internal::thread_move_t<thread>()   {
            return Internal::thread_move_t<thread>(*this);
        }
        inline thread& operator=(Internal::thread_move_t<thread> rhs)
        {
            thread new_thread(rhs);
            swap(new_thread);
            if (new_thread.joinable())
            {
                new_thread.detach(); // we should/could terminate the thread, this is tricky on some platforms so we leave it to the user.
            }
            return *this;
        }
        inline void swap(thread& rhs) { AZStd::swap(m_thread, rhs.m_thread); }

        bool joinable() const;
        void join();
        void detach();
        id get_id() const;
        native_handle_type native_handle();
        static unsigned hardware_concurrency();

        // Extensions
        //thread(AZStd::delegate<void ()> d,const thread_desc* desc = 0);

    private:
        thread(thread&);
        thread& operator=(thread&);

        native_thread_data_type     m_thread;
    };

    // 30.3
    class thread;
    inline void swap(thread& x, thread& y)      { x.swap(y); }
    namespace this_thread {
        thread::id get_id();
        void yield();
        ///extension, spins for the specified number of loops, yielding correctly on hyper threaded processors
        void pause(int numLoops);
        //template <class Clock, class Duration>
        //void sleep_until(const chrono::time_point<Clock, Duration>& abs_time);
        template <class Rep, class Period>
        void sleep_for(const chrono::duration<Rep, Period>& rel_time);
    }

    namespace Internal
    {
        /**
        * Thread info interface.
        */
        class thread_info
        {
        public:
            virtual ~thread_info() {}
            virtual void execute() = 0;
#if defined(AZ_PLATFORM_APPLE)
            thread_info()
                : m_name(nullptr) {}
            const char* m_name;
#endif
        };

        /**
        * Wrapper for the thread execute function.
        */
        template<typename F>
        class thread_info_impl
            : public thread_info
        {
        public:
            thread_info_impl(F f)
                : m_f(f) {}
            thread_info_impl(Internal::thread_move_t<F> f)
                : m_f(f) {}
            virtual void execute() { m_f(); }
        private:
            F m_f;

            void operator=(thread_info_impl&);
            thread_info_impl(thread_info_impl&);
        };

        template<typename F>
        class thread_info_impl<AZStd::reference_wrapper<F> >
            : public thread_info
        {
        public:
            thread_info_impl(AZStd::reference_wrapper<F> f)
                :  m_f(f)  {}
            virtual void execute() { m_f();  }
        private:
            void operator=(thread_info_impl&);
            thread_info_impl(thread_info_impl&);
            F& m_f;
        };

        template<typename F>
        class thread_info_impl<const AZStd::reference_wrapper<F> >
            : public thread_info
        {
        public:
            thread_info_impl(const AZStd::reference_wrapper<F> f)
                : m_f(f)  {}
            virtual void execute() { m_f(); }
        private:
            F& m_f;
            void operator=(thread_info_impl&);
            thread_info_impl(thread_info_impl&);
        };

        template<typename F>
        static AZ_INLINE thread_info*           create_thread_info(F f)
        {
            AZStd::allocator a;
            return new (a.allocate(sizeof(thread_info_impl<F>), AZStd::alignment_of< thread_info_impl<F> >::value))thread_info_impl<F>(f);
        }

        template<typename F>
        static AZ_INLINE thread_info*           create_thread_info(thread_move_t<F> f)
        {
            AZStd::allocator a;
            return new (a.allocate(sizeof(thread_info_impl<F>), AZStd::alignment_of< thread_info_impl<F> >::value))thread_info_impl<F>(f);
        }

        static AZ_INLINE void                   destroy_thread_info(thread_info*& ti)
        {
            if (ti)
            {
                ti->~thread_info();
                AZStd::allocator a;
                a.deallocate(ti, 0, 0);
                ti = 0;
            }
        }
    }
}

#if defined(AZ_PLATFORM_WINDOWS)
    #include <AzCore/std/parallel/internal/thread_win.h>
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION THREAD_H_SECTION_2
#include AZ_RESTRICTED_FILE(thread_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    #include <AzCore/std/parallel/internal/thread_linux.h>
#else
    #error Platform not supported
#endif
