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

#ifndef AZSTD_TYPE_TRAITS_REMOVE_EXTENT_INCLUDED
#define AZSTD_TYPE_TRAITS_REMOVE_EXTENT_INCLUDED

#include <AzCore/std/typetraits/type_trait_def.h>

namespace AZStd
{
    AZSTD_TYPE_TRAIT_DEF1(remove_extent, T, T)

    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_extent, T[N], T type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_extent, T const[N], T const type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_extent, T volatile[N], T volatile type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_extent, T const volatile[N], T const volatile type)

    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_extent, T[], T)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_extent, T const[], T const)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_extent, T volatile[], T volatile)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_extent, T const volatile[], T const volatile)
}

#endif // AZSTD_TYPE_TRAITS_REMOVE_EXTENT_INCLUDED
#pragma once