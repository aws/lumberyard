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

#ifndef CRYINCLUDE_EDITOR_UTIL_THREAD_H
#define CRYINCLUDE_EDITOR_UTIL_THREAD_H
#pragma once


#include <afxmt.h>
#include <deque>

enum EThreadWaitStatus
{
    THREAD_WAIT_FAILED,
    THREAD_WAIT_ABANDONED,
    THREAD_WAIT_OBJECT_0,
    THREAD_WAIT_TIMEOUT,
    THREAD_WAIT_IO_COMPLETION
};

//////////////////////////////////////////////////////////////////////////
// Thread.
//////////////////////////////////////////////////////////////////////////
class CRYEDIT_API CThread
{
public:
    CThread();

    void  Start();  // Start thread.

    static uint32 GetCurrentId();

protected:
    virtual ~CThread() {};
    static void ThreadFunc(void* param);

    virtual void Run() = 0; // Derived classes must ovveride this.

    UINT_PTR m_handle;
};

///////////////////////////////////////////////////////////////////////////////
//
// MTDeqeue class, Multithread Safe Deque container.
//
template <class T>
class CRYEDIT_API MTDeque
{
public:
    typedef T value_type;

    bool empty() const
    {
        cs.Lock();
        bool isempty = q.empty();
        cs.Unlock();
        return isempty;
    }

    void resize(int sz)
    {
        cs.Lock();
        q.resize(sz);
        cs.Unlock();
    }

    void clear()
    {
        cs.Lock();
        q.clear();
        cs.Unlock();
    }

    void push_front(const T& x)
    {
        cs.Lock();
        q.push_front(x);
        cs.Unlock();
    }

    void push_back(const T& x)
    {
        cs.Lock();
        q.push_back(x);
        cs.Unlock();
    }

    bool pop_front(T& to)
    {
        cs.Lock();
        if (q.empty())
        {
            cs.Unlock();
            return false;
        }
        to = q.front();
        q.pop_front();
        cs.Unlock();
        return true;
    }

    bool pop_back(T& to)
    {
        cs.Lock();
        if (q.empty())
        {
            cs.Unlock();
            return false;
        }
        to = q.back();
        q.pop_back();
        cs.Unlock();
        return true;
    }

private:
    std::deque<T> q;
    mutable CCriticalSection cs;
};

///////////////////////////////////////////////////////////////////////////////
//
// MTQueue class, Multithread Safe Queue container.
//
template <class T>
class MTQueue
{
public:
    bool empty() const { return q.empty(); };
    void clear() { return q.clear(); };
    void push(const T& x){ return q.push_back(x); };
    bool pop(T& to) { return q.pop_front(to); }

private:
    MTDeque<T> q;
};

#endif // CRYINCLUDE_EDITOR_UTIL_THREAD_H
