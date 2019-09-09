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

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_unordered_set.h>

#include <QSortFilterProxyModel>
#include <QSharedPointer>
#include <QCollator>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel
            : public QSortFilterProxyModel
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserFilterModel, AZ::SystemAllocator, 0);
            explicit AssetBrowserFilterModel(QObject* parent = nullptr);
            ~AssetBrowserFilterModel() override;

            //asset type filtering
            void SetFilter(FilterConstType filter);
            void FilterUpdatedSlotImmediate();

        Q_SIGNALS:
            void filterChanged();

            //////////////////////////////////////////////////////////////////////////
            //QSortFilterProxyModel
        protected:
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            bool filterAcceptsColumn(int source_column, const QModelIndex& /*source_parent*/) const override;
            bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
            //////////////////////////////////////////////////////////////////////////

        public Q_SLOTS:
            void filterUpdatedSlot();

        protected:
            //set for filtering columns
            //if the column is in the set the column is not filtered and is shown
            AZStd::fixed_unordered_set<int, 3, static_cast<int>(AssetBrowserEntry::Column::Count)> m_showColumn;
            bool m_alreadyRecomputingFilters = false;
            //asset source name match filter
            FilterConstType m_filter;
            QWeakPointer<const StringFilter> m_stringFilter;
            QWeakPointer<const CompositeFilter> m_assetTypeFilter;
            QCollator m_collator;  // cache the collator as its somewhat expensive to constantly create and destroy one.
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
