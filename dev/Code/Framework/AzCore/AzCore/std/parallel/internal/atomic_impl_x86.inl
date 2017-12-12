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
#pragma intrinsic(_ReadWriteBarrier)
#pragma intrinsic(_InterlockedCompareExchange16)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedAnd8)
#pragma intrinsic(_InterlockedAnd16)
#pragma intrinsic(_InterlockedOr8)
#pragma intrinsic(_InterlockedOr16)
#pragma intrinsic(_InterlockedXor8)
#pragma intrinsic(_InterlockedXor16)
#pragma intrinsic(_InterlockedIncrement16)
#pragma intrinsic(_InterlockedDecrement16)

namespace AZStd
{
    namespace internal
    {
        //
        //Helper functions, atomic operations for which there is no intrinsic available
        //

#pragma warning(push)         //ebx is used by cmpxchg8b, and also by the compiler as a frame pointer... we're going
#pragma warning(disable:4731) // to save it, but need to make the compiler be quiet too

        inline __int64 atomicRead64(volatile void* p)
        {
            AZ_Assert((reinterpret_cast<intptr_t>(p) & 7) == 0, "Alignment error");
            __int64 result;
            __asm
            {
                mov eax, p
                fild qword ptr [eax]
                fistp qword ptr [result]
            }
            return result;
        }

        inline void atomicWrite64(volatile void* p, __int64 value)
        {
            AZ_Assert((reinterpret_cast<intptr_t>(p) & 7) == 0, "Alignment error");
            __asm
            {
                mov eax, p
                fild qword ptr [value]
                fistp qword ptr [eax]
            }
        }

        inline char atomicExchange8(volatile void* p, char value)
        {
            __asm
            {
                mov edx, p
                mov al, value
                lock xchg [edx], al
            }
        }

        inline short atomicExchange16(volatile void* p, short value)
        {
            __asm
            {
                mov edx, p
                mov ax, value
                lock xchg [edx], ax
            }
        }

        inline __int64 atomicExchange64(volatile void* p, __int64 value)
        {
            int valueLow = static_cast<int>(value);
            int valueHigh = static_cast<int>(value >> 32);
            __asm
            {
                push ebx
                mov edi, p
                mov ecx, valueHigh
                mov ebx, valueLow
                mov eax, [edi]      //read current value non-atomically here... it's just a guess, if it's wrong we'll try again
                mov edx, [edi + 4]
tryAgain:
                lock cmpxchg8b qword ptr [edi]
                jnz tryAgain
                pop ebx
            }
        }

        inline char fetchAdd8(volatile void* p, char addValue)
        {
            __asm
            {
                mov edx, p
                mov al, addValue
                lock xadd [edx], al
            }
        }

        inline short fetchAdd16(volatile void* p, short addValue)
        {
            __asm
            {
                mov edx, p
                mov ax, addValue
                lock xadd [edx], ax
            }
        }

        inline __int64 fetchAdd64(volatile void* p, __int64 addValue)
        {
            int addValueLow = static_cast<int>(addValue);
            int addValueHigh = static_cast<int>(addValue >> 32);
            __asm
            {
                push ebx
                mov edi, p
                mov eax, [edi]      //read current value non-atomically here... it's just a guess, if it's wrong we'll try again
                mov edx, [edi + 4]
tryAgain:
                mov ebx, addValueLow
                mov ecx, addValueHigh
                add ebx, eax
                adc ecx, edx
                lock cmpxchg8b qword ptr [edi]
                jnz tryAgain
                pop ebx
            }
        }

        inline __int64 fetchAnd64(volatile void* p, __int64 andValue)
        {
            int andValueLow = static_cast<int>(andValue);
            int andValueHigh = static_cast<int>(andValue >> 32);
            __asm
            {
                push ebx
                mov edi, p
                mov eax, [edi]      //read current value non-atomically here... it's just a guess, if it's wrong we'll try again
                mov edx, [edi + 4]
tryAgain:
                mov ebx, andValueLow
                mov ecx, andValueHigh
                and ebx, eax
                and ecx, edx
                lock cmpxchg8b qword ptr [edi]
                jnz tryAgain
                pop ebx
            }
        }

        inline __int64 fetchOr64(volatile void* p, __int64 orValue)
        {
            int orValueLow = static_cast<int>(orValue);
            int orValueHigh = static_cast<int>(orValue >> 32);
            __asm
            {
                push ebx
                mov edi, p
                mov eax, [edi]      //read current value non-atomically here... it's just a guess, if it's wrong we'll try again
                mov edx, [edi + 4]
tryAgain:
                mov ebx, orValueLow
                mov ecx, orValueHigh
                or ebx, eax
                or ecx, edx
                lock cmpxchg8b qword ptr [edi]
                jnz tryAgain
                pop ebx
            }
        }

        inline __int64 fetchXor64(volatile void* p, __int64 xorValue)
        {
            int xorValueLow = static_cast<int>(xorValue);
            int xorValueHigh = static_cast<int>(xorValue >> 32);
            __asm
            {
                push ebx
                mov edi, p
                mov eax, [edi]      //read current value non-atomically here... it's just a guess, if it's wrong we'll try again
                mov edx, [edi + 4]
tryAgain:
                mov ebx, xorValueLow
                mov ecx, xorValueHigh
                xor ebx, eax
                xor ecx, edx
                lock cmpxchg8b qword ptr [edi]
                jnz tryAgain
                pop ebx
            }
        }

        inline char compareExchange8(volatile void* p, char compare, char value)
        {
            __asm
            {
                mov edx, p
                mov cl, value
                mov al, compare
                lock cmpxchg [edx], cl
            }
        }
#pragma warning(pop)

        //
        //storage class, aligns to necessary alignment for atomic operations, as T can be any generic type
        //
        template<typename T>
        struct atomic_base_storage<T, 1>
        {
            atomic_base_storage() { }
            atomic_base_storage(T v)
                : m_value(v) { }
        protected:
            T m_value;
        };

        template<typename T>
        struct atomic_base_storage<T, 2>
        {
            atomic_base_storage() { }
            atomic_base_storage(T v)
                : m_value(v) { }
        protected:
            AZ_ALIGN(T m_value, 2);
        };

        //
        //load operations
        //

        //64-bit loads need special handling on x86
        template<typename T, memory_order order>
        struct atomic_load_helper<T, 8, order>
        {
            static T load(volatile T* addr)
            {
                atomic_barrier_helper<order>::pre_load_barrier();
                __int64 v = atomicRead64(addr);
                atomic_barrier_helper<order>::post_load_barrier();
                return *reinterpret_cast<T*>(&v);
            }
        };

        //
        //store operations
        //

        //64-bit stores need special handling on x86
        template<typename T, memory_order order>
        struct atomic_store_helper<T, 8, order>
        {
            static void store(volatile T* addr, T v)
            {
                atomic_barrier_helper<order>::pre_store_barrier();
                atomicWrite64(addr, *reinterpret_cast<__int64*>(&v));
                atomic_barrier_helper<order>::post_store_barrier();
            }
        };


        //
        //other atomic operations
        //

        template<typename T, memory_order order>
        struct atomic_operation_helper<T, 1, order>
        {
            static T exchange(volatile T* addr, T v)
            {
                char result = atomicExchange8(addr, *reinterpret_cast<char*>(&v));
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
                char expectedChar = *reinterpret_cast<char*>(&expected);
                char result = compareExchange8(addr, expectedChar, *reinterpret_cast<char*>(&desired));
                if (result == expectedChar)
                {
                    return true;
                }
                expected = *reinterpret_cast<T*>(&result);
                return false;
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
            static T fetch_add(volatile T* addr, T v)
            {
                char result = fetchAdd8(addr, *reinterpret_cast<char*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                char result = fetchAdd8(addr, -(*reinterpret_cast<char*>(&v)));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                char result = _InterlockedAnd8(reinterpret_cast<volatile char*>(addr),
                        *reinterpret_cast<char*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                char result = _InterlockedOr8(reinterpret_cast<volatile char*>(addr),
                        *reinterpret_cast<char*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                char result = _InterlockedXor8(reinterpret_cast<volatile char*>(addr),
                        *reinterpret_cast<char*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_inc(volatile T* addr)
            {
                char result = fetchAdd8(addr, 1);
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_dec(volatile T* addr)
            {
                char result = fetchAdd8(addr, -1);
                return *reinterpret_cast<T*>(&result);
            }
        };

        template<typename T, memory_order order>
        struct atomic_operation_helper<T, 2, order>
        {
            static T exchange(volatile T* addr, T v)
            {
                short result = atomicExchange16(addr, *reinterpret_cast<short*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired)
            {
                return compare_exchange_strong(addr, expected, desired); //no spurious fail on x86
            }
            static bool compare_exchange_weak(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence, no spurious fail on x86
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired)
            {
                short expectedShort = *reinterpret_cast<short*>(&expected);
                short result = _InterlockedCompareExchange16(reinterpret_cast<volatile short*>(addr),
                        *reinterpret_cast<short*>(&desired), expectedShort);
                if (result == expectedShort)
                {
                    return true;
                }
                expected = *reinterpret_cast<T*>(&result);
                return false;
            }
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
            static T fetch_add(volatile T* addr, T v)
            {
                short result = fetchAdd16(addr, *reinterpret_cast<short*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                short result = fetchAdd16(addr, -(*reinterpret_cast<short*>(&v)));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                short result = _InterlockedAnd16(reinterpret_cast<volatile short*>(addr),
                        *reinterpret_cast<short*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                short result = _InterlockedOr16(reinterpret_cast<volatile short*>(addr),
                        *reinterpret_cast<short*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                short result = _InterlockedXor16(reinterpret_cast<volatile short*>(addr),
                        *reinterpret_cast<short*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_inc(volatile T* addr)
            {
                short result = _InterlockedIncrement16(reinterpret_cast<volatile short*>(addr)) - 1;
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_dec(volatile T* addr)
            {
                short result = _InterlockedDecrement16(reinterpret_cast<volatile short*>(addr)) + 1;
                return *reinterpret_cast<T*>(&result);
            }
        };

        template<typename T, memory_order order>
        struct atomic_operation_helper<T, 8, order>
        {
            static T exchange(volatile T* addr, T v)
            {
                __int64 result = atomicExchange64(addr, *reinterpret_cast<__int64*>(&v));
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
            static bool compare_exchange_strong(volatile T* addr, T& expected, T desired, memory_order failure_order)
            {
                return compare_exchange_strong(addr, expected, desired); //always full fence on x86
            }
            static T fetch_add(volatile T* addr, T v)
            {
                __int64 result = fetchAdd64(addr, *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_sub(volatile T* addr, T v)
            {
                __int64 result = fetchAdd64(addr, -(*reinterpret_cast<__int64*>(&v)));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_and(volatile T* addr, T v)
            {
                __int64 result = fetchAnd64(addr, *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_or(volatile T* addr, T v)
            {
                __int64 result = fetchOr64(addr, *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_xor(volatile T* addr, T v)
            {
                __int64 result = fetchXor64(addr, *reinterpret_cast<__int64*>(&v));
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_inc(volatile T* addr)
            {
                __int64 result = fetchAdd64(addr, 1);
                return *reinterpret_cast<T*>(&result);
            }
            static T fetch_dec(volatile T* addr)
            {
                __int64 result = fetchAdd64(addr, -1);
                return *reinterpret_cast<T*>(&result);
            }
        };
    }
}
