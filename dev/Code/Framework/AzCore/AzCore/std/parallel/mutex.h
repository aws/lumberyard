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

namespace AZStd
{
    /**
    * Mutex facilitates protection against data races and allows
    * thread-safe synchronization of data between threads.
    * \ref C++0x (30.4.1)
    */
    class mutex
    {
    public:
        typedef native_mutex_handle_type native_handle_type;

        mutex();
        ~mutex();

        void lock();
        bool try_lock();
        void unlock();

        native_handle_type native_handle();

        // Extensions
        mutex(const char* name);
    private:
        mutex(const mutex&) {}
        mutex& operator=(const mutex&) { return *this; }

        native_mutex_data_type  m_mutex;
    };

    /**
    * Recursive mutex provides a recursive mutex with exclusive ownership semantics.
    * \ref C++0x (30.4.1.2)
    */
    class recursive_mutex
    {
    public:
        typedef native_recursive_mutex_handle_type native_handle_type;

        recursive_mutex();
        ~recursive_mutex();

        void lock();
        bool try_lock();
        void unlock();

        native_handle_type native_handle();

        // Extensions
        recursive_mutex(const char* name);
    private:
        recursive_mutex(const recursive_mutex&)     {}
        recursive_mutex& operator=(const recursive_mutex&) { return *this; }

        native_recursive_mutex_data_type    m_mutex;
    };

#if 0 // can't be implemented on all supported platforms as of now
      /**
       * A TimedMutex type shall meet the requirements for a Mutex type. In addition
       * it will meet requirements for duration.
       * \ref C++0x (30.4.2)
       */
    class timed_mutex
    {
    public:
        typedef native_mutex_handle_type native_handle_type;

        timed_mutex();
        ~timed_mutex();

        void lock();
        bool try_lock();
        //template <class Rep, class Period>
        //bool try_lock_for(const chrono::duration<Rep, Period>& rel_time);
        //template <class Clock, class Duration>
        //bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time);
        void unlock();
        native_handle_type native_handle();

        // Extensions
        timed_mutex(const char* name);
    private:
        timed_mutex(const timed_mutex&)  {}
        timed_mutex& operator=(const timed_mutex&) {}

        native_mutex_data_type  m_mutex;
    };

    /**
    * Recursive mutex with exclusive ownership, have the features of a timed_mutex.
    * \ref C++0x (30.4.2)
    */
    class recursive_timed_mutex
    {
    public:
        typedef native_recursive_mutex_handle_type native_handle_type;

        recursive_timed_mutex();
        ~recursive_timed_mutex();

        void lock();
        bool try_lock();
        //template <class Rep, class Period>
        //bool try_lock_for(const chrono::duration<Rep, Period>& rel_time);
        //template <class Clock, class Duration>
        //bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time);
        void unlock();

        native_handle_type native_handle();

        // Extensions
        recursive_timed_mutex(const char* name);
    private:
        recursive_timed_mutex(const recursive_timed_mutex&)     {}
        recursive_timed_mutex& operator=(const recursive_timed_mutex&) {}

        native_recursive_mutex_data_type    m_mutex;
    };
#endif
}

#if defined(AZ_PLATFORM_WINDOWS)
    #include <AzCore/std/parallel/internal/mutex_win.h>
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/mutex_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/mutex_h_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    #include <AzCore/std/parallel/internal/mutex_linux.h>
#else
    #error Platform not supported
#endif
