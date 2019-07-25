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
#include "DeployWorkerBase.h"

#include <QProcess>

#include <AzCore/Socket/AzSocket.h>
#include <AzCore/std/string/conversions.h> // for stoul

#include "DeployNotificationsBus.h"
#include "NetworkUtils.h"
#include "JsonPreProcessor.h"

namespace
{
#if defined(AZ_PLATFORM_WINDOWS)
    const char* shaderCompilerPathRoot = "Tools/CrySCompileServer/x64/profile";
    const char* wafCommand = "lmbr_waf.bat";
#else
    const char* shaderCompilerPathRoot = "Tools/CrySCompileServer/osx/profile";
    const char* wafCommand = "sh lmbr_waf.sh";
#endif

    const char* shaderCompilerExeName = "CrySCompileServer";
    const char* shaderCompilerConfigFile = "config.ini";


    const char* PlatformOptionToWafTarget(PlatformOptions platform)
    {
        switch (platform)
        {
            case PlatformOptions::Android_ARMv7:
                return "android_armv7_clang";
            case PlatformOptions::Android_ARMv8:
                return "android_armv8_clang";

        #if defined(AZ_PLATFORM_APPLE_OSX)
            case PlatformOptions::iOS:
                return "ios";
        #endif

            default:
                return nullptr;
        }
    }
}


DeployWorkerBase::DeployWorkerBase(const char* systemConfigFile)
    : m_deploymentConfig()
    , m_systemConfig(systemConfigFile)
    , m_wafProcess(new QProcess())
    , m_finishedConnection()
{
    m_wafProcess->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(m_wafProcess, &QProcess::readyReadStandardOutput,
        [this]()
        {
            const QString msg(m_wafProcess->readAllStandardOutput());
            DEPLOY_LOG_INFO("%s", msg.toUtf8().data());
        });
}

DeployWorkerBase::~DeployWorkerBase()
{
    AZ_Warning("DeploymentTool", m_wafProcess->state() == QProcess::NotRunning, "Deployment process not completed, terminating!");

    QObject::disconnect(m_finishedConnection);

    m_wafProcess->kill();
    delete m_wafProcess;
}

StringOutcome DeployWorkerBase::ApplyConfiguration(const DeploymentConfig& deployConfig)
{
    m_deploymentConfig = deployConfig;

    StringOutcome outcome = m_systemConfig.Load();
    if (outcome.IsSuccess())
    {
        outcome = m_systemConfig.ApplyConfiguration(m_deploymentConfig);
    }

    return outcome;
}

void DeployWorkerBase::Run()
{
    AZ_Assert(!m_isRunning, "Worker is already running.");

    m_isRunning = true;

    StringOutcome outcome = Prepare();
    if (!outcome.IsSuccess())
    {
        DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
        m_isRunning = false;
        return;
    }

    if (m_deploymentConfig.m_buildGame)
    {
        StartBuildAndDeploy();
    }
    else
    {
        StartDeploy();
    }

    m_isRunning = false;
}

bool DeployWorkerBase::IsRunning() 
{
    return m_isRunning;
}

bool DeployWorkerBase::DeviceIsConnected(const char* deviceId)
{
    AZ_Assert(deviceId, "Expected valid deviceId");
    DeviceMap connectedDevices;
    GetConnectedDevices(connectedDevices);
    return connectedDevices.contains(QString(deviceId));
}

void DeployWorkerBase::StartBuildAndDeploy()
{
    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessStatusChange, "Building");

    QObject::disconnect(m_finishedConnection);

    m_finishedConnection = QObject::connect(m_wafProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), // use static_cast to specify which overload of QProcess::finished to use
        [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            if (exitStatus == QProcess::NormalExit && exitCode == 0)
            {
                StartDeploy();
            }
            else
            {
                DEPLOY_LOG_ERROR(m_wafProcess->errorString().toUtf8().data());
                DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
            }
        });

    StartWafCommand("build", GetWafBuildArgs());
}

void DeployWorkerBase::StartDeploy()
{
    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessStatusChange, "Deploying");

    QObject::disconnect(m_finishedConnection);

    m_finishedConnection = QObject::connect(m_wafProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), // use static_cast to specify which overload of QProcess::finished to use
        [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            bool isSuccess = false;
            if (exitStatus == QProcess::NormalExit && exitCode == 0)
            {
                StringOutcome outcome = Launch();
                isSuccess = outcome.IsSuccess();

                if (!isSuccess)
                {
                    DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
                }
            }
            else
            {
                DEPLOY_LOG_ERROR(m_wafProcess->errorString().toUtf8().data());
            }

            DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, isSuccess);
        });

    StartWafCommand("deploy", GetWafDeployArgs());
}

bool DeployWorkerBase::LaunchShaderCompiler() const
{
    AZStd::vector<AZStd::string> localIpAddrs;
    if (!DeployTool::GetAllHostIPAddrs(localIpAddrs))
    {
        DEPLOY_LOG_WARN("[WARN] Failed to get the host computer's local IP addresses.  If the specified IP address is local to the host machine, the shader compiler may not be properly launched.");
    }

    bool isLocalIpAddr = false;
    for (const auto& ipAddr : localIpAddrs)
    {
        if (m_deploymentConfig.m_shaderCompilerIpAddress.compare(ipAddr) == 0)
        {
            isLocalIpAddr = true;
            break;
        }
    }

    // Try to connect to the shader compiler
    AZSOCKET socket = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(socket))
    {
        DEPLOY_LOG_ERROR("[ERROR] Could not create valid socket!");
        return false;
    }

    AZ::AzSock::AzSocketAddress socketAddress;
    if (!socketAddress.SetAddress(m_deploymentConfig.m_shaderCompilerIpAddress, AZStd::stoul(m_deploymentConfig.m_shaderCompilerPort)))
    {
        DEPLOY_LOG_ERROR("[ERROR] Could not set socket address!");
        return false;
    }

    AZ::s32 result = AZ::AzSock::Connect(socket, socketAddress);
    if (AZ::AzSock::SocketErrorOccured(result))
    {
        if (isLocalIpAddr)
        {
            // search for the most recent version of the shader compiler detected, this is mostly for windows
            // where there can be multiple versions built with different compilers
            QDir shaderCompilerRootDir(shaderCompilerPathRoot);
            QFileInfoList executables = shaderCompilerRootDir.entryInfoList(QDir::Files | QDir::Executable, QDir::Time);

            const QFileInfo* shaderCompilerFile = nullptr;
            for (const QFileInfo& exe : executables)
            {
                if (exe.fileName().startsWith(shaderCompilerExeName, Qt::CaseInsensitive))
                {
                    shaderCompilerFile = &exe;
                    break;
                }
            }

            if (!shaderCompilerFile)
            {
                DEPLOY_LOG_ERROR("[ERROR] Unable locate the remote shader compiler!");
                return false;
            }

            // update the shader compiler configuration
            QString configFile = shaderCompilerRootDir.filePath(shaderCompilerConfigFile);

            ConfigFileContainer shaderCompilerConfig(configFile.toUtf8().data());
            shaderCompilerConfig.Load(); // doesn't matter if load fails, ie. file doesn't exit

            shaderCompilerConfig.SetString("port", m_deploymentConfig.m_shaderCompilerPort.c_str());

            if (!DeployTool::IsLocalhost(m_deploymentConfig.m_deviceIpAddress))
            {
                const char* whitelistKey = "whitelist";
                const AZStd::string& deviceIp = m_deploymentConfig.m_deviceIpAddress;

                QString whitelist(shaderCompilerConfig.GetString(whitelistKey).c_str());

                QStringList ipAddresses = whitelist.split(',', QString::SkipEmptyParts);
                if (!ipAddresses.contains(deviceIp.c_str()))
                {
                    ipAddresses.append(deviceIp.c_str());
                }

                whitelist = ipAddresses.join(',');
                shaderCompilerConfig.SetString(whitelistKey, whitelist.toUtf8().data());
            }

            StringOutcome outcome = shaderCompilerConfig.Write();
            if (!outcome.IsSuccess())
            {
                DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
                return false;
            }

            // launch the shader compiler
            QString shaderCompilerRoot(shaderCompilerFile->absolutePath());
            QString shaderCompiler(shaderCompilerFile->absoluteFilePath());

            DEPLOY_LOG_INFO("[INFO] Starting shader compiler...");
            DEPLOY_LOG_DEBUG("[DEBUG] %s", shaderCompiler.toUtf8().data());

        #if defined(AZ_PLATFORM_WINDOWS)
            QString program = shaderCompiler;
            QStringList args;
        #else
            // launching the shader compiler directly will make it a background process only visible to activity monitor
            // the "g" option will prevent the window from getting focus after launch
            QString program = "open";
            QStringList args { "-g", shaderCompiler };
        #endif

            if (!QProcess::startDetached(program, args, shaderCompilerRoot))
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to launch shader compiler");
                return false;
            }
        }
        else
        {
            DEPLOY_LOG_ERROR("[ERROR] Unable to validate connection to specified shader compiler IP address and port!");
            return false;
        }
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
            DEPLOY_LOG_ERROR("[ERROR] Could not send request to shader compiler!");
            return false;
        }

        // We were able to connect to the shader compiler. So, it's already running.
        DEPLOY_LOG_INFO("[INFO] Valid shader compiler detected.");

        // Close the connection and move on.
        result = AZ::AzSock::Shutdown(socket, SD_BOTH);
        if (AZ::AzSock::SocketErrorOccured(result))
        {
            DEPLOY_LOG_WARN("[WARN] Could not shutdown socket!");
        }
        result = AZ::AzSock::CloseSocket(socket);
        if (AZ::AzSock::SocketErrorOccured(result))
        {
            DEPLOY_LOG_WARN("[WARN] Could not close socket!");
        }
    }

    return true;
}

void DeployWorkerBase::StartWafCommand(const char* commandType, const AZStd::string& commandArgs)
{
    const char* wafTarget = PlatformOptionToWafTarget(m_deploymentConfig.m_platformOption);
    if (!wafTarget)
    {
        DEPLOY_LOG_ERROR("[ERROR] Failed to deduce WAF build target from selected platform!");
        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
        return;
    }

    AZStd::string wafCmd = AZStd::move(
        AZStd::string::format("%s %s_%s_%s --enabled-game-projects=%s -pall %s",
                            wafCommand,
                            commandType,
                            wafTarget,
                            m_deploymentConfig.m_buildConfiguration.c_str(),
                            m_deploymentConfig.m_projectName.c_str(),
                            commandArgs.c_str()));

    DEPLOY_LOG_INFO("[INFO] Running WAF command: %s", wafCmd.c_str());
    m_wafProcess->start(wafCmd.c_str());

    if (!m_wafProcess->waitForStarted())
    {
        DEPLOY_LOG_ERROR("%s", m_wafProcess->errorString().toUtf8().data());
        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
    }
}

bool DeployWorkerBase::RunBlockingCommand(const AZStd::string& command, QString* output) const
{
    DEPLOY_LOG_DEBUG("[DEBUG] Running %s", command.c_str());

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    process.start(command.c_str());

    if (!process.waitForStarted() || !process.waitForFinished())
    {
        return false;
    }

    if ((process.exitStatus() != QProcess::NormalExit) || (process.exitCode() != 0))
    {
        return false;
    }

    if (output)
    {
        *output = process.readAllStandardOutput();
    }

    return true;
}

StringOutcome DeployWorkerBase::LoadJsonData(const AZStd::string& file, rapidjson::Document& jsonData)
{
    AZStd::string fileContents;
    StringOutcome outcome = ReadFile(file.c_str(), fileContents);
    if (!outcome.IsSuccess())
    {
        return outcome;
    }

    // Use the pre processor to remove comments.
    DeployTool::JsonPreProcessor jsonPreProcessor(fileContents.c_str());
    jsonData.Parse(jsonPreProcessor.GetResult().c_str());

    return AZ::Success(AZStd::string());
}

StringOutcome DeployWorkerBase::GetUserSettingsValue(const char* groupName, const char* keyName, PlatformOptions platformOption)
{
    AZ_Assert(groupName, "Expected valid groupName");
    AZ_Assert(keyName, "Expected valid keyName");

    // Look in the user settings first, then fall back to default setting if not found.
    const char* userSettingsFile = "_WAF_/user_settings.options";

    // Get the platform specific Output Folder key
    AZStd::string platformOutputFolderKey;
    if (platformOption == PlatformOptions::Android_ARMv7)
    {
        platformOutputFolderKey = "out_folder_android_armv7_clang";
    }
    else if (platformOption == PlatformOptions::Android_ARMv8)
    {
        platformOutputFolderKey = "out_folder_android_armv8_clang";
    }
    else if (platformOption == PlatformOptions::iOS)
    {
        platformOutputFolderKey = "out_folder_ios";
    }
    else
    {
        DEPLOY_LOG_ERROR("[ERROR] Unknown platform detected.");
        return AZ::Failure<AZStd::string>("Unknown platform detected.");
    }

    bool foundValue = false;
    AZStd::string resultValue;

    // Check the user settings file.
    if (QFile::exists(userSettingsFile))
    {
        QSettings userWafSetting(userSettingsFile, QSettings::IniFormat);
        if (userWafSetting.status() == QSettings::NoError)
        {
            const QString keyFormat("%1/%2");
            QString outputFolderKey = keyFormat.arg(groupName, keyName);
            if (userWafSetting.contains(outputFolderKey))
            {
                resultValue = userWafSetting.value(outputFolderKey).toByteArray().data();
                foundValue = true;
            }
        }
    }

    // Make sure the setting value was found
    if (!foundValue)
    {
        AZStd::string msg = AZStd::string::format(
            "Failed to get setting value group: '%s' key: '%s' from user_settings.options.",
            groupName,
            keyName);

        return AZ::Failure<AZStd::string>(AZStd::move(msg));
    }

    return AZ::Success(resultValue);
}

AZStd::string DeployWorkerBase::GetPlatformSpecficDefaultSettingsFilename(PlatformOptions platformOption, const char* devRoot)
{
    AZStd::string defaultSettingsFile;
    if (platformOption == PlatformOptions::Android_ARMv7)
    {
        defaultSettingsFile = AZStd::string::format("%s/_WAF_/settings/platforms/platform.android_armv7_clang.json", devRoot);
    }
    else if (platformOption == PlatformOptions::Android_ARMv8)
    {
        defaultSettingsFile = AZStd::string::format("%s/_WAF_/settings/platforms/platform.android_armv8_clang.json", devRoot);
    }
    else if (platformOption == PlatformOptions::iOS)
    {
        defaultSettingsFile = AZStd::string::format("%s/_WAF_/settings/platforms/platform.ios.json", devRoot);
    }
    else
    {
        AZ_Error("DeploymentTool", false, "[ERROR] Unknown platform detected in GetPlatformSpecficDefaultSettingsFilename.", devRoot);
    }
    return defaultSettingsFile;
}

StringOutcome DeployWorkerBase::GetPlatformSpecficDefaultAttributeValue(const char* name, PlatformOptions platformOption, const char* devRoot)
{
    AZ_Assert(name, "Expected valid name of attribute");

    AZStd::string defaultSettingsFile = GetPlatformSpecficDefaultSettingsFilename(platformOption, devRoot);

    AZStd::string resultValue;

    rapidjson::Document defaultWafSettings;
    StringOutcome outcome = LoadJsonData(defaultSettingsFile, defaultWafSettings);
    if (!outcome.IsSuccess())
    {
        return outcome;
    }

    // Read the value from "attributes" : { "default_folder_name" : value }
    const char* attributesMemberName = "attributes";
    if (defaultWafSettings.HasMember(attributesMemberName))
    {
        if (defaultWafSettings[attributesMemberName].HasMember(name))
        {
            resultValue = defaultWafSettings[attributesMemberName][name].GetString();
        }
        else
        {
            AZStd::string msg = AZStd::string::format("Failed to find '%s' in %s section of config file '%s'",
                name,
                attributesMemberName,
                defaultSettingsFile.c_str());
            return AZ::Failure<AZStd::string>(AZStd::move(msg));
        }
    }
    else    
    {
        AZStd::string msg = AZStd::string::format("Failed to find '%s' member in config file '%s'", attributesMemberName, defaultSettingsFile.c_str());
        return AZ::Failure<AZStd::string>(AZStd::move(msg));
    }

    return AZ::Success(resultValue);
}

StringOutcome DeployWorkerBase::GetPlatformSpecficDefaultSettingsValue(const char* groupName, const char* keyName, PlatformOptions platformOption, const char* devRoot)
{
    AZ_Assert(groupName, "Expected valid groupName");
    AZ_Assert(keyName, "Expected valid keyName");

    AZStd::string defaultSettingsFile = GetPlatformSpecficDefaultSettingsFilename(platformOption, devRoot);

    rapidjson::Document defaultWafSettings;
    StringOutcome outcome = LoadJsonData(defaultSettingsFile, defaultWafSettings);
    if (!outcome.IsSuccess())
    {
        return outcome;
    }

    const char* settingKey = "settings";
    const char* defaultValueKey = "default_value";
    const char* attributeKey = "attribute";

    bool foundValue = false;
    AZStd::string resultValue;

    if (defaultWafSettings.HasMember(groupName))
    {
        for (auto iter = defaultWafSettings[groupName].Begin(); iter != defaultWafSettings[groupName].End(); ++iter)
        {
            const auto& entry = *iter;

            if (strcmp(entry[attributeKey].GetString(), keyName) == 0)
            {
                resultValue = entry[defaultValueKey].GetString();
                foundValue = true;
            }
        }
    }

    if (!foundValue)
    {
        AZStd::string msg = AZStd::string::format("Failed to find '%s' in %s section of config file '%s'",
            keyName,
            groupName,
            defaultSettingsFile.c_str());
        return AZ::Failure<AZStd::string>(AZStd::move(msg));
    }

    return AZ::Success(resultValue);
}

StringOutcome DeployWorkerBase::GetCommonBuildConfigurationsDefaultSettingsValue(const char* buildConfiguration, const char* name, const char* devRoot)
{
    AZ_Assert(buildConfiguration, "Expected valid name of build configuration");
    AZ_Assert(name, "Expected valid name of setting");
    AZ_Assert(devRoot, "Expected valid dev root");

    AZStd::string defaultSettingsFile = AZStd::string::format("%s/_WAF_/settings/build_configurations.json", devRoot);

    AZStd::string resultValue;

    rapidjson::Document defaultWafSettings;
    StringOutcome outcome = LoadJsonData(defaultSettingsFile, defaultWafSettings);
    if (!outcome.IsSuccess())
    {
        return outcome;
    }

    // Read the value from "configurations" : { buildConfiguration : { "default_output_ext" : value } }
    const char* configurationsMemberName = "configurations";
    if (defaultWafSettings.HasMember(configurationsMemberName))
    {
        if (defaultWafSettings[configurationsMemberName].HasMember(buildConfiguration))
        {
            if (defaultWafSettings[configurationsMemberName][buildConfiguration].HasMember(name))
            {
                resultValue = defaultWafSettings[configurationsMemberName][buildConfiguration][name].GetString();
            }
            else
            {
                AZStd::string msg = AZStd::string::format("Failed to find '%s' in '%s' in '%s' section of config file '%s'",
                    name,
                    buildConfiguration,
                    configurationsMemberName,
                    defaultSettingsFile.c_str());
                return AZ::Failure<AZStd::string>(AZStd::move(msg));
            }
        }
        else
        {
            AZStd::string msg = AZStd::string::format("Failed to find '%s' in '%s' section of config file '%s'",
                buildConfiguration,
                configurationsMemberName,
                defaultSettingsFile.c_str());
            return AZ::Failure<AZStd::string>(AZStd::move(msg));
        }
    }
    else
    {
        AZStd::string msg = AZStd::string::format("Failed to find '%s' member in config file '%s'", configurationsMemberName, defaultSettingsFile.c_str());
        return AZ::Failure<AZStd::string>(AZStd::move(msg));
    }

    return AZ::Success(resultValue);
}
