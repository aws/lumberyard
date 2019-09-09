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

#include <QMainWindow>
#include <QStringList>
#include <QStringListModel>
#include "native/utilities/LogPanel.h"
#include <QPointer>
#include "native/assetprocessor.h"
#include <AzQtComponents/Components/FilteredSearchWidget.h>

namespace Ui {
    class MainWindow;
}
class GUIApplicationManager;
class QListWidgetItem;
class QFileSystemWatcher;
class QSettings;
namespace AssetProcessor
{
    class JobSortFilterProxyModel;
    class JobsModel;
}

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:
    struct Config
    {
        // Asset Status
        int jobStatusColumnWidth = -1;
        int jobSourceColumnWidth = -1;
        int jobPlatformColumnWidth = -1;
        int jobKeyColumnWidth = -1;
        int jobCompletedColumnWidth = -1;

        // Event Log Details
        int logTypeColumnWidth = -1;
    };

    /*!
     * Loads the button config data from a settings object.
     */
    static Config loadConfig(QSettings& settings);

    /*!
     * Returns default button config data.
     */
    static Config defaultConfig();

    explicit MainWindow(GUIApplicationManager* guiApplicationManager, QWidget* parent = 0);
    void Activate();
    ~MainWindow();

public Q_SLOTS:
    void ShowWindow();
    
    void SyncWhiteListAndRejectedList(QStringList whiteList, QStringList rejectedList);
    void FirstTimeAddedToRejctedList(QString ipAddress);
    void SaveLogPanelState();
    void OnAssetProcessorStatusChanged(const AssetProcessor::AssetProcessorStatusEntry entry);

    void OnRescanButtonClicked();
    void HighlightAsset(QString assetPath);

protected Q_SLOTS:
    void ApplyConfig();

private:

    class LogSortFilterProxy : public QSortFilterProxyModel
    {
    public:
        LogSortFilterProxy(QObject* parentOjbect);
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
        void onTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

    private:
        QSet<AzToolsFramework::Logging::LogLine::LogType> m_logTypes;
    };

    Ui::MainWindow* ui;
    GUIApplicationManager* m_guiApplicationManager;
    AzToolsFramework::Logging::LogTableModel* m_logsModel;
    AssetProcessor::JobSortFilterProxyModel* m_jobSortFilterProxy;
    LogSortFilterProxy* m_logSortFilterProxy;
    AssetProcessor::JobsModel* m_jobsModel;
    QPointer<AssetProcessor::LogPanel> m_loggingPanel;
    int m_processJobsCount = 0;
    int m_createJobCount = 0;
    QFileSystemWatcher* m_fileSystemWatcher;
    Config m_config;

    void SetContextLogDetailsVisible(bool visible);
    void SetContextLogDetails(const QMap<QString, QString>& details);
    void ClearContextLogDetails();

    void EditConnection(const QModelIndex& index);
    void OnConnectionContextMenu(const QPoint& point);
    void OnEditConnection(bool checked);
    void OnAddConnection(bool checked);
    void OnRemoveConnection(bool checked);
    void OnSupportClicked(bool checked);
    void OnConnectionSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    QStringListModel m_rejectedAddresses;
    QStringListModel m_whitelistedAddresses;
    
    void OnWhiteListedConnectionsListViewClicked();
    void OnRejectedConnectionsListViewClicked();
    
    void OnWhiteListCheckBoxToggled();
    
    void OnAddHostNameWhiteListButtonClicked();
    void OnAddIPWhiteListButtonClicked();

    void OnToWhiteListButtonClicked();
    void OnToRejectedListButtonClicked();

    void JobSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void JobLogSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void DesktopOpenJobLogs();

    void ResetLoggingPanel();
    void ShowJobViewContextMenu(const QPoint& pos);
    void ShowLogLineContextMenu(const QPoint& pos);
    void ShowJobLogContextMenu(const QPoint& pos);
};

