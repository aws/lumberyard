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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Main header included by every file in Renderer.

#pragma once

#if defined(ENGINE_API)
    #error ENGINE_API should only be defined in this header
#endif

#if AZ_TRAIT_COMPILER_ENABLE_WINDOWS_DLLS
    #define ENGINE_EXPORT_PUBLIC
    #if defined(ENGINE_EXPORTS)
        #define ENGINE_API __declspec(dllexport)
    #else
        #if defined(DEDICATED_SERVER)
            #define ENGINE_API 
        #else
            #define ENGINE_API __declspec(dllimport)
        #endif
    #endif
#elif defined(ANDROID) || defined(APPLE)
    #if defined(ENGINE_EXPORTS)
        #define ENGINE_EXPORT_PUBLIC __attribute__((visibility("default")))
        #define ENGINE_API __attribute__((visibility("default")))
    #else
        #define ENGINE_EXPORT_PUBLIC
        #define ENGINE_API __attribute__((visibility("default")))
    #endif
#else
#define ENGINE_EXPORT_PUBLIC
#define ENGINE_API
#endif


