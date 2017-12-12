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
#ifndef AZSTD_TYPE_TRAITS_EXTENT_INCLUDED
#define AZSTD_TYPE_TRAITS_EXTENT_INCLUDED

#include <AzCore/std/typetraits/type_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <class T, AZStd::size_t N>
        struct extent_imp
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = 0);
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct extent_imp<T[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::extent_imp<T, N - 1>::value));
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct extent_imp<T const[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::extent_imp<T, N - 1>::value));
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct extent_imp<T volatile[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::extent_imp<T, N - 1>::value));
        };

        template <class T, AZStd::size_t R, AZStd::size_t N>
        struct extent_imp<T const volatile[R], N>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = (::AZStd::Internal::extent_imp<T, N - 1>::value));
        };

        template <class T, AZStd::size_t R>
        struct extent_imp<T[R], 0>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = R);
        };

        template <class T, AZStd::size_t R>
        struct extent_imp<T const[R], 0>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = R);
        };

        template <class T, AZStd::size_t R>
        struct extent_imp<T volatile[R], 0>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = R);
        };

        template <class T, AZStd::size_t R>
        struct extent_imp<T const volatile[R], 0>
        {
            AZSTD_STATIC_CONSTANT(AZStd::size_t, value = R);
        };
    }

    template <class T, AZStd::size_t N = 0>
    struct extent
        : public ::AZStd::integral_constant<AZStd::size_t, ::AZStd::Internal::extent_imp<T, N>::value>
    {};
}

#endif // AZSTD_TYPE_TRAITS_EXTENT_INCLUDED
#pragma once