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

#include <AzToolsFramework/AssetBrowser/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/Thumbnails/ThumbnailDelegate.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <QMenu>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTreeView::AssetBrowserTreeView(QWidget* parent)
            : QTreeViewWithStateSaving(parent)
            , m_assetBrowserModel(nullptr)
            , m_assetBrowserSortFilterProxyModel(nullptr)
            , m_delegate(new Thumbnailer::ThumbnailDelegate)
        {
            setSortingEnabled(true);
            setItemDelegate(m_delegate.data());

            setContextMenuPolicy(Qt::CustomContextMenu);

            connect(this, &QTreeView::customContextMenuRequested, this, &AssetBrowserTreeView::OnContextMenu);
        }

        AssetBrowserTreeView::~AssetBrowserTreeView()
        {
        }

        void AssetBrowserTreeView::LoadState(const QString& name)
        {
            Q_ASSERT(model());

            auto crc = AZ::Crc32(name.toUtf8().data());
            InitializeTreeViewSaving(crc);
            SetTreeViewExpandedFunction(AZStd::bind(&AssetBrowserTreeView::ExpandSourceFiles, this, AZStd::placeholders::_1));
            ApplyTreeViewSnapshot();
        }

        //! Ideally I would capture state on destructor, however
        //! Qt sets the model to NULL before destructor is called
        void AssetBrowserTreeView::SaveState() const
        {
            CaptureTreeViewSnapshot();
        }

        AZStd::vector<AssetBrowserEntry*> AssetBrowserTreeView::GetSelectedAssets() const
        {
            QModelIndexList sourceIndexes;
            for (const auto& index : selectedIndexes())
            {
                sourceIndexes.push_back(m_assetBrowserSortFilterProxyModel->mapToSource(index));
            }

            AZStd::vector<AssetBrowserEntry*> entries;
            m_assetBrowserModel->SourceIndexesToAssetDatabaseEntries(sourceIndexes, entries);
            return entries;
        }

        void AssetBrowserTreeView::SelectProduct(AZ::Data::AssetId assetID)
        {
            if (!assetID.IsValid())
            {
                return;
            }
            SelectProduct(QModelIndex(), assetID);
        }

        void AssetBrowserTreeView::SetThumbnailContext(const char* thumbnailContext) const
        {
            m_delegate->SetThumbnailContext(thumbnailContext);
        }

        bool AssetBrowserTreeView::SelectProduct(const QModelIndex& idxParent, AZ::Data::AssetId assetID)
        {
            int elements = model()->rowCount(idxParent);
            for (int idx = 0; idx < elements; ++idx)
            {
                auto rowIdx = model()->index(idx, 0, idxParent);
                auto modelIdx = m_assetBrowserSortFilterProxyModel->mapToSource(rowIdx);
                auto assetEntry = static_cast<AssetBrowserEntry*>(modelIdx.internalPointer());
                auto productEntry = azrtti_cast<ProductAssetBrowserEntry*>(assetEntry);
                if (productEntry && productEntry->GetAssetId() == assetID)
                {
                    selectionModel()->clear();
                    selectionModel()->select(rowIdx, QItemSelectionModel::Select);
                    setCurrentIndex(rowIdx);
                    return true;
                }
                if (SelectProduct(rowIdx, assetID))
                {
                    expand(rowIdx);
                    return true;
                }
            }
            return false;
        }

        bool AssetBrowserTreeView::ExpandSourceFiles(const QModelIndex& idx)
        {
            if (!idx.isValid())
            {
                return false;
            }
            auto modelIdx = m_assetBrowserSortFilterProxyModel->mapToSource(idx);
            AssetBrowserEntry* entry = reinterpret_cast<AssetBrowserEntry*>(modelIdx.internalPointer());
            return (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source);
        }

        void AssetBrowserTreeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            QTreeView::selectionChanged(selected, deselected);
            Q_EMIT selectionChangedSignal(selected, deselected);
        }

        void AssetBrowserTreeView::startDrag(Qt::DropActions supportedActions)
        {
            QModelIndexList indexes = selectedIndexes();
            if (indexes.count() > 0)
            {
                QMimeData* mimeData = model()->mimeData(indexes);
                EBUS_EVENT(AssetBrowserInteractionNotificationsBus, StartDrag, mimeData);
            }
            QAbstractItemView::startDrag(supportedActions);
        }

        void AssetBrowserTreeView::setModel(QAbstractItemModel* model)
        {
            m_assetBrowserSortFilterProxyModel = static_cast<AssetBrowserFilterModel*>(model);
            m_assetBrowserModel = static_cast<AssetBrowserModel*>(m_assetBrowserSortFilterProxyModel->sourceModel());
            QTreeViewWithStateSaving::setModel(model);
        }

        void AssetBrowserTreeView::OnContextMenu(const QPoint&)
        {
            auto selectedAssets = GetSelectedAssets();
            if (selectedAssets.size() != 1)
            {
                return;
            }

            QMenu menu(this);
            AssetBrowserInteractionNotificationsBus::Broadcast(
                &AssetBrowserInteractionNotificationsBus::Events::AddContextMenuActions, this, &menu, selectedAssets);
            if (!menu.isEmpty())
            {
                menu.exec(QCursor::pos());
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include <AssetBrowser/AssetBrowserTreeView.moc>