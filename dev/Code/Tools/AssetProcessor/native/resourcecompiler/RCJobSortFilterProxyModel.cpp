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

#include <native/resourcecompiler/RCJobSortFilterProxyModel.h>
#include <native/resourcecompiler/JobsModel.h> //for jobsModel column enum

namespace AssetProcessor
{
    JobSortFilterProxyModel::JobSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        setSortCaseSensitivity(Qt::CaseInsensitive);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    bool JobSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex jobStateIndex = sourceModel()->index(sourceRow, AssetProcessor::JobsModel::Column::ColumnStatus, sourceParent);
        QString jobState = sourceModel()->data(jobStateIndex).toString();
        if (m_filterRegexExpEmpty)
        {
            if (jobState.compare(tr("Pending"), Qt::CaseSensitive) != 0 && jobState.compare(tr("Completed"), Qt::CaseSensitive) != 0)
            {
                return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
            }

            return false;
        }

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    void JobSortFilterProxyModel::OnFilterRegexExpChanged(QRegExp regExp, bool isFilterRegexExpEmpty)
    {
        m_filterRegexExpEmpty = isFilterRegexExpEmpty;
        setFilterRegExp(regExp);
    }
} //namespace AssetProcessor

#include <native/resourcecompiler/RCJobSortFilterProxyModel.moc>
