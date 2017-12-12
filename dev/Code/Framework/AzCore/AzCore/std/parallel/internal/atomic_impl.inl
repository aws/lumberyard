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

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/conditional.h>

#if !defined(AZ_USE_STD_ATOMIC)

#if defined(AZ_PLATFORM_WINDOWS)
    #include <AzCore/std/parallel/internal/atomic_impl_x86_x64.inl>
    #if defined(AZ_PLATFORM_WINDOWS_X64)
        #include <AzCore/std/parallel/internal/atomic_impl_x64.inl>
    #else
        #include <AzCore/std/parallel/internal/atomic_impl_x86.inl>
    #endif
#else
    #include <AzCore/std/parallel/internal/atomic_impl_locks.inl>
#endif

namespace AZStd
{
    template <typename T>
    T kill_dependency(T t)
    {
        return t;
    }

    namespace internal
    {
        inline bool memory_order_is_weaker_or_equal(memory_order order, memory_order compare_order)
        {
            return (order <= compare_order);
        }

        //
        //common functionality for all atomic types
        //
        template<typename T>
        struct atomic_base
            : public atomic_base_storage<T, sizeof(T)>
        {
            typedef atomic_base_storage<T, sizeof(T)> base_type;

            atomic_base() { }
            atomic_base(T v)
                : base_type(v) { }

            bool is_lock_free() const volatile { return AZ_ATOMICS_LOCK_FREE == 2; }

            void store(T v, memory_order order = memory_order_seq_cst) volatile
            {
                AZ_Assert((order == memory_order_relaxed) || (order == memory_order_release) || (order == memory_order_seq_cst), "Invalid memory ordering type for store");
                if (order == memory_order_relaxed)
                {
                    store<memory_order_relaxed>(v);
                }
                else if (order == memory_order_release)
                {
                    store<memory_order_release>(v);
                }
                else
                {
                    store<memory_order_seq_cst>(v);
                }
            }
            T load(memory_order order = memory_order_seq_cst) const volatile
            {
                AZ_Assert((order == memory_order_relaxed) || (order == memory_order_consume) || (order == memory_order_acquire) || (order == memory_order_seq_cst),
                    "Invalid memory ordering type for load");
                if (order == memory_order_relaxed)
                {
                    return load<memory_order_relaxed>();
                }
                else if (order == memory_order_consume)
                {
                    return load<memory_order_consume>();
                }
                else if (order == memory_order_acquire)
                {
                    return load<memory_order_acquire>();
                }
                else
                {
                    return load<memory_order_seq_cst>();
                }
            }
            T exchange(T v, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return exchange<memory_order_relaxed>(v);
                }
                else if (order == memory_order_consume)
                {
                    return exchange<memory_order_consume>(v);
                }
                else if (order == memory_order_acquire)
                {
                    return exchange<memory_order_acquire>(v);
                }
                else if (order == memory_order_release)
                {
                    return exchange<memory_order_release>(v);
                }
                else if (order == memory_order_acq_rel)
                {
                    return exchange<memory_order_acq_rel>(v);
                }
                else
                {
                    return exchange<memory_order_seq_cst>(v);
                }
            }
            bool compare_exchange_weak(T& expected, T desired, memory_order success, memory_order failure) volatile
            {
                AZ_Assert((failure != memory_order_release) && (failure != memory_order_acq_rel),
                    "Invalid memory ordering type for failure");
                AZ_Assert(memory_order_is_weaker_or_equal(failure, success),
                    "Failure order must be weaker than or equal to success order");
                if (success == memory_order_relaxed)
                {
                    return compare_exchange_weak<memory_order_relaxed>(expected, desired, failure);
                }
                else if (success == memory_order_consume)
                {
                    return compare_exchange_weak<memory_order_consume>(expected, desired, failure);
                }
                else if (success == memory_order_acquire)
                {
                    return compare_exchange_weak<memory_order_acquire>(expected, desired, failure);
                }
                else if (success == memory_order_release)
                {
                    return compare_exchange_weak<memory_order_release>(expected, desired, failure);
                }
                else if (success == memory_order_acq_rel)
                {
                    return compare_exchange_weak<memory_order_acq_rel>(expected, desired, failure);
                }
                else
                {
                    return compare_exchange_weak<memory_order_seq_cst>(expected, desired, failure);
                }
            }
            bool compare_exchange_strong(T& expected, T desired, memory_order success, memory_order failure) volatile
            {
                AZ_Assert((failure != memory_order_release) && (failure != memory_order_acq_rel),
                    "Invalid memory ordering type for failure");
                AZ_Assert(memory_order_is_weaker_or_equal(failure, success),
                    "Failure order must be weaker than or equal to success order");
                if (success == memory_order_relaxed)
                {
                    return compare_exchange_strong<memory_order_relaxed>(expected, desired, failure);
                }
                else if (success == memory_order_consume)
                {
                    return compare_exchange_strong<memory_order_consume>(expected, desired, failure);
                }
                else if (success == memory_order_acquire)
                {
                    return compare_exchange_strong<memory_order_acquire>(expected, desired, failure);
                }
                else if (success == memory_order_release)
                {
                    return compare_exchange_strong<memory_order_release>(expected, desired, failure);
                }
                else if (success == memory_order_acq_rel)
                {
                    return compare_exchange_strong<memory_order_acq_rel>(expected, desired, failure);
                }
                else
                {
                    return compare_exchange_strong<memory_order_seq_cst>(expected, desired, failure);
                }
            }
            bool compare_exchange_weak(T& expected, T desired, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return compare_exchange_weak<memory_order_relaxed>(expected, desired);
                }
                else if (order == memory_order_consume)
                {
                    return compare_exchange_weak<memory_order_consume>(expected, desired);
                }
                else if (order == memory_order_acquire)
                {
                    return compare_exchange_weak<memory_order_acquire>(expected, desired);
                }
                else if (order == memory_order_release)
                {
                    return compare_exchange_weak<memory_order_release>(expected, desired);
                }
                else if (order == memory_order_acq_rel)
                {
                    return compare_exchange_weak<memory_order_acq_rel>(expected, desired);
                }
                else
                {
                    return compare_exchange_weak<memory_order_seq_cst>(expected, desired);
                }
            }
            bool compare_exchange_strong(T& expected, T desired, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return compare_exchange_strong<memory_order_relaxed>(expected, desired);
                }
                else if (order == memory_order_consume)
                {
                    return compare_exchange_strong<memory_order_consume>(expected, desired);
                }
                else if (order == memory_order_acquire)
                {
                    return compare_exchange_strong<memory_order_acquire>(expected, desired);
                }
                else if (order == memory_order_release)
                {
                    return compare_exchange_strong<memory_order_release>(expected, desired);
                }
                else if (order == memory_order_acq_rel)
                {
                    return compare_exchange_strong<memory_order_acq_rel>(expected, desired);
                }
                else
                {
                    return compare_exchange_strong<memory_order_seq_cst>(expected, desired);
                }
            }

            operator T() const volatile {
                return load();
            }

            //
            //The following template functions are extensions to the standard, useful if you don't trust the compiler
            // to optimize away the branches
            //
            template<memory_order order>
            void store(T v) volatile
            {
                atomic_store_helper<T, sizeof(T), order>::store(&this->m_value, v);
            }
            template<memory_order order>
            T load() const volatile
            {
                return atomic_load_helper<T, sizeof(T), order>::load(const_cast<T*>(&this->m_value));
            }
            template<memory_order order>
            T exchange(T v) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::exchange(&this->m_value, v);
            }
            template<memory_order order>
            bool compare_exchange_weak(T& expected, T desired) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::compare_exchange_weak(&this->m_value, expected,
                    desired);
            }
            template<memory_order order>
            bool compare_exchange_weak(T& expected, T desired, memory_order failure_order) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::compare_exchange_weak(&this->m_value, expected,
                    desired, failure_order);
            }
            template<memory_order order>
            bool compare_exchange_strong(T& expected, T desired) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::compare_exchange_strong(&this->m_value, expected,
                    desired);
            }
            template<memory_order order>
            bool compare_exchange_strong(T& expected, T desired, memory_order failure_order) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::compare_exchange_strong(&this->m_value, expected,
                    desired, failure_order);
            }
        };

        //
        //common functionality for integral atomic types
        //
        template<typename T>
        struct atomic_integral
            : public atomic_base<T>
        {
            typedef atomic_base<T> base_type;

            atomic_integral() { }
            atomic_integral(T v)
                : base_type(v) { }

            T fetch_add(T v, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_add<memory_order_relaxed>(v);
                }
                else if (order == memory_order_consume)
                {
                    return fetch_add<memory_order_consume>(v);
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_add<memory_order_acquire>(v);
                }
                else if (order == memory_order_release)
                {
                    return fetch_add<memory_order_release>(v);
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_add<memory_order_acq_rel>(v);
                }
                else
                {
                    return fetch_add<memory_order_seq_cst>(v);
                }
            }
            T fetch_sub(T v, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_sub<memory_order_relaxed>(v);
                }
                else if (order == memory_order_consume)
                {
                    return fetch_sub<memory_order_consume>(v);
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_sub<memory_order_acquire>(v);
                }
                else if (order == memory_order_release)
                {
                    return fetch_sub<memory_order_release>(v);
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_sub<memory_order_acq_rel>(v);
                }
                else
                {
                    return fetch_sub<memory_order_seq_cst>(v);
                }
            }
            T fetch_and(T v, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_and<memory_order_relaxed>(v);
                }
                else if (order == memory_order_consume)
                {
                    return fetch_and<memory_order_consume>(v);
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_and<memory_order_acquire>(v);
                }
                else if (order == memory_order_release)
                {
                    return fetch_and<memory_order_release>(v);
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_and<memory_order_acq_rel>(v);
                }
                else
                {
                    return fetch_and<memory_order_seq_cst>(v);
                }
            }
            T fetch_or(T v, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_or<memory_order_relaxed>(v);
                }
                else if (order == memory_order_consume)
                {
                    return fetch_or<memory_order_consume>(v);
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_or<memory_order_acquire>(v);
                }
                else if (order == memory_order_release)
                {
                    return fetch_or<memory_order_release>(v);
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_or<memory_order_acq_rel>(v);
                }
                else
                {
                    return fetch_or<memory_order_seq_cst>(v);
                }
            }
            T fetch_xor(T v, memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_xor<memory_order_relaxed>(v);
                }
                else if (order == memory_order_consume)
                {
                    return fetch_xor<memory_order_consume>(v);
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_xor<memory_order_acquire>(v);
                }
                else if (order == memory_order_release)
                {
                    return fetch_xor<memory_order_release>(v);
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_xor<memory_order_acq_rel>(v);
                }
                else
                {
                    return fetch_xor<memory_order_seq_cst>(v);
                }
            }
            ///extension
            T fetch_inc(memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_inc<memory_order_relaxed>();
                }
                else if (order == memory_order_consume)
                {
                    return fetch_inc<memory_order_consume>();
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_inc<memory_order_acquire>();
                }
                else if (order == memory_order_release)
                {
                    return fetch_inc<memory_order_release>();
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_inc<memory_order_acq_rel>();
                }
                else
                {
                    return fetch_inc<memory_order_seq_cst>();
                }
            }
            ///extension
            T fetch_dec(memory_order order = memory_order_seq_cst) volatile
            {
                if (order == memory_order_relaxed)
                {
                    return fetch_dec<memory_order_relaxed>();
                }
                else if (order == memory_order_consume)
                {
                    return fetch_dec<memory_order_consume>();
                }
                else if (order == memory_order_acquire)
                {
                    return fetch_dec<memory_order_acquire>();
                }
                else if (order == memory_order_release)
                {
                    return fetch_dec<memory_order_release>();
                }
                else if (order == memory_order_acq_rel)
                {
                    return fetch_dec<memory_order_acq_rel>();
                }
                else
                {
                    return fetch_dec<memory_order_seq_cst>();
                }
            }

            T operator++(int) volatile { return fetch_inc<memory_order_seq_cst>(); }
            T operator--(int) volatile { return fetch_dec<memory_order_seq_cst>(); }
            T operator++() volatile    { return fetch_inc<memory_order_seq_cst>() + 1; }
            T operator--() volatile    { return fetch_dec<memory_order_seq_cst>() - 1; }
            T operator+=(T v) volatile { return fetch_add<memory_order_seq_cst>(v) + v; }
            T operator-=(T v) volatile { return fetch_sub<memory_order_seq_cst>(v) - v; }
            T operator&=(T v) volatile { return fetch_and<memory_order_seq_cst>(v) & v; }
            T operator|=(T v) volatile { return fetch_or<memory_order_seq_cst>(v) | v; }
            T operator^=(T v) volatile { return fetch_xor<memory_order_seq_cst>(v) ^ v; }

            //
            //The following template functions are extensions to the standard, useful if you don't trust the compiler
            // to optimize away the branches
            //
            template<memory_order order>
            T fetch_add(T v) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_add(&this->m_value, v);
            }
            template<memory_order order>
            T fetch_sub(T v) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_sub(&this->m_value, v);
            }
            template<memory_order order>
            T fetch_and(T v) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_and(&this->m_value, v);
            }
            template<memory_order order>
            T fetch_or(T v) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_or(&this->m_value, v);
            }
            template<memory_order order>
            T fetch_xor(T v) volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_xor(&this->m_value, v);
            }
            template<memory_order order>
            T fetch_inc() volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_inc(&this->m_value);
            }
            template<memory_order order>
            T fetch_dec() volatile
            {
                return atomic_operation_helper<T, sizeof(T), order>::fetch_dec(&this->m_value);
            }
        };
    }

    /**
     * Boolean atomic type
     */
    struct atomic_bool
        : public internal::atomic_base<bool>
    {
        atomic_bool()
            : atomic_bool(false) { }
        atomic_bool(bool v)
            : internal::atomic_base<bool>(v) { }
        bool operator=(bool v) volatile { store(v); return v; }
        atomic_bool(const atomic_bool&) = delete;
        atomic_bool& operator=(const atomic_bool& v) volatile = delete;
    };

    /**
     * Address atomic type, acts like a void*
     */
    struct atomic_address
        : public internal::atomic_base<void*>
    {
        atomic_address()
            : atomic_address(nullptr) { }
        atomic_address(void* v)
            : internal::atomic_base<void*>(v) { }
        void* operator=(void* v) volatile { store(v); return v; }

        void* fetch_add(ptrdiff_t v, memory_order order = memory_order_seq_cst) volatile
        {
            if (order == memory_order_relaxed)
            {
                return fetch_add<memory_order_relaxed>(v);
            }
            else if (order == memory_order_consume)
            {
                return fetch_add<memory_order_consume>(v);
            }
            else if (order == memory_order_acquire)
            {
                return fetch_add<memory_order_acquire>(v);
            }
            else if (order == memory_order_release)
            {
                return fetch_add<memory_order_release>(v);
            }
            else if (order == memory_order_acq_rel)
            {
                return fetch_add<memory_order_acq_rel>(v);
            }
            else
            {
                return fetch_add<memory_order_seq_cst>(v);
            }
        }
        void* fetch_sub(ptrdiff_t v, memory_order order = memory_order_seq_cst) volatile
        {
            if (order == memory_order_relaxed)
            {
                return fetch_sub<memory_order_relaxed>(v);
            }
            else if (order == memory_order_consume)
            {
                return fetch_sub<memory_order_consume>(v);
            }
            else if (order == memory_order_acquire)
            {
                return fetch_sub<memory_order_acquire>(v);
            }
            else if (order == memory_order_release)
            {
                return fetch_sub<memory_order_release>(v);
            }
            else if (order == memory_order_acq_rel)
            {
                return fetch_sub<memory_order_acq_rel>(v);
            }
            else
            {
                return fetch_sub<memory_order_seq_cst>(v);
            }
        }

        void* operator+=(ptrdiff_t v) volatile { return static_cast<char*>(fetch_add<memory_order_seq_cst>(v)) + v; }
        void* operator-=(ptrdiff_t v) volatile { return static_cast<char*>(fetch_sub<memory_order_seq_cst>(v)) - v; }

        //
        //The following template functions are extensions to the standard, useful if you don't trust the compiler
        // to optimize away the branches
        //
        template<memory_order order>
        void* fetch_add(ptrdiff_t v) volatile
        {
            return internal::atomic_operation_helper<void*, sizeof(void*), order>::fetch_add(&m_value,
                reinterpret_cast<void*>(v));
        }
        template<memory_order order>
        void* fetch_sub(ptrdiff_t v) volatile
        {
            return internal::atomic_operation_helper<void*, sizeof(void*), order>::fetch_sub(&m_value,
                reinterpret_cast<void*>(v));
        }

        atomic_address(const atomic_address&) = delete;
        atomic_address& operator=(const atomic_address& v) volatile = delete;
    };

    /**
     * Generic atomic type, will work with any type having a supported size.
     */
    template <typename T>
    struct atomic
        : public AZStd::Utils::if_c<AZStd::is_integral<T>::value, internal::atomic_integral<T>, internal::atomic_base<T>>::type
    {
        using base = typename AZStd::Utils::if_c<AZStd::is_integral<T>::value, internal::atomic_integral<T>, internal::atomic_base<T>>::type;
        atomic()
            : base(T()) { }
        atomic(T v)
            : base(v) { }
        T operator=(T v) volatile { this->store(v); return v; }
        // delete copy, cv operator =
        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) volatile = delete;
    };

    /**
     * Specialization of generic atomic type for pointers, all arithmetic is in units of the size of the pointed type.
     * Derives from atomic_address, which is consistent for conversions, but means we must re-implement operations
     * to work with generic T* type instead of void* type.
     */
    template <typename T>
    struct atomic<T*>
        : public atomic_address
    {
        atomic() 
            : atomic_address(nullptr) { }
        atomic(T* v)
            : atomic_address(v) { }
        T* operator=(T* v) volatile { store(v); return v; }

        void store(T* v, memory_order order = memory_order_seq_cst) volatile 
        { 
            atomic_address::store(const_cast<typename AZStd::remove_const<T>::type*&>(v), order);
        }
        T* load(memory_order order = memory_order_seq_cst) const volatile
        {
            return static_cast<T*>(atomic_address::load(order));
        }
        operator T*() const volatile {
            return load();
        }
        T* exchange(T* v, memory_order order = memory_order_seq_cst) volatile
        {
            return static_cast<T*>(atomic_address::exchange(const_cast<typename AZStd::remove_const<T>::type*&>(v), order));
        }
        bool compare_exchange_weak(T*& expected, T* desired, memory_order success, memory_order failure) volatile
        {
            return atomic_address::compare_exchange_weak(reinterpret_cast<void*&>(
                const_cast<typename AZStd::remove_const<T>::type*&>(expected)), 
                const_cast<typename AZStd::remove_const<T>::type*&>(desired),
                success, failure);
        }
        bool compare_exchange_strong(T*& expected, T* desired, memory_order success, memory_order failure) volatile
        {
            return atomic_address::compare_exchange_strong(reinterpret_cast<void*&>(
                const_cast<typename AZStd::remove_const<T>::type*&>(expected)), 
                const_cast<typename AZStd::remove_const<T>::type*&>(desired),
                success, failure);
        }
        bool compare_exchange_weak(T*& expected, T* desired, memory_order order = memory_order_seq_cst) volatile
        {
            return atomic_address::compare_exchange_weak(reinterpret_cast<void*&>(
                const_cast<typename AZStd::remove_const<T>::type*&>(expected)), 
                const_cast<typename AZStd::remove_const<T>::type*&>(desired), order);
        }
        bool compare_exchange_strong(T*& expected, T* desired, memory_order order = memory_order_seq_cst) volatile
        {
            return atomic_address::compare_exchange_strong(reinterpret_cast<void*&>(
                const_cast<typename AZStd::remove_const<T>::type*&>(expected)), 
                const_cast<typename AZStd::remove_const<T>::type*&>(desired), order);
        }
        T* fetch_add(ptrdiff_t v, memory_order order = memory_order_seq_cst) volatile
        {
            return static_cast<T*>(atomic_address::fetch_add(v * sizeof(T), order));
        }
        T* fetch_sub(ptrdiff_t v, memory_order order = memory_order_seq_cst) volatile
        {
            return static_cast<T*>(atomic_address::fetch_sub(v * sizeof(T), order));
        }

        T* operator++(int) volatile { return fetch_add(1); }
        T* operator--(int) volatile { return fetch_sub(1); }
        T* operator++() volatile    { return fetch_add(1) + 1; }
        T* operator--() volatile    { return fetch_sub(1) - 1; }
        T* operator+=(ptrdiff_t v) volatile { return fetch_add(v) + v; }
        T* operator-=(ptrdiff_t v) volatile { return fetch_sub(v) - v; }

        //
        //The following template functions are extensions to the standard, useful if you don't trust the compiler
        // to optimize away the branches
        //
        template<memory_order order>
        void store(T* v) volatile
        {
            atomic_address::store<order>(v);
        }
        template<memory_order order>
        T* load() const volatile
        {
            return static_cast<T*>(atomic_address::load<order>());
        }
        template<memory_order order>
        T* exchange(T* v) volatile
        {
            return static_cast<T*>(atomic_address::exchange<order>(v));
        }
        template<memory_order order>
        bool compare_exchange_weak(T*& expected, T* desired) volatile
        {
            return atomic_address::compare_exchange_weak<order>(reinterpret_cast<void*&>(expected), desired);
        }
        template<memory_order order>
        bool compare_exchange_weak(T*& expected, T* desired, memory_order failure_order) volatile
        {
            return atomic_address::compare_exchange_weak<order>(reinterpret_cast<void*&>(expected), desired, failure_order);
        }
        template<memory_order order>
        bool compare_exchange_strong(T*& expected, T* desired) volatile
        {
            return atomic_address::compare_exchange_strong<order>(reinterpret_cast<void*&>(expected), desired);
        }
        template<memory_order order>
        bool compare_exchange_strong(T*& expected, T* desired, memory_order failure_order) volatile
        {
            return atomic_address::compare_exchange_strong<order>(reinterpret_cast<void*&>(expected), desired, failure_order);
        }
        template<memory_order order>
        T* fetch_add(ptrdiff_t v) volatile
        {
            return static_cast<T*>(atomic_address::fetch_add<order>(v * sizeof(T)));
        }
        template<memory_order order>
        T* fetch_sub(ptrdiff_t v) volatile
        {
            return static_cast<T*>(atomic_address::fetch_sub<order>(v * sizeof(T)));
        }

        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) volatile = delete;
    };

    /**
     * Basic atomic flag type, provides test-and-set functionality only, this is the only type required to be
     * lock-free by the standard. Note that our default locking implementation for platforms without native atomics
     * does not bother to make this lock-free.
     */
    struct atomic_flag
    {
        atomic_flag(bool flag=false)
            : m_flag(flag)
        { }
        bool test_and_set(memory_order order = memory_order_seq_cst) volatile { return m_flag.exchange(true, order); }
        void clear(memory_order order = memory_order_seq_cst) volatile { m_flag.store(false, order); }
    private:
        atomic_flag(const atomic_flag&) = delete;
        atomic_flag& operator=(const atomic_flag&) = delete;

        atomic_bool m_flag;
    };

    using atomic_char = atomic<char>;
    using atomic_schar = atomic<signed char>;
    using atomic_uchar = atomic<unsigned char>;
    using atomic_wchar_t = atomic<wchar_t>;
    using atomic_short = atomic<short>;
    using atomic_ushort = atomic<unsigned short>;
    using atomic_int = atomic<int>;
    using atomic_uint = atomic<unsigned int>;
    using atomic_long = atomic<long>;
    using atomic_ulong = atomic<unsigned long>;
    using atomic_llong = atomic<AZ::s64>;
    using atomic_ullong = atomic<AZ::u64>;
    using atomic_size_t = atomic<size_t>;
    using atomic_ptrdiff_t = atomic<ptrdiff_t>;
}
#endif // !AZ_USE_STD_ATOMIC

namespace AZStd
{
    inline void atomic_init(atomic_bool* atom, bool desired)
    {
        *atom = desired;
    }

    inline void atomic_init(volatile atomic_bool* atom, bool desired)
    {
        *atom = desired;
    }

    template <class T>
    inline void atomic_init(atomic<T>* atom, T desired)
    {
        *atom = desired;
    }

    template <class T>
    inline void atomic_init(volatile atomic<T>* atom, T desired)
    {
        *atom = desired;
    }

    template <class T>
    inline bool atomic_is_lock_free(const atomic<T>* atom)
    {
        return atom->is_lock_free();
    }

    template <class T>
    inline bool atomic_is_lock_free(const volatile atomic<T>* atom)
    {
        return atom->is_lock_free();
    }

    template <class T>
    inline void atomic_store(atomic<T>* atom, T desired)
    {
        atom->store(desired);
    }

    template <class T>
    inline void atomic_store(volatile atomic<T>* atom, T desired)
    {
        atom->store(desired);
    }

    template <class T>
    inline void atomic_store_explicit(atomic<T>* atom, T desired, memory_order order)
    {
        atom->store(desired, order);
    }

    template <class T>
    inline void atomic_store_explicit(volatile atomic<T>* atom, T desired, memory_order order)
    {
        atom->store(desired, order);
    }

    template <class T>
    inline T atomic_load(atomic<T>* atom)
    {
        return atom->load();
    }

    template <class T>
    inline T atomic_load(volatile atomic<T>* atom)
    {
        return atom->load();
    }

    template <class T>
    inline T atomic_load_explicit(atomic<T>* atom, memory_order order)
    {
        return atom->load(order);
    }

    template <class T>
    inline T atomic_load_explicit(volatile atomic<T>* atom, memory_order order)
    {
        return atom->load(order);
    }

    template <class T>
    inline T atomic_exchange(atomic<T>* atom, T desired)
    {
        return atom->exchange(desired);
    }

    template <class T>
    inline T atomic_exchange(volatile atomic<T>* atom, T desired)
    {
        return atom->exchange(desired);
    }

    template <class T>
    inline T atomic_exchange_explicit(atomic<T>* atom, T desired, memory_order order)
    {
        return atom->exchange(desired, order);
    }

    template <class T>
    inline T atomic_exchange_explicit(volatile atomic<T>* atom, T desired, memory_order order)
    {
        return atom->exchange(desired, order);
    }

    template <class T>
    inline bool atomic_compare_exchange_weak(atomic<T>* atom, T* expected, T desired)
    {
        return atom->compare_exchange_weak(*expected, desired);
    }

    template <class T>
    inline bool atomic_compare_exchange_weak(volatile atomic<T>* atom, T* expected, T desired)
    {
        return atom->compare_exchange_weak(*expected, desired);
    }

    template <class T>
    inline bool atomic_compare_exchange_weak_explicit(atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail)
    {
        return atom->compare_exchange_weak(*expected, desired, success, fail);
    }

    template <class T>
    inline bool atomic_compare_exchange_weak_explicit(volatile atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail)
    {
        return atom->compare_exchange_weak(*expected, desired, success, fail);
    }

    template <class T>
    inline bool atomic_compare_exchange_strong(atomic<T>* atom, T* expected, T desired)
    {
        return atom->compare_exchange_strong(*expected, desired);
    }

    template <class T>
    inline bool atomic_compare_exchange_strong(volatile atomic<T>* atom, T* expected, T desired)
    {
        return atom->compare_exchange_strong(*expected, desired);
    }

    template <class T>
    inline bool atomic_compare_exchange_strong_explicit(atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail)
    {
        return atom->compare_exchange_strong(*expected, desired, success, fail);
    }

    template <class T>
    inline bool atomic_compare_exchange_strong_explicit(volatile atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail)
    {
        return atom->compare_exchange_strong(*expected, desired, success, fail);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_add(atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_add(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_add(volatile atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_add(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_add_explicit(atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_add(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_add_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_add(arg, order);
    }

    template <class T>
    inline T* atomic_fetch_add(atomic<T*>* atom, ptrdiff_t arg)
    {
        return atom->fetch_add(arg);
    }

    template <class T>
    inline T* atomic_fetch_add(volatile atomic<T*>* atom, ptrdiff_t arg)
    {
        return atom->fetch_add(arg);
    }

    template <class T>
    inline T* atomic_fetch_add_explicit(atomic<T*>* atom, ptrdiff_t arg, memory_order order)
    {
        return atom->fetch_add(arg, order);
    }

    template <class T>
    inline T* atomic_fetch_add_explicit(volatile atomic<T*>* atom, ptrdiff_t arg, memory_order order)
    {
        return atom->fetch_add(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_sub(atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_sub(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_sub(volatile atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_sub(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_sub_explicit(atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_sub(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_sub_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_sub(arg, order);
    }

    template <class T>
    inline T* atomic_fetch_sub(atomic<T*>* atom, ptrdiff_t arg)
    {
        return atom->fetch_sub(arg);
    }

    template <class T>
    inline T* atomic_fetch_sub(volatile atomic<T*>* atom, ptrdiff_t arg)
    {
        return atom->fetch_sub(arg);
    }

    template <class T>
    inline T* atomic_fetch_sub_explicit(atomic<T*>* atom, ptrdiff_t arg, memory_order order)
    {
        return atom->fetch_sub(arg, order);
    }

    template <class T>
    inline T* atomic_fetch_sub_explicit(volatile atomic<T*>* atom, ptrdiff_t arg, memory_order order)
    {
        return atom->fetch_sub(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_and(atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_and(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_and(volatile atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_and(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_and_explicit(atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_and(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_and_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_and(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_or(atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_or(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_or(volatile atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_or(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_or_explicit(atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_or(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_or_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_or(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_xor(atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_xor(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_xor(volatile atomic<Integral>* atom, Integral arg)
    {
        return atom->fetch_xor(arg);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_xor_explicit(atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_xor(arg, order);
    }

    template <class Integral, class>
    inline Integral atomic_fetch_xor_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order)
    {
        return atom->fetch_xor(arg, order);
    }

    inline bool atomic_flag_test_and_set(volatile atomic_flag* atom)
    {
        return atom->test_and_set();
    }

    inline bool atomic_flag_test_and_set_explicit(volatile atomic_flag* atom, memory_order order)
    {
        return atom->test_and_set(order);
    }

    inline void atomic_flag_clear(volatile atomic_flag* atom)
    {
        return atom->clear();
    }

    inline void atomic_flag_clear_explicit(volatile atomic_flag* atom, memory_order order)
    {
        return atom->clear(order);
    }
}

