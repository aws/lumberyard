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

#include <native/resourcecompiler/RCCommon.h>
#include <QVector>
#include <QHash>
#include <QDateTime>
#include <QAbstractItemModel>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <native/assetprocessor.h>

namespace AssetProcessor
{
    //! CachedJobInfo stores all the necessary information needed for showing a particular job including its log
    struct CachedJobInfo
    {
        QueueElementID m_elementId;
        QDateTime m_completedTime;
        AzToolsFramework::AssetSystem::JobStatus m_jobState;
        unsigned int m_jobRunKey;
        AZ::Uuid m_builderGuid;
    };
    /**
    * The JobsModel class contains list of jobs from both the Database and the RCController
    */
    class JobsModel
        : public QAbstractItemModel
    {
        Q_OBJECT
    public:

        enum DataRoles
        {
            logRole = Qt::UserRole + 1,
        };

        enum Column
        {
            ColumnStatus,
            ColumnSource,
            ColumnPlatform,
            ColumnJobKey,
            ColumnCompleted,
            Max
        };

        explicit JobsModel(QObject* parent = nullptr);
        virtual ~JobsModel();
        QModelIndex parent(const QModelIndex& index) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        int itemCount() const;
        CachedJobInfo* getItem(int index) const;
        QString GetStatusInString(const AzToolsFramework::AssetSystem::JobStatus& state) const;
        void PopulateJobsFromDatabase();

public Q_SLOTS:
        void OnJobStatusChanged(JobEntry entry, AzToolsFramework::AssetSystem::JobStatus status);
        void OnJobRemoved(AzToolsFramework::AssetSystem::JobInfo jobInfo);
        void OnSourceRemoved(QString sourceRelPath);

    protected:
        QVector<CachedJobInfo*> m_cachedJobs;
        QHash<AssetProcessor::QueueElementID, int> m_cachedJobsLookup; // QVector uses int as type of index.  

        void RemoveJob(const AssetProcessor::QueueElementID& elementId);
    };

} //namespace AssetProcessor