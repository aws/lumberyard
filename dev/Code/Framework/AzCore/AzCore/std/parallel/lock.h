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

    template <typename Mutex>
    class upgrade_lock
    {
    protected:
        Mutex* m_mutex;
        bool m_owns;

    public:
        typedef Mutex mutex_type;

        upgrade_lock()
            : m_mutex(nullptr)
            , m_owns(false)
        {}

        explicit upgrade_lock(Mutex& mtx)
            : m_mutex(&mtx)
            , m_owns(false)
        {
            lock();
        }

        upgrade_lock(Mutex& mtx, adopt_lock_t)
            : m_mutex(&mtx)
            , m_owns(true)
        {
        }

        upgrade_lock(Mutex& mtx, defer_lock_t)
            : m_mutex(&mtx)
            , m_owns(false)
        {}

        upgrade_lock(Mutex& mtx, try_to_lock_t)
            : m_mutex(&mtx)
            , m_owns(false)
        {
            try_lock();
        }

        //template <class Clock, class Duration>
        //upgrade_lock(Mutex& mtx, const chrono::time_point<Clock, Duration>& t)
        //    : m_mutex(&mtx), m_owns(mtx.try_lock_upgrade_until(t))
        //{
        //}
        //template <class Rep, class Period>
        //upgrade_lock(Mutex& mtx, const chrono::duration<Rep, Period>& d)
        //    : m_mutex(&mtx), m_owns(mtx.try_lock_upgrade_for(d))
        //{
        //}

        upgrade_lock(upgrade_lock<Mutex>&& other)
            : m_mutex(other.m_mutex)
            , m_owns(other.m_owns)
        {
            other.m_owns = false;
            other.m_mutex = nullptr;
        }

        upgrade_lock(unique_lock<Mutex>&& other)
            : m_mutex(other.m_mutex)
            , m_owns(other.m_owns)
        {
            if (m_owns)
            {
                m_mutex->unlock_and_lock_upgrade();
            }
            other.m_owns = false;
            other.m_mutex = nullptr;
        }

        //std-2104 unique_lock move-assignment should not be noexcept
        upgrade_lock& operator=(upgrade_lock<Mutex>&& other)
        {
            upgrade_lock temp(AZStd::move(other));
            swap(temp);
            return *this;
        }

        upgrade_lock& operator=(unique_lock<Mutex>&& other)
        {
            upgrade_lock temp(AZStd::move(other));
            swap(temp);
            return *this;
        }

        // Conversion from shared locking
        upgrade_lock(shared_lock<mutex_type>&& sl, try_to_lock_t)
            : m_mutex(0)
            , m_owns(false)
        {
            if (sl.owns_lock())
            {
                if (sl.mutex()->try_unlock_shared_and_lock_upgrade())
                {
                    m_mutex = sl.release();
                    m_owns = true;
                }
            }
            else
            {
                m_mutex = sl.release();
            }
        }

        /* template <class Clock, class Duration>
         upgrade_lock(shared_lock<mutex_type>&&  sl, const chrono::time_point<Clock, Duration>& abs_time)
             : m_mutex(0)
             , m_owns(false)
         {
             if (sl.owns_lock())
             {
                 if (sl.mutex()->try_unlock_shared_and_lock_upgrade_until(abs_time))
                 {
                     m_mutex = sl.release();
                     m_owns = true;
                 }
             }
             else
             {
                 m_mutex = sl.release();
             }
         }

         template <class Rep, class Period>
         upgrade_lock(shared_lock<mutex_type>&& sl, const chrono::duration<Rep, Period>& rel_time)
             : m_mutex(0)
             , m_owns(false)
         {
             if (sl.owns_lock())
             {
                 if (sl.mutex()->try_unlock_shared_and_lock_upgrade_for(rel_time))
                 {
                     m_mutex = sl.release();
                     m_owns = true;
                 }
             }
             else
             {
                 m_mutex = sl.release();
             }
         }*/

        void swap(upgrade_lock& other)
        {
            AZStd::swap(m_mutex, other.m_mutex);
            AZStd::swap(m_owns, other.m_owns);
        }

        Mutex* mutex() const
        {
            return m_mutex;
        }

        Mutex* release()
        {
            Mutex* const res = m_mutex;
            m_mutex = 0;
            m_owns = false;
            return res;
        }

        ~upgrade_lock()
        {
            if (owns_lock())
            {
                m_mutex->unlock_upgrade();
            }
        }

        void lock()
        {
            AZ_Assert(m_mutex != nullptr, "You need mutex to lock it!");
            AZ_Assert(!owns_lock(), "upgrade_lock owns already the mutex");
            if (m_mutex)
            {
                m_mutex->lock_upgrade();
                m_owns = true;
            }
        }

        bool try_lock()
        {
            AZ_Assert(m_mutex != nullptr, "You need mutex to lock it!");
            AZ_Assert(!owns_lock(), "upgrade_lock owns already the mutex");
            if (m_mutex)
            {
                m_owns = m_mutex->try_lock_upgrade();
            }
            return m_owns;
        }

        void unlock()
        {
            AZ_Assert(m_mutex != nullptr, "You need mutex to lock it!");
            AZ_Assert(owns_lock(), "upgrade_lock doesn't own the mutex");
            if (m_mutex)
            {
                m_mutex->unlock_upgrade();
                m_owns = false;
            }
        }

        /* template <class Rep, class Period>
         bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
         {
             AZ_Assert(m_mutex != nullptr, "You need mutex to lock it!");
             AZ_Assert(!owns_lock(), "upgrade_lock owns already the mutex");
             if (m_mutex)
             {
                 m_owns = m_mutex->try_lock_upgrade_for(rel_time);
             }
             return m_owns;
         }
         template <class Clock, class Duration>
         bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
         {
             AZ_Assert(m_mutex != nullptr, "You need mutex to lock it!");
             AZ_Assert(!owns_lock(), "upgrade_lock owns already the mutex");
             if (m_mutex)
             {
                 m_owns = m_mutex->try_lock_upgrade_until(abs_time);
             }
             return m_owns;
         }*/

        explicit operator bool() const
        {
            return owns_lock();
        }

        bool owns_lock() const
        {
            return m_owns;
        }

        friend class shared_lock<Mutex>;
        friend class unique_lock<Mutex>;
    };

    template<typename Mutex>
    void swap(upgrade_lock<Mutex>& lhs, upgrade_lock<Mutex>& rhs)
    {
        lhs.swap(rhs);
    }

    template<typename Mutex>
    unique_lock<Mutex>::unique_lock(upgrade_lock<Mutex>&& other)
        : m_mutex(other.m_mutex)
        , m_owns(other.m_owns)
    {
        if (m_owns)
        {
            m_mutex->unlock_upgrade_and_lock();
        }
        other.release();
    }

    template <class Mutex>
    class upgrade_to_unique_lock
    {
    private:
        upgrade_lock<Mutex>* source;
        unique_lock<Mutex> exclusive;

    public:
        typedef Mutex mutex_type;

        explicit upgrade_to_unique_lock(upgrade_lock<Mutex>& mtx)
            : source(&mtx)
            , exclusive(AZStd::move(*source))
        {
        }

        ~upgrade_to_unique_lock()
        {
            if (source)
            {
                *source = AZStd::move(upgrade_lock<Mutex>(AZStd::move(exclusive)));
            }
        }

        upgrade_to_unique_lock(upgrade_to_unique_lock<Mutex>&& other)
            : source(other.source)
            , exclusive(AZStd::move(other.exclusive))
        {
            other.source = 0;
        }

        upgrade_to_unique_lock& operator=(upgrade_to_unique_lock<Mutex>&& other)
        {
            upgrade_to_unique_lock temp(other);
            swap(temp);
            return *this;
        }

        void swap(upgrade_to_unique_lock& other)
        {
            AZStd::swap(source, other.source);
            exclusive.swap(other.exclusive);
        }

        explicit operator bool() const
        {
            return owns_lock();
        }

        bool owns_lock() const
        {
            return exclusive.owns_lock();
        }
        Mutex* mutex() const
        {
            return exclusive.mutex();
        }
    };
}

#endif // AZSTD_LOCK_H
#pragma once