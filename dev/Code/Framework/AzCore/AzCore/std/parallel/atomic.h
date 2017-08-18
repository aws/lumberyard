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
#ifndef AZSTD_ATOMIC_H
#define AZSTD_ATOMIC_H 1

#include <AzCore/std/parallel/config.h>

#if defined(AZ_PLATFORM_PS4) /*|| defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_XBONE)*/ || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#define AZ_USE_STD_ATOMIC
#include <atomic>
#endif // AZ_PLATFORM_PS4

#if defined(AZ_USE_STD_ATOMIC)

namespace AZStd
{
    using std::atomic;
    using std::atomic_bool;
    using std::atomic_flag;
    using std::memory_order_acq_rel;
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_relaxed;
    using std::memory_order_consume;
    using std::memory_order_seq_cst;

    typedef std::atomic<char>       atomic_char;
    typedef std::atomic<unsigned char>  atomic_uchar;
    typedef std::atomic<short>      atomic_short;
    typedef std::atomic<unsigned short> atomic_ushort;
    typedef std::atomic<int>        atomic_int;
    typedef std::atomic<unsigned int>   atomic_uint;
    typedef std::atomic<long>       atomic_long;
    typedef std::atomic<unsigned long>  atomic_ulong;
    typedef std::atomic<long long>      atomic_llong;
    typedef std::atomic<unsigned long long> atomic_ullong;
    typedef std::atomic<size_t>     atomic_size_t;
    typedef std::atomic<ptrdiff_t>      atomic_ptrdiff_t;
}
#else // !AZ_USE_STD_ATOMIC
namespace AZStd
{
    /**
     * Explanation of memory_order, taken from C++0X draft, section 29.1:
     *
     * memory_order_relaxed - the operation does not order memory
     * memory_order_release - the operation performs a release operation on the affected memory location, thus making
     *                        regular memory writes visible to other threads through the atomic variable to which it is
     *                        applied
     * memory_order_acquire - the operation performs an acquire operation on the affected memory location, thus making
     *                        regular memory writes in other threads released through the atomic variable to which it
     *                        is applied visible to the current thread
     * memory_order_consume - the operation performs a consume operation on the affected memory location, thus making
     *                        regular memory writes in other threads released through the atomic variable to which it
     *                        is applied visible to the regular memory reads that are dependencies of this consume
     *                        operation.
     * memory_order_acq_rel - the operation has both acquire and release semantics
     * memory_order_seq_cst - the operation has both acquire and release semantics, and, in addition, has
     *                        sequentially-consistent operation ordering
     *
     *
     * Or, I prefer to think of the different semantics as barriers: an acquire prevents memory operations from moving
     * before/earlier than the acquire, a release prevents memory operations from moving after/later than the release.
     * Acquire+release is a full barrier that prevents any movement across it. Sequentially-consistent ordering is
     * even stronger than acquire+release, as acquire+release only imposes an order on the operations in a single
     * thread, whereas sequentially-consistent imposes a total order on all operations in all threads.
     */
    enum memory_order
    {
        memory_order_relaxed,
        memory_order_consume,
        memory_order_acquire,
        memory_order_release,
        memory_order_acq_rel,
        memory_order_seq_cst
    };

    template<typename T>
    T kill_dependency(T y);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_X360) || defined(AZ_PLATFORM_PS3) || defined(AZ_PLATFORM_XBONE) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    ///Specifies if integral types are lock-free, 0=never, 1=sometimes, 2=always
    #define ATOMIC_INTEGRAL_LOCK_FREE 2
    ///Specifies if address types are lock-free, 0=never, 1=sometimes, 2=always
    #define ATOMIC_ADDRESS_LOCK_FREE 2
#else
    ///Specifies if integral types are lock-free, 0=never, 1=sometimes, 2=always
    #define ATOMIC_INTEGRAL_LOCK_FREE 0
    ///Specifies if address types are lock-free, 0=never, 1=sometimes, 2=always
    #define ATOMIC_ADDRESS_LOCK_FREE 0
#endif

    template<typename T>
    struct atomic;

    struct atomic_bool;
    bool atomic_is_lock_free(const volatile atomic_bool*);
    void atomic_store(volatile atomic_bool*, bool);
    void atomic_store_explicit(volatile atomic_bool*, bool, memory_order);
    bool atomic_load(const volatile atomic_bool*);
    bool atomic_load_explicit(const volatile atomic_bool*, memory_order);
    bool atomic_exchange(volatile atomic_bool*, bool);
    bool atomic_exchange_explicit(volatile atomic_bool*, bool, memory_order);
    bool atomic_compare_exchange_weak(volatile atomic_bool*, bool*, bool);
    bool atomic_compare_exchange_strong(volatile atomic_bool*, bool*, bool);
    bool atomic_compare_exchange_weak_explicit(volatile atomic_bool*, bool*, bool, memory_order, memory_order);
    bool atomic_compare_exchange_strong_explicit(volatile atomic_bool*, bool*, bool, memory_order, memory_order);

#define AZSTD_ATOMIC_INTEGRAL_DECLARE(_name, _type)                                       \
    struct atomic_##_name;                                                                \
    bool atomic_is_lock_free(const volatile atomic_##_name*);                             \
    void atomic_store(volatile atomic_##_name*, _type);                                   \
    void atomic_store_explicit(volatile atomic_##_name*, _type, memory_order);            \
    _type atomic_load(const volatile atomic_##_name*);                                    \
    _type atomic_load_explicit(const volatile atomic_##_name*, memory_order);             \
    _type atomic_exchange(volatile atomic_##_name*, _type);                               \
    _type atomic_exchange_explicit(volatile atomic_##_name*, _type, memory_order);        \
    bool atomic_compare_exchange_weak(volatile atomic_##_name*, _type*, _type);           \
    bool atomic_compare_exchange_strong(volatile atomic_##_name*, _type*, _type);         \
    bool atomic_compare_exchange_weak_explicit(volatile atomic_##_name*, _type*, _type,   \
        memory_order, memory_order);                                                      \
    bool atomic_compare_exchange_strong_explicit(volatile atomic_##_name*, _type*, _type, \
        memory_order, memory_order);                                                      \
    _type atomic_fetch_add(volatile atomic_##_name*, _type);                              \
    _type atomic_fetch_add_explicit(volatile atomic_##_name*, _type, memory_order);       \
    _type atomic_fetch_sub(volatile atomic_##_name*, _type);                              \
    _type atomic_fetch_sub_explicit(volatile atomic_##_name*, _type, memory_order);       \
    _type atomic_fetch_and(volatile atomic_##_name*, _type);                              \
    _type atomic_fetch_and_explicit(volatile atomic_##_name*, _type, memory_order);       \
    _type atomic_fetch_or(volatile atomic_##_name*, _type);                               \
    _type atomic_fetch_or_explicit(volatile atomic_##_name*, _type, memory_order);        \
    _type atomic_fetch_xor(volatile atomic_##_name*, _type);                              \
    _type atomic_fetch_xor_explicit(volatile atomic_##_name*, _type, memory_order);

#define AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(_name, _type) \
    AZSTD_ATOMIC_INTEGRAL_DECLARE(_name, _type)                  \
    template<>                                                   \
    struct atomic<_type>;

    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(char, char);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(uchar, unsigned char);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(short, short);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(ushort, unsigned short);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(int, int);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(uint, unsigned int);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(long, long);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(ulong, unsigned long);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(llong, long long);
    AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(ullong, unsigned long long);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(char16_t, char16_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(char32_t, char32_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE_WITH_GENERIC(wchar_t, wchar_t);

    //the following are typedefs not types, so they do not get a generic specialization
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_least8_t, int_least8_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_least8_t, uint_least8_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_least16_t, int_least16_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_least16_t, uint_least16_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_least32_t, int_least32_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_least32_t, uint_least32_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_least64_t, int_least64_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_least64_t, uint_least64_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_fast8_t, int_fast8_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_fast8_t, uint_fast8_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_fast16_t, int_fast16_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_fast16_t, uint_fast16_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_fast32_t, int_fast32_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_fast32_t, uint_fast32_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(int_fast64_t, int_fast64_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uint_fast64_t, uint_fast64_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(intptr_t, intptr_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uintptr_t, uintptr_t);
    AZSTD_ATOMIC_INTEGRAL_DECLARE(size_t, size_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(ssize_t, ssize_t);
    AZSTD_ATOMIC_INTEGRAL_DECLARE(ptrdiff_t, ptrdiff_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(intmax_t, intmax_t);
    //AZSTD_ATOMIC_INTEGRAL_DECLARE(uintmax_t, uintmax_t);

#undef AZSTD_ATOMIC_INTEGRAL_DECLARE

    struct atomic_address;
    bool atomic_is_lock_free(const volatile atomic_address*);
    void atomic_store(volatile void**, void*);
    void atomic_store_explicit(volatile atomic_address*, void*, memory_order);
    void* atomic_load(const volatile atomic_address*);
    void* atomic_load_explicit(const volatile atomic_address*, memory_order);
    void* atomic_exchange(volatile atomic_address*, void*);
    void* atomic_exchange_explicit(volatile atomic_address*, void*, memory_order);
    bool atomic_compare_exchange_weak(volatile atomic_address*, void**, void*);
    bool atomic_compare_exchange_strong(volatile atomic_address*, void**, void*);
    bool atomic_compare_exchange_weak_explicit(volatile atomic_address*, void**, void*, memory_order, memory_order);
    bool atomic_compare_exchange_strong_explicit(volatile atomic_address*, void**, void*, memory_order, memory_order);
    void* atomic_fetch_add(volatile atomic_address*, ptrdiff_t);
    void* atomic_fetch_add_explicit(volatile atomic_address*, ptrdiff_t, memory_order);
    void* atomic_fetch_sub(volatile atomic_address*, ptrdiff_t);
    void* atomic_fetch_sub_explicit(volatile atomic_address*, ptrdiff_t, memory_order);

    template<typename T>
    struct atomic<T*>;

    struct atomic_flag;
    bool atomic_flag_test_and_set(volatile atomic_flag*);
    bool atomic_flag_test_and_set_explicit(volatile atomic_flag*, memory_order);
    void atomic_flag_clear(volatile atomic_flag*);
    void atomic_flag_clear_explicit(volatile atomic_flag*, memory_order);

    void atomic_thread_fence(memory_order order);
    void atomic_signal_fence(memory_order order);
}

#include <AzCore/std/parallel/internal/atomic_impl.inl>

#endif // !AZ_USE_STD_ATOMIC

#endif // AZSTD_ATOMIC_H
#pragma once