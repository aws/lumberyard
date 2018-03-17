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

#include <AzCore/Script/ScriptTimePoint.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntryCache.h>


#include <QMimeData>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        const int AssetBrowserModel::m_column = static_cast<int>(AssetBrowserEntry::Column::DisplayName);

        AssetBrowserModel::AssetBrowserModel(QObject* parent)
            : QAbstractTableModel(parent)
            , m_rootEntry(nullptr)
            , m_loaded(false)
            , m_addingEntry(false)
            , m_removingEntry(false)
        {
            AssetBrowserModelRequestBus::Handler::BusConnect();
            AZ::TickBus::Handler::BusConnect();
        }

        AssetBrowserModel::~AssetBrowserModel()
        {
            AssetBrowserModelRequestBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
        }

        QModelIndex AssetBrowserModel::index(int row, int column, const QModelIndex& parent) const
        {
            if (!hasIndex(row, column, parent))
            {
                return QModelIndex();
            }

            AssetBrowserEntry* parentEntry;
            if (!parent.isValid())
            {
                parentEntry = m_rootEntry.get();
            }
            else
            {
                parentEntry = reinterpret_cast<AssetBrowserEntry*>(parent.internalPointer());
            }

            AssetBrowserEntry* childEntry = parentEntry->m_children[row];

            if (!childEntry)
            {
                return QModelIndex();
            }

            QModelIndex index;
            GetEntryIndex(childEntry, index);
            return index;
        }

        int AssetBrowserModel::rowCount(const QModelIndex& parent) const
        {
            if (!m_rootEntry)
            {
                return 0;
            }

            if (parent.isValid())
            {
                if ((parent.column() != static_cast<int>(AssetBrowserEntry::Column::DisplayName)) &&
                    (parent.column() != static_cast<int>(AssetBrowserEntry::Column::Name)))
                {
                    return 0;
                }
            }

            AssetBrowserEntry* parentAssetEntry;
            if (!parent.isValid())
            {
                parentAssetEntry = m_rootEntry.get();
            }
            else
            {
                parentAssetEntry = static_cast<AssetBrowserEntry*>(parent.internalPointer());
            }
            return parentAssetEntry->GetChildCount();
        }

        int AssetBrowserModel::columnCount(const QModelIndex& /*parent*/) const
        {
            return static_cast<int>(AssetBrowserEntry::Column::Count);
        }

        QVariant AssetBrowserModel::data(const QModelIndex& index, int role) const
        {
            if (!index.isValid())
            {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
            {
                const AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                return item->GetDisplayName().c_str();
            }

            if (role == Roles::EntryRole)
            {
                const AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                return QVariant::fromValue(item);
            }

            return QVariant();
        }

        Qt::ItemFlags AssetBrowserModel::flags(const QModelIndex& index) const
        {
            Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

            if (index.isValid())
            {
                // allow retrieval of mimedata of sources or products only (i.e. cant drag folders or root)
                AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                if (item && (item->RTTI_IsTypeOf(ProductAssetBrowserEntry::RTTI_Type()) || item->RTTI_IsTypeOf(SourceAssetBrowserEntry::RTTI_Type())))
                {
                    return Qt::ItemIsDragEnabled | defaultFlags;
                }
            }
            return defaultFlags;
            //return Qt::ItemFlags(~Qt::ItemIsDragEnabled & defaultFlags);
        }

        QMimeData* AssetBrowserModel::mimeData(const QModelIndexList& indexes) const
        {
            QMimeData* mimeData = new QMimeData;

            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                    if (item)
                    {
                        item->AddToMimeData(mimeData);
                    }
                }
            }
            return mimeData;
        }

        QVariant AssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (orientation == Qt::Horizontal && role == Roles::EntryRole)
            {
                return tr(AssetBrowserEntry::m_columnNames[section]);
            }

            return QAbstractItemModel::headerData(section, orientation, role);
        }

        void AssetBrowserModel::SourceIndexesToAssetIds(const QModelIndexList& indexes, AZStd::vector<AZ::Data::AssetId>& assetIds)
        {
            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());

                    if (item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                    {
                        assetIds.push_back(static_cast<ProductAssetBrowserEntry*>(item)->GetAssetId());
                    }
                }
            }
        }

        void AssetBrowserModel::SourceIndexesToAssetDatabaseEntries(const QModelIndexList& indexes, AZStd::vector<AssetBrowserEntry*>& entries)
        {
            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                    entries.push_back(item);
                }
            }
        }

        AZStd::shared_ptr<RootAssetBrowserEntry> AssetBrowserModel::GetRootEntry() const
        {
            return m_rootEntry;
        }

        void AssetBrowserModel::SetRootEntry(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry)
        {
            m_rootEntry = rootEntry;
        }

        QModelIndex AssetBrowserModel::parent(const QModelIndex& child) const
        {
            if (!child.isValid())
            {
                return QModelIndex();
            }

            AssetBrowserEntry* childAssetEntry = static_cast<AssetBrowserEntry*>(child.internalPointer());
            AssetBrowserEntry* parentEntry = childAssetEntry->GetParent();

            QModelIndex parentIndex;
            if (GetEntryIndex(parentEntry, parentIndex))
            {
                return parentIndex;
            }
            return QModelIndex();
        }

        bool AssetBrowserModel::IsLoaded() const
        {
            return m_loaded;
        }

        void AssetBrowserModel::BeginAddEntry(AssetBrowserEntry* parent)
        {
            QModelIndex parentIndex;
            if (GetEntryIndex(parent, parentIndex))
            {
                m_addingEntry = true;
                int row = parent->GetChildCount();
                beginInsertRows(parentIndex, row, row);
            }
        }

        void AssetBrowserModel::EndAddEntry(AssetBrowserEntry* parent)
        {
            if (m_addingEntry)
            {
                m_addingEntry = false;
                endInsertRows();

                // we have to also invalidate our parent all the way up the chain.
                // since in this model, the children's data is actually relevant to the filtering of a parent
                // since a parent "matches" the filter if its children do.
                while (parent)
                {
                    QModelIndex parentIndex;
                    if (GetEntryIndex(parent, parentIndex))
                    {
                        Q_EMIT dataChanged(parentIndex, parentIndex);
                    }
                    parent = parent->GetParent();
                }
            }
        }

        void AssetBrowserModel::BeginRemoveEntry(AssetBrowserEntry* entry)
        {
            int row = entry->row();
            QModelIndex parentIndex;
            if (GetEntryIndex(entry->m_parentAssetEntry, parentIndex))
            {
                m_removingEntry = true;
                beginRemoveRows(parentIndex, row, row);
            }
        }

        void AssetBrowserModel::EndRemoveEntry()
        {
            if (m_removingEntry)
            {
                m_removingEntry = false;
                endRemoveRows();
            }
        }

        void AssetBrowserModel::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/) 
        {
            // if any entries changed since last tick, notify the views
            
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                if (!cache->m_dirtyThumbnailsSet.empty())
                {
                    for (AssetBrowserEntry* entry : cache->m_dirtyThumbnailsSet)
                    {
                        QModelIndex index;
                        if (GetEntryIndex(entry, index))
                        {
                            Q_EMIT dataChanged(index, index, { Roles::EntryRole });
                        }
                    }
                    cache->m_dirtyThumbnailsSet.clear();
                }
            }
        }

        bool AssetBrowserModel::GetEntryIndex(AssetBrowserEntry* entry, QModelIndex& index) const
        {
            if (!entry)
            {
                return false;
            }

            if (azrtti_istypeof<RootAssetBrowserEntry*>(entry))
            {
                index = QModelIndex();
                return true;
            }

            if (!entry->m_parentAssetEntry)
            {
                return false;
            }

            int row = entry->row();
            index = createIndex(row, m_column, entry);
            return true;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include <AssetBrowser/AssetBrowserModel.moc>
