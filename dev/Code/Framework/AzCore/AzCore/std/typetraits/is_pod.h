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
#ifndef AZSTD_TYPE_TRAITS_IS_POD_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_POD_INCLUDED

#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/is_scalar.h>
#include <AzCore/std/typetraits/internal/ice_or.h>
#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_pod_impl
        {
            AZSTD_STATIC_CONSTANT(
                bool, value =
                    (::AZStd::type_traits::ice_or<
                             ::AZStd::is_scalar<T>::value,
                             ::AZStd::is_void<T>::value,
                         AZSTD_IS_POD(T)
                         >::value));
        };

        template <typename T, AZStd::size_t sz>
        struct is_pod_impl<T[sz]>
            : public is_pod_impl<T>
        {};
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_pod, T, ::AZStd::Internal::is_pod_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_POD_INCLUDED
#pragma once
