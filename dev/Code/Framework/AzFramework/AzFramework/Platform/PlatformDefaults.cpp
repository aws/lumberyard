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

#include <Platform/PlatformDefaults.h>
namespace AzFramework
{
    const char* PlatformNames[PlatformId::NumPlatforms] = { PlatformPC, PlatformES3, PlatformIOS, PlatformOSX, PlatformXenia, PlatformProvo, PlatformSalem, PlatformServer };

    PlatformFlags PlatformHelper::GetPlatformFlagFromPlatformIndex(const PlatformId& platformIndex)
    {
        if (platformIndex < 0 || platformIndex > PlatformId::NumPlatforms)
        {
            return PlatformFlags::Platform_NONE;
        }
        return static_cast<PlatformFlags>(1 << platformIndex);
    }

    AZStd::vector<AZStd::string> PlatformHelper::GetPlatforms(const PlatformFlags& platformFlags)
    {
        AZStd::vector<AZStd::string> platforms;
        for (int platformNum = 0; platformNum < PlatformId::NumPlatforms; ++platformNum)
        {
            if ((platformFlags & static_cast<PlatformFlags>(1 << platformNum)) != PlatformFlags::Platform_NONE)
            {
                platforms.push_back(PlatformNames[platformNum]);
            }
        }

        return platforms;
    }

    AZStd::vector<PlatformId> PlatformHelper::GetPlatformIndices(const PlatformFlags& platformFlags)
    {
        AZStd::vector<PlatformId> platformIndices;
        for (int i = 0; i < PlatformId::NumPlatforms; i++)
        {
            PlatformId index = static_cast<PlatformId>(i);
            if ((GetPlatformFlagFromPlatformIndex(index) & platformFlags) != PlatformFlags::Platform_NONE)
            {
                platformIndices.emplace_back(index);
            }
        }
        return platformIndices;
    }

    PlatformFlags PlatformHelper::GetPlatformFlag(const AZStd::string& platform)
    {
        int platformIndex = GetPlatformIndexFromName(platform.c_str());
        if (platformIndex == PlatformId::Invalid)
        {
            AZ_Error("PlatformDefault", false, "Invalid Platform ( %s ).\n", platform.c_str());
            return PlatformFlags::Platform_NONE;
        }

        return static_cast<PlatformFlags>(1 << platformIndex);
    }

    const char* PlatformHelper::GetPlatformName(const PlatformId& platform)
    { 
        if (platform < 0 || platform > PlatformId::NumPlatforms)
        {
            return "invalid";
        }
        return PlatformNames[platform];
    }

    int PlatformHelper::GetPlatformIndexFromName(const char* platformName)
    {
        AZStd::string platform(platformName);
        for (int idx = 0; idx < PlatformId::NumPlatforms; idx++)
        {
            if (PlatformNames[idx] && platform.compare(PlatformNames[idx]) == 0)
            {
                return idx;
            }
        }

        return PlatformId::Invalid;
    }
}
