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
#ifndef AZSTD_SEMAPHORE_WINDOWS_H
#define AZSTD_SEMAPHORE_WINDOWS_H


/**
* This file is to be included from the semaphore.h only. It should NOT be included by the user.
*/

namespace AZStd
{
    inline semaphore::semaphore(unsigned int initialCount, unsigned int maximumCount)
    {
        m_semaphore = CreateSemaphore(NULL, initialCount, maximumCount, 0);
        AZ_Assert(m_semaphore != NULL, "CreateSemaphore error: %d\n", GetLastError());
    }

    inline semaphore::semaphore(const char* name, unsigned int initialCount, unsigned int maximumCount)
    {
        (void)name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
        m_semaphore = CreateSemaphore(NULL, initialCount, maximumCount, 0);
        AZ_Assert(m_semaphore != NULL, "CreateSemaphore error: %d\n", GetLastError());
    }

    inline semaphore::~semaphore()
    {
        BOOL ret = CloseHandle(m_semaphore);
        (void)ret;
        AZ_Assert(ret, "CloseHandle error: %d\n", GetLastError());
    }

    AZ_FORCE_INLINE void semaphore::acquire()
    {
        WaitForSingleObject(m_semaphore, AZ_INFINITE);
    }

    template <class Rep, class Period>
    AZ_FORCE_INLINE bool semaphore::try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
    {
        chrono::milliseconds timeToTry = rel_time;
        return (WaitForSingleObject(m_semaphore, static_cast<DWORD>(timeToTry.count())) == AZ_WAIT_OBJECT_0);
    }
    /*template <class Clock, class Duration>
    AZ_FORCE_INLINE bool semaphore::try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
        chrono::milliseconds now = chrono::system_clock::now().time_since_epoch();
        if(now<abs_time)
        {
            chrono::milliseconds timeToTry = abs_time - now;
            return try_acquire_for(timeToTry);
        }
        return false;
    }*/

    AZ_FORCE_INLINE void semaphore::release(unsigned int releaseCount)
    {
        ReleaseSemaphore(m_semaphore, releaseCount, NULL);
    }

    AZ_FORCE_INLINE semaphore::native_handle_type semaphore::native_handle()
    {
        return m_semaphore;
    }
}

#endif // AZSTD_SEMAPHORE_WINDOWS_H
#pragma once