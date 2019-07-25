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

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

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
#include "DeployWorker_devicefarm.h"
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <CloudCanvas/ICloudCanvasEditor.h>
#include <aws/core/client/ClientConfiguration.h>
#include "RealDeviceFarmClient.h"

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
    // check for device farm run(s) status every 10 seconds
    // when Device Farm and a project is selected.
    const int deviceFarmRunsPollFrequencyInMilliseconds = 10 * 1000;

    const Range hostRemoteLogPortRange = { 4700, 4750 };

    const char* noDevicesConnectedEntry = "No Devices Connected";
    const char* noProjectsFound = "No Projects Found";
    const char* noDevicePoolsFound = "No Device Pools Found";

    // Test types
    const char* deviceFarmTestTypeBuiltInFuzz = "Built-in fuzz";

    const char* targetPlatformKey = "targetPlatform";
    const char* buildConfigKey = "buildConfiguration";
    const char* assetModeKey = "assetMode";
    const char* buildGameKey = "buildGame";
    const char* cleanDeviceKey = "cleanDevice";
    const char* shaderCompilerIpAddressKey = "shaderCompilerIpAddress";
    const char* shaderCompilerUseAPKey = "shaderCompilerUseAP";
    const char* deviceFarmExecutionTimeoutKey = "deviceFarmExecutionTimeout";    
    
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

    , m_deviceFarmRunErrorIcon("://error.png")
    , m_deviceFarmRunInProgressIcon("://in_progress.png")
    , m_deviceFarmRunSuccessIcon("://success.png")
    , m_deviceFarmRunWarningIcon("://warning.png")

    , m_defaultDeployOutputBrush()
    , m_defaultRemoteDeviceOutputBrush()

    , m_devicePollTimer(new QTimer(this))
    , m_deviceFarmRunsPollTimer(new QTimer(this))
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

    connect(m_deviceFarmRunsPollTimer.data(), &QTimer::timeout, this, &DeploymentToolWindow::OnDeviceFarmRunsPollTimer);
    m_deviceFarmRunsPollTimer->start(0);

    // force all connections to be queued to ensure any event that modifies UI will be processed on the correct thread.
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnLog, this, &DeploymentToolWindow::LogToWindow, Qt::QueuedConnection);
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnDeployProcessStatusChange, this, &DeploymentToolWindow::OnDeployStatusChanged, Qt::QueuedConnection);
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnDeployProcessFinished, this, &DeploymentToolWindow::OnDeployFinished, Qt::QueuedConnection);
    connect(&m_notificationsBridge, &DeployTool::NotificationsQBridge::OnRemoteLogConnectionStateChange, this, &DeploymentToolWindow::OnRemoteLogConnectionStateChanged, Qt::QueuedConnection);

    InitializeUi();
    connect(m_ui->deviceFarmProjectNameComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DeploymentToolWindow::OnDeviceFarmProjectComboBoxValueChanged);    
    connect(m_ui->addDeviceFarmProjectButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnAddDeviceFarmProjectButtonClick);
    connect(m_ui->removeDeviceFarmProjectButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnRemoveDeviceFarmProjectButtonClick);

    connect(m_ui->addDeviceFarmDevicePoolButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnAddDeviceFarmDevicePoolButtonClick);
    connect(m_ui->removeDeviceFarmDevicePoolButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnRemoveDeviceFarmDevicePoolButtonClick);


    QStringList headerLabels;
    headerLabels.append("Status");
    headerLabels.append("Name");
    headerLabels.append("Device pool");
    headerLabels.append("Test type");
    headerLabels.append("Progress");
    m_ui->deviceFarmRunsTable->setColumnCount(headerLabels.size());
    m_ui->deviceFarmRunsTable->setHorizontalHeaderLabels(headerLabels);

    QHeaderView* header = m_ui->deviceFarmRunsTable->horizontalHeader();
    header->setSectionResizeMode(deviceFarmRunsTableColumnStatus, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(deviceFarmRunsTableColumnName, QHeaderView::Stretch);
    header->setSectionResizeMode(deviceFarmRunsTableColumnDevicePool, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(deviceFarmRunsTableColumnTestType, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(deviceFarmRunsTableColumnProgress, QHeaderView::ResizeToContents);

    m_ui->deviceFarmRunsTable->verticalHeader()->setVisible(false);
    m_ui->deviceFarmRunsTable->setAlternatingRowColors(true);

    // Use a custom menu and only show it when a row is clicked.
    m_ui->deviceFarmRunsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui->deviceFarmRunsTable, &QWidget::customContextMenuRequested, this, [this](const QPoint& point)
    {    
        int clickedRow = m_ui->deviceFarmRunsTable->rowAt(point.y());
        if (clickedRow != -1)
        {
            // Right click context menu for the Device Farm runs table.
            QMenu contextMenu(tr("Context menu"), this);
            QAction* openInAwsConsoleAction = new QAction("View details in AWS Console", m_ui->deviceFarmRunsTable);
            connect(openInAwsConsoleAction, &QAction::triggered, this, [this, clickedRow]() {OnOpenInAwsConsoleAction(clickedRow); });
            QAction* deleteRunAction = new QAction("Delete Run", m_ui->deviceFarmRunsTable);
            connect(deleteRunAction, &QAction::triggered, this, [this, clickedRow]() {OnDeleteRunAction(clickedRow); });
            QAction* deleteAllRunsAction = new QAction("Delete All Runs", m_ui->deviceFarmRunsTable);
            connect(deleteAllRunsAction, &QAction::triggered, this, [this]() {OnDeleteAllRunsAction(); });
            contextMenu.addAction(openInAwsConsoleAction);
            contextMenu.addAction(deleteRunAction);
            contextMenu.addAction(deleteAllRunsAction);
            contextMenu.exec(m_ui->deviceFarmRunsTable->mapToGlobal(point));
        }
    });

    connect(m_ui->deployTargetsTabWidget, SIGNAL(currentChanged(int)), this, SLOT(OnDeployTargetsTabChanged()));
}

DeploymentToolWindow::~DeploymentToolWindow()
{
    // Wait for any pending list runs response before shutting down.
    while (m_waitingForListRunsResponse)
    {
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
    }

    SaveUiState();
}

void DeploymentToolWindow::OnDeployStatusChanged(const QString& status)
{
    m_ui->deployStatusLabel->setText(status);
}

void DeploymentToolWindow::OnDeployFinished(bool success)
{
    if (success)
    {
        DEPLOY_LOG_INFO("Deploy Succeeded!");

        // If this is a local device, start the remote log and show that tab.
        if (m_deploymentCfg.m_localDevice)
        {
            m_lastDeployedDevice = m_ui->targetDeviceComboBox->currentText();

            const bool loadCurrentLevel = IsAnyLevelLoaded() && m_ui->loadCurrentLevelCheckBox->isChecked();

            // make the remote log tab the active one
            m_ui->outputTabWidget->setCurrentIndex(1);
            m_remoteLog.Start(m_deploymentCfg.m_deviceIpAddress, m_deploymentCfg.m_hostRemoteLogPort, loadCurrentLevel);
        }
    }
    else
    {
        DEPLOY_LOG_ERROR("**** Deploy Failed! *****");
    }

    Reset();
}

void DeploymentToolWindow::OnRemoteLogConnectionStateChanged(bool isConnected)
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

        m_ui->localDeviceDeployButton->setEnabled(isPlatformAssetsEnabled);
        m_ui->deviceFarmDeployButton->setEnabled(isPlatformAssetsEnabled);
    }

    m_ui->cleanDeviceLabel->setHidden(hidePlatformOptions);
    m_ui->cleanDeviceCheckBox->setHidden(hidePlatformOptions);

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
    const QString currentDevice = m_ui->targetDeviceComboBox->currentData().toString();
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

void DeploymentToolWindow::OnDeviceFarmRunsPollTimer()
{
    // If the Device Farm is selected and a project is selected, refresh the list of runs.
    if (m_deviceFarmDriver.get() && !m_ui->deployTargetsDeviceFarmTab->isHidden())
    {
        const QString currentProjectArn = m_ui->deviceFarmProjectNameComboBox->currentData().toString();
        if (m_ui->deviceFarmProjectNameComboBox->count() > 0 && currentProjectArn != noProjectsFound)
        {
            ReloadDeviceFarmRunsList(currentProjectArn.toUtf8().data());
        }
    }
    m_deviceFarmRunsPollTimer->start(deviceFarmRunsPollFrequencyInMilliseconds);
}

void DeploymentToolWindow::OnLocalDeviceDeployClick()
{
    if (!m_deployWorker)
    {
        InternalDeployFailure("Invalid platform configuration");
        return;
    }

    if (m_deployWorker->IsRunning())
    {
        InternalDeployFailure("A deploy is already running.");
        return;
    }

    m_remoteLog.Stop();

    m_devicePollTimer->stop();

    m_ui->localDeviceDeployButton->setText("Deploying...");

    m_ui->deployOutputTextBox->setPlainText("");
    m_ui->remoteOutputTextBox->setPlainText("");

    m_animatedSpinningIcon.start();
    m_ui->localDeviceDeploySpinnerLabel->setHidden(false);

    ToggleInteractiveUI(false);

    // reset the current tab to the deploy output
    m_ui->outputTabWidget->setCurrentIndex(0);

    if (!PopulateDeploymentConfigFromUi(true))
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

    m_deploymentCfg.m_localDevice = true;

    // get project name
    m_deploymentCfg.m_projectName = m_bootstrapConfig.GetGameFolder();

    // update the bootstrap
    outcome = m_bootstrapConfig.ApplyConfiguration(m_deploymentCfg);
    if (!outcome.IsSuccess())
    {
        InternalDeployFailure(outcome.GetError());
        return;
    }

    if ((m_deploymentCfg.m_platformOption == PlatformOptions::Android_ARMv7 || m_deploymentCfg.m_platformOption == PlatformOptions::Android_ARMv8)
        && DeployTool::IsLocalhost(m_deploymentCfg.m_deviceIpAddress))
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

void DeploymentToolWindow::OnDeviceFarmDeployClick()
{
    if (!m_deployWorker)
    {
        InternalDeployFailure("Invalid platform configuration");
        return;
    }

    if (m_deployWorker->IsRunning())
    {
        InternalDeployFailure("A deploy is already running.");
        return;
    }

    m_ui->deviceFarmDeployButton->setText("Deploying...");

    m_ui->deployOutputTextBox->setPlainText("");

    m_animatedSpinningIcon.start();
    m_ui->deviceFarmDeploySpinnerLabel->setHidden(false);

    ToggleInteractiveUI(false);

    // reset the current tab to the deploy output
    m_ui->outputTabWidget->setCurrentIndex(0);

    if (!PopulateDeploymentConfigFromUi(false))
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

    m_deploymentCfg.m_localDevice = false;

    // get project name
    m_deploymentCfg.m_projectName = m_bootstrapConfig.GetGameFolder();

    // update the bootstrap
    outcome = m_bootstrapConfig.ApplyConfiguration(m_deploymentCfg);
    if (!outcome.IsSuccess())
    {
        InternalDeployFailure(outcome.GetError());
        return;
    }

    // Check to make sure there is a valid project selected.
    const QString currentProjectArn = m_ui->deviceFarmProjectNameComboBox->currentData().toString();
    if (m_ui->deviceFarmProjectNameComboBox->count() == 0 ||
        currentProjectArn == noProjectsFound)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Project Error"),
            QObject::tr("Please select a valid Project."));
        return;
    }

    // Check to make sure there is a valid device pool selected.
    const QString currentDevicePoolArn = m_ui->deviceFarmDevicePoolNameComboBox->currentData().toString();
    if (m_ui->deviceFarmDevicePoolNameComboBox->count() == 0 ||
        currentDevicePoolArn == noDevicePoolsFound)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Device Pool Error"),
            QObject::tr("Please select a valid Device Pool."));
        return;
    }

    // Make a new deploy worker right now based on the current settings.
    m_deployWorker.reset(
        new DeployTool::DeployWorkerDeviceFarm(
            m_deviceFarmDriver,
            m_ui->deviceFarmExecutionTimeoutSpinBox->value(),
            m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data(),
            m_ui->deviceFarmDevicePoolNameComboBox->currentData().toString().toUtf8().data()));

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

void DeploymentToolWindow::OnDeviceFarmProjectComboBoxValueChanged(int index)
{
    ReloadDeviceFarmDevicePoolsList(m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data());
    ReloadDeviceFarmRunsList(m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data());
}

void DeploymentToolWindow::OnAddDeviceFarmProjectButtonClick()
{
    // Pop open dialog to get a name of the new project
    const AZStd::string projectName = QInputDialog::getText(this, tr("Project Name:"), QString()).toUtf8().data();
    if (projectName.empty())
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Project Error"),
            QObject::tr("Invalid Project name."));
        return;
    }

    // Create a new project
    m_deviceFarmDriver->CreateProject(
        projectName,
        [&](bool success, const AZStd::string& msg, const DeployTool::DeviceFarmDriver::Project& project)
    {
        if (success)
        {
            // Add the new project to the list and select it.
            m_ui->deviceFarmProjectNameComboBox->addItem(project.name.c_str(), project.arn.c_str());
            int index = m_ui->deviceFarmProjectNameComboBox->findData(project.arn.c_str());
            m_ui->deviceFarmProjectNameComboBox->setCurrentIndex(index);

            // Remove noProjectsFound if it was found
            index = m_ui->deviceFarmProjectNameComboBox->findData(noProjectsFound);
            if (index != -1)
            {
                m_ui->deviceFarmProjectNameComboBox->removeItem(index);
            }
        }
        else
        {
            DEPLOY_LOG_ERROR("[ERROR] Failed to create Project: %s", msg.c_str());
        }
    });
}

void DeploymentToolWindow::OnRemoveDeviceFarmProjectButtonClick()
{
    const QString devicePoolName = m_ui->deviceFarmProjectNameComboBox->currentText();

    const int confirmResult = QMessageBox::question(this,
        tr("Delete Device Project?"),
        QStringLiteral("Are you sure you want to delete '%1'").arg(devicePoolName),
        QMessageBox::Yes, QMessageBox::No);

    if (confirmResult == QMessageBox::Yes)
    {
        AZStd::string projectArn = m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data();

        m_deviceFarmDriver->DeleteProject(
            projectArn,
            [&, projectArn](bool success, const AZStd::string& msg)
        {
            if (success)
            {
                int index = m_ui->deviceFarmProjectNameComboBox->findData(projectArn.c_str());
                if (index != -1)
                {
                    m_ui->deviceFarmProjectNameComboBox->removeItem(index);
                    if (m_ui->deviceFarmProjectNameComboBox->count() != 0)
                    {
                        m_ui->deviceFarmProjectNameComboBox->setCurrentIndex(0);
                    }
                    else
                    {
                        // If there is no items left, add the noProjectsFound item.
                        m_ui->deviceFarmProjectNameComboBox->addItem(noProjectsFound, noProjectsFound);
                    }
                }
            }
            else
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to delete Project: %s", msg.c_str());
            }
        });
    }
}

void DeploymentToolWindow::OnAddDeviceFarmDevicePoolButtonClick()
{
    // Device pools are created inside of a project so a valid project must be selected.
    const QString currentProjectArn = m_ui->deviceFarmProjectNameComboBox->currentData().toString();
    if (m_ui->deviceFarmProjectNameComboBox->count() == 0 || currentProjectArn == noProjectsFound)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Device Farm Project Error"),
            QObject::tr("Please select a valid project."));
        return;
    }

    const AZStd::string projectArn = currentProjectArn.toUtf8().data();

    // Filter device list by current platform
    const AZStd::string platform = m_deploymentCfg.m_platformOption == PlatformOptions::iOS ? "IOS" : "ANDROID";

    // Open the Device Farm Pool Window
    m_deviceFarmDevicePoolWindow.reset(new DeployTool::DeviceFarmDevicePoolWindow(this, m_deviceFarmDriver, projectArn, platform));
    if (m_deviceFarmDevicePoolWindow->exec() == QDialog::Accepted)
    {
        // New DevicePool was created, add it to the list and set it as the current index.
        DeployTool::DeviceFarmDriver::DevicePool devicePool = m_deviceFarmDevicePoolWindow->GetDevicePoolResult();
        m_ui->deviceFarmDevicePoolNameComboBox->addItem(devicePool.name.c_str(), devicePool.arn.c_str());
        int index = m_ui->deviceFarmDevicePoolNameComboBox->findData(devicePool.arn.c_str());
        m_ui->deviceFarmDevicePoolNameComboBox->setCurrentIndex(index);

        // Remove noDevicePoolsFound if it was found
        index = m_ui->deviceFarmDevicePoolNameComboBox->findData(noDevicePoolsFound);
        if (index != -1)
        {
            m_ui->deviceFarmDevicePoolNameComboBox->removeItem(index);
        }
    }
}

void DeploymentToolWindow::OnRemoveDeviceFarmDevicePoolButtonClick()
{
    QString devicePoolName = m_ui->deviceFarmDevicePoolNameComboBox->currentText();

    const int confirmResult = QMessageBox::question(this,
        tr("Delete Device Pool?"),
        QStringLiteral("Are you sure you want to delete '%1'").arg(devicePoolName),
        QMessageBox::Yes, QMessageBox::No);

    if (confirmResult == QMessageBox::Yes)
    {
        AZStd::string devicePoolArn = m_ui->deviceFarmDevicePoolNameComboBox->currentData().toString().toUtf8().data();

        m_deviceFarmDriver->DeleteDevicePool(
            devicePoolArn,
            [&, devicePoolArn](bool success, const AZStd::string& msg)
        {
            if (success)
            {
                int index = m_ui->deviceFarmDevicePoolNameComboBox->findData(devicePoolArn.c_str());
                if (index != -1)
                {
                    m_ui->deviceFarmDevicePoolNameComboBox->removeItem(index);
                    if (m_ui->deviceFarmDevicePoolNameComboBox->count() != 0)
                    {
                        m_ui->deviceFarmDevicePoolNameComboBox->setCurrentIndex(0);
                    }
                    else
                    {
                        // If there is no items left, add the noDevicePoolsFound item.
                        m_ui->deviceFarmDevicePoolNameComboBox->addItem(noProjectsFound, noDevicePoolsFound);
                    }
                }
            }
            else
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to delete Device Pool: %s", msg.c_str());
            }
        });
    }
}

void DeploymentToolWindow::OnOpenInAwsConsoleAction(int row)
{    
    AZStd::string runArn = m_ui->deviceFarmRunsTable->item(row, deviceFarmRunsTableColumnName)->data(Qt::UserRole).toString().toUtf8().data();
    AZStd::string url = DeployTool::DeviceFarmDriver::GetAwsConsoleJobUrl(
        m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data(),
        runArn);

    // Pop open the url
    QDesktopServices::openUrl(QUrl(url.c_str()));
}

void DeploymentToolWindow::OnDeleteRunAction(int row)
{
    const QString runName = m_ui->deviceFarmRunsTable->item(row, deviceFarmRunsTableColumnName)->text();

    const int confirmResult = QMessageBox::question(this,
        tr("Delete Run?"),
        QStringLiteral("Are you sure you want to delete '%1'").arg(runName),
        QMessageBox::Yes, QMessageBox::No);

    if (confirmResult == QMessageBox::Yes)
    {
        const AZStd::string runArn = m_ui->deviceFarmRunsTable->item(row, deviceFarmRunsTableColumnName)->data(Qt::UserRole).toString().toUtf8().data();

        m_deviceFarmDriver->DeleteRun(
            runArn,
            [&, runArn](bool success, const AZStd::string& msg)
        {
            if (success)
            {
                // Refresh the list
                ReloadDeviceFarmRunsList(m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data());
            }
            else
            {
                DEPLOY_LOG_ERROR("[ERROR] Failed to delete Project: %s", msg.c_str());
            }
        });
    }
}

void DeploymentToolWindow::OnDeleteAllRunsAction()
{
    const int confirmResult = QMessageBox::question(this,
        tr("Delete All Runs?"),
        QStringLiteral("Are you sure you want to delete all runs?"),
        QMessageBox::Yes, QMessageBox::No);

    if (confirmResult == QMessageBox::Yes)
    {
        AZStd::vector<AZStd::string> runArns;
        for (int row = 0; row < m_ui->deviceFarmRunsTable->rowCount(); row++)
        {
            runArns.push_back(m_ui->deviceFarmRunsTable->item(row, deviceFarmRunsTableColumnName)->data(Qt::UserRole).toString().toUtf8().data());
        }

        for (const AZStd::string runArn : runArns)
        {
            m_deviceFarmDriver->DeleteRun(
                runArn,
                [&, runArn](bool success, const AZStd::string& msg)
            {
                if (success)
                {
                    // Refresh the list
                    ReloadDeviceFarmRunsList(m_ui->deviceFarmProjectNameComboBox->currentData().toString().toUtf8().data());
                }
                else
                {
                    DEPLOY_LOG_ERROR("[ERROR] Failed to delete Project: %s", msg.c_str());
                }
            });
        }
    }
}

void DeploymentToolWindow::OnDeployTargetsTabChanged()
{
    // The Device Farm tab was selected
    if (DeviceFarmDeployTargetTabSelected())
    {
        // Make sure they've got their default profile setup or have selected a profile
        CloudCanvas::AWSClientCredentials credsProvider;
        EBUS_EVENT_RESULT(credsProvider, CloudCanvas::CloudCanvasEditorRequestBus, GetCredentials);

        if (!credsProvider.get() || credsProvider->GetAWSCredentials().GetAWSAccessKeyId().empty())
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Credentials Error"),
                QObject::tr("Your AWS credentials are not configured correctly. Please ensure the Cloud Gem Framework gem is enabled using the ProjectConfigurator and check your AWS credentials using the AWS Credentials manager."));

            m_ui->deployTargetsTabWidget->setCurrentIndex(0);
        }
        else
        {
            // Device Farm only works in the lovely state of Oregon
            Aws::Client::ClientConfiguration clientConfiguration = Aws::Client::ClientConfiguration();
            clientConfiguration.region = Aws::Region::US_WEST_2;

            // Create the Device Farm Interface and the Driver
            m_deviceFarmClient.reset(new DeployTool::RealDeviceFarmClient(credsProvider->GetAWSCredentials(), clientConfiguration));
            m_deviceFarmDriver.reset(new DeployTool::DeviceFarmDriver(m_deviceFarmClient));

            ReloadDeviceFarmProjectsList();

            m_loadCurrentLevelCheckBoxLastCheckedState = m_ui->loadCurrentLevelCheckBox->isChecked();
            m_ui->loadCurrentLevelCheckBox->setChecked(false);
            m_ui->loadCurrentLevelCheckBox->setEnabled(false);
        }
    }
    else
    {
        m_ui->loadCurrentLevelCheckBox->setChecked(m_loadCurrentLevelCheckBoxLastCheckedState);
        m_ui->loadCurrentLevelCheckBox->setEnabled(m_interactiveUIEnabled);
    }
}

void DeploymentToolWindow::ToggleInteractiveUI(bool enabled)
{
    // base options
    m_ui->platformComboBox->setEnabled(enabled);
    m_ui->buildConfigComboBox->setEnabled(enabled);
    m_ui->targetDeviceComboBox->setEnabled(enabled);
    m_ui->deviceIpAddressTextField->setEnabled(enabled);
    m_ui->buildGameCheckBox->setEnabled(enabled);
    m_ui->loadCurrentLevelCheckBox->setEnabled(enabled && !DeviceFarmDeployTargetTabSelected());

    // asset options
    m_ui->assetModeComboBox->setEnabled(enabled);
    m_ui->assetProcessorIpAddressComboBox->setEnabled(enabled);
    m_ui->shaderCompilerUseAPCheckBox->setEnabled(enabled);
    m_ui->shaderCompilerIpAddressTextField->setEnabled(enabled);

    // platform options
    m_ui->cleanDeviceCheckBox->setEnabled(enabled);

    // other
    m_ui->localDeviceDeployButton->setEnabled(enabled);
    m_ui->deviceFarmDeployButton->setEnabled(enabled);

    // re-evaluate the IP options if we are enabling edits again
    if (enabled)
    {
        UpdateIpAddressOptions();
    }

    m_interactiveUIEnabled = enabled;
}

void DeploymentToolWindow::UpdateIpAddressOptions()
{
    m_ui->assetProcessorIpAddressComboBox->setEnabled(m_deploymentCfg.m_useVFS || m_deploymentCfg.m_shaderCompilerUseAP);
}

bool DeploymentToolWindow::PopulateDeploymentConfigFromUi(bool localDevice)
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

    if (!ValidateIPAddress(m_deploymentCfg.m_assetProcessorIpAddress)
        || !ValidateIPAddress(m_deploymentCfg.m_shaderCompilerIpAddress))
    {
        return false;
    }

    if (localDevice)
    {
        if (m_deploymentCfg.m_deviceId.empty())
        {
            DEPLOY_LOG_ERROR("[ERROR] Invalid device selection");
            return false;
        }

        if (!ValidateIPAddress(m_deploymentCfg.m_deviceIpAddress))
        {
            DEPLOY_LOG_ERROR("[ERROR] Invalid device id address");
            return false;
        }
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
    m_ui->localDeviceDeploySpinnerLabel->setHidden(true);
    m_ui->deviceFarmDeploySpinnerLabel->setHidden(true);
    m_ui->localDeviceDeployButton->setText("Deploy to local device");
    m_ui->deviceFarmDeployButton->setText("Deploy to Device Farm");

    m_ui->deployStatusLabel->setText("Idle");
    ToggleInteractiveUI(true);

    m_devicePollTimer->start(0);

    m_deploymentCfg.m_hostRemoteLogPort = defaultRemoteConsolePort;

    // Reset the deploy worker based on to the currently selected
    // platform so local device polling will work.
    OnPlatformChanged(m_ui->platformComboBox->currentText());
}

void DeploymentToolWindow::InitializeUi()
{
    // setup
    m_ui->setupUi(this);

    m_ui->localDeviceDeployButton->setProperty("class", "Primary");
    m_ui->deviceFarmDeployButton->setProperty("class", "Primary");

    m_ui->remoteStatusIconLabel->setPixmap(m_notConnectedIcon.pixmap(QSize(16, 16)));

    m_ui->localDeviceDeploySpinnerLabel->setMovie(&m_animatedSpinningIcon);
    m_ui->localDeviceDeploySpinnerLabel->setHidden(true);

    m_ui->deviceFarmDeploySpinnerLabel->setMovie(&m_animatedSpinningIcon);
    m_ui->deviceFarmDeploySpinnerLabel->setHidden(true);

    m_animatedSpinningIcon.setScaledSize(QSize(16, 16));

    m_ui->outputTabWidget->setCurrentIndex(0);

    connect(m_ui->platformComboBox, &QComboBox::currentTextChanged, this, &DeploymentToolWindow::OnPlatformChanged);
    connect(m_ui->assetModeComboBox, &QComboBox::currentTextChanged, this, &DeploymentToolWindow::OnAssetModeChanged);
    connect(m_ui->shaderCompilerUseAPCheckBox, &QCheckBox::stateChanged, this, &DeploymentToolWindow::OnShaderCompilerUseAPToggle);
    connect(m_ui->localDeviceDeployButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnLocalDeviceDeployClick);
    connect(m_ui->deviceFarmDeployButton, &QPushButton::clicked, this, &DeploymentToolWindow::OnDeviceFarmDeployClick);

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

    QString platform = m_deploySettings.value(targetPlatformKey).toString();
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
    m_ui->assetModeComboBox->setCurrentText(m_deploySettings.value(assetModeKey).toString());
    if (m_deploySettings.contains(shaderCompilerIpAddressKey))
    {
        m_ui->shaderCompilerIpAddressTextField->setText(m_deploySettings.value(shaderCompilerIpAddressKey).toString());
    }
    if (m_deploySettings.contains(shaderCompilerUseAPKey))
    {
        m_ui->shaderCompilerUseAPCheckBox->setChecked(m_deploySettings.value(shaderCompilerUseAPKey).toBool());
    }

    // Device Farm settings (not saved in cloud)
    if (m_deploySettings.contains(deviceFarmExecutionTimeoutKey))
    {
        m_ui->deviceFarmExecutionTimeoutSpinBox->setValue(m_deploySettings.value(deviceFarmExecutionTimeoutKey).toInt());
    }

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

    m_ui->deviceFarmTestTypeComboBox->clear();
    m_ui->deviceFarmTestTypeComboBox->addItem(deviceFarmTestTypeBuiltInFuzz, deviceFarmTestTypeBuiltInFuzz);
    m_ui->deviceFarmTestTypeComboBox->setCurrentIndex(0);

    // re-evaluate what options should be shown
    OnPlatformChanged(m_ui->platformComboBox->currentText());
}

void DeploymentToolWindow::SaveUiState()
{
    // base options
    m_deploySettings.setValue(targetPlatformKey, m_ui->platformComboBox->currentText());
    m_deploySettings.setValue(buildConfigKey, m_ui->buildConfigComboBox->currentText());
    m_deploySettings.setValue(buildGameKey, m_ui->buildGameCheckBox->isChecked());

    // asset options
    m_deploySettings.setValue(assetModeKey, m_ui->assetModeComboBox->currentText());
    m_deploySettings.setValue(shaderCompilerIpAddressKey, m_ui->shaderCompilerIpAddressTextField->text());
    m_deploySettings.setValue(shaderCompilerUseAPKey, m_ui->shaderCompilerUseAPCheckBox->isChecked());    

    // Device Farm settings (not saved in cloud)
    m_deploySettings.setValue(deviceFarmExecutionTimeoutKey, m_ui->deviceFarmExecutionTimeoutSpinBox->value());

    // platform options
    m_deploySettings.setValue(cleanDeviceKey, m_ui->cleanDeviceCheckBox->isChecked());
}

void DeploymentToolWindow::ReloadDeviceFarmProjectsList()
{
    // Retrieve the projects from the device farm
    m_deviceFarmDriver->ListProjects([&](bool success, const AZStd::string& msg, AZStd::vector<DeployTool::DeviceFarmDriver::Project>& projects)
    {
        if (success)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_deviceFarmProjectsReloadMutex);

            m_ui->deviceFarmProjectNameComboBox->blockSignals(true);

            // get the current selection and clear the contents
            const QString currentProjectArn = m_ui->deviceFarmProjectNameComboBox->currentData().toString();
            m_ui->deviceFarmProjectNameComboBox->clear();

            // re-populate the projects list
            for (auto project : projects)
            {
                m_ui->deviceFarmProjectNameComboBox->addItem(project.name.c_str(), project.arn.c_str());
            }

            if (m_ui->deviceFarmProjectNameComboBox->count() == 0)
            {
                m_ui->deviceFarmProjectNameComboBox->addItem(noProjectsFound, noProjectsFound);
            }

            // attempt to reselect the previous selected device
            int index = m_ui->deviceFarmProjectNameComboBox->findData(currentProjectArn);
            if (index == -1)
            {
                index = 0;
            }

            m_ui->deviceFarmProjectNameComboBox->setCurrentIndex(index);

            m_ui->deviceFarmProjectNameComboBox->blockSignals(false);

            OnDeviceFarmProjectComboBoxValueChanged(index);
        }
        else
        {
            DEPLOY_LOG_ERROR("[ERROR] Failed to get project list, check Aws Credentials. %s", msg.c_str());
        }
    });
}

void DeploymentToolWindow::ReloadDeviceFarmDevicePoolsList(const AZStd::string& projectArn)
{
    // Retrieve the projects from the device farm
    m_deviceFarmDriver->ListDevicePools(projectArn, [&](bool success, const AZStd::string& msg, AZStd::vector<DeployTool::DeviceFarmDriver::DevicePool>& devicePools)
    {
        if (success)
        {
            AZStd::lock_guard<AZStd::mutex> locker(m_deviceFarmDevicePoolsReloadMutex);

            m_ui->deviceFarmDevicePoolNameComboBox->blockSignals(true);

            // get the current selection and clear the contents
            QString currentDevicePoolArn = m_ui->deviceFarmDevicePoolNameComboBox->currentData().toString();
            m_ui->deviceFarmDevicePoolNameComboBox->clear();

            // re-populate the device pool list
            for (auto devicePool : devicePools)
            {
                m_ui->deviceFarmDevicePoolNameComboBox->addItem(devicePool.name.c_str(), devicePool.arn.c_str());
            }

            if (m_ui->deviceFarmDevicePoolNameComboBox->count() == 0)
            {
                m_ui->deviceFarmDevicePoolNameComboBox->addItem(noDevicePoolsFound, noDevicePoolsFound);
            }

            // attempt to reselect the previous selected device
            int index = m_ui->deviceFarmDevicePoolNameComboBox->findData(currentDevicePoolArn);
            if (index == -1)
            {
                index = 0;
            }

            m_ui->deviceFarmDevicePoolNameComboBox->setCurrentIndex(index);

            m_ui->deviceFarmDevicePoolNameComboBox->blockSignals(false);
        }
        else
        {
            DEPLOY_LOG_ERROR("[ERROR] Failed to get device pool list, check Aws Credentials. %s", msg.c_str());
        }
    });
}

void DeploymentToolWindow::ReloadDeviceFarmRunsList(const AZStd::string& projectArn)
{
    // If there is already a reload in flight, wait for it to complete.
    while (m_waitingForListRunsResponse)
    {
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
    }

    m_waitingForListRunsResponse = true;

    // Retrieve the projects from the device farm
    m_deviceFarmDriver->ListRuns(projectArn, [&](bool success, const AZStd::string& msg, AZStd::vector<DeployTool::DeviceFarmDriver::Run>& runs)
    {
        AZStd::lock_guard<AZStd::mutex> locker(m_deviceFarmRunsReloadMutex);

        m_ui->deviceFarmRunsTable->clearContents();
        m_ui->deviceFarmRunsTable->setRowCount(runs.size());

        int row = 0;
        for (auto run : runs)
        {
            // Set the status icon
            QTableWidgetItem* statusItem = new QTableWidgetItem();
            if (run.result == "FAILED" || run.result == "ERRORED")
            {
                statusItem->setIcon(m_deviceFarmRunErrorIcon);
            }
            else if (run.result == "PASSED")
            {
                statusItem->setIcon(m_deviceFarmRunSuccessIcon);
            }
            else if (run.result == "STOPPED")
            {
                statusItem->setIcon(m_deviceFarmRunWarningIcon);
            }
            else
            {
                statusItem->setIcon(m_deviceFarmRunInProgressIcon);
            }
            statusItem->setToolTip(QStringLiteral("Current run status is '%1'").arg(run.result.c_str()));
            m_ui->deviceFarmRunsTable->setItem(row, deviceFarmRunsTableColumnStatus, statusItem);

            // Set the run name
            QTableWidgetItem* nameItem = new QTableWidgetItem(run.name.c_str());
            nameItem->setData(Qt::UserRole, QString(run.arn.c_str()));
            m_ui->deviceFarmRunsTable->setItem(row, deviceFarmRunsTableColumnName, nameItem);

            AZStd::string devicePoolId = DeployTool::DeviceFarmDriver::GetIdFromARN(run.devicePoolArn);

            // The Id needs to be further split down by a / to get the second part
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(devicePoolId.c_str(), tokens, '/');
            if (tokens.size() >= 2)
            {
                devicePoolId = tokens[1];
            }

            // try to find the name of the device pool in the list and set the device pool name.
            QString devicePoolName = "Not found";
            for (int x = 0; x < m_ui->deviceFarmDevicePoolNameComboBox->count(); x++)
            {
                // Just use the Id to compare, the built in Device Pools will have different prefixes
                // in the run data vs the pool list data.
                AZStd::string currentDevicePoolId = DeployTool::DeviceFarmDriver::GetIdFromARN(
                    m_ui->deviceFarmDevicePoolNameComboBox->itemData(x).toString().toUtf8().data()
                );

                // The Id needs to be further split down by a / to get the second part
                AZStd::vector<AZStd::string> tokens;
                AzFramework::StringFunc::Tokenize(currentDevicePoolId.c_str(), tokens, '/');
                if (tokens.size() >= 2)
                {
                    currentDevicePoolId = tokens[1];
                }

                if (currentDevicePoolId == devicePoolId)
                {
                    devicePoolName = m_ui->deviceFarmDevicePoolNameComboBox->itemText(x);
                }
            }
            m_ui->deviceFarmRunsTable->setItem(row, deviceFarmRunsTableColumnDevicePool, new QTableWidgetItem(devicePoolName));

            // Set the Test type
            QTableWidgetItem* testTypeItem = new QTableWidgetItem(run.testType.c_str());
            m_ui->deviceFarmRunsTable->setItem(row, deviceFarmRunsTableColumnTestType, testTypeItem);

            QString info = QStringLiteral("%1/%2 done").arg(run.completedJobs).arg(run.totalJobs);
            m_ui->deviceFarmRunsTable->setItem(row, deviceFarmRunsTableColumnProgress, new QTableWidgetItem(info));

            row++;
        }

        m_waitingForListRunsResponse = false;
    });
}

bool DeploymentToolWindow::DeviceFarmDeployTargetTabSelected()
{
    return m_ui->deployTargetsTabWidget->currentIndex() == 1;
}

#include <DeploymentToolWindow.moc>
