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

#include <platform.h>
#include <AzCore/Debug/Trace.h>
#include <ProjectDefines.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define STDAFX_H_SECTION_1 1
#define STDAFX_H_SECTION_2 2
#endif

#if !defined(_RELEASE)
    // Enable logging for non-Release builds.
    #define ENABLE_AUDIO_LOGGING
    // Production code is enabled in non-Release builds.
    #define INCLUDE_AUDIO_IMPL_PRODUCTION_CODE
#endif // !_RELEASE


#if defined(AZ_PLATFORM_WINDOWS)
    #include <windows.h>
    #if defined(AZ_PLATFORM_WINDOWS_X64)
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Windows64
    #else
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Windows32
    #endif
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_1
#include AZ_RESTRICTED_FILE(StdAfx_h, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STDAFX_H_SECTION_2
#include AZ_RESTRICTED_FILE(StdAfx_h, AZ_RESTRICTED_PLATFORM)
#endif

#if defined(AZ_PLATFORM_APPLE_OSX)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Mac
#endif

#if defined(AZ_PLATFORM_APPLE_IOS)
///////////////////////////////////////////////////////////////////////////////////////////////////
// iOS
#endif

#if defined(AZ_PLATFORM_APPLE_TV)
///////////////////////////////////////////////////////////////////////////////////////////////////
// AppleTV
#endif

#if defined(AZ_PLATFORM_ANDROID)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Android
#endif

#if defined(AZ_PLATFORM_LINUX_X64)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Linux64
#endif


#include <AudioAllocators.h>
#include <AudioLogger.h>

namespace Audio
{
    extern CAudioLogger g_audioImplLogger_nosound;
} // namespace Audio

