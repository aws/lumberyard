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
#include <AzCore/Math/Crc.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>

#include <QMenu>
#include <QHeaderView>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QPen>
#include <QPainter>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTreeView::AssetBrowserTreeView(QWidget* parent)
            : QTreeViewWithStateSaving(parent)
            , m_assetBrowserModel(nullptr)
            , m_assetBrowserSortFilterProxyModel(nullptr)
            , m_delegate(new EntryDelegate(this))
        {
            setSortingEnabled(true);
            setItemDelegate(m_delegate.data());
            header()->hide();
            setContextMenuPolicy(Qt::CustomContextMenu);

            setMouseTracking(true);

            connect(this, &QTreeView::customContextMenuRequested, this, &AssetBrowserTreeView::OnContextMenu);
            connect(this, &QTreeView::expanded, [&](const QModelIndex& index)
                {
                    if (!m_sendMetrics || !index.isValid())
                    {
                        return;
                    }
                    auto data = index.data(AssetBrowserModel::Roles::EntryRole);
                    if (data.canConvert<const AssetBrowserEntry*>())
                    {
                        auto entry = qvariant_cast<const AssetBrowserEntry*>(data);
                        auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                        if (source)
                        {
                            EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::AssetBrowserAction,
                                AssetBrowserActionType::SourceExpanded, source->GetSourceUuid(),  source->GetExtension().c_str(), source->GetChildCount());
                        }
                    }
                });
            connect(this, &QTreeView::collapsed, [&](const QModelIndex& index)
                {
                    if (!m_sendMetrics || !index.isValid())
                    {
                        return;
                    }
                    auto data = index.data(AssetBrowserModel::Roles::EntryRole);
                    if (data.canConvert<const AssetBrowserEntry*>())
                    {
                        auto entry = qvariant_cast<const AssetBrowserEntry*>(data);
                        auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                        if (source)
                        {
                            EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::AssetBrowserAction,
                                AssetBrowserActionType::SourceCollapsed, source->GetSourceUuid(), source->GetExtension().c_str(), source->GetChildCount());
                        }
                    }
                });
        }

        AssetBrowserTreeView::~AssetBrowserTreeView() = default;

        void AssetBrowserTreeView::LoadState(const QString& name)
        {
            Q_ASSERT(model());

            m_sendMetrics = false; // do not send metrics events when loading tree state
            auto crc = AZ::Crc32(name.toUtf8().data());
            InitializeTreeViewSaving(crc);
            ApplyTreeViewSnapshot();
            m_sendMetrics = true;
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

        void AssetBrowserTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
        {
            QTreeView::drawBranches(painter, rect, index);

            auto data = index.data(AssetBrowserModel::Roles::EntryRole);
            if (data.canConvert<const AssetBrowserEntry*>())
            {
                auto entry = qvariant_cast<const AssetBrowserEntry*>(data);
                if (azrtti_istypeof<const ProductAssetBrowserEntry*>(entry))
                {
                    painter->save();
                    painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);

                    // Model index used to walk up the tree to get information about ancestors
                    // for determining where to draw branch lines and when to draw connecting lines
                    // between parents and uncles.
                    QModelIndex ancestorIndex = index.parent();

                    // Compute the depth of the current entity in the hierarchy to determine where to draw the branch lines
                    int depth = 1;
                    while (ancestorIndex.isValid())
                    {
                        ancestorIndex = ancestorIndex.parent();
                        ++depth;
                    }

                    QColor whiteColor(255, 255, 255);

                    QPen pen;
                    pen.setColor(whiteColor);
                    pen.setWidthF(2.5f);
                    painter->setPen(pen);

                    style()->standardPalette().background();

                    QRect entryRect = visualRect(index);

                    int totalIndent = indentation() * (depth - 1) - (indentation() / 2);

                    int x = totalIndent;
                    int midY = entryRect.y() + (entryRect.height() / 2);

                    // draw horizontal line
                    painter->drawLine(x, midY, x + (indentation()), midY);

                    // number of children the parent has
                    int rowCount = m_assetBrowserSortFilterProxyModel->rowCount(index.parent());
                    // is it last child of the parent
                    bool isLast = (rowCount == (index.row() + 1));

                    int y1 = entryRect.y();
                    // if item is last draw vertical line to the center of the item, else to the whole height of the item to connect with item below
                    int y2 = isLast ? (midY) : (entryRect.y() + entryRect.height());
                    // draw vertical line
                    painter->drawLine(x, y1, x, y2);

                    painter->restore();
                }
            }
        }

        void AssetBrowserTreeView::startDrag(Qt::DropActions supportedActions)
        {
            if (m_sendMetrics && currentIndex().isValid())
            {
                auto data = currentIndex().data(AssetBrowserModel::Roles::EntryRole);
                if (data.canConvert<const AssetBrowserEntry*>())
                {
                    auto entry = qvariant_cast<const AssetBrowserEntry*>(data);
                    auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                    if (source)
                    {
                        EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::AssetBrowserAction,
                            AssetBrowserActionType::SourceDragged, source->GetSourceUuid(), source->GetExtension().c_str(), source->GetChildCount());
                    }
                    else
                    {
                        auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
                        if (product)
                        {
                            source = azrtti_cast<const SourceAssetBrowserEntry*>(product->GetParent());
                            EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBusTraits::AssetBrowserAction,
                                AssetBrowserActionType::ProductDragged, source->GetSourceUuid(), source->GetExtension().c_str(), source->GetChildCount());
                        }
                    }
                }
            }

            QTreeViewWithStateSaving::startDrag(supportedActions);
        }

        void AssetBrowserTreeView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
        {
            // if selected entry is being removed, clear selection so not to select (and attempt to preview) other entries potentially marked for deletion
            if (selectionModel() && selectionModel()->selectedIndexes().size() == 1)
            {
                QModelIndex selectedIndex = selectionModel()->selectedIndexes().first();
                QModelIndex parentSelectedIndex = selectedIndex.parent();
                if (parentSelectedIndex == parent && selectedIndex.row() >= start && selectedIndex.row() <= end)
                {
                    selectionModel()->clear();
                }
            }
            QTreeView::rowsAboutToBeRemoved(parent, start, end);
        }

        void AssetBrowserTreeView::SetThumbnailContext(const char* thumbnailContext) const
        {
            m_delegate->SetThumbnailContext(thumbnailContext);
        }

        void AssetBrowserTreeView::SetShowSourceControlIcons(bool showSourceControlsIcons) const
        {
            m_delegate->SetShowSourceControlIcons(showSourceControlsIcons);
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

        void AssetBrowserTreeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            QTreeView::selectionChanged(selected, deselected);
            Q_EMIT selectionChangedSignal(selected, deselected);
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
            AssetBrowserInteractionNotificationBus::Broadcast(
                &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, selectedAssets);
            if (!menu.isEmpty())
            {
                menu.exec(QCursor::pos());
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include <AssetBrowser/Views/AssetBrowserTreeView.moc>
