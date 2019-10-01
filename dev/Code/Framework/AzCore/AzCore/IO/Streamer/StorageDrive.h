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
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/chrono/clocks.h>

namespace AZ
{
    namespace IO
    {
        //! Platform agnostic version of a storage drive, such as hdd, ssd, dvd, etc.
        //! This stream stack entry is responsible for accessing a storage drive to 
        //! retrieve file information and data.
        //! This entry is designed as a catch-all for any reads that weren't handled
        //! by platform specific implementations or the virtual file system. It should
        //! by the last entry in the stack as it will not forward calls to the next entry.
        class StorageDrive
            : public StreamStackEntry
        {
        public:
            explicit StorageDrive(u32 maxFileHandles);
            ~StorageDrive() override = default;

            void SetNext(AZStd::shared_ptr<StreamStackEntry> next) override;

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
            static constexpr s32 s_maxRequests = 1;

            size_t FindFileInCache(const RequestPath& filePath) const;
            void ReadFile(FileRequest* request);
            void CancelRequest(FileRequest* request);

            TimedAverageWindow<s_statisticsWindowSize> m_fileOpenCloseTimeAverage;
            TimedAverageWindow<s_statisticsWindowSize> m_readTimeAverage;
            AverageWindow<u64, float, s_statisticsWindowSize> m_readSizeAverage;
            //! File requests that are queued for processing.
            AZStd::deque<FileRequest*> m_pendingRequests;
            //! The last time a file handle was used to access a file. The handle is stored in m_fileHandles.
            AZStd::vector<AZStd::chrono::system_clock::time_point> m_fileLastUsed;
            //! The file path to the file handle. The handle is stored in m_fileHandles.
            AZStd::vector<RequestPath> m_filePaths;
            //! A list of file handles that's being cached in case they're needed again in the future.
            AZStd::vector<AZStd::unique_ptr<SystemFile>> m_fileHandles;
            //! The offset into the file that's cached by the active cache slot.
            u64 m_activeOffset = 0;
            //! The index into m_fileHandles for the file that's currently being read.
            size_t m_activeCacheSlot = s_fileNotFound;
        };
    } // namespace IO
} // namespace AZ