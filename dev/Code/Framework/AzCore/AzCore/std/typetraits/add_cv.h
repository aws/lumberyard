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
#ifndef AZSTD_TYPE_TRAITS_ADD_CV_INCLUDED
#define AZSTD_TYPE_TRAITS_ADD_CV_INCLUDED

#include <AzCore/std/typetraits/type_trait_def.h>

namespace AZStd
{
    // * convert a type T to a const volatile type - add_cv<T>
    AZSTD_TYPE_TRAIT_DEF1(add_cv, T, T const volatile)
    AZSTD_TYPE_TRAIT_PARTIAL_SPEC1_1(typename T, add_cv, T &, T &)
}

#endif // AZSTD_TYPE_TRAITS_ADD_CV_INCLUDED
#pragma once