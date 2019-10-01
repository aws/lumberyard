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

#include "ConnectionEditDialog.h"

#include <AzCore/base.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <AzToolsFramework/UI/Logging/LogTableModel.h>
#include <AzToolsFramework/UI/Logging/LogTableItemDelegate.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <native/resourcecompiler/JobsModel.h>
#include <native/ui/ui_MainWindow.h>
#include <native/utilities/IniConfiguration.h>
#include <native/resourcecompiler/JobsModel.h>

#include "../utilities/GUIApplicationManager.h"
#include "../utilities/ApplicationServer.h"
#include "../connection/connectionManager.h"
#include "../connection/connection.h"
#include "../resourcecompiler/rccontroller.h"
#include "../resourcecompiler/RCJobSortFilterProxyModel.h"
#include "../shadercompiler/shadercompilerModel.h"

#include <limits.h>

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
#include <QFileSystemWatcher>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif // Q_OS_WIN

static const char* g_showContextDetailsKey = "ShowContextDetailsTable";
static const QString g_jobFilteredSearchWidgetState = QStringLiteral("jobFilteredSearchWidget");

MainWindow::Config MainWindow::loadConfig(QSettings& settings)
{
    using namespace AzQtComponents;

    Config config = defaultConfig();

    // Asset Status
    {
        ConfigHelpers::GroupGuard assetStatus(&settings, QStringLiteral("AssetStatus"));
        ConfigHelpers::read<int>(settings, QStringLiteral("JobStatusColumnWidth"), config.jobStatusColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobSourceColumnWidth"), config.jobSourceColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobPlatformColumnWidth"), config.jobPlatformColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobKeyColumnWidth"), config.jobKeyColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobCompletedColumnWidth"), config.jobCompletedColumnWidth);
    }

    // Event Log Details
    {
        ConfigHelpers::GroupGuard eventLogDetails(&settings, QStringLiteral("EventLogDetails"));
        ConfigHelpers::read<int>(settings, QStringLiteral("LogTypeColumnWidth"), config.logTypeColumnWidth);
    }

    return config;
}

MainWindow::Config MainWindow::defaultConfig()
{
    Config config;

    config.jobStatusColumnWidth = 100;
    config.jobSourceColumnWidth = 160;
    config.jobPlatformColumnWidth = 100;
    config.jobKeyColumnWidth = 120;
    config.jobCompletedColumnWidth = 80;

    config.logTypeColumnWidth = 150;

    return config;
}

MainWindow::MainWindow(GUIApplicationManager* guiApplicationManager, QWidget* parent)
    : QMainWindow(parent)
    , m_guiApplicationManager(guiApplicationManager)
    , m_jobSortFilterProxy(new AssetProcessor::JobSortFilterProxyModel(this))
    , m_logSortFilterProxy(new LogSortFilterProxy(this))
    , m_jobsModel(new AssetProcessor::JobsModel(this))
    , m_logsModel(new AzToolsFramework::Logging::LogTableModel(this))
    , ui(new Ui::MainWindow)
    , m_loggingPanel(nullptr)
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    ui->setupUi(this);

    // Don't show the "Filter by:" text on this filter widget
    ui->jobFilteredSearchWidget->clearLabelText();
    ui->detailsFilterWidget->clearLabelText();
}

void MainWindow::Activate()
{
    using namespace AssetProcessor;

    ui->projectLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Project"))
        .arg(m_guiApplicationManager->GetGameName()));

    ui->rootLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Root"))
        .arg(m_guiApplicationManager->GetSystemRoot().absolutePath()));

    ui->portLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Processor port"))
        .arg(m_guiApplicationManager->GetApplicationServer()->GetServerListeningPort()));

    connect(ui->supportButton, &QPushButton::clicked, this, &MainWindow::OnSupportClicked);

    ui->buttonList->addTab(QStringLiteral("Assets"));
    ui->buttonList->addTab(QStringLiteral("Shaders"));
    ui->buttonList->addTab(QStringLiteral("Connections"));
    ui->buttonList->addTab(QStringLiteral("Logs"));
    ui->buttonList->addTab(QStringLiteral("Tools"));

    connect(ui->buttonList, &AzQtComponents::SegmentBar::currentChanged, ui->dialogStack, &QStackedWidget::setCurrentIndex);
    const int startIndex = 0;
    ui->dialogStack->setCurrentIndex(startIndex);
    ui->buttonList->setCurrentIndex(startIndex);

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
    connect(ui->connectionTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::OnConnectionSelectionChanged);


    ui->editConnectionButton->setEnabled(false);
    ui->removeConnectionButton->setEnabled(false);
    connect(ui->editConnectionButton, &QPushButton::clicked, this, &MainWindow::OnEditConnection);
    connect(ui->addConnectionButton, &QPushButton::clicked, this, &MainWindow::OnAddConnection);
    connect(ui->removeConnectionButton, &QPushButton::clicked, this, &MainWindow::OnRemoveConnection);
    connect(ui->connectionTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        EditConnection(index);
    });

    ui->connectionTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->connectionTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::OnConnectionContextMenu);

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

    QRegExpValidator* hostNameValidator = new QRegExpValidator(validHostName, this);
    ui->whiteListAddHostNameLineEdit->setValidator(hostNameValidator);
    
    QRegExpValidator* ipValidator = new QRegExpValidator(validIP, this);
    ui->whiteListAddIPLineEdit->setValidator(ipValidator);

    //Job view
    m_jobSortFilterProxy->setSourceModel(m_jobsModel);
    m_jobSortFilterProxy->setDynamicSortFilter(true);
    m_jobSortFilterProxy->setFilterKeyColumn(JobsModel::ColumnSource);

    ui->jobTreeView->setModel(m_jobSortFilterProxy);
    ui->jobTreeView->setSortingEnabled(true);
    ui->jobTreeView->header()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    ui->jobTreeView->setToolTip(tr("Click to view Job Log"));

    ui->detailsFilterWidget->SetTypeFilterVisible(true);
    connect(ui->detailsFilterWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, m_logSortFilterProxy, &QSortFilterProxyModel::setFilterFixedString);
    connect(ui->detailsFilterWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, m_logSortFilterProxy, &LogSortFilterProxy::onTypeFilterChanged);

    // add filters for each logging type
    ui->detailsFilterWidget->AddTypeFilter("Status", "Debug", AzToolsFramework::Logging::LogLine::TYPE_DEBUG);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Message", AzToolsFramework::Logging::LogLine::TYPE_MESSAGE);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Warning", AzToolsFramework::Logging::LogLine::TYPE_WARNING);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Error", AzToolsFramework::Logging::LogLine::TYPE_ERROR);

    m_logSortFilterProxy->setDynamicSortFilter(true);
    m_logSortFilterProxy->setSourceModel(m_logsModel);
    m_logSortFilterProxy->setFilterKeyColumn(AzToolsFramework::Logging::LogTableModel::ColumnMessage);
    m_logSortFilterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    
    ui->jobLogTableView->setModel(m_logSortFilterProxy);
    ui->jobLogTableView->setItemDelegate(new AzToolsFramework::Logging::LogTableItemDelegate(ui->jobLogTableView));
    ui->jobLogTableView->setExpandOnSelection();

    connect(ui->jobTreeView->header(), &QHeaderView::sortIndicatorChanged, m_jobSortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::sort);
    connect(ui->jobTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::JobSelectionChanged);
    ui->jobFilteredSearchWidget->SetTypeFilterVisible(true);

    ui->jobContextLogTableView->setModel(new AzToolsFramework::Logging::ContextDetailsLogTableModel(ui->jobContextLogTableView));
    ui->jobContextLogTableView->setItemDelegate(new AzQtComponents::TableViewItemDelegate(ui->jobContextLogTableView));
    ui->jobContextLogTableView->setExpandOnSelection();

    // Don't collapse the jobContextContainer
    ui->jobDialogSplitter->setCollapsible(2, false);

    // Note: the settings can't be used in ::MainWindow(), because the application name
    // hasn't been set up and therefore the settings will load from somewhere different than later
    // on.
    QSettings settings;
    bool showContextDetails = settings.value(g_showContextDetailsKey, false).toBool();
    ui->jobContextLogDetails->setChecked(showContextDetails);

    // The context log details are shown by default, so only do anything with them on startup
    // if they should be hidden based on the loaded settings
    if (!showContextDetails)
    {
        SetContextLogDetailsVisible(showContextDetails);
    }

    connect(ui->jobContextLogDetails, &QCheckBox::toggled, this, [this](bool visible) {
        SetContextLogDetailsVisible(visible);

        QSettings settings;
        settings.setValue(g_showContextDetailsKey, visible);
    });

    connect(ui->jobLogTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::JobLogSelectionChanged);

    const auto statuses = {AzToolsFramework::AssetSystem::JobStatus::Failed,
        AzToolsFramework::AssetSystem::JobStatus::Completed,
        AzToolsFramework::AssetSystem::JobStatus::Queued,
        AzToolsFramework::AssetSystem::JobStatus::InProgress};
    const auto category = tr("Status");
    for (const auto status : statuses)
    {
        ui->jobFilteredSearchWidget->AddTypeFilter(category, JobsModel::GetStatusInString(status),
            QVariant::fromValue(status));
    }
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        m_jobSortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::OnJobStatusFilterChanged);
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_jobSortFilterProxy,
        static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetProcessor::JobSortFilterProxyModel::setFilterRegExp));
    {
        QSettings settings(this);
        ui->jobFilteredSearchWidget->readSettings(settings, g_jobFilteredSearchWidgetState);
    }
    auto writeJobFilterSettings = [this]()
    {
        QSettings settings(this);
        ui->jobFilteredSearchWidget->writeSettings(settings, g_jobFilteredSearchWidgetState);
    };
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        this, writeJobFilterSettings);
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        this, writeJobFilterSettings);

    //Shader view
    ui->shaderTreeView->setModel(m_guiApplicationManager->GetShaderCompilerModel());
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnTimeStamp, 80);
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnServer, 40);
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnError, 220);
    ui->shaderTreeView->header()->setSectionResizeMode(ShaderCompilerModel::ColumnError, QHeaderView::Stretch);
    ui->shaderTreeView->header()->setStretchLastSection(false);

    //Log View
    m_loggingPanel = ui->LoggingPanel;
    m_loggingPanel->SetStorageID(AZ_CRC("AssetProcessor::LogPanel", 0x75baa468));

    connect(ui->logButton, &QPushButton::clicked, this, &MainWindow::DesktopOpenJobLogs);

    if (!m_loggingPanel->LoadState())
    {
        // if unable to load state then show the default tabs
        ResetLoggingPanel();
    }

    AzQtComponents::ConfigHelpers::loadConfig<Config, MainWindow>(m_fileSystemWatcher, &m_config, QStringLiteral("style:AssetProcessorConfig.ini"), this, std::bind(&MainWindow::ApplyConfig, this));
    ApplyConfig();

    connect(m_loggingPanel, &AzToolsFramework::LogPanel::StyledLogPanel::TabsReset, this, &MainWindow::ResetLoggingPanel);
    connect(m_guiApplicationManager->GetRCController(), &AssetProcessor::RCController::JobStatusChanged, m_jobsModel, &AssetProcessor::JobsModel::OnJobStatusChanged);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::JobRemoved, m_jobsModel, &AssetProcessor::JobsModel::OnJobRemoved);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::SourceDeleted, m_jobsModel, &AssetProcessor::JobsModel::OnSourceRemoved);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::SourceFolderDeleted, m_jobsModel, &AssetProcessor::JobsModel::OnFolderRemoved);

    connect(ui->jobTreeView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowJobViewContextMenu);
    connect(ui->jobContextLogTableView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowLogLineContextMenu);
    connect(ui->jobLogTableView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowJobLogContextMenu);

    m_jobsModel->PopulateJobsFromDatabase();

    // Tools tab:
    connect(ui->fullScanButton, &QPushButton::clicked, this, &MainWindow::OnRescanButtonClicked);

    settings.beginGroup("Options");
    bool zeroAnalysisModeFromSettings = settings.value("EnableZeroAnalysis", QVariant(true)).toBool();
    settings.endGroup();

    QObject::connect(ui->modtimeSkippingCheckBox, &QCheckBox::stateChanged, this, 
        [this](int newCheckState)
    {
        bool newOption = newCheckState == Qt::Checked ? true : false;
        m_guiApplicationManager->GetAssetProcessorManager()->SetEnableModtimeSkippingFeature(newOption);
        QSettings settingsInCallback;
        settingsInCallback.beginGroup("Options");
        settingsInCallback.setValue("EnableZeroAnalysis", QVariant(newOption));
        settingsInCallback.endGroup();
    });

    m_guiApplicationManager->GetAssetProcessorManager()->SetEnableModtimeSkippingFeature(zeroAnalysisModeFromSettings);
    ui->modtimeSkippingCheckBox->setCheckState(zeroAnalysisModeFromSettings ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::OnRescanButtonClicked()
{
    m_guiApplicationManager->Rescan();
}

void MainWindow::OnSupportClicked(bool checked)
{
    QDesktopServices::openUrl(
        QStringLiteral("https://docs.aws.amazon.com/lumberyard/latest/userguide/asset-pipeline-processor.html"));
}

void MainWindow::EditConnection(const QModelIndex& index)
{
    if (index.data(ConnectionManager::UserConnectionRole).toBool())
    {
        ConnectionEditDialog dialog(m_guiApplicationManager->GetConnectionManager(), index, this);

        dialog.exec();
    }
}

void MainWindow::OnConnectionContextMenu(const QPoint& point)
{
    QPersistentModelIndex index = ui->connectionTreeView->indexAt(point);

    bool isUserConnection = index.isValid() && index.data(ConnectionManager::UserConnectionRole).toBool();
    QMenu menu(this);

    QAction* editConnectionAction = menu.addAction("&Edit Connection...");
    editConnectionAction->setEnabled(isUserConnection);
    connect(editConnectionAction, &QAction::triggered, this, [index, this] {
        EditConnection(index);
    });

    menu.exec(ui->connectionTreeView->viewport()->mapToGlobal(point));
}

void MainWindow::OnEditConnection(bool checked)
{
    auto selectedIndices = ui->connectionTreeView->selectionModel()->selectedRows();
    Q_ASSERT(selectedIndices.count() > 0);

    // Only edit the first connection. Guaranteed above by the edit connection button only being enabled if one connection is selected
    EditConnection(selectedIndices[0]);
}

void MainWindow::OnAddConnection(bool checked)
{
    m_guiApplicationManager->GetConnectionManager()->addUserConnection();
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

    QModelIndexList list = ui->connectionTreeView->selectionModel()->selectedRows();
    for (QModelIndex index: list)
    {
        manager->removeConnection(index);
    }
}

void MainWindow::OnConnectionSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    auto selectedIndices = ui->connectionTreeView->selectionModel()->selectedRows();
    int selectionCount = selectedIndices.count();

    bool anyUserConnectionsSelected = false;
    for (int i = 0; i < selectionCount; i++)
    {
        QModelIndex selectedIndex = selectedIndices[i];
        if (selectedIndex.data(ConnectionManager::UserConnectionRole).toBool())
        {
            anyUserConnectionsSelected = true;
            break;
        }
    }

    ui->removeConnectionButton->setEnabled(anyUserConnectionsSelected);
    ui->editConnectionButton->setEnabled(anyUserConnectionsSelected && (selectionCount == 1));
}


MainWindow::~MainWindow()
{
    m_guiApplicationManager = nullptr;
    delete ui;
}



void MainWindow::ShowWindow()
{
    AzQtComponents::bringWindowToTop(this);
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

    ui->APStatusValueLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Status"))
        .arg(text));
}

void MainWindow::HighlightAsset(QString assetPath)
{
    // Make sure that the currently active tab is the job list
    ui->buttonList->setCurrentIndex(0);

    // clear all filters
    ui->jobFilteredSearchWidget->ClearTextFilter();
    ui->jobFilteredSearchWidget->ClearTypeFilter();

    // jobs are listed with relative source asset paths, so we need to remove
    // the scan folder prefix from the absolute path
    bool success = true;
    AZStd::vector<AZStd::string> scanFolders;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders, scanFolders);
    if (success)
    {
        for (auto& scanFolder : scanFolders)
        {
            if (assetPath.startsWith(scanFolder.c_str(), Qt::CaseInsensitive))
            {
                // + 1 for the path separator
                assetPath = assetPath.mid(static_cast<int>(scanFolder.size() + 1));
                break;
            }
        }
    }

    // apply the filter for our asset path
    ui->jobFilteredSearchWidget->SetTextFilter(assetPath);

    // select the first item in the list
    ui->jobTreeView->setCurrentIndex(m_jobSortFilterProxy->index(0, 0));
}

void MainWindow::ApplyConfig()
{
    using namespace AssetProcessor;

    // Asset Status
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnStatus, m_config.jobStatusColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnSource, m_config.jobSourceColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnPlatform, m_config.jobPlatformColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnJobKey, m_config.jobKeyColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnCompleted, m_config.jobCompletedColumnWidth);

    // Event Log Details
    ui->jobLogTableView->header()->resizeSection(AzToolsFramework::Logging::LogTableModel::ColumnType, m_config.logTypeColumnWidth);
}

MainWindow::LogSortFilterProxy::LogSortFilterProxy(QObject* parentOjbect) : QSortFilterProxyModel(parentOjbect) 
{
}

bool MainWindow::LogSortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_logTypes.empty())
    {
        QModelIndex testIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        Q_ASSERT(testIndex.isValid());
        auto indexLogType = static_cast<AzToolsFramework::Logging::LogLine::LogType>(testIndex.data(AzToolsFramework::Logging::LogTableModel::LogTypeRole).toInt());
        if (!m_logTypes.contains(indexLogType))
        {
            return false;
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void MainWindow::LogSortFilterProxy::onTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
{
    beginResetModel();
    m_logTypes.clear();

    for (auto typeIter = activeTypeFilters.begin(), endIter = activeTypeFilters.end(); typeIter != endIter; ++typeIter)
    {
        m_logTypes.insert(static_cast<AzToolsFramework::Logging::LogLine::LogType>(typeIter->metadata.toInt()));
    }
    endResetModel();
}

void MainWindow::SetContextLogDetailsVisible(bool visible)
{
    using namespace AzQtComponents;

    const char* soloClass = "solo"; // see AssetsTab.qss; this is what provides the right margin around the widgets for the context details

    if (visible)
    {
        Style::removeClass(ui->jobContextLogDetails, soloClass);

        ui->jobLogLayout->removeWidget(ui->jobContextLogBar);
        ui->jobContextLayout->insertWidget(0, ui->jobContextLogBar);
    }
    else
    {
        Style::addClass(ui->jobContextLogDetails, soloClass);

        ui->jobContextLayout->removeWidget(ui->jobContextLogBar);
        ui->jobLogLayout->addWidget(ui->jobContextLogBar);
    }
    ui->jobContextContainer->setVisible(visible);
    ui->jobContextLogLabel->setVisible(visible);
}

void MainWindow::SetContextLogDetails(const QMap<QString, QString>& details)
{
    auto model = qobject_cast<AzToolsFramework::Logging::ContextDetailsLogTableModel*>(ui->jobContextLogTableView->model());

    if (details.isEmpty())
    {
        ui->jobContextLogStackedWidget->setCurrentWidget(ui->jobContextLogPlaceholderLabel);
    }
    else
    {
        ui->jobContextLogStackedWidget->setCurrentWidget(ui->jobContextLogTableView);
    }

    model->SetDetails(details);
}

void MainWindow::ClearContextLogDetails()
{
    SetContextLogDetails({});
}

void MainWindow::JobSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (selected.indexes().length() != 0)
    {
        // SelectionMode is set to QAbstractItemView::SingleSelection so there is only one selected item at a time
        AZStd::string jobLog = m_jobSortFilterProxy->data(selected.indexes()[0], AssetProcessor::JobsModel::DataRoles::logRole).toString().toUtf8().data();
        
        if (!m_logsModel)
        {
            return;
        }

        m_logsModel->Clear();
        AzToolsFramework::Logging::LogLine::ParseLog(jobLog.c_str(), jobLog.length() + 1,
            AZStd::bind(&AzToolsFramework::Logging::LogTableModel::AppendLineAsync, m_logsModel, AZStd::placeholders::_1));
        m_logsModel->CommitLines();
        ui->jobLogTableView->scrollToBottom();
        ui->jobLogStackedWidget->setCurrentWidget(ui->jobLogTableView);
    }
    // The only alternative is that there has been only a deselection, as otherwise both selected and deselected would be empty
    else
    {
        ui->jobLogStackedWidget->setCurrentWidget(ui->jobLogPlaceholderLabel);
    }

    ClearContextLogDetails();
}

void MainWindow::JobLogSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (selected.count() == 1)
    {
        const QMap<QString, QString> details = selected.indexes().first().data(AzToolsFramework::Logging::LogTableModel::DetailsRole).value<QMap<QString, QString>>();
        SetContextLogDetails(details);
    }
    else
    {
        ClearContextLogDetails();
    }
}

void MainWindow::DesktopOpenJobLogs()
{
    char resolvedDir[AZ_MAX_PATH_LEN * 2];
    QString currentDir;
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(AssetUtilities::ComputeJobLogFolder().c_str(), resolvedDir, sizeof(resolvedDir));

    currentDir = QString::fromUtf8(resolvedDir);

    if (QFile::exists(currentDir))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(currentDir));
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[Error] Logs folder (%s) does not exists.\n", currentDir.toUtf8().constData());
    }
}

void MainWindow::ResetLoggingPanel()
{
    if (m_loggingPanel)
    {
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Debug", "", ""));
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Messages", "", "", true, true, true, false));
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
    }
}

void MainWindow::ShowJobLogContextMenu(const QPoint& pos)
{
    using namespace AzToolsFramework::Logging;
    QModelIndex sourceIndex = ui->jobLogTableView->indexAt(pos);

    // If there is no index under the mouse cursor, let check the selected index of the view
    if (!sourceIndex.isValid())
    {
        const auto indexes = ui->jobLogTableView->selectionModel()->selectedIndexes();

        if (!indexes.isEmpty())
        {
            sourceIndex = indexes.first();
        }
    }

    QMenu menu;
    QAction* line = menu.addAction(tr("Copy Line"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.data(LogTableModel::LogLineTextRole).toString());
    });
    QAction* lineDetails = menu.addAction(tr("Copy Line With Details"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.data(LogTableModel::CompleteLogLineTextRole).toString());
    });
    menu.addAction(tr("Copy All"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(m_logsModel->toString(true));
    });

    if (!sourceIndex.isValid())
    {
        line->setEnabled(false);
        lineDetails->setEnabled(false);
    }

    menu.exec(ui->jobLogTableView->viewport()->mapToGlobal(pos));
}

static QString FindAbsoluteFilePath(const AssetProcessor::CachedJobInfo* cachedJobInfo)
{
    using namespace AzToolsFramework;

    bool result = false;
    AZ::Data::AssetInfo info;
    AZStd::string watchFolder;
    QByteArray assetNameUtf8 = cachedJobInfo->m_elementId.GetInputAssetName().toUtf8();
    AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, assetNameUtf8.constData(), info, watchFolder);
    if (!result)
    {
        AZ_Error("AssetProvider", false, "Failed to locate asset info for '%s'.", assetNameUtf8.constData());
    }

    return result
        ? QDir(watchFolder.c_str()).absoluteFilePath(info.m_relativePath.c_str())
        : QString();
};

static void SendShowInAssetBrowserResponse(const QString& filePath, ConnectionManager* connectionManager, unsigned int connectionId, QByteArray data)
{
    Connection* connection = connectionManager->getConnection(connectionId);
    if (!connection)
    {
        return;
    }

    AzToolsFramework::AssetSystem::WantAssetBrowserShowResponse response;
    bool readFromStream = AZ::Utils::LoadObjectFromBufferInPlace(data.data(), data.size(), response);
    AZ_Assert(readFromStream, "AssetProcessor failed to deserialize from stream");
    if (!readFromStream)
    {
        return;
    }

#ifdef AZ_PLATFORM_WINDOWS
    // Required on Windows to allow the other process to come to the foreground
    AllowSetForegroundWindow(response.m_processId);
#endif // #ifdef AZ_PLATFORM_WINDOWS

    AzToolsFramework::AssetSystem::AssetBrowserShowRequest message;
    message.m_filePath = filePath.toUtf8().data();
    connection->Send(AzFramework::AssetSystem::DEFAULT_SERIAL, message);
}

void MainWindow::ShowJobViewContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;

    auto cachedJobInfoAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->jobTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(proxyIndex);
        return m_jobsModel->getItem(sourceIndex.row());
    };

    const CachedJobInfo* item = cachedJobInfoAt(pos);

    if (item)
    {
        QMenu menu;
        QAction* action = menu.addAction("Show in Asset Browser", this, [&]()
        {
            ConnectionManager* connectionManager = m_guiApplicationManager->GetConnectionManager();

            QString filePath = FindAbsoluteFilePath(item);

            AzToolsFramework::AssetSystem::WantAssetBrowserShowRequest requestMessage;

            auto& connectionMap = connectionManager->getConnectionMap();
            auto connections = connectionMap.values();
            for (auto connection : connections)
            {
                using namespace AzFramework::AssetSystem;

                // Ask the Editor, and only the Editor, if it wants to receive
                // the message for showing an asset in the AssetBrowser.
                // This also allows the Editor to send back it's Process ID, which
                // allows the Windows platform to call AllowSetForegroundWindow()
                // which is required to bring the Editor window to the foreground
                if (connection->Identifier() == ConnectionIdentifiers::Editor)
                {
                    unsigned int connectionId = connection->ConnectionId();
                    connection->SendRequest(requestMessage, [connectionManager, connectionId, filePath](AZ::u32 type, QByteArray data) {
                        SendShowInAssetBrowserResponse(filePath, connectionManager, connectionId, data);
                    });
                }
            }
        });

        menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
        {
            AzQtComponents::ShowFileOnDesktop(FindAbsoluteFilePath(item));
        });

        menu.addAction(tr("Copy"), this, [&]()
        {
            QGuiApplication::clipboard()->setText(FindAbsoluteFilePath(item));
        });

        // Get the internal path to the log file
        const QModelIndex proxyIndex = ui->jobTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(proxyIndex);
        QVariant pathVariant = m_jobsModel->data(sourceIndex, JobsModel::logFileRole);

        // Get the absolute path of the log file
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        char resolvedPath[AZ_MAX_PATH_LEN];
        fileIO->ResolvePath(pathVariant.toByteArray().constData(), resolvedPath, AZ_MAX_PATH_LEN);

        QFileInfo fileInfo(resolvedPath);
        auto openLogFile = menu.addAction(tr("Open log file"), this, [&]()
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        });
        openLogFile->setEnabled(fileInfo.exists());

        auto logDir = fileInfo.absoluteDir();
        auto openLogFolder = menu.addAction(tr("Open folder with log file"), this, [&]()
        {
            if (fileInfo.exists())
            {
                AzQtComponents::ShowFileOnDesktop(fileInfo.absoluteFilePath());
            }
            else
            {
                // If the file doesn't exist, but the directory does, just open the directory
                AzQtComponents::ShowFileOnDesktop(logDir.absolutePath());
            }
        });
        // Only open and show the folder if the file actually exists, otherwise it's confusing
        openLogFolder->setEnabled(fileInfo.exists());

        menu.exec(ui->jobTreeView->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::ShowLogLineContextMenu(const QPoint& pos)
{
    using namespace AzToolsFramework::Logging;
    QModelIndex sourceIndex = ui->jobContextLogTableView->indexAt(pos);

    // If there is no index under the mouse cursor, let check the selected index of the view
    if (!sourceIndex.isValid())
    {
        const auto indexes = ui->jobContextLogTableView->selectionModel()->selectedIndexes();

        if (!indexes.isEmpty())
        {
            sourceIndex = indexes.first();
        }
    }

    QMenu menu;
    QAction* key = menu.addAction(tr("Copy Selected Key"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.sibling(sourceIndex.row(), ContextDetailsLogTableModel::ColumnKey).data().toString());
    });
    QAction* value = menu.addAction(tr("Copy Selected Value"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.sibling(sourceIndex.row(), ContextDetailsLogTableModel::ColumnValue).data().toString());
    });
    menu.addAction(tr("Copy All Values"), this, [&]()
    {
        auto model = qobject_cast<ContextDetailsLogTableModel*>(ui->jobContextLogTableView->model());
        QGuiApplication::clipboard()->setText(model->toString());
    });

    if (!sourceIndex.isValid())
    {
        key->setEnabled(false);
        value->setEnabled(false);
    }

    menu.exec(ui->jobContextLogTableView->viewport()->mapToGlobal(pos));
}

#include <native/ui/MainWindow.moc>
