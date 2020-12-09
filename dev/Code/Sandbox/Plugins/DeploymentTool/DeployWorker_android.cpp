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
#include "DeployWorker_android.h"

#include <QRegExp>

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include "DeployNotificationsBus.h"
#include "NetworkUtils.h"


namespace
{
    struct DeviceInfo
    {
        enum class Property : unsigned char
        {
            Manufacturer = 0,
            Model,
            Version,

            TOTAL,
        };

        DeviceInfo()
            : m_manufacturer("Unknown")
            , m_model("Unknown")
            , m_version("Unknown")
        {
        }

        void SetProperty(Property property, const QString& value)
        {
            switch (property)
            {
                case Property::Manufacturer:
                    m_manufacturer = value;
                    break;
                case Property::Model:
                    m_model = value;
                    break;
                case Property::Version:
                    m_version = value;
                    break;
                default:
                    break;
            }
        }

        QString m_manufacturer;
        QString m_model;
        QString m_version;
    };
}


DeployWorkerAndroid::DeployWorkerAndroid()
    : DeployWorkerBase(GetSystemConfigFileName())
    , m_adbPath()
    , m_package()
    , m_installAPK(false)
{
    rapidjson::Document environment;
    StringOutcome outcome = LoadJsonData("_WAF_/environment.json", environment);
    if (outcome.IsSuccess())
    {
        if (!environment.HasMember("LY_ANDROID_SDK") || !environment["LY_ANDROID_SDK"].IsString())
        {
            DEPLOY_LOG_ERROR("[ERROR] LY_ANDROID_SDK configuration is invalid in environment.json");
        }
        else
        {
            m_adbPath = AZStd::move(AZStd::string::format("%s/platform-tools/adb", environment["LY_ANDROID_SDK"].GetString()));

            // make sure the adb daemon is running
            RunBlockingCommand(m_adbPath.c_str(), { "start-server" });
        }
    }
    else
    {
        DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
    }
}

DeployWorkerAndroid::~DeployWorkerAndroid()
{
}

const char* DeployWorkerAndroid::GetSystemConfigFileName() const
{
    return "system_android_es3.cfg";
}

bool DeployWorkerAndroid::GetConnectedDevices(DeviceMap& connectedDevices) const
{
    if (m_adbPath.empty())
    {
        return false;
    }

    const char* adbPath = m_adbPath.c_str();

    QString devicesOutput;
    if (RunBlockingCommand(adbPath, { "devices" }, &devicesOutput))
    {
        const int devicePropertyCount = static_cast<int>(DeviceInfo::Property::TOTAL);
        const char* devicePropertyKeys[devicePropertyCount] = {
            "ro.product.manufacturer",
            "ro.product.model",
            "ro.build.version.release"
        };

        QStringList lines = devicesOutput.split('\n', Qt::SkipEmptyParts);
        for (const auto& line : lines)
        {
            QString trimmedLine = line.trimmed();

            if (    trimmedLine.isEmpty()
                ||  trimmedLine.startsWith("*")
                ||  trimmedLine.startsWith("list", Qt::CaseInsensitive)
                ||  trimmedLine.startsWith("emulator", Qt::CaseInsensitive))
            {
                continue;
            }

            // using a regular expression to account for spaces and tabs
            QStringList tokens = trimmedLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);

            QString deviceId = tokens[0];
            QString deviceStatus = tokens[1];

            if (   deviceStatus.compare("unauthorized", Qt::CaseInsensitive) == 0
                || deviceStatus.compare("offline", Qt::CaseInsensitive) == 0)
            {
                connectedDevices[deviceId] = QString("%1 [%2]").arg(deviceId, deviceStatus.toUpper());
            }
            else
            {
                DeviceInfo deviceInfo;
                for (int i = 0; i < devicePropertyCount; ++i)
                {
                    const char* propertyKey = devicePropertyKeys[i];

                    QString output;
                    if (RunBlockingCommand(adbPath, { "-s", deviceId, "shell", "getprop", propertyKey }, &output))
                    {
                        deviceInfo.SetProperty(static_cast<DeviceInfo::Property>(i), output.trimmed());
                    }
                }

                connectedDevices[deviceId] = QString("%1 %2 (Android %3)").arg(deviceInfo.m_manufacturer, deviceInfo.m_model, deviceInfo.m_version);
            }
        }
    }

    return true;
}

AZStd::string DeployWorkerAndroid::GetWafBuildArgs() const
{
    return "--deploy-android=False";
}

AZStd::string DeployWorkerAndroid::GetWafDeployArgs() const
{
    const char* cleanInstall = (m_deploymentConfig.m_cleanDevice ? "True" : "False");
    const char* installExe = (m_installAPK ? "True" : "False");
    const char* baseOptions = "--from-editor-deploy --deploy-android=True --deploy-android-asset-mode=loose";

    AZStd::string deploymentOptions = AZStd::move(
        AZStd::string::format("%s --deploy-android-clean-device=%s --deploy-android-executable=%s --deploy-android-replace-apk=%s --deploy-android-device-filter=%s",
                            baseOptions,
                            cleanInstall,
                            installExe,
                            installExe,
                            m_deploymentConfig.m_deviceId.c_str()
    ));

    return deploymentOptions;
}

StringOutcome DeployWorkerAndroid::Prepare()
{
    // Check to make sure the selected device is still connected
    const char* deviceId = m_deploymentConfig.m_deviceId.c_str();
    if (!DeviceIsConnected(deviceId))
    {
        return AZ::Failure<AZStd::string>("Device no longer connected");
    }

    if (m_adbPath.empty())
    {
        return AZ::Failure<AZStd::string>("Failed to locate adb");
    }

    const char* adbPath = m_adbPath.c_str();    

    // make sure the device is useable.  Under the following scenarios the device is...
    // command failure  - not authorized for use
    // offline          - not responding or formally connected to adb
    // bootloader       - currently running the bootloader
    // device           - authorized for use
    QString deviceStatus;
    bool ret = RunBlockingCommand(adbPath, { "-s", deviceId, "get-state" }, &deviceStatus);
    if (!ret || deviceStatus.trimmed() != "device")
    {
        return AZ::Failure<AZStd::string>("Target device is either not ready or authorized for use");
    }

    // if targeting armv8, make sure the device can run it
    if (m_deploymentConfig.m_platformOption == PlatformOptions::Android_ARMv8)
    {
        QString deviceAbi;
        if (!RunBlockingCommand(adbPath, { "-s", deviceId, "shell", "getprop", "ro.product.cpu.abi" }, &deviceAbi))
        {
            return AZ::Failure<AZStd::string>("Unexpected error occured during device validation");
        }

        if (deviceAbi.trimmed() != "arm64-v8a")
        {
            return AZ::Failure<AZStd::string>("Target device does not support ARMv8");
        }
    }

    // get the package name
    rapidjson::Document projectConfig;
    StringOutcome outcome = LoadJsonData((m_deploymentConfig.m_projectName + "/project.json"), projectConfig);
    if (!outcome.IsSuccess())
    {
        return outcome;
    }

    m_package = projectConfig["android_settings"]["package_name"].GetString();
    const char* packageName = m_package.c_str();

    // auto install when clean or build game is checked, otherwise check if the app is installed already
    m_installAPK = (m_deploymentConfig.m_cleanDevice || m_deploymentConfig.m_buildGame);
    if (!m_installAPK)
    {
        QString output;
        RunBlockingCommand(adbPath, { "-s", deviceId, "shell", "pm", "list", "packages", packageName }, &output);

        // empty output (after stripping preceding and trailing whitespace) indicates the package is not installed
        m_installAPK = output.trimmed().isEmpty();
    }

    // attempt to stop the app on the device
    RunBlockingCommand(adbPath, { "-s", deviceId, "shell", "am", "force-stop", packageName });

    return AZ::Success(AZStd::string());
}

StringOutcome DeployWorkerAndroid::Launch()
{
    using namespace DeployTool;

    if (!LaunchShaderCompiler())
    {
        return AZ::Failure<AZStd::string>("An error occurred while trying validate the Shader Compiler");
    }

    const char* adbPath = m_adbPath.c_str();
    const char* deviceId = m_deploymentConfig.m_deviceId.c_str();
    const QString tcpArgFormat("tcp:%1");

    if (    (m_deploymentConfig.m_useVFS || m_deploymentConfig.m_shaderCompilerUseAP)
        &&  IsLocalhost(m_deploymentConfig.m_assetProcessorIpAddress))
    {
        QString assetProcessorPort = tcpArgFormat.arg(m_deploymentConfig.m_assetProcessorPort.c_str());

        if (!RunBlockingCommand(adbPath, { "-s", deviceId, "reverse", assetProcessorPort, assetProcessorPort }))
        {
            return AZ::Failure<AZStd::string>("Failed to run adb reverse on the Asset Processor port");
        }
    }

    if (    !m_deploymentConfig.m_shaderCompilerUseAP
        &&  IsLocalhost(m_deploymentConfig.m_shaderCompilerIpAddress))
    {
        QString shaderCompilerPort = tcpArgFormat.arg(m_deploymentConfig.m_shaderCompilerPort.c_str());

        if (!RunBlockingCommand(adbPath, { "-s", deviceId, "reverse", shaderCompilerPort, shaderCompilerPort }))
        {
            return AZ::Failure<AZStd::string>("Failed to run adb reverse on the Shader Compiler port");
        }
    }

    if (IsLocalhost(m_deploymentConfig.m_deviceIpAddress))
    {
        QString localPort = tcpArgFormat.arg(m_deploymentConfig.m_hostRemoteLogPort);
        QString remotePort = tcpArgFormat.arg(m_deploymentConfig.m_deviceRemoteLogPort.c_str());

        if (!RunBlockingCommand(adbPath, { "-s", deviceId, "forward", localPort, remotePort }))
        {
            return AZ::Failure<AZStd::string>("Failed to run adb forward on the Remote Log port");
        }
    }

    // unlock the screen
    if (!RunBlockingCommand(adbPath, { "-s", deviceId, "shell", "input", "keyevent", "82" }))
    {
        return AZ::Failure<AZStd::string>("Failed to unlock screen");
    }

    QStringList launchAppArgs = {
        "-s", deviceId, "shell", "am", "start", "-n", QString("%1/.%2Activity").arg(m_package.c_str(), m_deploymentConfig.m_projectName.c_str())
    };
    if (!RunBlockingCommand(adbPath, launchAppArgs))
    {
        return AZ::Failure<AZStd::string>("Failed to launch application");
    }

    return AZ::Success(AZStd::string());
}
