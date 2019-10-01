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

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/ReadSplitter.h>
#include <AzCore/IO/Streamer/StreamerContext.h>

namespace AZ
{
    namespace IO
    {
        ReadSplitter::ReadSplitter(u64 maxReadSize)
            : StreamStackEntry("Read splitter")
            , m_maxReadSize(maxReadSize)
        {
        }

        void ReadSplitter::PrepareRequest(FileRequest* request)
        {
            AZ_Assert(request, "PrepareRequest was provided a null request.");
            if (!m_next)
            {
                request->SetStatus(FileRequest::Status::Failed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            if (request->GetOperationType() != FileRequest::Operation::Read)
            {
                StreamStackEntry::PrepareRequest(request);
                return;
            }

            FileRequest::ReadData& data = request->GetReadData();
            if (data.m_size <= m_maxReadSize)
            {
                m_averageNumSubReads.PushEntry(1);
                StreamStackEntry::PrepareRequest(request);
                return;
            }

            u64 offset = data.m_offset;
            u64 size = data.m_size;
            u8* output = reinterpret_cast<u8*>(data.m_output);

            u64 numSubReads = 0;
            while (size > 0)
            {
                u64 readSize = AZStd::min(size, m_maxReadSize);

                FileRequest* mainRequest = m_context->GetNewInternalRequest();
                // Don't assign ourselves as the owner as there's nothing to do on callback.
                mainRequest->CreateRead(nullptr, request, output, *data.m_path, offset, readSize);
                m_next->PrepareRequest(mainRequest);

                offset += readSize;
                size -= readSize;
                output += readSize;
                numSubReads++;
            }
            m_averageNumSubReads.PushEntry(numSubReads);
        }

        void ReadSplitter::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            statistics.push_back(Statistic::CreateFloat(m_name, "Avg. num sub reads", m_averageNumSubReads.CalculateAverage()));
            StreamStackEntry::CollectStatistics(statistics);
        }
    } // namespace IO
} // namesapce AZ