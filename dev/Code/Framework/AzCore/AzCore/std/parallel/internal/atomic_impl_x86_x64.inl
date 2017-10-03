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

#   include <intrin.h>
#   pragma intrinsic(_ReadWriteBarrier)
#   pragma intrinsic(_InterlockedExchange)
#   pragma intrinsic(_InterlockedCompareExchange)
#   pragma intrinsic(_InterlockedExchangeAdd)
#   pragma intrinsic(_InterlockedAnd)
#   pragma intrinsic(_InterlockedOr)
#   pragma intrinsic(_InterlockedXor)
#   pragma intrinsic(_InterlockedIncrement)
#   pragma intrinsic(_InterlockedDecrement)

#ifndef MemoryBarrier
#   ifdef AZ_PLATFORM_WINDOWS_X64
    // AMD64
#       define AZ_MemoryBarrier __faststorefence
#   else 
    // x86
        AZ_FORCE_INLINE void AZ_MemoryBarrier()
        {
            LONG Barrier;
            __asm {
                xchg Barrier, eax
            }
        }
#   endif
#else
#   define AZ_MemoryBarrier MemoryBarrier
#endif 
//
//Shared implementation for x86 and x64
//

namespace AZStd
{
    namespace internal
    {
        //
        //storage class, aligns to necessary alignment for atomic operations, as T can be any generic type
        //
        template<typename T, size_t size>
        struct atomic_base_storage;

        template<typename T>
        struct atomic_base_storage<T, 4>
        {
            atomic_base_storage() { }
            atomic_base_storage(T v)
                : m_value(v) { }
        protected:
            AZ_ALIGN(T m_value, 4);
        };

        template<typename T>
        struct atomic_base_storage<T, 8>
        {
            atomic_base_storage() { }
            atomic_base_storage(T v)
                : m_value(v) { }
        protected:
            AZ_ALIGN(T m_value, 8);
        };

        //
        //storage converter, converts T to atomic type, padding and zeroing as necessary
        //
        template<typename T, size_t size>
        struct atomic_storage_converter;

        template<typename T>
        struct atomic_storage_converter<T, 4>
        {
            typedef long AtomicType;

            static long toAtomicType(T v)
            {
                return *reinterpret_cast<long*>(&v);
            }
            static T fromAtomicType(long v)
            {
                return *reinterpret_cast<T*>(&v);
            }
        };

        //
        //x86/x64 Memory Model:
        //
        //Stores ordered relative to stores.
        //Loads ordered relative to loads.
        //Loads can be re-ordered before stores.
        //
        //Locked instructions have a total order, so we get sequential-constistency automatically.
        //
        //Note that the compiler can still re-order even if the memory model does not, we use the compiler-only
        //_ReadWriteBarrier to prevent this where necessary.
        //

        //
        //barriers
        //
        template<memory_order order>
        struct atomic_barrier_helper
        {
        };

        template<>
        struct atomic_barrier_helper<memory_order_relaxed>
        {
            static void pre_load_barrier()   { }
            static void post_load_barrier()  { }
            static void pre_store_barrier()  { }
            static void post_store_barrier() { }
        };

        //barrier here is probably stronger than necessary
        template<>
        struct atomic_barrier_helper<memory_order_consume>
        {
            static void pre_load_barrier()   { }
            static void post_load_barrier()  { _ReadWriteBarrier(); }
            static void pre_store_barrier()  { }
            static void post_store_barrier() { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
        };

        template<>
        struct atomic_barrier_helper<memory_order_acquire>
        {
            static void pre_load_barrier()   { }
            static void post_load_barrier()  { _ReadWriteBarrier(); }
            static void pre_store_barrier()  { }
            static void post_store_barrier() { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
        };

        template<>
        struct atomic_barrier_helper<memory_order_release>
        {
            static void pre_load_barrier()   { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
            static void post_load_barrier()  { }
            static void pre_store_barrier()  { _ReadWriteBarrier(); }
            static void post_store_barrier() { }
        };

        template<>
        struct atomic_barrier_helper<memory_order_acq_rel>
        {
            static void pre_load_barrier()   { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
            static void post_load_barrier()  { _ReadWriteBarrier(); }
            static void pre_store_barrier()  { _ReadWriteBarrier(); }
            static void post_store_barrier() { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
        };

        template<>
        struct atomic_barrier_helper<memory_order_seq_cst>
        {
            static void pre_load_barrier()   { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
            static void post_load_barrier()  { _ReadWriteBarrier(); }
            static void pre_store_barrier()  { _ReadWriteBarrier(); }
            static void post_store_barrier() { AZ_MemoryBarrier(); _ReadWriteBarrier(); }
        };

        //
        //load operations
        //
        //We are also careful not to use T::operator= for any loads/stores, it could be doing many non-atomic things,
        //instead we cast and load/store as a fundamental type, T is required to be bitwise assignable anyway.
        //

        template<typename T, size_t size, memory_order order>
        struct atomic_load_helper
        {
        };

        template<typename T, memory_order order>
        struct atomic_load_helper<T, 1, order>
        {
            static T load(volatile T* addr)
            {
                atomic_barrier_helper<order>::pre_load_barrier();
                char v = *reinterpret_cast<volatile char*>(addr);
                atomic_barrier_helper<order>::post_load_barrier();
                return *reinterpret_cast<T*>(&v);
            }
        };

        template<typename T, memory_order order>
        struct atomic_load_helper<T, 2, order>
        {
            static T load(volatile T* addr)
            {
                atomic_barrier_helper<order>::pre_load_barrier();
                short v = *reinterpret_cast<volatile short*>(addr);
                atomic_barrier_helper<order>::post_load_barrier();
                return *reinterpret_cast<T*>(&v);
            }
        };

        template<typename T, memory_order order>
        struct atomic_load_helper<T, 4, order>
        {
            static T load(volatile T* addr)
            {
                atomic_barrier_helper<order>::pre_load_barrier();
                int v = *reinterpret_cast<volatile int*>(addr);
                atomic_barrier_helper<order>::post_load_barrier();
                return *reinterpret_cast<T*>(&v);
            }
        };


        //
        //store operations
        //

        template<typename T, size_t size, memory_order order>
        struct atomic_store_helper
        {
        };

        template<typename T, memory_order order>
        struct atomic_store_helper<T, 1, order>
        {
            static void store(volatile T* addr, T v)
            {
                atomic_barrier_helper<order>::pre_store_barrier();
                *reinterpret_cast<volatile char*>(addr) = *reinterpret_cast<char*>(&v);
                atomic_barrier_helper<order>::post_store_barrier();
            }
        };

        template<typename T, memory_order order>
        struct atomic_store_helper<T, 2, order>
        {
            static void store(volatile T* addr, T v)
            {
                atomic_barrier_helper<order>::pre_store_barrier();
                *reinterpret_cast<volatile short*>(addr) = *reinterpret_cast<short*>(&v);
                atomic_barrier_helper<order>::post_store_barrier();
            }
        };

        template<typename T, memory_order order>
        struct atomic_store_helper<T, 4, order>
        {
            static void store(volatile T* addr, T v)
            {
                atomic_barrier_helper<order>::pre_store_barrier();
                *reinterpret_cast<volatile int*>(addr) = *reinterpret_cast<int*>(&v);
                atomic_barrier_helper<order>::post_store_barrier();
            }
        };


        //
        //other atomic operations
        //

        //default implementation uses atomic_storage_converter to convert T to 32-bits, this implementation
        // is only actually used on x64, as x86 specializes all sizes
        template<typename T, size_t size, memory_order order>
        struct atomic_operation_helper
        {
            typedef atomic_storage_converter<T, size> convert;

            static T exchange(volatile T* addr, T v)
            {
                long result = _InterlockedExchange(reinterpret_cast<volatile long*>(addr),
                        convert::toAtomicType(v));
                return convert::fromAtomicType(result);
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
                long expectedLong = convert::toAtomicType(expected);
                long result = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(addr),
                        convert::toAtomicType(desired), expectedLong);
                if (result == expectedLong)
                {
                    return true;
                }
                expected = convert::fromAtomicType(result);
                return false;
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
            static T fetch_add(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v);
                long result = *reinterpret_cast<volatile long*>(addr);
                long oldValue;
                do
                {
                    oldValue = result;
                    long newValue = convert::mask(oldValue + longValue);
                    result = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(addr), newValue, oldValue);
                } while (result != oldValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v);
                long result = *reinterpret_cast<volatile long*>(addr);
                long oldValue;
                do
                {
                    oldValue = result;
                    long newValue = convert::mask(oldValue - longValue);
                    result = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(addr), newValue, oldValue);
                } while (result != oldValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v); //extra var needed to avoid compiler bug
                long result = _InterlockedAnd(reinterpret_cast<volatile long*>(addr), longValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v); //extra var needed to avoid compiler bug
                long result = _InterlockedOr(reinterpret_cast<volatile long*>(addr), longValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v); //extra var needed to avoid compiler bug
                long result = _InterlockedXor(reinterpret_cast<volatile long*>(addr), longValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_inc(volatile T* addr)
            {
                long result = *reinterpret_cast<volatile long*>(addr);
                long oldValue;
                do
                {
                    oldValue = result;
                    long newValue = convert::increment(oldValue);
                    result = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(addr), newValue, oldValue);
                } while (result != oldValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_dec(volatile T* addr)
            {
                long result = *reinterpret_cast<volatile long*>(addr);
                long oldValue;
                do
                {
                    oldValue = result;
                    long newValue = convert::decrement(oldValue);
                    result = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(addr), newValue, oldValue);
                } while (result != oldValue);
                return convert::fromAtomicType(result);
            }
        };

        //32-bit specialization is the same on both x86 and x64
        template<typename T, memory_order order>
        struct atomic_operation_helper<T, 4, order>
        {
            typedef atomic_storage_converter<T, 4> convert;

            static T exchange(volatile T* addr, T v)
            {
                long result = _InterlockedExchange(reinterpret_cast<volatile long*>(addr),
                        convert::toAtomicType(v));
                return convert::fromAtomicType(result);
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired)
            {
                return compare_exchange_strong(addr, expected, desired); //no spurious fail on x86
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                (void)failure_order;
                return compare_exchange_strong(addr, expected, desired); //always full fence, no spurious fail on x86
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired)
            {
                long expectedLong = convert::toAtomicType(expected);
                long result = _InterlockedCompareExchange(reinterpret_cast<volatile long*>(addr),
                        convert::toAtomicType(desired), expectedLong);
                if (result == expectedLong)
                {
                    return true;
                }
                expected = convert::fromAtomicType(result);
                return false;
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
            static T fetch_add(volatile T* addr, T v)
            {
                long result = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(addr),
                        convert::toAtomicType(v));
                return convert::fromAtomicType(result);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                long result = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(addr),
                        -convert::toAtomicType(v));
                return convert::fromAtomicType(result);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v); //extra var needed to avoid compiler bug
                long result = _InterlockedAnd(reinterpret_cast<volatile long*>(addr), longValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v); //extra var needed to avoid compiler bug
                long result = _InterlockedOr(reinterpret_cast<volatile long*>(addr), longValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                long longValue = convert::toAtomicType(v); //extra var needed to avoid compiler bug
                long result = _InterlockedXor(reinterpret_cast<volatile long*>(addr), longValue);
                return convert::fromAtomicType(result);
            }
            static T fetch_inc(volatile T* addr)
            {
                long result = _InterlockedIncrement(reinterpret_cast<volatile long*>(addr)) - 1;
                return convert::fromAtomicType(result);
            }
            static T fetch_dec(volatile T* addr)
            {
                long result = _InterlockedDecrement(reinterpret_cast<volatile long*>(addr)) + 1;
                return convert::fromAtomicType(result);
            }
        };
    }

    inline void atomic_thread_fence(memory_order order)
    {
        if (order != memory_order_relaxed)
        {
            AZ_MemoryBarrier();
        }
    }

    inline void atomic_signal_fence(memory_order order)
    {
        if (order != memory_order_relaxed)
        {
            _ReadWriteBarrier();
        }
    }
}
