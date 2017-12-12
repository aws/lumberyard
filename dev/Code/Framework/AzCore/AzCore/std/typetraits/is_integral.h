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
#ifndef AZSTD_TYPE_TRAITS_IS_INTEGRAL_H
#define AZSTD_TYPE_TRAITS_IS_INTEGRAL_H 1

#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    //* is a type T an [cv-qualified-] integral type described in the standard (3.9.1p3)
    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_integral, T, false)

    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, unsigned char, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, unsigned short, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, unsigned int, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, unsigned long, true)

    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, signed char, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, signed short, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, signed int, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, signed long, true)

    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, bool, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, char, true)

    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral,wchar_t,true)

    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, ::AZ::u64, true)
    AZSTD_TYPE_TRAIT_BOOL_CONST_VOLATILE_SPEC1(is_integral, ::AZ::s64, true)
}

#endif // AZSTD_TYPE_TRAITS_IS_INTEGRAL_H
#pragma once