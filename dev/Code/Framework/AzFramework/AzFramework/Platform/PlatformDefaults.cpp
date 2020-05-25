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

#include <AzFramework/StringFunc/StringFunc.h>

namespace AzFramework
{
    const char* PlatformNames[PlatformId::NumPlatformIds] = { PlatformPC, PlatformES3, PlatformIOS, PlatformOSX, PlatformXenia, PlatformProvo, PlatformSalem, PlatformServer, PlatformAll, PlatformAllCient };

    PlatformFlags PlatformHelper::GetPlatformFlagFromPlatformIndex(const PlatformId& platformIndex)
    {
        if (platformIndex < 0 || platformIndex > PlatformId::NumPlatformIds)
        {
            return PlatformFlags::Platform_NONE;
        }
        if (platformIndex == PlatformId::ALL)
        {
            return PlatformFlags::Platform_ALL;
        }
        if (platformIndex == PlatformId::ALL_CLIENT)
        {
            return PlatformFlags::Platform_ALL_CLIENT;
        }
        return static_cast<PlatformFlags>(1 << platformIndex);
    }

    AZStd::vector<AZStd::string> PlatformHelper::GetPlatforms(const PlatformFlags& platformFlags)
    {
        AZStd::vector<AZStd::string> platforms;
        for (int platformNum = 0; platformNum < PlatformId::NumPlatformIds; ++platformNum)
        {
            const bool isAllPlatforms = PlatformId::ALL == static_cast<PlatformId>(platformNum)
                && ((platformFlags & PlatformFlags::Platform_ALL) != PlatformFlags::Platform_NONE);

            const bool isAllClientPlatforms = PlatformId::ALL_CLIENT == static_cast<PlatformId>(platformNum)
                && ((platformFlags & PlatformFlags::Platform_ALL_CLIENT) != PlatformFlags::Platform_NONE);

            if (isAllPlatforms || isAllClientPlatforms
                || (platformFlags & static_cast<PlatformFlags>(1 << platformNum)) != PlatformFlags::Platform_NONE)
            {
                platforms.push_back(PlatformNames[platformNum]);
            }
        }

        return platforms;
    }

    AZStd::vector<AZStd::string> PlatformHelper::GetPlatformsInterpreted(const PlatformFlags& platformFlags)
    {
        return GetPlatforms(GetPlatformFlagsInterpreted(platformFlags));
    }

    AZStd::vector<PlatformId> PlatformHelper::GetPlatformIndices(const PlatformFlags& platformFlags)
    {
        AZStd::vector<PlatformId> platformIndices;
        for (int i = 0; i < PlatformId::NumPlatformIds; i++)
        {
            PlatformId index = static_cast<PlatformId>(i);
            if ((GetPlatformFlagFromPlatformIndex(index) & platformFlags) != PlatformFlags::Platform_NONE)
            {
                platformIndices.emplace_back(index);
            }
        }
        return platformIndices;
    }

    AZStd::vector<AzFramework::PlatformId> PlatformHelper::GetPlatformIndicesInterpreted(const PlatformFlags& platformFlags)
    {
        return GetPlatformIndices(GetPlatformFlagsInterpreted(platformFlags));
    }

    PlatformFlags PlatformHelper::GetPlatformFlag(const AZStd::string& platform)
    {
        int platformIndex = GetPlatformIndexFromName(platform.c_str());
        if (platformIndex == PlatformId::Invalid)
        {
            AZ_Error("PlatformDefault", false, "Invalid Platform ( %s ).\n", platform.c_str());
            return PlatformFlags::Platform_NONE;
        }

        if(platformIndex == PlatformId::ALL)
        {
            return PlatformFlags::Platform_ALL;
        }

        if (platformIndex == PlatformId::ALL_CLIENT)
        {
            return PlatformFlags::Platform_ALL_CLIENT;
        }

        return static_cast<PlatformFlags>(1 << platformIndex);
    }

    const char* PlatformHelper::GetPlatformName(const PlatformId& platform)
    { 
        if (platform < 0 || platform > PlatformId::NumPlatformIds)
        {
            return "invalid";
        }
        return PlatformNames[platform];
    }

    int PlatformHelper::GetPlatformIndexFromName(const char* platformName)
    {
        AZStd::string platform(platformName);
        for (int idx = 0; idx < PlatformId::NumPlatformIds; idx++)
        {
            if (PlatformNames[idx] && platform.compare(PlatformNames[idx]) == 0)
            {
                return idx;
            }
        }

        return PlatformId::Invalid;
    }

    AZStd::string PlatformHelper::GetCommaSeparatedPlatformList(const PlatformFlags& platformFlags)
    {
        AZStd::vector<AZStd::string> platformNames = GetPlatforms(platformFlags);
        AZStd::string platformsString;
        AzFramework::StringFunc::Join(platformsString, platformNames.begin(), platformNames.end(), ", ");
        return platformsString;
    }

    AzFramework::PlatformFlags PlatformHelper::GetPlatformFlagsInterpreted(const PlatformFlags& platformFlags)
    {
        PlatformFlags returnFlags = PlatformFlags::Platform_NONE;

        if((platformFlags & PlatformFlags::Platform_ALL) != PlatformFlags::Platform_NONE)
        {
            for (int i = 0; i < NumPlatforms; ++i)
            {
                auto platformId = static_cast<PlatformId>(i);
                
                if (platformId != PlatformId::ALL && platformId != PlatformId::ALL_CLIENT)
                {
                    returnFlags |= GetPlatformFlagFromPlatformIndex(platformId);
                }
            }
        }
        else if((platformFlags & PlatformFlags::Platform_ALL_CLIENT) != PlatformFlags::Platform_NONE)
        {
            for (int i = 0; i < NumPlatforms; ++i)
            {
                auto platformId = static_cast<PlatformId>(i);
                
                if (platformId != PlatformId::ALL && platformId != PlatformId::ALL_CLIENT && platformId != PlatformId::SERVER)
                {
                    returnFlags |= GetPlatformFlagFromPlatformIndex(platformId);
                }
            }
        }
        else
        {
            returnFlags = platformFlags;
        }

        return returnFlags;
    }

    bool PlatformHelper::IsSpecialPlatform(const PlatformFlags& platformFlags)
    {
        return (platformFlags & PlatformFlags::Platform_ALL) != PlatformFlags::Platform_NONE
            || (platformFlags & PlatformFlags::Platform_ALL_CLIENT) != PlatformFlags::Platform_NONE;
    }

    bool HasFlagHelper(PlatformFlags flags, PlatformFlags checkPlatform)
    {
        return (flags & checkPlatform) == checkPlatform;
    }


    bool PlatformHelper::HasPlatformFlag(PlatformFlags flags, PlatformId checkPlatform)
    {
        // If checkPlatform contains any kind of invalid id, just exit out here
        if(checkPlatform == PlatformId::Invalid || checkPlatform == NumPlatforms)
        {
            return false;
        }

        // ALL_CLIENT + SERVER = ALL
        if(HasFlagHelper(flags, PlatformFlags::Platform_ALL_CLIENT | PlatformFlags::Platform_SERVER))
        {
            flags = PlatformFlags::Platform_ALL;
        }

        if(HasFlagHelper(flags, PlatformFlags::Platform_ALL))
        {
            // It doesn't matter what checkPlatform is set to in this case, just return true
            return true;
        }

        if(HasFlagHelper(flags, PlatformFlags::Platform_ALL_CLIENT))
        {
            return checkPlatform != PlatformId::SERVER;
        }

        return HasFlagHelper(flags, GetPlatformFlagFromPlatformIndex(checkPlatform));
    }

}
