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

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/IO/FileIO.h>

namespace AzFramework
{
    using namespace AZ;

    namespace AssetSystem
    {

        BaseAssetProcessorMessage::BaseAssetProcessorMessage(bool requireFencing /*= false*/)
            :m_requireFencing(requireFencing)
        {
        }

        //---------------------------------------------------------------------
        void BaseAssetProcessorMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("RequireFencing", &BaseAssetProcessorMessage::m_requireFencing);
            }
        }

        bool BaseAssetProcessorMessage::RequireFencing() const
        {
            return m_requireFencing;
        }

        //---------------------------------------------------------------------
        unsigned int NegotiationMessage::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::Negotiation", 0x141ebc28);
            return messageType;
        }

        unsigned int NegotiationMessage::GetMessageType() const
        {
            return MessageType();
        }

        void NegotiationMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<NegotiationMessage>()
                    ->Version(1)
                    ->Field("ApiVersion", &NegotiationMessage::m_apiVersion)
                    ->Field("Identifier", &NegotiationMessage::m_identifier)
                    ->Field("NegotiationInfo", &NegotiationMessage::m_negotiationInfoMap);
            }
        }

        //---------------------------------------------------------------------
        void RequestPing::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<RequestPing, BaseAssetProcessorMessage>()
                    ->Version(1);
            }
        }

        unsigned int RequestPing::MessageType()
        {
            return AZ_CRC("AssetSystem::RequestPing", 0xa6124cfb);
        }

        unsigned int RequestPing::GetMessageType() const
        {
            return RequestPing::MessageType();
        }

        //---------------------------------------------------------------------
        void ResponsePing::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ResponsePing, BaseAssetProcessorMessage>()
                    ->Version(1);
            }
        }

        unsigned int ResponsePing::MessageType()
        {
            return AZ_CRC("AssetSystem::RequestPing", 0xa6124cfb);
        }

        unsigned int ResponsePing::GetMessageType() const
        {
            return ResponsePing::MessageType();
        }

        //---------------------------------------------------------------------
        RequestAssetStatus::RequestAssetStatus(const char* sourceData, bool isStatusRequest, bool requireFencing /*= true*/)
            : BaseAssetProcessorMessage(requireFencing)
            , m_searchTerm(sourceData)
            , m_isStatusRequest(isStatusRequest)
        {
            AZ_Assert(sourceData, "Invalid source data for RequestAssetStatus");
        }


        RequestAssetStatus::RequestAssetStatus(bool requireFencing /*= true*/)
            :BaseAssetProcessorMessage(requireFencing)
        {
        }

        // these share the same message type since they're request and response.
        unsigned int RequestAssetStatus::MessageType()
        {
            static unsigned int messageTypeAssetStatus = AZ_CRC("AssetSystem::RequestAssetStatus", 0x63146187);
            return messageTypeAssetStatus;
        }

        unsigned int RequestAssetStatus::GetMessageType() const
        {
            return RequestAssetStatus::MessageType();
        }

        void RequestAssetStatus::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<RequestAssetStatus, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("SearchTerm", &RequestAssetStatus::m_searchTerm)
                    ->Field("IsStatusRequest", &RequestAssetStatus::m_isStatusRequest);
            }
        }

        //---------------------------------------------------------------------
        void RequestAssetProcessorStatus::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<RequestAssetProcessorStatus, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Platform", &RequestAssetProcessorStatus::m_platform);
            }
        }

        unsigned int RequestAssetProcessorStatus::MessageType()
        {
            static unsigned int messageAssetProcessorState = AZ_CRC("AssetSystem::RequestAssetProcessorStatus", 0x5172b959);
            return messageAssetProcessorState;
        }

        unsigned int RequestAssetProcessorStatus::GetMessageType() const
        {
            return RequestAssetProcessorStatus::MessageType();
        }

        //------------------------------------------------------------------------
        void ResponseAssetProcessorStatus::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ResponseAssetProcessorStatus, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("NumberOfPendingCopyJobs", &ResponseAssetProcessorStatus::m_numberOfPendingJobs)
                    ->Field("IsAssetProcessorReady", &ResponseAssetProcessorStatus::m_isAssetProcessorReady);
            }
        }

        unsigned int ResponseAssetProcessorStatus::GetMessageType() const
        {
            return RequestAssetProcessorStatus::MessageType();
        }

        //---------------------------------------------------------------------
        unsigned int ResponseAssetStatus::GetMessageType() const
        {
            return RequestAssetStatus::MessageType();
        }

        void ResponseAssetStatus::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ResponseAssetStatus, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetStatus", &ResponseAssetStatus::m_assetStatus);
            }
        }

        //---------------------------------------------------------------------
        GetRelativeProductPathFromFullSourceOrProductPathRequest::GetRelativeProductPathFromFullSourceOrProductPathRequest(const AZ::OSString& sourceOrProductPath)
        {
            AZ_Assert(!sourceOrProductPath.empty(), "GetAssetIdRequest: asset path is empty");
            m_sourceOrProductPath = sourceOrProductPath;
        }

        unsigned int GetRelativeProductPathFromFullSourceOrProductPathRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::GetRelativeProductPathFromFullSourceOrProductPath", 0x53293a7a);
            return messageType;
        }

        unsigned int GetRelativeProductPathFromFullSourceOrProductPathRequest::GetMessageType() const
        {
            return MessageType();
        }

        void GetRelativeProductPathFromFullSourceOrProductPathRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetRelativeProductPathFromFullSourceOrProductPathRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("SourceOrProductPath", &GetRelativeProductPathFromFullSourceOrProductPathRequest::m_sourceOrProductPath);
            }
        }

        //---------------------------------------------------------------------
        GetRelativeProductPathFromFullSourceOrProductPathResponse::GetRelativeProductPathFromFullSourceOrProductPathResponse(bool resolved, const AZ::OSString& relativeProductPath)
        {
            m_relativeProductPath = relativeProductPath;
            m_resolved = resolved;
        }

        unsigned int GetRelativeProductPathFromFullSourceOrProductPathResponse::GetMessageType() const
        {
            return GetRelativeProductPathFromFullSourceOrProductPathRequest::MessageType();
        }

        //void GetAssetPathResponse::Reflect(AZ::ReflectContext* context)
        void GetRelativeProductPathFromFullSourceOrProductPathResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetRelativeProductPathFromFullSourceOrProductPathResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("RelativeProductPath", &GetRelativeProductPathFromFullSourceOrProductPathResponse::m_relativeProductPath)
                    ->Field("Resolved", &GetRelativeProductPathFromFullSourceOrProductPathResponse::m_resolved);
            }
        }

        //---------------------------------------------------------------------
        GetFullSourcePathFromRelativeProductPathRequest::GetFullSourcePathFromRelativeProductPathRequest(const AZ::OSString& relativeProductPath)
        {
            AZ_Assert(!relativeProductPath.empty(), "GetFullSourcePathFromRelativeProductPathRequest called with empty path");
            m_relativeProductPath = relativeProductPath;
        }

        unsigned int GetFullSourcePathFromRelativeProductPathRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::GetFullSourcePathFromRelativeProductPath", 0x08057afe);
            return messageType;
        }

        unsigned int GetFullSourcePathFromRelativeProductPathRequest::GetMessageType() const
        {
            return MessageType();
        }

        void GetFullSourcePathFromRelativeProductPathRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetFullSourcePathFromRelativeProductPathRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("RelativeProductPath", &GetFullSourcePathFromRelativeProductPathRequest::m_relativeProductPath);
            }
        }

        //---------------------------------------------------------------------
        GetFullSourcePathFromRelativeProductPathResponse::GetFullSourcePathFromRelativeProductPathResponse(bool resolved, const AZ::OSString& fullSourcePath)
        {
            m_fullSourcePath = fullSourcePath;
            m_resolved = resolved;
        }

        unsigned int GetFullSourcePathFromRelativeProductPathResponse::GetMessageType() const
        {
            return GetFullSourcePathFromRelativeProductPathRequest::MessageType();
        }

        void GetFullSourcePathFromRelativeProductPathResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<GetFullSourcePathFromRelativeProductPathResponse, BaseAssetProcessorMessage>()
                ->Version(2)
                ->Field("FullSourcePath", &GetFullSourcePathFromRelativeProductPathResponse::m_fullSourcePath)
                ->Field("Resolved", &GetFullSourcePathFromRelativeProductPathResponse::m_resolved);
            }
        }

        //---------------------------------------------------------------------
        SourceAssetInfoRequest::SourceAssetInfoRequest(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
            : m_assetId(assetId),
            m_assetType(assetType)
        {
        }

        SourceAssetInfoRequest::SourceAssetInfoRequest(const char* assetPath)
            : m_assetPath(assetPath)
        {
        }


        unsigned int SourceAssetInfoRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::SourceAssetInfoRequest", 0x350a86f3);
            return messageType;
        }

        unsigned int SourceAssetInfoRequest::GetMessageType() const
        {
            return MessageType();
        }

        void SourceAssetInfoRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SourceAssetInfoRequest, BaseAssetProcessorMessage>()
                    ->Version(2)
                    ->Field("AssetId", &SourceAssetInfoRequest::m_assetId)
                    ->Field("AssetType", &SourceAssetInfoRequest::m_assetType)
                    ->Field("AssetPath", &SourceAssetInfoRequest::m_assetPath);
            }
        }

        //---------------------------------------------------------------------
        SourceAssetInfoResponse::SourceAssetInfoResponse(const AZ::Data::AssetInfo& assetInfo, const char* rootFolder)
            : m_assetInfo(assetInfo)
            , m_rootFolder(rootFolder)
        {
        }

        unsigned int SourceAssetInfoResponse::GetMessageType() const
        {
            return SourceAssetInfoRequest::MessageType();
        }

        void SourceAssetInfoResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SourceAssetInfoResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Found", &SourceAssetInfoResponse::m_found)
                    ->Field("WatchFolder", &SourceAssetInfoResponse::m_rootFolder)
                    ->Field("AssetInfo", &SourceAssetInfoResponse::m_assetInfo);
            }
        }

        //---------------------------------------------------------------------
        AssetInfoRequest::AssetInfoRequest(const AZ::Data::AssetId& assetId)
            : m_assetId(assetId)
        {
        }

        AssetInfoRequest::AssetInfoRequest(const char* assetPath)
            : m_assetPath(assetPath)
        {
        }

        unsigned int AssetInfoRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::AssetInfoRequest", 0xfe3e020a);
            return messageType;
        }

        unsigned int AssetInfoRequest::GetMessageType() const
        {
            return MessageType();
        }

        void AssetInfoRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetInfoRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetId", &AssetInfoRequest::m_assetId)
                    ->Field("AssetPath", &AssetInfoRequest::m_assetPath);
            }
        }

        //---------------------------------------------------------------------
        AssetInfoResponse::AssetInfoResponse(const AZ::Data::AssetInfo& assetInfo)
            : m_assetInfo(assetInfo)
        {
        }

        unsigned int AssetInfoResponse::GetMessageType() const
        {
            return AssetInfoRequest::MessageType();
        }

        void AssetInfoResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetInfoResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Found", &AssetInfoResponse::m_found)
                    ->Field("AssetInfo", &AssetInfoResponse::m_assetInfo);
            }
        }

        //-----------------------------------------------------------------------------
        RegisterSourceAssetRequest::RegisterSourceAssetRequest(const AZ::Data::AssetType& assetType, const char* assetFileFilter)
            : m_assetType(assetType),
            m_assetFileFilter(assetFileFilter)
        {
        }

        unsigned int RegisterSourceAssetRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::RegisterSourceAssetRequest", 0x5f414e59);
            return messageType;
        }

        unsigned int RegisterSourceAssetRequest::GetMessageType() const
        {
            return MessageType();
        }

        void RegisterSourceAssetRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<RegisterSourceAssetRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetType", &RegisterSourceAssetRequest::m_assetType)
                    ->Field("AssetFileFilter", &RegisterSourceAssetRequest::m_assetFileFilter);
            }
        }

        //---------------------------------------------------------------------

        UnregisterSourceAssetRequest::UnregisterSourceAssetRequest(const AZ::Data::AssetType& assetType)
            : m_assetType(assetType)
        {
        }

        unsigned int UnregisterSourceAssetRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessor::UnregisterSourceAssetRequest", 0xfbe53ee1);
            return messageType;
        }

        unsigned int UnregisterSourceAssetRequest::GetMessageType() const
        {
            return MessageType();
        }

        void UnregisterSourceAssetRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<UnregisterSourceAssetRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("AssetType", &UnregisterSourceAssetRequest::m_assetType);
            }
        }

        //---------------------------------------------------------------------

        unsigned int ShowAssetProcessorRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::ShowWindow", 0x33a6cd1f);
            return messageType;
        }

        unsigned int ShowAssetProcessorRequest::GetMessageType() const
        {
            return MessageType();
        }

        void ShowAssetProcessorRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ShowAssetProcessorRequest>()
                    ->SerializeWithNoData();
            }
        }

        //---------------------------------------------------------------------

        unsigned int ShowAssetInAssetProcessorRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::ShowAssetInAssetProcessor", 0x3c9d1be0);
            return messageType;
        }

        unsigned int ShowAssetInAssetProcessorRequest::GetMessageType() const
        {
            return MessageType();
        }

        void ShowAssetInAssetProcessorRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ShowAssetInAssetProcessorRequest>()
                    ->Version(1)
                    ->Field("AssetPath", &ShowAssetInAssetProcessorRequest::m_assetPath);
            }
        }

        //---------------------------------------------------------------------
        FileOpenRequest::FileOpenRequest(const char* filePath, AZ::u32 mode)
            : m_filePath(filePath)
            , m_mode(mode)
        {
            AZ_Assert(filePath, "FileOpenRequest: Invalid/null filePath");
        }

        unsigned int FileOpenRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileOpen", 0x344f46ca);
            return messageType;
        }

        unsigned int FileOpenRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileOpenRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileOpenRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &FileOpenRequest::m_filePath)
                    ->Field("Mode", &FileOpenRequest::m_mode);
            }
        }

        //---------------------------------------------------------------------
        FileOpenResponse::FileOpenResponse(AZ::u32 fileHandle, AZ::u32 returnCode)
            : m_fileHandle(fileHandle)
            , m_returnCode(returnCode)
        {
        }

        unsigned int FileOpenResponse::GetMessageType() const
        {
            return FileOpenRequest::MessageType();
        }

        void FileOpenResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileOpenResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FileHandle", &FileOpenResponse::m_fileHandle)
                    ->Field("ResultCode", &FileOpenResponse::m_returnCode);
            }
        }

        //---------------------------------------------------------------------
        FileCloseRequest::FileCloseRequest(AZ::u32 fileHandle)
            : m_fileHandle(fileHandle)
        {
        }

        unsigned int FileCloseRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileClose", 0xb99bf25e);
            return messageType;
        }

        unsigned int FileCloseRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileCloseRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileCloseRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FileHandle", &FileCloseRequest::m_fileHandle);
            }
        }

        //---------------------------------------------------------------------
        FileReadRequest::FileReadRequest(AZ::u32 fileHandle, AZ::u64 bytesToRead, bool failOnFewerRead /* =false */)
            : m_fileHandle(fileHandle)
            , m_bytesToRead(bytesToRead)
            , m_failOnFewerRead(failOnFewerRead)
        {
            AZ_Assert(fileHandle != AZ::IO::InvalidHandle, "FileReadRequest: Invalid handle %u", fileHandle);
            //AZ_Assert(bytesToRead > 0, "FileReadRequest: It is invalid to request a read size of 0");
        }

        unsigned int FileReadRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileRead", 0x08688409);
            return messageType;
        }

        unsigned int FileReadRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileReadRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileReadRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FileHandle", &FileReadRequest::m_fileHandle)
                    ->Field("BytesToRead", &FileReadRequest::m_bytesToRead)
                    ->Field("FailOnFewerRead", &FileReadRequest::m_failOnFewerRead);
            }
        }

        //---------------------------------------------------------------------
        FileReadResponse::FileReadResponse(AZ::u32 resultCode, void* data, AZ::u64 dataLength)
            : m_resultCode(resultCode)
        {
            AZ_Assert((data == nullptr && dataLength == 0) || (data && dataLength > 0), "FileReadResponse: data buffer and data length do not match");
            if (data && dataLength > 0)
            {
                m_data.resize_no_construct(dataLength);
                memcpy(m_data.data(), data, dataLength);
            }
        }

        unsigned int FileReadResponse::GetMessageType() const
        {
            return FileReadRequest::MessageType();
        }

        void FileReadResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileReadResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileReadResponse::m_resultCode)
                    ->Field("Data", &FileReadResponse::m_data);
            }
        }

        //---------------------------------------------------------------------
        FileWriteRequest::FileWriteRequest(AZ::u32 fileHandle, const void* data, AZ::u64 dataLength)
            : m_fileHandle(fileHandle)
        {
            AZ_Assert(fileHandle != AZ::IO::InvalidHandle, "FileWriteRequest: Invalid handle %u", fileHandle);
            if (data && dataLength > 0)
            {
                m_data.resize_no_construct(dataLength);
                memcpy(m_data.data(), data, dataLength);
            }
        }

        unsigned int FileWriteRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileWrite", 0xd7f109c5);
            return messageType;
        }

        unsigned int FileWriteRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileWriteRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileWriteRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FileHandle", &FileWriteRequest::m_fileHandle)
                    ->Field("Data", &FileWriteRequest::m_data);
            }
        }

        //---------------------------------------------------------------------
        FileWriteResponse::FileWriteResponse(AZ::u32 resultCode, AZ::u64 bytesWritten)
            : m_resultCode(resultCode)
            , m_bytesWritten(bytesWritten)
        {
        }

        unsigned int FileWriteResponse::GetMessageType() const
        {
            return FileWriteRequest::MessageType();
        }

        void FileWriteResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileWriteResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileWriteResponse::m_resultCode)
                    ->Field("BytesWritten", &FileWriteResponse::m_bytesWritten);
            }
        }

        //---------------------------------------------------------------------
        FileTellRequest::FileTellRequest(AZ::u32 fileHandle)
            : m_fileHandle(fileHandle)
        {
            AZ_Assert(fileHandle != AZ::IO::InvalidHandle, "FileTellRequest: Invalid handle: %u", fileHandle);
        }

        unsigned int FileTellRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileTell", 0x96762daa);
            return messageType;
        }

        unsigned int FileTellRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileTellRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileTellRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FileHandle", &FileTellRequest::m_fileHandle);
            }
        }

        //---------------------------------------------------------------------
        FileTellResponse::FileTellResponse(AZ::u32 resultCode, AZ::u64 offset)
            : m_resultCode(resultCode)
            , m_offset(offset)
        {
        }

        unsigned int FileTellResponse::GetMessageType() const
        {
            return FileTellRequest::MessageType();
        }

        void FileTellResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileTellResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileTellResponse::m_resultCode)
                    ->Field("Offset", &FileTellResponse::m_offset);
            }
        }

        //---------------------------------------------------------------------
        FileSeekRequest::FileSeekRequest(AZ::u32 fileHandle, AZ::u32 mode, AZ::s64 offset)
            : m_fileHandle(fileHandle)
            , m_seekMode(mode)
            , m_offset(offset)
        {
            AZ_Assert(fileHandle != AZ::IO::InvalidHandle, "FileSeekRequest: invalid file handle: %u", fileHandle);
        }

        unsigned int FileSeekRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileSeek", 0x44073bf9);
            return messageType;
        }

        unsigned int FileSeekRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileSeekRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileSeekRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FileHandle", &FileSeekRequest::m_fileHandle)
                    ->Field("Mode", &FileSeekRequest::m_seekMode)
                    ->Field("Offset", &FileSeekRequest::m_offset);
            }
        }

        //---------------------------------------------------------------------
        FileSeekResponse::FileSeekResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int FileSeekResponse::GetMessageType() const
        {
            return FileSeekRequest::MessageType();
        }

        void FileSeekResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileSeekResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileSeekResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        FileIsReadOnlyRequest::FileIsReadOnlyRequest(const AZ::OSString& filePath)
            : m_filePath(filePath)
        {
            AZ_Assert(!filePath.empty(), "FileIsReadOnlyRequest: empty file path");
        }

        unsigned int FileIsReadOnlyRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::IsReadOnly", 0x6ee110cf);
            return messageType;
        }

        unsigned int FileIsReadOnlyRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileIsReadOnlyRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileIsReadOnlyRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &FileIsReadOnlyRequest::m_filePath);
            }
        }

        //---------------------------------------------------------------------
        FileIsReadOnlyResponse::FileIsReadOnlyResponse(bool isReadOnly)
            : m_isReadOnly(isReadOnly)
        {
        }

        unsigned int FileIsReadOnlyResponse::GetMessageType() const
        {
            return FileIsReadOnlyRequest::MessageType();
        }

        void FileIsReadOnlyResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileIsReadOnlyResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("IsReadOnly", &FileIsReadOnlyResponse::m_isReadOnly);
            }
        }

        //---------------------------------------------------------------------
        PathIsDirectoryRequest::PathIsDirectoryRequest(const AZ::OSString& path)
            : m_path(path)
        {
            AZ_Assert(!path.empty(), "PathIsDirectoryRequest: empty path");
        }

        unsigned int PathIsDirectoryRequest::MessageType()
        {
            unsigned int messageType = AZ_CRC("AssetSystem::IsDirectory", 0xfe975e0e);
            return messageType;
        }

        unsigned int PathIsDirectoryRequest::GetMessageType() const
        {
            return MessageType();
        }

        void PathIsDirectoryRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PathIsDirectoryRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Path", &PathIsDirectoryRequest::m_path);
            }
        }

        //---------------------------------------------------------------------
        PathIsDirectoryResponse::PathIsDirectoryResponse(bool isDir)
            : m_isDir(isDir)
        {
        }

        unsigned int PathIsDirectoryResponse::GetMessageType() const
        {
            return PathIsDirectoryRequest::MessageType();
        }

        void PathIsDirectoryResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PathIsDirectoryResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("IsDirectory", &PathIsDirectoryResponse::m_isDir);
            }
        }

        //---------------------------------------------------------------------
        FileSizeRequest::FileSizeRequest(const AZ::OSString& filePath)
            : m_filePath(filePath)
        {
            AZ_Assert(!filePath.empty(), "FileSizeRequest: empty file path");
        }

        unsigned int FileSizeRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileSize", 0x67ffe104);
            return messageType;
        }

        unsigned int FileSizeRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileSizeRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileSizeRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &FileSizeRequest::m_filePath);
            }
        }

        //---------------------------------------------------------------------
        FileSizeResponse::FileSizeResponse(AZ::u32 resultCode, AZ::u64 size)
            : m_resultCode(resultCode)
            , m_size(size)
        {
        }

        unsigned int FileSizeResponse::GetMessageType() const
        {
            return FileSizeRequest::MessageType();
        }

        void FileSizeResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileSizeResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileSizeResponse::m_resultCode)
                    ->Field("Size", &FileSizeResponse::m_size);
            }
        }

        //---------------------------------------------------------------------
        FileModTimeRequest::FileModTimeRequest(const AZ::OSString& filePath)
            : m_filePath(filePath)
        {
            AZ_Assert(!filePath.empty(), "FileModTimeRequest: empty file path");
        }

        unsigned int FileModTimeRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileModTime", 0xee6f318c);
            return messageType;
        }

        unsigned int FileModTimeRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileModTimeRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileModTimeRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &FileModTimeRequest::m_filePath);
            }
        }

        //---------------------------------------------------------------------
        FileModTimeResponse::FileModTimeResponse(AZ::u64 modTime)
            : m_modTime(modTime)
        {
        }

        unsigned int FileModTimeResponse::GetMessageType() const
        {
            return FileModTimeRequest::MessageType();
        }

        void FileModTimeResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileModTimeResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ModTime", &FileModTimeResponse::m_modTime);
            }
        }

        //---------------------------------------------------------------------
        FileExistsRequest::FileExistsRequest(const AZ::OSString& filePath)
            : m_filePath(filePath)
        {
            AZ_Assert(!filePath.empty(), "FileExistsRequest: empty file path");
        }

        unsigned int FileExistsRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileExists", 0xa2ef8699);
            return messageType;
        }

        unsigned int FileExistsRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileExistsRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileExistsRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &FileExistsRequest::m_filePath);
            }
        }

        //---------------------------------------------------------------------
        FileExistsResponse::FileExistsResponse(bool exists)
            : m_exists(exists)
        {
        }

        unsigned int FileExistsResponse::GetMessageType() const
        {
            return FileExistsRequest::MessageType();
        }

        void FileExistsResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileExistsResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Exists", &FileExistsResponse::m_exists);
            }
        }

        //---------------------------------------------------------------------
        FileFlushRequest::FileFlushRequest(AZ::u32 fileHandle)
            : m_fileHandle(fileHandle)
        {
            AZ_Assert(fileHandle != AZ::IO::InvalidHandle, "FileFlushRequest: invalid file handle: %u", fileHandle);
        }

        unsigned int FileFlushRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileFlush", 0x1e7b2735);
            return messageType;
        }

        unsigned int FileFlushRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileFlushRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileFlushRequest, BaseAssetProcessorMessage>()
                ->Version(1)
                ->Field("FileHandle", &FileFlushRequest::m_fileHandle);
            }
        }

        //---------------------------------------------------------------------
        FileFlushResponse::FileFlushResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int FileFlushResponse::GetMessageType() const
        {
            return FileFlushRequest::MessageType();
        }

        void FileFlushResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileFlushResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileFlushResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        PathCreateRequest::PathCreateRequest(const AZ::OSString& path)
            : m_path(path)
        {
            AZ_Assert(!path.empty(), "PathCreateRequest: path is empty");
        }

        unsigned int PathCreateRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::PathCreate", 0xc047183b);
            return messageType;
        }

        unsigned int PathCreateRequest::GetMessageType() const
        {
            return MessageType();
        }

        void PathCreateRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PathCreateRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Path", &PathCreateRequest::m_path);
            }
        }

        //---------------------------------------------------------------------
        PathCreateResponse::PathCreateResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int PathCreateResponse::GetMessageType() const
        {
            return PathCreateRequest::MessageType();
        }

        void PathCreateResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PathCreateResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &PathCreateResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        PathDestroyRequest::PathDestroyRequest(const AZ::OSString& path)
            : m_path(path)
        {
            AZ_Assert(!path.empty(), "PathDestroyRequest: path is empty");
        }

        unsigned int PathDestroyRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::PathDestroy", 0xe761d533);
            return messageType;
        }

        unsigned int PathDestroyRequest::GetMessageType() const
        {
            return MessageType();
        }

        void PathDestroyRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PathDestroyRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Path", &PathDestroyRequest::m_path);
            }
        }

        //---------------------------------------------------------------------
        PathDestroyResponse::PathDestroyResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int PathDestroyResponse::GetMessageType() const
        {
            return PathDestroyRequest::MessageType();
        }

        void PathDestroyResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PathDestroyResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &PathDestroyResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        FileRemoveRequest::FileRemoveRequest(const AZ::OSString& filePath)
            : m_filePath(filePath)
        {
            AZ_Assert(!filePath.empty(), "FileRemoveRequest: path is empty");
        }

        unsigned int FileRemoveRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileRemove", 0x78f0fd19);
            return messageType;
        }

        unsigned int FileRemoveRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileRemoveRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileRemoveRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("FilePath", &FileRemoveRequest::m_filePath);
            }
        }

        //---------------------------------------------------------------------
        FileRemoveResponse::FileRemoveResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int FileRemoveResponse::GetMessageType() const
        {
            return FileRemoveRequest::MessageType();
        }

        void FileRemoveResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileRemoveResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileRemoveResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        FileCopyRequest::FileCopyRequest(const AZ::OSString& srcPath, const AZ::OSString& destPath)
            : m_srcPath(srcPath)
            , m_destPath(destPath)
        {
            AZ_Assert(!srcPath.empty(), "Source path is empty");
            AZ_Assert(!destPath.empty(), "Destination path is empty");
        }

        unsigned int FileCopyRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FileCopy", 0xdd857eec);
            return messageType;
        }

        unsigned int FileCopyRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileCopyRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileCopyRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("SourcePath", &FileCopyRequest::m_srcPath)
                    ->Field("DestPath", &FileCopyRequest::m_destPath);
            }
        }

        //---------------------------------------------------------------------
        FileCopyResponse::FileCopyResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int FileCopyResponse::GetMessageType() const
        {
            return FileCopyRequest::MessageType();
        }

        void FileCopyResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileCopyResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileCopyResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        FileRenameRequest::FileRenameRequest(const AZ::OSString& srcPath, const AZ::OSString& destPath)
            : m_srcPath(srcPath)
            , m_destPath(destPath)
        {
            AZ_Assert(!srcPath.empty(), "Source path is empty");
            AZ_Assert(!destPath.empty(), "Destination path is empty");
        }

        unsigned int FileRenameRequest::MessageType()
        {
            unsigned int messageType = AZ_CRC("AssetSystem::FileRename", 0xc9edb467);
            return messageType;
        }

        unsigned int FileRenameRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FileRenameRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileRenameRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("SourcePath", &FileRenameRequest::m_srcPath)
                    ->Field("DestPath", &FileRenameRequest::m_destPath);
            }
        }

        //---------------------------------------------------------------------
        FileRenameResponse::FileRenameResponse(AZ::u32 resultCode)
            : m_resultCode(resultCode)
        {
        }

        unsigned int FileRenameResponse::GetMessageType() const
        {
            return FileRenameRequest::MessageType();
        }

        void FileRenameResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FileRenameResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FileRenameResponse::m_resultCode);
            }
        }

        //---------------------------------------------------------------------
        FindFilesRequest::FindFilesRequest(const AZ::OSString& path, const AZ::OSString& filter)
            : m_path(path)
            , m_filter(filter)
        {
            AZ_Assert(!path.empty(), "FindFilesRequest: empty path");
            AZ_Assert(!filter.empty(), "FindFilesRequest: empty filter");
        }

        unsigned int FindFilesRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetSystem::FindFiles", 0xf06cf14f);
            return messageType;
        }

        unsigned int FindFilesRequest::GetMessageType() const
        {
            return MessageType();
        }

        void FindFilesRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FindFilesRequest, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("Path", &FindFilesRequest::m_path)
                    ->Field("Filter", &FindFilesRequest::m_filter);
            }
        }

        //---------------------------------------------------------------------
        FindFilesResponse::FindFilesResponse(AZ::u32 resultCode, const FileList& files)
            : m_resultCode(resultCode)
            , m_files(files)
        {
        }

        unsigned int FindFilesResponse::GetMessageType() const
        {
            return FindFilesRequest::MessageType();
        }

        void FindFilesResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<FindFilesResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("ResultCode", &FindFilesResponse::m_resultCode)
                    ->Field("Files", &FindFilesResponse::m_files);
            }
        }

        //---------------------------------------------------------------------------
        AssetNotificationMessage::AssetNotificationMessage(const AZ::OSString& data, NotificationType type, const AZ::Data::AssetType& assetType)
            : m_data(data)
            , m_type(type)
            , m_assetType(assetType)
        {
            AZ_Assert(!data.empty(), "AssetNotificationMessage: empty string");
        }

        unsigned int AssetNotificationMessage::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessorManager::AssetNotification", 0xd6191df5);
            return messageType;
        }

        unsigned int AssetNotificationMessage::GetMessageType() const
        {
            return MessageType();
        }

        void AssetNotificationMessage::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<AssetNotificationMessage, BaseAssetProcessorMessage>()
                    ->Version(5)
                    ->Field("StringData", &AssetNotificationMessage::m_data)
                    ->Field("NotificationType", &AssetNotificationMessage::m_type)
                    ->Field("size", &AssetNotificationMessage::m_sizeBytes)
                    ->Field("assetId", &AssetNotificationMessage::m_assetId)
                    ->Field("assetType", &AssetNotificationMessage::m_assetType)
                    ->Field("legacyAssetIds", &AssetNotificationMessage::m_legacyAssetIds)
                    ->Field("dependencies", &AssetNotificationMessage::m_dependencies);
            }
        }

        //----------------------------------------------------------------------------
        unsigned int SaveAssetCatalogRequest::MessageType()
        {
            static unsigned int messageType = AZ_CRC("AssetProcessorManager::SaveCatalogRequest", 0x79e0f02f);
            return messageType;
        }

        unsigned int SaveAssetCatalogRequest::GetMessageType() const
        {
            return MessageType();
        }

        void SaveAssetCatalogRequest::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SaveAssetCatalogRequest>()
                    ->SerializeWithNoData();
            }
        }

        //----------------------------------------------------------------------------

        unsigned int SaveAssetCatalogResponse::GetMessageType() const
        {
            return SaveAssetCatalogRequest::MessageType();
        }

        void SaveAssetCatalogResponse::Reflect(AZ::ReflectContext* context)
        {
            auto serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<SaveAssetCatalogResponse, BaseAssetProcessorMessage>()
                    ->Version(1)
                    ->Field("CatalogSaved", &SaveAssetCatalogResponse::m_saved);
            }
        }
    } // namespace AssetSystem
} // namespace AzFramework
