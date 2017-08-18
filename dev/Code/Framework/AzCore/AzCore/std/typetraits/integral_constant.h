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
#ifndef AZSTD_TYPE_TRAITS_INTEGRAL_CONSTANT_H
#define AZSTD_TYPE_TRAITS_INTEGRAL_CONSTANT_H 1

#include <AzCore/std/typetraits/config.h>

#define AZSTD_STATIC_CONSTANT(_Type, _Assignment)   static const _Type _Assignment

namespace AZStd
{
    using std::integral_constant;
    using std::true_type;
    using std::false_type;
}

#endif // AZSTD_TYPE_TRAITS_INTEGRAL_CONSTANT_H
#pragma once