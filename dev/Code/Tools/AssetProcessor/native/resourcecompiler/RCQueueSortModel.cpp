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
#include <native/resourcecompiler/RCQueueSortModel.h>
#include "RCCommon.h"
#include "rcjoblistmodel.h"
#include "rcjob.h"
#include "native/assetprocessor.h"
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AssetProcessor
{
    RCQueueSortModel::RCQueueSortModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        // jobs assigned to "all" platforms are always active.
        m_currentlyConnectedPlatforms.insert(QString("all"));
    }

    void RCQueueSortModel::AttachToModel(RCJobListModel* target)
    {
        if (target)
        {
            setDynamicSortFilter(true);
            BusConnect();
            m_sourceModel = target;

            setSourceModel(target);
            setSortRole(RCJobListModel::jobIndexRole);
            sort(0);
        }
        else
        {
            BusDisconnect();
            setSourceModel(nullptr);
            m_sourceModel = nullptr;
        }
    }

    RCJob* RCQueueSortModel::GetNextPendingJob()
    {
        if (m_dirtyNeedsResort)
        {
            setDynamicSortFilter(false);
            QSortFilterProxyModel::sort(0);
            setDynamicSortFilter(true);
            m_dirtyNeedsResort = false;
        }
        for (int idx = 0; idx < rowCount(); ++idx)
        {
            QModelIndex parentIndex = mapToSource(index(idx, 0));
            RCJob* actualJob = m_sourceModel->getItem(parentIndex.row());
            if ((actualJob) && (actualJob->GetState() == RCJob::pending))
            {
                return actualJob;
            }
        }
        // there are no jobs to do.
        return nullptr;
    }

    bool RCQueueSortModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
    {
        (void)source_parent;
        RCJob* actualJob = m_sourceModel->getItem(source_row);
        if (!actualJob)
        {
            return false;
        }

        if (actualJob->GetState() != RCJob::pending)
        {
            return false;
        }

        return true;
    }

    bool RCQueueSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        RCJob* leftJob = m_sourceModel->getItem(left.row());
        RCJob* rightJob = m_sourceModel->getItem(right.row());

        // auto fail jobs always take priority to give user feedback asap.
        bool autoFailLeft = leftJob->IsAutoFail();
        bool autoFailRight = rightJob->IsAutoFail();
        if (autoFailLeft)
        {
            if (!autoFailRight)
            {
                return true; // left before right
            }
        }
        else if (autoFailRight)
        {
            return false; // right before left.
        }

        // first thing to check is in platform.
        bool leftActive = m_currentlyConnectedPlatforms.contains(leftJob->GetPlatformInfo().m_identifier.c_str());
        bool rightActive = m_currentlyConnectedPlatforms.contains(rightJob->GetPlatformInfo().m_identifier.c_str());

        if (leftActive)
        {
            if (!rightActive)
            {
                return true; // left before right
            }
        }
        else if (rightActive)
        {
            return false; // right before left.
        }

        // critical jobs take priority
        if (leftJob->IsCritical())
        {
            if (!rightJob->IsCritical())
            {
                return true; // left wins.
            }
        }
        else if (rightJob->IsCritical())
        {
            return false; // right wins
        }

        int leftJobEscalation = leftJob->JobEscalation();
        int rightJobEscalation = rightJob->JobEscalation();

        // This function, even though its called lessThan(), really is asking, does LEFT come before RIGHT
        // The higher the escalation, the more important the request, and thus the sooner we want to process the job
        // Which means if left has a higher escalation number than right, its LESS THAN right.
        if (leftJobEscalation != rightJobEscalation)
        {
            return leftJobEscalation > rightJobEscalation;
        }

        // arbitrarily, lets have PC get done first since pc-format assets are what the editor uses.
        if (leftJob->GetPlatformInfo().m_identifier != rightJob->GetPlatformInfo().m_identifier)
        {
            if (leftJob->GetPlatformInfo().m_identifier == AzToolsFramework::AssetSystem::GetHostAssetPlatform())
            {
                return true; // left wins.
            }
            if (rightJob->GetPlatformInfo().m_identifier == AzToolsFramework::AssetSystem::GetHostAssetPlatform())
            {
                return false; // right wins
            }
        }

        int priorityLeft = leftJob->GetPriority();
        int priorityRight = rightJob->GetPriority();

        if (priorityLeft != priorityRight)
        {
            return priorityLeft > priorityRight;
        }
        
        // if we get all the way down here it means we're dealing with two assets which are not
        // in any compile groups, not a priority platform, not a priority type, priority platform, etc.
        // we can arrange these any way we want, but must pick at least a stable order.
        return leftJob->GetJobEntry().m_jobRunKey < rightJob->GetJobEntry().m_jobRunKey;
    }

    void RCQueueSortModel::AssetProcessorPlatformConnected(const AZStd::string platform)
    {
        QMetaObject::invokeMethod(this, "ProcessPlatformChangeMessage", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(platform.c_str())), Q_ARG(bool, true));
    }

    void RCQueueSortModel::AssetProcessorPlatformDisconnected(const AZStd::string platform)
    {
        QMetaObject::invokeMethod(this, "ProcessPlatformChangeMessage", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(platform.c_str())), Q_ARG(bool, false));
    }

    void RCQueueSortModel::ProcessPlatformChangeMessage(QString platformName, bool connected)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "RCQueueSortModel: Platform %s has %s.", platformName.toUtf8().data(), connected ? "connected" : "disconnected");
        m_dirtyNeedsResort = true;
        if (connected)
        {
            m_currentlyConnectedPlatforms.insert(platformName);
        }
        else
        {
            m_currentlyConnectedPlatforms.remove(platformName);
        }
    }
    void RCQueueSortModel::AddJobIdEntry(AssetProcessor::RCJob* rcJob)
    {
        m_currentJobRunKeyToJobEntries[rcJob->GetJobEntry().m_jobRunKey] = rcJob;
    }

    void RCQueueSortModel::RemoveJobIdEntry(AssetProcessor::RCJob* rcJob)
    {
        m_currentJobRunKeyToJobEntries.erase(rcJob->GetJobEntry().m_jobRunKey);
    }

    void RCQueueSortModel::OnEscalateJobs(AssetProcessor::JobIdEscalationList jobIdEscalationList)
    {
        for (const auto& jobIdEscalationPair : jobIdEscalationList)
        {
            auto found = m_currentJobRunKeyToJobEntries.find(jobIdEscalationPair.first);

            if (found != m_currentJobRunKeyToJobEntries.end())
            {
                m_sourceModel->UpdateJobEscalation(found->second, jobIdEscalationPair.second);
            }
        }
    }

} // end namespace AssetProcessor

#include <native/resourcecompiler/RCQueueSortModel.moc>