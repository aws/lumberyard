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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h> // for AssetSystemJobRequestBus
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneUI/CommonWidgets/JobWatcher.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            const int JobWatcher::s_jobQueryInterval = 250; // ms

            JobWatcher::JobWatcher(const AZStd::string& sourceAssetFullPath, Uuid traceTag)
                : m_jobQueryTimer(new QTimer(this))
                , m_sourceAssetFullPath(sourceAssetFullPath)
                , m_hasReportedAvailableJobs(false)
                , m_traceTag(traceTag)
            {
                AZ_TraceContext("Tag", m_traceTag);
                connect(m_jobQueryTimer, &QTimer::timeout, this, &JobWatcher::OnQueryJobs);
                m_jobQueryTimer->start(s_jobQueryInterval);
            }

            bool IsJobFinishedProcessing(const AzToolsFramework::AssetSystem::JobInfo& job)
            {
                return job.m_status == AzToolsFramework::AssetSystem::JobStatus::Completed
                    || job.m_status == AzToolsFramework::AssetSystem::JobStatus::Failed
                    || job.m_status == AzToolsFramework::AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit;
            }

            void JobWatcher::OnQueryJobs()
            {
                AZ_TraceContext("Tag", m_traceTag);
                
                // Query for the relevant jobs
                Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> result = Failure();
                EBUS_EVENT_RESULT(result, AzToolsFramework::AssetSystemJobRequestBus, GetAssetJobsInfo, m_sourceAssetFullPath, true);
 
                if (!result.IsSuccess())
                {
                    m_jobQueryTimer->stop();
                    emit JobQueryFailed();
                    return;
                }

                AzToolsFramework::AssetSystem::JobInfoContainer& allJobs = result.GetValue();

                // Signal what jobs are pending (if we haven't yet)
                if (!m_hasReportedAvailableJobs)
                {
                    AZStd::vector<AZStd::string> jobPlatforms;
                    for (const AzToolsFramework::AssetSystem::JobInfo& info : allJobs)
                    {
                        // Account for potential of duplicates
                        if (AZStd::find(jobPlatforms.begin(), jobPlatforms.end(), info.m_platform) == jobPlatforms.end())
                        {
                            jobPlatforms.push_back(info.m_platform);
                        }
                    }

                    emit JobsForSourceFileFound(jobPlatforms);
                    m_hasReportedAvailableJobs = true;
                }

                // If all jobs are finished processing, then notify the user what the final status was
                //  and give a full log of everything
                if (AZStd::all_of(allJobs.begin(), allJobs.end(), IsJobFinishedProcessing))
                {
                    for (const AzToolsFramework::AssetSystem::JobInfo& job : allJobs)
                    {
                        bool wasSuccessful = job.m_status == AzToolsFramework::AssetSystem::JobStatus::Completed;

                        Outcome<AZStd::string> logFetchResult = Failure();
                        EBUS_EVENT_RESULT(logFetchResult, AzToolsFramework::AssetSystemJobRequestBus, GetJobLog, job.m_jobRunKey);

                        emit JobProcessingComplete(
                            job.m_platform, 
                            job.m_jobRunKey, 
                            wasSuccessful, 
                            logFetchResult.IsSuccess() ? logFetchResult.GetValue() : "");
                    }

                    m_jobQueryTimer->stop();
                    emit AllJobsComplete();
                }
            }
        } // SceneUI
    } // SceneAPI
} // AZ

#include <CommonWidgets/JobWatcher.moc>