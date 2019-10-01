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

#include <time.h>
#include <errno.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>

namespace AZStd
{
    namespace Internal
    {
        AZ_FORCE_INLINE timespec CurrentTime()
        {
            struct timespec ts;
            clock_serv_t cclock;
            mach_timespec_t mts;
            host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);
            ts.tv_sec = mts.tv_sec;
            ts.tv_nsec = mts.tv_nsec;
            return ts;
        }

        template <class Rep, class Period>
        AZ_FORCE_INLINE timespec CurrentTimeAndOffset(const chrono::duration<Rep, Period>& rel_time)
        {
            timespec ts = CurrentTime();

            chrono::seconds timeInSeconds = rel_time;
            ts.tv_sec += timeInSeconds.count();
            ts.tv_nsec += chrono::microseconds(rel_time - timeInSeconds).count() * 1000L;

            return ts;
        }
    }
}