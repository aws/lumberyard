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
#ifndef AZSTD_TYPE_TRAITS_REMOVE_VOLATILE_INCLUDED
#define AZSTD_TYPE_TRAITS_REMOVE_VOLATILE_INCLUDED

#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/internal/cv_traits_impl.h>
#include <AzCore/std/typetraits/type_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T, bool is_const>
        struct remove_volatile_helper
        {
            typedef T type;
        };

        template <typename T>
        struct remove_volatile_helper<T, true>
        {
            typedef T const type;
        };

        template <typename T>
        struct remove_volatile_impl
        {
            typedef typename remove_volatile_helper<
                typename cv_traits_imp<T*>::unqualified_type
                , ::AZStd::is_const<T>::value
                >::type type;
        };
    }
    // * convert a type T to a non-volatile type - remove_volatile<T>
    AZSTD_TYPE_TRAIT_DEF1(remove_volatile, T, typename AZStd::Internal::remove_volatile_impl<T>::type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_volatile, T &, T &)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_volatile, T volatile[N], T type[N])
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_volatile, T const volatile[N], T const type[N])
}

#endif // AZSTD_TYPE_TRAITS_REMOVE_VOLATILE_INCLUDED
#pragma once