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

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/parallel/thread.h>

#include <AzCore/std/parallel/threadbus.h>

namespace AZStd
{
    namespace Platform
    {
        void PostThreadRun();
        HANDLE CreateThread(unsigned stackSize, unsigned (__stdcall* threadRunFunction)(void*), AZStd::Internal::thread_info* ti, unsigned int* id);
        unsigned HardwareConcurrency();
    }

    // Since we avoid including windows.h in header files in the code, we are declaring storage for CRITICAL_SECTION and CONDITION_VARIABLE
    // Make sure at compile that that the storage is enough to store the variables
    AZ_STATIC_ASSERT(sizeof(native_mutex_data_type) >= sizeof(CRITICAL_SECTION), "native_mutex_data_type is used to store CRITICAL_SECTION, it should be big enough!");
    AZ_STATIC_ASSERT(sizeof(native_cond_var_data_type) >= sizeof(CONDITION_VARIABLE), "native_mutex_data_type is used to store CONDITION_VARIABLE, it should be big enough!");

    namespace Internal
    {
        /**
         * Thread run function
         */
        unsigned __stdcall thread_run_function(void* param)
        {
            thread_info* ti = reinterpret_cast<thread_info*>(param);
            ti->execute();
            destroy_thread_info(ti);

            EBUS_EVENT(ThreadEventBus, OnThreadExit, this_thread::get_id());

            Platform::PostThreadRun();

            return 0;
        }

        /**
         * Create and run thread
         */
        HANDLE create_thread(const thread_desc* desc, thread_info* ti, unsigned int* id)
        {
            unsigned stackSize = 0;
            *id = native_thread_invalid_id;
            HANDLE hThread;
            if (desc && desc->m_stackSize != -1)
            {
                stackSize = desc->m_stackSize;
            }

            hThread = Platform::CreateThread(stackSize, &thread_run_function, ti, id);
            if (hThread == NULL)
            {
                return hThread;
            }

            if (desc && desc->m_priority >= -15 && desc->m_priority <= 15)
            {
                ::SetThreadPriority(hThread, desc->m_priority);
            }

            if (desc && desc->m_cpuId != -1)
            {
                SetThreadAffinityMask(hThread, DWORD_PTR(desc->m_cpuId));
            }

            EBUS_EVENT(ThreadEventBus, OnThreadEnter, thread::id(*id), desc);

            ::ResumeThread(hThread);

            if (desc && desc->m_name)
            {
                #pragma pack(push,8)
                typedef struct tagTHREADNAME_INFO
                {
                    DWORD dwType;     // Must be 0x1000
                    LPCSTR szName;    // Pointer to name (in user address space)
                    DWORD dwThreadID; // Thread ID (-1 for caller thread)
                    DWORD dwFlags;    // Reserved for future use; must be zero
                } THREADNAME_INFO;
                #pragma pack(pop)

                THREADNAME_INFO info;

                info.dwType = 0x1000;
                info.szName = desc->m_name;
                info.dwThreadID = *id;
                info.dwFlags = 0;

                __try
                {
                    RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (const ULONG_PTR*)&info);
                }
                __except (GetExceptionCode() == 0x406D1388 ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }

            return hThread;
        }
    }


    thread::thread()
    {
        m_thread.m_id = native_thread_invalid_id;
    }

    thread::thread(Internal::thread_move_t<thread> rhs)
    {
        m_thread.m_handle = rhs->m_thread.m_handle;
        m_thread.m_id = rhs->m_thread.m_id;
        rhs->m_thread.m_id = native_thread_invalid_id;
    }

    thread::~thread()
    {
        AZ_Assert(!joinable(), "You must call detach or join before you delete a thread!");
    }

    void thread::join()
    {
        AZ_Assert(joinable(), "Thread must be joinable!");
        if (m_thread.m_id != native_thread_invalid_id)
        {
            WaitForSingleObject(m_thread.m_handle, INFINITE);
            detach();
        }
    }
    void thread::detach()
    {
        AZ_Assert(joinable(), "Thread must be joinable!");
        if (m_thread.m_id != native_thread_invalid_id)
        {
            CloseHandle(m_thread.m_handle);
            m_thread.m_id = native_thread_invalid_id;
        }
    }

    /// Return number of physical processors
    unsigned thread::hardware_concurrency()
    {
        return Platform::HardwareConcurrency();
    }
    //////////////////////////////////////////////////////////////////////////
}
