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
#ifndef AZSTD_TYPE_TRAITS_HAS_NOTHROW_ASSIGN_INCLUDED
#define AZSTD_TYPE_TRAITS_HAS_NOTHROW_ASSIGN_INCLUDED

#include <AzCore/std/typetraits/has_trivial_assign.h>
#include <AzCore/std/typetraits/intrinsics.h>

namespace AZStd
{
    namespace Internal
    {
        template <class T>
        struct has_nothrow_assign_imp
        {
            AZSTD_STATIC_CONSTANT(bool, value = (::AZStd::type_traits::ice_or< ::AZStd::has_trivial_assign<T>::value, AZSTD_HAS_NOTHROW_ASSIGN(T)>::value));
        };
    }

    AZSTD_TYPE_TRAIT_BOOL_DEF1(has_nothrow_assign, T, ::AZStd::Internal::has_nothrow_assign_imp<T>::value)
}

#endif // AZSTD_TYPE_TRAITS_HAS_NOTHROW_ASSIGN_INCLUDED
#pragma once