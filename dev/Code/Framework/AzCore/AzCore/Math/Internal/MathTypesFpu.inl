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

#include <float.h>
#include <math.h>

#define AZ_FLT_MAX          FLT_MAX
#define AZ_FLT_EPSILON      FLT_EPSILON

namespace AZ
{
    //define our simd type. Should not be used in FPU mode, but declared anyway to reduce #ifdef's in class declarations.
    struct SimdVectorType { };  //using a struct to make sure we don't get any unexpected conversions

    AZ_MATH_FORCE_INLINE bool IsFiniteFloat(float x)
    {
#if defined(AZ_PLATFORM_ANDROID)
        // on android cmath undefines isfinite
        return __isfinitef(x) != 0;
#else
        return (isfinite(x) != 0);
#endif
    }
}
