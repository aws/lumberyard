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
#ifndef AZSTD_TYPE_TRAITS_IS_REFERENCE_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_REFERENCE_INCLUDED

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_reference, T, false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_1(typename T, is_reference, T &, true)

    template<typename T>
    constexpr bool is_reference_v = is_reference<T>::value;
}

#endif // AZSTD_TYPE_TRAITS_IS_REFERENCE_INCLUDED
#pragma once
