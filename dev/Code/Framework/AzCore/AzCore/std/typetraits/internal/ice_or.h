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
#ifndef AZSTD_TYPE_TRAITS_INTERNAL_ICE_OR_INCLUDED
#define AZSTD_TYPE_TRAITS_INTERNAL_ICE_OR_INCLUDED

#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    namespace type_traits
    {
        template <bool b1, bool b2, bool b3 = false, bool b4 = false, bool b5 = false, bool b6 = false, bool b7 = false>
        struct ice_or;

        template <bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7>
        struct ice_or
        {
            AZSTD_STATIC_CONSTANT(bool, value = true);
        };

        template <>
        struct ice_or<false, false, false, false, false, false, false>
        {
            AZSTD_STATIC_CONSTANT(bool, value = false);
        };
    }
}

#endif // AZSTD_TYPE_TRAITS_INTERNAL_ICE_OR_INCLUDED
#pragma once