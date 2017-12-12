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

#include "DeploymentUtil.h"
#include "DeploymentConfig.h"
#include "CommandLauncher.h"
#include "SystemConfigContainer.h"

// Utility to build/deploy projects for android devices
class AndroidDeploymentUtil : public DeploymentUtil
{
public:
    AndroidDeploymentUtil(IDeploymentTool& deploymentTool, DeploymentConfig& cfg);
    AndroidDeploymentUtil(const AndroidDeploymentUtil& rhs) = delete;
    AndroidDeploymentUtil& operator=(const AndroidDeploymentUtil& rhs) = delete;
    AndroidDeploymentUtil& operator=(AndroidDeploymentUtil&& rhs) = delete;
    ~AndroidDeploymentUtil();

    static const char* GetSystemConfigFileName();

    void ConfigureAssetProcessor() override;
    void BuildAndDeploy() override;
    void DeployFromFile(const AZStd::string& buildPath) override;
    StringOutcome Launch() override;

private:

    SystemConfigContainer m_androidConfig;
    QMetaObject::Connection m_deployCallbackConnection;
    QMetaObject::Connection m_deployLogConnection;
    QProcess* m_deployProcess;

    const char* GetBuildTarget() const;
    const char* GetBuildConfiguration() const;
    void DeployCallback(int exitCode, QProcess::ExitStatus exitStatus);
    // try to push bootstrap.cfg to possible emulated sdcard paths in s_internalStoragePaths
    bool PushBootstrapCfg(QProcess* process);
    // try to push game.xml to possible emulated sdcard paths in s_internalStoragePaths
    bool PushGameXml(QProcess* process);
};

