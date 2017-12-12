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

#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ExportJobProcessingHandler.h>
#include <SceneAPI/SceneUI/CommonWidgets/JobWatcher.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <QDateTime>
#include <QRegExp>
#include <time.h> // For card timestamp

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            ExportJobProcessingHandler::ExportJobProcessingHandler(const AZStd::string& sourceAssetPath, QObject* parent)
                : ProcessingHandler(parent)
                , m_sourceAssetPath(sourceAssetPath)
            {
            }

            void ExportJobProcessingHandler::BeginProcessing()
            {
                emit UserMessageUpdated("Waiting for jobs to complete...");
                emit SubtextUpdated("Check Asset Processor for detailed progress");
                emit AddInfo(AZStd::string::format("Processing '%s'", m_sourceAssetPath.c_str()));

                m_jobWatcher.reset(new JobWatcher(m_sourceAssetPath));
                connect(m_jobWatcher.get(), &JobWatcher::JobsForSourceFileFound, this, &ExportJobProcessingHandler::OnJobsForSourceFileFound);
                connect(m_jobWatcher.get(), &JobWatcher::JobProcessingComplete, this, &ExportJobProcessingHandler::OnJobProcessingComplete);
                connect(m_jobWatcher.get(), &JobWatcher::AllJobsComplete, this, &ExportJobProcessingHandler::OnAllJobsComplete);
            }
            
            void ExportJobProcessingHandler::OnJobQueryFailed()
            {
                emit AddError("Failed to retrieve job information from Asset Processor");
                emit ProcessingComplete();
            }

            void ExportJobProcessingHandler::OnJobsForSourceFileFound(const AZStd::vector<AZStd::string>& jobPlatforms)
            {
                if (jobPlatforms.size())
                {
                    AZStd::string result;
                    AzFramework::StringFunc::Join(result, jobPlatforms.begin(), jobPlatforms.end(), ", ");
                    emit AddInfo("Jobs pending for the following platforms: " + result);
                }
                else
                {
                    emit AddAssert("No jobs scheduled for any platform, this may indicate an error in the asset processor, or else a zero-net change.");
                    emit ProcessingComplete();
                }
            }

            void ExportJobProcessingHandler::OnJobProcessingComplete(const AZStd::string& platform, AZ::u64 jobId, bool success, const AZStd::string& fullLogText)
            {
                if (success)
                {
                    emit AddInfo(AZStd::string::format("Job #%i for platform '%s' compiled successfully", jobId, platform.c_str()));
                }
                else
                {
                    bool parseResult = AzToolsFramework::Logging::LogEntry::ParseLog(fullLogText.c_str(), aznumeric_cast<AZ::u64>(fullLogText.length()),
                        [this](const AzToolsFramework::Logging::LogEntry& entry)
                        {
                            if (entry.GetSeverity() != AzToolsFramework::Logging::LogEntry::Severity::Message)
                            {
                                emit AddLogEntry(entry);
                            }
                        });

                    if (!parseResult)
                    {
                        emit AddError("Failed to parse log. See Asset Processor for more info.");
                    }
                    emit AddError(AZStd::string::format("Job #%i for platform '%s' failed", jobId, platform.c_str()));
                }
            }
            
            void ExportJobProcessingHandler::OnAllJobsComplete()
            {
                emit UserMessageUpdated("All jobs complete");
                emit SubtextUpdated("");
                emit ProcessingComplete();
            }
        }
    }
}

#include <Handlers/ProcessingHandlers/ExportJobProcessingHandler.moc>