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

#include "StdAfx.h"
#include <AzToolsFramework/Asset/AssetSystemComponent.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        void OnAssetSystemMessage(unsigned int /*typeId*/, const void* buffer, unsigned int bufferSize)
        {
            SourceFileNotificationMessage message;
            if (!AZ::Utils::LoadObjectFromBufferInPlace(buffer, bufferSize, message))
            {
                AZ_TracePrintf("AssetSystem", "Problem deserializing SourceFileNotificationMessage");
                return;
            }

            switch (message.m_type)
            {
            case SourceFileNotificationMessage::FileChanged:
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::SourceFileChanged,
                    message.m_relativeSourcePath, message.m_scanFolder, message.m_sourceUUID);
                break;
            case SourceFileNotificationMessage::FileRemoved:
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::SourceFileRemoved,
                    message.m_relativeSourcePath, message.m_scanFolder, message.m_sourceUUID);
                break;
            case SourceFileNotificationMessage::FileFailed :
                AssetSystemBus::QueueBroadcast(&AssetSystemBus::Events::SourceFileFailed,
                    message.m_relativeSourcePath, message.m_scanFolder, message.m_sourceUUID);
                break;
            default:
                AZ_TracePrintf("AssetSystem", "Unknown SourceFileNotificationMessage type");
                break;
            }
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> SendAssetJobsRequest(AssetJobsInfoRequest request, AssetJobsInfoResponse &response)
        {
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetJobsInfo Info for search term: %s", request.m_searchTerm.c_str());
                return AZ::Failure();
            }

            if (response.m_isSuccess)
            {
                return AZ::Success(response.m_jobList);
            }

            return AZ::Failure();
        }

        void AssetSystemComponent::Activate()
        {
            AzFramework::SocketConnection* socketConn = AzFramework::SocketConnection::GetInstance();
            AZ_Assert(socketConn, "AzToolsFramework::AssetSystem::AssetSystemComponent requires a valid socket conection!");
            if (socketConn)
            {
                m_cbHandle = socketConn->AddMessageHandler(AZ_CRC("AssetProcessorManager::SourceFileNotification", 0x8bfc4d1c),
                    [](unsigned int typeId, unsigned int /*serial*/, const void* data, unsigned int dataLength)
                {
                    OnAssetSystemMessage(typeId, data, dataLength);
                });
            }

            AssetSystemRequestBus::Handler::BusConnect();
            AssetSystemJobRequestBus::Handler::BusConnect();
            AzToolsFramework::ToolsAssetSystemBus::Handler::BusConnect();
        }

        void AssetSystemComponent::Deactivate()
        {
            AzToolsFramework::ToolsAssetSystemBus::Handler::BusDisconnect();
            AssetSystemJobRequestBus::Handler::BusDisconnect();
            AssetSystemRequestBus::Handler::BusDisconnect();

            AzFramework::SocketConnection* socketConn = AzFramework::SocketConnection::GetInstance();
            AZ_Assert(socketConn, "AzToolsFramework::AssetSystem::AssetSystemComponent requires a valid socket conection!");
            if (socketConn)
            {
                socketConn->RemoveMessageHandler(AZ_CRC("AssetProcessorManager::SourceFileNotification", 0x8bfc4d1c), m_cbHandle);
            }

            AssetSystemBus::ClearQueuedEvents();
        }

        void AssetSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            //source file
            SourceFileNotificationMessage::Reflect(context);

            // Requests
            AssetJobsInfoRequest::Reflect(context);
            AssetJobLogRequest::Reflect(context);
            GetScanFoldersRequest::Reflect(context);

            // Responses
            AssetJobsInfoResponse::Reflect(context);
            AssetJobLogResponse::Reflect(context);
            GetScanFoldersResponse::Reflect(context);

            //JobInfo
            AzToolsFramework::AssetSystem::JobInfo::Reflect(context);

            //AssetSystemComponent
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetSystemComponent, AZ::Component>()
                    ;
            }
        }

        void AssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AssetProcessorToolsConnection", 0x734669bc));
        }

        void AssetSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AssetProcessorToolsConnection", 0x734669bc));
        }

        void AssetSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AssetProcessorConnection", 0xf0cd75cd));
        }

        bool AssetSystemComponent::GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& outputPath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                outputPath = fullPath;
                return false;
            }

            AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathRequest request(fullPath);
            AzFramework::AssetSystem::GetRelativeProductPathFromFullSourceOrProductPathResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetId request for %s", fullPath.c_str());
                outputPath = fullPath;
                return false;
            }

            outputPath = response.m_relativeProductPath;
            return response.m_resolved;
        }

        bool AssetSystemComponent::GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullPath)
        {
            auto foundIt = m_assetSourceRelativePathToFullPathCache.find(relPath);
            if (foundIt != m_assetSourceRelativePathToFullPathCache.end())
            {
                fullPath = foundIt->second;
                return true;
            }

            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                fullPath = "";
                return false;
            }

            AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathRequest request(relPath);
            AzFramework::AssetSystem::GetFullSourcePathFromRelativeProductPathResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetPath request for %s", relPath.c_str());
                fullPath = "";
                return false;
            }


            if (response.m_resolved)
            {
                fullPath = response.m_fullSourcePath;
                m_assetSourceRelativePathToFullPathCache[relPath] = fullPath;
                return true;
            }
            else
            {
                fullPath = "";
                return false;
            }
        }

        const char* AssetSystemComponent::GetAbsoluteDevGameFolderPath()
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                return fileIO->GetAlias("@devassets@");
            }
            return "";
        }

        const char* AssetSystemComponent::GetAbsoluteDevRootFolderPath()
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO)
            {
                return fileIO->GetAlias("@devroot@");
            }
            return "";
        }

        void AssetSystemComponent::UpdateQueuedEvents()
        {
            AssetSystemBus::ExecuteQueuedEvents();
        }

        bool AssetSystemComponent::GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzFramework::AssetSystem::SourceAssetInfoRequest request(assetId, assetType);
            AzFramework::AssetSystem::SourceAssetInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetAssetInfoById request for %s", assetId.ToString<AZStd::string>().c_str());
                return false;
            }

            assetInfo = response.m_assetInfo;
            rootFilePath = response.m_rootFolder;
            return true;
        }

        bool AssetSystemComponent::GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzFramework::AssetSystem::SourceAssetInfoRequest request(sourcePath);
            AzFramework::AssetSystem::SourceAssetInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetSourceInfoBySourcePath request for %s", sourcePath);
                return false;
            }

            if (response.m_found)
            {
                assetInfo = response.m_assetInfo;
                watchFolder = response.m_rootFolder;
            }
            return response.m_found;
        }

        bool AssetSystemComponent::GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            AzFramework::AssetSystem::SourceAssetInfoRequest request(sourceUuid, AZ::Uuid::CreateNull());
            AzFramework::AssetSystem::SourceAssetInfoResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetSourceInfoBySourceUUID request for uuid: ", sourceUuid.ToString<AZ::OSString>().c_str());
                return false;
            }

            if (response.m_found)
            {
                assetInfo = response.m_assetInfo;
                watchFolder = response.m_rootFolder;
            }
            return response.m_found;
        }

        bool AssetSystemComponent::GetScanFolders(AZStd::vector<AZStd::string>& scanFolders)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return false;
            }

            GetScanFoldersRequest request;
            GetScanFoldersResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetScanFolders request");
                return false;
            }

            scanFolders.insert(scanFolders.end(), response.m_scanFolders.begin(), response.m_scanFolders.end());
            return !response.m_scanFolders.empty();
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> AssetSystemComponent::GetAssetJobsInfo(const AZStd::string& path, const bool escalateJobs)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobsInfoRequest request(path);
            request.m_escalateJobs = escalateJobs;
            AssetJobsInfoResponse response;

            return SendAssetJobsRequest(request, response);
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> AssetSystemComponent::GetAssetJobsInfoByAssetID(const AZ::Data::AssetId& assetId, const bool escalateJobs)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobsInfoRequest request;
            request.m_assetId = assetId;
            request.m_escalateJobs = escalateJobs;
            AssetJobsInfoResponse response;

            return SendAssetJobsRequest(request, response);
        }

        AZ::Outcome<JobStatus> AssetSystemComponent::GetAssetJobsStatusByJobKey(const AZStd::string& jobKey, const bool escalateJobs)
        {
            // Strategy
            // First find all jobInfos for the inputted jobkey.
            // if there is no jobKey than return missing.
            // Otherwise if even one job failed than return failed.
            // If none of the job failed than check to see whether any job status are queued or inprogress, if yes than return the appropriate status.
            // Otherwise return completed status

            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> result = AZ::Failure();
            result = GetAssetJobsInfoByJobKey(jobKey, escalateJobs);
            if (!result.IsSuccess())
            {
                return AZ::Failure();
            }

            AzToolsFramework::AssetSystem::JobInfoContainer& jobInfos = result.GetValue();

            if (!jobInfos.size())
            {
                return AZ::Success(JobStatus::Missing);
            }

            bool isAnyJobInProgress = false;
            for (const AzToolsFramework::AssetSystem::JobInfo& jobInfo : jobInfos)
            {
                if (jobInfo.m_status == JobStatus::Failed)
                {
                    return AZ::Success(JobStatus::Failed);
                }
                else if (jobInfo.m_status == JobStatus::Queued)
                {
                    return AZ::Success(JobStatus::Queued);
                }
                else if (jobInfo.m_status == JobStatus::InProgress)
                {
                    isAnyJobInProgress = true;
                }
            }

            if (isAnyJobInProgress)
            {
                return AZ::Success(JobStatus::InProgress);
            }

            return AZ::Success(JobStatus::Completed);
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> AssetSystemComponent::GetAssetJobsInfoByJobKey(const AZStd::string& jobKey, const bool escalateJobs)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobsInfoRequest request(jobKey);
            request.m_isSearchTermJobKey = true;
            request.m_escalateJobs = escalateJobs;
            AssetJobsInfoResponse response;

            return SendAssetJobsRequest(request, response);
        }

        AZ::Outcome<AZStd::string> AssetSystemComponent::GetJobLog(AZ::u64 jobrunkey)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return AZ::Failure();
            }

            AssetJobLogRequest request(jobrunkey);
            AssetJobLogResponse response;

            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetJobLog Info for jobrunkey: %llu", jobrunkey);
                return AZ::Failure();
            }

            if (response.m_isSuccess)
            {
                return AZ::Success(response.m_jobLog);
            }

            return AZ::Failure();
        }

        void AssetSystemComponent::SourceFileChanged(AZStd::string relativePath, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/)
        {
            m_assetSourceRelativePathToFullPathCache.erase(relativePath);
        }

        void AssetSystemComponent::SourceFileRemoved(AZStd::string relativePath, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/)
        {
            m_assetSourceRelativePathToFullPathCache.erase(relativePath);
        }

        void AssetSystemComponent::SourceFileFailed(AZStd::string relativePath, AZStd::string /*scanFolder*/, AZ::Uuid /*sourceUUID*/)
        {
            m_assetSourceRelativePathToFullPathCache.erase(relativePath);
        }

        void AssetSystemComponent::RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return;
            }

            AzFramework::AssetSystem::RegisterSourceAssetRequest request(assetType, assetFileFilter);

            if (!SendRequest(request))
            {
                AZ_Error("Editor", false, "Failed to send RegisterSourceAssetType request for asset type %s", assetType.ToString<AZStd::string>().c_str());
            }
        }

        void AssetSystemComponent::UnregisterSourceAssetType(const AZ::Data::AssetType& assetType)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                return;
            }

            AzFramework::AssetSystem::UnregisterSourceAssetRequest request(assetType);

            if (!SendRequest(request))
            {
                AZ_Error("Editor", false, "Failed to send UnregisterSourceAssetType request for asset type %s", assetType.ToString<AZStd::string>().c_str());
            }
        }

    } // namespace AssetSystem
} // namespace AzToolsFramework
