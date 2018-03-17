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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <QMimeData>
#include <QModelIndex>
#include <QPointer>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserModel;
        class AssetBrowserFilterModel;
        class EntryDelegate;

        class AssetBrowserTreeView
            : public QTreeViewWithStateSaving
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTreeView, AZ::SystemAllocator, 0);

            explicit AssetBrowserTreeView(QWidget* parent = nullptr);
            ~AssetBrowserTreeView() override;

            //////////////////////////////////////////////////////////////////////////
            // QTreeView
            //////////////////////////////////////////////////////////////////////////
            void setModel(QAbstractItemModel* model) override;

            void LoadState(const QString& name);
            void SaveState() const;

            AZStd::vector<AssetBrowserEntry*> GetSelectedAssets() const;
            void SelectProduct(AZ::Data::AssetId assetID);

            void SetThumbnailContext(const char* context) const;
            void SetShowSourceControlIcons(bool showSourceControlsIcons) const;

        Q_SIGNALS:
            void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);

        protected:
            void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;
            void startDrag(Qt::DropActions supportedActions) override;

        protected Q_SLOTS:
            void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
            void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;
            
        private:
            QMimeData m_mimeDataContainer;
            QPointer<AssetBrowserModel> m_assetBrowserModel;
            QPointer<AssetBrowserFilterModel> m_assetBrowserSortFilterProxyModel;
            QScopedPointer<EntryDelegate> m_delegate;
            bool m_sendMetrics = false;

            bool SelectProduct(const QModelIndex& idxParent, AZ::Data::AssetId assetID);
            void SendMetricsEvent(AssetBrowserActionType actionType, const QModelIndex& index);

        private Q_SLOTS:
            void OnContextMenu(const QPoint& /*point*/);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework