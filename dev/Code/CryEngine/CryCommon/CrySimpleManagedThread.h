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
#ifndef CRYINCLUDE_CRYCOMMON_CRYSIMPLEMANAGEDTHREAD_H
#define CRYINCLUDE_CRYCOMMON_CRYSIMPLEMANAGEDTHREAD_H
#pragma once

#include <CryThread.h>
#include <memory>

// Adapter that allows us to use CrySimpleThread more safely (lifetime and ownership
// are managed). This implementation holds a shared reference to itself until
// Terminate so that it can ensure Run is called on a valid object.
//
// Users should bind anything they need to keep alive as a shared_pointer to the
// function that will be invoked, or a weak_ptr that they subsequently check before
// use. The function reference is cleared on Terminate freeing any resource handles
// that the thread has.
class CrySimpleManagedThread
    : protected CrySimpleThread < >
{
public:
    // Function type; user should AZStd::bind any necessary arguments
    typedef AZStd::function<void()> function_type;

    // Factory function; creates, names and starts the thread
    static std::shared_ptr<CrySimpleManagedThread> CreateThread(const char* name, const function_type& fn);

    // Waits for the thread to finish executing
    void Join();

protected:
    // Copies are not permitted (this also suppresses implicit move)
    CrySimpleManagedThread(const CrySimpleManagedThread&) = delete;
    void operator=(const CrySimpleManagedThread&) = delete;

    // Initializing constructor
    CrySimpleManagedThread(const function_type& function);

    // Ensures the thread object lives as long as necessary
    std::shared_ptr<CrySimpleManagedThread> m_self;

    // Function to run (user supplied)
    function_type m_function;

    // see CrySimpleThread<>
    void Run() override;
    void Terminate() override;
};

inline std::shared_ptr<CrySimpleManagedThread> CrySimpleManagedThread::CreateThread(const char* name, const function_type& fn)
{
    // Note: Can't make_shared here because make_shared can't access a protected constructor
    auto t = std::shared_ptr<CrySimpleManagedThread>(new CrySimpleManagedThread(fn));
    t->m_self = t;
    t->SetName(name);
    t->Start();
    return t;
}

inline void CrySimpleManagedThread::Join()
{
    WaitForThread();
    Stop();
}

inline CrySimpleManagedThread::CrySimpleManagedThread(const function_type& function)
    : m_function(function)
{
}

inline void CrySimpleManagedThread::Run()
{
    // Execute user supplied function
    if (m_function)
    {
        m_function();
    }
}

inline void CrySimpleManagedThread::Terminate()
{
    function_type empty;
    m_function.swap(empty);
    m_self.reset();
}

#endif
