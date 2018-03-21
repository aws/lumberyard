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
    } // namespace AssetSystem
} // namespace AzToolsFramework
