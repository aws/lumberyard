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

#include <QHash>
#include <QString>

#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

#include "DeploymentConfig.h"
#include "SystemConfigContainer.h"


using StringOutcome = AZ::Outcome<AZStd::string, AZStd::string>;
using DeviceMap = QHash<QString, QString>; // <device id, display name>


class QProcess;

// abstract class for platform independent project building / deployment
class DeployWorkerBase
{
public:
    DeployWorkerBase(const char* systemConfigFile);
    virtual ~DeployWorkerBase();

    virtual const char* GetSystemConfigFileName() const = 0;
    virtual bool GetConnectedDevices(DeviceMap& connectedDevices) const = 0;

    StringOutcome ApplyConfiguration(const DeploymentConfig& deployConfig);

    void Run();
    bool IsRunning();

    bool DeviceIsConnected(const char* deviceId);

    static AZStd::string GetPlatformSpecficDefaultSettingsFilename(PlatformOptions platformOption, const char* devRoot = ".");
    static StringOutcome GetPlatformSpecficDefaultAttributeValue(const char* name, PlatformOptions platformOption, const char* devRoot = ".");
    static StringOutcome GetPlatformSpecficDefaultSettingsValue(const char* groupName, const char* keyName, PlatformOptions platformOption, const char* devRoot = ".");
    static StringOutcome GetCommonBuildConfigurationsDefaultSettingsValue(const char* buildConfiguration, const char* name, const char* devRoot = ".");

protected:
    virtual AZStd::string GetWafBuildArgs() const = 0;
    virtual AZStd::string GetWafDeployArgs() const = 0;

    virtual StringOutcome Prepare() = 0;
    virtual void StartBuildAndDeploy();
    virtual void StartDeploy();
    virtual StringOutcome Launch() = 0;

    bool LaunchShaderCompiler() const;

    void StartWafCommand(const char* commandType, const AZStd::string& commandArgs);
    bool RunBlockingCommand(const AZStd::string& command, QString* output = nullptr) const;

    static StringOutcome LoadJsonData(const AZStd::string& file, rapidjson::Document& jsonData);
    static StringOutcome GetUserSettingsValue(const char* groupName, const char* keyName, PlatformOptions platformOption);

    DeploymentConfig m_deploymentConfig;
    SystemConfigContainer m_systemConfig;

    QProcess* m_wafProcess;
    QMetaObject::Connection m_finishedConnection;

    bool m_isRunning = false;

private:
    AZ_DISABLE_COPY_MOVE(DeployWorkerBase);
};