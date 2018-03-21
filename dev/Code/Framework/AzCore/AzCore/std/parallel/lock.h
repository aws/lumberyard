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
#ifndef AZSTD_LOCK_H
#define AZSTD_LOCK_H 1

#include <AzCore/std/parallel/config.h>
#include <AzCore/std/createdestroy.h>
//#include <AzCore/std/algorithm.h> // for swap

namespace AZStd
{
    template<class Mutex>
    class upgrade_lock;

    struct defer_lock_t { };    // do not acquire ownership of the mutex
    struct try_to_lock_t { };   // try to acquire ownership of the mutex without blocking
    struct adopt_lock_t { };    // assume the calling thread has already

    template <class Mutex>
    class lock_guard
    {
    public:
        typedef Mutex mutex_type;
        AZ_FORCE_INLINE explicit lock_guard(mutex_type& mtx)
            : m_mutex(mtx)         { m_mutex.lock(); }
        AZ_FORCE_INLINE lock_guard(mutex_type& mtx, adopt_lock_t)
            : m_mutex(mtx)    {}
        AZ_FORCE_INLINE ~lock_guard()
        {
            m_mutex.unlock();
        }
    private:
        AZ_FORCE_INLINE lock_guard(lock_guard const&) {}
        AZ_FORCE_INLINE lock_guard& operator=(lock_guard const&) {}

        mutex_type& m_mutex; // exposition only
    };

    template <class Mutex>
    class unique_lock
    {
        template<class M>
        friend class upgrade_lock;
    public:
        typedef Mutex mutex_type;
        // 30.4.3.2.1 construct/copy/destroy
        AZ_FORCE_INLINE unique_lock()
            : m_mutex(nullptr)
            , m_owns(false) {}
        AZ_FORCE_INLINE explicit unique_lock(mutex_type& mtx)
            : m_mutex(&mtx)
            , m_owns(false)
        {
            m_mutex->lock();
            m_owns = true;
        }
        AZ_FORCE_INLINE unique_lock(mutex_type& mtx, defer_lock_t)
            : m_mutex(&mtx)
            , m_owns(false) {}
        AZ_FORCE_INLINE unique_lock(mutex_type& mtx, try_to_lock_t)
            : m_mutex(&mtx) { m_owns = m_mutex->try_lock(); }
        AZ_FORCE_INLINE unique_lock(mutex_type& mtx, adopt_lock_t)
            : m_mutex(&mtx)
            , m_owns(true) {}
        //template <class Clock, class Duration>
        //AZ_FORCE_INLINE unique_lock(mutex_type& mtx, const chrono::time_point<Clock, Duration>& abs_time)
        //{}
        //template <class Rep, class Period>
        //AZ_FORCE_INLINE unique_lock(mutex_type& mtx, const chrono::duration<Rep, Period>& rel_time)
        //{}

        AZ_FORCE_INLINE unique_lock(unique_lock&& u)
            : m_mutex(u.m_mutex)
            , m_owns(u.m_owns)
        {
            u.m_mutex = nullptr;
            u.m_owns = false;
        }

        unique_lock(upgrade_lock<Mutex>&& other);

        AZ_FORCE_INLINE unique_lock& operator=(unique_lock&& u)
        {
            if (m_owns)
            {
                m_mutex->unlock();
            }

            m_mutex = u.m_mutex;
            m_owns = u.m_owns;
            u.m_mutex = nullptr;
            u.m_owns = false;
        }

        AZ_FORCE_INLINE ~unique_lock()
        {
            if (m_owns)
            {
                m_mutex->unlock();
            }
        }

        // 30.4.3.2.2 locking
        AZ_FORCE_INLINE void lock()
        {
            AZ_Assert(m_mutex != nullptr, "Mutex not set!");
            m_mutex->lock();
            m_owns = true;
        }
        AZ_FORCE_INLINE bool try_lock()
        {
            AZ_Assert(m_mutex != nullptr, "Mutex not set!");
            m_owns = m_mutex->try_lock();
            return m_owns;
        }
        /*template <class Rep, class Period>
        AZ_FORCE_INLINE bool try_lock_for(const chrono::duration<Rep, Period>& rel_time);
        template <class Clock, class Duration>
        AZ_FORCE_INLINE bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time);*/
        AZ_FORCE_INLINE void unlock()
        {
            AZ_Assert(m_mutex != nullptr, "Mutex not set!");
            AZ_Assert(m_owns, "We must own the mutex to unlock!");
            m_mutex->unlock();
            m_owns = false;
        }
        // 30.4.3.2.3 modifiers
        AZ_FORCE_INLINE void swap(unique_lock& rhs)
        {
            (void)rhs;
            // We need to solve that swap include
            AZ_Assert(false, "Todo");
            //AZStd::swap(m_mutex,rhs.m_mutex);
            //AZStd::swap(m_owns,rhs.m_owns);
        }
        AZ_FORCE_INLINE mutex_type* release()
        {
            mutex_type* mutex = m_mutex;
            //if( m_owns ) m_mutex->unlock();
            m_mutex = nullptr;
            m_owns = false;
            return mutex;
        }
        // 30.4.3.2.4 observers
        AZ_FORCE_INLINE bool owns_lock() const { return m_owns; }
        AZ_FORCE_INLINE operator bool () const { return m_owns; }
        AZ_FORCE_INLINE mutex_type* mutex() const { return m_mutex; }

    private:
        unique_lock(unique_lock const&);
        unique_lock& operator=(unique_lock const&);

        mutex_type* m_mutex; // exposition only
        bool        m_owns; // exposition only
    };

    template <class Mutex>
    void swap(unique_lock<Mutex>& x, unique_lock<Mutex>& y)
    {
        x.swap(y);
    }

    template <class Mutex>
    class shared_lock
    {
    public:
        typedef Mutex mutex_type;
        AZ_FORCE_INLINE explicit shared_lock(mutex_type& mutex)
            : m_mutex(mutex) { m_mutex.lock_shared(); }
        AZ_FORCE_INLINE shared_lock(mutex_type& mutex, adopt_lock_t)
            : m_mutex(mutex) {}
        AZ_FORCE_INLINE ~shared_lock() { m_mutex.unlock_shared(); }

    private:
        AZ_FORCE_INLINE shared_lock(shared_lock const&);
        AZ_FORCE_INLINE shared_lock& operator=(shared_lock const&);
        mutex_type& m_mutex; // exposition only
    };
}

#endif // AZSTD_LOCK_H
#pragma once
