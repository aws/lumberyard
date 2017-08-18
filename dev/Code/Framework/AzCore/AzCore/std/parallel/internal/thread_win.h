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
#ifndef AZSTD_THREAD_WINDOWS_H
#define AZSTD_THREAD_WINDOWS_H

#ifndef YieldProcessor
#    define YieldProcessor _mm_pause
#endif

namespace AZStd
{
    namespace Internal
    {
        /**
         * Create and run thread
         */
        HANDLE create_thread(const thread_desc* desc, thread_info* ti, unsigned int* id);
    }

    //////////////////////////////////////////////////////////////////////////
    // thread
    template <class F>
    inline thread::thread(F f, const thread_desc* desc)
    {
        Internal::thread_info* ti = Internal::create_thread_info(f);
        m_thread.m_handle = Internal::create_thread(desc, ti, &m_thread.m_id);
    }
    //template <class F,class A1>
    //thread::thread(F f,A1 a1,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}
    //template <class F,class A1,class A2>
    //thread::thread(F f,A1 a1,A2 a2,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3,class A4>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3,a4));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3,class A4,class A5>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3,a4,a5));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3,class A4,class A5,class A6>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3,a4,a5,a6));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3,a4,a5,a6,a7));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3,a4,a5,a6,a7,a8));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template <class F,class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8,class A9>
    //thread::thread(F f,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9,const thread_desc* desc)
    //{
    //  Internal::thread_info* ti = Internal::create_thread_info(AZStd::bind(AZStd::type<void>(),f,a1,a2,a3,a4,a5,a6,a7,a8,a9));
    //  m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    //}

    //template<class F>
    //thread::thread(Internal::thread_move_t<F> f)
    //{
    //  m_data.m_info = Internal::create_thread_info(f);
    //  m_data.m_hThread = Internal::create_thread(0,m_data.m_info,&m_data.m_id);
    //}

    inline bool thread::joinable() const
    {
        if (m_thread.m_id == native_thread_invalid_id)
        {
            return false;
        }
        return m_thread.m_id != this_thread::get_id().m_id;
    }
    inline thread::id thread::get_id() const
    {
        return id(m_thread.m_id);
    }
    thread::native_handle_type
    inline thread::native_handle()
    {
        return m_thread.m_handle;
    }
    //////////////////////////////////////////////////////////////////////////

    namespace this_thread
    {
        AZ_FORCE_INLINE thread::id get_id()
        {
#if defined(AZ_PLATFORM_WINDOWS_X64)
            DWORD threadId = __readgsdword(0x48);
#elif defined(AZ_PLATFORM_WINDOWS)
            DWORD threadId = __readfsdword(0x24);
#else
            DWORD threadId = GetCurrentThreadId();
#endif
            return thread::id(threadId);
        }
        AZ_FORCE_INLINE void yield()        { ::Sleep(0); }
        AZ_FORCE_INLINE void pause(int numLoops)
        {
            for (int i = 0; i < numLoops; ++i)
            {
                YieldProcessor(); //pause instruction on x86, allows hyperthread to run
            }
        }
        //template <class Clock, class Duration>
        //AZ_FORCE_INLINE void sleep_until(const chrono::time_point<Clock, Duration>& abs_time)
        //{
        //  chrono::milliseconds now = chrono::system_clock::now().time_since_epoch();
        //  AZ_Assert(now<abs_time,"Absolute time must be in the future!");
        //  chrono::milliseconds toSleep = abs_time - now;
        //  ::Sleep((DWORD)toSleep.count());
        //}
        template <class Rep, class Period>
        AZ_FORCE_INLINE void sleep_for(const chrono::duration<Rep, Period>& rel_time)
        {
            chrono::milliseconds toSleep = rel_time;
            ::Sleep((DWORD)toSleep.count());
        }
    }
}

#endif // AZSTD_THREAD_WINDOWS_H
#pragma once