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
#ifndef AZSTD_CONDITIONAL_VARIABLE_LINUX_H
#define AZSTD_CONDITIONAL_VARIABLE_LINUX_H

#include <AzCore/std/parallel/internal/time_linux.h>

/**
 * This file is to be included from the mutex.h only. It should NOT be included by the user.
 */

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // Condition variable
    inline condition_variable::condition_variable()
    {
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable::condition_variable(const char* name)
    {
        (void)name;
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable::~condition_variable()
    {
        pthread_cond_destroy(&m_cond_var);
    }

    inline void condition_variable::notify_one()
    {
        pthread_cond_signal(&m_cond_var);
    }
    inline void condition_variable::notify_all()
    {
        pthread_cond_broadcast(&m_cond_var);
    }
    inline void condition_variable::wait(unique_lock<mutex>& lock)
    {
        pthread_cond_wait(&m_cond_var, lock.mutex()->native_handle());
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
        const auto now = chrono::system_clock::now();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now);
        }

        return false;
    }
    template <class Clock, class Duration, class Predicate>
    inline bool condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {
        const auto now = chrono::system_clock::now();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now, pred);
        }

        return false;
    }
    template <class Rep, class Period>
    inline bool condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time)
    {
        timespec ts = Internal::CurrentTimeAndOffset(rel_time);

        int ret = pthread_cond_timedwait(&m_cond_var, lock.mutex()->native_handle(), &ts);
        return (ret == ETIMEDOUT);
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
        return &m_cond_var;
    }

    //////////////////////////////////////////////////////////////////////////
    // Condition variable any
    inline condition_variable_any::condition_variable_any()
    {
        pthread_mutex_init(&m_mutex, NULL);
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable_any::condition_variable_any(const char* name)
    {
        pthread_mutex_init(&m_mutex, NULL);
        (void)name;
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable_any::~condition_variable_any()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond_var);
    }

    inline void condition_variable_any::notify_one()
    {
        pthread_mutex_lock(&m_mutex);
        pthread_cond_signal(&m_cond_var);
        pthread_mutex_unlock(&m_mutex);
    }
    inline void condition_variable_any::notify_all()
    {
        pthread_mutex_lock(&m_mutex);
        pthread_cond_broadcast(&m_cond_var);
        pthread_mutex_unlock(&m_mutex);
    }

    template<class Lock>
    inline void condition_variable_any::wait(Lock& lock)
    {
        pthread_mutex_lock(&m_mutex);
        lock.unlock();
        // We need to make sure we use CriticalSection based mutex.
        pthread_cond_wait(&m_cond_var, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
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
        pthread_mutex_lock(&m_mutex);
        lock.unlock();

        // We need to make sure we use CriticalSection based mutex.
        timespec ts = Internal::CurrentTimeAndOffset(rel_time);

        int ret = pthread_cond_timedwait(&m_cond_var, lock.mutex()->native_handle(), &ts);

        pthread_mutex_unlock(&m_mutex);
        lock.lock();
        return (ret == ETIMEDOUT);
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
        return &m_cond_var;
    }
    //////////////////////////////////////////////////////////////////////////
}



#endif // AZSTD_MUTEX_LINUX_H
#pragma once