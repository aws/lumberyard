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
#include "DeploymentUtil.h"

#if _MSC_VER == 1900
    #define CRYSCOMPILESERVER_EXE_BUILD_TOOL "_vc140x64"
#elif _MSC_VER == 1800
    #define CRYSCOMPILESERVER_EXE_BUILD_TOOL "_vc120x64"
#else // _MSC_VER
    #define CRYSCOMPILESERVER_EXE_BUILD_TOOL ""
#endif // _MSC_VER

#define CRYSCOMPILESERVER_EXE_NAME "CrySCompileServer" CRYSCOMPILESERVER_EXE_BUILD_TOOL ".exe"

#pragma message("Setting CrySCompileServer executable name the deployment plugin to " CRYSCOMPILESERVER_EXE_NAME)


const char* DeploymentUtil::s_shaderCompilerCmd = "Tools\\CrySCompileServer\\x64\\profile\\" CRYSCOMPILESERVER_EXE_NAME;
const char* DeploymentUtil::s_pathToShaderCompiler = "Tools\\CrySCompileServer\\x64\\profile\\";
const char* DeploymentUtil::s_debugBuildCfg = "debug";
const char* DeploymentUtil::s_profileBuildCfg = "profile";
const char* DeploymentUtil::s_releaseBuildCfg = "release";
const char* DeploymentUtil::s_buildOptions = " -p all --progress";

DeploymentUtil::DeploymentUtil(IDeploymentTool& deploymentTool, DeploymentConfig& cfg)
    : m_deploymentTool(deploymentTool)
    , m_cfg(cfg)
    , m_cmdLauncher(new CommandLauncher())
{
}

DeploymentUtil::~DeploymentUtil()
{
    delete m_cmdLauncher;
}

void DeploymentUtil::LogStdOut(QProcess* process)
{
    m_deploymentTool.Log(process->readAllStandardOutput().toStdString().c_str());
}

