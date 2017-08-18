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
#ifndef AZSTD_TYPE_TRAITS_IS_SCALAR_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_SCALAR_INCLUDED

#include <AzCore/std/typetraits/is_arithmetic.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_member_pointer.h>
#include <AzCore/std/typetraits/internal/ice_or.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_scalar_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_arithmetic<T>::value,
                             ::AZStd::is_enum<T>::value,
                             ::AZStd::is_pointer<T>::value,
                             ::AZStd::is_member_pointer<T>::value
                         >::value));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_scalar, T, ::AZStd::Internal::is_scalar_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_SCALAR_INCLUDED
#pragma once