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

#include "StdAfx.h"
#include "AndroidDeploymentUtil.h"
#include "ConfigFileContainer.h"
#include <AzCore/JSON/document.h>
#include <AzCore/std/string/conversions.h>

namespace 
{
#if defined (AZ_PLATFORM_WINDOWS)
    static const char* g_wafCmd = "lmbr_waf.bat";
#else
    static const char* g_wafCmd = "lmbr_waf.sh";
#endif
    const char* s_systemConfigFileName = "system_android_es3.cfg";

    AZStd::string GetDeployOptionsString(const DeploymentConfig& deploymentConfiguration)
    {
        AZStd::string deploymentOptions = AZStd::move(AZStd::string::format("--deploy-android=True --deploy-android-asset-mode=loose --deploy-android-clean-device=%s --from-editor-deploy", (deploymentConfiguration.m_cleanDeviceBeforeInstall)? "True" : "False"));
        if (deploymentConfiguration.m_installExecutable)
        {
            deploymentOptions += " --deploy-android-executable=True --deploy-android-replace-apk=True";
        }
        else
        {
            deploymentOptions += " --deploy-android-executable=False";
        }

        return deploymentOptions;
    }
}

AndroidDeploymentUtil::AndroidDeploymentUtil(IDeploymentTool& deploymentTool, DeploymentConfig& cfg)
    : DeploymentUtil(deploymentTool, cfg)
    , m_androidConfig(s_systemConfigFileName)
    , m_deployProcess(new QProcess())
{ 
    // connect to deploy callback
    m_deployCallbackConnection = QObject::connect(m_deployProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), // use static_cast to specify which overload of QProcess::finished to use
        m_deployProcess,
        [this](int exitCode, QProcess::ExitStatus exitStatus)
    {
        DeployCallback(exitCode, exitStatus);
    });

    // connect to output log
    m_deployProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_deployLogConnection = QObject::connect(m_deployProcess, &QProcess::readyReadStandardOutput, m_deployProcess,
        [this]()
    {
        LogStdOut(m_deployProcess);
    });

    StringOutcome outcome = m_androidConfig.ReadContents();
    if (!outcome.IsSuccess())
    {
        m_deploymentTool.LogEndLine(outcome.GetError());
    }
}

AndroidDeploymentUtil::~AndroidDeploymentUtil() 
{
    QObject::disconnect(m_deployLogConnection);
    QObject::disconnect(m_deployCallbackConnection);
    m_deployProcess->kill();
    delete m_deployProcess;
}

const char* AndroidDeploymentUtil::GetSystemConfigFileName()
{
    return s_systemConfigFileName;
}

void AndroidDeploymentUtil::ConfigureAssetProcessor()
{
    StringOutcome outcome = m_androidConfig.UpdateWithDeploymentOptions(m_cfg);
    if (!outcome.IsSuccess())
    {
        m_deploymentTool.LogEndLine(outcome.GetError());
    }
}

void AndroidDeploymentUtil::BuildAndDeploy()
{
    ConfigureAssetProcessor();

    AZStd::string deploymentOptions = GetDeployOptionsString(m_cfg);
    AZStd::string buildCmd = AZStd::move(AZStd::string::format("%s %s build_android_armv7_clang_%s %s", g_wafCmd, deploymentOptions.c_str(), m_cfg.m_buildConfiguration.c_str(), s_buildOptions));

    if (!m_cmdLauncher->AsyncProcess(buildCmd.c_str(), m_deployProcess))
    {
        m_deploymentTool.Error(m_deployProcess->errorString().toStdString().c_str());
    }
}

void AndroidDeploymentUtil::DeployFromFile(const AZStd::string& buildPath)
{
    ConfigureAssetProcessor();

    AZStd::string deploymentOptions = GetDeployOptionsString(m_cfg);
    AZStd::string deployCmd = AZStd::move(AZStd::string::format("%s %s deploy_android_armv7_clang_%s", g_wafCmd, deploymentOptions.c_str(), m_cfg.m_buildConfiguration.c_str()));

    if (!m_cmdLauncher->AsyncProcess(deployCmd.c_str(), m_deployProcess))
    {
        m_deploymentTool.Error(m_deployProcess->errorString().toStdString().c_str());
    }
}

void AndroidDeploymentUtil::DeployCallback(int exitCode, QProcess::ExitStatus exitStatus)
{
    // error deploying
    if (exitCode != 0 || exitStatus != QProcess::ExitStatus::NormalExit) 
    {
        m_deploymentTool.Error(m_deployProcess->errorString().toStdString().c_str());
        return;
    }

    StringOutcome outcome = Launch();
    if (!outcome.IsSuccess())
    {
        m_deploymentTool.Error(outcome.GetError());
    }
    else
    {
        m_deploymentTool.Success("Launch successful");
    }
}

StringOutcome AndroidDeploymentUtil::Launch()
{
    QProcess* launchProcess = new QProcess();
    launchProcess->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(launchProcess, &QProcess::readyReadStandardOutput, launchProcess,
        [launchProcess, this]()
    {
        LogStdOut(launchProcess);
    });

    if (m_cfg.m_useVFS)
    {
        AZStd::string revPortCmd = AZStd::move(AZStd::string::format("adb reverse tcp:%s tcp:%s", m_cfg.m_assetProcessorRemoteIpPort, m_cfg.m_assetProcessorRemoteIpPort));
        m_deploymentTool.LogEndLine(revPortCmd);
        LMBR_ENSURE(m_cmdLauncher->SyncProcess(revPortCmd.c_str(), launchProcess), "Failed to reverse port");
    }

    LMBR_ENSURE(LaunchShaderCompiler(), "Failed to launch shader compiler");

    const char unlockScreenCmd[] = "adb shell input keyevent 82";

    m_deploymentTool.LogEndLine(unlockScreenCmd);
    LMBR_ENSURE(m_cmdLauncher->SyncProcess(unlockScreenCmd, launchProcess), "Failed to unlock screen");
    
    // read project.json
    AZStd::string fileContents;
    StringOutcome outcome = ConfigFileContainer::ReadFile((m_cfg.m_projectName + "/project.json").c_str(), fileContents);
    if (!outcome.IsSuccess()) 
    {
        return outcome;
    }

    // get activity and package name
    rapidjson::Document projectCfg;
    projectCfg.Parse(fileContents.c_str());
    const char* packageName = projectCfg["android_settings"]["package_name"].GetString();
    AZStd::string runCmd = AZStd::move(AZStd::string::format("adb shell am start -n %s/%s.%sActivity", packageName, packageName, m_cfg.m_projectName.c_str())); 
    // launch game
    m_deploymentTool.LogEndLine(runCmd);
    LMBR_ENSURE(m_cmdLauncher->SyncProcess(runCmd.c_str(), launchProcess), "Failed to launch application");

    return AZ::Success();
}
