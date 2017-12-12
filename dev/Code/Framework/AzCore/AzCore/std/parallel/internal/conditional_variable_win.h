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
#ifndef AZSTD_CONDITIONAL_VARIABLE_WINDOWS_H
#define AZSTD_CONDITIONAL_VARIABLE_WINDOWS_H

/**
 * This file is to be included from the mutex.h only. It should NOT be included by the user.
 */

namespace AZStd
{
#define AZ_COND_VAR_CAST(m) reinterpret_cast<PCONDITION_VARIABLE>(&m)
#define AZ_STD_MUTEX_CAST(m) reinterpret_cast<LPCRITICAL_SECTION>(&m)


    //////////////////////////////////////////////////////////////////////////
    // Condition variable
    inline condition_variable::condition_variable()
    {
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable::condition_variable(const char* name)
    {
        (void)name;
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable::~condition_variable()
    {
    }

    inline void condition_variable::notify_one()
    {
        WakeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }
    inline void condition_variable::notify_all()
    {
        WakeAllConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }
    inline void condition_variable::wait(unique_lock<mutex>& lock)
    {
        SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), lock.mutex()->native_handle(), AZ_INFINITE);
    }
    template <class Predicate>
    inline void condition_variable::wait(unique_lock<mutex>& lock, Predicate pred)
    {
        while (!pred())
        {
            wait(lock);
        }
    }

    template <class Clock, class Duration>
    inline bool condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time)
    {
        chrono::system_clock::time_point now = chrono::system_clock::now();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now);
        }

        return false;
    }
    template <class Clock, class Duration, class Predicate>
    inline bool condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {
        chrono::system_clock::time_point now = chrono::system_clock::now();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now, pred);
        }

        return false;
    }
    template <class Rep, class Period>
    inline bool condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time)
    {
        chrono::milliseconds toWait = rel_time;
        // We need to make sure we use CriticalSection based mutex.
        int ret = SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), lock.mutex()->native_handle(), static_cast<DWORD>(toWait.count()));
        return (ret == AZ_WAIT_OBJECT_0);
    }
    template <class Rep, class Period, class Predicate>
    inline bool condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        while (!pred())
        {
            if (!wait_for(lock, rel_time))
            {
                return pred();
            }
        }
        return pred();
    }
    condition_variable::native_handle_type
    inline condition_variable::native_handle()
    {
        return AZ_COND_VAR_CAST(m_cond_var);
    }

    //////////////////////////////////////////////////////////////////////////
    // Condition variable any
    inline condition_variable_any::condition_variable_any()
    {
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable_any::condition_variable_any(const char* name)
    {
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        (void)name;
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable_any::~condition_variable_any()
    {
        DeleteCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    inline void condition_variable_any::notify_one()
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        WakeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    inline void condition_variable_any::notify_all()
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        WakeAllConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    template<class Lock>
    inline void condition_variable_any::wait(Lock& lock)
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        lock.unlock();
        // We need to make sure we use CriticalSection based mutex.
        SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), AZ_STD_MUTEX_CAST(m_mutex), AZ_INFINITE);
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        lock.lock();
    }
    template <class Lock, class Predicate>
    inline void condition_variable_any::wait(Lock& lock, Predicate pred)
    {
        while (!pred())
        {
            wait(lock);
        }
    }
    template <class Lock, class Clock, class Duration>
    inline bool condition_variable_any::wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time)
    {
        chrono::milliseconds now = chrono::system_clock::now().time_since_epoch();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now);
        }

        return false;
    }
    template <class Lock, class Clock, class Duration, class Predicate>
    inline bool condition_variable_any::wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {
        chrono::milliseconds now = chrono::system_clock::now().time_since_epoch();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now, pred);
        }

        return false;
    }
    template <class Lock, class Rep, class Period>
    inline bool condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time)
    {
        chrono::milliseconds toWait = rel_time;
        EnterCriticalSection(&m_mutex);
        lock.unlock();

        // We need to make sure we use CriticalSection based mutex.
        int ret = SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), AZ_STD_MUTEX_CAST(m_mutex), toWait.count());
        LeaveCriticalSection(&m_mutex);
        lock.lock();
        return (ret == AZ_WAIT_OBJECT_0);
    }
    template <class Lock, class Rep, class Period, class Predicate>
    inline bool condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        while (!pred())
        {
            if (!wait_for(lock, rel_time))
            {
                return pred();
            }
        }
        return pred();
    }
    condition_variable_any::native_handle_type
    inline condition_variable_any::native_handle()
    {
        return AZ_COND_VAR_CAST(m_cond_var);
    }
    //////////////////////////////////////////////////////////////////////////
#undef AZ_COND_VAR_CAST //
#undef AZ_STD_MUTEX_CAST
}



#endif // AZSTD_MUTEX_WINDOWS_H
#pragma once