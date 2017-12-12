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
#ifndef AZSTD_TYPE_TRAITS_IS_VOID_INCLUDED
#define AZSTD_TYPE_TRAITS_IS_VOID_INCLUDED

#include <AzCore/std/typetraits/bool_trait_def.h>

namespace AZStd
{
    //* is a type T void - is_void<T>
    AZSTD_TYPE_TRAIT_BOOL_DEF1(is_void, T, false)
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(is_void, void, true)
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(is_void, void const, true)
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(is_void, void volatile, true)
    AZSTD_TYPE_TRAIT_BOOL_SPEC1(is_void, void const volatile, true)
}

#endif // AZSTD_TYPE_TRAITS_IS_VOID_INCLUDED
#pragma once