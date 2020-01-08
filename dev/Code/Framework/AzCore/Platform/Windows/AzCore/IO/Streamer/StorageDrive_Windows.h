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

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace IO
    {
        class StorageDriveWin
            : public StreamStackEntry
        {
        public:
            StorageDriveWin(const AZStd::vector<const char*>& drivePaths, bool hasSeekPenalty, u32 maxFileHandles);
            ~StorageDriveWin() override;

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
            static const AZStd::chrono::microseconds s_averageSeekTime;
            static constexpr size_t s_fileNotFound = static_cast<size_t>(-1);

            void ReadRequest(FileRequest* request);
            bool CancelRequest(FileRequest* request);
            size_t FindFileInCache(const RequestPath& filePath) const;
            bool IsServicedByThisDrive(const char* filePath) const;
            
            TimedAverageWindow<s_statisticsWindowSize> m_fileOpenCloseTimeAverage;
            TimedAverageWindow<s_statisticsWindowSize> m_readTimeAverage;
            AverageWindow<u64, float, s_statisticsWindowSize> m_readSizeAverage;
            AZStd::deque<FileRequest*> m_pendingRequests;
            AZStd::vector<AZStd::chrono::system_clock::time_point> m_fileLastUsed;
            AZStd::vector<RequestPath> m_filePaths;
            AZStd::vector<HANDLE> m_fileHandles;
            AZStd::vector<AZStd::string> m_drivePaths;
            size_t m_activeCacheSlot = s_fileNotFound;
            u64 m_activeOffset = 0;
            u32 m_maxFileHandles = 1;
            bool m_cachesInitialized = false;
            bool m_hasSeekPenalty = true;
        };
    } // namespace IO
} // namespace AZ