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
#include "MainWindow.h"

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/UI/Logging/GenericLogPanel.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

#include <native/ui/ui_MainWindow.h>
#include <native/utilities/IniConfiguration.h>

#include "../utilities/GUIApplicationManager.h"
#include "../utilities/ApplicationServer.h"
#include "../connection/connectionManager.h"
#include "../resourcecompiler/rccontroller.h"
#include "../resourcecompiler/RCJobSortFilterProxyModel.h"
#include "../shadercompiler/shadercompilerModel.h"

#include <QDialog>
#include <QTreeView>
#include <QHeaderView>
#include <QAction>
#include <QLineEdit>
#include <QCheckBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkInterface>
#include <QGroupBox>
#include <QHostInfo>
#include <QWindow>
#include <QHostAddress>
#include <QRegExpValidator>
#include <QMessageBox>
#include <QSettings>
#include <native/resourcecompiler/JobsModel.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif // Q_OS_WIN

MainWindow::MainWindow(GUIApplicationManager* guiApplicationManager, QWidget* parent)
    : QMainWindow(parent)
    , m_guiApplicationManager(guiApplicationManager)
    , m_sortFilterProxy(new AssetProcessor::JobSortFilterProxyModel(this))
    , m_jobsModel(new AssetProcessor::JobsModel(this))
    , ui(new Ui::MainWindow)
    , m_loggingPanel(nullptr)
{
    ui->setupUi(this);
}

void MainWindow::Activate()
{
    using namespace AssetProcessor;

    int listeningPort = m_guiApplicationManager->GetApplicationServer()->GetServerListeningPort();
    ui->port->setText(QString::number(listeningPort));

    ui->gameProject->setText(m_guiApplicationManager->GetGameName());
    ui->gameRoot->setText(m_guiApplicationManager->GetSystemRoot().absolutePath());

    connect(ui->buttonList, &QListWidget::currentItemChanged, this, &MainWindow::OnPaneChanged);
    connect(ui->supportButton, &QPushButton::clicked, this, &MainWindow::OnSupportClicked);

    ui->buttonList->setCurrentRow(0);

    //Connection view
    ui->connectionTreeView->setModel(m_guiApplicationManager->GetConnectionManager());
    ui->connectionTreeView->setEditTriggers(QAbstractItemView::CurrentChanged);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::StatusColumn, 100);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::IdColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::IpColumn, 150);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::PortColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::PlatformColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::AutoConnectColumn, 40);
    ui->connectionTreeView->header()->setSectionResizeMode(ConnectionManager::PlatformColumn, QHeaderView::Stretch);
    ui->connectionTreeView->header()->setStretchLastSection(false);

    connect(ui->addConnectionButton, &QPushButton::clicked, this, &MainWindow::OnAddConnection);
    connect(ui->removeConnectionButton, &QPushButton::clicked, this, &MainWindow::OnRemoveConnection);

    //white list connections
    connect(m_guiApplicationManager->GetConnectionManager(), &ConnectionManager::FirstTimeAddedToRejctedList, this, &MainWindow::FirstTimeAddedToRejctedList);
    connect(m_guiApplicationManager->GetConnectionManager(), &ConnectionManager::SyncWhiteListAndRejectedList, this, &MainWindow::SyncWhiteListAndRejectedList);
    connect(ui->whiteListWhiteListedConnectionsListView, &QListView::clicked, this, &MainWindow::OnWhiteListedConnectionsListViewClicked);
    ui->whiteListWhiteListedConnectionsListView->setModel(&m_whitelistedAddresses);
    connect(ui->whiteListRejectedConnectionsListView, &QListView::clicked, this, &MainWindow::OnRejectedConnectionsListViewClicked);
    ui->whiteListRejectedConnectionsListView->setModel(&m_rejectedAddresses);
    
    connect(ui->whiteListEnableCheckBox, &QCheckBox::toggled, this, &MainWindow::OnWhiteListCheckBoxToggled);
    
    connect(ui->whiteListAddHostNamePushButton, &QPushButton::clicked, this, &MainWindow::OnAddHostNameWhiteListButtonClicked);
    connect(ui->whiteListAddIPPushButton, &QPushButton::clicked, this, &MainWindow::OnAddIPWhiteListButtonClicked);
    
    connect(ui->whiteListToWhiteListPushButton, &QPushButton::clicked, this, &MainWindow::OnToWhiteListButtonClicked);
    connect(ui->whiteListToRejectedListPushButton, &QPushButton::clicked, this, &MainWindow::OnToRejectedListButtonClicked);

    //set the input validator for ip addresses on the add address line edit
    QRegExp validHostName("^((?=.{1,255}$)[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?(?:\\.[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?)*\\.?)$");
    QRegExp validIP("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(\\/([0-9]|[1-2][0-9]|3[0-2]))?$|^((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:)))(%.+)?s*(\\/([0-9]|[1-9][0-9]|1[0-1][0-9]|12[0-8]))?$");

    QRegExpValidator *hostNameValidator = new QRegExpValidator(validHostName, this);
    ui->whiteListAddHostNameLineEdit->setValidator(hostNameValidator);
    
    QRegExpValidator *ipValidator = new QRegExpValidator(validIP, this);
    ui->whiteListAddIPLineEdit->setValidator(ipValidator);

    ui->whiteListEnableCheckBox->setStyleSheet("\
        QCheckBox::indicator:checked{image: url(:/AssetProcessor_checkbox_blue_checked.png);}\
        QCheckBox::indicator:checked:hover{image: url(:/AssetProcessor_checkbox_blue_checked.png);}\
        QCheckBox::indicator:checked:pressed{image: url(:/AssetProcessor_checkbox_blue_checked.png);}\
        QCheckBox::indicator:indeterminate:hover{image: url(:/AssetProcessor_checkbox_blue_checked.png);}\
        QCheckBox::indicator:indeterminate:pressed{image: url(:/AssetProcessor_checkbox_blue_checked.png);}");

    ui->whiteListTopBarWidget->setStyleSheet("background-color: rgb(71, 71, 73);");
    ui->whiteListBottomBarWidget->setStyleSheet("background-color: rgb(71, 71, 73);");
    ui->whiteListAddIPLineEdit->setStyleSheet("background-color: rgb(48, 48, 48);");
    ui->whiteListAddHostNameLineEdit->setStyleSheet("background-color: rgb(48, 48, 48);");

    //Job view
    m_sortFilterProxy->setSourceModel(m_jobsModel);
    m_sortFilterProxy->setDynamicSortFilter(true);
    m_sortFilterProxy->setFilterKeyColumn(JobsModel::ColumnSource);

    ui->jobTreeView->setModel(m_sortFilterProxy);
    ui->jobTreeView->setSortingEnabled(true);
    ui->jobTreeView->header()->setDefaultAlignment(Qt::AlignHCenter);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnStatus, 100);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnSource, 160);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnPlatform, 100);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnJobKey, 120);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnCompleted, 80);
    ui->jobTreeView->header()->setSectionResizeMode(JobsModel::ColumnSource, QHeaderView::Stretch);
    ui->jobTreeView->header()->setStretchLastSection(false);

    ui->jobTreeView->setToolTip(tr("Double click to view Job Log"));

    connect(ui->jobTreeView->header(), &QHeaderView::sortIndicatorChanged, m_sortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::sort);
    connect(ui->jobTreeView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &index)
    {
        // read the log file and show it to the user
        AZStd::string jobLog = m_sortFilterProxy->data(index, AssetProcessor::JobsModel::DataRoles::logRole).toString().toUtf8().data();
        QDialog logDialog;
        logDialog.setMinimumSize(1024, 400);
        QHBoxLayout* pLayout = new QHBoxLayout(&logDialog);
        logDialog.setLayout(pLayout);
        AzToolsFramework::LogPanel::GenericLogPanel* logPanel = new AzToolsFramework::LogPanel::GenericLogPanel(&logDialog);
        logPanel->SetCurrentItemsExpandToFit(true);
        logDialog.layout()->addWidget(logPanel);
        logPanel->ParseData(jobLog.c_str(), jobLog.size());

        auto tabsResetFunction = [logPanel]() -> void
        {
            logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("All output", "", ""));
            logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
        };

        tabsResetFunction();

        connect(logPanel, &AzToolsFramework::LogPanel::BaseLogPanel::TabsReset, this, tabsResetFunction);
        logDialog.adjustSize();
        logDialog.exec();
       
    });
    connect(ui->jobFilterLineEdit, &QLineEdit::textChanged, this, &MainWindow::OnJobFilterRegExpChanged);
    connect(ui->jobFilterClearButton, &QPushButton::clicked, this, &MainWindow::OnJobFilterClear);

    //Shader view
    ui->shaderTreeView->setModel(m_guiApplicationManager->GetShaderCompilerModel());
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnTimeStamp, 80);
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnServer, 40);
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnError, 220);
    ui->shaderTreeView->header()->setSectionResizeMode(ShaderCompilerModel::ColumnError, QHeaderView::Stretch);
    ui->shaderTreeView->header()->setStretchLastSection(false);

    //Log View
    m_loggingPanel = new AssetProcessor::LogPanel(ui->LogDialog);
    m_loggingPanel->setObjectName("LoggingPanel");
    m_loggingPanel->SetStorageID(AZ_CRC("AssetProcessor::LogPanel", 0x75baa468));
    ui->LogDialog->layout()->addWidget(m_loggingPanel);
    QPushButton* logButton = new QPushButton(ui->LogDialog);
    logButton->setText(tr("Open Logs Folder"));
    QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(logButton->sizePolicy().hasHeightForWidth());
    logButton->setSizePolicy(sizePolicy2);
    ui->LogDialog->layout()->addWidget(logButton);

    connect(logButton, &QPushButton::clicked, this, []()
    {
        QString currentDir(AZ::IO::FileIOBase::GetInstance()->GetAlias("@log@"));
        QDir dir(currentDir);
        QString logFolder = dir.filePath("logs");
        if (QFile::exists(logFolder))
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(logFolder));
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[Error] Logs folder (%s) does not exists.\n", logFolder.toUtf8().constData());
        }
    });

    auto loggingPanelResetFunction = [this]() -> void
    {
        if (m_loggingPanel)
        {
            m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Debug", "", ""));
            m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Messages", "", "", true , true, true, false));
            m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
        }
    };

    if (!m_loggingPanel->LoadState())
    {
        // if unable to load state then show the default tabs
        loggingPanelResetFunction();
    }

    connect(m_loggingPanel, &AzToolsFramework::LogPanel::BaseLogPanel::TabsReset, this, loggingPanelResetFunction);
    connect(m_guiApplicationManager->GetRCController(), &AssetProcessor::RCController::JobStatusChanged, m_jobsModel, &AssetProcessor::JobsModel::OnJobStatusChanged);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::JobRemoved, m_jobsModel, &AssetProcessor::JobsModel::OnJobRemoved);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::SourceDeleted, m_jobsModel, &AssetProcessor::JobsModel::OnSourceRemoved);
    m_jobsModel->PopulateJobsFromDatabase();
    // Tools tab:
    connect(ui->fullScanButton, &QPushButton::clicked, this, &MainWindow::OnRescanButtonClicked);

    QSettings settings;
    settings.beginGroup("Options");
    bool fastAnalysModeFromSettings = settings.value("EnableFastAnalysis", QVariant(false)).toBool();
    settings.endGroup();

    QObject::connect(ui->fastAnalysisCheckBox, &QCheckBox::stateChanged, this, 
        [this](int newCheckState)
        {
            bool newOption = newCheckState == Qt::Checked ? true : false;
            m_guiApplicationManager->GetAssetProcessorManager()->SetEnableAnalysisSkippingFeature(newOption);
            QSettings settingsInCallback;
            settingsInCallback.beginGroup("Options");
            settingsInCallback.setValue("EnableFastAnalysis", QVariant(newOption));
            settingsInCallback.endGroup();
        });
        
    ui->fastAnalysisCheckBox->setCheckState(fastAnalysModeFromSettings ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::OnRescanButtonClicked()
{
    m_guiApplicationManager->Rescan();
}

void MainWindow::OnSupportClicked(bool checked)
{
    QDesktopServices::openUrl(QString("https://docs.aws.amazon.com/lumberyard/latest/userguide/asset-pipeline-processor.html"));
}

void MainWindow::OnJobFilterClear(bool checked)
{
    ui->jobFilterLineEdit->setText(QString());
    ui->filterLabel->setText(QString());
}

void MainWindow::OnJobFilterRegExpChanged()
{
    QRegExp regExp(ui->jobFilterLineEdit->text(), Qt::CaseInsensitive, QRegExp::PatternSyntax::RegExp);
    bool isFilterTextEmpty = ui->jobFilterLineEdit->text().isEmpty();
    m_sortFilterProxy->OnFilterRegexExpChanged(regExp, isFilterTextEmpty);
    if (isFilterTextEmpty)
    {
        ui->filterLabel->setText(QString());
    }
    else
    {
        ui->filterLabel->setText(QString("Showing %1 out of %2 rows.").arg(m_sortFilterProxy->rowCount()).arg(m_jobsModel->rowCount()));
    }
}

void MainWindow::OnAddConnection(bool checked)
{
    m_guiApplicationManager->GetConnectionManager()->addConnection();
}

void MainWindow::OnWhiteListedConnectionsListViewClicked() 
{
    ui->whiteListRejectedConnectionsListView->clearSelection();
}

void MainWindow::OnRejectedConnectionsListViewClicked()
{
    ui->whiteListWhiteListedConnectionsListView->clearSelection();
}

void MainWindow::OnWhiteListCheckBoxToggled() 
{
    if (!ui->whiteListEnableCheckBox->isChecked())
    {
        //warn this is not safe
        if(QMessageBox::Ok == QMessageBox::warning(this, tr("!!!WARNING!!!"), tr("Turning off white listing poses a significant security risk as it would allow any device to connect to your asset processor and that device will have READ/WRITE access to the Asset Processors file system. Only do this if you sure you know what you are doing and accept the risks."),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
            ui->whiteListRejectedConnectionsListView->clearSelection();
            ui->whiteListWhiteListedConnectionsListView->clearSelection();
            ui->whiteListAddHostNameLineEdit->setEnabled(false);
            ui->whiteListAddHostNamePushButton->setEnabled(false);
            ui->whiteListAddIPLineEdit->setEnabled(false);
            ui->whiteListAddIPPushButton->setEnabled(false);
            ui->whiteListWhiteListedConnectionsListView->setEnabled(false);
            ui->whiteListRejectedConnectionsListView->setEnabled(false);
            ui->whiteListToWhiteListPushButton->setEnabled(false);
            ui->whiteListToRejectedListPushButton->setEnabled(false);
        }
        else
        {
            ui->whiteListEnableCheckBox->setChecked(true);
        }
    }
    else
    {
        ui->whiteListAddHostNameLineEdit->setEnabled(true);
        ui->whiteListAddHostNamePushButton->setEnabled(true);
        ui->whiteListAddIPLineEdit->setEnabled(true);
        ui->whiteListAddIPPushButton->setEnabled(true);
        ui->whiteListWhiteListedConnectionsListView->setEnabled(true);
        ui->whiteListRejectedConnectionsListView->setEnabled(true);
        ui->whiteListToWhiteListPushButton->setEnabled(true);
        ui->whiteListToRejectedListPushButton->setEnabled(true);
    }
    
    m_guiApplicationManager->GetConnectionManager()->WhiteListingEnabled(ui->whiteListEnableCheckBox->isChecked());
}

void MainWindow::OnAddHostNameWhiteListButtonClicked()
{
    QString text = ui->whiteListAddHostNameLineEdit->text();
    const QRegExpValidator *hostnameValidator = static_cast<const QRegExpValidator *>(ui->whiteListAddHostNameLineEdit->validator());
    int pos;
    QValidator::State state = hostnameValidator->validate(text, pos);
    if (state == QValidator::Acceptable)
    {
        m_guiApplicationManager->GetConnectionManager()->AddWhiteListedAddress(text);
        ui->whiteListAddHostNameLineEdit->clear();
    }
}

void MainWindow::OnAddIPWhiteListButtonClicked()
{
    QString text = ui->whiteListAddIPLineEdit->text();
    const QRegExpValidator *ipValidator = static_cast<const QRegExpValidator *>(ui->whiteListAddIPLineEdit->validator());
    int pos;
    QValidator::State state = ipValidator->validate(text, pos);
    if (state== QValidator::Acceptable)
    {
        m_guiApplicationManager->GetConnectionManager()->AddWhiteListedAddress(text);
        ui->whiteListAddIPLineEdit->clear();
    }
}

void MainWindow::OnToRejectedListButtonClicked()
{
    QModelIndexList indices = ui->whiteListWhiteListedConnectionsListView->selectionModel()->selectedIndexes();
    if(!indices.isEmpty() && indices.first().isValid())
    {
        QString itemText = indices.first().data(Qt::DisplayRole).toString();
        m_guiApplicationManager->GetConnectionManager()->RemoveWhiteListedAddress(itemText);
        m_guiApplicationManager->GetConnectionManager()->AddRejectedAddress(itemText, true);
    }
}

void MainWindow::OnToWhiteListButtonClicked()
{
    QModelIndexList indices = ui->whiteListRejectedConnectionsListView->selectionModel()->selectedIndexes();
    if (!indices.isEmpty() && indices.first().isValid())
    {
        QString itemText = indices.front().data(Qt::DisplayRole).toString();
        m_guiApplicationManager->GetConnectionManager()->RemoveRejectedAddress(itemText);
        m_guiApplicationManager->GetConnectionManager()->AddWhiteListedAddress(itemText);
    }
}

void MainWindow::OnRemoveConnection(bool checked)
{
    ConnectionManager* manager = m_guiApplicationManager->GetConnectionManager();

    QModelIndexList list = ui->connectionTreeView->selectionModel()->selectedIndexes();
    for (QModelIndex index: list)
    {
        manager->removeConnection(index);
    }
}

void MainWindow::OnPaneChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!current)
    {
        current = previous;
    }

    ui->dialogStack->setCurrentIndex(ui->buttonList->row(current));
}


MainWindow::~MainWindow()
{
    m_guiApplicationManager = nullptr;
    delete ui;
}



void MainWindow::ShowWindow()
{
    setWindowState(Qt::WindowActive);
    show();
    raise();

    // activateWindow only works if the window is shown, so let's include
    // a slight delay to make sure the events for showing the window
    // have been processed.
    QTimer::singleShot(0, this, [this]() {
        activateWindow();
        windowHandle()->requestActivate();

        // The Windows OS has all kinds of issues with bringing a window to the front
        // from another application.
        // For the moment, we assume that any other application calling this
        // has called AllowSetForegroundWindow(), otherwise, the code below
        // won't work.
#ifdef Q_OS_WIN
        HWND hwnd = reinterpret_cast<HWND>(winId());
        SetForegroundWindow(hwnd);

        // Hack to get this to show up for sure
        ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif // Q_OS_WIN
    });
}


void MainWindow::SyncWhiteListAndRejectedList(QStringList whiteList, QStringList rejectedList)
{
    m_whitelistedAddresses.setStringList(whiteList);
    m_rejectedAddresses.setStringList(rejectedList);
}

void MainWindow::FirstTimeAddedToRejctedList(QString ipAddress)
{
    QMessageBox* msgBox = new QMessageBox(this);
    msgBox->setText(tr("!!!Rejected Connection!!!"));
    msgBox->setInformativeText(ipAddress + tr(" tried to connect and was rejected because it was not on the white list. If you want this connection to be allowed go to connections tab and add it to white list."));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setDefaultButton(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::NonModal);
    msgBox->setModal(false);
    msgBox->show();
}

void MainWindow::SaveLogPanelState()
{
    if (m_loggingPanel)
    {
        m_loggingPanel->SaveState();
    }
}

void MainWindow::OnAssetProcessorStatusChanged(const AssetProcessor::AssetProcessorStatusEntry entry)
{
    using namespace AssetProcessor;
    QString text;
    switch (entry.m_status)
    {
    case AssetProcessorStatus::Initializing_Gems:
        text = tr("Initializing Gem...%1").arg(entry.m_extraInfo);
        break;
    case AssetProcessorStatus::Initializing_Builders:
        text = tr("Initializing Builders...");
        break;
    case AssetProcessorStatus::Scanning_Started:
        text = tr("Scanning...");
        break;
    case AssetProcessorStatus::Analyzing_Jobs:
        m_createJobCount = entry.m_count;

        if (m_processJobsCount + m_createJobCount > 0)
        {
            text = tr("Working, analyzing jobs remaining %1, processing jobs remaining %2...").arg(m_createJobCount).arg(m_processJobsCount);
        }
        else
        {
            text = tr("Idle...");
            m_guiApplicationManager->RemoveOldTempFolders();  
        }
        break;
    case AssetProcessorStatus::Processing_Jobs:
        m_processJobsCount = entry.m_count;

        if (m_processJobsCount + m_createJobCount > 0)
        {
            text = tr("Working, analyzing jobs remaining %1, processing jobs remaining %2...").arg(m_createJobCount).arg(m_processJobsCount);
        }
        else
        {
            text = tr("Idle...");
            m_guiApplicationManager->RemoveOldTempFolders();
        }
        break;
    default:
        text = QString();
    }
    ui->APStatusValueLabel->setText(text);
}

#include <native/ui/MainWindow.moc>
