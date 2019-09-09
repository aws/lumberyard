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
#ifndef AZ_UNITY_BUILD

#include <AzCore/std/time.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define TIME_CPP_SECTION_1 1
#define TIME_CPP_SECTION_2 2
#define TIME_CPP_SECTION_3 3
#define TIME_CPP_SECTION_4 4
#define TIME_CPP_SECTION_5 5
#define TIME_CPP_SECTION_6 6
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TIME_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/time_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/time_cpp_provo.inl"
    #endif
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#   include <time.h>
#   include <errno.h>
#elif defined(AZ_PLATFORM_WINDOWS)
#   include <AzCore/std/parallel/config.h>  // We need this for windows.h to use LARGE_INTEGER,QueryPerformanceCounter and QueryPerformanceFrequency
#   include <AzCore/PlatformIncl.h>
#endif

#if defined(AZ_PLATFORM_APPLE)
    #include <mach/mach.h>
    #include <mach/clock.h>
    #include <mach/mach_time.h>
#endif

namespace AZStd
{
    AZStd::sys_time_t GetTimeTicksPerSecond()
    {
#if AZ_TRAIT_OS_USE_WINDOWS_QUERY_PERFORMANCE_COUNTER
        static AZStd::sys_time_t freq = 0;
        if (freq == 0)
        {
            QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        }
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TIME_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/time_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/time_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif (defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE))
        // Value is in nano seconds, return number of nano seconds in one second
        AZStd::sys_time_t freq = 1000000000L;
#else
        #error Implement this!
#endif
        return freq;
    }

    AZStd::sys_time_t GetTimeNowTicks()
    {
        AZStd::sys_time_t timeNow;
#if AZ_TRAIT_OS_USE_WINDOWS_QUERY_PERFORMANCE_COUNTER
        QueryPerformanceCounter((LARGE_INTEGER*)&timeNow);
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TIME_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/time_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/time_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif (defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE))
        struct timespec ts;
#   if defined(AZ_PLATFORM_APPLE)
        clock_serv_t cclock;
        mach_timespec_t mts;
        kern_return_t ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        if (ret == KERN_SUCCESS)
        {
            ret = clock_get_time(cclock, &mts);
            if (ret == KERN_SUCCESS)
            {
                ts.tv_sec = mts.tv_sec;
                ts.tv_nsec = mts.tv_nsec;
            }
            else
            {
                AZ_Assert(false, "clock_get_time error: %d\n", ret);
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
        else
        {
            AZ_Assert(false, "clock_get_time error: %d\n", ret);
        }
#   else
        int result = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);  // Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not subject to NTP adjustments.
        if (result < 0)
        {
            // NOTE(HS): WSL doesn't implement support for CLOCK_MONOTONIC_RAW and there
            // is no way to check platform for conditional compile. Fallback to supported
            // option if desired option fails.
            result = clock_gettime(CLOCK_MONOTONIC, &ts);
        }
        AZ_Assert(result != -1, "clock_gettime error: %s\n", strerror(errno));
        (void)result;
#   endif

        timeNow =  ts.tv_sec * GetTimeTicksPerSecond() + ts.tv_nsec;
#else
#error You need to implement this!
#endif
        return timeNow;
    }

    AZStd::sys_time_t GetTimeNowMicroSecond()
    {
        AZStd::sys_time_t timeNowMicroSecond;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TIME_CPP_SECTION_4
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/time_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/time_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        timeNowMicroSecond =  GetTimeNowTicks() / 1000L;
#elif defined(AZ_PLATFORM_WINDOWS)
        // NOTE: The default line below was not working on systems with smaller TicksPerSecond() values (like in Windows7, for example)
        // So we are now spreading the division between the numerator and denominator to maintain better precision
        timeNowMicroSecond = (GetTimeNowTicks() * 1000) / (GetTimeTicksPerSecond() / 1000);
#else
        timeNowMicroSecond = GetTimeNowTicks() / (GetTimeTicksPerSecond() / 1000000);
#endif
        return timeNowMicroSecond;
    }

    // time in seconds is the same as the POSIX time_t
    AZStd::sys_time_t GetTimeNowSecond()
    {
        AZStd::sys_time_t timeNowSecond;
#if AZ_TRAIT_OS_USE_WINDOWS_QUERY_PERFORMANCE_COUNTER
        // Using get tick count, since it's more stable for longer time measurements.
        timeNowSecond = GetTickCount() / 1000;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TIME_CPP_SECTION_5
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/time_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/time_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif (defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE))
        struct timespec ts;
#   if defined(AZ_PLATFORM_APPLE)
        clock_serv_t cclock;
        mach_timespec_t mts;
        kern_return_t ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        if (ret == KERN_SUCCESS)
        {
            ret = clock_get_time(cclock, &mts);
            if (ret == KERN_SUCCESS)
            {
                ts.tv_sec = mts.tv_sec;
                ts.tv_nsec = mts.tv_nsec;
            }
            else
            {
                AZ_Assert(false, "clock_get_time error: %d\n", ret);
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
        else
        {
            AZ_Assert(false, "clock_get_time error: %d\n", ret);
        }
#   else
        int result = clock_gettime(CLOCK_MONOTONIC_RAW, &ts);  // Similar to CLOCK_MONOTONIC, but provides access to a raw hardware-based time that is not subject to NTP adjustments.
        (void)result;
        AZ_Assert(result != -1, "clock_gettime error: %s\n", strerror(errno));
#   endif
        timeNowSecond =  ts.tv_sec;

#else
        timeNowSecond = GetTimeNowTicks() / GetTimeTicksPerSecond();
#endif
        return timeNowSecond;
    }

    AZ::u64 GetTimeUTCMilliSecond()
    {
        AZ::u64 utc;
#if AZ_TRAIT_OS_USE_WINDOWS_QUERY_PERFORMANCE_COUNTER
        FILETIME UTCFileTime;
        GetSystemTimeAsFileTime(&UTCFileTime);
        // store time in 100 of nanoseconds since January 1, 1601 UTC
        utc = (AZ::u64)UTCFileTime.dwHighDateTime << 32 | UTCFileTime.dwLowDateTime;
        // convert to since 1970/01/01 00:00:00 UTC
        utc -= 116444736000000000;
        // convert to millisecond
        utc /= 10000;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION TIME_CPP_SECTION_6
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/time_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/time_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        struct timespec ts;
#   if defined(AZ_PLATFORM_APPLE)
        clock_serv_t cclock;
        mach_timespec_t mts;
        kern_return_t ret = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        if (ret == KERN_SUCCESS)
        {
            ret = clock_get_time(cclock, &mts);
            if (ret == KERN_SUCCESS)
            {
                ts.tv_sec = mts.tv_sec;
                ts.tv_nsec = mts.tv_nsec;
            }
            else
            {
                AZ_Assert(false, "clock_get_time error: %d\n", ret);
            }
            mach_port_deallocate(mach_task_self(), cclock);
        }
        else
        {
            AZ_Assert(false, "clock_get_time error: %d\n", ret);
        }
#   else
        int result = clock_gettime(CLOCK_REALTIME, &ts);
        (void)result;
        AZ_Assert(result != -1, "clock_gettime error %s\n", strerror(errno));
#   endif
        utc =  ts.tv_sec * 1000L +  ts.tv_nsec / 1000000L;

#else
        AZ_Assert(false, "UTC time not available on platform.");
        utc = 0;
#endif
        return utc;
    }
}

#endif // #ifndef AZ_UNITY_BUILD
