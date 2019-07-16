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

#include <AzQtComponents/Components/FilteredSearchWidget.h>

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
        using namespace AzToolsFramework::AssetSystem;

        const QModelIndex jobStateIndex = sourceModel()->index(sourceRow,
            AssetProcessor::JobsModel::Column::ColumnStatus, sourceParent);

        JobStatus jobStatus = sourceModel()->data(jobStateIndex, AssetProcessor::JobsModel::statusRole)
            .value<JobStatus>();
        if (jobStatus == JobStatus::Failed_InvalidSourceNameExceedsMaxLimit)
            jobStatus = JobStatus::Failed;

        if (!m_activeTypeFilters.isEmpty() && !m_activeTypeFilters.contains(jobStatus))
        {
            return false;
        }

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    void JobSortFilterProxyModel::OnJobStatusFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
    {
        m_activeTypeFilters.clear();
        for (const auto filter : activeTypeFilters)
        {
            m_activeTypeFilters << filter.metadata.value<AzToolsFramework::AssetSystem::JobStatus>();
        }

        invalidateFilter();
    }
} //namespace AssetProcessor

#include <native/resourcecompiler/RCJobSortFilterProxyModel.moc>
