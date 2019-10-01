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

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/clocks.h>
#include "RemoteFileIO.h"

namespace AZ
{
    namespace IO
    {
        class RemoteStorageDrive
            : public StreamStackEntry
        {
        public:
            explicit RemoteStorageDrive(u32 maxFileHandles);
            ~RemoteStorageDrive() override;

            void PrepareRequest(FileRequest* request) override;
            bool ExecuteRequests() override;
            s32 GetAvailableRequestSlots() const override;
            void UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
                AZStd::deque<AZStd::shared_ptr<Request>>& externalPending) override;

            bool GetFileSize(u64& result, const RequestPath& filePath) override;

            void FlushCache(const RequestPath& filePath) override;
            void FlushEntireCache() override;

            void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

        protected:
            static constexpr size_t s_fileNotFound = static_cast<size_t>(-1);
            static constexpr s32 s_maxRequests = 1;
            
            void ReadFile(FileRequest* request);
            bool CancelRequest(FileRequest* request);
            size_t FindFileInCache(const RequestPath& filePath) const;

            RemoteFileIO m_fileIO;
            TimedAverageWindow<s_statisticsWindowSize> m_fileOpenCloseTimeAverage;
            TimedAverageWindow<s_statisticsWindowSize> m_readTimeAverage;
            AverageWindow<u64, float, s_statisticsWindowSize> m_readSizeAverage;
            AZStd::deque<FileRequest*> m_pendingRequests;
            AZStd::vector<AZStd::chrono::system_clock::time_point> m_fileLastUsed;
            AZStd::vector<RequestPath> m_filePaths;
            AZStd::vector<HandleType> m_fileHandles;
            size_t m_activeCacheSlot = s_fileNotFound;
        };
    } // namespace IO
} // namespace AZ