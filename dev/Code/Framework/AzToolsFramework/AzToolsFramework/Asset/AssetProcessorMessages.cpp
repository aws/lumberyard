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
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Crc.h>

namespace AzToolsFramework
{
    using namespace AZ;
    using namespace AzFramework::AssetSystem;

    namespace AssetSystem
    {
        //---------------------------------------------------------------------
        AssetJobsInfoRequest::AssetJobsInfoRequest(const AZ::OSString& searchTerm)
            : m_searchTerm(searchTerm)
        {
            AZ_Assert(!searchTerm.empty(), "AssetJobsInfoRequest: Search Term is empty");
        }

        unsigned int AssetJobsInfoRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::AssetJobsInfoRequest", 0xbd18de74);
            return messageType;
        }

        unsigned int AssetJobsInfoRequest::GetMessageType() const
        {
            return MessageType();
        }

        void AssetJobsInfoRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobsInfoRequest, BaseAssetProcessorMessage>()
                    ->Version(4)
                    ->Field("SearchTerm", &AssetJobsInfoRequest::m_searchTerm)
                    ->Field("EscalateJobs", &AssetJobsInfoRequest::m_escalateJobs)
                    ->Field("IsSearchTermJobKey", &AssetJobsInfoRequest::m_isSearchTermJobKey)
                    ->Field("AssetId", &AssetJobsInfoRequest::m_assetId);
            }
        }

        bool AssetJobsInfoRequest::RequireFencing()
        {
            return true;
        }

        //---------------------------------------------------------------------
        AssetJobsInfoResponse::AssetJobsInfoResponse(AssetSystem::JobInfoContainer& jobList, bool isSuccess)
            : m_isSuccess(isSuccess)
        {
            m_jobList.swap(jobList);
        }

        AssetJobsInfoResponse::AssetJobsInfoResponse(AssetSystem::JobInfoContainer&& jobList, bool isSuccess)
            : m_isSuccess(isSuccess)
            , m_jobList(AZStd::move(jobList))
        {
        }

        unsigned int AssetJobsInfoResponse::GetMessageType() const
        {
            return AssetJobsInfoRequest::MessageType();
        }

        void AssetJobsInfoResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobsInfoResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("JobList", &AssetJobsInfoResponse::m_jobList)
                    ->Field("Success", &AssetJobsInfoResponse::m_isSuccess);
            }
        }

        //---------------------------------------------------------------------
        AssetJobLogRequest::AssetJobLogRequest(AZ::s64 jobRunKey)
            : m_jobRunKey(jobRunKey)
        {
            AZ_Assert(m_jobRunKey > 0 , "AssetJobLogRequest: asset run key is invalid");
        }

        unsigned int AssetJobLogRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::AssetJobLogRequest", 0xfbb80fd3);
            return messageType;
        }

        unsigned int AssetJobLogRequest::GetMessageType() const
        {
            return MessageType();
        }

        void AssetJobLogRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobLogRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("JobRunKey", &AssetJobLogRequest::m_jobRunKey);
            }
        }

        bool AssetJobLogRequest::RequireFencing()
        {
            return true;
        }

        //---------------------------------------------------------------------
        AssetJobLogResponse::AssetJobLogResponse(const AZStd::string& jobLog,  bool isSuccess)
            : m_jobLog(jobLog)
            , m_isSuccess(isSuccess)
        {
        }

        unsigned int AssetJobLogResponse::GetMessageType() const
        {
            return AssetJobLogRequest::MessageType();
        }

        void AssetJobLogResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetJobLogResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("JobLog", &AssetJobLogResponse::m_jobLog)
                    ->Field("Success", &AssetJobLogResponse::m_isSuccess);
            }
        }

        //---------------------------------------------------------------------
        SourceFileNotificationMessage::SourceFileNotificationMessage(const AZ::OSString& relativeSourcePath, const AZ::OSString& scanFolder, NotificationType type, AZ::Uuid sourceUUID)
            : m_relativeSourcePath(relativeSourcePath)
            , m_scanFolder(scanFolder)
            , m_type(type)
            , m_sourceUUID(sourceUUID)
        {
            AZ_Assert(!m_relativeSourcePath.empty(), "SourceFileNotificationMessage: empty relative path");
            AZ_Assert(!scanFolder.empty(), "SourceFileNotificationMessage: empty scanFolder");
        }

        unsigned int SourceFileNotificationMessage::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessorManager::SourceFileNotification", 0x8bfc4d1c);
            return messageType;
        }

        unsigned int SourceFileNotificationMessage::GetMessageType() const
        {
            return MessageType();
        }

        void SourceFileNotificationMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SourceFileNotificationMessage, BaseAssetProcessorMessage>()
                    ->Version(2)
                    ->Field("RelativeSourcePath", &SourceFileNotificationMessage::m_relativeSourcePath)
                    ->Field("ScanFolder", &SourceFileNotificationMessage::m_scanFolder)
                    ->Field("NotificationType", &SourceFileNotificationMessage::m_type)
                    ->Field("SourceUUID", &SourceFileNotificationMessage::m_sourceUUID);
            }
        }

        //---------------------------------------------------------------------
        unsigned int GetScanFoldersRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::GetScanFoldersRequest", 0x01274152);
            return messageType;
        }

        unsigned int GetScanFoldersRequest::GetMessageType() const
        {
            return MessageType();
        }

        void GetScanFoldersRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetScanFoldersRequest>()
                    ->Version(1)
                    ->SerializeWithNoData();
            }
        }

        //---------------------------------------------------------------------
        GetScanFoldersResponse::GetScanFoldersResponse(const AZStd::vector<AZStd::string>& scanFolders)
            : m_scanFolders(scanFolders)
        {
        }

        GetScanFoldersResponse::GetScanFoldersResponse(AZStd::vector<AZStd::string>&& scanFolders)
            : m_scanFolders(AZStd::move(scanFolders))
        {
        }

        unsigned int GetScanFoldersResponse::GetMessageType() const
        {
            return GetScanFoldersRequest::MessageType();
        }

        void GetScanFoldersResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetScanFoldersResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ScanFolders", &GetScanFoldersResponse::m_scanFolders);
            }
        }

        //---------------------------------------------------------------------
        unsigned int GetAssetSafeFoldersRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::GetAssetSafeFoldersRequest", 0xf58fd05c);
            return messageType;
        }

        unsigned int GetAssetSafeFoldersRequest::GetMessageType() const
        {
            return MessageType();
        }

        void GetAssetSafeFoldersRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetAssetSafeFoldersRequest>()
                    ->Version(1)
                    ->SerializeWithNoData();
            }
        }

        //---------------------------------------------------------------------
        GetAssetSafeFoldersResponse::GetAssetSafeFoldersResponse(const AZStd::vector<AZStd::string>& assetSafeFolders)
            : m_assetSafeFolders(assetSafeFolders)
        {
        }

        GetAssetSafeFoldersResponse::GetAssetSafeFoldersResponse(AZStd::vector<AZStd::string>&& assetSafeFolders)
            : m_assetSafeFolders(AZStd::move(assetSafeFolders))
        {
        }

        unsigned int GetAssetSafeFoldersResponse::GetMessageType() const
        {
            return GetAssetSafeFoldersRequest::MessageType();
        }

        void GetAssetSafeFoldersResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetAssetSafeFoldersResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetSafeFolders", &GetAssetSafeFoldersResponse::m_assetSafeFolders);
            }
        }

        //---------------------------------------------------------------------
        void FileInfosNotificationMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileInfosNotificationMessage, BaseAssetProcessorMessage>()
                    ->Field("NotificationType", &FileInfosNotificationMessage::m_type)
                    ->Field("FileID", &FileInfosNotificationMessage::m_fileID)
                    ->Version(1);
            }
        }

        unsigned FileInfosNotificationMessage::GetMessageType() const
        {
            static unsigned int messageType = AZ_CRC("FileProcessor::FileInfosNotification", 0x001c43f5);
            return messageType;
        }
    } // namespace AssetSystem
} // namespace AzToolsFramework
