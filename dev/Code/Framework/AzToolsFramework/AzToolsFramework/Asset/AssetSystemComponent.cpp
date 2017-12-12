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

#include "stdafx.h"
#include <AzToolsFramework/Asset/AssetSystemComponent.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

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
                    [this](unsigned int typeId, const void* data, unsigned int dataLength)
                {
                    OnAssetSystemMessage(typeId, data, dataLength);
                });
            }

            AssetSystemRequestBus::Handler::BusConnect();
            AssetSystemJobRequestBus::Handler::BusConnect();
        }

        void AssetSystemComponent::Deactivate()
        {
            AssetSystemJobRequestBus::Handler::BusDisconnect();
            AssetSystemRequestBus::Handler::BusDisconnect();

            AzFramework::SocketConnection* socketConn = AzFramework::SocketConnection::GetInstance();
            AZ_Assert(socketConn, "AzToolsFramework::AssetSystem::AssetSystemComponent requires a valid socket conection!");
            if (socketConn)
            {
                socketConn->RemoveMessageHandler(AZ_CRC("AssetProcessorManager::SourceFileNotification", 0x8bfc4d1c), m_cbHandle);
            }
        }

        void AssetSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            //source file
            SourceFileInfoRequest::Reflect(context);
            SourceFileInfoResponse::Reflect(context);
            SourceFileNotificationMessage::Reflect(context);

            // Requests
            AssetJobsInfoRequest::Reflect(context);
            AssetJobLogRequest::Reflect(context);

            // Responses
            AssetJobsInfoResponse::Reflect(context);
            AssetJobLogResponse::Reflect(context);

            //JobInfo
            AzToolsFramework::AssetSystem::JobInfo::Reflect(context);
            AzToolsFramework::AssetSystem::SourceFileInfo::Reflect(context);

            //AssetSystemComponent
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetSystemComponent, AZ::Component>()
                    ->SerializerForEmptyClass();
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

            fullPath = response.m_fullSourcePath;
            return response.m_resolved;
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

        bool AssetSystemComponent::GetSourceAssetInfoById(const AZ::Uuid& /*guid*/, AZStd::string& /*watchFolder*/, AZStd::string& /*relativePath*/)
        {
            AZ_Assert(false, "Not implemented yet");
            return {};
        }

        bool AssetSystemComponent::GetSourceFileInfoByPath(SourceFileInfo& result, const char* sourcePath)
        {
            AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
            if (!engineConnection || !engineConnection->IsConnected())
            {
                AZ_Error("Editor", false, "Failed to establish a SocketConnection or it hasn't been connected yet.");
                return false;
            }

            SourceFileInfoRequest request(sourcePath);
            SourceFileInfoResponse response;
            
            if (!SendRequest(request, response))
            {
                AZ_Error("Editor", false, "Failed to send GetSourceFileInfoByPath request for search term: %s", sourcePath);
                return false;
            }

            if (response.m_infoFound)
            {
                result.m_watchFolder = AZStd::move(response.m_watchFolder);
                result.m_relativePath = AZStd::move(response.m_relativePath);
                result.m_sourceGuid = response.m_sourceGuid;
            }

            return response.m_infoFound;
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
    } // namespace AssetSystem
} // namespace AzToolsFramework
