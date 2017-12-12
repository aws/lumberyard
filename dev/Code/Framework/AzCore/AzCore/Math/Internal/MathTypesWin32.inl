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

#include <xmmintrin.h>
#include <float.h>
#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
#include <math.h>
#endif

#define AZ_FLT_MAX  FLT_MAX
#define AZ_FLT_EPSILON      FLT_EPSILON

namespace AZ
{
    typedef __m128 SimdVectorType;

    namespace Internal
    {
        static AZ_ALIGN(const float g_simd1111[4], 16) = { 1.0f, 1.0f, 1.0f, 1.0f };
        static AZ_ALIGN(const float g_simd1000[4], 16) = { 1.0f, 0.0f, 0.0f, 0.0f };
        static AZ_ALIGN(const float g_simd0001[4], 16) = { 0.0f, 0.0f, 0.0f, 1.0f };
        static AZ_ALIGN(const float g_sinEstimateCoefficients[4], 16) = { 1.0f, -1.66521856991541e-1f, 8.199913018755e-3f, -1.61475937228e-4f };
        static AZ_ALIGN(const float g_cosEstimateCoefficients[4], 16) = { 1.0f, -4.95348008918096e-1f, 3.878259962881e-2f, -9.24587976263e-4f };
        static AZ_ALIGN(const u32 g_simdAbsMask[4], 16) = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
        static AZ_ALIGN(const u32 g_simdNegateMask[4], 16) = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
        static AZ_ALIGN(const u32 g_simdNegateXMask[4], 16) = { 0x80000000, 0x0, 0x0, 0x0 };
        static AZ_ALIGN(const u32 g_simdNegateYMask[4], 16) = { 0x0, 0x80000000, 0x0, 0x0 };
        static AZ_ALIGN(const u32 g_simdNegateZWMask[4], 16) = { 0x0, 0x0, 0x80000000, 0x80000000 };
        static AZ_ALIGN(const u32 g_simdNegateXYZMask[4], 16) = { 0x80000000, 0x80000000, 0x80000000, 0x0 };
        static AZ_ALIGN(const u32 g_simdWMask[4], 16) = { 0xffffffff, 0xffffffff, 0xffffffff, 0x0 };
    }

    inline bool IsFiniteFloat(float x) { return (isfinite(x) != 0); }
}
