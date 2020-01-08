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
#include <atomic>

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

    using std::atomic_init;
    using std::atomic_is_lock_free;
    using std::atomic_store;
    using std::atomic_store_explicit;
    using std::atomic_load;
    using std::atomic_load_explicit;
    using std::atomic_exchange;
    using std::atomic_exchange_explicit;
    using std::atomic_compare_exchange_weak;
    using std::atomic_compare_exchange_weak_explicit;
    using std::atomic_compare_exchange_strong;
    using std::atomic_compare_exchange_strong_explicit;
    using std::atomic_fetch_add;
    using std::atomic_fetch_add_explicit;
    using std::atomic_fetch_sub;
    using std::atomic_fetch_sub_explicit;
    using std::atomic_fetch_and;
    using std::atomic_fetch_and_explicit;
    using std::atomic_fetch_or;
    using std::atomic_fetch_or_explicit;
    using std::atomic_fetch_xor;
    using std::atomic_fetch_xor_explicit;
    using std::atomic_flag_test_and_set;
    using std::atomic_flag_test_and_set_explicit;
    using std::atomic_flag_clear;
    using std::atomic_flag_clear_explicit;
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
