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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_ICE_NOT_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_ICE_NOT_INCLUDED

#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    namespace type_traits
    {
        template <bool b>
        struct ice_not
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };

        template <>
        struct ice_not<true>
        {
            AZSTD_STATIC_CONSTANT(bool, value = false);
        };
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_ICE_NOT_INCLUDED
#pragma once