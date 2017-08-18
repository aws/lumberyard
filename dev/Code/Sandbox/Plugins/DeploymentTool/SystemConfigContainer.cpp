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
#include "stdafx.h"
#include "SystemConfigContainer.h"
#include "DeploymentConfig.h"

const char* SystemConfigContainer::s_shaderCompilerIPKey = "r_ShaderCompilerServer";
const char* SystemConfigContainer::s_shaderCompilerPortKey = "r_ShaderCompilerPort";
const char* SystemConfigContainer::s_assetProcessorShaderCompilerKey = "r_AssetProcessorShaderCompiler";

SystemConfigContainer::SystemConfigContainer(const char* systemConfigFileName)
    : ConfigFileContainer(systemConfigFileName)
{
}

StringOutcome SystemConfigContainer::UpdateWithDeploymentOptions(const DeploymentConfig& deploymentConfig)
{
    SetUseAssetProcessorShaderCompiler(deploymentConfig.m_shaderCompilerAP);
    SetShaderCompilerIP(deploymentConfig.m_shaderCompilerIP);
    SetShaderCompilerPort(deploymentConfig.m_shaderCompilerPort);
    return WriteContents();
}

AZStd::string SystemConfigContainer::GetShaderCompilerIP() const
{
    return GetString(s_shaderCompilerIPKey);
}

AZStd::string SystemConfigContainer::GetShaderCompilerPort() const
{
    return GetString(s_shaderCompilerPortKey);
}

bool SystemConfigContainer::GetUseAssetProcessorShaderCompiler() const
{
    return GetBool(s_assetProcessorShaderCompilerKey);
}

AZStd::string SystemConfigContainer::GetShaderCompilerIPIncludeComments() const
{
    return GetStringIncludingComments(s_shaderCompilerIPKey);
}

AZStd::string SystemConfigContainer::GetShaderCompilerPortIncludeComments() const
{
    return GetStringIncludingComments(s_shaderCompilerPortKey);
}

bool SystemConfigContainer::GetUseAssetProcessorShaderCompilerIncludeComments() const
{
    return GetBoolIncludingComments(s_assetProcessorShaderCompilerKey);
}

void SystemConfigContainer::SetShaderCompilerIP(const AZStd::string& newIP)
{
    SetString(s_shaderCompilerIPKey, newIP);
}

void SystemConfigContainer::SetShaderCompilerPort(const AZStd::string& newPort)
{
    SetString(s_shaderCompilerPortKey, newPort);
}

void SystemConfigContainer::SetUseAssetProcessorShaderCompiler(bool useAssetProcessor)
{
    SetBool(s_assetProcessorShaderCompilerKey, useAssetProcessor);
}
