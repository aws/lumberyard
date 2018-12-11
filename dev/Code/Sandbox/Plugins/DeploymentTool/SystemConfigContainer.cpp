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
#include "SystemConfigContainer.h"

#include <AzFramework/StringFunc/StringFunc.h>

#include "DeploymentConfig.h"
#include "DeployNotificationsBus.h"
#include "NetworkUtils.h"


namespace
{
    const char* shaderCompilerIPKey = "r_ShaderCompilerServer";
    const char* shaderCompilerPortKey = "r_ShaderCompilerPort";

    const char* assetProcessorShaderCompilerKey = "r_AssetProcessorShaderCompiler";

    const char* remoteConsoleAllowedAddressesKey = "log_RemoteConsoleAllowedAddresses";
    const char* remoteConsolePortKey = "log_RemoteConsolePort";
}


SystemConfigContainer::SystemConfigContainer(const char* systemConfigFileName)
    : ConfigFileContainer(systemConfigFileName ? systemConfigFileName : "")
{
}

StringOutcome SystemConfigContainer::ApplyConfiguration(const DeploymentConfig& deploymentConfig)
{
    SetString(shaderCompilerIPKey, deploymentConfig.m_shaderCompilerIpAddress);
    SetString(shaderCompilerPortKey, deploymentConfig.m_shaderCompilerPort);

    SetBool(assetProcessorShaderCompilerKey, deploymentConfig.m_shaderCompilerUseAP);

    AZStd::vector<AZStd::string> localIpAddrs;
    if (!DeployTool::GetAllHostIPAddrs(localIpAddrs))
    {
        DEPLOY_LOG_WARN("[WARN] Failed to get the host computer's local IP addresses.  The remote log connection may not work correctly.");
    }

    AZStd::string allowedAddresses = GetString(remoteConsoleAllowedAddressesKey, true);

    AZStd::vector<AZStd::string> allowedIpAddrs;
    AzFramework::StringFunc::Tokenize(allowedAddresses.c_str(), allowedIpAddrs, ',');

    for (const auto& localAddr : localIpAddrs)
    {
        const auto& iterator = AZStd::find(allowedIpAddrs.begin(), allowedIpAddrs.end(), localAddr);
        if (iterator == allowedIpAddrs.end())
        {
            if (!allowedAddresses.empty())
            {
                allowedAddresses.append(",");
            }
            allowedAddresses.append(localAddr);
        }
    }

    SetString(remoteConsoleAllowedAddressesKey, allowedAddresses);
    SetString(remoteConsolePortKey, deploymentConfig.m_deviceRemoteLogPort);

    return Write();
}

AZStd::string SystemConfigContainer::GetShaderCompilerIP() const
{
    return GetString(shaderCompilerIPKey);
}

AZStd::string SystemConfigContainer::GetShaderCompilerPort() const
{
    return GetString(shaderCompilerPortKey);
}

bool SystemConfigContainer::GetUseAssetProcessorShaderCompiler() const
{
    return GetBool(assetProcessorShaderCompilerKey);
}
