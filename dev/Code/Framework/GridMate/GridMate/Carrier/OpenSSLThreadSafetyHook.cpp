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

#include <GridMate/Carrier/OpenSSLThreadSafetyHook.h>

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)

#include <openssl/crypto.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/array.h>


namespace GridMate
{
    class OpenSSLThreadSafetyHookImpl
    {
    public:
        AZ_CLASS_ALLOCATOR(OpenSSLThreadSafetyHookImpl, GridMate::GridMateAllocatorMP, 0);

        OpenSSLThreadSafetyHookImpl()
        {
            void* threadIdCB = reinterpret_cast<void*>(CRYPTO_THREADID_get_callback());
            if (threadIdCB == nullptr)
            {
                CRYPTO_THREADID_set_callback(&SslThreadIdCbImpl);
                AZ_Assert(CRYPTO_THREADID_get_callback() == &SslThreadIdCbImpl, "OpenSSLThreadSafetyHook threadIdCB was not properly installed, threadIdCB=%p (expected %p)!", CRYPTO_THREADID_get_callback(), &SslThreadIdCbImpl);
            }
            else
            {
                AZ_TracePrintf("GridMate", "OpenSSLThreadSafetyHook threadIdCB was not installed because a threadIdCB is already installed (%p)!\n", threadIdCB);
            }
            void* lockingCB = reinterpret_cast<void*>(CRYPTO_get_locking_callback());
            if (lockingCB == nullptr)
            {
                CRYPTO_set_locking_callback(&SslLockCbImpl);
                AZ_Assert(CRYPTO_get_locking_callback() == &SslLockCbImpl, "OpenSSLThreadSafetyHook lockingCB was not properly installed, lockingCB=%p (expected %p)!", CRYPTO_get_locking_callback(), &SslLockCbImpl);
            }
            else
            {
                AZ_TracePrintf("GridMate", "OpenSSLThreadSafetyHook lockingCB was not installed because a lockingCB is already installed (%p)!\n", lockingCB);
            }
        }

        ~OpenSSLThreadSafetyHookImpl()
        {
            if (CRYPTO_get_locking_callback() == &SslLockCbImpl)
            {
                CRYPTO_set_locking_callback(nullptr);
                AZ_Assert(CRYPTO_get_locking_callback() == nullptr, "OpenSSLThreadSafetyHook was not properly uninstalled, lockingCB=%p (expected nullptr)!", CRYPTO_get_locking_callback());
            }
            else
            {
                AZ_TracePrintf("GridMate", "OpenSSLThreadSafetyHook was not uninstalled because lockingCB=%p (expected %p)!\n", CRYPTO_get_locking_callback(), &SslLockCbImpl);
            }
        }

        static void SslLockCbImpl(int mode, int n, const char* file, int line)
        {
            (void)file;
            (void)line;

            if (mode & CRYPTO_LOCK)
            {
                s_singleton->m_mutexes[n].lock();
                //AZ_TracePrintf("", "SslLockCbImpl(%x, %d, %s, %d) from thread id 0x%x.\n", mode, n, file, line, AZStd::this_thread::get_id().m_id);
            }
            else
            {
                //AZ_TracePrintf("", "    SslLockCbImpl(%x, %d, %s, %d) from thread id 0x%x.\n", mode, n, file, line, AZStd::this_thread::get_id().m_id);
                s_singleton->m_mutexes[n].unlock();
            }
        }

        static void SslThreadIdCbImpl(CRYPTO_THREADID* id)
        {
            //AZ_TracePrintf("", "SslThreadIdCbImpl() called from thread id 0x%x.\n", AZStd::this_thread::get_id().m_id);
            CRYPTO_THREADID_set_numeric(id, AZStd::this_thread::get_id().m_id);
        }

        AZStd::array<AZStd::recursive_mutex, CRYPTO_NUM_LOCKS> m_mutexes;

        static OpenSSLThreadSafetyHookImpl* s_singleton;
    };

    OpenSSLThreadSafetyHookImpl* OpenSSLThreadSafetyHookImpl::s_singleton = nullptr;

    AZStd::atomic_int OpenSSLThreadSafetyHook::s_refCount{ 0 };

    OpenSSLThreadSafetyHook::OpenSSLThreadSafetyHook()
    {
        if (s_refCount.fetch_add(1) == 0)
        {
            OpenSSLThreadSafetyHookImpl::s_singleton = aznew OpenSSLThreadSafetyHookImpl;
        }
    }

    OpenSSLThreadSafetyHook::~OpenSSLThreadSafetyHook()
    {
        if (s_refCount.fetch_sub(1) == 1)
        {
            delete OpenSSLThreadSafetyHookImpl::s_singleton;
        }
    }
}   // namespace GridMate

#endif // AZ_PLATFORM_WINDOWS || AZ_PLATFORM_LINUX
