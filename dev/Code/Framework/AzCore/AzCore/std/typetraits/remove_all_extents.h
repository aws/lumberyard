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
#ifndef AZSTD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_INCLUDED
#define AZSTD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_INCLUDED

#include <AzCore/std/typetraits/type_trait_def.h>

namespace AZStd
{
    AZSTD_TYPE_TRAIT_DEF1(remove_all_extents, T, T)

    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_all_extents, T[N], typename AZStd::remove_all_extents<T>::type type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_all_extents, T const[N], typename AZStd::remove_all_extents<T const>::type type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_all_extents, T volatile[N], typename AZStd::remove_all_extents<T volatile>::type type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_2(typename T, AZStd::size_t N, remove_all_extents, T const volatile[N], typename AZStd::remove_all_extents<T const volatile>::type type)

    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_all_extents, T[], typename AZStd::remove_all_extents<T>::type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_all_extents, T const[], typename AZStd::remove_all_extents<T const>::type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_all_extents, T volatile[], typename AZStd::remove_all_extents<T volatile>::type)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, remove_all_extents, T const volatile[], typename AZStd::remove_all_extents<T const volatile>::type)
}

#endif // AZSTD_TYPE_TRAITS_REMOVE_ALL_EXTENTS_INCLUDED
#pragma once