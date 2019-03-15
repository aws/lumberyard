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
#include "DeploymentTool_precompiled.h"
#include "BootstrapConfigContainer.h"

#include <AzFramework/StringFunc/StringFunc.h>

#include "DeploymentConfig.h"


namespace
{
    const char* bootstrapFilePath = "bootstrap.cfg";

    const char* assetsKey = "assets";
    const char* gameFolderKey = "sys_game_folder";
    const char* remoteFileSystemKey = "remote_filesystem";
    const char* remoteIPKey = "remote_ip";
    const char* remotePortKey = "remote_port";
    const char* whitelistKey = "white_list";
    const char* waitForConnectKey = "wait_for_connect";

    const char* androidConnectToRemoteKey = "android_connect_to_remote";
    const char* iosConnectToRemoteKey = "ios_connect_to_remote";
}

BootstrapConfigContainer::BootstrapConfigContainer()
    : ConfigFileContainer(bootstrapFilePath)
{
}

BootstrapConfigContainer::~BootstrapConfigContainer()
{
}

StringOutcome BootstrapConfigContainer::ApplyConfiguration(const DeploymentConfig& deploymentConfig)
{
    SetBool(remoteFileSystemKey, deploymentConfig.m_useVFS);

    SetString(remoteIPKey, deploymentConfig.m_assetProcessorIpAddress);
    SetString(remotePortKey, deploymentConfig.m_assetProcessorPort);

    AZStd::string allowedAddresses = GetString(whitelistKey, true);

    AZStd::vector<AZStd::string> allowedIpAddrs;
    AzFramework::StringFunc::Tokenize(allowedAddresses.c_str(), allowedIpAddrs, ',');

    const auto& iterator = AZStd::find(allowedIpAddrs.begin(), allowedIpAddrs.end(), deploymentConfig.m_deviceIpAddress);
    if (iterator == allowedIpAddrs.end())
    {
        if (!allowedAddresses.empty())
        {
            allowedAddresses.append(",");
        }
        allowedAddresses.append(deploymentConfig.m_deviceIpAddress);
    }

    SetString(whitelistKey, allowedAddresses);

    SetBool(waitForConnectKey, false);

    switch (deploymentConfig.m_platformOption)
    {
        case PlatformOptions::Android_ARMv7:
        case PlatformOptions::Android_ARMv8:
            SetBool(androidConnectToRemoteKey, deploymentConfig.m_shaderCompilerUseAP);
            break;

    #if defined(AZ_PLATFORM_APPLE_OSX)
        case PlatformOptions::iOS:
            SetBool(iosConnectToRemoteKey, deploymentConfig.m_shaderCompilerUseAP);
            break;
    #endif // defined(AZ_PLATFORM_APPLE_OSX)

        default:
            break;
    }

    return Write();
}

AZStd::string BootstrapConfigContainer::GetHostAssetsType() const
{
    AZStd::string assetsType = GetString(assetsKey);

#if defined(AZ_PLATFORM_APPLE_OSX)
    AZStd::string platfromSpecificAssetKey = AZStd::move(AZStd::string::format("osx_%s", assetsKey));

    AZStd::string platformAssets = GetString(platfromSpecificAssetKey);
    if (!platformAssets.empty())
    {
        assetsType = AZStd::move(platformAssets);
    }
#endif // defined(AZ_PLATFORM_APPLE_OSX)

    return assetsType;
}

AZStd::string BootstrapConfigContainer::GetAssetsTypeForPlatform(PlatformOptions platform) const
{
    AZStd::string assetsType = GetString(assetsKey);

    AZStd::string platfromSpecificAssetKey;
    switch (platform)
    {
        case PlatformOptions::Android_ARMv7:
        case PlatformOptions::Android_ARMv8:
            platfromSpecificAssetKey = AZStd::move(AZStd::string::format("android_%s", assetsKey));
            break;

    #if defined(AZ_PLATFORM_APPLE_OSX)
        case PlatformOptions::iOS:
            platfromSpecificAssetKey = AZStd::move(AZStd::string::format("ios_%s", assetsKey));
            break;
    #endif // defined(AZ_PLATFORM_APPLE_OSX)

        default:
            break;
    }

    if (!platfromSpecificAssetKey.empty())
    {
        AZStd::string platformAssets = GetString(platfromSpecificAssetKey);
        if (!platformAssets.empty())
        {
            assetsType = AZStd::move(platformAssets);
        }
    }

    return assetsType;
}

AZStd::string BootstrapConfigContainer::GetGameFolder() const
{
    return GetString(gameFolderKey);
}

AZStd::string BootstrapConfigContainer::GetRemoteIP() const
{
    return GetString(remoteIPKey);
}
