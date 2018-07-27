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
#include <AzCore/Socket/AzSocket.h>
#include <AzCore/std/string/conversions.h>

#if _MSC_VER >= 1910
    #define CRYSCOMPILESERVER_EXE_BUILD_TOOL "_vc141x64"
#elif _MSC_VER >= 1900
    #define CRYSCOMPILESERVER_EXE_BUILD_TOOL "_vc140x64"
#elif _MSC_VER >= 1800
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

bool DeploymentUtil::LaunchShaderCompiler()
{
    // Try to connect to the shader compiler
    AZSOCKET socket = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(socket))
    {
        m_deploymentTool.LogEndLine("Could not create valid socket!");
        return false;
    }
    AZ::AzSock::AzSocketAddress socketAddress;
    if (!socketAddress.SetAddress(m_cfg.m_shaderCompilerIP, AZStd::stoul(m_cfg.m_shaderCompilerPort)))
    {
        m_deploymentTool.LogEndLine("Could not set socket address!");
        return false;
    }
    AZ::s32 result = AZ::AzSock::Connect(socket, socketAddress);

    if (AZ::AzSock::SocketErrorOccured(result))
    {
        // We weren't able to connect. So, start the shader compiler.
        m_deploymentTool.LogEndLine("Starting shader compiler...");
        m_deploymentTool.LogEndLine(s_shaderCompilerCmd);
        return m_cmdLauncher->DetachProcess(s_shaderCompilerCmd, s_pathToShaderCompiler);
    }
    else
    {
        // XML for Remote Shader Compiler v2.3 for a RequestLine job type.
        // Using Mac Metal with LLVM compiler because it's supported in both Mac and PC.
        const AZStd::string data = "<?xml version=\"1.0\"?><Compile Version=\"2.3\" JobType=\"RequestLine\" Project=\"SamplesProject\" Platform=\"Mac\" Compiler=\"METAL_LLVM_DXC\" Language=\"METAL\" ShaderList=\"ShaderList_METAL.txt\" ShaderRequest=\"&lt;3&gt;FixedPipelineEmu@FPPS()()(2a2a0505)(0)(0)(0)(PS)\"/>";
        
        const uint64 size = data.size();
        AZ::AzSock::Send(socket, (const char*)&size, 8, 0);
        result = AZ::AzSock::Send(socket, data.c_str(), static_cast<uint32>(size), 0);
        if (AZ::AzSock::SocketErrorOccured(result))
        {
            m_deploymentTool.LogEndLine("Could not send request to shader compiler!");
            return false;
        }

        // We were able to connect to the shader compiler. So, it's already running.
        m_deploymentTool.LogEndLine("Shader compiler already running. Skipping shader compiler launch...");
        
        // Close the connection and move on.
        result = AZ::AzSock::Shutdown(socket, SD_BOTH);
        if (AZ::AzSock::SocketErrorOccured(result))
        {
            m_deploymentTool.LogEndLine("Could not shutdown socket!");
        }
        result = AZ::AzSock::CloseSocket(socket);
        if (AZ::AzSock::SocketErrorOccured(result))
        {
            m_deploymentTool.LogEndLine("Could not close socket!");
        }
    }

    return true;
}

