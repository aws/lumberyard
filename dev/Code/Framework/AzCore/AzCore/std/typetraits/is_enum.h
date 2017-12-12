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
#ifndef AZSTD_TYPE_TRAITS_IS_ENUM_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_ENUM_INCLUDED

#include <AzCore/std/typetraits/intrinsics.h>
#ifndef AZSTD_IS_ENUM
    #include <AzCore/std/typetraits/add_reference.h>
    #include <AzCore/std/typetraits/is_arithmetic.h>
    #include <AzCore/std/typetraits/is_reference.h>
    #include <AzCore/std/typetraits/is_convertible.h>
    #include <AzCore/std/typetraits/is_array.h>
    #ifdef AZ_COMPILER_GCC
        #include <AzCore/std/typetraits/is_function.h>
    #endif
    #if defined(AZSTD_TYPE_TRAITS_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION)
        #include <AzCore/std/typetraits/is_class.h>
        #include <AzCore/std/typetraits/is_union.h>
    #endif
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
#ifndef AZSTD_IS_ENUM
    namespace Internal
    {
    #if defined(AZSTD_TYPE_TRAITS_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION)
        template <typename T>
        struct is_class_or_union
        {
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_class<T>::value
                         , ::AZStd::is_union<T>::value
                         >::value));
        };
    #else
        template <typename T>
        struct is_class_or_union
        {
            template <class U>
            static ::AZStd::type_traits::yes_type is_class_or_union_tester(void(U::*)(void));
            template <class U>
            static ::AZStd::type_traits::no_type is_class_or_union_tester(...);
            AZSTD_STATIC_CONSTANT(bool, value = sizeof(is_class_or_union_tester<T>(0)) == sizeof(::AZStd::type_traits::yes_type));
        };
    #endif

        struct int_convertible
        {
            int_convertible(int);
        };

        // Don't evaluate convertibility to int_convertible unless the type
        // is non-arithmetic. This suppresses warnings with GCC.
        template <bool is_typename_arithmetic_or_reference = true>
        struct is_enum_helper
        {
            template <typename T>
            struct type
            {
                AZSTD_STATIC_CONSTANT(bool, value = false);
            };
        };

        template <>
        struct is_enum_helper<false>
        {
            template <typename T>
            struct type
                : public ::AZStd::is_convertible<AZStd::add_lvalue_reference_t<T>, ::AZStd::Internal::int_convertible>
            {};
        };

        template <typename T>
        struct is_enum_impl
        {
        #if defined(AZ_COMPILER_GCC)
            #ifdef AZSTD_TYPE_TRAITS_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
            // We MUST check for is_class_or_union on conforming compilers in
            // order to correctly deduce that noncopyable types are not enums
            // (dwa 2002/04/15)...
            AZSTD_STATIC_CONSTANT(bool, selector =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_arithmetic<T>::value
                         , ::AZStd::is_reference<T>::value
                         , ::AZStd::is_function<T>::value
                         , is_class_or_union<T>::value
                         , is_array<T>::value
                         >::value));
            #else
            // ...however, not checking is_class_or_union on non-conforming
            // compilers prevents a dependency recursion.
            AZSTD_STATIC_CONSTANT(bool, selector =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_arithmetic<T>::value
                         , ::AZStd::is_reference<T>::value
                         , ::AZStd::is_function<T>::value
                         , is_array<T>::value
                         >::value));
            #endif
        #else // !defined(AZ_COMPILER_GCC):
            AZSTD_STATIC_CONSTANT(bool, selector =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_arithmetic<T>::value
                         , ::AZStd::is_reference<T>::value
                         , is_class_or_union<T>::value
                         , is_array<T>::value
                         >::value));
        #endif

            typedef ::AZStd::Internal::is_enum_helper<selector> se_t;
            typedef typename se_t::template type<T> helper;
            AZSTD_STATIC_CONSTANT(bool, value = helper::value);
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_enum, T, ::AZStd::Internal::is_enum_impl<T>::value)

#else // AZSTD_IS_ENUM
    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_enum, T, AZSTD_IS_ENUM(T))
#endif
}
#endif // AZSTD_TYPE_TRAITS_IS_ENUM_INCLUDED
#pragma once