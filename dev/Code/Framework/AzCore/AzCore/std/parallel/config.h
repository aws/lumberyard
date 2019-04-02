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

#include <AzCore/std/base.h>

#include <AzCore/PlatformDef.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CONFIG_H_SECTION_1 1
#define CONFIG_H_SECTION_2 2
#endif

/**
 * \page Parallel Parallel Processing
 *
 * Parallel processing is supported as much as possible on
 * each platform. It has lot's of limitations, please refer to each primitive
 * for more information. We are conforming with \ref C++0x but of course have
 * have limited support.
 *
 * \li \ref AZStd::atomic                                           
 * \li \ref AZStd::mutex (a.k.a critical_section)                   
 * \li \ref AZStd::recursive_mutex (a.k.a critical_section)         
 * \li \ref AZStd::semaphore                                        
 * \li \ref AZStd::thread                                           
 * \li \ref AZStd::condition_variable                               
 * \li \ref AZStd::lock
 * \li \ref ParallelContainers
 *
 */
#if AZ_TRAIT_USE_WINDOWS_SYNCHRONIZATION_LIBRARY
    #include <intrin.h>
    #include <AzCore/std/typetraits/aligned_storage.h>
    #pragma intrinsic (_InterlockedIncrement)
    #pragma intrinsic (_InterlockedDecrement)

    //////////////////////////////////////////////////////////////////////////
    // forward declare to avoid windows.h
    typedef void *HANDLE;
    typedef unsigned long DWORD;
    typedef int BOOL;
    typedef long LONG, *LPLONG;
    typedef const char* LPCSTR;
    typedef const wchar_t *LPCWSTR;
#define BASE_API_IMPORT __declspec(dllimport)
#ifndef WAIT_OBJECT_0
#   define AZ_WAIT_OBJECT_0 0
#else 
#   define AZ_WAIT_OBJECT_0 WAIT_OBJECT_0
#endif 
#ifndef INFINITE
#   define AZ_INFINITE            0xFFFFFFFF  // Infinite timeout
#else
#   define AZ_INFINITE            INFINITE 
#endif

    extern "C"
    {
        // CRITICAL SECTION
        typedef struct _RTL_CRITICAL_SECTION RTL_CRITICAL_SECTION;
        typedef RTL_CRITICAL_SECTION CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

        BASE_API_IMPORT void __stdcall InitializeCriticalSection(LPCRITICAL_SECTION);
        BASE_API_IMPORT void __stdcall DeleteCriticalSection(LPCRITICAL_SECTION);
        BASE_API_IMPORT void __stdcall EnterCriticalSection(LPCRITICAL_SECTION);
        BASE_API_IMPORT void __stdcall LeaveCriticalSection(LPCRITICAL_SECTION);
        BASE_API_IMPORT BOOL __stdcall TryEnterCriticalSection(LPCRITICAL_SECTION);
    
        // CONDITIONAL VARIABLE
        typedef struct _RTL_CONDITION_VARIABLE RTL_CONDITION_VARIABLE;
        typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;

        BASE_API_IMPORT void __stdcall InitializeConditionVariable(PCONDITION_VARIABLE);
        BASE_API_IMPORT void __stdcall WakeConditionVariable(PCONDITION_VARIABLE);
        BASE_API_IMPORT void __stdcall WakeAllConditionVariable(PCONDITION_VARIABLE);
        BASE_API_IMPORT BOOL __stdcall SleepConditionVariableCS(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD);

        // Generic
        BASE_API_IMPORT DWORD _stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
        BASE_API_IMPORT BOOL _stdcall CloseHandle(HANDLE hObject);
        BASE_API_IMPORT DWORD _stdcall GetLastError();
        BASE_API_IMPORT void _stdcall Sleep(DWORD);
        typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

        // Semaphore
        BASE_API_IMPORT HANDLE _stdcall CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName);
        BASE_API_IMPORT HANDLE _stdcall CreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
        BASE_API_IMPORT BOOL _stdcall ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);
        #ifndef CreateSemaphore
        #   ifdef UNICODE
        #       define CreateSemaphore CreateSemaphoreW
        #   else
        #       define CreateSemaphore CreateSemaphoreA
        #   endif
        #endif // !def CreateSemaphore

        // Event
        BASE_API_IMPORT HANDLE _stdcall CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
        BASE_API_IMPORT HANDLE _stdcall CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
        #ifndef CreateEvent
        #   ifdef UNICODE
        #       define CreateEvent CreateEventW
        #   else
        #       define CreateEvent CreateEventA
        #   endif
        #endif // !def CreateSemaphore
        BASE_API_IMPORT BOOL _stdcall SetEvent(HANDLE);
    }
    //////////////////////////////////////////////////////////////////////////



#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CONFIG_H_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/config_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/config_h_provo.inl"
    #endif
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#   include <pthread.h>
#   include <semaphore.h>
#   include <sys/time.h>
#endif

#if defined(AZ_PLATFORM_APPLE)
#   include <mach/mach.h>
#endif

/**
 * Native synchronization types.
 */
namespace AZStd
{
    namespace Internal
    {
        class thread_info;
    }

#if AZ_TRAIT_USE_WINDOWS_SYNCHRONIZATION_LIBRARY
    // Mutex
    typedef AZStd::aligned_storage<40,8>::type  native_mutex_data_type; // declare storage for CRITICAL_SECTION (40 bytes on x64, 24 on x32) to avoid windows.h include
    typedef CRITICAL_SECTION*                   native_mutex_handle_type;
    typedef native_mutex_data_type              native_recursive_mutex_data_type;
    typedef CRITICAL_SECTION*                   native_recursive_mutex_handle_type;

    typedef AZStd::aligned_storage<8,8>::type  native_cond_var_data_type; // declare storage for CONDITIONAL_VARIABLE (8 bytes on x64, 4 bytes on x86) to avoid windows.h include
    typedef CONDITION_VARIABLE*  native_cond_var_handle_type;

    // Semaphore
    typedef HANDLE              native_semaphore_data_type;
    typedef HANDLE              native_semaphore_handle_type;

    // Thread
    typedef unsigned            native_thread_id_type;
    static const unsigned       native_thread_invalid_id = 0;
    struct native_thread_data_type
    {
        HANDLE                          m_handle;
        native_thread_id_type           m_id;
    };
    typedef HANDLE              native_thread_handle_type;

#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CONFIG_H_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/config_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/config_h_provo.inl"
    #endif
#elif defined (AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    // Mutex
    typedef pthread_mutex_t     native_mutex_data_type;
    typedef pthread_mutex_t*    native_mutex_handle_type;
    typedef pthread_mutex_t     native_recursive_mutex_data_type;
    typedef pthread_mutex_t*    native_recursive_mutex_handle_type;

    // Condition variable
    typedef pthread_cond_t      native_cond_var_data_type;
    typedef pthread_cond_t*     native_cond_var_handle_type;

    // Semaphore
#if !defined(AZ_PLATFORM_APPLE)
    typedef sem_t           native_semaphore_data_type;
    typedef sem_t*          native_semaphore_handle_type;
#endif

    // Thread
    typedef pthread_t       native_thread_id_type;
#if defined(AZ_PLATFORM_ANDROID)
    static pthread_t        native_thread_invalid_id = LONG_MIN;
#else
    static pthread_t        native_thread_invalid_id = 0;
#endif
    typedef pthread_t       native_thread_data_type;
    typedef pthread_t       native_thread_handle_type;

#endif

#if defined(AZ_PLATFORM_APPLE)
    typedef semaphore_t         native_semaphore_data_type;
    typedef semaphore_t*        native_semaphore_handle_type;
#endif

    /**
     * Thread id used only as internal type, use thread::id.
     * \note we can't use class id forward in the thread class like the standard implements
     * because some compilers fail to compile it. This this works fine and it's 100% complaint.
     */
    struct thread_id
    {
        thread_id()
            : m_id(native_thread_invalid_id) {}
        thread_id(native_thread_id_type threadId)
            : m_id(threadId) {}
        native_thread_id_type m_id;
    };

    AZ_FORCE_INLINE bool operator==(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id == y.m_id; }
    AZ_FORCE_INLINE bool operator!=(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id != y.m_id; }
    AZ_FORCE_INLINE bool operator<(AZStd::thread_id  x, AZStd::thread_id y)     { return x.m_id < y.m_id; }
    AZ_FORCE_INLINE bool operator<=(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id <= y.m_id; }
    AZ_FORCE_INLINE bool operator>(AZStd::thread_id  x, AZStd::thread_id y)     { return x.m_id > y.m_id; }
    AZ_FORCE_INLINE bool operator>=(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id >= y.m_id; }
    //  template<class charT, class traits>
    //  basic_ostream<charT, traits>&
    //      operator<< (basic_ostream<charT, traits>& out, AZStd::thread::id id);
}
