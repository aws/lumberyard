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
    #include <AzCore/std/typetraits/is_class.h>
    #include <AzCore/std/typetraits/is_union.h>
#endif

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
#ifndef AZSTD_IS_ENUM
    namespace Internal
    {
        template <typename T>
        struct is_class_or_union
        {
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_class<T>::value
                         , ::AZStd::is_union<T>::value
                         >::value));
        };

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
            AZSTD_STATIC_CONSTANT(bool, selector =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_arithmetic<T>::value
                         , ::AZStd::is_reference<T>::value
                         , is_class_or_union<T>::value
                         , is_array<T>::value
                         >::value));

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
