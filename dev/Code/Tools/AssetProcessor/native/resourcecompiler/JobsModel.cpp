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

#include <native/resourcecompiler/JobsModel.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/assetUtils.h>
#include <native/AssetDatabase/AssetDatabase.h>

namespace AssetProcessor
{
    JobsModel::JobsModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    JobsModel::~JobsModel()
    {
        for (int idx = 0; idx < m_cachedJobs.size(); idx++)
        {
            delete m_cachedJobs[idx];
        }

        m_cachedJobs.clear();
        m_cachedJobsLookup.clear();
    }

    QModelIndex JobsModel::parent(const QModelIndex& index) const
    {
        AZ_UNUSED(index);
        return QModelIndex();
    }

    QModelIndex JobsModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row >= rowCount(parent) || column >= columnCount(parent))
        {
            return QModelIndex();
        }
        return createIndex(row, column);
    }

    int JobsModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : itemCount();
    }

    int JobsModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : Column::Max;
    }

    QVariant JobsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case ColumnStatus:
                return tr("Status");
            case ColumnSource:
                return tr("Source");
            case ColumnPlatform:
                return tr("Platform");
            case ColumnJobKey:
                return tr("Job Key");
            case ColumnCompleted:
                return tr("Completed");
            default:
                break;
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    int JobsModel::itemCount() const
    {
        return m_cachedJobs.size();
    }


    QVariant JobsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (index.row() >= itemCount())
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::DisplayRole:
            switch (index.column())
            {
            case ColumnStatus:
                return GetStatusInString(getItem(index.row())->m_jobState);
            case ColumnSource:
                return getItem(index.row())->m_elementId.GetInputAssetName();
            case ColumnPlatform:
                return getItem(index.row())->m_elementId.GetPlatform();
            case ColumnJobKey:
                return getItem(index.row())->m_elementId.GetJobDescriptor();
            case ColumnCompleted:
                return getItem(index.row())->m_completedTime.toString("hh:mm:ss.zzz");
            default:
                break;
            }
        case logRole:
        {
            AzToolsFramework::AssetSystem::JobInfo jobInfo;
            AzToolsFramework::AssetSystem::AssetJobLogResponse jobLogResponse;
            jobInfo.m_sourceFile = getItem(index.row())->m_elementId.GetInputAssetName().toUtf8().data();
            jobInfo.m_platform = getItem(index.row())->m_elementId.GetPlatform().toUtf8().data();
            jobInfo.m_jobKey = getItem(index.row())->m_elementId.GetJobDescriptor().toUtf8().data();
            jobInfo.m_builderGuid = getItem(index.row())->m_builderGuid;
            jobInfo.m_jobRunKey = getItem(index.row())->m_jobRunKey;
            AssetUtilities::ReadJobLog(jobInfo, jobLogResponse);
            return QVariant(jobLogResponse.m_jobLog.c_str());
        }
        case Qt::TextAlignmentRole:
            switch (index.column())
            {
            case ColumnStatus:  // fall through intentional
            case ColumnPlatform:  // fall through intentional
            case ColumnJobKey:  // fall through intentional
            case ColumnCompleted:
                return Qt::AlignHCenter;
            default:
                break;
            }

        default:
            break;
        }

        return QVariant();
    }

    CachedJobInfo* JobsModel::getItem(int index) const
    {
        if (index >= 0 && index < m_cachedJobs.size())
        {
            return m_cachedJobs[index];
        }

        return nullptr; //invalid index
    }

    QString JobsModel::GetStatusInString(const AzToolsFramework::AssetSystem::JobStatus& state) const
    {
        using namespace AzToolsFramework::AssetSystem;

        switch (state)
        {
        case JobStatus::Queued:
            return tr("Pending");
        case JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:  // fall through intentional
        case JobStatus::Failed:
            return tr("Failed");
        case JobStatus::Completed:
            return tr("Completed");
        case JobStatus::InProgress:
            return tr("InProgress");
        }
        return QString();
    }

    void JobsModel::PopulateJobsFromDatabase()
    {
        beginResetModel();
        AZStd::string databaseLocation;
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

        if (!databaseLocation.empty())
        {
            AssetProcessor::AssetDatabaseConnection assetDatabaseConnection;
            assetDatabaseConnection.OpenDatabase();

            auto jobsFunction = [this, &assetDatabaseConnection](AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry)
            {
                AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
                assetDatabaseConnection.GetSourceBySourceID(entry.m_sourcePK, source);
                CachedJobInfo* jobInfo = new CachedJobInfo();
                jobInfo->m_elementId.SetInputAssetName(source.m_sourceName.c_str());
                jobInfo->m_elementId.SetPlatform(entry.m_platform.c_str());
                jobInfo->m_elementId.SetJobDescriptor(entry.m_jobKey.c_str());
                jobInfo->m_jobState = entry.m_status;
                jobInfo->m_jobRunKey = entry.m_jobRunKey;
                jobInfo->m_builderGuid = entry.m_builderGuid;
                jobInfo->m_completedTime = QDateTime::fromMSecsSinceEpoch(entry.m_lastLogTime);
                m_cachedJobs.push_back(jobInfo);
                m_cachedJobsLookup.insert(jobInfo->m_elementId, m_cachedJobs.size() - 1);
                return true;
            };
            assetDatabaseConnection.QueryJobsTable(jobsFunction);
        }

        endResetModel();
    }

    void JobsModel::OnJobStatusChanged(JobEntry entry, AzToolsFramework::AssetSystem::JobStatus status)
    {
        QueueElementID elementId(entry.m_relativePathToFile, entry.m_platformInfo.m_identifier.c_str(), entry.m_jobKey);
        CachedJobInfo* jobInfo = nullptr;
        unsigned int jobIndex = 0;
        auto iter = m_cachedJobsLookup.find(elementId);
        if (iter == m_cachedJobsLookup.end())
        {
            jobInfo = new CachedJobInfo();
            jobInfo->m_elementId.SetInputAssetName(entry.m_relativePathToFile.toUtf8().data());
            jobInfo->m_elementId.SetPlatform(entry.m_platformInfo.m_identifier.c_str());
            jobInfo->m_elementId.SetJobDescriptor(entry.m_jobKey.toUtf8().data());
            jobInfo->m_jobRunKey = entry.m_jobRunKey;
            jobInfo->m_builderGuid = entry.m_builderGuid;
            jobInfo->m_jobState = status;
            jobIndex = m_cachedJobs.size();
            beginInsertRows(QModelIndex(), jobIndex, jobIndex);
            m_cachedJobs.push_back(jobInfo);
            m_cachedJobsLookup.insert(jobInfo->m_elementId, jobIndex);
            endInsertRows();
        }
        else
        {
            jobIndex = iter.value();
            jobInfo = m_cachedJobs[jobIndex];
            jobInfo->m_jobState = status;
            jobInfo->m_jobRunKey = entry.m_jobRunKey;
            jobInfo->m_builderGuid = entry.m_builderGuid;
            if (jobInfo->m_jobState == AzToolsFramework::AssetSystem::JobStatus::Completed || jobInfo->m_jobState == AzToolsFramework::AssetSystem::JobStatus::Failed)
            {
                jobInfo->m_completedTime = QDateTime::currentDateTime();
            }
            else
            {
                jobInfo->m_completedTime = QDateTime();
            }
            Q_EMIT dataChanged(index(jobIndex, 0, QModelIndex()), index(jobIndex, columnCount() - 1, QModelIndex()));
        }
    }

    void JobsModel::OnSourceRemoved(QString sourceRelPath)
    {
        // when a source is removed, we need to eliminate all job entries for that source regardless of all other details of it.
        QList<AssetProcessor::QueueElementID> elementsToRemove;
        for (int index = 0; index < m_cachedJobs.count(); ++index)
        {
            if (QString::compare(m_cachedJobs[index]->m_elementId.GetInputAssetName(), sourceRelPath, Qt::CaseSensitive) == 0)
            {
                elementsToRemove.push_back(m_cachedJobs[index]->m_elementId);
            }
        }

        // now that we've collected all the elements to remove, we can remove them.  
        // Doing it this way avoids problems with mutating these cache structures while iterating them.
        for (const AssetProcessor::QueueElementID& removal : elementsToRemove)
        {
            RemoveJob(removal);
        }
    }

    void JobsModel::OnJobRemoved(AzToolsFramework::AssetSystem::JobInfo jobInfo)
    {
        RemoveJob(QueueElementID(jobInfo.m_sourceFile.c_str(), jobInfo.m_platform.c_str(), jobInfo.m_jobKey.c_str()));
    }

    void JobsModel::RemoveJob(const AssetProcessor::QueueElementID& elementId)
    {
        auto iter = m_cachedJobsLookup.find(elementId);
        if (iter != m_cachedJobsLookup.end())
        {
            unsigned int jobIndex = iter.value();
            CachedJobInfo* jobInfo = m_cachedJobs[jobIndex];
            beginRemoveRows(QModelIndex(), jobIndex, jobIndex);
            m_cachedJobs.remove(jobIndex);
            delete jobInfo;
            m_cachedJobsLookup.erase(iter);
            // Since we are storing the jobIndex for each job for faster lookup therefore 
            // we need to update the jobIndex for jobs that were after the removed job. 
            for (int idx = jobIndex; idx < m_cachedJobs.size(); idx++)
            {
                CachedJobInfo* jobInfo = m_cachedJobs[idx];
                auto iterator = m_cachedJobsLookup.find(jobInfo->m_elementId);
                if (iterator != m_cachedJobsLookup.end())
                {
                    unsigned int index = iterator.value();
                    m_cachedJobsLookup[jobInfo->m_elementId] = --index;
                }
                else
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "OnJobRemoved: Job ( Source: %s, Platform: %s, JobKey: %s ) not found.\n ", 
                        elementId.GetInputAssetName().toUtf8().data(), elementId.GetPlatform().toUtf8().data(), elementId.GetJobDescriptor().toUtf8().data());
                }
            }
            endRemoveRows();
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "OnJobRemoved: Job ( Source: %s, Platform: %s, JobKey: %s ) not found.\n ", 
                elementId.GetInputAssetName().toUtf8().data(), elementId.GetPlatform().toUtf8().data(), elementId.GetJobDescriptor().toUtf8().data());
        }
    }
} //namespace AssetProcessor

#include <native/resourcecompiler/JobsModel.moc>