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
#include <QStringList>
#include <QFile>
#include <QRunnable>
#include <QCoreApplication>
#include <QFileInfo>
#include <QPair>
#include <QDateTime>
#include <QMutexLocker>
#include <QDateTime>
#include <QElapsedTimer>

#include <AzCore/Serialization/Utils.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/std/algorithm.h>

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetRegistry.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>

#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/utilities/AssetUtils.h"
#include "native/utilities/assetUtilEBusHelper.h"
#include "native/AssetDatabase/AssetDatabase.h"
#include "native/AssetManager/assetScanFolderInfo.h"
#include <AzCore/std/utils.h>
#include "native/utilities/ByteArrayStream.h"
#include "native/resourcecompiler/RCBuilder.h"
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/sort.h>

#include <AzCore/IO/FileIO.h>

namespace AssetProcessor
{
    const AZ::u32 FAILED_FINGERPRINT = 1;
    const int MILLISECONDS_BETWEEN_CREATE_JOBS_STATUS_UPDATE = 1000;

    using namespace AzToolsFramework::AssetSystem;
    using namespace AzFramework::AssetSystem;

    AssetProcessorManager::AssetProcessorManager(AssetProcessor::PlatformConfiguration* config, QObject* parent)
        : QObject(parent)
        , m_platformConfig(config)
    {

        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "AssetProcessor will process assets from gameproject %s.\n", AssetUtilities::ComputeGameName().toUtf8().data());

        m_stateData = AZStd::shared_ptr<AssetDatabaseConnection>(aznew AssetDatabaseConnection());
        m_stateData->OpenDatabase();

        MigrateScanFolders();

        m_highestJobRunKeySoFar = m_stateData->GetHighestJobRunKey() + 1;


        // cache this up front.  Note that it can fail here, and will retry later.
        InitializeCacheRoot();

        m_absoluteDevFolderPath[0] = 0;
        m_absoluteDevGameFolderPath[0] = 0;
        
        QDir assetRoot;
        if (AssetUtilities::ComputeAssetRoot(assetRoot))
        {
            azstrcpy(m_absoluteDevFolderPath, AZ_MAX_PATH_LEN, assetRoot.absolutePath().toUtf8().constData());
            QString absoluteDevGameFolderPath = assetRoot.absoluteFilePath(AssetUtilities::ComputeGameName());
            azstrcpy(m_absoluteDevGameFolderPath, AZ_MAX_PATH_LEN, absoluteDevGameFolderPath.toUtf8().constData());
        }

        AssetProcessor::ProcessingJobInfoBus::Handler::BusConnect();
    }

    AssetProcessorManager::~AssetProcessorManager()
    {
        AssetProcessor::ProcessingJobInfoBus::Handler::BusDisconnect();
    }

    template <class R>
    inline bool AssetProcessorManager::Recv(unsigned int connId, QByteArray payload, R& request)
    {
        bool readFromStream = AZ::Utils::LoadObjectFromBufferInPlace(payload.data(), payload.size(), request);
        AZ_Assert(readFromStream, "AssetProcessorManager::Recv: Could not deserialize from stream (type=%u)", request.GetMessageType());
        return readFromStream;
    }

    bool AssetProcessorManager::InitializeCacheRoot()
    {
        if (AssetUtilities::ComputeProjectCacheRoot(m_cacheRootDir))
        {
            m_normalizedCacheRootPath = AssetUtilities::NormalizeDirectoryPath(m_cacheRootDir.absolutePath());
            return !m_normalizedCacheRootPath.isEmpty();
        }

        return false;
    }

    void AssetProcessorManager::PopulateFilesForFingerprinting(JobDetails& jobDetails)
    {
        QString fullPathToFile(jobDetails.m_jobEntry.m_absolutePathToFile);
        for (int idx = 0; idx < m_platformConfig->MetaDataFileTypesCount(); idx++)
        {
            QPair<QString, QString> metaDataFileType = m_platformConfig->GetMetaDataFileTypeAt(idx);

            if (!metaDataFileType.second.isEmpty() && !fullPathToFile.endsWith(metaDataFileType.second, Qt::CaseInsensitive))
            {
                continue;
            }

            QString metaDataFileName;
            QDir assetRoot;
            AssetUtilities::ComputeAssetRoot(assetRoot);
            QString gameName = AssetUtilities::ComputeGameName();
            QString fullMetaPath = assetRoot.filePath(gameName + "/" + metaDataFileType.first);

            if (QFile::exists(fullMetaPath))
            {
                metaDataFileName = fullMetaPath;
            }
            else
            {
                if (metaDataFileType.second.isEmpty())
                {
                    // ADD the metadata file extension to the end of the filename
                    metaDataFileName = fullPathToFile + "." + metaDataFileType.first;
                }
                else
                {
                    // REPLACE the file's extension with the metadata file extension.
                    QFileInfo fileInfo = QFileInfo(jobDetails.m_jobEntry.m_absolutePathToFile);
                    metaDataFileName = fileInfo.path() + '/' + fileInfo.completeBaseName() + "." + metaDataFileType.first;
                }
            }
            // meta files will always be inserted first in this list
            jobDetails.m_fingerprintFilesList.push_back(metaDataFileName.toUtf8().data());
        }

        // This dependency list is already sorted so the order of files is guaranteed for the same source file  
        for (AssetProcessor::SourceFileDependencyInternal& sourceFileDependency : jobDetails.m_sourceFileDependencyList)
        {
            jobDetails.m_fingerprintFilesList.push_back(sourceFileDependency.m_sourceFileDependency.m_sourceFileDependencyPath);
        }
    }

    void AssetProcessorManager::OnAssetScannerStatusChange(AssetProcessor::AssetScanningStatus status)
    {
        if (status == AssetProcessor::AssetScanningStatus::Started)
        {
            // Ensure that the source file list is populated before
            // we assess any files before a scan
            m_SourceFilesInDatabase.clear();
            auto sourcesFunction = [this](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                {
                    AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolder;
                    if (m_stateData->GetScanFolderByScanFolderID(entry.m_scanFolderPK, scanFolder))
                    {
                        QString encodedRelative = QString::fromUtf8(entry.m_sourceName.c_str()).toLower();
                        QString encodedScanFolder = QString::fromUtf8(scanFolder.m_scanFolder.c_str()).toLower();
                        QString withoutOutputPrefix = encodedRelative;
                        if (!scanFolder.m_outputPrefix.empty())
                        {
                            withoutOutputPrefix = withoutOutputPrefix.remove(0, static_cast<int>(scanFolder.m_outputPrefix.size()) + 1);
                        }

                        QString finalAbsolute = (QString("%1/%2").arg(encodedScanFolder).arg(withoutOutputPrefix)).toLower();
                        m_SourceFilesInDatabase[finalAbsolute] = encodedRelative;
                    }

                    return true;
                };
            m_stateData->QuerySourcesTable(sourcesFunction);

            m_isCurrentlyScanning = true;
            return;
        }
        else if ((status == AssetProcessor::AssetScanningStatus::Completed) ||
                 (status == AssetProcessor::AssetScanningStatus::Stopped))
        {
            m_isCurrentlyScanning = false;
            // we cannot invoke this immediately - the scanner might be done, but we aren't actually ready until we've processed all remaining messages:
            QMetaObject::invokeMethod(this, "CheckMissingFiles", Qt::QueuedConnection);
        }
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // JOB STATUS REQUEST HANDLING
    void AssetProcessorManager::OnJobStatusChanged(JobEntry jobEntry, JobStatus status)
    {
        //this function just adds an removes to a maps to speed up job status, we don't actually write
        //to the database until it either succeeds or fails
        AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(jobEntry.m_relativePathToFile.toUtf8().data());
        AZ::Uuid legacySourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(jobEntry.m_relativePathToFile.toUtf8().data(), false); // legacy source uuid

        if (status == JobStatus::Queued)
        {
            // freshly queued files start out queued.
            JobInfo& jobInfo = m_jobRunKeyToJobInfoMap.insert_key(jobEntry.m_jobRunKey).first->second;
            jobInfo.m_platform = jobEntry.m_platformInfo.m_identifier;
            jobInfo.m_builderGuid = jobEntry.m_builderGuid;
            jobInfo.m_sourceFile = jobEntry.m_relativePathToFile.toUtf8().constData();
            jobInfo.m_watchFolder = jobEntry.m_absolutePathToFile.left(jobEntry.m_absolutePathToFile.length() - (jobEntry.m_relativePathToFile.length() + 1)).toUtf8().constData(); //adding +1 for the separator
            jobInfo.m_jobKey = jobEntry.m_jobKey.toUtf8().constData();
            jobInfo.m_jobRunKey = jobEntry.m_jobRunKey;
            jobInfo.m_status = status;

            m_jobKeyToJobRunKeyMap.insert(AZStd::make_pair(jobEntry.m_jobKey.toUtf8().data(), jobEntry.m_jobRunKey));
            SourceInfo sourceInfo = { jobInfo.m_watchFolder.c_str(), jobEntry.m_relativePathToFile };

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

                m_sourceUUIDToSourceNameMap.insert({ sourceUUID, sourceInfo });

                //adding legacy source uuid as well
                m_sourceUUIDToSourceNameMap.insert({ legacySourceUUID, sourceInfo });
            }

            Q_EMIT SourceQueued(sourceUUID, legacySourceUUID, sourceInfo.m_watchFolder, sourceInfo.m_sourceName);
        }
        else
        {
            if (status == JobStatus::InProgress)
            {
                //update to in progress status
                m_jobRunKeyToJobInfoMap[jobEntry.m_jobRunKey].m_status = JobStatus::InProgress;
            }
            else //if failed or succeeded remove from the map
            {
                m_jobRunKeyToJobInfoMap.erase(jobEntry.m_jobRunKey);

                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);
                    m_sourceUUIDToSourceNameMap.erase(sourceUUID);
                    m_sourceUUIDToSourceNameMap.erase(legacySourceUUID);
                }

                Q_EMIT SourceFinished(sourceUUID, legacySourceUUID);

                auto found = m_jobKeyToJobRunKeyMap.equal_range(jobEntry.m_jobKey.toUtf8().data());

                for (auto iter = found.first; iter != found.second; ++iter)
                {
                    if (iter->second == jobEntry.m_jobRunKey)
                    {
                        m_jobKeyToJobRunKeyMap.erase(iter);
                        break;
                    }
                }
            }
        }
    }


    //! A network request came in, Given a Job Run Key (from the above Job Request), asking for the actual log for that job.
    void AssetProcessorManager::ProcessGetAssetJobLogRequest(NetworkRequestID requestId, BaseAssetProcessorMessage* message, bool fencingFailed)
    {
        AssetJobLogRequest* request = azrtti_cast<AssetJobLogRequest*>(message);
        if (!request)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessGetAssetJobLogRequest: Message is not of type %d.Incoming message type is %d.\n", AssetJobLogRequest::MessageType(), message->GetMessageType());
            return;
        }

        AssetJobLogResponse response;
        ProcessGetAssetJobLogRequest(*request, response);
        EBUS_EVENT_ID(requestId.first, AssetProcessor::ConnectionBus, SendResponse, requestId.second, response);
    }

    void AssetProcessorManager::ProcessGetAssetJobLogRequest(const AssetJobLogRequest& request, AssetJobLogResponse& response)
    {
        JobInfo jobInfo;

        bool hasSpace = false;
        AssetProcessor::DiskSpaceInfoBus::BroadcastResult(hasSpace, &AssetProcessor::DiskSpaceInfoBusTraits::CheckSufficientDiskSpace, m_cacheRootDir.absolutePath().toUtf8().data(), 0, false);

        if (!hasSpace)
        {
            AZ_TracePrintf("AssetProcessorManager", "Warn: AssetProcessorManager: Low disk space detected\n");
            response.m_jobLog = "Warn: Low disk space detected.  Log file may be missing or truncated.  Asset processing is likely to fail.\n";
        }

        //look for the job in flight first
        bool found = false;
        auto foundElement = m_jobRunKeyToJobInfoMap.find(request.m_jobRunKey);
        if (foundElement != m_jobRunKeyToJobInfoMap.end())
        {
            found = true;
            jobInfo = foundElement->second;
        }
        else
        {
            // get the job infos by that job run key.
            JobInfoContainer jobInfos;
            if (!m_stateData->GetJobInfoByJobRunKey(request.m_jobRunKey, jobInfos))
            {
                AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: Failed to find the job for a request.\n");
                response.m_jobLog.append("Error: AssetProcessorManager: Failed to find the job for a request.");
                response.m_isSuccess = false;

                return;
            }

            AZ_Assert(jobInfos.size() == 1, "Should only have found one jobInfo!!!");
            jobInfo = AZStd::move(jobInfos[0]);
            found = true;
        }

        if (jobInfo.m_status == JobStatus::Failed_InvalidSourceNameExceedsMaxLimit)
        {
            response.m_jobLog.append(AZStd::string::format("Warn: Source file name exceeds the maximum length allowed (%d).", AP_MAX_PATH_LEN).c_str());
            response.m_isSuccess = true;
            return;
        }

        AssetUtilities::ReadJobLog(jobInfo, response);
    }

    //! A network request came in asking, for a given input asset, what the status is of any jobs related to that request
    void AssetProcessorManager::ProcessGetAssetJobsInfoRequest(NetworkRequestID requestId, BaseAssetProcessorMessage* message, bool fencingFailed)
    {
        AssetJobsInfoRequest* request = azrtti_cast<AssetJobsInfoRequest*>(message);
        if (!request)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessGetAssetJobsInfoRequest: Message is not of type %d.Incoming message type is %d.\n", AssetJobsInfoRequest::MessageType(), message->GetMessageType());
            return;
        }

        AssetJobsInfoResponse response;
        ProcessGetAssetJobsInfoRequest(*request, response);
        EBUS_EVENT_ID(requestId.first, AssetProcessor::ConnectionBus, SendResponse, requestId.second, response);
    }

    void AssetProcessorManager::ProcessGetAssetJobsInfoRequest(AssetJobsInfoRequest& request, AssetJobsInfoResponse& response)
    {
        if (request.m_assetId.IsValid())
        {
            //If the assetId is valid than search both the database and the pending queue and update the searchTerm with the source name
            bool found = false;
            {
                // First check the queue as this is the cheapest to do.
                AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);
                auto foundSource = m_sourceUUIDToSourceNameMap.find(request.m_assetId.m_guid);
                if (foundSource != m_sourceUUIDToSourceNameMap.end())
                {
                    // if we are here it means that the database have no knowledge about this source asset but it is in the queue for processing
                    request.m_searchTerm = foundSource->second.m_sourceName.toUtf8().data();
                    found = true;
                }
            }
            if (!found)
            {
                // If not found, then check the database.
                AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;
                if (m_stateData->GetSourceBySourceGuid(request.m_assetId.m_guid, entry))
                {
                    request.m_searchTerm = entry.m_sourceName.c_str();
                    found = true;
                }
            }
            if (!found)
            {
                // If still not found it means that this source asset is neither in the database nor in the queue for processing
                AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessGetAssetJobsInfoRequest: AssetProcessor unable to find the requested source asset having uuid (%s).\n", 
                    request.m_assetId.m_guid.ToString<AZStd::string>().c_str());
                response = AssetJobsInfoResponse(AzToolsFramework::AssetSystem::JobInfoContainer(), false);
                return;
            }
        }
        QString normalizedInputAssetPath;
 
        AzToolsFramework::AssetSystem::JobInfoContainer jobList;
        AssetProcessor::JobIdEscalationList jobIdEscalationList;
        if (!request.m_isSearchTermJobKey)
        {
            normalizedInputAssetPath = AssetUtilities::NormalizeFilePath(request.m_searchTerm.c_str());

            if (QFileInfo(normalizedInputAssetPath).isAbsolute())
            {
                QString scanFolderName;
                QString relativePathToFile;
                if (!m_platformConfig->ConvertToRelativePath(normalizedInputAssetPath, relativePathToFile, scanFolderName))
                {
                    response = AssetJobsInfoResponse(AzToolsFramework::AssetSystem::JobInfoContainer(), false);
                    return;
                }

                normalizedInputAssetPath = relativePathToFile;
            }

            //any queued or in progress jobs will be in the map:
            for (const auto& entry : m_jobRunKeyToJobInfoMap)
            {
                if (AzFramework::StringFunc::Equal(entry.second.m_sourceFile.c_str(), normalizedInputAssetPath.toUtf8().constData()))
                {
                    jobList.push_back(entry.second);
                    if (request.m_escalateJobs)
                    {
                        jobIdEscalationList.append(qMakePair(entry.second.m_jobRunKey, AssetProcessor::JobEscalation::AssetJobRequestEscalation));
                    }
                }
            }
        }
        else
        {
            auto found = m_jobKeyToJobRunKeyMap.equal_range(request.m_searchTerm.c_str());

            for (auto iter = found.first; iter != found.second; ++iter)
            {
                auto foundJobRunKey = m_jobRunKeyToJobInfoMap.find(iter->second);
                if (foundJobRunKey != m_jobRunKeyToJobInfoMap.end())
                {
                    AzToolsFramework::AssetSystem::JobInfo& jobInfo = foundJobRunKey->second;
                    jobList.push_back(jobInfo);
                    if (request.m_escalateJobs)
                    {
                        jobIdEscalationList.append(qMakePair(iter->second, AssetProcessor::JobEscalation::AssetJobRequestEscalation));
                    }
                }
            }
        }

        if (!jobIdEscalationList.empty())
        {
            Q_EMIT EscalateJobs(jobIdEscalationList);
        }

        AzToolsFramework::AssetSystem::JobInfoContainer jobListDataBase;
        if (!request.m_isSearchTermJobKey)
        {
            //any succeeded or failed jobs will be in the table
            m_stateData->GetJobInfoBySourceName(normalizedInputAssetPath.toUtf8().constData(), jobListDataBase);
        }
        else
        {
            //check the database for all jobs with that job key 
            m_stateData->GetJobInfoByJobKey(request.m_searchTerm, jobListDataBase);
        }

        for (const AzToolsFramework::AssetSystem::JobInfo& job : jobListDataBase)
        {
            auto result = AZStd::find_if(jobList.begin(), jobList.end(),
                [&job](AzToolsFramework::AssetSystem::JobInfo& entry) -> bool
                {
                    return
                        AzFramework::StringFunc::Equal(entry.m_platform.c_str(), job.m_platform.c_str()) &&
                        AzFramework::StringFunc::Equal(entry.m_jobKey.c_str(), job.m_jobKey.c_str()) &&
                        AzFramework::StringFunc::Equal(entry.m_sourceFile.c_str(), job.m_sourceFile.c_str());
                });
            if (result == jobList.end())
            {
                // A job for this asset has already completed and was registered with the database so report that one as well.
                jobList.push_back(job);
            }
        }

        response = AssetJobsInfoResponse(jobList, true);
    }

    void AssetProcessorManager::CheckMissingFiles()
    {
        if (!m_activeFiles.isEmpty())
        {
            // not ready yet, we have not drained the queue.
            QTimer::singleShot(10, this, SLOT(CheckMissingFiles()));
            return;
        }

        if (m_isCurrentlyScanning)
        {
            return;
        }

        for (auto iter = m_SourceFilesInDatabase.begin(); iter != m_SourceFilesInDatabase.end(); iter++)
        {
            CheckDeletedSourceFile(iter.key(), iter.value());
        }

        // we want to remove any left over scan folders from the database only after
        // we remove all the products from source files we are no longer interested in, 
        // we do it last instead of when we update scan folders because the scan folders table CASCADE DELETE on the sources, jobs,
        // products table and we want to do this last after cleanup of disk.
        for (auto entry : m_scanFoldersInDatabase)
        {
            if (!m_stateData->RemoveScanFolder(entry.second.m_scanFolderID))
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckMissingFiles: Unable to remove Scan Folder having id %d from the database.", entry.second.m_scanFolderID);
                return;
            }
        }

        m_scanFoldersInDatabase.clear();
        m_SourceFilesInDatabase.clear();
        QMetaObject::invokeMethod(this, "CheckForIdle", Qt::QueuedConnection);
    }

    void AssetProcessorManager::QuitRequested()
    {
        m_quitRequested = true;
        m_filesToExamine.clear();
        Q_EMIT ReadyToQuit(this);
    }

    //! This request comes in and is expected to do whatever heuristic is required in order to determine if an asset actually exists in the database.
    void AssetProcessorManager::OnRequestAssetExists(NetworkRequestID groupID, QString platform, QString searchTerm)
    {
        QString productName = GuessProductOrSourceAssetName(searchTerm, false);

        Q_EMIT SendAssetExistsResponse(groupID, !productName.isEmpty());
    }

    QString AssetProcessorManager::GuessProductOrSourceAssetName(QString searchTerm, bool useLikeSearch)
    {
        // Search the product table
        QString productName = AssetUtilities::GuessProductNameInDatabase(searchTerm, m_stateData.get());

        if (!productName.isEmpty())
        {
            return productName;
        }

        // Search the source table
        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;

        if (!useLikeSearch && m_stateData->GetProductsBySourceName(searchTerm, products))
        {
            return searchTerm;
        }
        else if (useLikeSearch && m_stateData->GetProductsLikeSourceName(searchTerm, AzToolsFramework::AssetDatabase::AssetDatabaseConnection::LikeType::StartsWith, products))
        {
            return searchTerm;
        }

        return QString();
    }

    void AssetProcessorManager::RequestReady(NetworkRequestID networkRequestId, BaseAssetProcessorMessage* message, QString platform, bool fencingFailed)
    {
        using namespace AzFramework::AssetSystem;
        using namespace AzToolsFramework::AssetSystem;

        if (message->GetMessageType() == AssetJobsInfoRequest::MessageType())
        {
            ProcessGetAssetJobsInfoRequest(networkRequestId, message, fencingFailed);
        }
        else if (message->GetMessageType() == AssetJobLogRequest::MessageType())
        {
            ProcessGetAssetJobLogRequest(networkRequestId, message, fencingFailed);
        }

        delete message;
    }

    void AssetProcessorManager::AssetCancelled(AssetProcessor::JobEntry jobEntry)
    {
        if (m_quitRequested)
        {
            return;
        }
        // Remove the log file for the cancelled job
        AZStd::string logFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobEntry);
        EraseLogFile(logFile.c_str());

        OnJobStatusChanged(jobEntry, JobStatus::Failed);
        // we know that things have changed at this point; ensure that we check for idle
        QMetaObject::invokeMethod(this, "CheckForIdle", Qt::QueuedConnection);
    }

    void AssetProcessorManager::AssetFailed(AssetProcessor::JobEntry jobEntry)
    {
        if (m_quitRequested)
        {
            return;
        }

        m_AssetProcessorIsBusy = true;
        Q_EMIT AssetProcessorManagerIdleState(false);

        // if its a fake "autofail job" or other reason for it not to exist in the DB, don't do anything here.
        if (!jobEntry.m_addToDatabase)
        {
            return;
        }

        // wipe the times so that it will try again next time.
        // note:  Leave the prior successful products where they are, though.

        // We have to include a fingerprint in the database for this job, otherwise when assets change that
        // affect this failed job, the failed assets won't get rescanned and won't be in the database and
        // therefore won't get reprocessed. Set it to FAILED_FINGERPRINT.
        //create/update the source record for this job
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
        AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
        if (m_stateData->GetSourcesBySourceName(jobEntry.m_relativePathToFile, sources))
        {
            AZ_Assert(sources.size() == 1, "Should have only found one source!!!");
            source = AZStd::move(sources[0]);
        }
        else
        {
            //if we didn't find a source, we make a new source
            const ScanFolderInfo* scanFolder = m_platformConfig->GetScanFolderForFile(jobEntry.m_absolutePathToFile);
            if (!scanFolder)
            {
                //can't find the scan folder this source came from!?
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for this source!!!");
            }

            //add the new source
            AddSourceToDatabase(source, scanFolder, jobEntry.m_relativePathToFile);
        }

        //create/update the job
        AzToolsFramework::AssetDatabase::JobDatabaseEntry job;
        AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
        if (m_stateData->GetJobsBySourceID(source.m_sourceID, jobs, jobEntry.m_builderGuid, jobEntry.m_jobKey, jobEntry.m_platformInfo.m_identifier.c_str()))
        {
            AZ_Assert(jobs.size() == 1, "Should have only found one job!!!");
            job = AZStd::move(jobs[0]);

            //we only want to keep the first fail and the last fail log
            //if it has failed before, both first and last will be set, only delete last fail file if its not the first fail
            if (job.m_firstFailLogTime && job.m_firstFailLogTime != job.m_lastFailLogTime)
            {
                EraseLogFile(job.m_lastFailLogFile.c_str());
            }

            //we failed so the last fail is the same as the current
            job.m_lastFailLogTime = job.m_lastLogTime = QDateTime::currentMSecsSinceEpoch();
            job.m_lastFailLogFile = job.m_lastLogFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobEntry);

            //if we have never failed before also set the first fail to be the last fail
            if (!job.m_firstFailLogTime)
            {
                job.m_firstFailLogTime = job.m_lastFailLogTime;
                job.m_firstFailLogFile = job.m_lastFailLogFile;
            }
        }
        else
        {
            //if we didn't find a job, we make a new one
            job.m_sourcePK = source.m_sourceID;
            job.m_fingerprint = FAILED_FINGERPRINT;
            job.m_jobKey = jobEntry.m_jobKey.toUtf8().constData();
            job.m_platform = jobEntry.m_platformInfo.m_identifier;
            job.m_builderGuid = jobEntry.m_builderGuid;

            //if this is a new job that failed then first filed ,last failed and current are the same
            job.m_firstFailLogTime = job.m_lastFailLogTime = job.m_lastLogTime = QDateTime::currentMSecsSinceEpoch();
            job.m_firstFailLogFile = job.m_lastFailLogFile = job.m_lastLogFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobEntry);
        }

        //set the random key
        job.m_jobRunKey = jobEntry.m_jobRunKey;

        //set the new status
        job.m_status = jobEntry.m_absolutePathToFile.length() < AP_MAX_PATH_LEN ? JobStatus::Failed : JobStatus::Failed_InvalidSourceNameExceedsMaxLimit;

        //create/update job
        if (!m_stateData->SetJob(job))
        {
            //somethings wrong...
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to update the job in the database!!!");
        }

#if defined(BATCH_MODE) && !defined(UNIT_TEST)
        AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed processing: %s %s %s %s\n", jobEntry.m_relativePathToFile.toUtf8().constData(), jobEntry.m_builderGuid.ToString<AZStd::string>().c_str(), jobEntry.m_jobKey.toUtf8().constData(), jobEntry.m_platform.toUtf8().constData());
#else
        const ScanFolderInfo* scanFolder = m_platformConfig->GetScanFolderForFile(jobEntry.m_absolutePathToFile);
        AzToolsFramework::AssetSystem::SourceFileNotificationMessage message(AZ::OSString(source.m_sourceName.c_str()), AZ::OSString(scanFolder->ScanPath().toUtf8().constData()), AzToolsFramework::AssetSystem::SourceFileNotificationMessage::FileFailed, source.m_sourceGuid);
        EBUS_EVENT(AssetProcessor::ConnectionBus, Send, 0, message);
#endif

        OnJobStatusChanged(jobEntry, JobStatus::Failed);
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Failed %s, (%s)... \n",
            jobEntry.m_relativePathToFile.toUtf8().constData(),
            jobEntry.m_platformInfo.m_identifier.c_str());
        AZ_TracePrintf(AssetProcessor::DebugChannel, "AssetProcessed [fail] Jobkey \"%s\", Builder UUID \"%s\", Fingerprint %u ) \n",
            jobEntry.m_jobKey.toUtf8().constData(),
            jobEntry.m_builderGuid.ToString<AZStd::string>().c_str(),
            jobEntry.m_computedFingerprint);

        // we know that things have changed at this point; ensure that we check for idle after we've finished processing all of our assets
        // and don't rely on the file watcher to check again.
        // If we rely on the file watcher only, it might fire before the AssetMessage signal has been responded to and the
        // Asset Catalog may not realize that things are dirty by that point.
        QMetaObject::invokeMethod(this, "CheckForIdle", Qt::QueuedConnection);
    }

    void AssetProcessorManager::AssetProcessed_Impl()
    {
        m_processedQueued = false;
        if (m_quitRequested || m_assetProcessedList.empty())
        {
            return;
        }
       
        // Note: if we get here, the scanning / createjobs phase has finished
        // because we no longer start any jobs until it has finished.  So there is no reason
        // to delay notification or processing.

        // lower case the cache root so that we can get the valid relative
        // path on macOS since the product name we get is always lower case
        // but the cache root is not resulting in a relative path that is
        // unusable
        QDir cacheRootDirLowerCase(m_cacheRootDir.absolutePath().toLower());

        // before we accept this outcome, let's make sure that the same product was not emitted by some other job.  we detect this by finding other jobs
        // with the same product, but with different sources.
        for (auto itProcessedAsset = m_assetProcessedList.begin(); itProcessedAsset != m_assetProcessedList.end(); )
        {
            bool remove = false;
            for (const AssetBuilderSDK::JobProduct& product : itProcessedAsset->m_response.m_outputProducts)
            {
                // if the claimed product does not exist, remove it so it does not get into the database
                if (!QFile::exists(product.m_productFileName.c_str()))
                {
                    remove = true;
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was expecting product file %s... but it already appears to be gone. \n", product.m_productFileName.c_str());
                }
                else
                {
                    // database products, if present, will be in the form "platform/game/subfolders/productfile", convert
                    // our new products to the same thing by removing the cache root
                    QString newProductName = product.m_productFileName.c_str();
                    newProductName = AssetUtilities::NormalizeFilePath(newProductName);
                    if (!newProductName.startsWith(m_normalizedCacheRootPath, Qt::CaseInsensitive))
                    {
                        AZ_Error(AssetProcessor::ConsoleChannel, "AssetProcessed(\" << %s << \", \" << %s << \" ... ) cache file \"  %s << \" does not appear to be within the cache!.\n",
                            itProcessedAsset->m_entry.m_relativePathToFile.toUtf8().constData(),
                            itProcessedAsset->m_entry.m_platformInfo.m_identifier.c_str(),
                            newProductName.toUtf8().constData());
                    }
                    newProductName = cacheRootDirLowerCase.relativeFilePath(newProductName).toLower();

                    // query all sources for this exact new product name
                    // the intention here is to find out conflicts where two different sources produce the same exact product.
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
                    if (m_stateData->GetSourcesByProductName(newProductName.toUtf8().constData(), sources))
                    {
                        for (const auto& source : sources)
                        {
                            if (azstricmp(source.m_sourceName.c_str(), itProcessedAsset->m_entry.m_relativePathToFile.toUtf8().constData()) != 0)
                            {
                                remove = true;
                                //this means we have a dupe product name for a different source
                                //usually this is caused by /blah/x.tif and an /blah/x.dds in the source folder
                                //they both become /blah/x.dds in the cache
                                //Not much of an option here, if we find a dupe we already lost access to the
                                //first one in the db because it was overwritten. So do not commit this new one and
                                //set the first for reprocessing. That way we will get the original back.

                                //delete the original sources products
                                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                                m_stateData->GetProductsBySourceID(source.m_sourceID, products);
                                DeleteProducts(products);

                                //set the fingerprint to failed
                                AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
                                m_stateData->GetJobsBySourceID(source.m_sourceID, jobs);
                                for (auto& job : jobs)
                                {
                                    job.m_fingerprint = FAILED_FINGERPRINT;
                                    m_stateData->SetJob(job);
                                }

                                //delete product files for this new source
                                for (const auto& product : itProcessedAsset->m_response.m_outputProducts)
                                {
                                    if (!QFile::exists(product.m_productFileName.c_str()))
                                    {
                                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was expecting to delete product file %s... but it already appears to be gone. \n", product.m_productFileName.c_str());
                                    }
                                    else if (!QFile::remove(product.m_productFileName.c_str()))
                                    {
                                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was unable to delete product file %s...\n", product.m_productFileName.c_str());
                                    }
                                    else
                                    {
                                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Deleted product file %s\n", product.m_productFileName.c_str());
                                    }
                                }

                                //let people know what happened
                                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "%s has failed because another source %s has already produced the same product %s. Rebuild the original Source.\n",
                                    itProcessedAsset->m_entry.m_absolutePathToFile.toUtf8().constData(), source.m_sourceName.c_str(), newProductName.toUtf8().constData());

                                //recycle the original source
                                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanfolder;
                                AZStd::string fullSourcePath = source.m_sourceName;
                                if (m_stateData->GetScanFolderByScanFolderID(source.m_scanFolderPK, scanfolder))
                                {
                                    fullSourcePath = AZStd::string::format("%s/%s", scanfolder.m_scanFolder.c_str(), source.m_sourceName.c_str());
                                    AssessFileInternal(fullSourcePath.c_str(), false);
                                }

                                QString duplicateProduct = cacheRootDirLowerCase.absoluteFilePath(newProductName);

                                JobDetails jobdetail;
                                jobdetail.m_jobEntry = JobEntry(itProcessedAsset->m_entry.m_absolutePathToFile, itProcessedAsset->m_entry.m_relativePathToFile, itProcessedAsset->m_entry.m_builderGuid, itProcessedAsset->m_entry.m_platformInfo, itProcessedAsset->m_entry.m_jobKey, 0, GenerateNewJobRunKey(), itProcessedAsset->m_entry.m_sourceFileUUID);
                                jobdetail.m_autoFail = true;
                                jobdetail.m_critical = true;
                                jobdetail.m_priority = INT_MAX; // front of the queue.
                                // the new lines make it easier to copy and paste the file names.
                                jobdetail.m_jobParam[AZ_CRC(AutoFailReasonKey)] = AZStd::string::format(
                                        "A different source file\n%s\nis already outputting the product\n%s\n"
                                        "Please check other files in the same folder as source file and make sure no two sources output the product file.\n"
                                        "For example, you can't have a DDS file and a TIF file in the same folder, as they would cause overwriting.\n",
                                        fullSourcePath.c_str(),
                                        duplicateProduct.toUtf8().data());

                                Q_EMIT AssetToProcess(jobdetail);// forwarding this job to rccontroller to fail it
                            }
                        }
                    }
                }
            }

            if (remove)
            {
                //we found a dupe remove this entry from the processed list so it does not get into the db
                itProcessedAsset = m_assetProcessedList.erase(itProcessedAsset);
            }
            else
            {
                ++itProcessedAsset;
            }
        }

        //process the asset list
        for (AssetProcessedEntry& processedAsset : m_assetProcessedList)
        {
            // update products / delete no longer relevant products
            // note that the cache stores products WITH the name of the platform in it so you don't have to do anything
            // to those strings to process them.

            //create/update the source record for this job
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
            AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
            auto scanFolder = m_platformConfig->GetScanFolderForFile(processedAsset.m_entry.m_absolutePathToFile);
            if (!scanFolder)
            {
                //can't find the scan folder this source came from!?
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for this source!!!");
            }

            if (m_stateData->GetSourcesBySourceNameScanFolderId(processedAsset.m_entry.m_relativePathToFile, scanFolder->ScanFolderID(), sources))
            {
                AZ_Assert(sources.size() == 1, "Should have only found one source!!!");
                source = AZStd::move(sources[0]);
            }
            else
            {
                //if we didn't find a source, we make a new source
                //add the new source
                AddSourceToDatabase(source, scanFolder, processedAsset.m_entry.m_relativePathToFile);
            }

            //create/update the job
            AzToolsFramework::AssetDatabase::JobDatabaseEntry job;
            AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
            if (m_stateData->GetJobsBySourceID(source.m_sourceID, jobs, processedAsset.m_entry.m_builderGuid, processedAsset.m_entry.m_jobKey, processedAsset.m_entry.m_platformInfo.m_identifier.c_str()))
            {
                AZ_Assert(jobs.size() == 1, "Should have only found one job!!!");
                job = AZStd::move(jobs[0]);
            }
            else
            {
                //if we didn't find a job, we make a new one
                job.m_sourcePK = source.m_sourceID;
            }

            job.m_fingerprint = processedAsset.m_entry.m_computedFingerprint;
            job.m_jobKey = processedAsset.m_entry.m_jobKey.toUtf8().constData();
            job.m_platform = processedAsset.m_entry.m_platformInfo.m_identifier;
            job.m_builderGuid = processedAsset.m_entry.m_builderGuid;
            job.m_jobRunKey = processedAsset.m_entry.m_jobRunKey;


            if (!AZ::IO::FileIOBase::GetInstance()->Exists(job.m_lastLogFile.c_str()))
            {
                // its okay for the log to not exist, if there was no log for it (for example simple jobs that just copy assets and did not encounter any problems will generate no logs)
                job.m_lastLogFile.clear();
            }

            // delete any previous failed job logs:
            bool deletedFirstFailedLog = EraseLogFile(job.m_firstFailLogFile.c_str());
            bool deletedLastFailedLog = EraseLogFile(job.m_lastFailLogFile.c_str());

            // also delete the existing log file since we're about to replace it:
            EraseLogFile(job.m_lastLogFile.c_str());

            // if we deleted them, then make sure the DB no longer tracks them either.
            if (deletedLastFailedLog)
            {
                job.m_lastFailLogTime = 0;
                job.m_lastFailLogFile.clear();
            }

            if (deletedFirstFailedLog)
            {
                job.m_firstFailLogTime = 0;
                job.m_firstFailLogFile.clear();
            }

            //set the new status and update log
            job.m_status = JobStatus::Completed;
            job.m_lastLogTime = QDateTime::currentMSecsSinceEpoch();
            job.m_lastLogFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(processedAsset.m_entry);

            // create/update job:
            if (!m_stateData->SetJob(job))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to update the job in the database!");
            }

            //query prior products for this job id
            AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer priorProducts;
            m_stateData->GetProductsByJobID(job.m_jobID, priorProducts);

            //make new product entries from the job response output products
            AZStd::vector<AZStd::pair<AzToolsFramework::AssetDatabase::ProductDatabaseEntry, const AssetBuilderSDK::JobProduct*>> newProducts;
            AZStd::vector<AZStd::vector<AZ::u32>> newLegacySubIDs;  // each product has a vector of legacy subids;
            for (const AssetBuilderSDK::JobProduct& product : processedAsset.m_response.m_outputProducts)
            {
                // prior products, if present, will be in the form "platform/game/subfolders/productfile", convert
                // our new products to the same thing by removing the cache root
                QString newProductName = product.m_productFileName.c_str();
                newProductName = AssetUtilities::NormalizeFilePath(newProductName);
                if (!newProductName.startsWith(m_normalizedCacheRootPath, Qt::CaseInsensitive))
                {
                    AZ_Error(AssetProcessor::ConsoleChannel, "AssetProcessed(\" << %s << \", \" << %s << \" ... ) cache file \"  %s << \" does not appear to be within the cache!.\n",
                        processedAsset.m_entry.m_relativePathToFile.toUtf8().constData(),
                        processedAsset.m_entry.m_platformInfo.m_identifier.c_str(),
                        newProductName.toUtf8().constData());
                }
                newProductName = cacheRootDirLowerCase.relativeFilePath(newProductName).toLower();

                //make a new product entry for this file
                AzToolsFramework::AssetDatabase::ProductDatabaseEntry newProduct;
                newProduct.m_jobPK = job.m_jobID;
                newProduct.m_productName = newProductName.toUtf8().constData();
                newProduct.m_assetType = product.m_productAssetType;
                newProduct.m_subID = product.m_productSubID;

                //This is the legacy product guid, its only use is for backward compatibility as before the asset id's guid was created off of the relative product name.
                // Right now when we query for an asset guid we first match on the source guid which is correct and secondarily match on the product guid. Eventually this will go away.
                newProductName = newProductName.right(newProductName.length() - newProductName.indexOf('/') - 1); // remove PLATFORM and an extra slash
                newProductName = newProductName.right(newProductName.length() - newProductName.indexOf('/') - 1); // remove GAMENAME and an extra slash
                newProduct.m_legacyGuid = AZ::Uuid::CreateName(newProductName.toUtf8().constData());

                //push back the new product into the new products list
                newProducts.emplace_back(newProduct, &product);
                newLegacySubIDs.push_back(product.m_legacySubIDs);
            }

            //now we want to remove any lingering product files from the previous build that no longer exist
            //so subtract the new products from the prior products, whatever is left over in prior products no longer exists
            if (!priorProducts.empty())
            {
                for (const auto& pair : newProducts)
                {
                    const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& newProductEntry = pair.first;
                    priorProducts.erase(AZStd::remove(priorProducts.begin(), priorProducts.end(), newProductEntry), priorProducts.end());
                }
            }

            // we need to delete these product files from the disk as they no longer exist and inform everyone we did so
            for (const auto& priorProduct : priorProducts)
            {
                // product name will be in the form "platform/game/relativeProductPath"
                QString productName = priorProduct.m_productName.c_str();

                // the full file path is gotten by adding the product name to the cache root
                QString fullProductPath = cacheRootDirLowerCase.absoluteFilePath(productName);

                // relative file path is gotten by removing the platform and game from the product name
                QString relativeProductPath = productName;
                relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // remove PLATFORM and an extra slash
                relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // remove GAMENAME and an extra slash

                AZ::Data::AssetId assetId(source.m_sourceGuid, priorProduct.m_subID);

                // also compute the legacy ids that used to refer to this asset

                AZ::Data::AssetId legacyAssetId(priorProduct.m_legacyGuid, 0);
                AZ::Data::AssetId legacySourceAssetId(AssetUtilities::CreateSafeSourceUUIDFromName(source.m_sourceName.c_str(), false), priorProduct.m_subID);

                AssetNotificationMessage message(relativeProductPath.toUtf8().constData(), AssetNotificationMessage::AssetRemoved, priorProduct.m_assetType);
                message.m_assetId = assetId;

                if (legacyAssetId != assetId)
                {
                    message.m_legacyAssetIds.push_back(legacyAssetId);
                }

                if (legacySourceAssetId != assetId)
                {
                    message.m_legacyAssetIds.push_back(legacySourceAssetId);
                }

                bool shouldDeleteFile = true;
                for (const auto& pair : newProducts)
                {
                    const auto& currentProduct = pair.first;

                    if (AzFramework::StringFunc::Equal(currentProduct.m_productName.c_str(), priorProduct.m_productName.c_str()))
                    {
                        // This is a special case - The subID and other fields differ but it outputs the same actual product file on disk
                        // so let's not delete that product file since by the time we get here, it has already replaced it in the cache folder
                        // with the new product.
                        shouldDeleteFile = false;
                        break;
                    }
                }
                //delete the full file path
                if (shouldDeleteFile)
                {
                    if (!QFile::exists(fullProductPath))
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was expecting to delete %s ... but it already appears to be gone. \n", fullProductPath.toUtf8().constData());

                        // we still need to tell everyone that its gone!

                        Q_EMIT AssetMessage(processedAsset.m_entry.m_platformInfo.m_identifier.c_str(), message); // we notify that we are aware of a missing product either way.
                    }
                    else if (!QFile::remove(fullProductPath))
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was unable to delete file %s will retry next time...\n", fullProductPath.toUtf8().constData());
                        continue; // do not update database
                    }
                    else
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Deleting file %s because the recompiled input file no longer emitted that product.\n", fullProductPath.toUtf8().constData());

                        Q_EMIT AssetMessage(QString(processedAsset.m_entry.m_platformInfo.m_identifier.c_str()), message); // we notify that we are aware of a missing product either way.
                    }
                }
                else
                {
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "File %s was replaced with a new, but different file.\n", fullProductPath.toUtf8().constData());
                    // Don't report that the file has been removed as it's still there, but as a different kind of file (different sub id, type, etc.).
                }

                //trace that we are about to remove a lingering prior product from the database
                // because of On Delete Cascade this will also remove any legacy subIds associated with that product automatically.
                if (!m_stateData->RemoveProduct(priorProduct.m_productID))
                {
                    //somethings wrong...
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to remove lingering prior products from the database!!! %s", priorProduct.ToString().c_str());
                }
                else
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Removed lingering prior product %s\n", priorProduct.ToString().c_str());
                }

                QString parentFolderName = QFileInfo(fullProductPath).absolutePath();
                m_checkFoldersToRemove.insert(parentFolderName);
            }

            //trace that we are about to update the products in the database

            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Processed \"%s\" (\"%s\")... \n",
                processedAsset.m_entry.m_relativePathToFile.toUtf8().constData(),
                processedAsset.m_entry.m_platformInfo.m_identifier.c_str());
            AZ_TracePrintf(AssetProcessor::DebugChannel, "JobKey \"%s\", Builder UUID \"%s\", Fingerprint %u ) \n",
                processedAsset.m_entry.m_jobKey.toUtf8().constData(),
                processedAsset.m_entry.m_builderGuid.ToString<AZStd::string>().c_str(),
                processedAsset.m_entry.m_computedFingerprint);

            AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;

            //set the new products
            for (size_t productIdx = 0; productIdx < newProducts.size(); ++productIdx)
            {
                auto& pair = newProducts[productIdx];
                AzToolsFramework::AssetDatabase::ProductDatabaseEntry& newProduct = pair.first;
                AZStd::vector<AZ::u32>& subIds = newLegacySubIDs[productIdx];

                if (!m_stateData->SetProduct(newProduct))
                {
                    //somethings wrong...
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to set new product in the the database!!! %s", newProduct.ToString().c_str());
                }
                else
                {
                    m_stateData->RemoveLegacySubIDsByProductID(newProduct.m_productID);
                    for (AZ::u32 subId : subIds)
                    {
                        AzToolsFramework::AssetDatabase::LegacySubIDsEntry entryToCreate(newProduct.m_productID, subId);
                        m_stateData->CreateOrUpdateLegacySubID(entryToCreate);
                    }

                    // Remove all previous dependencies
                    if (!m_stateData->RemoveProductDependencyByProductId(newProduct.m_productID))
                    {
                        AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to remove old product dependencies for product %d", newProduct.m_productID);
                    }

                    const AssetBuilderSDK::JobProduct* jobProduct = pair.second;
                    // Build up the list of new dependencies
                    for (auto& productDependency : jobProduct->m_dependencies)
                    {
                        dependencyContainer.emplace_back(newProduct.m_productID, productDependency.m_dependencyId.m_guid, productDependency.m_dependencyId.m_subId, productDependency.m_flags);
                    }
                }
            }

            // Set the new dependencies
            if (!m_stateData->SetProductDependencies(dependencyContainer))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to set product dependencies");
            }

            // now we need notify everyone about the new products
            for (size_t productIdx = 0; productIdx < newProducts.size(); ++productIdx)
            {
                auto& pair = newProducts[productIdx];
                AzToolsFramework::AssetDatabase::ProductDatabaseEntry& newProduct = pair.first;
                AZStd::vector<AZ::u32>& subIds = newLegacySubIDs[productIdx];

                // product name will be in the form "platform/game/relativeProductPath"
                QString productName = QString::fromUtf8(newProduct.m_productName.c_str());

                // the full file path is gotten by adding the product name to the cache root
                QString fullProductPath = m_cacheRootDir.absoluteFilePath(productName);

                // relative file path is gotten by removing the platform and game from the product name
                QString relativeProductPath = productName;
                relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // remove PLATFORM and an extra slash
                relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // remove GAMENAME and an extra slash

                AssetNotificationMessage message(relativeProductPath.toUtf8().constData(), AssetNotificationMessage::AssetChanged, newProduct.m_assetType);
                AZ::Data::AssetId assetId(source.m_sourceGuid, newProduct.m_subID);
                AZ::Data::AssetId legacyAssetId(newProduct.m_legacyGuid, 0);
                AZ::Data::AssetId legacySourceAssetId(AssetUtilities::CreateSafeSourceUUIDFromName(source.m_sourceName.c_str(), false), newProduct.m_subID);

                message.m_data = relativeProductPath.toUtf8().data();
                message.m_sizeBytes = QFileInfo(fullProductPath).size();
                message.m_assetId = assetId;
                
                message.m_dependencies.reserve(dependencyContainer.size());

                for (auto& entry : dependencyContainer)
                {
                    message.m_dependencies.emplace_back(AZ::Data::AssetId(entry.m_dependencySourceGuid, entry.m_dependencySubID), entry.m_dependencyFlags);
                }

                if (legacyAssetId != assetId)
                {
                    message.m_legacyAssetIds.push_back(legacyAssetId);
                }

                if (legacySourceAssetId != assetId)
                {
                    message.m_legacyAssetIds.push_back(legacySourceAssetId);
                }

                for (AZ::u32 newLegacySubId : subIds)
                {
                    AZ::Data::AssetId createdSubID(source.m_sourceGuid, newLegacySubId);
                    if ((createdSubID != legacyAssetId) && (createdSubID != legacySourceAssetId) && (createdSubID != assetId))
                    {
                        message.m_legacyAssetIds.push_back(createdSubID);
                    }
                }

                Q_EMIT AssetMessage(QString(processedAsset.m_entry.m_platformInfo.m_identifier.c_str()), message);

                AddKnownFoldersRecursivelyForFile(fullProductPath, m_cacheRootDir.absolutePath());
            }
#if defined(BATCH_MODE) && !defined(UNIT_TEST)
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Processed: %s successfully processed for the platform %s.\n", processedAsset.m_entry.m_absolutePathToFile.toUtf8().constData(), processedAsset.m_platform.toUtf8().constData());
#else
            // notify the system about inputs:
            Q_EMIT InputAssetProcessed(processedAsset.m_entry.m_absolutePathToFile, QString(processedAsset.m_entry.m_platformInfo.m_identifier.c_str()));
            OnJobStatusChanged(processedAsset.m_entry, JobStatus::Completed);
#endif
        }

        m_assetProcessedList.clear();
        // we know that things have changed at this point; ensure that we check for idle after we've finished processing all of our assets
        // and don't rely on the file watcher to check again.
        // If we rely on the file watcher only, it might fire before the AssetMessage signal has been responded to and the
        // Asset Catalog may not realize that things are dirty by that point.
        QMetaObject::invokeMethod(this, "CheckForIdle", Qt::QueuedConnection);
    }

    void AssetProcessorManager::AssetProcessed(JobEntry jobEntry, AssetBuilderSDK::ProcessJobResponse response)
    {
        if (m_quitRequested)
        {
            return;
        }

        m_AssetProcessorIsBusy = true;
        Q_EMIT AssetProcessorManagerIdleState(false);

        // if its a fake "autosuccess job" or other reason for it not to exist in the DB, don't do anything here.
        if (!jobEntry.m_addToDatabase)
        {
            return;
        }

        m_assetProcessedList.push_back(AssetProcessedEntry(jobEntry, response));

        if (!m_processedQueued)
        {
            QMetaObject::invokeMethod(this, "AssetProcessed_Impl", Qt::QueuedConnection);
            m_processedQueued = true;
        }
    }

    void AssetProcessorManager::CheckSource(const FileEntry& source)
    {
        // when this function is triggered, it means that a file appeared because it was modified or added or deleted,
        // and the grace period has elapsed.
        // this is the first point at which we MIGHT be interested in a file.
        // to avoid flooding threads we queue these up for later checking.

        AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckSource: %s %s\n", source.m_fileName.toUtf8().constData(), source.m_isDelete ? "true" : "false");

        QString normalizedFilePath = AssetUtilities::NormalizeFilePath(source.m_fileName);

        if (m_platformConfig->IsFileExcluded(normalizedFilePath))
        {
            return;
        }

        // if metadata file change, pretend the actual file changed
        // the fingerprint will be different anyway since metadata file is folded in
        QString lowerName = normalizedFilePath.toLower();

        for (int idx = 0; idx < m_platformConfig->MetaDataFileTypesCount(); idx++)
        {
            QPair<QString, QString> metaInfo = m_platformConfig->GetMetaDataFileTypeAt(idx);
            QString originalName = normalizedFilePath;

            if (normalizedFilePath.endsWith("." + metaInfo.first, Qt::CaseInsensitive))
            {
                //its a meta file.  What was the original?

                normalizedFilePath = normalizedFilePath.left(normalizedFilePath.length() - (metaInfo.first.length() + 1));
                if (!metaInfo.second.isEmpty())
                {
                    // its not empty - replace the meta file with the original extension
                    normalizedFilePath += ".";
                    normalizedFilePath += metaInfo.second;
                }

                // we need the actual casing of the source file
                // but the metafile might have different casing... Qt will fail to get the -actual- casing of the source file, which we need.  It uses string ops internally.
                // so we have to work around this by using the Dir that the file is in:

                QFileInfo newInfo(normalizedFilePath);
                QStringList searchPattern;
                searchPattern << newInfo.fileName();

                QStringList actualCasing = newInfo.absoluteDir().entryList(searchPattern, QDir::Files);

                if (actualCasing.empty())
                {
                    QString warning = QCoreApplication::translate("Warning", "Warning:  Metadata file (%1) missing source file (%2)\n").arg(originalName).arg(normalizedFilePath);
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, warning.toUtf8().constData());
                    return;
                }

                // the casing might be different, too, so retrieve the actual case of the actual source file here:
                normalizedFilePath = newInfo.absoluteDir().absoluteFilePath(actualCasing[0]);
                lowerName = normalizedFilePath.toLower();
                break;
            }
        }
        // even if the entry already exists,
        // overwrite the entry here, so if you modify, then delete it, its the latest action thats always on the list.

        m_filesToExamine[lowerName] = FileEntry(normalizedFilePath, source.m_isDelete);

        if (!source.m_isDelete)
        {
            QStringList relativeSourcePathQueue = CheckSourceFileDependency(normalizedFilePath);

            for (const QString& relPath : relativeSourcePathQueue)
            {
                // If we are here make sure that the dependent source file is not present either in the fileToExamine queue nor in the SourceDependencyUUIDToSourceName Map, because if they are 
                // it indicates that the file is already waiting to be processed and we dont have to put it back into the queue
                AZ::Uuid dependentSourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(relPath.toUtf8().data());
                QString dependentSourceFilePath = m_platformConfig->FindFirstMatchingFile(relPath);
                QString normalizedFilePath = AssetUtilities::NormalizeFilePath(dependentSourceFilePath).toLower();
                bool fileAlreadyInQueue = (m_sourceDependencyUUIDToSourceNameMap.find(dependentSourceUUID) != m_sourceDependencyUUIDToSourceNameMap.end()) || (m_filesToExamine.find(normalizedFilePath) != m_filesToExamine.end());
                if (!fileAlreadyInQueue)
                {
                    AssessFileInternal(dependentSourceFilePath, false);
                }
            }
        }

        m_AssetProcessorIsBusy = true;

        if (!m_queuedExamination)
        {
            m_queuedExamination = true;
            QTimer::singleShot(0, this, SLOT(ProcessFilesToExamineQueue()));
            Q_EMIT NumRemainingJobsChanged(m_activeFiles.size() + m_filesToExamine.size() + aznumeric_cast<int>(m_jobsToProcessLater.size()));
        }
    }

    void AssetProcessorManager::CheckDeletedProductFile(QString fullProductFile)
    {
        // this might be interesting, but only if its a known product!
        // the dictionary in statedata stores only the relative path, not the platform.
        // which means right now we have, for example
        // d:/game/root/Cache/SamplesProject/IOS/SamplesProject/textures/favorite.tga
        // ^^^^^^^^^^^^  engine root
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ cache root
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ platform root
        {
            QMutexLocker locker(&m_processingJobMutex);
            auto found = m_processingProductInfoList.find(fullProductFile.toLower().toUtf8().data());
            if (found != m_processingProductInfoList.end())
            {
                // if we get here because we just deleted a product file before we copy/move the new product file
                // than its totally safe to ignore this deletion.
                return;
            }
        }
        if (QFile::exists(fullProductFile))
        {
            // this is actually okay - it may have been temporarily deleted because it was in the process of being compiled.
            return;
        }

        //remove the cache root from the cached product path
        QString relativeProductFile = m_cacheRootDir.relativeFilePath(fullProductFile);

        //platform
        QString platform = relativeProductFile;// currently <platform>/<gamename>/<relative_asset_path>
        platform = platform.left(platform.indexOf('/')); // also consume the extra slash - remove PLATFORM

        //we are going to force the processor to re process the source file associated with this product
        //we do that by setting the fingerprint to some other value than which will be recomputed
        //we only want to notify any listeners that the product file was removed for this particular product
        AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
        if (!m_stateData->GetSourcesByProductName(relativeProductFile, sources))
        {
            return;
        }
        AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
        if (!m_stateData->GetJobsByProductName(relativeProductFile, jobs, AZ::Uuid::CreateNull(), QString(), platform))
        {
            return;
        }
        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
        if (!m_stateData->GetProductsByProductName(relativeProductFile, products, AZ::Uuid::CreateNull(), QString(), platform))
        {
            return;
        }

        // pretend that its source changed.  Add it to the things to keep watching so that in case MORE
        // products change. We don't start processing until all have been deleted
        for (auto& source : sources)
        {
            //we should only have one source
            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanfolder;
            if (m_stateData->GetScanFolderByScanFolderID(source.m_scanFolderPK, scanfolder))
            {
                // there's one more thing to account for here, and thats the fact that the sourceName may have an outputPrefix appended on to it
                // so for example, the scan folder might be c:/ly/dev/Gems/Clouds/Assets
                // but the outputPrefix might be Clouds/Assets, meaning "put it in that folder in the cache instead of just at the root"
                // When we have an outputprefix, we prepend it to SourceName so that its a unique source
                // so for example, the sourceName might be Clouds/Assets/blah.tif
                // if you were to blindly concatenate them you'd end up with c:/ly/dev/Gems/Clouds/Assets/Clouds/Assets/blah.tif
                //                                                           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                //                                                              The watch folder is here
                //                                                                                       ^^^^^^^^^^^^^^
                //                                                                                        Prefix prepended
                //                                                                                                      ^^^^^^^^
                //                                                                                                      actual name
                // so remove the output prefix from sourcename if present before doing any source ops on it.

                AZStd::string sourceName(source.m_sourceName);

                if (!scanfolder.m_outputPrefix.empty())
                {
                    sourceName = sourceName.substr(scanfolder.m_outputPrefix.size() + 1);
                }
                AZStd::string fullSourcePath = AZStd::string::format("%s/%s", scanfolder.m_scanFolder.c_str(), sourceName.c_str());

                AssessFileInternal(fullSourcePath.c_str(), false);
            }
        }

        // currently <platform>/<gamename>/<relative_asset_path>
        // remove PLATFORM and GAMENAME so that we only have the relative asset path which should match the db
        QString relativePath(relativeProductFile);
        relativePath = relativePath.right(relativePath.length() - relativePath.indexOf('/') - 1); // also consume the extra slash - remove PLATFORM
        relativePath = relativePath.right(relativePath.length() - relativePath.indexOf('/') - 1); // also consume the extra slash - remove GAMENAME

        //set the fingerprint on the job that made this product
        for (auto& job : jobs)
        {
            for (auto& product : products)
            {
                if (job.m_jobID == product.m_jobPK)
                {
                    //set failed fingerprint
                    job.m_fingerprint = FAILED_FINGERPRINT;

                    // clear it and then queue reprocess on its parent:
                    m_stateData->SetJob(job);

                    // note that over here, we do not notify connected clients that their product has vanished
                    // this is because we have a record of its source file, and it is in the queue for processing.  
                    // Even if the source has disappeared too, that will simply result in the rest of the code
                    // dealing with this issue later when it figures that out.
                    // If the source file is reprocessed and no longer outputs this product, the "AssetProcessed_impl" function will handle notifying
                    // of actually removed products.
                    // If the source file is gone, that will notify for the products right there and then.
                }
            }
        }
    }

    bool AssetProcessorManager::DeleteProducts(const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products)
    {
        bool successfullyRemoved = true;
        // delete the products.
        // products have names like "pc/SamplesProject/textures/blah.dds" and do include platform roots!
        // this means the actual full path is something like
        // [cache root] / [platform] / [product name]
        for (const auto& product : products)
        {
            //get the source for this product
            AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
            if (!m_stateData->GetSourceByProductID(product.m_productID, source))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Source for Product %s not found!!!", product.m_productName.c_str());
            }

            QString fullProductPath = m_cacheRootDir.absoluteFilePath(product.m_productName.c_str());
            QString relativeProductPath(product.m_productName.c_str());
            relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // also consume the extra slash - remove PLATFORM
            relativeProductPath = relativeProductPath.right(relativeProductPath.length() - relativeProductPath.indexOf('/') - 1); // also consume the extra slash - remove GAMENAME

            if (QFile::exists(fullProductPath))
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Deleting file %s because either its source file %s was removed or the builder did not emit this job.\n", fullProductPath.toUtf8().constData(), relativeProductPath.toUtf8().constData());

                successfullyRemoved &= QFile::remove(fullProductPath);

                if (successfullyRemoved)
                {
                    AzToolsFramework::AssetDatabase::JobDatabaseEntry job;
                    if (!m_stateData->GetJobByProductID(product.m_productID, job))
                    {
                        AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find job for Product %s!!!", product.m_productName.c_str());
                    }

                    if (!m_stateData->RemoveProduct(product.m_productID))
                    {
                        AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to remove Product %s!!!", product.m_productName.c_str());
                    }

                    AZ::Data::AssetId assetId(source.m_sourceGuid, product.m_subID);
                    AZ::Data::AssetId legacyAssetId(product.m_legacyGuid, 0);
                    AZ::Data::AssetId legacySourceAssetId(AssetUtilities::CreateSafeSourceUUIDFromName(source.m_sourceName.c_str(), false), product.m_subID);

                    AssetNotificationMessage message(relativeProductPath.toUtf8().constData(), AssetNotificationMessage::AssetRemoved, product.m_assetType);
                    message.m_assetId = assetId;

                    if (legacyAssetId != assetId)
                    {
                        message.m_legacyAssetIds.push_back(legacyAssetId);
                    }

                    if (legacySourceAssetId != assetId)
                    {
                        message.m_legacyAssetIds.push_back(legacySourceAssetId);
                    }
                    Q_EMIT AssetMessage(job.m_platform.c_str(), message);

                    QString parentFolderName = QFileInfo(fullProductPath).absolutePath();
                    m_checkFoldersToRemove.insert(parentFolderName);
                }
            }
            else
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "An expected product %s was not present.\n", fullProductPath.toUtf8().constData());
            }
        }

        return successfullyRemoved;
    }

    void AssetProcessorManager::CheckDeletedSourceFile(QString normalizedPath, QString relativeSourceFile)
    {
        // getting here means an input asset has been deleted
        // and no overrides exist for it.
        // we must delete its products.

        // Check if this file causes any file types to be re-evaluated
        CheckMetaDataRealFiles(relativeSourceFile);

        AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
        if (m_stateData->GetSourcesBySourceName(relativeSourceFile, sources))
        {
            for (const auto& source : sources)
            {
                AzToolsFramework::AssetSystem::JobInfo jobInfo;
                jobInfo.m_sourceFile = relativeSourceFile.toUtf8().constData();

                AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
                if (m_stateData->GetJobsBySourceID(source.m_sourceID, jobs))
                {
                    for (auto& job : jobs)
                    {
                        // ToDo:  Add BuilderUuid here once we do the JobKey feature.
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                        if (m_stateData->GetProductsByJobID(job.m_jobID, products))
                        {
                            if (!DeleteProducts(products))
                            {
                                // try again in a while.  Achieve this by recycling the item back into
                                // the queue as if it had been deleted again.
                                CheckSource(FileEntry(normalizedPath, true));
                                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Delete failed on %s. Will retry!. \n", normalizedPath.toUtf8().constData());
                            }
                        }
                        else
                        {
                            // even with no products, still need to clear the fingerprint:
                            job.m_fingerprint = FAILED_FINGERPRINT;
                            m_stateData->SetJob(job);
                        }

                        // notify the GUI to remove any failed jobs that are currently onscreen:
                        jobInfo.m_platform = job.m_platform;
                        jobInfo.m_jobKey = job.m_jobKey;
                        Q_EMIT JobRemoved(jobInfo);
                    }
                }
                // delete the source from the database too since otherwise it believes we have no products.
                m_stateData->RemoveSource(source.m_sourceID);
               
            }
        }

        Q_EMIT SourceDeleted(relativeSourceFile);  // note that this removes it from the RC Queue Model, also
    }

    void AssetProcessorManager::AddKnownFoldersRecursivelyForFile(QString fullFile, QString root)
    {
        QString normalizedRoot = AssetUtilities::NormalizeFilePath(root);

        // also track parent folders up to the specified root.
        QString parentFolderName = QFileInfo(fullFile).absolutePath();
        QString normalizedParentFolder = AssetUtilities::NormalizeFilePath(parentFolderName);

        if (!normalizedParentFolder.startsWith(normalizedRoot, Qt::CaseInsensitive))
        {
            return; // not interested in folders not in the root.
        }

        while (normalizedParentFolder.compare(normalizedRoot, Qt::CaseInsensitive) != 0)
        {
            m_knownFolders.insert(normalizedParentFolder.toLower());
            int pos = normalizedParentFolder.lastIndexOf(QChar('/'));
            if (pos >= 0)
            {
                normalizedParentFolder = normalizedParentFolder.left(pos);
            }
            else
            {
                break; // no more slashes
            }
        }
    }

    void AssetProcessorManager::CheckMissingJobs(QString relativeSourceFile, const ScanFolderInfo* scanFolder, const AZStd::vector<JobDetails>& jobsThisTime)
    {
        // Check to see if jobs were emitted last time by this builder, but are no longer being emitted this time - in which case we must eliminate old products.
        // whats going to be in the database is fingerprints for each job last time
        // this function is called once per source file, so in the array of jobsThisTime,
        // the relative path will always be the same.

        if ((relativeSourceFile.length()==0) && (jobsThisTime.empty()))
        {
            return;
        }

        // find all jobs from the last time of the platforms that are currently enabled
        JobInfoContainer jobsFromLastTime;
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : scanFolder->GetPlatforms())
        {
            QString platform = QString::fromUtf8(platformInfo.m_identifier.c_str());
            m_stateData->GetJobInfoBySourceName(relativeSourceFile.toUtf8().constData(), jobsFromLastTime, AZ::Uuid::CreateNull(), QString(), platform);
        }
        

        // so now we have jobsFromLastTime and jobsThisTime.  Whats in last time that is no longer being emitted now?
        if (jobsFromLastTime.empty())
        {
            return;
        }

        for (int oldJobIdx = azlossy_cast<int>(jobsFromLastTime.size()) - 1; oldJobIdx >= 0; --oldJobIdx)
        {
            const JobInfo& oldJobInfo = jobsFromLastTime[oldJobIdx];
            // did we find it this time?
            bool foundIt = false;
            for (const JobDetails& newJobInfo : jobsThisTime)
            {
                // the relative path is insensitive because some legacy data didn't have the correct case.
                if ((newJobInfo.m_jobEntry.m_builderGuid == oldJobInfo.m_builderGuid) &&
                    (QString::compare(newJobInfo.m_jobEntry.m_platformInfo.m_identifier.c_str(), oldJobInfo.m_platform.c_str()) == 0) &&
                    (QString::compare(newJobInfo.m_jobEntry.m_jobKey, oldJobInfo.m_jobKey.c_str()) == 0) &&
                    (QString::compare(newJobInfo.m_jobEntry.m_relativePathToFile, oldJobInfo.m_sourceFile.c_str(), Qt::CaseInsensitive) == 0)
                    )
                {
                    foundIt = true;
                    break;
                }
            }

            if (foundIt)
            {
                jobsFromLastTime.erase(jobsFromLastTime.begin() + oldJobIdx);
            }
        }

        // at this point, we contain only the jobs that are left over from last time and not found this time.
        //we want to remove all products for these jobs and the jobs
        for (const JobInfo& oldJobInfo : jobsFromLastTime)
        {
            // ToDo:  Add BuilderUuid here once we do the JobKey feature.
            AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
            if (m_stateData->GetProductsBySourceName(relativeSourceFile, products, oldJobInfo.m_builderGuid, oldJobInfo.m_jobKey.c_str(), oldJobInfo.m_platform.c_str()))
            {
                char tempBuffer[128];
                oldJobInfo.m_builderGuid.ToString(tempBuffer, AZ_ARRAY_SIZE(tempBuffer));

                AZ_TracePrintf(DebugChannel, "Removing products for job (%s, %s, %s, %s, %s) since it is no longer being emitted by its builder.\n",
                    oldJobInfo.m_sourceFile.c_str(),
                    oldJobInfo.m_platform.c_str(),
                    oldJobInfo.m_jobKey.c_str(),
                    oldJobInfo.m_builderGuid.ToString<AZStd::string>().c_str(),
                    tempBuffer);

                //delete products, which should remove them from the disk and database and send the notifications
                DeleteProducts(products);
            }

            //remove the jobs associated with these products
            m_stateData->RemoveJob(oldJobInfo.m_jobID);

            Q_EMIT JobRemoved(oldJobInfo);
        }
    }

    // clean all folders that are empty until you get to the root, or until you get to one that isn't empty.
    void AssetProcessorManager::CleanEmptyFolder(QString folder, QString root)
    {
        QString normalizedRoot = AssetUtilities::NormalizeFilePath(root);

        // also track parent folders up to the specified root.
        QString normalizedParentFolder = AssetUtilities::NormalizeFilePath(folder);
        QDir parentDir(folder);

        // keep walking up the tree until we either run out of folders or hit the root.
        while ((normalizedParentFolder.compare(normalizedRoot, Qt::CaseInsensitive) != 0) && (parentDir.exists()))
        {
            if (parentDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot).empty())
            {
                if (!parentDir.rmdir(normalizedParentFolder))
                {
                    break; // if we fail to remove for any reason we don't push our luck.
                }
            }
            if (!parentDir.cdUp())
            {
                break;
            }
            normalizedParentFolder = AssetUtilities::NormalizeFilePath(parentDir.absolutePath());
        }
    }

    void AssetProcessorManager::CheckModifiedSourceFile(QString normalizedPath, QString relativeSourceFile)
    {
        // a potential input file was modified or added.  We always pass these through our filters and potentially build it.
        // before we know what to do, we need to figure out if it matches some filter we care about.

        // note that if we get here during runtime, we've already eliminated overrides
        // so this is the actual file of importance.

        // check regexes.
        // get list of recognizers which match
        // for each platform in the recognizer:
        //    check the fingerprint and queue if appropriate!
        //    also queue if products missing.

        // Check if this file causes any file types to be re-evaluated
        CheckMetaDataRealFiles(relativeSourceFile);

        const ScanFolderInfo* scanFolder = m_platformConfig->GetScanFolderForFile(normalizedPath);
        if (!scanFolder)
        {
            // not in a folder we care about, either...
            AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckModifiedSourceFile: no scan folder found for %s.\n ", normalizedPath.toUtf8().constData());
            return;
        }

        // keep track of its parent folders so that if a folder disappears or is renamed, and we get the notification that this has occurred
        // we will know that it *was* a folder before now (otherwise we'd have no idea)
        AddKnownFoldersRecursivelyForFile(normalizedPath, scanFolder->ScanPath());

        AssetProcessor::BuilderInfoList builderInfoList;
        EBUS_EVENT(AssetProcessor::AssetBuilderInfoBus, GetMatchingBuildersInfo, normalizedPath.toUtf8().constData(), builderInfoList);

        if (builderInfoList.size())
        {
            ProcessBuilders(normalizedPath, relativeSourceFile, scanFolder, builderInfoList);
        }
    }

    bool AssetProcessorManager::AnalyzeJob(JobDetails& jobDetails, const ScanFolderInfo* scanFolder, bool& sentSourceFileChangedMessage)
    {
        // This function checks to see whether we need to process an asset or not, it returns true if we need to process it and false otherwise
        // It processes an asset if either there is a fingerprint mismatch between the computed and the last known fingerprint or if products are missing
        bool shouldProcessAsset = false;

        // First thing it checks is the computed fingerprint with its last known fingerprint in the database, if there is a mismatch than we need to process it
        AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs; //should only find one when we specify builder, job key, platform
        if (m_stateData->GetJobsBySourceName(jobDetails.m_jobEntry.m_relativePathToFile, jobs, jobDetails.m_jobEntry.m_builderGuid, jobDetails.m_jobEntry.m_jobKey, jobDetails.m_jobEntry.m_platformInfo.m_identifier.c_str()) &&
            jobs[0].m_fingerprint == jobDetails.m_jobEntry.m_computedFingerprint)
        {
            // If the fingerprint hasn't changed, we won't process it.. unless...is it missing a product.
            AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
            if (m_stateData->GetProductsBySourceName(jobDetails.m_jobEntry.m_relativePathToFile, products, jobDetails.m_jobEntry.m_builderGuid, jobDetails.m_jobEntry.m_jobKey, jobDetails.m_jobEntry.m_platformInfo.m_identifier.c_str()))
            {
                for (const auto& product : products)
                {
                    QString fullProductPath = m_cacheRootDir.absoluteFilePath(product.m_productName.c_str());
                    if (!QFile::exists(fullProductPath))
                    {
                        AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckModifiedInputAsset: Missing Products: %s on platform : %s\n", jobDetails.m_jobEntry.m_absolutePathToFile.toUtf8().constData(), product.m_productName.c_str(), jobDetails.m_jobEntry.m_platformInfo.m_identifier.c_str());
                        shouldProcessAsset = true;
                    }
                    else
                    {
                        QString absoluteCacheRoot = m_cacheRootDir.absolutePath();
                        AddKnownFoldersRecursivelyForFile(fullProductPath, absoluteCacheRoot);
                    }
                }
            }
        }
        else
        {
            // fingerprint for this job does not match
            // queue a job to process it by spawning a copy
            AZ_TracePrintf(AssetProcessor::DebugChannel, "AnalyzeJob: FP Mismatch '%s'\n", jobDetails.m_jobEntry.m_platformInfo.m_identifier.c_str());

            shouldProcessAsset = true;
            if (!sentSourceFileChangedMessage)
            {
                QString sourceFile(jobDetails.m_jobEntry.m_relativePathToFile);
                if (!scanFolder->GetOutputPrefix().isEmpty())
                {
                    sourceFile = sourceFile.right(sourceFile.length() - (scanFolder->GetOutputPrefix().length() + 1)); // adding one for separator
                }
                AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(jobDetails.m_jobEntry.m_relativePathToFile.toUtf8().data());
                AzToolsFramework::AssetSystem::SourceFileNotificationMessage message(AZ::OSString(sourceFile.toUtf8().constData()), AZ::OSString(scanFolder->ScanPath().toUtf8().constData()), AzToolsFramework::AssetSystem::SourceFileNotificationMessage::FileChanged, sourceUUID);
                EBUS_EVENT(AssetProcessor::ConnectionBus, Send, 0, message);
                sentSourceFileChangedMessage = true;
            }
        }

        if (!shouldProcessAsset)
        {
            //AZ_TracePrintf(AssetProcessor::DebugChannel, "AnalyzeJob: UpToDate: not processing on %s.\n", jobDetails.m_jobEntry.m_platform.toUtf8().constData());
            return false;
        }
        else
        {
            // we need to actually run the processor on this asset unless its already in the queue.
            //AZ_TracePrintf(AssetProcessor::DebugChannel, "AnalyzeJob: Needs processing on %s.\n", jobDetails.m_jobEntry.m_platform.toUtf8().constData());
            // no need to give the above text, since the earlier traceprintf will already cover it.

            // macOS requires that the cacheRootDir to not be all lowercase, otherwise file copies will not work correctly.
            // So use the lowerCasePath string to capture the parts that need to be lower case while keeping the cache root
            // mixed case.
            QString lowerCasePath = jobDetails.m_jobEntry.m_platformInfo.m_identifier.c_str();
            QString pathRel = QString("/") + QFileInfo(jobDetails.m_jobEntry.m_relativePathToFile).path();

            if (pathRel == "/.")
            {
                // if its in the current folder, avoid using ./ or /.
                pathRel = QString();
            }

            if (scanFolder->IsRoot())
            {
                // stuff which is found in the root continues to go to the root, rather than GAMENAME folder...
                lowerCasePath += pathRel;
            }
            else
            {
                lowerCasePath += "/" + AssetUtilities::ComputeGameName() + pathRel;
            }

            lowerCasePath = lowerCasePath.toLower();
            jobDetails.m_destinationPath = m_cacheRootDir.absoluteFilePath(lowerCasePath);
        }

        return true;
    }

    void AssetProcessorManager::CheckDeletedCacheFolder(QString normalizedPath)
    {
        QDir checkDir(normalizedPath);
        if (checkDir.exists())
        {
            // this is possible because it could have been moved back by the time we get here, in which case, we take no action.
            return;
        }

        // going to need to iterate on all files there, recursively, in order to emit them as having been deleted.
        // note that we don't scan here.  We use the asset database.
        QString cacheRootRemoved = m_cacheRootDir.relativeFilePath(normalizedPath);

        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
        m_stateData->GetProductsLikeProductName(cacheRootRemoved, AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, products);

        for (const auto& product : products)
        {
            QString fileFound = m_cacheRootDir.absoluteFilePath(product.m_productName.c_str());
            if (!QFile::exists(fileFound))
            {
                AssessDeletedFile(fileFound);
            }
        }

        m_knownFolders.remove(normalizedPath.toLower());
    }

    void AssetProcessorManager::CheckDeletedSourceFolder(QString normalizedPath, QString relativePath, const ScanFolderInfo* scanFolderInfo)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckDeletedSourceFolder...\n");
        // we deleted a folder that is somewhere that is a watched input folder.

        QDir checkDir(normalizedPath);
        if (checkDir.exists())
        {
            // this is possible because it could have been moved back by the time we get here, in which case, we take no action.
            return;
        }

        AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
        m_stateData->GetSourcesLikeSourceName(relativePath, AzToolsFramework::AssetDatabase::AssetDatabaseConnection::StartsWith, sources);

        AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckDeletedSourceFolder: %i matching files.\n", sources.size());

        QDir scanFolder(scanFolderInfo->ScanPath());
        for (const auto& source : sources)
        {
            // reconstruct full path:
            QString actualRelativePath = source.m_sourceName.c_str();

            if (!scanFolderInfo->GetOutputPrefix().isEmpty())
            {
                actualRelativePath = actualRelativePath.right(actualRelativePath.length() - (scanFolderInfo->GetOutputPrefix().length() + 1)); // adding one for separator
            }

            QString finalPath = scanFolder.absoluteFilePath(actualRelativePath);

            if (!QFile::exists(finalPath))
            {
                AssessDeletedFile(finalPath);
            }
        }

        m_knownFolders.remove(normalizedPath.toLower());
    }

    namespace
    {
        void ScanFolderInternal(QString inputFolderPath, QStringList& outputs)
        {
            QDir inputFolder(inputFolderPath);
            QFileInfoList entries = inputFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);

            for (const QFileInfo& entry : entries)
            {
                if (entry.isDir())
                {
                    //Entry is a directory
                    ScanFolderInternal(entry.absoluteFilePath(), outputs);
                }
                else
                {
                    //Entry is a file
                    outputs.push_back(entry.absoluteFilePath());
                }
            }
        }
    }

    void AssetProcessorManager::CheckMetaDataRealFiles(QString relativeSourceFile)
    {
        if (!m_platformConfig->IsMetaDataTypeRealFile(relativeSourceFile))
        {
            return;
        }

        QStringList extensions;
        for (int idx = 0; idx < m_platformConfig->MetaDataFileTypesCount(); idx++)
        {
            const auto& metaExt = m_platformConfig->GetMetaDataFileTypeAt(idx);
            if (!metaExt.second.isEmpty() && QString::compare(metaExt.first, relativeSourceFile, Qt::CaseInsensitive) == 0)
            {
                extensions.push_back(metaExt.second);
            }
        }

        AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sources;
        for (const auto& ext : extensions)
        {
            m_stateData->GetSourcesLikeSourceName(ext, AzToolsFramework::AssetDatabase::AssetDatabaseConnection::EndsWith, sources);
        }

        QString fullMatchingSourceFile;
        for (const auto& source : sources)
        {
            fullMatchingSourceFile = m_platformConfig->FindFirstMatchingFile(source.m_sourceName.c_str());
            if (!fullMatchingSourceFile.isEmpty())
            {
                AssessFileInternal(fullMatchingSourceFile, false);
            }
        }
    }

    void AssetProcessorManager::CheckCreatedSourceFolder(QString fullSourceFile)
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "CheckCreatedSourceFolder...\n");
        // this could have happened because its a directory rename
        QDir checkDir(fullSourceFile);
        if (!checkDir.exists())
        {
            // this is possible because it could have been moved back by the time we get here.
            // find all assets that are products that have this as their normalized path and then indicate that they are all deleted.
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Directory (%s) does not exist.\n", fullSourceFile.toUtf8().data());
            return;
        }

        // we actually need to scan this folder, without invoking the whole asset scanner:

        const AssetProcessor::ScanFolderInfo* info = m_platformConfig->GetScanFolderForFile(fullSourceFile);
        if (!info)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "No scan folder found for the directory: (%s).\n", fullSourceFile.toUtf8().data());
            return; // early out, its nothing we care about.
        }

        QStringList files;
        ScanFolderInternal(fullSourceFile, files);

        for (const QString fileEntry : files)
        {
            AssessModifiedFile(fileEntry);
        }
    }

    void AssetProcessorManager::ProcessFilesToExamineQueue()
    {
        // it is assumed that files entering this function are already normalized
        // that is, the path is normalized
        // and only has forward slashes.

        if (!m_platformConfig)
        {
            // this cannot be recovered from
            qFatal("Platform config is missing, we cannot continue.");
            return;
        }

        if ((m_normalizedCacheRootPath.isEmpty()) && (!InitializeCacheRoot()))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Cannot examine the queue yet - cache root is not ready!\n ");
            m_queuedExamination = true;
            QTimer::singleShot(250, this, SLOT(ProcessFilesToExamineQueue()));
            return;
        }

        if (m_isCurrentlyScanning)
        {
            // if we're currently scanning, then don't start processing yet, its not worth the IO thrashing.
            m_queuedExamination = true;
            QTimer::singleShot(250, this, SLOT(ProcessFilesToExamineQueue()));
            return;
        }

        FileExamineContainer swapped;
        m_filesToExamine.swap(swapped); // makes it okay to call CheckSource(...)

        QElapsedTimer elapsedTimer;
        elapsedTimer.start();

        int i = -1; // Starting at -1 so we can increment at the start of the loop instead of the end due to all the control flow that occurs inside the loop
        m_queuedExamination = false;
        for (const FileEntry& examineFile : swapped)
        {
            ++i;

            if (m_quitRequested)
            {
                return;
            }

            // CreateJobs can sometimes take a very long time, update the remaining count occasionally
            if (elapsedTimer.elapsed() >= MILLISECONDS_BETWEEN_CREATE_JOBS_STATUS_UPDATE)
            {
                int remainingInSwapped = swapped.size() - i;
                Q_EMIT NumRemainingJobsChanged(m_activeFiles.size() + remainingInSwapped + aznumeric_cast<int>(m_jobsToProcessLater.size()));
                elapsedTimer.restart();
            }

            // examination occurs here.
            // first, is it a source or is it a product in the cache folder?
            QString normalizedPath = examineFile.m_fileName;

            AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessFilesToExamineQueue: %s delete: %s.\n", examineFile.m_fileName.toUtf8().constData(), examineFile.m_isDelete ? "true" : "false");

            // debug-only check to make sure our assumption about normalization is correct.
            Q_ASSERT(normalizedPath == AssetUtilities::NormalizeFilePath(normalizedPath));

            // if its in the cache root then its a product file:
            bool isProductFile = examineFile.m_fileName.startsWith(m_normalizedCacheRootPath, Qt::CaseInsensitive);

            // strip the engine off it so that its a "normalized asset path" with appropriate slashes and such:
            if (isProductFile)
            {
                // its a product file.
                if (normalizedPath.length() >= AP_MAX_PATH_LEN)
                {
                    // if we are here it means that we have found a cache file whose filepath is greater than the maximum path length allowed
                    continue;
                }

                // we only care about deleted product files.
                if (examineFile.m_isDelete)
                {
                    if (normalizedPath.endsWith(QString(FENCE_FILE_EXTENSION), Qt::CaseInsensitive))
                    //if(m_knownFolders.contains(normalizedPath.toLower()))
                    {
                        QFileInfo fileInfo(normalizedPath);
                        if (fileInfo.isDir())
                        {
                            continue;
                        }
                        // its a fence file, now computing fenceId from it
                        int startPos = normalizedPath.lastIndexOf("~");
                        int endPos = normalizedPath.lastIndexOf(".");
                        QString fenceIdString = normalizedPath.mid(startPos + 1, endPos - startPos - 1);
                        bool isNumber = false;
                        int fenceId = fenceIdString.toInt(&isNumber);
                        if (isNumber)
                        {
                            Q_EMIT FenceFileDetected(fenceId);
                        }
                        else
                        {
                            AZ_TracePrintf(AssetProcessor::DebugChannel, "AssetProcessor: Unable to compute fenceId from fenceFile name %s.\n", normalizedPath.toUtf8().data());
                        }
                        continue;
                    }
                    if (m_knownFolders.contains(normalizedPath.toLower()))
                    {
                        CheckDeletedCacheFolder(normalizedPath);
                    }
                    else
                    {
                        AZ_TracePrintf(AssetProcessor::DebugChannel, "Cached file deleted\n");
                        CheckDeletedProductFile(normalizedPath);
                    }
                }
                else
                {
                    // a file was added or modified to the cache.
                    // we only care about the renames of folders, so cache folders here:
                    QFileInfo fileInfo(normalizedPath);
                    if (!fileInfo.isDir())
                    {
                        // keep track of its containing folder.
                        AddKnownFoldersRecursivelyForFile(normalizedPath, m_cacheRootDir.absolutePath());
                    }
                }
            }
            else
            {
                // its an source file.  check which scan folder it belongs to
                QString scanFolderName;
                QString relativePathToFile;
                if (!m_platformConfig->ConvertToRelativePath(normalizedPath, relativePathToFile, scanFolderName))
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessFilesToExamineQueue: Unable to find the relative path.\n");
                    continue;
                }

                if (normalizedPath.length() >= AP_MAX_PATH_LEN)
                {
                    // if we are here it means that we have found a source file whose filepath is greater than the maximum path length allowed
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "ProcessFilesToExamineQueue: %s filepath length %d exceeds the maximum path length (%d) allowed.\n", normalizedPath.toUtf8().constData(), normalizedPath.length(), AP_MAX_PATH_LEN);

                    JobInfoContainer jobInfos;
                    m_stateData->GetJobInfoBySourceName(relativePathToFile, jobInfos);

                    JobDetails job;
                    for (const auto& jobInfo : jobInfos)
                    {
                        const AssetBuilderSDK::PlatformInfo* const platformFromInfo = m_platformConfig->GetPlatformByIdentifier(jobInfo.m_platform.c_str());
                        AZ_Assert(platformFromInfo, "Error - somehow a job was created which was for a platform not in config.");
                        if (platformFromInfo)
                        {
                            job.m_jobEntry = JobEntry(normalizedPath, relativePathToFile, jobInfo.m_builderGuid, *platformFromInfo, jobInfo.m_jobKey.c_str(), 0, GenerateNewJobRunKey(), AZ::Uuid::CreateNull());

                            job.m_autoFail = true;
                            job.m_jobParam[AZ_CRC(AutoFailReasonKey)] = AZStd::string::format("Product file name would be too long: %s\n", normalizedPath.toUtf8().data());

                            Q_EMIT AssetToProcess(job);// forwarding this job to rccontroller to fail it
                        }
                    }

                    continue;
                }

                if (examineFile.m_isDelete)
                {
                    // if its a delete for a known input folder, we handle it differently.
                    if (m_knownFolders.contains(normalizedPath.toLower()))
                    {
                        const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderForFile(normalizedPath);

                        CheckDeletedSourceFolder(normalizedPath, relativePathToFile, scanFolderInfo);
                        continue;
                    }
                }
                else
                {
                    // a file was added to the source folder.
                    QFileInfo fileInfo(normalizedPath);
                    if (!fileInfo.isDir())
                    {
                        // keep track of its containing folder.
                        m_knownFolders.insert(AssetUtilities::NormalizeFilePath(fileInfo.absolutePath()).toLower());
                    }

                    if (fileInfo.isDir())
                    {
                        // if its a dir, we need to keep track of that too (no need to go recursive here since we will encounter this via a scan).
                        m_knownFolders.insert(normalizedPath.toLower());
                        // we actually need to scan this folder now...
                        CheckCreatedSourceFolder(normalizedPath);
                        continue;
                    }
                }

                // is it being overridden by a higher priority file?
                QString overrider;
                if (examineFile.m_isDelete)
                {
                    // if we delete it, check if its revealed by an underlying file:
                    overrider = m_platformConfig->FindFirstMatchingFile(relativePathToFile);

                    // if the overrider is the same file, it means that a file was deleted, then reappeared.
                    // if that happened there will be a message in the notification queue for that file reappearing, there
                    // is no need to add a double here.
                    if (overrider.compare(normalizedPath, Qt::CaseInsensitive) == 0)
                    {
                        overrider.clear();
                    }
                }
                else
                {
                    overrider = m_platformConfig->GetOverridingFile(relativePathToFile, scanFolderName);
                }

                if (!overrider.isEmpty())
                {
                    // this file is being overridden by an earlier file.
                    // ignore us, and pretend the other file changed:
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "File overridden by %s.\n", overrider.toUtf8().constData());
                    CheckSource(FileEntry(overrider, false));
                    continue;
                }

                // its an input file or a file we don't care about...
                // note that if the file now exists, we have to treat it as an input asset even if it came in as a delete.
                if (examineFile.m_isDelete && !QFile::exists(examineFile.m_fileName))
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Input was deleted and no overrider was found.\n");
                    const AssetProcessor::ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderForFile(normalizedPath);
                    QString sourceFile(relativePathToFile);
                    if (!scanFolderInfo->GetOutputPrefix().isEmpty())
                    {
                        sourceFile = sourceFile.right(sourceFile.length() - (scanFolderInfo->GetOutputPrefix().length() + 1)); // adding one for separator
                    }
                    AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(relativePathToFile.toUtf8().data());
                    AzToolsFramework::AssetSystem::SourceFileNotificationMessage message(AZ::OSString(sourceFile.toUtf8().constData()), AZ::OSString(scanFolderInfo->ScanPath().toUtf8().constData()), AzToolsFramework::AssetSystem::SourceFileNotificationMessage::FileRemoved, sourceUUID);
                    EBUS_EVENT(AssetProcessor::ConnectionBus, Send, 0, message);
                    CheckDeletedSourceFile(normalizedPath, relativePathToFile);
                }
                else
                {
                    // log-spam-reduction - the lack of the prior tag (input was deleted) which is rare can infer that the above branch was taken
                    //AZ_TracePrintf(AssetProcessor::DebugChannel, "Input is modified or is overriding something.\n");
                    CheckModifiedSourceFile(normalizedPath, relativePathToFile);
                }
            }
        }

        // instead of checking here, we place a message at the end of the queue.
        // this is because there may be additional scan or other results waiting in the queue
        // an example would be where the scanner found additional "copy" jobs waiting in the queue for finalization
        QMetaObject::invokeMethod(this, "CheckForIdle", Qt::QueuedConnection);
    }

    void AssetProcessorManager::CheckForIdle()
    {
        if (IsIdle())
        {
            if (!m_hasProcessedCriticalAssets)
            {
                // only once, when we finish startup
                m_stateData->VacuumAndAnalyze();
                m_hasProcessedCriticalAssets = true;
            }

            if (!m_quitRequested && m_AssetProcessorIsBusy)
            {
                m_AssetProcessorIsBusy = false;
                m_jobsToProcessLater.clear();
                Q_EMIT NumRemainingJobsChanged(m_activeFiles.size() + m_filesToExamine.size() + aznumeric_cast<int>(m_jobsToProcessLater.size()));
                Q_EMIT AssetProcessorManagerIdleState(true);
            }

            QTimer::singleShot(20, this, SLOT(RemoveEmptyFolders()));
        }
        else
        {
            m_AssetProcessorIsBusy = true;
            Q_EMIT AssetProcessorManagerIdleState(false);
            Q_EMIT NumRemainingJobsChanged(m_activeFiles.size() + m_filesToExamine.size() + aznumeric_cast<int>(m_jobsToProcessLater.size()));

            // wake up if there's work to do and we haven't scheduled to do it.
            if ((!m_alreadyScheduledUpdate) && ((!m_filesToExamine.isEmpty()) || (!m_activeFiles.isEmpty())))
            {
                // schedule additional updates
                m_alreadyScheduledUpdate = true;
                QTimer::singleShot(1, this, SLOT(ScheduleNextUpdate()));
            }
            else if (!m_jobsToProcessLater.empty() && !m_alreadyScheduledUpdate)
            {
                if (m_SourceDependencyInfoNeedsUpdate)
                {
                    //Resolve any source dependency uuid's to source name
                    UpdateSourceFileDependencyInfo();
                    UpdateSourceFileDependencyDatabase();
                    m_sourceFileDependencyInfoMap.clear();
                    m_dependsOnSourceToSourceMap.clear();
                    m_dependsOnSourceUuidToSourceMap.clear();
                    m_SourceDependencyInfoNeedsUpdate = false;
                    m_sourceDependencyUUIDToSourceNameMap.clear();
                }


                // Process jobs which we have stored for later processing
                for (JobToProcessEntry& entry : m_jobsToProcessLater)
                {
                    if (entry.m_jobsToAnalyze.size())
                    {
                        //Update source dependency list before forwarding the job to RCController
                        AnalyzeJobDetail(entry);

                        ProcessJobs(entry.m_sourceFileInfo.m_relativePath, entry.m_jobsToAnalyze, entry.m_sourceFileInfo.m_scanFolder);
                    }
                }
                m_jobsToProcessLater.clear();
                
                QTimer::singleShot(1, this, SLOT(CheckForIdle()));
            }
        }
    }

    // ----------------------------------------------------
    // ------------- File change Queue --------------------
    // ----------------------------------------------------
    void AssetProcessorManager::AssessFileInternal(QString fullFile, bool isDelete)
    {
        if (m_quitRequested)
        {
            return;
        }

        QString normalizedFullFile = AssetUtilities::NormalizeFilePath(fullFile);
        if (m_platformConfig->IsFileExcluded(normalizedFullFile))
        {
            return;
        }

        m_AssetProcessorIsBusy = true;
        Q_EMIT AssetProcessorManagerIdleState(false);

        AZ_TracePrintf(AssetProcessor::DebugChannel, "AssesFileInternal: %s %s\n", normalizedFullFile.toUtf8().constData(), isDelete ? "true" : "false");

        // this function is the raw function that gets called from the file monitor
        // whenever an asset has been modified or added (not deleted)
        // it should place the asset on a grace period list and not considered until changes stop happening to it.
        // note that file Paths come in raw, full absolute paths.

        if (!m_SourceFilesInDatabase.empty())
        {
            QString loweredFullFile = normalizedFullFile.toLower();
            m_SourceFilesInDatabase.remove(loweredFullFile);
        }

        FileEntry newEntry(normalizedFullFile, isDelete);

        if (m_alreadyActiveFiles.find(fullFile) != m_alreadyActiveFiles.end())
        {
            auto entryIt = std::find_if(m_activeFiles.begin(), m_activeFiles.end(),
                    [&normalizedFullFile](const FileEntry& entry)
                    {
                        return entry.m_fileName == normalizedFullFile;
                    });

            if (entryIt != m_activeFiles.end())
            {
                m_activeFiles.erase(entryIt);
            }
        }

        m_AssetProcessorIsBusy = true;
        m_activeFiles.push_back(newEntry);
        m_alreadyActiveFiles.insert(fullFile);

        Q_EMIT NumRemainingJobsChanged(m_activeFiles.size() + m_filesToExamine.size() + aznumeric_cast<int>(m_jobsToProcessLater.size()));

        if (!m_alreadyScheduledUpdate)
        {
            m_alreadyScheduledUpdate = true;
            QTimer::singleShot(1, this, SLOT(ScheduleNextUpdate()));
        }
    }


    void AssetProcessorManager::AssessAddedFile(QString filePath)
    {
        if (filePath.startsWith(m_normalizedCacheRootPath, Qt::CaseInsensitive))
        {
            // modifies/adds to the cache are irrelevant.  Deletions are all we care about
            return;
        }

        AssessFileInternal(filePath, false);
    }

    void AssetProcessorManager::AssessModifiedFile(QString filePath)
    {
        // we don't care about modified folders at this time.
        // you'll get a "folder modified" whenever a file in a folder is removed or added or modified
        // but you'll also get the actual file modify itself.
        if (!QFileInfo(filePath).isDir())
        {
            // we also don't care if you modify files in the cache, only deletions matter.
            if (!filePath.startsWith(m_normalizedCacheRootPath, Qt::CaseInsensitive))
            {
                AssessFileInternal(filePath, false);
            }
        }
    }

    void AssetProcessorManager::AssessDeletedFile(QString filePath)
    {
        AssessFileInternal(filePath, true);
    }

    void AssetProcessorManager::ScheduleNextUpdate()
    {
        m_alreadyScheduledUpdate = false;
        if (m_activeFiles.size() > 0)
        {
            DispatchFileChange();
        }
        else
        {
            QMetaObject::invokeMethod(this, "CheckForIdle", Qt::QueuedConnection);
        }
    }

    void AssetProcessorManager::RemoveEmptyFolders()
    {
        if (!m_AssetProcessorIsBusy)
        {
            if (m_checkFoldersToRemove.size())
            {
                QString dir = *m_checkFoldersToRemove.begin();
                CleanEmptyFolder(dir, m_normalizedCacheRootPath);
                m_checkFoldersToRemove.remove(dir);
                QTimer::singleShot(20, this, SLOT(RemoveEmptyFolders()));
            }
        }
    }

    void AssetProcessorManager::DispatchFileChange()
    {
        Q_ASSERT(m_activeFiles.size() > 0);

        if (m_quitRequested)
        {
            return;
        }

        // This was added because we found out that the consumer was not able to keep up, which led to the app taking forever to shut down
        // we want to make sure that our queue has at least this many to eat in a single gulp, so it remains busy, but we cannot let this number grow too large
        // or else it never returns to the main message pump and thus takes a while to realize that quit has been signalled.
        // if the processing thread ever runs dry, then this needs to be increased.
        int maxPerIteration = 50;

        // Burn through all pending files
        const FileEntry* firstEntry = &m_activeFiles.front();
        while (m_filesToExamine.size() < maxPerIteration)
        {
            m_alreadyActiveFiles.remove(firstEntry->m_fileName);
            CheckSource(*firstEntry);
            m_activeFiles.pop_front();

            if (m_activeFiles.size() == 0)
            {
                break;
            }
            firstEntry = &m_activeFiles.front();
        }

        if (!m_alreadyScheduledUpdate)
        {
            // schedule additional updates
            m_alreadyScheduledUpdate = true;
            QTimer::singleShot(1, this, SLOT(ScheduleNextUpdate()));
        }
    }


    bool AssetProcessorManager::IsIdle()
    {
        if ((!m_queuedExamination) && (m_filesToExamine.isEmpty()) && (m_activeFiles.isEmpty()) && 
            !m_processedQueued && m_assetProcessedList.empty() && m_jobsToProcessLater.empty())
        {
            return true;
        }

        return false;
    }

    bool AssetProcessorManager::HasProcessedCriticalAssets() const
    {
        return m_hasProcessedCriticalAssets;
    }

    void AssetProcessorManager::ProcessJobs(QString relativePathToFile, AZStd::vector<JobDetails>& jobsToAnalyze, const ScanFolderInfo* scanFolder)
    {
        bool sentSourceFileChangedMessage = false;

        CheckMissingJobs(relativePathToFile, scanFolder, jobsToAnalyze);

        for (JobDetails& newJob : jobsToAnalyze)
        {
            //Populate all the files needed for fingerprinting of this job
            PopulateFilesForFingerprinting(newJob);
            // Check the current builder jobs with the previous ones in the database
            newJob.m_jobEntry.m_computedFingerprint = AssetUtilities::GenerateFingerprint(newJob);
            newJob.m_jobEntry.m_computedFingerprintTimeStamp = QDateTime::currentMSecsSinceEpoch();
            if (newJob.m_jobEntry.m_computedFingerprint == 0)
            {
                // unable to fingerprint this file.
                AZ_TracePrintf(AssetProcessor::DebugChannel, "ProcessBuilders: Unable to fingerprint for platform: %s.\n", newJob.m_jobEntry.m_platformInfo.m_identifier.c_str());
                continue;
            }

            // Check to see whether we need to process this asset
            if (AnalyzeJob(newJob, scanFolder, sentSourceFileChangedMessage))
            {
                Q_EMIT AssetToProcess(newJob);
            }
        }
    }

    void AssetProcessorManager::ProcessCreateJobsResponse(AssetBuilderSDK::CreateJobsResponse& createJobsResponse, const AssetBuilderSDK::CreateJobsRequest& createJobsRequest)
    {
        m_SourceDependencyInfoNeedsUpdate = true;
        QString relativeFilePath(createJobsRequest.m_sourceFile.c_str());
        //If any source file does not create any jobs we still need to store it in this map so that AP is aware of this source file and help us resolve guids into file names
        AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(relativeFilePath.toUtf8().data());
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);
            m_sourceDependencyUUIDToSourceNameMap.insert(AZStd::make_pair(sourceUUID, relativeFilePath.toUtf8().data()));
        }

        if (createJobsResponse.m_sourceFileDependencyList.size())
        {
            //store source dependency info
            for (AssetBuilderSDK::SourceFileDependency& sourceFileDependencyEntry : createJobsResponse.m_sourceFileDependencyList)
            {
                AssetProcessor::SourceFileDependencyInternal sourceFileDependencyInternal;
                sourceFileDependencyInternal.m_relativeSourcePath = AssetUtilities::NormalizeFilePath(createJobsRequest.m_sourceFile.c_str()).toUtf8().data();
                sourceFileDependencyInternal.m_sourceWatchFolder = createJobsRequest.m_watchFolder;
                sourceFileDependencyInternal.m_builderId = createJobsRequest.m_builderid;
                sourceFileDependencyInternal.m_sourceUUID = sourceUUID;
                sourceFileDependencyInternal.m_sourceFileDependency = sourceFileDependencyEntry;
                QString sourceFileDependencyPath(sourceFileDependencyEntry.m_sourceFileDependencyPath.c_str());
                QFileInfo fileInfo(AssetUtilities::NormalizeFilePath(sourceFileDependencyPath));
                if (!sourceFileDependencyEntry.m_sourceFileDependencyPath.empty())
                {
                    if (fileInfo.isRelative())
                    {
                        QString filePath = fileInfo.filePath();
                        // if it is a relative path, resolve it to a full path with the first matching file
                        QString absoluteSourceFileDependencyPath = m_platformConfig->FindFirstMatchingFile(filePath);
                        if (absoluteSourceFileDependencyPath.isEmpty())
                        {
                            // If we are here it means we were unable to find the source dependency file, 
                            // The last thing we can try to locate the file is to assume that the dependency is in the same folder as the source file 
                            QString absoluteSourceFilePath = m_platformConfig->FindFirstMatchingFile(createJobsRequest.m_sourceFile.c_str());
                            QFileInfo sourceFileInfo(absoluteSourceFilePath);
                            QString sourceFileName = sourceFileInfo.fileName();
                            absoluteSourceFileDependencyPath = sourceFileInfo.absoluteDir().filePath(filePath);
                            if (!QFile::exists(absoluteSourceFileDependencyPath))
                            {
                                AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to find the source dependency file %s for source file %s.\n", sourceFileDependencyEntry.m_sourceFileDependencyPath.c_str(), relativeFilePath.toUtf8().data());
                                continue;
                            }
                        }
                        
                        // if we get here, we have the file info we need in order to proceed
                        fileInfo.setFile(absoluteSourceFileDependencyPath);
                    }
                }
                else
                {
                    // if we are here it means that AP is not aware of any source file having this uuid at this moment,
                    // we will search once again once all the create jobs are complete
                    m_sourceFileDependencyInfoMap.insert(AZStd::make_pair(relativeFilePath.toUtf8().data(), sourceFileDependencyInternal));
                    m_dependsOnSourceUuidToSourceMap.insert(AZStd::make_pair(sourceFileDependencyEntry.m_sourceFileDependencyUUID, sourceFileDependencyInternal.m_relativeSourcePath.c_str()));
                    continue;
                }

                QString relativePath;
                QString watchFolder;

                if (m_platformConfig->ConvertToRelativePath(fileInfo.filePath(), relativePath, watchFolder))
                {
                    sourceFileDependencyInternal.m_sourceFileDependency.m_sourceFileDependencyPath = fileInfo.filePath().toUtf8().data();
                    sourceFileDependencyInternal.m_relativeSourceFileDependencyPath = relativePath.toUtf8().data();
                    m_sourceFileDependencyInfoMap.insert(AZStd::make_pair(relativeFilePath.toUtf8().data(), sourceFileDependencyInternal));
                    m_dependsOnSourceToSourceMap.insert(AZStd::make_pair(relativePath.toUtf8().data(), sourceFileDependencyInternal.m_relativeSourcePath.c_str()));
                }
                else
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to find the source dependency file %s for source file %s.\n", sourceFileDependencyEntry.m_sourceFileDependencyPath.c_str(), relativeFilePath.toUtf8().data());
                    continue;
                }
            }
        }
    }

    void AssetProcessorManager::ProcessBuilders(QString normalizedPath, QString relativePathToFile, const ScanFolderInfo* scanFolder, const AssetProcessor::BuilderInfoList& builderInfoList)
    {

        // Go through the builder list in 2 phases:
        //  1.  Collect all the jobs
        //  2.  Dispatch all the jobs

        bool runJobLater = false;
        JobToProcessEntry entry;
        AZStd::vector<JobDetails> jobsToAnalyze;

        AZ::Uuid sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(relativePathToFile.toUtf8().constData());

        // Phase 1 : Collect all the jobs and responses
        for (const AssetBuilderSDK::AssetBuilderDesc& builderInfo : builderInfoList)
        {
            // If the builder's bus ID is null, then avoid processing (this should not happen)
            if (builderInfo.m_busId.IsNull())
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Skipping builder %s, no builder bus id defined.\n", builderInfo.m_name.data());
                continue;
            }

            QString builderVersionString = QString::number(builderInfo.m_version);

            // note that the relative path to file contains the output prefix since thats our data storage format in our database
            // however, that is an internal detail we do not want to expose to builders.
            // instead, we strip it out, before we send the request and if necessary, put it back after.
            QString actualRelativePath = relativePathToFile;
            if (!scanFolder->GetOutputPrefix().isEmpty())
            {
                actualRelativePath = actualRelativePath.remove(0, static_cast<int>(scanFolder->GetOutputPrefix().length()) + 1);
            }

            const AssetBuilderSDK::CreateJobsRequest createJobsRequest(builderInfo.m_busId, actualRelativePath.toUtf8().constData(), scanFolder->ScanPath().toUtf8().constData(), scanFolder->GetPlatforms(), sourceUUID);

            AssetBuilderSDK::CreateJobsResponse createJobsResponse;

            // Wrap with a log listener to redirect logging to a job specific log file and then send job request to the builder
            AZ::s64 runKey = GenerateNewJobRunKey();
            AssetProcessor::SetThreadLocalJobId(runKey);

            AZStd::string logFileName = AssetUtilities::ComputeJobLogFileName(createJobsRequest);
            {
                AssetUtilities::JobLogTraceListener jobLogTraceListener(logFileName, runKey, true);
                builderInfo.m_createJobFunction(createJobsRequest, createJobsResponse);
            }
            AssetProcessor::SetThreadLocalJobId(0);

            if (createJobsResponse.m_result == AssetBuilderSDK::CreateJobsResultCode::Failed)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Createjobs Failed: %s.\n", normalizedPath.toUtf8().constData());

                AZStd::string fullPathToLogFile = AssetUtilities::ComputeJobLogFolder();
                fullPathToLogFile += "/";
                fullPathToLogFile += logFileName.c_str();
                char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };

                AZ::IO::FileIOBase::GetInstance()->ResolvePath(fullPathToLogFile.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);

                JobDetails jobdetail;
                jobdetail.m_jobEntry = JobEntry(normalizedPath, actualRelativePath, builderInfo.m_busId, { "all", {} }, QString("CreateJobs_%1").arg(builderInfo.m_busId.ToString<AZStd::string>().c_str()), 0, runKey, sourceUUID);
                jobdetail.m_autoFail = true;
                jobdetail.m_critical = true;
                jobdetail.m_priority = INT_MAX; // front of the queue.

                // try reading the log yourself.
                AssetJobLogResponse response;
                jobdetail.m_jobParam[AZ_CRC(AutoFailReasonKey)] = AZStd::string::format(
                    "CreateJobs of %s has failed.\n"
                    "This is often because the asset is corrupt.\n"
                    "Please load it in the editor to see what might be wrong.\n",
                    actualRelativePath.toUtf8().data());
                
                AssetUtilities::ReadJobLog(resolvedBuffer, response);
                jobdetail.m_jobParam[AZ_CRC(AutoFailLogFile)].swap(response.m_jobLog);
                jobdetail.m_jobParam[AZ_CRC(AutoFailOmitFromDatabaseKey)] = "true"; // omit this job from the database.

                Q_EMIT AssetToProcess(jobdetail);// forwarding this job to rccontroller to fail it

                continue;
            }
            else if (createJobsResponse.m_result == AssetBuilderSDK::CreateJobsResultCode::ShuttingDown)
            {
                return;
            }
            else
            {
                // if we get here, we succeeded.
                {
                    // if we succeeded, we can erase any jobs that had failed createjobs last time for this builder:
                    AzToolsFramework::AssetSystem::JobInfo jobInfo;
                    jobInfo.m_sourceFile = relativePathToFile.toUtf8().constData();
                    jobInfo.m_platform = "all";
                    jobInfo.m_jobKey = AZStd::string::format("CreateJobs_%s", builderInfo.m_busId.ToString<AZStd::string>().c_str());
                    Q_EMIT JobRemoved(jobInfo);
                }

                for (AssetBuilderSDK::JobDescriptor& jobDescriptor : createJobsResponse.m_createJobOutputs)
                {
                    const AssetBuilderSDK::PlatformInfo* const infoForPlatform = m_platformConfig->GetPlatformByIdentifier(jobDescriptor.GetPlatformIdentifier().c_str());
                    AZ_Assert(infoForPlatform, "Somehow, a platform for a job was created in createjobs which cannot be found in the list of enabled platforms.");
                    if (infoForPlatform)
                    {
                        JobDetails newJob;
                        newJob.m_assetBuilderDesc = builderInfo;
                        newJob.m_critical = jobDescriptor.m_critical;
                        newJob.m_extraInformationForFingerprinting = builderVersionString + QString(jobDescriptor.m_additionalFingerprintInfo.c_str());
                        newJob.m_jobEntry = JobEntry(normalizedPath, relativePathToFile, builderInfo.m_busId, *infoForPlatform, jobDescriptor.m_jobKey.c_str(), 0, GenerateNewJobRunKey(), sourceUUID);
                        newJob.m_jobEntry.m_checkExclusiveLock = jobDescriptor.m_checkExclusiveLock;
                        newJob.m_jobParam = AZStd::move(jobDescriptor.m_jobParameters);
                        newJob.m_priority = jobDescriptor.m_priority;
                        newJob.m_watchFolder = scanFolder->ScanPath();
                        // note that until analysis completes, the jobId is not set and neither is the destination path.
                        jobsToAnalyze.push_back(AZStd::move(newJob));
                    }
                }

                ProcessCreateJobsResponse(createJobsResponse, createJobsRequest);

                if (createJobsResponse.m_sourceFileDependencyList.size())
                {
                    runJobLater = true;
                }
            }
        }
        
        if (runJobLater)
        {
            // If any jobs indicated they have dependencies, put whole set into the 'process later' queue, so it runs after its dependencies
            // The jobs need to be kept together in one batch because otherwise ProcessJobs is expecting to know about all the jobs and will delete any outputs not in the set
            entry.m_sourceFileInfo.m_relativePath = relativePathToFile;
            entry.m_sourceFileInfo.m_scanFolder = scanFolder;
            entry.m_jobsToAnalyze = AZStd::move(jobsToAnalyze);

            m_jobsToProcessLater.push_back(entry);
        }
        else
        {
            // Phase 2 : Process all the jobs and responses

            ProcessJobs(relativePathToFile, jobsToAnalyze, scanFolder);
        }
    }

    AZStd::shared_ptr<AssetDatabaseConnection> AssetProcessorManager::GetDatabaseConnection() const
    {
        return m_stateData;
    }

    void AssetProcessorManager::BeginIgnoringCacheFileDelete(const AZStd::string productPath)
    {
        QMutexLocker locker(&m_processingJobMutex);
        m_processingProductInfoList.insert(productPath);
    }

    void AssetProcessorManager::StopIgnoringCacheFileDelete(const AZStd::string productPath, bool queueAgainForProcessing)
    {
        QMutexLocker locker(&m_processingJobMutex);

        m_processingProductInfoList.erase(productPath);
        if (queueAgainForProcessing)
        {
            QMetaObject::invokeMethod(this, "AssessDeletedFile", Qt::QueuedConnection, Q_ARG(QString, QString(productPath.c_str())));
        }
    }

    AZ::s64 AssetProcessorManager::GenerateNewJobRunKey()
    {
        return m_highestJobRunKeySoFar++;
    }

    bool AssetProcessorManager::EraseLogFile(const char* fileName)
    {
        AZ_Assert(fileName, "Invalid call to EraseLogFile with a nullptr filename.");
        if ((!fileName) || (fileName[0] == 0))
        {
            // Sometimes logs are empty / missing already in the DB or empty in the "log" column.
            // this counts as success since there is no log there.
            return true;
        }
        // try removing it immediately - even if it doesn't exist, its quicker to delete it and notice it failed.
        if (!AZ::IO::FileIOBase::GetInstance()->Remove(fileName))
        {
            // we couldn't remove it.  Is it becuase it was already gone?  Because in that case, there's no problem.
            // we only worry if we were unable to delete it and it exists
            if (AZ::IO::FileIOBase::GetInstance()->Exists(fileName))
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Was unable to delete log file %s...\n", fileName);
                return false;
            }
        }

        return true; // if the file was either successfully removed or never existed in the first place, its gone, so we return true;
    }

    bool AssetProcessorManager::MigrateScanFolders()
    {
        // Migrate Scan Folders retrieves the last list of scan folders from the DB
        // it then finds out what scan folders SHOULD be in the database now, by matching the portable key

        // start with all of the scan folders that are currently in the database.
        m_stateData->QueryScanFoldersTable([this](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry)
            {
                // the database is case-insensitive, so we should emulate that here in our find()
                AZStd::string portableKey = entry.m_portableKey;
                AZStd::to_lower(portableKey.begin(), portableKey.end());
                m_scanFoldersInDatabase.insert(AZStd::make_pair(portableKey, entry));
                return true;
            });

        // now update them based on whats in the config file.
        for (int i = 0; i < m_platformConfig->GetScanFolderCount(); ++i)
        {
            AssetProcessor::ScanFolderInfo& scanFolderFromConfigFile = m_platformConfig->GetScanFolderAt(i);

            // for each scan folder in the config file, see if its port key already exists
            AZStd::string scanFolderFromConfigFileKeyLower = scanFolderFromConfigFile.GetPortableKey().toLower().toUtf8().constData();
            auto found = m_scanFoldersInDatabase.find(scanFolderFromConfigFileKeyLower.c_str());

            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanFolderToWrite;
            if (found != m_scanFoldersInDatabase.end())
            {
                // portable key was found, this means we have an existing database entry for this config file entry.
                scanFolderToWrite = AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry(
                        found->second.m_scanFolderID,
                        scanFolderFromConfigFile.ScanPath().toUtf8().constData(),
                        scanFolderFromConfigFile.GetDisplayName().toUtf8().constData(),
                        scanFolderFromConfigFile.GetPortableKey().toUtf8().constData(),
                        scanFolderFromConfigFile.GetOutputPrefix().toUtf8().constData(),
                        scanFolderFromConfigFile.IsRoot());
                //remove this scan path from the scan folders so what is left can deleted
                m_scanFoldersInDatabase.erase(found);
            }
            else
            {
                // no such key exists, its a new entry.
                scanFolderToWrite = AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry(
                        scanFolderFromConfigFile.ScanPath().toUtf8().constData(),
                        scanFolderFromConfigFile.GetDisplayName().toUtf8().constData(),
                        scanFolderFromConfigFile.GetPortableKey().toUtf8().constData(),
                        scanFolderFromConfigFile.GetOutputPrefix().toUtf8().constData(),
                        scanFolderFromConfigFile.IsRoot());
            }

            // update the database.
            bool res = m_stateData->SetScanFolder(scanFolderToWrite);

            AZ_Assert(res, "Failed to set a scan folder.");
            if (!res)
            {
                return false;
            }

            // update the in-memory value of the scan folder id from the above query.
            scanFolderFromConfigFile.SetScanFolderID(scanFolderToWrite.m_scanFolderID);
        }
        return true;
    }

    bool AssetProcessorManager::SearchSourceBySourceUUID(AZ::Uuid sourceUuid, QString& relSourcePath)
    {
        // Check the uuid in the database first, if not found we will check the uuid in the two internal maps
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceDatabaseEntry;
        if (m_stateData->GetSourceBySourceGuid(sourceUuid, sourceDatabaseEntry))
        {
            relSourcePath = sourceDatabaseEntry.m_sourceName.c_str();
            return true;
        }
        else
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sourceUUIDToSourceNameMapMutex);

            // Checking whether AP know about this source file, this map contain uuids of all known sources that we have forwarded to the RCController 
            auto foundSource = m_sourceUUIDToSourceNameMap.find(sourceUuid);
            if (foundSource != m_sourceUUIDToSourceNameMap.end())
            {
                relSourcePath = m_sourceUUIDToSourceNameMap[sourceUuid].m_sourceName;
                return true;
            }
            else
            {
                // Checking whether the depends on source file is one of the source files that we have hold back for processing 
                auto dependentSource = m_sourceDependencyUUIDToSourceNameMap.find(sourceUuid);
                if (dependentSource != m_sourceDependencyUUIDToSourceNameMap.end())
                {
                    relSourcePath = m_sourceDependencyUUIDToSourceNameMap[sourceUuid];
                    return true;
                }
                else
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Unable to find source file having uuid %s", sourceUuid.ToString<AZStd::string>().c_str());
                    return false;
                }
            }
        }
    }

    void AssetProcessorManager::UpdateSourceFileDependencyInfo()
    {
        for (auto iter = m_sourceFileDependencyInfoMap.begin(); iter != m_sourceFileDependencyInfoMap.end();)
        {
            AssetBuilderSDK::SourceFileDependency& entry = iter->second.m_sourceFileDependency;
            if (!entry.m_sourceFileDependencyUUID.IsNull() && entry.m_sourceFileDependencyPath.empty())
            {
                QString relSourcePath;
                if (!SearchSourceBySourceUUID(entry.m_sourceFileDependencyUUID, relSourcePath))
                {
                    iter = m_sourceFileDependencyInfoMap.erase(iter);
                }
                else
                {
                    QString absSourceFileDependentPath = m_platformConfig->FindFirstMatchingFile(relSourcePath);
                    entry.m_sourceFileDependencyPath = AssetUtilities::NormalizeFilePath(absSourceFileDependentPath).toUtf8().data();
                    iter->second.m_relativeSourceFileDependencyPath = relSourcePath.toUtf8().constData();

                    ++iter;
                }
            }
            else
            {
                ++iter;
            }
        }
    }

    void AssetProcessorManager::GetSourceDependenciesFromDatabase(AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& newSourceFileDependencies, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& databaseSourceFileDependencies)
    {
        AzToolsFramework::AssetDatabase::SourceFileDependencyEntry* lastEntry = nullptr;

        // newSourceFileDependencies is assumed to be sorted
        for (AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry : newSourceFileDependencies)
        {
            // Avoid making duplicate queries with the same guid-source pair
            if (lastEntry == nullptr || lastEntry->m_builderGuid != entry.m_builderGuid || lastEntry->m_source != entry.m_source)
            {
                m_stateData->GetSourceFileDependenciesByBuilderGUIDAndSource(entry.m_builderGuid, entry.m_source.c_str(), databaseSourceFileDependencies);
            }

            lastEntry = &entry;
        }
    }

    void AssetProcessorManager::CompareEmittedSourceDependenciesFromDatabase(AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& newSourceFileDependencies, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& databaseSourceFileDependencies)
    {
        using namespace AzToolsFramework::AssetDatabase;

        for (int idx = aznumeric_cast<int>(databaseSourceFileDependencies.size()) - 1; idx >= 0; --idx)
        {
            const SourceFileDependencyEntry& oldSourceFileDependencyEntry = databaseSourceFileDependencies[idx];
            bool foundIt = false;
            for (auto newDependencyIter = newSourceFileDependencies.begin(); newDependencyIter != newSourceFileDependencies.end(); ++newDependencyIter)
            {
                if((AzFramework::StringFunc::Equal(oldSourceFileDependencyEntry.m_source.c_str(), newDependencyIter->m_source.c_str())) &&
                    (AzFramework::StringFunc::Equal(oldSourceFileDependencyEntry.m_dependsOnSource.c_str(), newDependencyIter->m_dependsOnSource.c_str())) &&
                    (oldSourceFileDependencyEntry.m_builderGuid == newDependencyIter->m_builderGuid)
                  )
                {
                    foundIt = true;
                    newSourceFileDependencies.erase(newDependencyIter);
                    break;
                }
            }

            if (foundIt)
            {
                databaseSourceFileDependencies.erase(databaseSourceFileDependencies.begin() + idx);
            }
        }
    }

    void AssetProcessorManager::UpdateSourceFileDependencyDatabase()
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Updating the source dependency table
        SourceFileDependencyEntryContainer newSourceFileDependencies;
        SourceFileDependencyEntryContainer oldSourceFileDependencies;

        for (auto iter = m_sourceFileDependencyInfoMap.begin(); iter != m_sourceFileDependencyInfoMap.end(); ++iter)
        {
            AssetProcessor::SourceFileDependencyInternal& sourceFileDependency = iter->second;
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry sourceFileDependencyEntry(sourceFileDependency.m_builderId, sourceFileDependency.m_relativeSourcePath, sourceFileDependency.m_relativeSourceFileDependencyPath);

            newSourceFileDependencies.push_back();
            newSourceFileDependencies.back() = AZStd::move(sourceFileDependencyEntry);
        }

        // This function makes an assumption that newSourceFileDependencies is sorted (m_sourceFileDependencyInfoMap is sorted)
        GetSourceDependenciesFromDatabase(newSourceFileDependencies, oldSourceFileDependencies);
 
        if (oldSourceFileDependencies.size())
        {
            // remove the old entries which were not emitted this time and than add new entries;
            CompareEmittedSourceDependenciesFromDatabase(newSourceFileDependencies, oldSourceFileDependencies);

            //no entries to remove if it is empty
            if(oldSourceFileDependencies.size())
            {
                m_stateData->RemoveSourceFileDependencies(oldSourceFileDependencies);
            }
        }

        if (newSourceFileDependencies.size())
        {
            m_stateData->SetSourceFileDependencies(newSourceFileDependencies);
        }

        for (JobToProcessEntry& entry : m_jobsToProcessLater)
        {
            if (!entry.m_jobsToAnalyze.size())
            {
                // if we are here it means that we have a source file that do not create any jobs, we still need to update the database and add these source files
                SourceDatabaseEntry source;
                AddSourceToDatabase(source, entry.m_sourceFileInfo.m_scanFolder, entry.m_sourceFileInfo.m_relativePath);
            }
        }
    }


    void AssetProcessorManager::PopulateSourceDependencyList(JobDetails& jobDetail, QString relSourceFilePath, AZStd::unordered_set<AZStd::string>& sourceFileDependencyNameList)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AZ::Uuid builderUuid = jobDetail.m_jobEntry.m_builderGuid;
        sourceFileDependencyNameList.insert(relSourceFilePath.toUtf8().data());
        
        SourceFileDependencyEntryContainer container;
        if (m_stateData->GetSourceFileDependenciesByBuilderGUIDAndSource(builderUuid, relSourceFilePath.toUtf8().data(), container))
        {
            for (SourceFileDependencyEntry& entry : container)
            {
                auto dependentSourcefound = sourceFileDependencyNameList.find(entry.m_dependsOnSource);
                if (dependentSourcefound == sourceFileDependencyNameList.end())
                {
                    sourceFileDependencyNameList.insert(entry.m_dependsOnSource);
                    AssetProcessor::SourceFileDependencyInternal sourceFileInternalDependencyInternal;
                    sourceFileInternalDependencyInternal.m_builderId = entry.m_builderGuid;
                    sourceFileInternalDependencyInternal.m_relativeSourceFileDependencyPath = entry.m_dependsOnSource;
                    sourceFileInternalDependencyInternal.m_relativeSourcePath = entry.m_source;
                    sourceFileInternalDependencyInternal.m_sourceFileDependency.m_sourceFileDependencyPath = m_platformConfig->FindFirstMatchingFile(entry.m_dependsOnSource.c_str()).toUtf8().data();
                    sourceFileInternalDependencyInternal.m_sourceUUID = AssetUtilities::CreateSafeSourceUUIDFromName(entry.m_source.c_str());
                    sourceFileInternalDependencyInternal.m_sourceWatchFolder = jobDetail.m_watchFolder.toUtf8().data();
                    jobDetail.m_sourceFileDependencyList.push_back(sourceFileInternalDependencyInternal);
                    PopulateSourceDependencyList(jobDetail, sourceFileInternalDependencyInternal.m_relativeSourceFileDependencyPath.c_str(), sourceFileDependencyNameList);
                }
            }
        }
    }


    void AssetProcessorManager::AnalyzeJobDetail(JobToProcessEntry& jobEntry)
    {
        AZStd::unordered_set<AZStd::string> sourceFileDependencyNameList; // We want this list to prevent circular dependency
        for (JobDetails& jobDetail : jobEntry.m_jobsToAnalyze)
        {
            PopulateSourceDependencyList(jobDetail, jobDetail.m_jobEntry.m_relativePathToFile.toUtf8().data(), sourceFileDependencyNameList);

            // Sorting this list because these source dependencies will be used to generate fingerprint for the file and same dependencies in different order can change the fingerprint 
            AZStd::sort(jobDetail.m_sourceFileDependencyList.begin(), jobDetail.m_sourceFileDependencyList.end(),
                [](const AssetProcessor::SourceFileDependencyInternal& lhs, const AssetProcessor::SourceFileDependencyInternal& rhs)
            {
                return (AssetUtilities::ComputeCRC32(lhs.ToString().c_str()) < AssetUtilities::ComputeCRC32(rhs.ToString().c_str()));
            }
            );
        }
    }

    QStringList AssetProcessorManager::CheckSourceFileDependency(const QString& sourcePath)
    {
        // Check the database for the source file that have mentioned this file as a source file dependency
        // construct the full absolute path and call AssessFileInternal so that we put that file in the queue
        QStringList relativeSourcePathQueue;
        QString relativePath;
        QString scanFolder;
        if (m_platformConfig->ConvertToRelativePath(sourcePath, relativePath, scanFolder))
        {
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer container;
            m_stateData->GetSourceFileDependenciesByDependsOnSource(relativePath, container);
            for (AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry : container)
            {
                relativeSourcePathQueue.append(entry.m_source.c_str());
            }
            auto found = m_dependsOnSourceToSourceMap.equal_range(relativePath);
            for (auto iter = found.first; iter != found.second; ++iter)
            {
                QString dependentSourceRelativeName = iter->second;
                if (!relativeSourcePathQueue.contains(dependentSourceRelativeName, Qt::CaseInsensitive))
                {
                    relativeSourcePathQueue.append(dependentSourceRelativeName);
                }
            }

            AZ::Uuid sourceUuid = AssetUtilities::CreateSafeSourceUUIDFromName(relativePath.toUtf8().data());

            auto foundSource = m_dependsOnSourceUuidToSourceMap.equal_range(sourceUuid);

            for (auto iter = foundSource.first; iter != foundSource.second; ++iter)
            {
                QString dependentSourceRelativeName = iter->second;
                if (!relativeSourcePathQueue.contains(dependentSourceRelativeName, Qt::CaseInsensitive))
                {
                    relativeSourcePathQueue.append(dependentSourceRelativeName);
                }
            }

            return relativeSourcePathQueue;
        }

        return QStringList();
    }

    void AssetProcessorManager::AddSourceToDatabase(AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceDatabaseEntry, const ScanFolderInfo* scanFolder, QString relativeSourceFilePath)
    {
        sourceDatabaseEntry.m_scanFolderPK = scanFolder->ScanFolderID();

        if (!scanFolder->GetOutputPrefix().isEmpty())
        {
            // replace the "output prefix" part of the file name with the one from the ini file to sort out case sensitivity problems.
            QString withoutOutputPrefix = relativeSourceFilePath.remove(0, scanFolder->GetOutputPrefix().length() + 1);
            sourceDatabaseEntry.m_sourceName = AZStd::string::format("%s/%s", scanFolder->GetOutputPrefix().toUtf8().constData(), withoutOutputPrefix.toUtf8().data());
        }
        else
        {
            sourceDatabaseEntry.m_sourceName = relativeSourceFilePath.toUtf8().constData();
        }


        sourceDatabaseEntry.m_sourceGuid = AssetUtilities::CreateSafeSourceUUIDFromName(sourceDatabaseEntry.m_sourceName.c_str());
        if (!m_stateData->SetSource(sourceDatabaseEntry))
        {
            //somethings wrong...
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to add source to the database!!!");
        }
    }

    void AssetProcessorManager::CheckAssetProcessorIdleState()
    {
        Q_EMIT AssetProcessorManagerIdleState(IsIdle());
    }
} // namespace AssetProcessor

#include <native/AssetManager/assetProcessorManager.moc>
