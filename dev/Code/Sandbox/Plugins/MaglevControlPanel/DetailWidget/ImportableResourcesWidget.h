/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <QStandardItem>
#include <QComboBox>
#include <QTableView>
#include <QProgressDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QModelIndex>

#include <QSortFilterProxyModel>

#include <DetailWidget/PopupDialogWidget.h>

class ResourceManagementView;
class IAWSImporterModel;
class IImporterListModel;

class ImportableResourcesWidget
    : public PopupDialogWidget
{
    Q_OBJECT

public:

    ImportableResourcesWidget(QString resource_group, QSharedPointer<IAWSImporterModel> m_importerModel, ResourceManagementView* view);

private:

    enum class State
    {
        Listing,
        Configuring
    };

    QSharedPointer<IAWSImporterModel> m_importerModel;
    QSharedPointer<QSortFilterProxyModel> m_filterListProxyModel;
    QSharedPointer<QSortFilterProxyModel> m_filterConfigurationProxyModel;

    QComboBox* m_regionBox;
    QPushButton* m_typeButton;
    QLineEdit* m_searchEdit;
    QTableView* m_listTable;
    QTableView* m_selectionTable;
    QProgressDialog* m_progress;
    QLabel* m_loadingProcess;
    PopupDialogWidget* m_dialog;
    State m_currentState;
    QMenu* m_buttonMenu;
    QStringList m_typeList;

    bool m_listfinished;
    int m_finishedImportRequests;

    QString m_resource_group;
    QList<int> m_selectedRowList;

    ResourceManagementView* m_view;

    void CreateRegionRow();
    void CreateSearchRow();
    void CreateResourcesTable();
    void CreateLoadingLabel();
    void CreateConfigurationWindow();

    bool ResourceIsChosen();

    void ListStart(QLabel* loadingProcess);
    void ShowImportProcess();
    void ImportResource();

    int GetWidth(QTableView* table);
    int GetHeight(QTableView* table);

signals:

    void UpdateResourceNames(bool allNamesGiven);

public slots:

    void OnFilterEntered();
    void OnTypeButtonClicked(QAction* action);
    void OnRegionChanged();

    void OnListFinished();

    void OnTableContextMenuRequested(QPoint pos);
    void OnMenuConfigureClicked(int row);
    void OnMenuDeleteClicked(int row);
    void OnMenuCopyClicked(QModelIndex index);
    void ViewConsoleResource(const QString& resourceType, const QString& resourceName, const QString& region);

    void OnItemsChanged(QStandardItem* item);
    void OnImporterOutput(const QVariant& output, const char* outputType);

    void OnPrimaryButtonClick();
    void CheckResourceNameGiven(QModelIndex current, QModelIndex previous);

    void EnableConfigureButton(bool enable);
    void EnableImportButton(bool enable);
};