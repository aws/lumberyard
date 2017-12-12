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
#ifndef AZSTD_TYPE_TRAITS_HAS_TRIVIAL_ASSIGN_INCLUDED
#define AZSTD_TYPE_TRAITS_HAS_TRIVIAL_ASSIGN_INCLUDED

#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/is_pod.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/typetraits/internal/ice_and.h>
#include <AzCore/std/typetraits/internal/ice_or.h>
#include <AzCore/std/typetraits/internal/ice_not.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct has_trivial_assign_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value = (::AZStd::type_traits::ice_and<
                                                         ::AZStd::type_traits::ice_or< ::AZStd::is_pod<T>::value, AZSTD_HAS_TRIVIAL_ASSIGN(T) >::value,
                                                         ::AZStd::type_traits::ice_not< ::AZStd::is_const<T>::value >::value,
                                                         ::AZStd::type_traits::ice_not< ::AZStd::is_volatile<T>::value >::value
                                                     >::value));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(has_trivial_assign, T, ::AZStd::Internal::has_trivial_assign_impl<T>::value)
}
#endif // AZSTD_TYPE_TRAITS_HAS_TRIVIAL_ASSIGN_INCLUDED
#pragma once