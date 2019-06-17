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

#include <QAbstractItemModel>
#include <QDialog>
#include <QSortFilterProxyModel>

#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>

#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/View/Widgets/StatisticsDialog/NodeUsageTreeItem.h>
#include <ScriptCanvas/Core/Core.h>

namespace Ui
{
    class ScriptCanvasStatisticsDialog;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetNodeUsageFilterModel
        : public QSortFilterProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetNodeUsageFilterModel, AZ::SystemAllocator, 0);

        ScriptCanvasAssetNodeUsageFilterModel();

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

        void SetFilter(const QString& filterName);
        void SetNodeTypeFilter(const ScriptCanvas::NodeTypeIdentifier& nodeType);

    private:

        QString m_filter;
        QRegExp m_regex;

        ScriptCanvas::NodeTypeIdentifier m_nodeIdentifier;
    };

    class StatisticsDialog
        : public QDialog
        , AzFramework::AssetSystemBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(StatisticsDialog, AZ::SystemAllocator, 0);

        StatisticsDialog(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* scriptCanvasAssetModel, QWidget* widget = nullptr);
        ~StatisticsDialog();

        void InitStatisticsWindow();
        void ResetModel();

        //! Called by the AssetProcessor when an asset in the cache has been modified.
        void AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage /*message*/) override;

        //! Called by the AssetProcessor when an asset in the cache has been removed.
        void AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage /*message*/) override;

        void OnScriptCanvasAssetClicked(const QModelIndex& modelIndex);

        void showEvent(QShowEvent* showEvent) override;

    public slots:
        void OnSelectionCleared();
        void OnItemSelected(const GraphCanvas::GraphCanvasTreeItem* treeItem);

        void OnFilterUpdated(const QString& filterText);
        void OnScriptCanvasAssetRowsInserted(QModelIndex modelIndex, int first, int last);

    private:

        void TraverseTree(QModelIndex index = QModelIndex());
        void ProcessAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

        const NodePaletteModel& m_nodePaletteModel;

        AZStd::unique_ptr<Ui::ScriptCanvasStatisticsDialog>    m_ui;
        NodePaletteNodeUsageRootItem*                          m_treeRoot;

        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel*     m_scriptCanvasAssetBrowserModel;

        ScriptCanvasAssetNodeUsageTreeItemRoot* m_scriptCanvasAssetTreeRoot;
        GraphCanvas::GraphCanvasTreeModel* m_scriptCanvasAssetTree;
        ScriptCanvasAssetNodeUsageFilterModel* m_scriptCanvasAssetFilterModel;
    };
}