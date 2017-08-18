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
#include "stdafx.h"

#include "OutlinerSortFilterProxyModel.hxx"

#include <AzCore/Component/componentapplication.h>
#include <AzCore/Component/Entity.h>

#include "OutlinerListModel.hxx"

namespace AzToolsFramework
{
    OutlinerSortFilterProxyModel::OutlinerSortFilterProxyModel(QObject* pParent)
        : QSortFilterProxyModel(pParent)
    {
    }

    void OutlinerSortFilterProxyModel::OnItemExpanded(const QModelIndex& index)
    {
        emit ItemExpanded(mapToSource(index));
    }

    void OutlinerSortFilterProxyModel::OnItemCollapsed(const QModelIndex& index)
    {
        emit ItemCollapsed(mapToSource(index));
    }

    void OutlinerSortFilterProxyModel::OnExpandItem(const QModelIndex& index)
    {
        emit ExpandItem(mapFromSource(index));
    }

    void OutlinerSortFilterProxyModel::UpdateFilter()
    {
        invalidateFilter();
    }

    bool OutlinerSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant visibilityData = sourceModel()->data(index, OutlinerListModel::VisibilityRole);
        return visibilityData.isValid() ? visibilityData.toBool() : true;
    }

    bool OutlinerSortFilterProxyModel::lessThan(const QModelIndex& leftIndex, const QModelIndex& rightIndex) const
    {
        // compare data in these columns until something is lessThan
        const int sortColumns[] = {
            OutlinerListModel::ColumnSliceMembership,
            OutlinerListModel::ColumnName,
            OutlinerListModel::ColumnEntityId
        };

        for (int column : sortColumns)
        {
            // get data from desired column
            QVariant leftData = sourceModel()->data(leftIndex.sibling(leftIndex.row(), column));
            QVariant rightData = sourceModel()->data(rightIndex.sibling(rightIndex.row(), column));

            if (leftData < rightData)
            {
                return true;
            }
            else if (leftData > rightData)
            {
                return false;
            }
            // else they're equal so continue
        }

        // tie, left is not less than right.
        return false;
    }

    void OutlinerSortFilterProxyModel::sort(int /*column*/, Qt::SortOrder /*order*/)
    {
        // override any attempts to change sort
        QSortFilterProxyModel::sort(0, Qt::SortOrder::AscendingOrder);
    }
}

#include <UI/Outliner/OutlinerSortFilterProxyModel.moc>

