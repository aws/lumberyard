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
#ifndef AZSTD_TYPE_TRAITS_IS_CONVERTIBLE_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_CONVERTIBLE_INCLUDED

#include <AzCore/std/typetraits/intrinsics.h>
#ifndef AZSTD_IS_CONVERTIBLE
    #include <AzCore/std/typetraits/internal/yes_no_type.h>
    #include <AzCore/std/typetraits/config.h>
    #include <AzCore/std/typetraits/is_array.h>
    #include <AzCore/std/typetraits/add_reference.h>
    #include <AzCore/std/typetraits/ice.h>
    #include <AzCore/std/typetraits/is_arithmetic.h>
    #include <AzCore/std/typetraits/is_void.h>
    #include <AzCore/std/typetraits/is_abstract.h>
#if defined(AZ_COMPILER_MWERKS)
    #include <AzCore/std/typetraits/is_function.h>
    #include <AzCore/std/typetraits/remove_reference.h>
#endif

#endif // AZSTD_IS_CONVERTIBLE

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
#ifndef AZSTD_IS_CONVERTIBLE
    // is one type convertable to another?
    //
    // there are multiple versions of the is_convertible
    // template, almost every compiler seems to require its
    // own version.
    //
    // Thanks to Andrei Alexandrescu for the original version of the
    // conversion detection technique!
    //
    namespace Internal
    {
        #if defined(AZ_COMPILER_GCC)
        // special version for gcc compiler + recent Borland versions
        // note that this does not pass UDT's through (...)
        struct any_conversion
        {
            template <typename T>
            any_conversion(const volatile T&);
            template <typename T>
            any_conversion(T&);
        };

        template <typename T>
        struct checker
        {
            static AZStd::type_traits::no_type _m_check(any_conversion ...);
            static AZStd::type_traits::yes_type _m_check(T, int);
        };

        template <typename From, typename To>
        struct is_convertible_basic_impl
        {
            static From _m_from;
            static bool const value = sizeof(Internal::checker<To>::_m_check(_m_from, 0)) == sizeof(::AZStd::type_traits::yes_type);
        };
        #elif defined(AZ_COMPILER_MWERKS)
        //
        // CW works with the technique implemented above for EDG, except when From
        // is a function type (or a reference to such a type), in which case
        // any_conversion won't be accepted as a valid conversion. We detect this
        // exceptional situation and channel it through an alternative algorithm.
        //

        template <typename From, typename To, bool FromIsFunctionRef>
        struct is_convertible_basic_impl_aux;

        struct any_conversion
        {
            template <typename T>
            any_conversion(const volatile T&);
        };

        template <typename From, typename To>
        struct is_convertible_basic_impl_aux<From, To, false /*FromIsFunctionRef*/>
        {
            static ::AZStd::type_traits::no_type AZSTD_TYPE_TRAITS_DECL _m_check(any_conversion ...);
            static ::AZStd::type_traits::yes_type AZSTD_TYPE_TRAITS_DECL _m_check(To, int);
            static From _m_from;

            AZSTD_STATIC_CONSTANT(bool, value = sizeof(_m_check(_m_from, 0)) == sizeof(::AZStd::type_traits::yes_type));
        };

        template <typename From, typename To>
        struct is_convertible_basic_impl_aux<From, To, true /*FromIsFunctionRef*/>
        {
            static ::AZStd::type_traits::no_type AZSTD_TYPE_TRAITS_DECL _m_check(...);
            static ::AZStd::type_traits::yes_type AZSTD_TYPE_TRAITS_DECL _m_check(To);
            static From _m_from;
            AZSTD_STATIC_CONSTANT(bool, value = sizeof(_m_check(_m_from)) == sizeof(::AZStd::type_traits::yes_type));
        };

        template <typename From, typename To>
        struct is_convertible_basic_impl
            : is_convertible_basic_impl_aux<
                From, To,
                    ::AZStd::is_function<typename ::AZStd::remove_reference<From>::type>::value
                >
        {};
        #else
        //
        // This version seems to work pretty well for a wide spectrum of compilers,
        // however it does rely on undefined behaviour by passing UDT's through (...).
        //
        template <typename From, typename To>
        struct is_convertible_basic_impl
        {
            static ::AZStd::type_traits::no_type AZSTD_TYPE_TRAITS_DECL _m_check(...);
            static ::AZStd::type_traits::yes_type AZSTD_TYPE_TRAITS_DECL _m_check(To);
            static From _m_from;
            #ifdef AZ_COMPILER_MSVC
                #pragma warning(push)
                #pragma warning(disable:4244)
                #if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050000)
                    #pragma warning(disable:6334)
                #endif
            #endif
            AZSTD_STATIC_CONSTANT(bool, value = sizeof(_m_check(_m_from)) == sizeof(::AZStd::type_traits::yes_type));

            #ifdef AZ_COMPILER_MSVC
                #pragma warning(pop)
            #endif
        };
        #endif // is_convertible_impl

        template <typename From, typename To>
        struct is_convertible_impl
        {
            typedef add_lvalue_reference_t<From> ref_type;
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_and<
                             ::AZStd::type_traits::ice_or<
                                 ::AZStd::Internal::is_convertible_basic_impl<ref_type, To>::value,
                                 ::AZStd::is_void<To>::value
                             >::value,
                             ::AZStd::type_traits::ice_not<
                                 ::AZStd::is_array<To>::value
                             >::value
                         >::value)
                );
        };

        template <bool trivial1, bool trivial2, bool abstract_target>
        struct is_convertible_impl_select
        {
            template <class From, class To>
            struct rebind
            {
                typedef is_convertible_impl<From, To> type;
            };
        };

        template <>
        struct is_convertible_impl_select<true, true, false>
        {
            template <class From, class To>
            struct rebind
            {
                typedef true_type type;
            };
        };

        template <>
        struct is_convertible_impl_select<false, false, true>
        {
            template <class From, class To>
            struct rebind
            {
                typedef false_type type;
            };
        };

        template <>
        struct is_convertible_impl_select<true, false, true>
        {
            template <class From, class To>
            struct rebind
            {
                typedef false_type type;
            };
        };

        template <typename From, typename To>
        struct is_convertible_impl_dispatch_base
        {
            typedef is_convertible_impl_select<
                    ::AZStd::is_arithmetic<From>::value,
                    ::AZStd::is_arithmetic<To>::value,
                    ::AZStd::is_abstract<To>::value
                > selector;

            typedef typename selector::template rebind<From, To> isc_binder;
            typedef typename isc_binder::type type;
        };

        template <typename From, typename To>
        struct is_convertible_impl_dispatch
            : public is_convertible_impl_dispatch_base<From, To>::type
        {};

        //
        // Now add the full and partial specializations
        // for void types, these are common to all the
        // implementation above:
        //
        #define TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2_PART1(trait, spec1, spec2, value) \
    AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC2(trait, spec1, spec2, value)                  \
    AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC2(trait, spec1, spec2 const, value)            \
    AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC2(trait, spec1, spec2 volatile, value)         \
    AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC2(trait, spec1, spec2 const volatile, value)   \
    /**/

        #define TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2(trait, spec1, spec2, value)          \
    TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2_PART1(trait, spec1, spec2, value)                \
    TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2_PART1(trait, spec1 const, spec2, value)          \
    TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2_PART1(trait, spec1 volatile, spec2, value)       \
    TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2_PART1(trait, spec1 const volatile, spec2, value) \
    /**/

        TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2(is_convertible, void, void, true)

        #undef TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2
        #undef TT_AUX_BOOL_CV_VOID_TRAIT_SPEC2_PART1

        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename To, is_convertible, void, To, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename From, is_convertible, From, void, true)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename To, is_convertible, void const, To, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename To, is_convertible, void volatile, To, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename To, is_convertible, void const volatile, To, false)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename From, is_convertible, From, void const, true)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename From, is_convertible, From, void volatile, true)
        AZSTD_TYPE_TRAIT_BOOL_IMPL_PARTIAL_SPEC2_1(typename From, is_convertible, From, void const volatile, true)
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF2(is_convertible, From, To, (::AZStd::Internal::is_convertible_impl_dispatch<From, To>::value))

#else
    AZSTD_TYPE_TRAIT_BOOL_DEF2(is_convertible, From, To, AZSTD_IS_CONVERTIBLE(From, To))
#endif
}

#endif // AZSTD_TYPE_TRAITS_IS_CONVERTIBLE_INCLUDED
#pragma once