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

#include "ConfigFileContainer.h"

struct DeploymentConfig;

class SystemConfigContainer : private ConfigFileContainer
{
public:
    SystemConfigContainer(const char* systemConfigFileName);
    ~SystemConfigContainer() = default;

    SystemConfigContainer(const SystemConfigContainer& rhs) = delete;
    SystemConfigContainer& operator=(const SystemConfigContainer& rhs) = delete;
    SystemConfigContainer& operator=(SystemConfigContainer&& rhs) = delete;

    using ConfigFileContainer::ReadContents;
    using ConfigFileContainer::WriteContents;

    StringOutcome UpdateWithDeploymentOptions(const DeploymentConfig& deploymentConfig);

    AZStd::string GetShaderCompilerIP() const;
    AZStd::string GetShaderCompilerPort() const;
    bool GetUseAssetProcessorShaderCompiler() const;

    AZStd::string GetShaderCompilerIPIncludeComments() const;
    AZStd::string GetShaderCompilerPortIncludeComments() const;
    bool GetUseAssetProcessorShaderCompilerIncludeComments() const;

    void SetShaderCompilerIP(const AZStd::string& newIP);
    void SetShaderCompilerPort(const AZStd::string& newPort);
    void SetUseAssetProcessorShaderCompiler(bool useAssetProcessor);

private:
    static const char* s_shaderCompilerIPKey;
    static const char* s_shaderCompilerPortKey;
    static const char* s_assetProcessorShaderCompilerKey;
};

