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

#ifndef CRYINCLUDE_EDITOR_UTIL_THREADWIN32_H
#define CRYINCLUDE_EDITOR_UTIL_THREADWIN32_H
#pragma once


#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <process.h>

typedef UINT_PTR ThreadHandle;

namespace threads
{
    ///////////////////////////////////////////////////////////////////////////////
    // Win32 threading implementation.

    typedef void (_cdecl * ThreadFuncType)(void*);

    // Thread.
    ThreadHandle    begin(void* f, void* param)
    {
        ThreadFuncType func = (ThreadFuncType)f;
        return _beginthread(func, 0, param);
    }

    void end(ThreadHandle handle)
    {
    }

    DWORD   getCurrentThreadId()
    {
        return GetCurrentThreadId();
    }

    // Critical section.
    ThreadHandle    createCriticalSection()
    {
        CRITICAL_SECTION* lpSection = new CRITICAL_SECTION;
        InitializeCriticalSection(lpSection);
        return (ThreadHandle)lpSection;
    }

    void    deleteCriticalSection(ThreadHandle handle)
    {
        CRITICAL_SECTION* lpSection = (CRITICAL_SECTION*)handle;
        DeleteCriticalSection(lpSection);
        delete lpSection;
    }

    void    enterCriticalSection(ThreadHandle handle)
    {
        EnterCriticalSection((CRITICAL_SECTION*)handle);
    }

    void    leaveCriticalSection(ThreadHandle handle)
    {
        LeaveCriticalSection((CRITICAL_SECTION*)handle);
    }

    // Shared by synchronization objects.
    bool        closeHandle(ThreadHandle handle)
    {
        assert(CloseHandle((HANDLE)handle) == TRUE);
        return true;
    }

    EThreadWaitStatus   waitObject(ThreadHandle handle, ThreadHandle milliseconds)
    {
        DWORD status = WaitForSingleObjectEx((HANDLE)handle, milliseconds, TRUE);
        switch (status)
        {
        case WAIT_ABANDONED:
            return THREAD_WAIT_ABANDONED;
        case WAIT_OBJECT_0:
            return THREAD_WAIT_OBJECT_0;
        case WAIT_TIMEOUT:
            return THREAD_WAIT_TIMEOUT;
        case WAIT_IO_COMPLETION:
            return THREAD_WAIT_IO_COMPLETION;
        }
        return THREAD_WAIT_FAILED;
    }

    // Mutex.
    ThreadHandle    createMutex(bool own)
    {
        return (ThreadHandle)CreateMutex(NULL, own, NULL);
    }

    bool    releaseMutex(ThreadHandle handle)
    {
        if (ReleaseMutex((HANDLE)handle) != 0)
        {
            return true;
        }
        return false;
    }

    // Semaphore.
    ThreadHandle    createSemaphore(uint32 initCount, uint32 maxCount)
    {
        return (ThreadHandle)CreateSemaphore(NULL, initCount, maxCount, NULL);
    }

    bool    releaseSemaphore(ThreadHandle handle, int releaseCount)
    {
        if (ReleaseSemaphore((HANDLE)handle, releaseCount, NULL) != 0)
        {
            return true;
        }
        return false;
    }

    // Event
    ThreadHandle    createEvent(bool manualReset, bool initialState)
    {
        return (ThreadHandle)CreateEvent(NULL, manualReset, initialState, NULL);
    }

    bool    setEvent(ThreadHandle handle)
    {
        return SetEvent((HANDLE)handle);
    }

    bool    resetEvent(ThreadHandle handle)
    {
        return ResetEvent((HANDLE)handle);
    }

    bool    pulseEvent(ThreadHandle handle)
    {
        return PulseEvent((HANDLE)handle);
    }
} // namespace threads.
#endif // CRYINCLUDE_EDITOR_UTIL_THREADWIN32_H
