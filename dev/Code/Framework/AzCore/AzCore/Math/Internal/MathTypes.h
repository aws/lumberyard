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
#ifndef AZCORE_MATH_INTERNAL_MATHTYPES_H
#define AZCORE_MATH_INTERNAL_MATHTYPES_H 1

#include <AzCore/base.h>

//This define specifies to use simd instructions on platforms which support them. Remove this define and the
// math will use floats only.
#define AZ_ALLOW_SIMD 1
//declare 2 macros, AZ_SIMD which indicates we are using SIMD instructions of some kind, and AZ_SIMD_* to specify we
// are using SIMD on the specified platform.
#if defined(AZ_ALLOW_SIMD)
    #if defined(AZ_PLATFORM_WINDOWS)
        #define AZ_SIMD 1
//#define AZ_AVX 1 // TODO optimize for AVX
        #define AZ_SIMD_WINDOWS 1
    #elif defined(AZ_PLATFORM_LINUX)
        #define AZ_SIMD 1
//#define AZ_AVX
        #define AZ_SIMD_LINUX 1
    #elif defined(AZ_PLATFORM_ANDROID)
// TODO: Add SIMD_SUPPORT
    #elif defined(AZ_PLATFORM_APPLE_OSX)
        #define AZ_SIMD 1
//#define AZ_AVX
        #define AZ_SIMD_APPLE_OSX 1

    #endif
#endif

//now include platform specific types
#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/MathTypesWin32.inl>
#elif defined(AZ_SIMD_WII)
    #include <AzCore/Math/Internal/MathTypesWii.inl>
#else
    #include <AzCore/Math/Internal/MathTypesFpu.inl>
#endif

#endif // AZCORE_MATH_INTERNAL_MATHTYPES_H
#pragma once