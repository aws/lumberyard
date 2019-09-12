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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// This file should only be included Once in DLL module.

#pragma once

#include <platform.h>

#if defined(AZ_MONOLITHIC_BUILD)
#   error It is not allowed to have AZ_MONOLITHIC_BUILD defined
#endif

#include <CryCommon.cpp>
#include <IRCLog.h>

#include <Random.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include "CryThreadImpl_windows.h"
#endif

#if defined(AZ_PLATFORM_WINDOWS)
#define TLSALLOC(k)     (*(k)=TlsAlloc(), TLS_OUT_OF_INDEXES==*(k))
#define TLSFREE(k)		(!TlsFree(k))
#define TLSGET(k)		TlsGetValue(k)
#define TLSSET(k, a)	(!TlsSetValue(k, a))
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
#define TLSALLOC(k)     pthread_key_create(k, 0)
#define TLSFREE(k)		pthread_key_delete(k)
#define TLSGET(k)		pthread_getspecific(k)
#define TLSSET(k, a)	pthread_setspecific(k, a)
#else
#error TLS Not supported!!
#endif

CRndGen CryRandom_Internal::g_random_generator;

struct SSystemGlobalEnvironment;
SSystemGlobalEnvironment* gEnv = 0;
IRCLog* g_pRCLog = 0;

//////////////////////////////////////////////////////////////////////////
// This is an entry to DLL initialization function that must be called for each loaded module
//////////////////////////////////////////////////////////////////////////
extern "C" DLL_EXPORT void ModuleInitISystem(ISystem* pSystem, const char* moduleName)
{
    if (gEnv) // Already registered.
    {
        return;
    }

    if (pSystem)
    {
        gEnv = pSystem->GetGlobalEnvironment();
    }
}

extern "C" DLL_EXPORT void InjectEnvironment(void* env)
{
    static bool injected = false;
    if (!injected)
    {
        AZ::Environment::Attach(reinterpret_cast<AZ::EnvironmentInstance>(env));
        injected = true;
    }
}

extern "C" DLL_EXPORT void DetachEnvironment()
{
    AZ::Environment::Detach();
}

//These functions are defined in WinBase.cpp for other platforms
#if defined(AZ_PLATFORM_WINDOWS)
void CrySleep(unsigned int dwMilliseconds)
{
    Sleep(dwMilliseconds);
}


void CryLowLatencySleep(unsigned int dwMilliseconds)
{
    CrySleep(dwMilliseconds);
}


LONG    CryInterlockedCompareExchange(LONG volatile* dst, LONG exchange, LONG comperand)
{
    return InterlockedCompareExchange(dst, exchange, comperand);
}

LONG  CryInterlockedIncrement(int volatile* lpAddend)
{
    return InterlockedIncrement((volatile LONG*)lpAddend);
}

LONG  CryInterlockedDecrement(int volatile* lpAddend)
{
    return InterlockedDecrement((volatile LONG*)lpAddend);
}

LONG    CryInterlockedExchangeAdd(LONG volatile* lpAddend, LONG Value)
{
    return InterlockedExchangeAdd(lpAddend, Value);
}
#endif

void SetRCLog(IRCLog* pRCLog)
{
    g_pRCLog = pRCLog;
}

void RCLog(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Info, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

void RCLogWarning(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Warning, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

void RCLogError(const char* szFormat, ...)
{
    va_list args;
    va_start (args, szFormat);
    if (g_pRCLog)
    {
        g_pRCLog->LogV(IRCLog::eType_Error, szFormat, args);
    }
    else
    {
        vprintf(szFormat, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

void RCLogContext(const char* szMessage)
{
    if (g_pRCLog)
    {
        g_pRCLog->Log(IRCLog::eType_Context, szMessage);
    }
    else
    {
        printf("%s\n", szMessage);
        fflush(stdout);
    }
}


//////////////////////////////////////////////////////////////////////////
// Log important data that must be printed regardless verbosity.

void PlatformLogOutput(const char*, ...) PRINTF_PARAMS(1, 2);

inline void PlatformLogOutput(const char* format, ...)
{
    assert(g_pRCLog);
    if (g_pRCLog)
    {
        va_list args;
        va_start(args, format);
        g_pRCLog->LogV(IRCLog::eType_Error,  format, args);
        va_end(args);
    }
}


// when using STL Port _STLP_DEBUG and _STLP_DEBUG_TERMINATE - avoid actually
// crashing (default terminator seems to kill the thread, which isn't nice).
#ifdef _STLP_DEBUG_TERMINATE
void __stl_debug_terminate(void)
{
    assert(0 && "STL Debug Error");
}
#endif

#ifdef _STLP_DEBUG_MESSAGE
void __stl_debug_message(const char* format_str, ...)
{
    va_list __args;
    va_start(__args, format_str);
    char __buffer[4096];
    azvsnprintf(__buffer, sizeof(__buffer), format_str, __args);
    PlatformLogOutput(__buffer); // Log error.
    va_end(__args);
}
#endif //_STLP_DEBUG_MESSAGE

//Indices for a box side used for culling
_MS_ALIGN(64) uint32  BoxSides[0x40 * 8] = {
    0, 0, 0, 0, 0, 0, 0, 0, //00
    0, 4, 6, 2, 0, 0, 0, 4, //01
    7, 5, 1, 3, 0, 0, 0, 4, //02
    0, 0, 0, 0, 0, 0, 0, 0, //03
    0, 1, 5, 4, 0, 0, 0, 4, //04
    0, 1, 5, 4, 6, 2, 0, 6, //05
    7, 5, 4, 0, 1, 3, 0, 6, //06
    0, 0, 0, 0, 0, 0, 0, 0, //07
    7, 3, 2, 6, 0, 0, 0, 4, //08
    0, 4, 6, 7, 3, 2, 0, 6, //09
    7, 5, 1, 3, 2, 6, 0, 6, //0a
    0, 0, 0, 0, 0, 0, 0, 0, //0b
    0, 0, 0, 0, 0, 0, 0, 0, //0c
    0, 0, 0, 0, 0, 0, 0, 0, //0d
    0, 0, 0, 0, 0, 0, 0, 0, //0e
    0, 0, 0, 0, 0, 0, 0, 0, //0f
    0, 2, 3, 1, 0, 0, 0, 4, //10
    0, 4, 6, 2, 3, 1, 0, 6, //11
    7, 5, 1, 0, 2, 3, 0, 6, //12
    0, 0, 0, 0, 0, 0, 0, 0, //13
    0, 2, 3, 1, 5, 4, 0, 6, //14
    1, 5, 4, 6, 2, 3, 0, 6, //15
    7, 5, 4, 0, 2, 3, 0, 6, //16
    0, 0, 0, 0, 0, 0, 0, 0, //17
    0, 2, 6, 7, 3, 1, 0, 6, //18
    0, 4, 6, 7, 3, 1, 0, 6, //19
    7, 5, 1, 0, 2, 6, 0, 6, //1a
    0, 0, 0, 0, 0, 0, 0, 0, //1b
    0, 0, 0, 0, 0, 0, 0, 0, //1c
    0, 0, 0, 0, 0, 0, 0, 0, //1d
    0, 0, 0, 0, 0, 0, 0, 0, //1e
    0, 0, 0, 0, 0, 0, 0, 0, //1f
    7, 6, 4, 5, 0, 0, 0, 4, //20
    0, 4, 5, 7, 6, 2, 0, 6, //21
    7, 6, 4, 5, 1, 3, 0, 6, //22
    0, 0, 0, 0, 0, 0, 0, 0, //23
    7, 6, 4, 0, 1, 5, 0, 6, //24
    0, 1, 5, 7, 6, 2, 0, 6, //25
    7, 6, 4, 0, 1, 3, 0, 6, //26
    0, 0, 0, 0, 0, 0, 0, 0, //27
    7, 3, 2, 6, 4, 5, 0, 6, //28
    0, 4, 5, 7, 3, 2, 0, 6, //29
    6, 4, 5, 1, 3, 2, 0, 6, //2a
    0, 0, 0, 0, 0, 0, 0, 0, //2b
    0, 0, 0, 0, 0, 0, 0, 0, //2c
    0, 0, 0, 0, 0, 0, 0, 0, //2d
    0, 0, 0, 0, 0, 0, 0, 0, //2e
    0, 0, 0, 0, 0, 0, 0, 0, //2f
    0, 0, 0, 0, 0, 0, 0, 0, //30
    0, 0, 0, 0, 0, 0, 0, 0, //31
    0, 0, 0, 0, 0, 0, 0, 0, //32
    0, 0, 0, 0, 0, 0, 0, 0, //33
    0, 0, 0, 0, 0, 0, 0, 0, //34
    0, 0, 0, 0, 0, 0, 0, 0, //35
    0, 0, 0, 0, 0, 0, 0, 0, //36
    0, 0, 0, 0, 0, 0, 0, 0, //37
    0, 0, 0, 0, 0, 0, 0, 0, //38
    0, 0, 0, 0, 0, 0, 0, 0, //39
    0, 0, 0, 0, 0, 0, 0, 0, //3a
    0, 0, 0, 0, 0, 0, 0, 0, //3b
    0, 0, 0, 0, 0, 0, 0, 0, //3c
    0, 0, 0, 0, 0, 0, 0, 0, //3d
    0, 0, 0, 0, 0, 0, 0, 0, //3e
    0, 0, 0, 0, 0, 0, 0, 0, //3f
};
