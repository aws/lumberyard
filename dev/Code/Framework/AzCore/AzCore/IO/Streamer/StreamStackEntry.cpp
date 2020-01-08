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
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ
{
    namespace IO
    {
        StreamStackEntry::StreamStackEntry(AZStd::string&& name)
            : m_name(AZStd::move(name))
        {
        }

        const AZStd::string& StreamStackEntry::GetName() const
        {
            return m_name;
        }

        void StreamStackEntry::SetNext(AZStd::shared_ptr<StreamStackEntry> next)
        {
            m_next = AZStd::move(next);
        }

        AZStd::shared_ptr<StreamStackEntry> StreamStackEntry::GetNext() const
        {
            return m_next;
        }

        void StreamStackEntry::SetContext(StreamerContext& context)
        {
            m_context = &context;
            if (m_next)
            {
                m_next->SetContext(context);
            }
        }

        void StreamStackEntry::PrepareRequest(FileRequest* request)
        {
            if (m_next)
            {
                m_next->PrepareRequest(request);
            }
            else
            {
                request->SetStatus(FileRequest::Status::Failed);
                m_context->MarkRequestAsCompleted(request);
            }
        }

        bool StreamStackEntry::ExecuteRequests()
        {
            if (m_next)
            {
                return m_next->ExecuteRequests();
            }
            else
            {
                return false;
            }
        }

        s32 StreamStackEntry::GetAvailableRequestSlots() const
        {
            if (m_next)
            {
                return m_next->GetAvailableRequestSlots();
            }
            else
            {
                return std::numeric_limits<s32>::max();
            }
        }

        void StreamStackEntry::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            AZStd::deque<AZStd::shared_ptr<Request>>& externalPending)
        {
            if (m_next)
            {
                m_next->UpdateCompletionEstimates(now, internalPending, externalPending);
            }
        }

        void StreamStackEntry::FinalizeRequest(FileRequest*)
        {
            // Finalization is called on a specific node, so doesn't need to be forwarded.
        }

        bool StreamStackEntry::GetFileSize(u64& result, const RequestPath& filePath)
        {
            return m_next ? m_next->GetFileSize(result, filePath) : false;
        }

        void StreamStackEntry::FlushCache(const RequestPath& filePath)
        {
            if (m_next)
            {
                m_next->FlushCache(filePath);
            }
        }

        void StreamStackEntry::FlushEntireCache()
        {
            if (m_next)
            {
                m_next->FlushEntireCache();
            }
        }

        void StreamStackEntry::CreateDedicatedCache(const RequestPath& filePath, const FileRange& range)
        {
            if (m_next)
            {
                m_next->CreateDedicatedCache(filePath, range);
            }
        }

        void StreamStackEntry::DestroyDedicatedCache(const RequestPath& filePath, const FileRange& range)
        {
            if (m_next)
            {
                m_next->DestroyDedicatedCache(filePath, range);
            }
        }

        void StreamStackEntry::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            if (m_next)
            {
                m_next->CollectStatistics(statistics);
            }
        }
    } // namespace IO
} // namespace AZ