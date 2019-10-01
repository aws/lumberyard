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
#pragma once

#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/void_t.h>

namespace AZStd
{
    template <typename T, typename U, typename = void>
    struct is_swappable_with_helper
        : false_type
    {};

    template <typename T, typename U>
    struct is_swappable_with_helper<T, U, void_t<decltype(swap(AZStd::declval<T>(), AZStd::declval<U>()))>>
        : true_type
    {};
    template <class T, class U>
    struct is_swappable_with
        : integral_constant<bool, conjunction_v<is_swappable_with_helper<T, U>, is_swappable_with_helper<U, T>>>
    {};

    template <class T, class U>
    constexpr bool is_swappable_with_v = is_swappable_with<T, U>::value;

    template <class T>
    struct is_swappable
        : is_swappable_with<T&, T&>
    {};

    template <class T>
    constexpr bool is_swappable_v = is_swappable<T>::value;
}
