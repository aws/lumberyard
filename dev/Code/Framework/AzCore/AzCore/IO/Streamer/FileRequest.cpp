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

#include <limits>
#include <AzCore/IO/Streamer/FileRequest.h>

namespace AZ
{
    namespace IO
    {
        FileRequest::FileRequest(Usage usage)
            : m_usage(usage)
        {
        }
        
        FileRequest::RequestLinkData::~RequestLinkData()
        {
            m_request.reset();
        }

        FileRequest::~FileRequest()
        {
            Reset();
        }

        void FileRequest::CreateRequestLink(AZStd::shared_ptr<Request>&& m_request)
        {
            AZ_Assert(m_operation == Operation::None, "Attempting to set FileRequest to 'RequestLink', but another task was already assigned.");
            RequestLinkData* data = new(m_payload) RequestLinkData();
            data->m_request = m_request;
            m_operation = Operation::RequestLink;
        }

        void FileRequest::CreateRead(StreamStackEntry* owner, FileRequest* parent, void* output, const RequestPath& path, u64 offset, u64 size)
        {
            AZ_Assert(m_operation == Operation::None, "Attempting to set FileRequest to 'Read', but another task was already assigned.");
            ReadData* data = new(m_payload) ReadData();
            data->m_output = output;
            data->m_path = &path;
            data->m_offset = offset;
            data->m_size = size;
            m_operation = Operation::Read;
            m_parent = parent;
            m_owner = owner;
            if (parent)
            {
                AZ_Assert(parent->m_dependencies < std::numeric_limits<decltype(parent->m_dependencies)>::max(), 
                    "A file request dependency was added, but the parent can't have any more dependencies.");
                ++parent->m_dependencies;
            }
        }

        void FileRequest::CreateCompressedRead(StreamStackEntry* owner, FileRequest* parent, const CompressionInfo& compressionInfo, 
            void* output, u64 readOffset, u64 readSize)
        {
            CreateCompressedRead(owner, parent, CompressionInfo(compressionInfo), output, readOffset, readSize);
        }

        void FileRequest::CreateCompressedRead(StreamStackEntry* owner, FileRequest* parent, CompressionInfo&& compressionInfo,
            void* output, u64 readOffset, u64 readSize)
        {
            AZ_Assert(m_operation == Operation::None, "Attempting to set FileRequest to 'CompressedRead', but another task was already assigned.");
            CompressedReadData* data = new(m_payload) CompressedReadData();
            data->m_compressionInfo = AZStd::move(compressionInfo);
            data->m_output = output;
            data->m_readOffset = readOffset;
            data->m_readSize = readSize;

            m_operation = Operation::CompressedRead;
            m_parent = parent;
            m_owner = owner;
            if (parent)
            {
                AZ_Assert(parent->m_dependencies < std::numeric_limits<decltype(parent->m_dependencies)>::max(),
                    "A file request dependency was added, but the parent can't have any more dependencies.");
                ++parent->m_dependencies;
            }
        }

        void FileRequest::CreateWait(StreamStackEntry* owner, FileRequest* parent)
        {
            AZ_Assert(parent, "A parent is needed to create a read operation with FileRequest.");
            AZ_Assert(m_operation == Operation::None, "Attempting to set FileRequest to 'Read', but another task was already assigned.");
            m_operation = Operation::Wait;
            m_parent = parent;
            m_owner = owner;
            AZ_Assert(parent->m_dependencies < std::numeric_limits<decltype(parent->m_dependencies)>::max(),
                "A file request dependency was added, but the parent can't have any more dependencies.");
            ++parent->m_dependencies;
        }

        void FileRequest::CreateCancel(FileRequest* target)
        {
            AZ_Assert(m_operation == Operation::None, "Attempting to set FileRequest to 'Cancel', but another task was already assigned.");
            CancelData* data = new(m_payload) CancelData();
            data->m_target = target;
            m_operation = Operation::Cancel;
        }

        FileRequest::RequestLinkData& FileRequest::GetRequestLinkData()
        {
            AZ_Assert(m_operation == Operation::RequestLink, "Can't retrieve a request link from an operation of type %i.", m_operation);
            return *reinterpret_cast<RequestLinkData*>(m_payload);
        }

        FileRequest::ReadData& FileRequest::GetReadData()
        {
            AZ_Assert(m_operation == Operation::Read, "Can't retrieve read data from an operation of type %i.", m_operation);
            return *reinterpret_cast<ReadData*>(m_payload);
        }

        FileRequest::CompressedReadData& FileRequest::GetCompressedReadData()
        {
            AZ_Assert(m_operation == Operation::CompressedRead, "Can't retrieve compressed read data from an operation of type %i.", m_operation);
            return *reinterpret_cast<CompressedReadData*>(m_payload);
        }

        FileRequest* FileRequest::GetCanceledRequest()
        {
            AZ_Assert(m_operation == Operation::Cancel, "Can't retrieve the canceled request from an operation of type %i.", m_operation);
            return reinterpret_cast<CancelData*>(m_payload)->m_target;
        }

        FileRequest::Status FileRequest::GetStatus() const
        {
            return m_status;
        }

        void FileRequest::SetStatus(Status newStatus)
        {
            Status currentStatus = m_status;
            switch (newStatus)
            {
            case Status::Pending:
                // Fall through
            case Status::Queued:
                // Fall through
            case Status::Processing:
                if (currentStatus == Status::Failed || 
                    currentStatus == Status::Canceled ||
                    currentStatus == Status::Completed)
                {
                    return;
                }
            case Status::Completed:
                if (currentStatus == Status::Failed || currentStatus == Status::Canceled)
                {
                    return;
                }
            case Status::Canceled:
                if (currentStatus == Status::Failed || currentStatus == Status::Completed)
                {
                    return;
                }
            case Status::Failed:
                // fall through
            default:
                break;
            }
            m_status = newStatus;
        }

        StreamStackEntry* FileRequest::GetOwner()
        {
            return m_owner;
        }

        FileRequest* FileRequest::GetParent()
        {
            return m_parent;
        }

        FileRequest::Operation FileRequest::GetOperationType() const
        {
            return m_operation;
        }

        void FileRequest::Reset()
        {
            switch (m_operation)
            {
            case Operation::None:
                break;
            case Operation::RequestLink:
                reinterpret_cast<RequestLinkData*>(m_payload)->~RequestLinkData();
                break;
            case Operation::Read:
                reinterpret_cast<ReadData*>(m_payload)->~ReadData();
                break;
            case Operation::CompressedRead:
                reinterpret_cast<CompressedReadData*>(m_payload)->~CompressedReadData();
                break;
            case Operation::Wait:
                reinterpret_cast<CancelData*>(m_payload)->~CancelData();
                break;
            default:
                AZ_Assert(false, "Unknown operation type '%i' in FileRequest.", m_operation);
                break;
            }
            m_parent = nullptr;
            m_owner = nullptr;
            m_status = Status::Pending;
            m_operation = Operation::None;
            m_dependencies = 0;
        }

        bool FileRequest::IsChildOf(FileRequest* parent) const
        {
            FileRequest* current = m_parent;
            while (current)
            {
                if (current == parent)
                {
                    return true;
                }
                current = current->m_parent;
            }
            return false;
        }

        void FileRequest::SetEstimatedCompletion(AZStd::chrono::system_clock::time_point time)
        {
            FileRequest* current = this;
            do
            {
                current->m_estimatedCompletion = time;
                current = current->m_parent;
            } while (current);
        }

        AZStd::chrono::system_clock::time_point FileRequest::GetEstimatedCompletion() const
        {
            return m_estimatedCompletion;
        }
    } // namespace IO
} // namespace AZ