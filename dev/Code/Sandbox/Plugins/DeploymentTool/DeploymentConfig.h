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

#include <AzCore/std/string/string.h>

#include <RemoteConsoleCore.h>


enum class PlatformOptions : unsigned char
{
    Android_ARMv7,
    Android_ARMv8,
    iOS,
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        PublicName,
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
        AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
        AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif

    Invalid
};

struct DeploymentConfig
{
    DeploymentConfig()
        : m_projectName()
        , m_buildConfiguration("profile")

        , m_deviceId()
        , m_deviceIpAddress("127.0.0.1")

        , m_assetProcessorIpAddress("127.0.0.1")
        , m_assetProcessorPort("45643")

        , m_shaderCompilerIpAddress("127.0.0.1")
        , m_shaderCompilerPort("61453")

        , m_deviceRemoteLogPort(AZStd::string::format("%d", defaultRemoteConsolePort))
        , m_hostRemoteLogPort(static_cast<AZ::u16>(defaultRemoteConsolePort))

        , m_platformOption(PlatformOptions::Android_ARMv7)
        , m_buildGame(false)
        , m_useVFS(false)
        , m_shaderCompilerUseAP(false)
        , m_cleanDevice(false)
        , m_localDevice(true)
    {
    }

    AZStd::string m_projectName;
    AZStd::string m_buildConfiguration;

    AZStd::string m_deviceId;
    AZStd::string m_deviceIpAddress;

    AZStd::string m_assetProcessorIpAddress;
    AZStd::string m_assetProcessorPort;

    AZStd::string m_shaderCompilerIpAddress;
    AZStd::string m_shaderCompilerPort;

    AZStd::string m_deviceRemoteLogPort; // used to set the log_RemoteConsolePort in system*.cfg
    AZ::u16 m_hostRemoteLogPort; // used specifically when port forwarding connections to localhost to avoid collisions with local instances of LY

    PlatformOptions m_platformOption;
    bool m_buildGame;
    bool m_useVFS;
    bool m_shaderCompilerUseAP;
    bool m_cleanDevice;
    bool m_localDevice;
};
