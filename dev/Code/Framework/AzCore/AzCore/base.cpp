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

#include <AzCore/base.h>

namespace AZ
{
    const char* GetPlatformName(PlatformID platform)
    {
        switch (platform)
        {
        case PLATFORM_WINDOWS_32:
            return "Win32";
        case PLATFORM_WINDOWS_64:
            return "Win64";
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        case PLATFORM_##PUBLICNAME:\
            return #PublicName;
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
            AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
            AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        case PLATFORM_LINUX_64:
            return "Linux";
        case PLATFORM_ANDROID:
            return "Android";
        case PLATFORM_ANDROID_64:
            return "Android64";
        case PLATFORM_APPLE_IOS:
            return "iOS";
        case PLATFORM_APPLE_OSX:
            return "OSX";
        case PLATFORM_APPLE_TV:
            return "AppleTV";
        default:
            AZ_Assert(false, "Platform %u is unknown.", static_cast<u32>(platform));
            return "";
        }
    }
} // namespace AZ
