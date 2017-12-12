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
#ifndef AZSTD_TYPE_TRAITS_RANK_INCLUDED
#define AZSTD_TYPE_TRAITS_RANK_INCLUDED

#include <AzCore/std/typetraits/size_t_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <class T, AZStd::size_t N>
        struct rank_imp
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = N);
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct rank_imp<T[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct rank_imp<T const[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct rank_imp<T volatile[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct rank_imp<T const volatile[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };

        template <class T, AZStd::size_t N>
        struct rank_imp<T[], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };
        template <class T, AZStd::size_t N>
        struct rank_imp<T const[], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };
        template <class T, AZStd::size_t N>
        struct rank_imp<T volatile[], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };
        template <class T, AZStd::size_t N>
        struct rank_imp<T const volatile[], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::rank_imp<T, N + 1>::value));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(rank, T, (::AZStd::Internal::rank_imp<T, 0>::value))
}
#endif // AZSTD_TYPE_TRAITS_RANK_INCLUDED
#pragma once