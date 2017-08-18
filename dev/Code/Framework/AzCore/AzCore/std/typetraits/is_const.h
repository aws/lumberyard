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
#ifndef AZSTD_TYPE_TRAITS_IS_CONST_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_CONST_INCLUDED

#include <AzCore/std/typetraits/internal/cv_traits_impl.h>

namespace AZStd
{
    using std::is_const;

    /**
     * Check if a member function type is const
     * Returns false for all non-const member functions
     *
     * Synatx: AZStd::is_method_t_const<decltype(&MyClass::MyMemberFunction)>::value
     */
    template <typename Function>
    struct is_method_t_const
    {
        static const bool value = false;
        using type = false_type;
    };

    template <typename Class, typename ReturnType, typename... Args>
    struct is_method_t_const<ReturnType(Class::*)(Args...) const>
    {
        static const bool value = true;
        using type = true_type;
    };

    // Uncomment when C++17 is supported
#if 0
    template <auto Function>
    using is_method_const = is_method_t_const<decltype(Function)>;

    template <auto Function>
    using is_method_const_t = is_method_t_const<decltype(Function)>::type;

    template <auto Function>
    constexpr bool is_method_const_v = is_method_t_const<decltype(Function)>::value;
#endif
}

#endif // AZSTD_TYPE_TRAITS_IS_CONST_INCLUDED
#pragma once