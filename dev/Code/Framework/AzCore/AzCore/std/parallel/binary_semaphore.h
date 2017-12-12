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
#ifndef AZSTD_BINARY_SEMAPHORE_H
#define AZSTD_BINARY_SEMAPHORE_H

#include <AzCore/base.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE
#   include <AzCore/std/parallel/config.h>
#   include <AzCore/std/chrono/types.h>

#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#   include <AzCore/std/parallel/mutex.h>
#   include <AzCore/std/parallel/conditional_variable.h>

#else
#   include <AzCore/std/parallel/semaphore.h>
#endif

namespace AZStd
{
    /**
     * Binary semaphore class (aka event). In general is implemented via standard semaphore with max count = 1,
     * but on "some" platforms there are more efficient ways of doing it.
     * \todo Move code to platform files.
     */
    class binary_semaphore
    {
    public:
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_XBONE) // ACCEPTED_USE

        typedef HANDLE native_handle_type;

        binary_semaphore(bool initialState = false)
        {
            m_event = CreateEvent(nullptr, false, initialState, nullptr);
            AZ_Assert(m_event != NULL, "CreateEvent error: %d\n", GetLastError());
        }
        binary_semaphore(const char* name, bool initialState = false)
        {
            (void)name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
            m_event = CreateEvent(nullptr, false, initialState, nullptr);
            AZ_Assert(m_event != NULL, "CreateEvent error: %d\n", GetLastError());
        }

        binary_semaphore(const binary_semaphore&) = delete;
        binary_semaphore& operator=(const binary_semaphore&) = delete;

        ~binary_semaphore()
        {
            BOOL ret = CloseHandle(m_event);
            (void)ret;
            AZ_Assert(ret, "CloseHandle error: %d\n", GetLastError());
        }
        void acquire()  { WaitForSingleObject(m_event, AZ_INFINITE); }

        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
        {
            chrono::milliseconds timeToTry = rel_time;
            return (WaitForSingleObject(m_event, static_cast<DWORD>(timeToTry.count())) == AZ_WAIT_OBJECT_0);
        }

        void release()  { SetEvent(m_event); }

        native_handle_type native_handle() { return m_event; }

    private:
        HANDLE m_event;

#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)

        typedef condition_variable::native_handle_type native_handle_type;

        binary_semaphore(bool initialState = false)
            : m_isReady(initialState)
        {}
        binary_semaphore(const char* name, bool initialState = false)
            : m_mutex(name)
            , m_condVar(name)
            , m_isReady(initialState)
        {}
        void acquire()
        {
            AZStd::unique_lock<AZStd::mutex> ulock(m_mutex);
            while (!m_isReady)
            {
                m_condVar.wait(ulock);
            }
            m_isReady = false;
        }

        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
        {
            AZStd::unique_lock<AZStd::mutex> ulock(m_mutex);
            if (!m_isReady)
            {
                m_condVar.wait_for(ulock, rel_time); // technically this can return soner than rel_time
                if (!m_isReady)
                {
                    return false;
                }
            }
            m_isReady = false;
            return true;
        }

        void release()
        {
            AZStd::lock_guard<AZStd::mutex> ulock(m_mutex);
            m_isReady = true;
            m_condVar.notify_one();
        }

        native_handle_type native_handle()
        {
            return m_condVar.native_handle();
        }

    private:
        // For Linux we use pthread and semaphores don't have max count, we can simulate the semaphore but this will be ineffcient compare to
        // custom solution
        mutex   m_mutex;
        condition_variable m_condVar;
        bool    m_isReady;
#else
        // For platforms with max count semaphore

        typedef native_semaphore_handle_type native_handle_type;

        binary_semaphore(bool initialState = false)
            : m_semaphore(initialState ? 1 : 0, 1) {}
        binary_semaphore(const char* name, bool initialState = false)
            : m_semaphore(name, initialState ? 1 : 0, 1) {}

        void acquire()  { m_semaphore.acquire(); }

        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time) { return m_semaphore.try_acquire_for(rel_time); }

        void release()  { m_semaphore.release(1); }

        native_handle_type native_handle() { return m_semaphore.native_handle(); }
    private:
        AZStd::semaphore m_semaphore;
#endif //
    };
}

#endif // AZSTD_SEMAPHORE_H
#pragma once
