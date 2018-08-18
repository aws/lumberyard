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

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qabstractsocket.h>

#include "DeploymentToolWindow.h"
#include "CommandLauncher.h"
#include "BootstrapConfigContainer.h"
#include "AndroidDeploymentUtil.h"
#include "EditorDefs.h"
#include "Settings.h"
#include "ui_DeploymentToolWindow.h"

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <Util/PathUtil.h>
#include <stdio.h>

namespace
{
    const int waitTimeForAssetCatalogUpdateInMilliseconds = 500;
    const char* hardwarePlatformKeyString = "hardwarePlatform";
    const char* buildConfigKeyString = "buildConfiguration";
    const char* fileTransferKeyString = "fileTransferSetting";

    const char* buildGameKeyString = "buildGame";
    const char* clearDeviceKeyString = "clearDevice";
    const char* installExeKeyString = "installExe";
}

const char* DeploymentToolWindow::s_settingsFileName = "DeploymentToolConfig.ini";
class AssetSystemListener
    : public AzFramework::AssetSystemInfoBus::Handler
{

public:
    AssetSystemListener()
    {
        AzFramework::AssetSystemInfoBus::Handler::BusConnect();
    }

    ~AssetSystemListener()
    {
        AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();
    }

    int GetJobsCount() const
    {
        return m_pendingJobsCount;
    }

    AZStd::string LastAssetProcessorTask() const
    {
        return m_lastAssetProcessorTask;
    }

    virtual void AssetCompilationSuccess(const AZStd::string& assetPath) override
    {
        m_lastAssetProcessorTask = assetPath;
    }

    virtual void AssetCompilationFailed(const AZStd::string& assetPath) override
    {
        m_failedJobs.insert(assetPath);
    }

    virtual void CountOfAssetsInQueue(const int& count) override
    {
        m_pendingJobsCount = count;
    }

private:
    int m_pendingJobsCount = 0;
    std::set<AZStd::string> m_failedJobs;
    AZStd::string m_lastAssetProcessorTask;
};

DeploymentToolWindow::DeploymentToolWindow()
    : m_ui(new Ui::DeploymentToolWindow())
    , m_closedIcon("://group_closed.png")
    , m_openIcon("://group_open.png")
    , m_animatedSpinningIcon("://spinner_2x_V1.gif")
    , m_deploySettings(s_settingsFileName, QSettings::IniFormat)
    , m_assetSystemListener(new AssetSystemListener())
    , m_deploymentUtil(nullptr)
    , m_hideAdvancedOptions(true)
{
    StringOutcome outcome = m_bootstrapConfig.ReadContents();
    if (!outcome.IsSuccess())
    {
        LogEndLine(outcome.GetError());
    }

    SetupUi();
    InitializeUi();
    RestoreUiState();

    GetIEditor()->RegisterNotifyListener(this);
}

DeploymentToolWindow::~DeploymentToolWindow()
{
    SaveUiState();

    GetIEditor()->UnregisterNotifyListener(this);

    delete m_deploymentUtil;
}

void DeploymentToolWindow::ToggleAdvanceOptions()
{
    m_hideAdvancedOptions = !m_hideAdvancedOptions;
    m_ui->advancedOptionsWidget->setHidden(m_hideAdvancedOptions);
    m_ui->advanceOptionsButton->setIcon(m_hideAdvancedOptions? m_closedIcon : m_openIcon);
}

void DeploymentToolWindow::Deploy()
{
    // Create our listener here so that we can determine when AssetProcessor has 
    // finished processing all of the assets that we write to so that the deploy 
    // command won't fail due to file permission errors.

    m_ui->deployButton->setText("Deploying...");
    m_ui->deployButton->setEnabled(false);
    m_ui->outputTextEdit->setPlainText("");

    m_animatedSpinningIcon.start();
    m_ui->deploySpinnerLabel->setHidden(false);

    if (!PopulateDeploymentConfigFromUi())
    {
        Error("Invalid deployment config!");
        return;
    }

    // configure bootstrap.cfg
    StringOutcome outcome = m_bootstrapConfig.ReadContents();
    if (!outcome.IsSuccess())
    {
        LogEndLine(outcome.GetError());
        return;
    }

    // get project name
    m_deploymentCfg.m_projectName = m_bootstrapConfig.GetGameFolder();
    if (m_deploymentCfg.m_useVFS)
    {
        outcome = m_bootstrapConfig.ConfigureForVFSUsage(m_deploymentCfg.m_assetProcessorRemoteIpAddress, m_deploymentCfg.m_assetProcessorRemoteIpPort);
    }
    else
    {
        outcome = m_bootstrapConfig.Reset();
    }
    
    if (!outcome.IsSuccess())
    {
        LogEndLine(outcome.GetError());
        return;
    }
    
    WaitForAssetProcessorToFinish();

    // get platform specific deployment util
    switch (m_deploymentCfg.m_platformOptions)
    {
    case PlatformOptions::Android:
        m_deploymentUtil = new AndroidDeploymentUtil(*this, m_deploymentCfg);
        break;
    }

    if (!m_deploymentUtil)
        return;

    if (m_deploymentCfg.m_buildGame)
    {
        m_deploymentUtil->BuildAndDeploy();
    }
    else
    {
        m_deploymentUtil->DeployFromFile(m_deploymentCfg.m_buildPath);
    }
}

bool DeploymentToolWindow::ValidateIPAddress(const QString& ipAddressString)
{
    QHostAddress ipAddress(ipAddressString);
    if (ipAddress.protocol() != QAbstractSocket::IPv4Protocol)
    {
        auto errorMessage = AZStd::string::format("IP address %s not valid IPv4 address", ipAddressString.toStdString().c_str());
        LogEndLine(errorMessage);
        return false;
    }

    return true;
}

bool DeploymentToolWindow::PopulateDeploymentConfigFromUi()
{
    m_deploymentCfg.m_buildConfiguration = m_ui->buildConfigComboBox->currentText().toLower().toStdString().c_str();
    m_deploymentCfg.m_useVFS = m_ui->fileTransferComboBox->currentText().contains("Virtual");
    m_deploymentCfg.m_buildGame = m_ui->buildGameCheckBox->isChecked();
    m_deploymentCfg.m_cleanDeviceBeforeInstall = m_ui->clearDeviceCheckBox->isChecked();
    m_deploymentCfg.m_installExecutable = m_ui->installExecCheckBox->isChecked();

    if (m_ui->platformComboBox->currentText().compare("android", Qt::CaseInsensitive) == 0)
    {
        m_deploymentCfg.m_platformOptions = PlatformOptions::Android;
    }

    m_deploymentCfg.m_assetProcessorRemoteIpAddress = m_ui->vfsRemoteIpTextField->text().toStdString().c_str();
    m_deploymentCfg.m_assetProcessorRemoteIpPort = m_ui->vfsRemotePortTextField->text().toStdString().c_str();

    m_deploymentCfg.m_shaderCompilerAP = m_ui->shaderCompilerUseAssetProcessorCheckBox->isChecked();
    m_deploymentCfg.m_shaderCompilerIP = m_ui->shaderIpAddressTextField->text().toStdString().c_str();
    m_deploymentCfg.m_shaderCompilerPort = m_ui->shaderPortTextField->text().toStdString().c_str();

    if (!ValidateIPAddress(m_ui->shaderIpAddressTextField->text()) || !ValidateIPAddress(m_ui->vfsRemoteIpTextField->text()))
    {
        return false;
    }

    return true;
}

void DeploymentToolWindow::WaitForAssetProcessorToFinish()
{
    int previousCount = 0;
    while (m_assetSystemListener->GetJobsCount() != 0)
    {
        if (previousCount != m_assetSystemListener->GetJobsCount())
        {
            AZStd::string waitMessage = AZStd::move(AZStd::string::format("Waiting for AssetProcessor to finish processing %d jobs...", m_assetSystemListener->GetJobsCount()));
            LogEndLine(waitMessage.c_str());
            previousCount = m_assetSystemListener->GetJobsCount();
        }
        Sleep(1);
    }

    // AssetProcessor writes out an assetcatalog.xml file in the game project cache folder
    // but does not count it as an asset that is being processed and so when we go to run
    // and adb push command on android it will fail copying the game project folder because
    // AssetProcessor has a lock on the file, which fails the deploy. Sleep for a bit (found
    // emperically that 500 ms is enough time) to give AssetProcessor the time it needs to
    // write out the file before doing the actual deploy.
    LogEndLine("Waiting for assetcatalog.xml to be updated.");
    Sleep(waitTimeForAssetCatalogUpdateInMilliseconds);
}

void DeploymentToolWindow::Success(const AZStd::string& msg)
{
    WriteToLog(msg + "\n");

    ResetDeployWidget();

    delete m_deploymentUtil;
    m_deploymentUtil = nullptr;
}

void DeploymentToolWindow::Error(const AZStd::string& msg)
{
    WriteToLog(msg + "\n");
    WriteToLog("Deploy cancelled.\n");

    ResetDeployWidget();

    delete m_deploymentUtil;
    m_deploymentUtil = nullptr;
}

void DeploymentToolWindow::Log(const AZStd::string& msg)
{
    WriteToLog(msg);
}

void DeploymentToolWindow::LogEndLine(const AZStd::string& msg)
{
    WriteToLog(msg + "\n");
}

void DeploymentToolWindow::WriteToLog(const AZStd::string& msg)
{
    m_ui->outputTextEdit->appendPlainText(QString(msg.c_str()));
    if (!m_ui->outputTextEdit->textCursor().hasSelection())
    {
        QScrollBar *sb = m_ui->outputTextEdit->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}

void DeploymentToolWindow::ResetDeployWidget()
{
    m_animatedSpinningIcon.stop();
    m_ui->deploySpinnerLabel->setHidden(true);

    m_ui->deployButton->setText("Deploy");
    m_ui->deployButton->setEnabled(true);
}

void DeploymentToolWindow::SetupUi()
{
    m_ui->setupUi(this);

    m_ui->advancedOptionsWidget->setHidden(m_hideAdvancedOptions);
    m_ui->advanceOptionsButton->setIcon(m_closedIcon);

    m_ui->deployButton->setProperty("class", "Primary");

    m_ui->deploySpinnerLabel->setMovie(&m_animatedSpinningIcon);
    m_ui->deploySpinnerLabel->setHidden(true);

    m_animatedSpinningIcon.setScaledSize(QSize(16, 16));

    connect(m_ui->deployButton, &QPushButton::clicked, this, &DeploymentToolWindow::Deploy);
    connect(m_ui->advanceOptionsButton, &QPushButton::clicked, this, &DeploymentToolWindow::ToggleAdvanceOptions);
}

void DeploymentToolWindow::InitializeUi()
{
    const char* systemFileName = nullptr;
    if (m_ui->platformComboBox->currentText().compare("android", Qt::CaseInsensitive) == 0)
    {
        systemFileName = AndroidDeploymentUtil::GetSystemConfigFileName();
    }

    SystemConfigContainer systemConfigFile(systemFileName);
    systemConfigFile.ReadContents();

    AZStd::string remoteShaderCompilerIpAddress = systemConfigFile.GetShaderCompilerIPIncludeComments();
    if (!remoteShaderCompilerIpAddress.empty())
    {
        m_ui->shaderIpAddressTextField->setText(remoteShaderCompilerIpAddress.c_str());
    }

    AZStd::string remoteShaderCompilerPort = systemConfigFile.GetShaderCompilerPortIncludeComments();
    if (!remoteShaderCompilerPort.empty())
    {
        m_ui->shaderPortTextField->setText(remoteShaderCompilerPort.c_str());
    }

    const bool remoteShaderCompilerUseAssetProcessor = systemConfigFile.GetUseAssetProcessorShaderCompilerIncludeComments();
    m_ui->shaderCompilerUseAssetProcessorCheckBox->setChecked(remoteShaderCompilerUseAssetProcessor);

    AZStd::string remoteIpAddress = m_bootstrapConfig.GetRemoteIPIncludingComments();
    if (!remoteIpAddress.empty())
    {
        m_ui->vfsRemoteIpTextField->setText(remoteIpAddress.c_str());
    }

    AZStd::string remoteIpPort = m_bootstrapConfig.GetRemotePortIncludingComments();
    if (!remoteIpPort.empty())
    {
        m_ui->vfsRemotePortTextField->setText(remoteIpPort.c_str());
    }
}

void DeploymentToolWindow::SaveUiState()
{ 
    m_deploySettings.setValue(hardwarePlatformKeyString, m_ui->platformComboBox->currentText());
    m_deploySettings.setValue(buildConfigKeyString, m_ui->buildConfigComboBox->currentText());
    m_deploySettings.setValue(fileTransferKeyString, m_ui->fileTransferComboBox->currentText());

    m_deploySettings.setValue(buildGameKeyString, m_ui->buildGameCheckBox->isChecked());
    m_deploySettings.setValue(clearDeviceKeyString, m_ui->clearDeviceCheckBox->isChecked());
    m_deploySettings.setValue(installExeKeyString, m_ui->installExecCheckBox->isChecked());
}

void DeploymentToolWindow::RestoreUiState()
{
    m_ui->platformComboBox->setCurrentText(m_deploySettings.value(hardwarePlatformKeyString).toString());
    m_ui->buildConfigComboBox->setCurrentText(m_deploySettings.value(buildConfigKeyString).toString());
    m_ui->fileTransferComboBox->setCurrentText(m_deploySettings.value(fileTransferKeyString).toString());

    m_ui->buildGameCheckBox->setChecked(m_deploySettings.value(buildGameKeyString).toBool());
    m_ui->clearDeviceCheckBox->setChecked(m_deploySettings.value(clearDeviceKeyString).toBool());
    m_ui->installExecCheckBox->setChecked(m_deploySettings.value(installExeKeyString).toBool());
}

#include <DeploymentToolWindow.moc>
