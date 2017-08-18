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

#include "WindowsAPIImplementation.h"
#if defined(AZ_PLATFORM_APPLE_OSX) || defined(AZ_PLATFORM_LINUX)

#include <errno.h>
#import <mach/mach_time.h>
#include <libkern/OSAtomic.h>

bool QueryPerformanceCounter(LARGE_INTEGER* counter)
{
#if defined(LINUX)
    // replaced gettimeofday
    // http://fixunix.com/kernel/378888-gettimeofday-resolution-linux.html
    timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    counter->QuadPart = (uint64_t)tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
#elif defined(APPLE)
    counter->QuadPart = mach_absolute_time();
#endif
    return true;
}

bool QueryPerformanceFrequency(LARGE_INTEGER* frequency)
{
#if defined(LINUX)
    // On Linux we'll use gettimeofday().  The API resolution is microseconds,
    // so we'll report that to the caller.
    frequency->u.LowPart  = 1000000;
    frequency->u.HighPart = 0;
#elif defined(APPLE)
    static mach_timebase_info_data_t s_kTimeBaseInfoData;
    if (s_kTimeBaseInfoData.denom == 0)
    {
        mach_timebase_info(&s_kTimeBaseInfoData);
    }
    // mach_timebase_info_data_t expresses the tick period in nanoseconds
    frequency->QuadPart = 1e+9 * (uint64_t)s_kTimeBaseInfoData.denom / (uint64_t)s_kTimeBaseInfoData.numer;
#endif
    return true;
}

int WSAGetLastError()
{
    return errno;
}

int32_t InterlockedIncrement(volatile int32_t* valueToIncrement)
{
    return OSAtomicIncrement32(valueToIncrement);
}

int64_t InterlockedIncrement(volatile int64_t* valueToIncrement)
{
    return OSAtomicIncrement64(valueToIncrement);
}

int32_t InterlockedDecrement(volatile int32_t* valueToDecrement)
{
    return OSAtomicDecrement32(valueToDecrement);
}

int64_t InterlockedDecrement(volatile int64_t* valueToDecrement)
{
    return OSAtomicDecrement64(valueToDecrement);
}

int64_t InterlockedAdd64(volatile int64_t* valueToUpdate, int64_t amountToAdd)
{
    return OSAtomicAdd64(amountToAdd, valueToUpdate);
}

#endif
