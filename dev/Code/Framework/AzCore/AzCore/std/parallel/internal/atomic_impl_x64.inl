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

#include <intrin.h>
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)

#include <emmintrin.h>
#pragma intrinsic(_InterlockedCompareExchange128)

namespace AZStd
{
    namespace internal
    {
        //
        //storage class, aligns to necessary alignment for atomic operations, as T can be any generic type
        //
        //x64 doesn't have all the necessary intrinsics for 8-bit and 16-bit atomics, and inline assembly isn't
        //supported by the x64 compiler. Easiest option is to just pad all atomics to at least 32-bits.
        //
        //Important to initialize padding to zero, and make sure it stays zero in all operations, as compare_exchange
        //will assume it is zeroed.
        //
        template<typename T>
        struct atomic_base_storage<T, 1>
        {
            atomic_base_storage()
                : m_padding0(0)
                , m_padding1(0)
                , m_padding2(0)  { }
            atomic_base_storage(T v)
                : m_value(v)
                , m_padding0(0)
                , m_padding1(0)
                , m_padding2(0) { }
        protected:
            AZ_ALIGN(T m_value, 4);
            char m_padding0;
            char m_padding1;
            char m_padding2;
        };

        template<typename T>
        struct atomic_base_storage<T, 2>
        {
            atomic_base_storage()
                : m_padding0(0)
                , m_padding1(0) { }
            atomic_base_storage(T v)
                : m_value(v)
                , m_padding0(0)
                , m_padding1(0) { }
        protected:
            AZ_ALIGN(T m_value, 4);
            char m_padding0;
            char m_padding1;
        };

#ifdef AZ_OS64
        template<typename T>
        struct atomic_base_storage<T, 16>
        {
            atomic_base_storage() {}
            atomic_base_storage(T v)
                : m_value(v) {}
        protected:
            AZ_ALIGN(T m_value, 16);
        };
#endif

        //
        //storage converter, converts T to atomic type, padding and zeroing as necessary
        //
        template<typename T>
        struct atomic_storage_converter<T, 1>
        {
            typedef long AtomicType;

            static long toAtomicType(T v)
            {
                return *reinterpret_cast<unsigned char*>(&v); //unsigned because we don't want sign extension
            }
            static T fromAtomicType(long v)
            {
                return *reinterpret_cast<T*>(&v);
            }
            static long mask(long v)      { return (v & 0x000000ff); }
            static long increment(long v) { return mask(v + 1); }
            static long decrement(long v) { return mask(v - 1); }
        };

        template<typename T>
        struct atomic_storage_converter<T, 2>
        {
            typedef long AtomicType;

            static long toAtomicType(T v)
            {
                return *reinterpret_cast<unsigned short*>(&v); //unsigned because we don't want sign extension
            }
            static T fromAtomicType(long v)
            {
                return *reinterpret_cast<T*>(&v);
            }
            static long mask(long v)      { return (v & 0x0000ffff); }
            static long increment(long v) { return mask(v + 1); }
            static long decrement(long v) { return mask(v - 1); }
        };

        template<typename T>
        struct atomic_storage_converter<T, 16>
        {
            typedef __m128i AtomicType;

            static AtomicType toAtomicType(T v)
            {
                return *reinterpret_cast<AtomicType*>(&v);
            }
            static T fromAtomicType(AtomicType v)
            {
                return *reinterpret_cast<T*>(&v);
            }
        };

        //
        //load operations
        //

        //no special handling needed for 64-bit loads on x64, unlike x86
        template<typename T, memory_order order>
        struct atomic_load_helper<T, 8, order>
        {
            static T load(volatile T* addr)
            {
                atomic_barrier_helper<order>::pre_load_barrier();
                __int64 v = *reinterpret_cast<volatile __int64*>(addr);
                atomic_barrier_helper<order>::post_load_barrier();
                return *reinterpret_cast<T*>(&v);
            }
        };

        template<typename T, memory_order order>
        struct atomic_load_helper<T, 16, order>
        {
            static T load(volatile T* addr)
            {
                atomic_barrier_helper<order>::pre_load_barrier();
                //volatile __m128i isn't supported by the compiler... so we're just going to pray and hope
                __m128i v = *reinterpret_cast<__m128i*>(const_cast<T*>(addr));
                atomic_barrier_helper<order>::post_load_barrier();
                return *reinterpret_cast<T*>(&v);
            }
        };

        //
        //store operations
        //

        //no special handling needed for 64-bit stores on x64, unlike x86
        template<typename T, memory_order order>
        struct atomic_store_helper<T, 8, order>
        {
            static void store(volatile T* addr, T v)
            {
                atomic_barrier_helper<order>::pre_store_barrier();
                *reinterpret_cast<volatile __int64*>(addr) = *reinterpret_cast<__int64*>(&v);
                atomic_barrier_helper<order>::post_store_barrier();
            }
        };

        template<typename T, memory_order order>
        struct atomic_store_helper<T, 16, order>
        {
            static void store(volatile T* addr, T v)
            {
                atomic_barrier_helper<order>::pre_store_barrier();
                //volatile __m128i isn't supported by the compiler... so we're just going to pray and hope
                *reinterpret_cast<__m128i*>(const_cast<T*>(addr)) = *reinterpret_cast<__m128i*>(&v);
                atomic_barrier_helper<order>::post_store_barrier();
            }
        };

        //
        //other atomic operations
        //

        template<typename T, memory_order order>
        struct atomic_operation_helper<T, 8, order>
        {
            static T exchange(volatile T* addr, T v)
            {
                __int64 result = _InterlockedExchange64(reinterpret_cast<volatile __int64*>(addr),
                        *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired)
            {
                return compare_exchange_strong(addr, expected, desired); //no spurious fail on x86
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired, memory_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence, no spurious fail on x86
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired)
            {
                __int64 expectedLong = *reinterpret_cast<__int64*>(&expected);
                __int64 result = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(addr),
                        *reinterpret_cast<__int64*>(&desired), expectedLong);
                if (result == expectedLong)
                {
                    return true;
                }
                expected = *reinterpret_cast<T*>(&result);
                return false;
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
            static T fetch_add(volatile T* addr, T v)
            {
                __int64 result = _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(addr),
                        *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                __int64 result = _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(addr),
                        -(*reinterpret_cast<__int64*>(&v)));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                __int64 result = _InterlockedAnd64(reinterpret_cast<volatile __int64*>(addr), *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                __int64 result = _InterlockedOr64(reinterpret_cast<volatile __int64*>(addr), *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                __int64 result = _InterlockedXor64(reinterpret_cast<volatile __int64*>(addr), *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_inc(volatile T* addr)
            {
                __int64 result = _InterlockedIncrement64(reinterpret_cast<volatile __int64*>(addr)) - 1;
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_dec(volatile T* addr)
            {
                __int64 result = _InterlockedDecrement64(reinterpret_cast<volatile __int64*>(addr)) + 1;
                return *reinterpret_cast<T*>(&result);
            }
        };

        template<typename T, memory_order order>
        struct atomic_operation_helper<T, 16, order>
        {
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired)
            {
                return compare_exchange_strong(addr, expected, desired); //no spurious fail on x86
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired, memory_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence, no spurious fail on x86
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired)
            {
                __int64 desiredHigh = *reinterpret_cast<__int64*>(&desired);
                __int64 desiredLow = *(reinterpret_cast<__int64*>(&desired) + 1);
                __int64* expectedPtr = reinterpret_cast<__int64*>(&expected);
                if (_InterlockedCompareExchange128(reinterpret_cast<volatile __int64*>(addr), desiredLow, desiredHigh,
                        expectedPtr))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
        };
    }
}
