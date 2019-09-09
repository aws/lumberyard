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
#include "DeploymentTool_precompiled.h"
#include "DeployWorker_ios.h"

#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

#include "DeployNotificationsBus.h"


DeployWorkerIos::DeployWorkerIos()
    : DeployWorkerBase(GetSystemConfigFileName())
    , m_xcodeProject()
    , m_xcodebuildProcess(new QProcess())
{
    m_xcodebuildProcess->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(m_xcodebuildProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            // since we are forcibly killing the process we want to look out for a crash exit
            // with return code 9 in order to consider the launch a success.  natural exit
            // likely means the app crashed
            bool isSuccess = ((exitStatus == QProcess::CrashExit) && (exitCode == 9));
            if (!isSuccess)
            {
                DEPLOY_LOG_ERROR(m_xcodebuildProcess->errorString().toUtf8().data());
            }

            DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, isSuccess);
        });

    QObject::connect(m_xcodebuildProcess, &QProcess::readyReadStandardOutput,
        [this]()
        {
            const int killProcessDelayInMilliseconds = 30 * 1000; // 30 second timeout :(

            QString output(m_xcodebuildProcess->readAllStandardOutput());
            QStringList lines = output.split('\n', QString::SkipEmptyParts);

            for (const auto& line : lines)
            {
                if (line.startsWith("writeDictToFile:"))
                {
                    DEPLOY_LOG_INFO("[INFO] Launching...");
                    QTimer::singleShot(killProcessDelayInMilliseconds, Qt::PreciseTimer,
                        [this]()
                        {
                            m_xcodebuildProcess->kill();
                        });
                }
            }
        });
}

DeployWorkerIos::~DeployWorkerIos()
{
    AZ_Warning("DeploymentTool", m_xcodebuildProcess->state() == QProcess::NotRunning, "iOS deployment process not completed, terminating!");

    m_xcodebuildProcess->kill();
    delete m_xcodebuildProcess;
}

const char* DeployWorkerIos::GetSystemConfigFileName() const
{
    return "system_ios_ios.cfg";
}

bool DeployWorkerIos::GetConnectedDevices(DeviceMap& connectedDevices) const
{
    // search for all connected devices using in instruments
    // Expected output:
    // <Device_Name> (<os_version>) [<device_id>]
    // <Device_Name> (<os_version>) [<device_id>] (Simulator)
    const char* physicalDeviceIdRegex = "^(\\S+.*) \\((\\S+)\\) \\[(\\S+)\\]";
    const int deviceNameGroup = 1;
    const int deviceVersionGroup = 2;
    const int deviceIdGroup = 3;

    QString output;
    bool ret = RunBlockingCommand("instruments -s devices", &output);
    if (ret)
    {
        QStringList lines = output.split('\n', QString::SkipEmptyParts);
        QRegularExpression regex(physicalDeviceIdRegex);

        for (const auto& line : lines)
        {
            if (!line.endsWith("(Simulator)"))
            {
                QRegularExpressionMatch match = regex.match(line);
                if (match.hasMatch())
                {
                    QString deviceName = QString("%1 (iOS %2)").arg(match.captured(deviceNameGroup), match.captured(deviceVersionGroup));
                    QString deviceId = match.captured(deviceIdGroup);

                    connectedDevices[deviceId] = deviceName;
                }
            }
        }
    }

    return ret;
}

AZStd::string DeployWorkerIos::GetWafBuildArgs() const
{
    return "--package-projects-automatically=True --run-xcode-for-packaging=True";
}

AZStd::string DeployWorkerIos::GetWafDeployArgs() const
{
    return "";
}

StringOutcome DeployWorkerIos::Prepare()
{
    // determine the xcode project file path from the waf user settings
    const char* userSettingsFile = "_WAF_/user_settings.options";
    const char* defaultSettingsFile = "_WAF_/default_settings.json";

    const char* iosProjectSettingsGroup = "iOS Project Generator";
    const char* iosProjectFolderKey = "ios_project_folder";
    const char* iosProjectNameKey = "ios_project_name";

    if (QFile::exists(userSettingsFile))
    {
        QSettings userWafSetting(userSettingsFile, QSettings::IniFormat);

        if (userWafSetting.status() == QSettings::NoError)
        {
            const QString keyFormat("%1/%2");

            QString folderKey = keyFormat.arg(iosProjectSettingsGroup, iosProjectFolderKey);
            QString fileKey = keyFormat.arg(iosProjectSettingsGroup, iosProjectNameKey);

            if (userWafSetting.contains(folderKey) && userWafSetting.contains(fileKey))
            {
                QByteArray folderName(userWafSetting.value(folderKey).toByteArray());
                QByteArray projectName(userWafSetting.value(fileKey).toByteArray());

                m_xcodeProject = AZStd::move(AZStd::string::format("%s/%s.xcodeproj", folderName.data(), projectName.data()));
            }
        }
    }

    if (m_xcodeProject.empty())
    {
        rapidjson::Document defaultWafSettings;
        StringOutcome outcome = LoadJsonData(defaultSettingsFile, defaultWafSettings);
        if (!outcome.IsSuccess())
        {
            return outcome;
        }

        const char* defaultValueKey = "default_value";
        const char* attributeKey = "attribute";

        AZStd::string folderName;
        AZStd::string projectName;

        const auto& iosProjectGroup = defaultWafSettings[iosProjectSettingsGroup];
        for (auto iter = iosProjectGroup.Begin(); iter != iosProjectGroup.End(); ++iter)
        {
            const auto& entry = *iter;

            if (strcmp(entry[attributeKey].GetString(), iosProjectFolderKey) == 0)
            {
                folderName = entry[defaultValueKey].GetString();
            }
            else if (strcmp(entry[attributeKey].GetString(), iosProjectNameKey) == 0)
            {
                projectName = entry[defaultValueKey].GetString();
            }
        }

        m_xcodeProject = AZStd::move(AZStd::string::format("%s/%s.xcodeproj", folderName.data(), projectName.data()));
    }

    if (m_xcodeProject.empty())
    {
        return AZ::Failure<AZStd::string>("Unable to determine the Xcode project path.");
    }

    return AZ::Success();
}

void DeployWorkerIos::StartDeploy()
{
    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessStatusChange, "Deploying");

    // building the game automatically runs the packaging so we just need to "launch" the app
    if (m_deploymentConfig.m_buildGame)
    {
        StringOutcome outcome = Launch();
        if (!outcome.IsSuccess())
        {
            DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
            DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
        }
    }
    // otherwise, run the packaging command directly to ensure the assets get updated correctly
    else
    {
        QObject::disconnect(m_finishedConnection);

        m_finishedConnection = QObject::connect(m_wafProcess,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus)
            {
                if (exitStatus == QProcess::NormalExit && exitCode == 0)
                {
                    StringOutcome outcome = Launch();
                    if (!outcome.IsSuccess())
                    {
                        DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
                        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
                    }
                }
                else
                {
                    DEPLOY_LOG_ERROR(m_wafProcess->errorString().toUtf8().data());
                    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
                }
            });

        StartWafCommand("package", "--run-xcode-for-packaging=False");
    }
}

StringOutcome DeployWorkerIos::Launch()
{
    if (!LaunchShaderCompiler())
    {
        return AZ::Failure<AZStd::string>("An error occured while trying validate the Shader Compiler");
    }

    DEPLOY_LOG_INFO("[INFO] Deploying to device.  This may take some time...");

    const char* projectName = m_deploymentConfig.m_projectName.c_str();

    AZStd::string xcodebuildCommand = AZStd::move(AZStd::string::format(
        "xcodebuild test -project %s -scheme %sIOSLauncher -destination \"platform=iOS,id=%s\" -configuration %s -only-testing:%sIOSLauncherTests/LumberyardXCTestWrapperTests/testRuntime RUN_WAF_BUILD=NO",
        m_xcodeProject.c_str(),
        projectName,
        m_deploymentConfig.m_deviceId.c_str(),
        m_deploymentConfig.m_buildConfiguration.c_str(),
        projectName
    ));

    m_xcodebuildProcess->start(xcodebuildCommand.c_str());
    if (!m_xcodebuildProcess->waitForStarted())
    {
        return AZ::Failure<AZStd::string>("Failed to invoke xcodebuild");
    }

    return AZ::Success();
}
