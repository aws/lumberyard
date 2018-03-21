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
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <QSharedPointer>
#include <QTimer>
#include <QCollator>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        //AssetBrowserFilterModel
        AssetBrowserFilterModel::AssetBrowserFilterModel(QObject* parent)
            : QSortFilterProxyModel(parent)
        {
            m_showColumn.insert(AssetBrowserModel::m_column);
        }

        AssetBrowserFilterModel::~AssetBrowserFilterModel() = default;

        void AssetBrowserFilterModel::SetFilter(FilterConstType filter)
        {
            connect(filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &AssetBrowserFilterModel::filterUpdatedSlot);
            m_filter = filter;
            invalidateFilter();
        }

        bool AssetBrowserFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            //get the source idx, if invalid early out
            QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
            if (!idx.isValid())
            {
                return false;
            }
            // no filter present, every entry is visible
            if (!m_filter)
            {
                return true;
            }

            //the entry is the internal pointer of the index
            auto entry = static_cast<AssetBrowserEntry*>(idx.internalPointer());

            // root should return true even if its not displayed in the treeview
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Root)
            {
                return true;
            }
            return m_filter->Match(entry);
        }

        bool AssetBrowserFilterModel::filterAcceptsColumn(int source_column, const QModelIndex&) const
        {
            //if the column is in the set we want to show it
            return m_showColumn.find(source_column) != m_showColumn.end();
        }

        bool AssetBrowserFilterModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
        {
            if (source_left.column() == source_right.column())
            {
                QVariant leftData = sourceModel()->data(source_left, AssetBrowserModel::Roles::EntryRole);
                QVariant rightData = sourceModel()->data(source_right, AssetBrowserModel::Roles::EntryRole);
                if (leftData.canConvert<const AssetBrowserEntry*>() && rightData.canConvert<const AssetBrowserEntry*>())
                {
                    auto leftEntry = qvariant_cast<const AssetBrowserEntry*>(leftData);
                    auto rightEntry = qvariant_cast<const AssetBrowserEntry*>(rightData);

                    // folders should always come first
                    if (azrtti_istypeof<const FolderAssetBrowserEntry*>(leftEntry) && azrtti_istypeof<const SourceAssetBrowserEntry*>(rightEntry))
                    {
                        return false;
                    }
                    if (azrtti_istypeof<const SourceAssetBrowserEntry*>(leftEntry) && azrtti_istypeof<const FolderAssetBrowserEntry*>(rightEntry))
                    {
                        return true;
                    }

                    // if both entries are of same type, sort alphabetically
                    QCollator collator;
                    collator.setNumericMode(true);
                    return collator.compare(leftEntry->GetDisplayName().c_str(), rightEntry->GetDisplayName().c_str()) > 0;
                }
            }
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        void AssetBrowserFilterModel::filterUpdatedSlot()
        {
            if (!m_alreadyRecomputingFilters)
            {
                m_alreadyRecomputingFilters = true;
                // de-bounce it, since we may get many filter updates all at once.
                QTimer::singleShot(0, this, [this]()
                {
                    m_alreadyRecomputingFilters = false;
                    auto compFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(m_filter);
                    if (compFilter)
                    {
                        auto& subFilters = compFilter->GetSubFilters();
                        auto it = AZStd::find_if(subFilters.begin(), subFilters.end(), [subFilters](FilterConstType filter) -> bool
                        {
                            auto assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(filter);
                            return !assetTypeFilter.isNull();
                        });
                        if (it != subFilters.end())
                        {
                            m_assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(*it);
                        }
                        it = AZStd::find_if(subFilters.begin(), subFilters.end(), [subFilters](FilterConstType filter) -> bool
                        {
                            auto stringFilter = qobject_cast<QSharedPointer<const StringFilter> >(filter);
                            return !stringFilter.isNull();
                        });
                        if (it != subFilters.end())
                        {
                            m_stringFilter = qobject_cast<QSharedPointer<const StringFilter> >(*it);
                        }
                    }
                    invalidateFilter();
                }
                );
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework// namespace AssetBrowser

#include <AssetBrowser/AssetBrowserFilterModel.moc>
