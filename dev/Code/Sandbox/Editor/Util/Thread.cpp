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

#include "StdAfx.h"
#include "thread.h"

#ifdef WIN32
#include "ThreadWin32.h"
#else
//#include "ThreadOS.h"
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CThread class.
//
///////////////////////////////////////////////////////////////////////////////
void CThread::ThreadFunc(void* param)
{
    CThread* thread = (CThread*)param;
    thread->Run();

    threads::end(thread->m_handle);
    delete thread;
}

CThread::CThread()
{
    m_handle = 0;
}

void    CThread::Start()    // Start thread.
{
    m_handle = threads::begin(ThreadFunc, this);
}

uint32 CThread::GetCurrentId()
{
    return threads::getCurrentThreadId();
}

/*
///////////////////////////////////////////////////////////////////////////////
//
// Monitor class.
//
///////////////////////////////////////////////////////////////////////////////
void CMonitor::Lock()
{
    m_mutex.Wait();
}

void CMonitor::Release()
{
    m_mutex.Release();
}

CMonitor::Condition::Condition( CMonitor *mon )
{
    m_semCount = 0;
    m_monitor = mon;
}

void CMonitor::Condition::Wait()
{
    m_semCount++;                   // One more thread waiting for this condition.
    m_monitor->Release();   // Release monitor lock.
    m_semaphore.Wait();     // Block until condition signaled.
    m_monitor->Lock();      // If signaled and unblocked, re-aquire monitor`s lock.
    m_semCount--;                   // Got monitor lock, no longer in wait state.
}

void CMonitor::Condition::Signal()
{
    // Release any thread blocked by semaphore.
    if (m_semCount > 0)
        m_semaphore.Release();
}
*/