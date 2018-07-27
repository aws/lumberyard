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
#ifndef RCQUEUEMODEL_H
#define RCQUEUEMODEL_H

#include "RCCommon.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QQueue>
#include <QMultiMap>

#include "rcjob.h"

namespace AssetProcessor
{
    class QueueElementID;
}


namespace AssetProcessor
{
    /**
     * The RCJobListModel class contains lists of RC jobs
     */
    class RCJobListModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        enum DataRoles
        {
            jobIndexRole = Qt::UserRole + 1,
            stateRole,
            displayNameRole,
            timeCreatedRole,
            timeLaunchedRole,
            timeCompletedRole,
            jobDataRole,
        };

        enum Column
        {
            ColumnState,
            ColumnJobId,
            ColumnCommand,
            ColumnCompleted,
            ColumnPlatform,
            Max
        };

        explicit RCJobListModel(QObject* parent = 0);


        QModelIndex parent(const QModelIndex&) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role) const;

        void markAsProcessing(RCJob* rcJob);
        void markAsStarted(RCJob* rcJob);
        void markAsCompleted(RCJob* rcJob);
        unsigned int jobsInFlight() const;

        void UpdateJobEscalation(AssetProcessor::RCJob* rcJob, int jobPrioririty);
        void UpdateRow(int jobIndex);

        bool isEmpty();
        void addNewJob(RCJob* rcJob);

        bool isInFlight(const QueueElementID& check) const;
        bool isInQueue(const QueueElementID& check) const;

        void PerformHeuristicSearch(QString searchTerm, QString platform, QSet<QueueElementID>& found, AssetProcessor::JobIdEscalationList& escalationList, bool& isStatusRequest);

        int itemCount() const;
        RCJob* getItem(int index) const;
        int GetIndexOfProcessingJob(const QueueElementID& elementId);

        ///! EraseJobs expects the database name of the source file.  (So with outputprefix)
        void EraseJobs(QString sourceFileDatabaseName, QVector<RCJob*>& pendingJobs);

    private:

        QVector<RCJob*> m_jobs;
        QSet<RCJob*> m_jobsInFlight;

        // profiler showed much of our time was spent in IsInQueue.
        QMultiMap<QueueElementID, RCJob*> m_jobsInQueueLookup;
    };
} // namespace AssetProcessor


#endif // RCQUEUEMODEL_H
