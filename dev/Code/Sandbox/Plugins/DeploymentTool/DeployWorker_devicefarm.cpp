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
#include "DeployWorker_devicefarm.h"
#include "DeployNotificationsBus.h"
#include <AzCore/Component/ComponentApplicationBus.h>

namespace DeployTool
{

DeployWorkerDeviceFarm::DeployWorkerDeviceFarm(
    std::shared_ptr<DeployTool::DeviceFarmDriver> deviceFarmDriver,
    int timeoutInMinutes,
    const AZStd::string& projectArn,
    const AZStd::string& devicePoolArn)
    : DeployWorkerBase(GetSystemConfigFileName())
    , m_deviceFarmDriver(deviceFarmDriver)
    , m_timeoutInMinutes(timeoutInMinutes)
    , m_projectArn(projectArn)
    , m_devicePoolArn(devicePoolArn)
{
}

DeployWorkerDeviceFarm::~DeployWorkerDeviceFarm()
{
    if (m_deployWorkerThread.joinable())
    {
        m_deployWorkerThread.join();
    }
}

void DeployWorkerDeviceFarm::DeployStateUpdateUploadCreate()
{
    // Create an upload slot, and kick off the deploy state machine.
    m_deviceFarmDeployState = DeviceFarmDeployState::UploadCreating;
    m_deviceFarmDriver->CreateUpload(m_appPath,
        m_projectArn,
        [&](bool success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Upload& upload)
    {
        if (success)
        {
            m_deviceFarmUpload = upload;
            m_deviceFarmDeployState = DeviceFarmDeployState::UploadCreated;
        }
        else
        {
            DEPLOY_LOG_ERROR("[ERROR] Failed to create upload slot. %s", msg.c_str());
            m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
        }
    });
}

void DeployWorkerDeviceFarm::DeployStateUpdateUploadCreated()
{
    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessStatusChange, "Uploading");
    DEPLOY_LOG_INFO("[INFO] Uploading %s ...", m_deviceFarmUpload.name.c_str());
    m_deviceFarmDeployState = DeviceFarmDeployState::UploadSending;
    if (!m_deviceFarmDriver->SendUpload(m_deviceFarmUpload))
    {
        DEPLOY_LOG_ERROR("[ERROR] Failed to send upload %s", m_deviceFarmUpload.name.c_str());
        m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
    }
    else
    {
        // Upload sent, issue a GetUpload to check the status.
        m_deviceFarmDeployState = DeviceFarmDeployState::UploadCheckingStatus;
        m_deviceFarmDriver->GetUpload(
            m_deviceFarmUpload.arn,
            [&](bool success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Upload& upload)
        {
            if (success)
            {
                m_deviceFarmUpload = upload;
                m_deviceFarmDeployState = DeviceFarmDeployState::UploadCheckedStatus;
            }
            else
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to get upload for status update. %s", msg.c_str());
                m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
            }
        });
    }
}

void DeployWorkerDeviceFarm::DeployStateUpdateUploadCheckedStatus()
{
    // If the upload succeeded, schedule the Run.
    if (m_deviceFarmUpload.status == "SUCCEEDED")
    {
        // Auto generate a run name with a date/time stamp
        const AZStd::string runName = AZStd::string::format("LY Run %s", QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss").toUtf8().data());

        // Upload finished, schedule the run
        m_deviceFarmDeployState = DeviceFarmDeployState::SchedulingRun;
        m_deviceFarmDriver->ScheduleRun(
            runName,
            m_deviceFarmUpload.arn,
            m_projectArn,
            m_devicePoolArn,
            m_timeoutInMinutes,
            [&](bool success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Run& run)
        {
            if (success)
            {
                DEPLOY_LOG_INFO("[INFO] Run '%s' successfully scheduled.", run.name.c_str());
                m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
            }
            else
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to schedule run. %s", msg.c_str());
                m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
            }
        });
    }
    else if (m_deviceFarmUpload.status == "FAILED")
    {
        DEPLOY_LOG_ERROR("[ERROR] Failed to upload %s", m_deviceFarmUpload.name.c_str());
        m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
    }
    else
    {
        // Still pending, issue a new GetUpload to check status
        m_deviceFarmDeployState = DeviceFarmDeployState::UploadCheckingStatus;
        m_deviceFarmDriver->GetUpload(m_deviceFarmUpload.arn,
            [&](bool success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Upload& upload)
        {
            if (success)
            {
                m_deviceFarmUpload = upload;
                m_deviceFarmDeployState = DeviceFarmDeployState::UploadCheckedStatus;
            }
            else
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to get upload for status update. %s", msg.c_str());
                m_deviceFarmDeployState = DeviceFarmDeployState::Idle;
            }
        });
    }
}

void DeployWorkerDeviceFarm::Update()
{
    if (m_deviceFarmDeployState == DeviceFarmDeployState::UploadCreate)
    {
        DeployStateUpdateUploadCreate();
    }
    // Upload was created, try to Send the Upload
    else if (m_deviceFarmDeployState == DeviceFarmDeployState::UploadCreated)
    {
        DeployStateUpdateUploadCreated();
    }
    // We have the result of the last GetUpload status update, check to see if it is ready.
    else if (m_deviceFarmDeployState == DeviceFarmDeployState::UploadCheckedStatus)
    {
        DeployStateUpdateUploadCheckedStatus();
    }
}

const char* DeployWorkerDeviceFarm::GetSystemConfigFileName() const
{
    return m_deploymentConfig.m_platformOption == PlatformOptions::iOS ? "system_ios_ios.cfg" : "system_android_es3.cfg";
}

bool DeployWorkerDeviceFarm::GetConnectedDevices(DeviceMap& connectedDevices) const
{
    // N/A for Device Farm
    return true;
}

AZStd::string DeployWorkerDeviceFarm::GetWafBuildArgs() const
{
    return m_deploymentConfig.m_platformOption == PlatformOptions::iOS
        ? "--package-projects-automatically=True --run-xcode-for-packaging=True"
        : "--deploy-android=False --android-asset-mode=apk_files";
}

AZStd::string DeployWorkerDeviceFarm::GetWafDeployArgs() const
{
    // N/A on Device Farm
    return "";
}

StringOutcome DeployWorkerDeviceFarm::Prepare()
{
    return AZ::Success(AZStd::string());
}

void DeployWorkerDeviceFarm::StartDeploy()
{
    QObject::disconnect(m_finishedConnection);

    // If the game build was skipped, call the package step here before deploying.
    // If the game build was not skipped, the package step would would have already
    // during the build.
    if (!m_deploymentConfig.m_buildGame)
    {
        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessStatusChange, "Packaging");
        m_finishedConnection = QObject::connect(m_wafProcess,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), // use static_cast to specify which overload of QProcess::finished to use
            [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            if (exitStatus == QProcess::NormalExit && exitCode == 0)
            {
                // No Waf deploy here, just call Launch to start Device Farm deploy.
                StringOutcome launchOutcome = Launch();
                if (!launchOutcome.IsSuccess())
                {
                    DEPLOY_LOG_ERROR(launchOutcome.GetError().c_str());
                    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
                }
            }
            else
            {
                DEPLOY_LOG_ERROR(m_wafProcess->errorString().toUtf8().data());
                DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
            }
        });

        StartWafCommand("package", GetWafBuildArgs());
    }
    else
    {
        // No Waf deploy here, just call Launch to start Device Farm deploy.
        StringOutcome launchOutcome = Launch();
        if (!launchOutcome.IsSuccess())
        {
            DEPLOY_LOG_ERROR(launchOutcome.GetError().c_str());
            DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
        }
    }
}

StringOutcome DeployWorkerDeviceFarm::Launch()
{
    DEPLOY_LOG_INFO("[INFO] Launching Device Farm Deploy");

    const AZStd::string outputFolderGroup = "Output Folder";

    // Get the platform specific Output Folder key
    AZStd::string platformOutputFolderKey;
    if (m_deploymentConfig.m_platformOption == PlatformOptions::Android_ARMv7)
    {
        platformOutputFolderKey = "out_folder_android_armv7_clang";
    }
    else if (m_deploymentConfig.m_platformOption == PlatformOptions::Android_ARMv8)
    {
        platformOutputFolderKey = "out_folder_android_armv8_clang";
    }
    else if (m_deploymentConfig.m_platformOption == PlatformOptions::iOS)
    {
        platformOutputFolderKey = "out_folder_ios";
    }
    else
    {
        DEPLOY_LOG_ERROR("[ERROR] Unknown platform detected.");
        return AZ::Failure<AZStd::string>("Unknown platform detected.");
    }

    // Read the output folder setting.
    StringOutcome appOutputFolderNameOutcome = GetUserSettingsValue(outputFolderGroup.c_str(), platformOutputFolderKey.c_str(), m_deploymentConfig.m_platformOption);
    if (!appOutputFolderNameOutcome.IsSuccess())
    {
        // Fall back and look in the attribute section of the platform specific config file.
        appOutputFolderNameOutcome = GetPlatformSpecficDefaultAttributeValue("default_folder_name", m_deploymentConfig.m_platformOption);
        if (!appOutputFolderNameOutcome.IsSuccess())
        {
            // Fall back failed, return the failed outcome.
            return appOutputFolderNameOutcome;
        }
    }
    AZStd::string appOutputFolderName = appOutputFolderNameOutcome.GetValue();

    // Read the output folder ext setting.
    AZStd::string platformOutputFolderExtKey = AZStd::string::format("output_folder_ext_%s", m_deploymentConfig.m_buildConfiguration.c_str());
    StringOutcome appOutputFolderExtNameOutcome = GetUserSettingsValue(outputFolderGroup.c_str(), platformOutputFolderExtKey.c_str(), m_deploymentConfig.m_platformOption);
    if (!appOutputFolderExtNameOutcome.IsSuccess())
    {
        // Fall back and look in the attribute section of the platform specific config file.
        appOutputFolderExtNameOutcome = GetCommonBuildConfigurationsDefaultSettingsValue(m_deploymentConfig.m_buildConfiguration.c_str(), "default_output_ext");
        if (!appOutputFolderExtNameOutcome.IsSuccess())
        {
            // Fall back failed, return the failed outcome.
            return appOutputFolderExtNameOutcome;
        }
    }

    AZStd::string appOutputFolderExtName = appOutputFolderExtNameOutcome.GetValue();

    // Add the target .Release, .Debug extension to the output folder name if it is not empty.
    if (!appOutputFolderExtName.empty())
    {
        appOutputFolderName = AZStd::string::format("%s.%s", appOutputFolderName.c_str(), appOutputFolderExtName.c_str());
    }

    // Construct the app filename only without the path.
    const AZStd::string appFilename = AZStd::string::format(
        "%sLauncher%s.%s",
        m_deploymentConfig.m_projectName.c_str(),
        m_deploymentConfig.m_platformOption == PlatformOptions::iOS ? "" : "_w_assets",
        m_deploymentConfig.m_platformOption == PlatformOptions::iOS ? "app" : "apk");

    // Create the path to the app
    AzFramework::StringFunc::Path::Join(appOutputFolderName.c_str(), appFilename.c_str(), m_appPath);

    // Get the root path of the app
    const char* rootPath = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(rootPath, &AZ::ComponentApplicationRequests::GetAppRoot);
    if (!rootPath)
    {
        return AZ::Failure<AZStd::string>("Failed to get app root.");
    }

    // The upload requires a full path.
    AzFramework::StringFunc::Path::Join(rootPath, m_appPath.c_str(), m_appPath);

    // The first state that kicks off the process.
    m_deviceFarmDeployState = DeviceFarmDeployState::UploadCreate;

    // Kick off the deploy on a thread so the UI is not locked up.
    m_deployWorkerThread = AZStd::thread([this]()
    {
        // Go through the launch stages.
        Update();
        while (m_deviceFarmDeployState != DeviceFarmDeployState::Idle)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            Update();
        }

        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, true);
    });
    
    return AZ::Success(AZStd::string());
}

} // namespace DeployTool