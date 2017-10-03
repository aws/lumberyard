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

#include <QRegExp>
#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ExportJobProcessingHandler.h>
#include <SceneAPI/SceneUI/CommonWidgets/JobWatcher.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            ExportJobProcessingHandler::ExportJobProcessingHandler(Uuid traceTag, const AZStd::string& sourceAssetPath, QObject* parent)
                : ProcessingHandler(traceTag, parent)
                , m_sourceAssetPath(sourceAssetPath)
            {
            }

            void ExportJobProcessingHandler::BeginProcessing()
            {
                emit StatusMessageUpdated("File processing...");
                
                m_jobWatcher.reset(new JobWatcher(m_sourceAssetPath, m_traceTag));
                connect(m_jobWatcher.get(), &JobWatcher::JobsForSourceFileFound, this, &ExportJobProcessingHandler::OnJobsForSourceFileFound);
                connect(m_jobWatcher.get(), &JobWatcher::JobProcessingComplete, this, &ExportJobProcessingHandler::OnJobProcessingComplete);
                connect(m_jobWatcher.get(), &JobWatcher::AllJobsComplete, this, &ExportJobProcessingHandler::OnAllJobsComplete);
            }
            
            void ExportJobProcessingHandler::OnJobQueryFailed()
            {
                AZ_TracePrintf(Utilities::ErrorWindow, "Failed to retrieve job information from Asset Processor");
                emit ProcessingComplete();
            }

            void ExportJobProcessingHandler::OnJobsForSourceFileFound(const AZStd::vector<AZStd::string>& jobPlatforms)
            {
                if (!jobPlatforms.size())
                {
                    AZ_TracePrintf(Utilities::WarningWindow, 
                        "No jobs scheduled for any platform, this may indicate an error in the asset processor, or else a zero-net change.");
                    emit ProcessingComplete();
                }
            }

            void ExportJobProcessingHandler::OnJobProcessingComplete(const AZStd::string& platform, AZ::u64 jobId, bool success, const AZStd::string& fullLogText)
            {
                bool parseResult = AzToolsFramework::Logging::LogEntry::ParseLog(fullLogText.c_str(), aznumeric_cast<AZ::u64>(fullLogText.length()),
                    [this](const AzToolsFramework::Logging::LogEntry& entry)
                    {
                        emit AddLogEntry(entry);
                    });
                
                AZ_TraceContext("Platform", platform);
                if (!parseResult)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Failed to parse log. See Asset Processor for more info.");
                }

                if (success)
                {
                    AZ_TracePrintf(Utilities::SuccessWindow, "Job #%i compiled successfully", jobId);
                }
                else
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Job #%i failed", jobId);
                }
            }
            
            void ExportJobProcessingHandler::OnAllJobsComplete()
            {
                emit StatusMessageUpdated("All jobs completed.");
                emit ProcessingComplete();
            }
        }
    }
}

#include <Handlers/ProcessingHandlers/ExportJobProcessingHandler.moc>