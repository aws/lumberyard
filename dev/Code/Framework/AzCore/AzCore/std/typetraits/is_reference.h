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

    #if defined(AZ_COMPILER_GCC) && (AZ_COMPILER_GCC < 3)
    // these allow us to work around illegally cv-qualified reference
    // types.
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_1(typename T, is_reference, T const, ::AZStd::is_reference<T>::value)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_1(typename T, is_reference, T volatile, ::AZStd::is_reference<T>::value)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_1(typename T, is_reference, T const volatile, ::AZStd::is_reference<T>::value)
    // However, the above specializations confuse gcc 2.96 unless we also
    // supply these specializations for array types
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_2(typename T, unsigned long N, is_reference, T[N], false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_2(typename T, unsigned long N, is_reference, const T[N], false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_2(typename T, unsigned long N, is_reference, volatile T[N], false)
    AZSTD_TYPE_TRAIT_BOOL_PARTIAL_SPEC1_2(typename T, unsigned long N, is_reference, const volatile T[N], false)
    #endif
}

#endif // AZSTD_TYPE_TRAITS_IS_REFERENCE_INCLUDED
#pragma once