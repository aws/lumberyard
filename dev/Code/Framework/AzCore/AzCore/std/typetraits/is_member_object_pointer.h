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

#ifndef AZSTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_INCLUDED

#include <AzCore/std/typetraits/is_member_pointer.h>
#include <AzCore/std/typetraits/is_member_function_pointer.h>
#include <AzCore/std/typetraits/internal/ice_and.h>
#include <AzCore/std/typetraits/internal/ice_not.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_member_object_pointer_impl
        {
            AZSTD_STATIC_CONSTANT(
                bool, value = (::AZStd::type_traits::ice_and<
                                       ::AZStd::is_member_pointer<T>::value,
                                       ::AZStd::type_traits::ice_not<
                                           ::AZStd::is_member_function_pointer<T>::value
                                       >::value
                                   >::value));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_member_object_pointer, T, ::AZStd::Internal::is_member_object_pointer_impl<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_IS_MEMBER_OBJECT_POINTER_INCLUDED
#pragma once