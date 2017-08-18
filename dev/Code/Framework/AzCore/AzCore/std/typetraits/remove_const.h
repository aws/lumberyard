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
#ifndef AZSTD_TYPE_TRAITS_REMOVE_CONST_INCLUDED
#define AZSTD_TYPE_TRAITS_REMOVE_CONST_INCLUDED

#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/typetraits/internal/cv_traits_impl.h>

#include <AzCore/std/typetraits/type_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T, bool is_vol>
        struct remove_const_helper
        {
            typedef T type;
        };

        template <typename T>
        struct remove_const_helper<T, true>
        {
            typedef T volatile type;
        };


        template <typename T>
        struct remove_const_impl
        {
            typedef typename remove_const_helper<
                typename cv_traits_imp<T*>::unqualified_type
                , ::AZStd::is_volatile<T>::value
                >::type type;
        };
    }

    // * convert a type T to non-const type - remove_const<T>
    AZSTD_TYPE_TRAIT_DEF1(remove_const, T, typename AZStd::Internal::remove_const_impl<T>::type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_const, T &, T &)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_const, T const[N], T type[N])
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_const, T const volatile[N], T volatile type[N])
}

#endif // AZSTD_TYPE_TRAITS_REMOVE_CONST_INCLUDED
#pragma once