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

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>

#if defined(AZ_COMPILER_MSVC)
    #pragma warning( push )
    #pragma warning( disable : 4127 ) // "conditional expression is constant"
#endif

namespace AZStd
{
    namespace internal
    {
        //
        //storage class
        //
        template<typename T, size_t size>
        struct atomic_base_storage
        {
            atomic_base_storage() { }
            atomic_base_storage(T v)
                : m_value(v) { }
        protected:
            T m_value;
        };


        //
        //lock manager
        //
        struct atomic_lock_manager
        {
            static mutex& get_lock_for_address(volatile void* addr)
            {
                //this is just the example implementation from n2427, it will be good enough

                const int LOGSIZE = 4;

                static mutex mutexTable[1 << LOGSIZE];

                AZStd::size_t u = (AZStd::size_t)addr;
                u += (u >> 2) + (u << 4);
                u += (u >> 7) + (u << 5);
                u += (u >> 17) + (u << 13);
                if (sizeof(AZStd::size_t) > 4)
                {
                    u += (u >> 31);
                }
                u &= ~((~(AZStd::size_t)0) << LOGSIZE);
                return mutexTable[u];
            }
        };

        //
        //storage converter, converts T to atomic type, this is necessary even in the locking implementation as T
        // is not required to have an assignment operator, equality operator, etc.
        //
        template<typename T, size_t size>
        struct atomic_storage_converter;

        template<typename T>
        struct atomic_storage_converter<T, 1>
        {
            typedef char AtomicType;

            AZ_FORCE_INLINE static char toAtomicType(T v)   { return *reinterpret_cast<char*>(&v); }
            AZ_FORCE_INLINE static T fromAtomicType(char v) { return *reinterpret_cast<T*>(&v); }
        };

        template<typename T>
        struct atomic_storage_converter<T, 2>
        {
            typedef short AtomicType;

            AZ_FORCE_INLINE static short toAtomicType(T v)   { return *reinterpret_cast<short*>(&v); }
            AZ_FORCE_INLINE static T fromAtomicType(short v) { return *reinterpret_cast<T*>(&v); }
        };

        template<typename T>
        struct atomic_storage_converter<T, 4>
        {
            typedef int AtomicType;

            AZ_FORCE_INLINE static int toAtomicType(T v)   { return *reinterpret_cast<int*>(&v); }
            AZ_FORCE_INLINE static T fromAtomicType(int v) { return *reinterpret_cast<T*>(&v); }
        };

        template<typename T>
        struct atomic_storage_converter<T, 8>
        {
            typedef AZ::s64 AtomicType;

            AZ_FORCE_INLINE static AtomicType toAtomicType(T v)   { return *reinterpret_cast<AtomicType*>(&v); }
            AZ_FORCE_INLINE static T fromAtomicType(AtomicType v) { return *reinterpret_cast<T*>(&v); }
        };

        template<typename T>
        struct atomic_storage_converter<T, 16>
        {
            typedef AZ::s64 AtomicType;

            AZ_FORCE_INLINE static AtomicType toAtomicType(T v)   { return *reinterpret_cast<AtomicType*>(&v); }
            AZ_FORCE_INLINE static T fromAtomicType(AtomicType v) { return *reinterpret_cast<T*>(&v); }
        };

        //
        //load operations
        //
        template<typename T, size_t size, memory_order order>
        struct atomic_load_helper
        {
            typedef atomic_storage_converter<T, size> convert;
            typedef typename convert::AtomicType AtomicType;

            static T load(volatile T* addr)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                return convert::fromAtomicType(*reinterpret_cast<volatile AtomicType*>(addr));
            }
        };


        //
        //store operations
        //
        template<typename T, size_t size, memory_order order>
        struct atomic_store_helper
        {
            typedef atomic_storage_converter<T, size> convert;
            typedef typename convert::AtomicType AtomicType;

            static void store(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                *reinterpret_cast<volatile AtomicType*>(addr) = convert::toAtomicType(v);
            }
        };


        //
        //other atomic operations
        //
        template<typename T, size_t size, memory_order order>
        struct atomic_operation_helper
        {
            typedef atomic_storage_converter<T, size> convert;
            typedef typename convert::AtomicType AtomicType;

            static T exchange(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType temp = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = convert::toAtomicType(v);
                return convert::fromAtomicType(temp);
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired)
            {
                return compare_exchange_strong(addr, expected, desired);
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired);
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType temp = *reinterpret_cast<volatile AtomicType*>(addr);
                if (temp == convert::toAtomicType(expected))
                {
                    *reinterpret_cast<volatile AtomicType*>(addr) = convert::toAtomicType(desired);
                    return true;
                }
                else
                {
                    *reinterpret_cast<AtomicType*>(&expected) = temp;
                    return false;
                }
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired);
            }
            static T fetch_add(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old + convert::toAtomicType(v);
                return convert::fromAtomicType(old);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old - convert::toAtomicType(v);
                return convert::fromAtomicType(old);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old & convert::toAtomicType(v);
                return convert::fromAtomicType(old);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old | convert::toAtomicType(v);
                return convert::fromAtomicType(old);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old ^ convert::toAtomicType(v);
                return convert::fromAtomicType(old);
            }
            static T fetch_inc(volatile T* addr)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old + 1;
                return convert::fromAtomicType(old);
            }
            static T fetch_dec(volatile T* addr)
            {
                lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
                AtomicType old = *reinterpret_cast<volatile AtomicType*>(addr);
                *reinterpret_cast<volatile AtomicType*>(addr) = old - 1;
                return convert::fromAtomicType(old);
            }
        };

        //specialization for pointer types
        //template<typename T, size_t size, memory_order order>
        //struct atomic_operation_helper<T*, size, order>
        //{
        //  static T* exchange(T* volatile* addr, T* v)
        //  {
        //      lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
        //      T* temp = *addr;
        //      *addr = v;
        //      return temp;
        //  }
        //  static bool compare_exchange_weak(T* volatile* addr, T*& expected, T* desired)
        //  {
        //      return compare_exchange_strong(addr, expected, desired);
        //  }
        //  static bool compare_exchange_weak(T* volatile* addr, T*& expected, T* desired, memory_order failure_order)
        //  {
        //      return compare_exchange_strong(addr, expected, desired);
        //  }
        //  static bool compare_exchange_strong(T* volatile* addr, T*& expected, T* desired)
        //  {
        //      lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
        //      if( *addr == expected )
        //      {
        //          *addr = desired;
        //          return true;
        //      }
        //      else
        //      {
        //          expected = *addr;
        //          return false;
        //      }
        //  }
        //  static bool compare_exchange_strong(T* volatile* addr, T*& expected, T* desired, memory_order failure_order)
        //  {
        //      return compare_exchange_strong(addr, expected, desired);
        //  }
        //  static T* fetch_add(T* volatile* addr, T* v)
        //  {
        //      lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
        //      T* old = *addr;
        //      *addr = (char*)old + (AZStd::size_t)v;
        //      return old;
        //  }
        //  static T* fetch_sub(T* volatile* addr, T* v)
        //  {
        //      lock_guard<mutex> lock(atomic_lock_manager::get_lock_for_address(addr));
        //      T* old = *addr;
        //      *addr = (char*)old - (AZStd::size_t)v;
        //      return old;
        //  }
        //};
    }
}

#if defined(AZ_COMPILER_MSVC)
    #pragma warning( pop )
#endif
