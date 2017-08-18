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

#ifndef MacSpecific_h
#define MacSpecific_h

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_APPLE_OSX) || defined(AZ_PLATFORM_LINUX)
#include <pthread.h>

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

typedef uint32_t DWORD;

typedef union _LARGE_INTEGER
{
    struct
    {
        uint32_t LowPart;
        int32_t HighPart;
    };
    struct
    {
        uint32_t LowPart;
        int32_t HighPart;
    } u;

    int64_t QuadPart;
} LARGE_INTEGER;

bool QueryPerformanceCounter(LARGE_INTEGER* counter);

bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);

int WSAGetLastError();

#if defined(AZ_PLATFORM_LINUX)
namespace PthreadImplementation
{
    static pthread_mutex_t g_interlockMutex;
}

template<typename T>
const volatile T InterlockedIncrement(volatile T* pT)
{
    pthread_mutex_lock(&PthreadImplementation::g_interlockMutex);
    ++(*pT);
    pthread_mutex_unlock(&PthreadImplementation::g_interlockMutex);
    return *pT;
}

template<typename T>
const volatile T InterlockedDecrement(volatile T* pT)
{
    pthread_mutex_lock(&PthreadImplementation::g_interlockMutex);
    --(*pT);
    pthread_mutex_unlock(&PthreadImplementation::g_interlockMutex);
    return *pT;
}

#elif defined(AZ_PLATFORM_APPLE_OSX)

int32_t InterlockedIncrement(volatile int32_t* valueToIncrement);

int64_t InterlockedIncrement(volatile int64_t* valueToIncrement);

int32_t InterlockedDecrement(volatile int32_t* valueToDecrement);

int64_t InterlockedDecrement(volatile int64_t* valueToDecrement);

int64_t InterlockedAdd64(volatile int64_t* valueToUpdate, int64_t amountToAdd);

#endif

#endif

#endif
