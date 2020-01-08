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

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ
{
    namespace IO
    {
        class StreamStackEntryMock
            : public StreamStackEntry
        {
        public:
            StreamStackEntryMock() : StreamStackEntry("StreamStackEntryMock") {}
            ~StreamStackEntryMock() override = default;

            MOCK_CONST_METHOD0(GetName, const AZStd::string&());
            
            MOCK_METHOD1(SetNext, void(AZStd::shared_ptr<StreamStackEntry>));
            MOCK_CONST_METHOD0(GetNext, AZStd::shared_ptr<StreamStackEntry>());

            MOCK_METHOD1(SetContext, void(StreamerContext&));
            
            MOCK_METHOD1(PrepareRequest, void(FileRequest*));
            MOCK_METHOD0(ExecuteRequests, bool());
            MOCK_METHOD1(FinalizeRequest, void(FileRequest*));
            MOCK_CONST_METHOD0(GetAvailableRequestSlots, s32());
            MOCK_METHOD3(UpdateCompletionEstimates, void(AZStd::chrono::system_clock::time_point, AZStd::vector<FileRequest*>&, AZStd::deque<AZStd::shared_ptr<Request>>&));

            MOCK_METHOD2(GetFileSize, bool(u64&, const RequestPath&));
            
            MOCK_METHOD1(FlushCache, void(const RequestPath&));
            MOCK_METHOD0(FlushEntireCache, void());
            MOCK_METHOD2(CreateDedicatedCache, void(const RequestPath&, const FileRange&));
            MOCK_METHOD2(DestroyDedicatedCache, void(const RequestPath&, const FileRange&));
            
            MOCK_CONST_METHOD1(CollectStatistics, void(AZStd::vector<Statistic>&));
        };
    } // namespace IO
} // namespace AZ
