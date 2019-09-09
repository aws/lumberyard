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
#include "DeploymentToolWindow.h"
#include "ui_DeploymentToolWindow.h"

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qabstractsocket.h>
#include <QTimer.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <ILevelSystem.h>
#include <ISystem.h>
#include <RemoteConsoleCore.h>

#include <EditorDefs.h> // required to be included before CryEdit
#include <CryEdit.h>

#include "BootstrapConfigContainer.h"
#include "DeployNotificationsBus.h"
#include "NetworkUtils.h"

#include "DeployWorker_android.h"
#if defined(AZ_PLATFORM_APPLE_OSX)
    #include "DeployWorker_ios.h"
#endif

namespace
{
    struct Range
    {
        Range()
            : m_min(std::numeric_limits<AZ::u16>::min())
            , m_max(std::numeric_limits<AZ::u16>::max())
        {
        }

        Range(AZ::u16 min, AZ::u16 max)
            : m_min(min)
            , m_max(max)
        {
        }

        AZ::u16 GetRandValueInRange() const
        {
            AZ::u16 value = static_cast<AZ::u16>(rand());
            return m_min + (value % (m_max - m_min));
        }

        AZ::u16 m_min;
        AZ::u16 m_max;
    };


    const int assetProcessorPollFrequencyInMilliseconds = 500;
    const int devicePollFrequencyInMilliseconds = 10 * 1000; // check for devices every 10 seconds

    const Range hostRemoteLogPortRange = { 4700, 4750 };

    const char* noDevicesConnectedEntry = "No Devices Connected";

    const char* targetPlatform = "targetPlatform";
    const char* buildConfigKey = "buildConfiguration";
    const char* assetMode = "assetMode";
    const char* buildGameKey = "buildGame";
    const char* cleanDeviceKey = "cleanDevice";

    const char* settingsFileName = "DeploymentToolConfig.ini";


    bool IsAnyLevelLoaded()
    {
        if (gEnv && gEnv->pSystem)
        {
            if (ILevelSystem* levelSystem = gEnv->pSystem->GetILevelSystem())
            {
                return (levelSystem->GetCurrentLevel() != nullptr);
            }
        }
        return false;
    }

    bool ValidateIPAddress(const AZStd::string& ipAddressString)
    {
        QHostAddress ipAddress(ipAddressString.c_str());
        if (ipAddress.protocol() != QAbstractSocket::IPv4Protocol)
        {
            DEPLOY_LOG_ERROR("[ERROR] %s is not a valid IPv4 address", ipAddressString.c_str());
            return false;
        }

        return true;
    }
}


DeploymentToolWindow::DeploymentToolWindow()
    : m_ui(new Ui::DeploymentToolWindow())
    , m_animatedSpinningIcon("://spinner_2x_V1.gif")

    , m_connectedIcon("://status_connected.png")
    , m_notConnectedIcon("://status_not_connected.png")

    , m_defaultDeployOutputBrush()
    , m_defaultRemoteDeviceOutputBrush()

    , m_devicePollTimer(new QTimer(this))
    , m_deployWorker(nullptr)

    , m_deploySettings(settingsFileName, QSettings::IniFormat)

    , m_remoteLog()
    , m_notificationsBridge()

    , m_deploymentCfg()
    , m_bootstrapConfig()

    , m_lastDeployedDevice()
{
    StringOutcome outcome = m_bootstrapConfig.Load();
    if (!outcome.IsSuccess())
    {
        AZStd::string error = AZStd::move(AZStd::string::format("[ERROR] %s", outcome.GetError().c_str()));
        LogToWindow(DeployTool::LogStream::DeployOutput, DeployTool::LogSeverity::Error, error.c_str());
    }

    connect(m_devicePollTimer.data(), &QTimer::timeout, this, &DeploymentToolWindow::OnDevicePollTimer);
    m_devicePollTimer->start(0);

    // force all connections to be queued to ensure any event that modifies UI will be processed on the correct thread.
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnLog, this, &DeploymentToolWindow::LogToWindow, Qt::QueuedConnection);
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnDeployProcessStatusChange, this, &DeploymentToolWindow::OnDeployStatusChange, Qt::QueuedConnection);
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnDeployProcessFinished, this, &DeploymentToolWindow::OnDeployFinished, Qt::QueuedConnection);
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnRemoteLogConnectionStateChange, this, &DeploymentToolWindow::OnRemoteLogConnectionStateChange, Qt::QueuedConnection);

    InitializeUi();
}

DeploymentToolWindow::~DeploymentToolWindow()
{
    SaveUiState();
}

void DeploymentToolWindow::OnDeployStatusChange(const QString& status)
{
    m_ui->deployStatusLabel->setText(status);
}

void DeploymentToolWindow::OnDeployFinished(bool success)
{
    if (success)
    {
        DEPLOY_LOG_INFO("Deploy Succeeded!");

        m_lastDeployedDevice = m_ui->targetDeviceComboBox->currentText();

        bool loadCurrentLevel = IsAnyLevelLoaded() && m_ui->loadCurrentLevelCheckBox->isChecked();

        // make the remote log tab the active one
        m_ui->outputTabWidget->setCurrentIndex(1);
        m_remoteLog.Start(m_deploymentCfg.m_deviceIpAddress, m_deploymentCfg.m_hostRemoteLogPort, loadCurrentLevel);
    }
    else
    {
        DEPLOY_LOG_ERROR("**** Deploy Failed! *****");
    }

    Reset();
}

void DeploymentToolWindow::OnRemoteLogConnectionStateChange(bool isConnected)
{
    const QIcon& statusIcon = (isConnected ? m_connectedIcon : m_notConnectedIcon);
    m_ui->remoteStatusIconLabel->setPixmap(statusIcon.pixmap(QSize(16, 16)));

    m_ui->remoteStatusLabel->setText(
        isConnected ?
        QString("Connected to %1").arg(m_lastDeployedDevice) :
        "Not Connected");
}

void DeploymentToolWindow::OnPlatformChanged(const QString& currentplatform)
{
    // reset the selected platform
    m_deploymentCfg.m_platformOption = PlatformOptions::Invalid;
    m_deployWorker.reset(nullptr);

    bool hidePlatformOptions = false;

    if (currentplatform.startsWith("android", Qt::CaseInsensitive))
    {
        m_deploymentCfg.m_platformOption = (currentplatform.endsWith("armv8", Qt::CaseInsensitive) ?
                                            PlatformOptions::Android_ARMv8 :
                                            PlatformOptions::Android_ARMv7);

        hidePlatformOptions = false;
        m_deployWorker.reset(new DeployWorkerAndroid());
    }
#if defined(AZ_PLATFORM_APPLE_OSX)
    else if (currentplatform.compare("ios", Qt::CaseInsensitive) == 0)
    {
        m_deploymentCfg.m_platformOption = PlatformOptions::iOS;

        hidePlatformOptions = true;
        m_deployWorker.reset(new DeployWorkerIos());
    }
#endif
    AZ_Assert(m_deploymentCfg.m_platformOption != PlatformOptions::Invalid, "Unable to determine the target platform selected.");

    if (m_deployWorker)
    {
        UpdateUiFromSystemConfig(m_deployWorker->GetSystemConfigFileName());

        // make sure the assets are enabled for this platform and disable the deploy button if not
        const AZStd::string platformAssets = m_bootstrapConfig.GetAssetsTypeForPlatform(m_deploymentCfg.m_platformOption);

        bool isPlatformAssetsEnabled = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(isPlatformAssetsEnabled, 
                &AzToolsFramework::AssetSystemRequestBus::Events::IsAssetPlatformEnabled, platformAssets.c_str());

        if (!isPlatformAssetsEnabled)
        {
            DEPLOY_LOG_ERROR(
                "[ERROR] Asset processing for platform \"%s\" is not enabled!  Update AssetProcessorPlatfromConfig.ini to enable asset type \"%s\" then restart the Editor and Asset Processor.",
                currentplatform.toStdString().c_str(), platformAssets.c_str());
        }

        m_ui->deployButton->setEnabled(isPlatformAssetsEnabled);
    }

    m_ui->platformOptionsGroup->setHidden(hidePlatformOptions);

    // re-evaluate what options should be shown
    OnAssetModeChanged(m_ui->assetModeComboBox->currentText());

    // update the device list
    m_devicePollTimer->stop();
    m_devicePollTimer->start(0);
}

void DeploymentToolWindow::OnAssetModeChanged(const QString& currentMode)
{
    m_deploymentCfg.m_useVFS = currentMode.contains("VFS");
    UpdateIpAddressOptions();
}

void DeploymentToolWindow::OnShaderCompilerUseAPToggle(int state)
{
    m_deploymentCfg.m_shaderCompilerUseAP = (state == Qt::Checked);
    UpdateIpAddressOptions();
}

void DeploymentToolWindow::OnDevicePollTimer()
{
    if (!m_deployWorker)
    {
        return;
    }

    DeviceMap connectedDevices;
    m_deployWorker->GetConnectedDevices(connectedDevices);

    // get the current selection and clear the contents
    QString currentDevice = m_ui->targetDeviceComboBox->currentData().toString();
    m_ui->targetDeviceComboBox->clear();

    // re-populate the device list
    for (DeviceMap::ConstIterator itr = connectedDevices.begin();
        itr != connectedDevices.end();
        ++itr)
    {
        m_ui->targetDeviceComboBox->addItem(itr.value(), itr.key());
    }

    if (m_ui->targetDeviceComboBox->count() == 0)
    {
        m_ui->targetDeviceComboBox->addItem(noDevicesConnectedEntry);
    }

    // attempt to reselect the previous selected device
    int index = m_ui->targetDeviceComboBox->findData(currentDevice);
    if (index == -1)
    {
        index = 0;
    }

    m_ui->targetDeviceComboBox->setCurrentIndex(index);

    // restart the timer
    m_devicePollTimer->start(devicePollFrequencyInMilliseconds);
}

void DeploymentToolWindow::OnDeployClick()
{
    if (!m_deployWorker)
    {
        InternalDeployFailure("Invalid platform configuration");
        return;
    }

    m_remoteLog.Stop();

    m_devicePollTimer->stop();

    m_ui->deployButton->setText("Deploying...");

    m_ui->deployOutputTextBox->setPlainText("");
    m_ui->remoteOutputTextBox->setPlainText("");

    m_animatedSpinningIcon.start();
    m_ui->deploySpinnerLabel->setHidden(false);

    ToggleInteractiveUI(false);

    // reset the current tab to the deploy output
    m_ui->outputTabWidget->setCurrentIndex(0);

    if (!PopulateDeploymentConfigFromUi())
    {
        InternalDeployFailure("Invalid deployment configuration!");
        return;
    }

    // configure bootstrap.cfg
    StringOutcome outcome = m_bootstrapConfig.Reload();
    if (!outcome.IsSuccess())
    {
        InternalDeployFailure(outcome.GetError());
        return;
    }

    // get project name
    m_deploymentCfg.m_projectName = m_bootstrapConfig.GetGameFolder();

    // update the bootstrap
    outcome = m_bootstrapConfig.ApplyConfiguration(m_deploymentCfg);
    if (!outcome.IsSuccess())
    {
        InternalDeployFailure(outcome.GetError());
        return;
    }

    if (    (m_deploymentCfg.m_platformOption == PlatformOptions::Android_ARMv7 || m_deploymentCfg.m_platformOption == PlatformOptions::Android_ARMv8)
        &&  DeployTool::IsLocalhost(m_deploymentCfg.m_deviceIpAddress))
    {
        m_deploymentCfg.m_hostRemoteLogPort = hostRemoteLogPortRange.GetRandValueInRange();
    }

    outcome = m_deployWorker->ApplyConfiguration(m_deploymentCfg);
    if (!outcome.IsSuccess())
    {
        InternalDeployFailure(outcome.GetError());
        return;
    }

    // first make sure a level is loaded, otherwise this will fail and cancel the deploy operation
    if (IsAnyLevelLoaded())
    {
        // make sure the level is updated for use in the runtime
        if (!CCryEditApp::Command_ExportToEngine())
        {
            InternalDeployFailure("Failed to export the level for runtime");
            return;
        }
    }

    DEPLOY_LOG_INFO("[INFO] Processing assets...");
    Run();
}

void DeploymentToolWindow::ToggleInteractiveUI(bool enabled)
{
    // base options
    m_ui->platformComboBox->setEnabled(enabled);
    m_ui->buildConfigComboBox->setEnabled(enabled);
    m_ui->targetDeviceComboBox->setEnabled(enabled);
    m_ui->deviceIpAddressTextField->setEnabled(enabled);
    m_ui->buildGameCheckBox->setEnabled(enabled);
    m_ui->loadCurrentLevelCheckBox->setEnabled(enabled);

    // asset options
    m_ui->assetModeComboBox->setEnabled(enabled);
    m_ui->assetProcessorIpAddressComboBox->setEnabled(enabled);
    m_ui->shaderCompilerUseAPCheckBox->setEnabled(enabled);
    m_ui->shaderCompilerIpAddressTextField->setEnabled(enabled);

    // platform options
    m_ui->cleanDeviceCheckBox->setEnabled(enabled);

    // other
    m_ui->deployButton->setEnabled(enabled);

    // re-evaluate the IP options if we are enabling edits again
    if (enabled)
    {
        UpdateIpAddressOptions();
    }
}

void DeploymentToolWindow::UpdateIpAddressOptions()
{
    m_ui->assetProcessorIpAddressComboBox->setEnabled(m_deploymentCfg.m_useVFS || m_deploymentCfg.m_shaderCompilerUseAP);
}

void DeploymentToolWindow::UpdateUiFromSystemConfig(const char* systemConfigFile)
{
    SystemConfigContainer systemConfig(systemConfigFile);
    StringOutcome outcome = systemConfig.Load();
    if (outcome.IsSuccess())
    {
        AZStd::string shaderCompilerIpAddress = systemConfig.GetShaderCompilerIP();
        if (!shaderCompilerIpAddress.empty())
        {
            m_ui->shaderCompilerIpAddressTextField->setText(shaderCompilerIpAddress.c_str());
        }

        const bool shaderCompilerUseAssetProcessor = systemConfig.GetUseAssetProcessorShaderCompiler();
        m_ui->shaderCompilerUseAPCheckBox->setChecked(shaderCompilerUseAssetProcessor);
    }
    else
    {
        DEPLOY_LOG_ERROR("[ERROR] %s", outcome.GetError().c_str());
    }
}

bool DeploymentToolWindow::PopulateDeploymentConfigFromUi()
{
    // base options
    m_deploymentCfg.m_buildConfiguration = m_ui->buildConfigComboBox->currentText().toLower().toUtf8().data();
    m_deploymentCfg.m_deviceId = m_ui->targetDeviceComboBox->currentData().toString().toUtf8().data();
    m_deploymentCfg.m_deviceIpAddress = m_ui->deviceIpAddressTextField->text().toUtf8().data();
    m_deploymentCfg.m_buildGame = m_ui->buildGameCheckBox->isChecked();

    // platform options
    m_deploymentCfg.m_cleanDevice = m_ui->cleanDeviceCheckBox->isChecked();

    // asset options
    m_deploymentCfg.m_assetProcessorIpAddress = m_ui->assetProcessorIpAddressComboBox->currentData().toString().toUtf8().data();
    m_deploymentCfg.m_shaderCompilerIpAddress = m_ui->shaderCompilerIpAddressTextField->text().toUtf8().data();

    if (m_deploymentCfg.m_deviceId.empty())
    {
        DEPLOY_LOG_ERROR("[ERROR] Invalid device selection");
        return false;
    }

    if (    !ValidateIPAddress(m_deploymentCfg.m_deviceIpAddress)
        ||  !ValidateIPAddress(m_deploymentCfg.m_assetProcessorIpAddress)
        ||  !ValidateIPAddress(m_deploymentCfg.m_shaderCompilerIpAddress))
    {
        return false;
    }

    return true;
}

void DeploymentToolWindow::Run()
{
    using namespace AzToolsFramework;

    const AZStd::string hostAssets = m_bootstrapConfig.GetHostAssetsType();
    const AZStd::string platformAssets = m_bootstrapConfig.GetAssetsTypeForPlatform(m_deploymentCfg.m_platformOption);

    int hostAssetsLeft = -1;
    AssetSystemRequestBus::BroadcastResult(hostAssetsLeft, 
            &AssetSystemRequestBus::Events::GetPendingAssetsForPlatform, hostAssets.c_str());

    int platformAssetsLeft = -1;
    AssetSystemRequestBus::BroadcastResult(platformAssetsLeft, 
            &AssetSystemRequestBus::Events::GetPendingAssetsForPlatform, platformAssets.c_str());

    if (platformAssetsLeft == -1)
    {
        InternalDeployFailure("Failed to query the status of the Asset Processor!  Check the status tray to verify the Asset Processor is running.");
    }
    else if (platformAssetsLeft > 0)
    {
        hostAssetsLeft = max(hostAssetsLeft, 0);
        AZStd::string status(AZStd::string::format("Remaining assets to process - %d host / %d platform", hostAssetsLeft, platformAssetsLeft));

        DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessStatusChange, status);

        QTimer::singleShot(assetProcessorPollFrequencyInMilliseconds, this, &DeploymentToolWindow::Run);
    }
    else
    {
        m_deployWorker->Run();
    }
}

void DeploymentToolWindow::InternalDeployFailure(const AZStd::string& reason)
{
    DEPLOY_LOG_ERROR("[ERROR] %s", reason.c_str());
    DeployTool::Notifications::Bus::Broadcast(&DeployTool::Notifications::DeployProcessFinished, false);
}

void DeploymentToolWindow::LogToWindow(DeployTool::LogStream stream, DeployTool::LogSeverity logSeverity, const QString& message)
{
    QTextEdit* outputWindow = nullptr;
    const QBrush* defaultBrush = nullptr;

    switch (stream)
    {
        case DeployTool::LogStream::DeployOutput:
            outputWindow = m_ui->deployOutputTextBox;
            defaultBrush = &m_defaultDeployOutputBrush;
            break;

        case DeployTool::LogStream::RemoteDeviceOutput:
            outputWindow = m_ui->remoteOutputTextBox;
            defaultBrush = &m_defaultRemoteDeviceOutputBrush;
            break;

        default:
            AZ_Assert(false, "Invalid stream type received on DeployLogNotificationBus");
            return;
    }

    // have to use the char format to change the text color because color() apparently doesn't
    // take stylesheets into account
    QTextCharFormat textFormat = outputWindow->currentCharFormat();

    switch (logSeverity)
    {
        case DeployTool::LogSeverity::Error:
            textFormat.setForeground(QBrush(QColor(Qt::red)));
            break;

        case DeployTool::LogSeverity::Warn:
            textFormat.setForeground(QBrush(QColor(Qt::yellow)));
            break;

        case DeployTool::LogSeverity::Debug:
            textFormat.setForeground(QBrush(QColor(Qt::green)));
            break;

        case DeployTool::LogSeverity::Info:
        default:
            textFormat.setForeground(*defaultBrush);
            break;
    }

    outputWindow->setCurrentCharFormat(textFormat);

    outputWindow->append(message);

    textFormat.setForeground(*defaultBrush);
    outputWindow->setCurrentCharFormat(textFormat);

    if (!outputWindow->textCursor().hasSelection())
    {
        QScrollBar* scrollbar = outputWindow->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }
}

void DeploymentToolWindow::Reset()
{
    m_animatedSpinningIcon.stop();
    m_ui->deploySpinnerLabel->setHidden(true);
    m_ui->deployButton->setText("Deploy");

    m_ui->deployStatusLabel->setText("Idle");
    ToggleInteractiveUI(true);

    m_devicePollTimer->start(0);

    m_deploymentCfg.m_hostRemoteLogPort = defaultRemoteConsolePort;
}

void DeploymentToolWindow::InitializeUi()
{
    // setup
    m_ui->setupUi(this);

    m_ui->deployButton->setProperty("class", "Primary");

    m_ui->remoteStatusIconLabel->setPixmap(m_notConnectedIcon.pixmap(QSize(16, 16)));

    m_ui->deploySpinnerLabel->setMovie(&m_animatedSpinningIcon);
    m_ui->deploySpinnerLabel->setHidden(true);

    m_animatedSpinningIcon.setScaledSize(QSize(16, 16));

    m_ui->outputTabWidget->setCurrentIndex(0);

    connect(m_ui->platformComboBox, &QComboBox::currentTextChanged, this, &DeploymentToolWindow::OnPlatformChanged);
    connect(m_ui->assetModeComboBox, &QComboBox::currentTextChanged, this, &DeploymentToolWindow::OnAssetModeChanged);
    connect(m_ui->shaderCompilerUseAPCheckBox, &QCheckBox::stateChanged, this, &DeploymentToolWindow::OnShaderCompilerUseAPToggle);
    connect(m_ui->deployButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnDeployClick);

    m_defaultDeployOutputBrush = m_ui->deployOutputTextBox->currentCharFormat().foreground();
    m_defaultRemoteDeviceOutputBrush = m_ui->remoteOutputTextBox->currentCharFormat().foreground();

    // base options
#if defined(AZ_PLATFORM_APPLE_OSX)
    // matching by default is case insensitive unless Qt::MatchCaseSensitive specified
    int index = m_ui->platformComboBox->findText("ios", Qt::MatchExactly);
    if (index == -1)
    {
        m_ui->platformComboBox->addItem("iOS");
    }
#endif

    QString platform = m_deploySettings.value(targetPlatform).toString();
    if (platform.startsWith("android", Qt::CaseInsensitive))
    {
        if (!platform.endsWith("armv8", Qt::CaseInsensitive))
        {
            platform = "Android ARMv7";
        }
    }

    m_ui->platformComboBox->setCurrentText(platform);

    m_ui->buildConfigComboBox->setCurrentText(m_deploySettings.value(buildConfigKey).toString());
    m_ui->buildGameCheckBox->setChecked(m_deploySettings.value(buildGameKey).toBool());

    // asset options
    m_ui->assetModeComboBox->setCurrentText(m_deploySettings.value(assetMode).toString());

    // populate the host machine IP addresses for the asset processor
    m_ui->assetProcessorIpAddressComboBox->clear();

    const QString ipDisplayFormat("%1 (%2)");
    const QString localhostDisplay("localhost");
    const QString localhostIP("127.0.0.1");

    AZStd::vector<AZStd::string> localIpAddrs;
    if (DeployTool::GetAllHostIPAddrs(localIpAddrs))
    {
        for (const AZStd::string& ipAddress : localIpAddrs)
        {
            QString value(ipAddress.c_str());
            QString displayName = ipDisplayFormat.arg(value, DeployTool::IsLocalhost(ipAddress) ? localhostDisplay : "Network Adapter");

            m_ui->assetProcessorIpAddressComboBox->addItem(displayName, value);
        }
    }

    if (m_ui->assetProcessorIpAddressComboBox->count() == 0)
    {
        DEPLOY_LOG_WARN("[WARN] Failed to get the host computer's local IP addresses.  Access to the Asset Processor may be limited or not work at all.");
        m_ui->assetProcessorIpAddressComboBox->addItem(ipDisplayFormat.arg(localhostIP, localhostDisplay), localhostIP);
    }

    AZStd::string assetProcessorIpAddress = m_bootstrapConfig.GetRemoteIP();
    if (!assetProcessorIpAddress.empty())
    {
        QString ipAddress(assetProcessorIpAddress.c_str());

        int index = m_ui->assetProcessorIpAddressComboBox->findData(ipAddress);
        if (index != -1)
        {
            m_ui->assetProcessorIpAddressComboBox->setCurrentIndex(index);
        }
        else
        {
            QString displayName = ipDisplayFormat.arg(ipAddress, "bootstrap.cfg");
            m_ui->assetProcessorIpAddressComboBox->addItem(displayName, ipAddress);

            m_ui->assetProcessorIpAddressComboBox->setCurrentIndex(m_ui->assetProcessorIpAddressComboBox->count() - 1);
        }
    }

    // platform options
    m_ui->cleanDeviceCheckBox->setChecked(m_deploySettings.value(cleanDeviceKey).toBool());

    // re-evaluate what options should be shown
    OnPlatformChanged(m_ui->platformComboBox->currentText());
}

void DeploymentToolWindow::SaveUiState()
{
    // base options
    m_deploySettings.setValue(targetPlatform, m_ui->platformComboBox->currentText());
    m_deploySettings.setValue(buildConfigKey, m_ui->buildConfigComboBox->currentText());
    m_deploySettings.setValue(buildGameKey, m_ui->buildGameCheckBox->isChecked());

    // asset options
    m_deploySettings.setValue(assetMode, m_ui->assetModeComboBox->currentText());

    // platform options
    m_deploySettings.setValue(cleanDeviceKey, m_ui->cleanDeviceCheckBox->isChecked());
}

#include <DeploymentToolWindow.moc>
