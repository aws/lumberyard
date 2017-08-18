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
#ifndef AZSTD_SEMAPHORE_APPLE_H
#define AZSTD_SEMAPHORE_APPLE_H

#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/internal/time_linux.h>

#include <unistd.h>

/**
 * This file is to be included from the semaphore.h only. It should NOT be included by the user.
 */

namespace AZStd
{
    inline semaphore::semaphore(unsigned int initialCount, unsigned int maximumCount)
    {
        // Clamp the semaphores initial value to prevent issues with the semaphore.
        int initSemCount = static_cast<int>(AZStd::min(initialCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        int result = semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, initSemCount);
        AZ_Assert(result == 0, "semaphore_create error %s\n", strerror(errno));

        int maxSemCount = static_cast<int>(AZStd::min(maximumCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        result = semaphore_create(mach_task_self(), &m_maxCountSemaphore, SYNC_POLICY_FIFO, maxSemCount);
        AZ_Assert(result == 0, "semaphore_create error for max count semaphore %s\n", strerror(errno));
    }

    inline semaphore::semaphore(const char* name, unsigned int initialCount, unsigned int maximumCount)
    {
        (void) name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
        // Clamp the semaphores initial value to prevent issues with the semaphore.
        int initSemCount = static_cast<int>(AZStd::min(initialCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        int result = semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, initSemCount);
        AZ_Assert(result == 0, "semaphore_create error %s\n", strerror(errno));
        
        int maxSemCount = static_cast<int>(AZStd::min(maximumCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        result = semaphore_create(mach_task_self(), &m_maxCountSemaphore, SYNC_POLICY_FIFO, maxSemCount);
        AZ_Assert(result == 0, "semaphore_create error for max count semaphore %s\n", strerror(errno));
    }

    inline semaphore::~semaphore()
    {
        int result = semaphore_destroy(mach_task_self(), m_semaphore);
        (void) result;
        AZ_Assert(result == 0, "semaphore_destroy error %s\n", strerror(errno));

        result = semaphore_destroy(mach_task_self(), m_maxCountSemaphore);
        AZ_Assert(result == 0, "semaphore_destroy error for max count semaphore:%s\n", strerror(errno));
    }

    AZ_FORCE_INLINE void semaphore::acquire()
    {
        int result = semaphore_signal(m_maxCountSemaphore);
        (void)result;
        AZ_Assert(result == 0, "semaphore_wait error for max count semaphore: %s\n", strerror(errno));

        result = semaphore_wait(m_semaphore);
        AZ_Assert(result == 0, "semaphore_wait error %s\n", strerror(errno));
    }

    template <class Rep, class Period>
    AZ_FORCE_INLINE bool semaphore::try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
    {
        mach_timespec_t mts;
        mts.tv_sec = static_cast<unsigned int>(chrono::seconds(rel_time).count());
        mts.tv_nsec = static_cast<clock_res_t>(chrono::nanoseconds(rel_time).count());

        int result = 0;
        while ((result = semaphore_timedwait(m_semaphore, mts)) == -1 && errno == EINTR)
        {
            continue; /* Restart if interrupted by handler */
        }

        AZ_Assert(result == 0 || errno == ETIMEDOUT, "semaphore_timedwait error %s\n", strerror(errno));
        return result == 0;
    }

    AZ_FORCE_INLINE void semaphore::release(unsigned int releaseCount)
    {
        int result = semaphore_wait(m_maxCountSemaphore);
        AZ_Assert(result == 0, "semaphore_wait error for max count semaphore: %s\n", strerror(errno));

        while (releaseCount)
        {
            result = semaphore_signal(m_semaphore);
            AZ_Assert(result == 0, "semaphore_signal error: %s\n", strerror(errno));

            if (result != 0)
            {
                break; // exit on error
            }

            --releaseCount;
        }
    }

    AZ_FORCE_INLINE semaphore::native_handle_type semaphore::native_handle()
    {
        return &m_semaphore;
    }
}

#endif // AZSTD_SEMAPHORE_APPLE_H
#pragma once
