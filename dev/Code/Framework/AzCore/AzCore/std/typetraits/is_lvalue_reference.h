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
#ifndef AZSTD_TYPE_TRAITS_IS_LVALUE_REFERENCE_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_LVALUE_REFERENCE_INCLUDED

#include <AzCore/std/typetraits/bool_trait_def.h>
#include <AzCore/std/typetraits/internal/wrap.h>
#include <AzCore/std/typetraits/internal/yes_no_type.h>

namespace AZStd
{
    namespace Internal {
        template <class T>
        T & (*is_lvalue_reference_helper1(::AZStd::type_traits::wrap<T>))(::AZStd::type_traits::wrap<T>);
        char is_lvalue_reference_helper1(...);

        template <class T>
            ::AZStd::type_traits::no_type is_lvalue_reference_helper2(T & (*)(::AZStd::type_traits::wrap<T>));
        ::AZStd::type_traits::yes_type is_lvalue_reference_helper2(...);

        template <typename T>
        struct is_lvalue_reference_impl
        {
            AZSTD_STATIC_CONSTANT(
                bool, value = sizeof(
                            ::AZStd::Internal::is_lvalue_reference_helper2(
                                ::AZStd::Internal::is_lvalue_reference_helper1(::AZStd::type_traits::wrap<T>()))) == 1
                );
        };

        AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_lvalue_reference, void, false)
        //AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_lvalue_reference,void const,false)
        //AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_lvalue_reference,void volatile,false)
        //AZSTD_TYPE_TRAIT_BOOL_IMPL_SPEC1(is_lvalue_reference,void const volatile,false)
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_lvalue_reference, T, ::AZStd::Internal::is_lvalue_reference_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_LVALUE_REFERENCE_INCLUDED
#pragma once