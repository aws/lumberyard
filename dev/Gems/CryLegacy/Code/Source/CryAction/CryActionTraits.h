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

#include "ProjectDefines.h"

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryActionTraits_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryActionTraits_h_provo.inl"
    #endif
#else
    #define AZ_LEGACY_CRYACTION_TRAIT_SCREENSHOT_EXTENSION "tif"
    #define AZ_LEGACY_CRYACTION_TRAIT_SET_COMPRESSOR_THREAD_PRIORITY 0
    #if defined(WIN32) || defined(LINUX) || defined(APPLE)
        #define AZ_LEGACY_CRYACTION_TRAIT_USE_ZLIB_COMPRESSOR_THREAD 1
    #endif
#endif
