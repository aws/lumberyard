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

//This define specifies to use simd instructions on platforms which support them. Remove this define and the
// math will use floats only.
#define AZ_ALLOW_SIMD 1
//declare 2 macros, AZ_SIMD which indicates we are using SIMD instructions of some kind, and AZ_TRAIT_USE_PLATFORM_SIMD to specify we
// are using SIMD on the specified platform.
#if defined(AZ_ALLOW_SIMD)
    #if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/MathTypes_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/MathTypes_h_provo.inl"
    #endif
    #elif defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE_OSX)
        #define AZ_TRAIT_USE_PLATFORM_SIMD 1
    #endif

    #if AZ_TRAIT_USE_PLATFORM_SIMD
        #define AZ_SIMD 1
    #endif
#endif

//now include platform specific types
#if AZ_TRAIT_USE_PLATFORM_SIMD
    #include <AzCore/Math/Internal/MathTypesWin32.inl>
#else
    #include <AzCore/Math/Internal/MathTypesFpu.inl>
#endif
