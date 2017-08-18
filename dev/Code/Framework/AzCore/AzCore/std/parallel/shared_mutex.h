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

#include <AzCore/std/parallel/conditional_variable.h>

namespace AZStd
{
    /**
     * shared_mutex - read/writer lock, base on the boost::upgrade_mutex, so you can have upgrade lock too.
     * You can lock a shared_mutex for writer/exclusive with lock, and have multiple readers/shared with lock_shared.
     * As for upgrades, you can have only one reader (in upgraded state) at a time. He can be upgraded to writer/exclusive
     * access at any time. Attempts to get multiple lock_upgrade will result in a block/wait on the second lock_upgrade, same as exclusive lock.
     *
     * \note look at using the native RWLocks on different systems
     * \note when using shared_mutex it's worth checking that using shared_mutex is actually of benefit
     * in many cases it doesn't provide the hoped-for performance gains due to contention on the mutex itself.
     */
    class shared_mutex
    {
        mutex m_mutex;
        condition_variable m_gate1;
        condition_variable m_gate2;
        unsigned m_state;

        static const unsigned WriteEntered = 1U << (sizeof(unsigned) * CHAR_BIT - 1);
        static const unsigned UpgradeableEntered = WriteEntered >> 1;
        static const unsigned NumReadersMask = ~(WriteEntered | UpgradeableEntered);

    public:

        shared_mutex()
            : m_state(0) {}
        ~shared_mutex();

        shared_mutex(const shared_mutex&) = delete;
        shared_mutex& operator=(const shared_mutex&) = delete;

        // Exclusive ownership

        void lock();
        bool try_lock();
        void unlock();

        // Shared ownership
        void lock_shared();
        bool try_lock_shared();
        void unlock_shared();

        // Upgrade ownership
        void lock_upgrade();
        bool try_lock_upgrade();
        void unlock_upgrade();

        // Shared <-> Exclusive
        bool try_unlock_shared_and_lock();
        void unlock_and_lock_shared();

        // Shared <-> Upgrade
        bool try_unlock_shared_and_lock_upgrade();
        void unlock_upgrade_and_lock_shared();

        // Upgrade <-> Exclusive
        void unlock_upgrade_and_lock();
        bool try_unlock_upgrade_and_lock();
        void unlock_and_lock_upgrade();
    };

    ///////////////////////////////////////////////////////////////////
    // Implementation
    ///////////////////////////////////////////////////////////////////

    AZ_INLINE shared_mutex::~shared_mutex()
    {
        lock_guard<mutex> lock(m_mutex);
    }

    AZ_INLINE void shared_mutex::lock()
    {
        unique_lock<mutex> lock(m_mutex);
        while (m_state & (WriteEntered | UpgradeableEntered))
        {
            m_gate1.wait(lock);
        }
        m_state |= WriteEntered;
        while (m_state & NumReadersMask)
        {
            m_gate2.wait(lock);
        }
    }

    AZ_INLINE bool shared_mutex::try_lock()
    {
        unique_lock<mutex> lock(m_mutex);
        if (m_state == 0)
        {
            m_state = WriteEntered;
            return true;
        }
        return false;
    }

    AZ_INLINE void shared_mutex::unlock()
    {
        //{  // \note currenly our implementation requires the mutex to lock for notify, enable this once the condition variable is compliant
        lock_guard<mutex> lock(m_mutex);
        m_state = 0;
        //}
        m_gate1.notify_all();
    }

    AZ_INLINE void shared_mutex::lock_shared()
    {
        unique_lock<mutex> lock(m_mutex);
        while ((m_state & WriteEntered) || (m_state & NumReadersMask) == NumReadersMask)
        {
            m_gate1.wait(lock);
        }
        unsigned num_readers = (m_state & NumReadersMask) + 1;
        m_state &= ~NumReadersMask;
        m_state |= num_readers;
    }

    AZ_INLINE bool shared_mutex::try_lock_shared()
    {
        unique_lock<mutex> lock(m_mutex);
        unsigned num_readers = m_state & NumReadersMask;
        if (!(m_state & WriteEntered) && num_readers != NumReadersMask)
        {
            ++num_readers;
            m_state &= ~NumReadersMask;
            m_state |= num_readers;
            return true;
        }
        return false;
    }

    AZ_INLINE void shared_mutex::unlock_shared()
    {
        lock_guard<mutex> lock(m_mutex);
        unsigned num_readers = (m_state & NumReadersMask) - 1;
        m_state &= ~NumReadersMask;
        m_state |= num_readers;
        if (m_state & WriteEntered)
        {
            if (num_readers == 0)
            {
                m_gate2.notify_one();
            }
        }
        else
        {
            if (num_readers == NumReadersMask - 1)
            {
                m_gate1.notify_one();
            }
        }
    }

    AZ_INLINE void shared_mutex::lock_upgrade()
    {
        unique_lock<mutex> lock(m_mutex);
        while ((m_state & (WriteEntered | UpgradeableEntered)) || (m_state & NumReadersMask) == NumReadersMask)
        {
            m_gate1.wait(lock);
        }
        unsigned num_readers = (m_state & NumReadersMask) + 1;
        m_state &= ~NumReadersMask;
        m_state |= UpgradeableEntered | num_readers;
    }

    AZ_INLINE bool shared_mutex::try_lock_upgrade()
    {
        unique_lock<mutex> lock(m_mutex);
        unsigned num_readers = m_state & NumReadersMask;
        if (!(m_state & (WriteEntered | UpgradeableEntered))
            && num_readers != NumReadersMask)
        {
            ++num_readers;
            m_state &= ~NumReadersMask;
            m_state |= UpgradeableEntered | num_readers;
            return true;
        }
        return false;
    }

    AZ_INLINE void shared_mutex::unlock_upgrade()
    {
        //{ // \note currenly our implementation requires the mutex to lock for notify, enable this once the condition variable is compliant
        lock_guard<mutex> lock(m_mutex);
        unsigned num_readers = (m_state & NumReadersMask) - 1;
        m_state &= ~(UpgradeableEntered | NumReadersMask);
        m_state |= num_readers;
        //}
        m_gate1.notify_all();
    }

    // Shared <-> Exclusive

    AZ_INLINE bool shared_mutex::try_unlock_shared_and_lock()
    {
        unique_lock<mutex> lock(m_mutex);
        if (m_state == 1)
        {
            m_state = WriteEntered;
            return true;
        }
        return false;
    }

    AZ_INLINE void shared_mutex::unlock_and_lock_shared()
    {
        //{ // \note currenly our implementation requires the mutex to lock for notify, enable this once the condition variable is compliant
        lock_guard<mutex> lock(m_mutex);
        m_state = 1;
        //}
        m_gate1.notify_all();
    }

    // Shared <-> Upgrade

    AZ_INLINE bool shared_mutex::try_unlock_shared_and_lock_upgrade()
    {
        unique_lock<mutex> lock(m_mutex);
        if (!(m_state & (WriteEntered | UpgradeableEntered)))
        {
            m_state |= UpgradeableEntered;
            return true;
        }
        return false;
    }

    AZ_INLINE void shared_mutex::unlock_upgrade_and_lock_shared()
    {
        //{ // \note currenly our implementation requires the mutex to lock for notify, enable this once the condition variable is compliant
        lock_guard<mutex> lock(m_mutex);
        m_state &= ~UpgradeableEntered;
        //}
        m_gate1.notify_all();
    }


    // Upgrade <-> Exclusive
    AZ_INLINE void shared_mutex::unlock_upgrade_and_lock()
    {
        unique_lock<mutex> lock(m_mutex);
        unsigned num_readers = (m_state & NumReadersMask) - 1;
        m_state &= ~(UpgradeableEntered | NumReadersMask);
        m_state |= WriteEntered | num_readers;
        while (m_state & NumReadersMask)
        {
            m_gate2.wait(lock);
        }
    }

    AZ_INLINE bool shared_mutex::try_unlock_upgrade_and_lock()
    {
        unique_lock<mutex> lock(m_mutex);
        if (m_state == (UpgradeableEntered | 1))
        {
            m_state = WriteEntered;
            return true;
        }
        return false;
    }

    AZ_INLINE void shared_mutex::unlock_and_lock_upgrade()
    {
        //{ // \note currently our implementation requires the mutex to lock for notify, enable this once the condition variable is compliant
        lock_guard<mutex> lock(m_mutex);
        m_state = UpgradeableEntered | 1;
        //}
        m_gate1.notify_all();
    }
}
