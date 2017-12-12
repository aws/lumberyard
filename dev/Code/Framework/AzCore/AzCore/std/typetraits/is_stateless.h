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
#ifndef AZSTD_TYPE_TRAITS_IS_STATELESS_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_STATELESS_INCLUDED

#include <AzCore/std/typetraits/has_trivial_constructor.h>
#include <AzCore/std/typetraits/has_trivial_copy.h>
#include <AzCore/std/typetraits/has_trivial_destructor.h>
#include <AzCore/std/typetraits/is_class.h>
#include <AzCore/std/typetraits/is_empty.h>
#include <AzCore/std/typetraits/internal/ice_and.h>

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    namespace Internal
    {
        template <typename T>
        struct is_stateless_impl
        {
            AZSTD_STATIC_CONSTANT(bool, value =
                    (::AZStd::type_traits::ice_and<
                             ::AZStd::has_trivial_constructor<T>::value,
                             ::AZStd::has_trivial_copy<T>::value,
                             ::AZStd::has_trivial_destructor<T>::value,
                             ::AZStd::is_class<T>::value,
                             ::AZStd::is_empty<T>::value
                         >::value));
        };
    } // namespace Internal

    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_stateless, T, ::AZStd::Internal::is_stateless_impl<T>::value)
}
#endif // AZSTD_TYPE_TRAITS_IS_STATELESS_INCLUDED
#pragma once