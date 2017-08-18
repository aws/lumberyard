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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_ICE_EQ_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_ICE_EQ_INCLUDED

#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    namespace type_traits
    {
        template <int b1, int b2>
        struct ice_eq
        {
            AZSTD_STATIC_CONSTANT(bool, value = (b1 == b2));
        };

        template <int b1, int b2>
        struct ice_ne
        {
            AZSTD_STATIC_CONSTANT(bool, value = (b1 != b2));
        };

        template <int b1, int b2>
        bool const ice_eq<b1, b2>::value;
        template <int b1, int b2>
        bool const ice_ne<b1, b2>::value;
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_ICE_EQ_INCLUDED
#pragma once