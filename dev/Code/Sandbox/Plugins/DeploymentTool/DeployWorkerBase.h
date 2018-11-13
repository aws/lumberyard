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

#include "DeploymentConfig.h"
#include "CommandLauncher.h"
#include "IDeploymentTool.h"
#include "BootstrapConfigContainer.h"

using StringOutcome = AZ::Outcome<void, AZStd::string>;

// abstract class for platform independent project building / deployment
class DeploymentUtil 
{
public:
    DeploymentUtil(IDeploymentTool& deploymentTool, DeploymentConfig& cfg);
    DeploymentUtil(const DeploymentUtil& rhs) = delete;
    DeploymentUtil& operator=(const DeploymentUtil& rhs) = delete;
    DeploymentUtil& operator=(DeploymentUtil&& rhs) = delete;
    virtual ~DeploymentUtil();
    
    virtual void ConfigureAssetProcessor() = 0;
    virtual void BuildAndDeploy() = 0;
    virtual void DeployFromFile(const AZStd::string& buildPath) = 0;
    virtual StringOutcome Launch() = 0;

protected:
    static const char* s_shaderCompilerCmd;
    static const char* s_pathToShaderCompiler;
    static const char* s_debugBuildCfg;
    static const char* s_profileBuildCfg;
    static const char* s_releaseBuildCfg;
    static const char* s_buildOptions;

    DeploymentConfig& m_cfg;
    IDeploymentTool& m_deploymentTool;
    CommandLauncher* m_cmdLauncher;

    // Log std out of process
    void LogStdOut(QProcess* process);

    // Launch shader compiler
    bool LaunchShaderCompiler();
};