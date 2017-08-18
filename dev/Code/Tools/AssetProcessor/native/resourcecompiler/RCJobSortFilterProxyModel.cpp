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
#if defined(MODEL_TEST)
#include "modeltest.h"
#endif

#include "RCJobSortFilterProxyModel.h"
#include "native/resourcecompiler/rcjoblistmodel.h" // for Column enum

RCJobSortFilterProxyModel::RCJobSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
#if defined(MODEL_TEST)
    // Test model behaviour from creation in debug
    new ModelTest(this, this);
#endif

    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

bool RCJobSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex jobStateIndex = sourceModel()->index(sourceRow, AssetProcessor::RCJobListModel::Column::ColumnState, sourceParent);
    QString jobState = sourceModel()->data(jobStateIndex).toString();
    if (jobState.compare(tr("Pending"), Qt::CaseSensitive) != 0)
    {
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    return false;
}

#include <native/resourcecompiler/RCJobSortFilterProxyModel.moc>
