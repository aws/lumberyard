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

#include <AzCore/std/parallel/config.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_integral.h>

// Pre-VS2015.2 std::atomics have broken alignment and are not binary compatible
// so we've kept our implementation until we can ensure that there are no libs or
// users on pre VS2015.2. See <atomic> for more info.
#if !defined(AZ_COMPILER_MSVC) || AZ_COMPILER_MSVC > 1900
#define AZ_USE_STD_ATOMIC
#include <atomic>
#endif

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
    using std::memory_order;

    typedef std::atomic<char>       atomic_char;
    typedef std::atomic<unsigned char>  atomic_uchar;
    typedef std::atomic<wchar_t>        atomic_wchar_t;
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

    using std::atomic_thread_fence;
    using std::atomic_signal_fence;
    using std::kill_dependency;
}

///Specifies if integral types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_INTEGRAL_LOCK_FREE ATOMIC_INTEGRAL_LOCK_FREE
///Specifies if address types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_ADDRESS_LOCK_FREE ATOMIC_ADDRESS_LOCK_FREE
#define AZ_ATOMIC_BOOL_LOCK_FREE ATOMIC_BOOL_LOCK_FREE
#define AZ_ATOMIC_CHAR_LOCK_FREE ATOMIC_CHAR_LOCK_FREE
#define AZ_ATOMIC_CHAR16_T_LOCK_FREE ATOMIC_CHAR16_T_LOCK_FREE
#define AZ_ATOMIC_CHAR32_T_LOCK_FREE ATOMIC_CHAR32_T_LOCK_FREE
#define AZ_ATOMIC_WCHAR_T_LOCK_FREE ATOMIC_WCHAR_T_LOCK_FREE
#define AZ_ATOMIC_SHORT_LOCK_FREE ATOMIC_SHORT_LOCK_FREE
#define AZ_ATOMIC_INT_LOCK_FREE ATOMIC_INT_LOCK_FREE
#define AZ_ATOMIC_LONG_LOCK_FREE ATOMIC_LONG_LOCK_FREE
#define AZ_ATOMIC_LLONG_LOCK_FREE ATOMIC_LLONG_LOCK_FREE
#define AZ_ATOMIC_POINTER_LOCK_FREE ATOMIC_POINTER_LOCK_FREE
#define AZ_ATOMIC_FLAG_INIT ATOMIC_FLAG_INIT
#define AZ_ATOMIC_VAR_INIT ATOMIC_VAR_INIT

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

    template <typename T>
    struct atomic;

    template <typename T>
    struct atomic<T*>;

    struct atomic_bool;
    struct atomic_flag;

    void atomic_thread_fence(memory_order order);
    void atomic_signal_fence(memory_order order);

    template <typename T>
    T kill_dependency(T y);
}

#if defined(AZ_PLATFORM_WINDOWS)
///Specifies if integral types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_INTEGRAL_LOCK_FREE 2
///Specifies if address types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_ADDRESS_LOCK_FREE 2
#define AZ_ATOMIC_BOOL_LOCK_FREE 2
#define AZ_ATOMIC_CHAR_LOCK_FREE 2
#define AZ_ATOMIC_CHAR16_T_LOCK_FREE 2
#define AZ_ATOMIC_CHAR32_T_LOCK_FREE 2
#define AZ_ATOMIC_WCHAR_T_LOCK_FREE 2
#define AZ_ATOMIC_SHORT_LOCK_FREE 2
#define AZ_ATOMIC_INT_LOCK_FREE 2
#define AZ_ATOMIC_LONG_LOCK_FREE 2
#define AZ_ATOMIC_LLONG_LOCK_FREE 2
#define AZ_ATOMIC_POINTER_LOCK_FREE 2

#define AZ_ATOMICS_LOCK_FREE 2
#else
///Specifies if integral types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_INTEGRAL_LOCK_FREE 0
///Specifies if address types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_ADDRESS_LOCK_FREE 0
#define AZ_ATOMIC_BOOL_LOCK_FREE 0
#define AZ_ATOMIC_CHAR_LOCK_FREE 0
#define AZ_ATOMIC_CHAR16_T_LOCK_FREE 0
#define AZ_ATOMIC_CHAR32_T_LOCK_FREE 0
#define AZ_ATOMIC_WCHAR_T_LOCK_FREE 0
#define AZ_ATOMIC_SHORT_LOCK_FREE 0
#define AZ_ATOMIC_INT_LOCK_FREE 0
#define AZ_ATOMIC_LONG_LOCK_FREE 0
#define AZ_ATOMIC_LLONG_LOCK_FREE 0
#define AZ_ATOMIC_POINTER_LOCK_FREE 0

#define AZ_ATOMICS_LOCK_FREE 0
#endif

#define AZ_ATOMIC_VAR_INIT(val) { val }
#define AZ_ATOMIC_FLAG_INIT { 0 }

#endif // !AZ_USE_STD_ATOMIC

namespace AZStd
{
    void atomic_init(atomic_bool* atom, bool desired);
    void atomic_init(volatile atomic_bool* atom, bool desired);

    template <class T>
    void atomic_init(atomic<T>* atom, T desired);

    template <class T>
    void atomic_init(volatile atomic<T>* atom, T desired);

    template <class T>
    bool atomic_is_lock_free(const atomic<T>* atom);

    template <class T>
    bool atomic_is_lock_free(const volatile atomic<T>* atom);

    template <class T>
    void atomic_store(atomic<T>* atom, T desired);

    template <class T>
    void atomic_store(volatile atomic<T>* atom, T desired);

    template <class T>
    void atomic_store_explicit(atomic<T>* atom, T desired, memory_order order);

    template <class T>
    void atomic_store_explicit(volatile atomic<T>* atom, T desired, memory_order order);

    template <class T>
    T atomic_load(atomic<T>* atom);

    template <class T>
    T atomic_load(volatile atomic<T>* atom);

    template <class T>
    T atomic_load_explicit(atomic<T>* atom, memory_order order);

    template <class T>
    T atomic_load_explicit(volatile atomic<T>* atom, memory_order order);

    template <class T>
    T atomic_exchange(atomic<T>* atom, T desired);

    template <class T>
    T atomic_exchange(volatile atomic<T>* atom, T desired);

    template <class T>
    T atomic_exchange_explicit(atomic<T>* atom, T desired, memory_order order);

    template <class T>
    T atomic_exchange_explicit(volatile atomic<T>* atom, T desired, memory_order order);

    template <class T>
    bool atomic_compare_exchange_weak(atomic<T>* atom, T* expected, T desired);

    template <class T>
    bool atomic_compare_exchange_weak(volatile atomic<T>* atom, T* expected, T desired);

    template <class T>
    bool atomic_compare_exchange_strong(atomic<T>* atom, T* expected, T desired);

    template <class T>
    bool atomic_compare_exchange_strong(volatile atomic<T>* atom, T* expected, T desired);

    template <class T>
    bool atomic_compare_exchange_weak_explicit(atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail);

    template <class T>
    bool atomic_compare_exchange_weak_explicit(volatile atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail);

    template <class T>
    bool atomic_compare_exchange_strong_explicit(atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail);

    template <class T>
    bool atomic_compare_exchange_strong_explicit(volatile atomic<T>* atom, T* expected, T desired, memory_order success, memory_order fail);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_add(atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_add(volatile atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_add_explicit(atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_add_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order);

    template <class T>
    T* atomic_fetch_add(atomic<T*>* atom, ptrdiff_t arg);

    template <class T>
    T* atomic_fetch_add(volatile atomic<T*>* atom, ptrdiff_t arg);

    template <class T>
    T* atomic_fetch_add_explicit(atomic<T*>* atom, ptrdiff_t arg, memory_order order);

    template <class T>
    T* atomic_fetch_add_explicit(volatile atomic<T*>* atom, ptrdiff_t arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_sub(atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_sub(volatile atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_sub_explicit(atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_sub_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order);

    template <class T>
    T* atomic_fetch_sub(atomic<T*>* atom, ptrdiff_t arg);

    template <class T>
    T* atomic_fetch_sub(volatile atomic<T*>* atom, ptrdiff_t arg);

    template <class T>
    T* atomic_fetch_sub_explicit(atomic<T*>* atom, ptrdiff_t arg, memory_order order);

    template <class T>
    T* atomic_fetch_sub_explicit(volatile atomic<T*>* atom, ptrdiff_t arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_and(atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_and(volatile atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_and_explicit(atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_and_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_or(atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_or(volatile atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_or_explicit(atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_or_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_xor(atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_xor(volatile atomic<Integral>* atom, Integral arg);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_xor_explicit(atomic<Integral>* atom, Integral arg, memory_order order);

    template <class Integral, class = AZStd::enable_if_t<AZStd::is_integral<Integral>::value>>
    Integral atomic_fetch_xor_explicit(volatile atomic<Integral>* atom, Integral arg, memory_order order);

    bool atomic_flag_test_and_set(volatile atomic_flag* atom);
    bool atomic_flag_test_and_set_explicit(volatile atomic_flag* atom, memory_order order);
    void atomic_flag_clear(volatile atomic_flag* atom);
    void atomic_flag_clear_explicit(volatile atomic_flag* atom, memory_order order);
}

#include <AzCore/std/parallel/internal/atomic_impl.inl>
