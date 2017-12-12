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

// Description : Declares the types and functions that implement the
//               OpenGL rendering functionality of DXGL

#ifndef __GLCOMMON__
#define __GLCOMMON__

#include "GLPlatform.hpp"
#import <sys/utsname.h>
#import <Foundation/Foundation.h>

#ifdef  _DEBUG
#   define DXMETAL_DEBUG_PIPELINE
#   define DXMETAL_DEBUG_SHADER_COMPILER
#endif // _DEBUG

#ifdef DXMETAL_DEBUG_PIPELINE
#   define LOG_METAL_PIPELINE_ERRORS(...)               NSLog(__VA_ARGS__)
#else
#   define LOG_METAL_PIPELINE_ERRORS(...)
#endif // DXMETAL_DEBUG_PIPELINE

#ifdef DXMETAL_DEBUG_SHADER_COMPILER
#   define LOG_METAL_PIPELINE_ERRORS(...)               NSLog(__VA_ARGS__)
#   define LOG_METAL_SHADER_ERRORS(...)                 NSLog(__VA_ARGS__)
// Dump shader source every time a new one is compiled
#   define LOG_METAL_SHADER_SOURCE(...)                 NSLog(__VA_ARGS__)
// Dump verbouse shader inputs information during pipeline creation
#   define LOG_METAL_SHADER_REFLECTION_VALIDATION(...)  NSLog(__VA_ARGS__)
#else
#   define LOG_METAL_SHADER_ERRORS(...)
#   define LOG_METAL_SHADER_SOURCE(...)
#   define LOG_METAL_SHADER_REFLECTION_VALIDATION(...)
#endif // DXMETAL_DEBUG_SHADER_COMPILER

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
DXMETAL_TODO("consider this floating around. Those defines define some points to modify for the future extensions.")
//  Is implemented for GL ES 3.1+
#define DXGL_SUPPORT_MULTISAMPLING                  0
#define DXGL_SUPPORT_MULTISAMPLED_TEXTURES          0
#define DXGL_SUPPORT_COMPUTE                        1
#define DXGL_SUPPORT_SHADER_STORAGE_BLOCKS          1
//  Is implemented for GL 4.1+
#define DXGL_SUPPORT_TEXTURE_BUFFERS                1

#define DXMETAL_MAX_ENTRIES_BUFFER_ARG_TABLE        31
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace NCryMetal
{
    inline bool SupportTextureViews()
    {
        return true;
    }

    inline bool SupportsStencilTextures()
    {
        return true;
    }
    
    inline int GetAvailableMRTbpp()
    {
        static int ret = 0;
        
        if (ret == 0)
        {
            // Figure out availble MRT bpp
            // https://developer.apple.com/library/mac/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/MetalFeatureSetTables/MetalFeatureSetTables.html
            // "16 bytes per pixel in iOS GPU Family 1; 32 bytes per pixel in iOS GPU Family 2 and 3"
            // There's no way to get GPU family via code, we have to deduct it from the device name.
            // A7 is family 1 = 128bpp
            // A8 is family 2 = 256bpp
            // A9 is family 3 = 256bpp
            // We only check for devices that support Metal which are shown in the link above on Table 2-1.
            struct utsname sysInfo;
            uname(&sysInfo);
            NSString* deviceCode = [NSString stringWithCString:sysInfo.machine encoding:NSUTF8StringEncoding];
            
            // A7 GPUs only have 128bpp available
            if ([deviceCode rangeOfString:@"iPhone6"].location != NSNotFound || // iPhone 5s
                [deviceCode rangeOfString:@"iPad4"].location != NSNotFound)  // iPad Air, iPad Mini 2, iPad Mini 3
            {
                ret = 128;
            }
            else // Assume newer hardware
            {
                ret = 256;
            }
        }
        
        return ret;
    }
    
    static bool s_isIosMinVersion9_0 = false;
    static bool s_isOsxMinVersion10_11 = false;

    //Cache the OS version which can then be used to query if certain API calls are enabled/disabled.
    inline void CacheMinOSVersionInfo()
    {
#if defined(AZ_PLATFORM_APPLE_OSX)
        s_isOsxMinVersion10_11 = [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){10, 11, 0}];
#elif defined(AZ_PLATFORM_APPLE_IOS)
        s_isIosMinVersion9_0 = [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){9, 0, 0}];
#endif
    }
}

enum
{
    DXGL_NUM_SUPPORTED_VIEWPORTS = 1,
    DXGL_NUM_SUPPORTED_SCISSOR_RECTS = 1,
};


////////////////////////////////////////////////////////////////////////////
// Conform extension definitions
////////////////////////////////////////////////////////////////////////////

namespace NCryMetal
{
    typedef void* TWindowContext;
}

#endif //__GLCOMMON__

