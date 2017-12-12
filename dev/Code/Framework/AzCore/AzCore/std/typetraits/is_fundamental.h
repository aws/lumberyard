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
#ifndef AZSTD_TYPE_TRAITS_IS_FUNDAMENTAL_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_FUNDAMENTAL_INCLUDED

#include <AzCore/std/typetraits/is_arithmetic.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/internal/ice_or.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_fundamental_impl
            : public ::AZStd::type_traits::ice_or<
                    ::AZStd::is_arithmetic<T>::value
                , ::AZStd::is_void<T>::value
                >
        {};
    }

    //* is a type T a fundamental type described in the standard (3.9.1)
    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_fundamental, T, ::AZStd::Internal::is_fundamental_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_FUNDAMENTAL_INCLUDED
#pragma once