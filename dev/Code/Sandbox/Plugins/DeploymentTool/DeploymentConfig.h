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

#include "stdafx.h"

enum class PlatformOptions
{
    Android,
    iOS,
    XBoxOne, // ACCEPTED_USE
    PS4 // ACCEPTED_USE
};

struct DeploymentConfig
{
    DeploymentConfig() 
        : m_assetProcessorRemoteIpAddress("127.0.0.1")
        , m_assetProcessorRemoteIpPort("45643")
        , m_shaderCompilerIP("127.0.0.1")
        , m_shaderCompilerPort("61453")
        , m_compiler("Clang")
        , m_buildConfiguration("Profile")
        , m_platformOptions(PlatformOptions::Android)
        , m_buildGame(false)
        , m_useVFS(false)
        , m_shaderCompilerAP(false)
        , m_installExecutable(true)
        , m_cleanDeviceBeforeInstall(false)
    { }

    AZStd::string m_buildPath;
    AZStd::string m_assetProcessorRemoteIpAddress;
    AZStd::string m_assetProcessorRemoteIpPort;
    AZStd::string m_shaderCompilerIP;
    AZStd::string m_shaderCompilerPort;
    AZStd::string m_projectName;
    AZStd::string m_compiler;
    AZStd::string m_buildConfiguration;
    PlatformOptions m_platformOptions;
    bool m_buildGame;
    bool m_useVFS;
    bool m_shaderCompilerAP;
    bool m_installExecutable;
    bool m_cleanDeviceBeforeInstall;
};
