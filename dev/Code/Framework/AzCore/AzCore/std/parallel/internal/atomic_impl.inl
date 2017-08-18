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
#ifndef AZSTD_PARALLEL_ATOMIC_IMPL_INL
#define AZSTD_PARALLEL_ATOMIC_IMPL_INL 1

#include <AzCore/std/base.h>

#if defined(AZ_PLATFORM_WINDOWS)
    #include <AzCore/std/parallel/internal/atomic_impl_x86_x64.inl>
    #if defined(AZ_PLATFORM_WINDOWS_X64)
        #include <AzCore/std/parallel/internal/atomic_impl_x64.inl>
    #else
        #include <AzCore/std/parallel/internal/atomic_impl_x86.inl>
    #endif
#elif defined(AZ_PLATFORM_X360)
    #include <AzCore/std/parallel/internal/atomic_impl_x360.inl>
#else
    #include <AzCore/std/parallel/internal/atomic_impl_locks.inl>
#endif

namespace AZStd
{
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

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_PS4)
            bool is_lock_free() const volatile { return true; }
#else
            bool is_lock_free() const volatile { return false; }
#endif
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
        atomic_bool() { }
        atomic_bool(bool v)
            : internal::atomic_base<bool>(v) { }
        bool operator=(bool v) volatile { store(v); return v; }
    private:
        atomic_bool(const atomic_bool&);
        atomic_bool& operator=(const atomic_bool& v) volatile;
    };

    /**
     * Address atomic type, acts like a void*
     */
    struct atomic_address
        : public internal::atomic_base<void*>
    {
        atomic_address() { }
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

    private:
        atomic_address(const atomic_address&);
        atomic_address& operator=(const atomic_address& v) volatile;
    };

#define AZSTD_ATOMIC_INTEGRAL_DEFINE(_name, _type)                   \
    struct atomic_##_name                                            \
        : public internal::atomic_integral<_type>                    \
    {                                                                \
        atomic_##_name() { }                                         \
        atomic_##_name(_type v)                                      \
            : internal::atomic_integral<_type>(v) { }                \
        _type operator=(_type v) volatile { store(v); return v; }    \
    private:                                                         \
        atomic_##_name(const atomic_##_name &);                      \
        atomic_##_name& operator=(const atomic_##_name& v) volatile; \
    };

#define AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(_name, _type)   \
    AZSTD_ATOMIC_INTEGRAL_DEFINE(_name, _type)                    \
    template<>                                                    \
    struct atomic<_type>                                          \
        : public atomic_##_name                                   \
    {                                                             \
        atomic() { }                                              \
        atomic(_type v)                                           \
            : atomic_##_name(v) { }                               \
        _type operator=(_type v) volatile { store(v); return v; } \
    private:                                                      \
        atomic(const atomic&);                                    \
        atomic& operator=(const atomic& v) volatile;              \
    };

    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(char, char);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(schar, signed char);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(uchar, unsigned char);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(short, short);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(ushort, unsigned short);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(int, int);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(uint, unsigned int);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(long, long);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(ulong, unsigned long);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(llong, AZ::s64);
    AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(ullong, AZ::u64);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(char16_t, char16_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(char32_t, char32_t);
#if defined(AZ_PLATFORM_X360)
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(wchar_t, wchar_t);
#else
    //AZSTD_ATOMIC_INTEGRAL_DEFINE_WITH_GENERIC(wchar_t, wchar_t);
#endif

    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_least8_t, int_least8_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_least8_t, uint_least8_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_least16_t, int_least16_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_least16_t, uint_least16_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_least32_t, int_least32_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_least32_t, uint_least32_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_least64_t, int_least64_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_least64_t, uint_least64_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_fast8_t, int_fast8_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_fast8_t, uint_fast8_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_fast16_t, int_fast16_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_fast16_t, uint_fast16_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_fast32_t, int_fast32_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_fast32_t, uint_fast32_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(int_fast64_t, int_fast64_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uint_fast64_t, uint_fast64_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(intptr_t, intptr_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uintptr_t, uintptr_t);
    AZSTD_ATOMIC_INTEGRAL_DEFINE(size_t, size_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(ssize_t, ssize_t);
    AZSTD_ATOMIC_INTEGRAL_DEFINE(ptrdiff_t, ptrdiff_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(intmax_t, intmax_t);
    //AZSTD_ATOMIC_INTEGRAL_DEFINE(uintmax_t, uintmax_t);

#undef AZSTD_ATOMIC_INTEGRAL_DEFINE


    /**
     * Generic atomic type, will work with any type having a supported size.
     */
    template<typename T>
    struct atomic
        : public internal::atomic_base<T>
    {
        atomic() { }
        atomic(T v)
            : internal::atomic_base<T>(v) { }
        T operator=(T v) volatile { this->store(v); return v; }
    private:
        atomic(const atomic&);
        atomic& operator=(const atomic&) volatile;
    };

    /**
     * Specialization of generic atomic type for pointers, all arithmetic is in units of the size of the pointed type.
     * Derives from atomic_address, which is consistent for conversions, but means we must re-implement operations
     * to work with generic T* type instead of void* type.
     */
    template<typename T>
    struct atomic<T*>
        : public atomic_address
    {
        atomic() { }
        atomic(T* v)
            : atomic_address(v) { }
        T* operator=(T* v) volatile { store(v); return v; }

        void store(T* v, memory_order order = memory_order_seq_cst) volatile { atomic_address::store(v, order); }
        T* load(memory_order order = memory_order_seq_cst) const volatile
        {
            return static_cast<T*>(atomic_address::load(order));
        }
        operator T*() const volatile {
            return load();
        }
        T* exchange(T* v, memory_order order = memory_order_seq_cst) volatile
        {
            return static_cast<T*>(atomic_address::exchange(v, order));
        }
        bool compare_exchange_weak(T*& expected, T* desired, memory_order success, memory_order failure) volatile
        {
            return atomic_address::compare_exchange_weak(reinterpret_cast<void*&>(expected), desired,
                success, failure);
        }
        bool compare_exchange_strong(T*& expected, T* desired, memory_order success, memory_order failure) volatile
        {
            return atomic_address::compare_exchange_strong(reinterpret_cast<void*&>(expected), desired,
                success, failure);
        }
        bool compare_exchange_weak(T*& expected, T* desired, memory_order order = memory_order_seq_cst) volatile
        {
            return atomic_address::compare_exchange_weak(reinterpret_cast<void*&>(expected), desired, order);
        }
        bool compare_exchange_strong(T*& expected, T* desired, memory_order order = memory_order_seq_cst) volatile
        {
            return atomic_address::compare_exchange_strong(reinterpret_cast<void*&>(expected), desired, order);
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

    private:
        atomic(const atomic&);
        atomic& operator=(const atomic&) volatile;
    };

    /**
     * Basic atomic flag type, provides test-and-set functionality only, this is the only type required to be
     * lock-free by the standard. Note that our default locking implementation for platforms without native atomics
     * does not bother to make this lock-free.
     */
    struct atomic_flag
    {
        atomic_flag() { }
        bool test_and_set(memory_order order = memory_order_seq_cst) volatile { return m_flag.exchange(true, order); }
        void clear(memory_order order = memory_order_seq_cst) volatile { m_flag.store(false, order); }
    private:
        atomic_flag(const atomic_flag&);
        atomic_flag& operator=(const atomic_flag&);

        atomic_bool m_flag;
    };
}

#endif
#pragma once
