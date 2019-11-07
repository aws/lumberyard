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
#pragma once

#include <AzCore/base.h>

// math will use floats only.
#define AZ_ALLOW_SIMD 1

//declare 2 macros, AZ_SIMD which indicates we are using SIMD instructions of some kind, and AZ_TRAIT_USE_PLATFORM_SIMD to specify we
// are using SIMD on the specified platform.
#if AZ_ALLOW_SIMD && AZ_TRAIT_USE_PLATFORM_SIMD
#   define AZ_SIMD 1
#endif

#include <float.h>
#include <math.h>

#define AZ_FLT_MAX          FLT_MAX
#define AZ_FLT_EPSILON      FLT_EPSILON

#include <AzCore/Math/Internal/MathTypes_Platform.h>

namespace AZ
{
    inline bool IsFiniteFloat(float x) { return (azisfinite(x) != 0); }
}