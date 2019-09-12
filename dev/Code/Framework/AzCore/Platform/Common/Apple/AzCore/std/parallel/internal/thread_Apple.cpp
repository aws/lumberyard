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
#ifndef AZ_UNITY_BUILD

#include <AzCore/std/parallel/thread.h>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)

#include <AzCore/std/parallel/threadbus.h>

#include <sched.h>
#include <errno.h>

#if defined(AZ_PLATFORM_APPLE)
    #include <mach/thread_policy.h>
    #include <mach/thread_act.h>
#endif

namespace AZStd
{
    namespace Internal
    {
        void*   thread_run_function(void* param)
        {
            thread_info* ti = reinterpret_cast<thread_info*>(param);
#if defined(AZ_PLATFORM_APPLE)
            const char* name = "AZStd thread";
            if (ti->m_name)
            {
                name = ti->m_name;
            }
            pthread_setname_np(name);
#endif
            ti->execute();
            destroy_thread_info(ti);
            EBUS_EVENT(ThreadEventBus, OnThreadExit, this_thread::get_id());
            pthread_exit(nullptr);
            return nullptr;
        }


        pthread_t create_thread(const thread_desc* desc, thread_info* ti)
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);

            size_t stackSize = 2 * 1024 * 1024;
            int priority = -1;
            const char* name = "AZStd thread";
            if (desc)
            {
                if (desc->m_stackSize != -1)
                {
                    stackSize = desc->m_stackSize;
                }
                if (desc->m_priority >= sched_get_priority_min(SCHED_FIFO) && desc->m_priority <= sched_get_priority_max(SCHED_FIFO))
                {
                    priority = desc->m_priority;
                }
                if (desc->m_name)
                {
                    name = desc->m_name;
                }

                pthread_attr_setdetachstate(&attr, desc->m_isJoinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);

                if (desc->m_cpuId >= 0) // TODO: add a proper mask to support more than one core that can run the thread!!!
                {
#if !defined(AZ_PLATFORM_APPLE) && !defined(AZ_PLATFORM_ANDROID)
                    cpu_set_t cpuset;
                    CPU_ZERO(&cpuset);
                    CPU_SET(desc->m_cpuId, &cpuset);

                    int result = pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
                    (void)result;
                    AZ_Warning("System", result == 0, "pthread_setaffinity_np failed %s\n", strerror(errno));
#endif
                }
            }

            pthread_attr_setstacksize(&attr, stackSize);

#if defined(AZ_PLATFORM_ANDROID)
            AZ_Warning("System", priority <= 0, "Thread priorities %d not supported on Android!\n", priority);
            (void)priority;
#else
            if (priority == -1)
            {
                pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
            }
            else
            {
                struct sched_param    schedParam;
                memset(&schedParam, 0, sizeof(schedParam));
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
                schedParam.sched_priority = priority;
                pthread_attr_setschedparam(&attr, &schedParam);
            }
#endif // !AZ_PLATFORM_ANDROID (not supported at v r10d)

            pthread_t tId;
            int res = pthread_create(&tId, &attr, &thread_run_function, ti);
            (void)res;
            AZ_Assert(res == 0, "pthread failed %s", strerror(errno));
            pthread_attr_destroy(&attr);
#if defined(AZ_PLATFORM_APPLE)
            if (desc && desc->m_cpuId >= 0) // TODO: add a proper mask to support more than one core that can run the thread!!!
            {
                mach_port_t mach_thread = pthread_mach_thread_np(tId);
                thread_affinity_policy_data_t policyData = { desc->m_cpuId };
                thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policyData, 1);
            }
#else
            pthread_setname_np(tId, name);
#endif
            EBUS_EVENT(ThreadEventBus, OnThreadEnter, thread::id(tId), desc);
            return tId;
        }
    }


    thread::thread()
    {
        m_thread = native_thread_invalid_id;
    }

    /*thread::thread(AZStd::delegate<void ()> d,const thread_desc* desc)
    {
        Internal::thread_info* ti = Internal::create_thread_info(d);
        m_thread.m_handle = Internal::create_thread(desc,ti,&m_thread.m_id);
    }*/

    thread::thread(Internal::thread_move_t<thread> rhs)
    {
        m_thread = rhs->m_thread;
        rhs->m_thread = native_thread_invalid_id;
    }

    thread::~thread()
    {
        AZ_Assert(!joinable(), "You must call detach or join before you delete a thread!");
    }

    void thread::join()
    {
        AZ_Assert(joinable(), "Thread must be joinable!");
        if (!pthread_equal(m_thread, native_thread_invalid_id))
        {
            pthread_join(m_thread, nullptr);
            m_thread = native_thread_invalid_id;
        }
    }
    void thread::detach()
    {
        AZ_Assert(joinable(), "Thread must be joinable!");
        if (!pthread_equal(m_thread, native_thread_invalid_id))
        {
            pthread_detach(m_thread);
            m_thread = native_thread_invalid_id;
        }
    }

    unsigned thread::hardware_concurrency()
    {
        return static_cast<unsigned int>(sysconf(_SC_NPROCESSORS_ONLN));
    }
    //////////////////////////////////////////////////////////////////////////
}

#endif // #if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)

#endif // #ifndef AZ_UNITY_BUILD
