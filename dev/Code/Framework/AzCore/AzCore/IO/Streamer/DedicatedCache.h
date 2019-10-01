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

#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/FileRange.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace IO
    {
        class DedicatedCache
            : public StreamStackEntry
        {
        public:
            DedicatedCache(u64 cacheSize, u32 blockSize, bool onlyEpilogWrites);
            
            void SetNext(AZStd::shared_ptr<StreamStackEntry> next) override;
            void SetContext(StreamerContext& context) override;

            void PrepareRequest(FileRequest* request) override;
            bool ExecuteRequests() override;
            // FinalizeRequest doesn't need to be implemented here as the calls will be directly
            // going to the specific stack entries.
            // GetAvailableRequestSlots is not specialized because the dedicated caches are often
            // small and specific to a tiny subset of files that are loaded. It would therefore
            // return a small number of slots that would needlessly hamper streaming as it doesn't
            // apply to the majority of files.

            void FlushCache(const RequestPath& filePath) override;
            void FlushEntireCache() override;
            void CreateDedicatedCache(const RequestPath& filePath, const FileRange& range) override;
            void DestroyDedicatedCache(const RequestPath& filePath, const FileRange& range) override;
            
            void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

        private:
            void ReadFile(FileRequest* request); 
            size_t FindCache(const RequestPath& filename, FileRange range);
            size_t FindCache(const RequestPath& filename, u64 offset);

            static constexpr size_t s_fileNotFound = static_cast<size_t>(-1);

            AZStd::vector<RequestPath> m_cachedFileNames;
            AZStd::vector<FileRange> m_cachedFileRanges;
            AZStd::vector<AZStd::unique_ptr<BlockCache>> m_cachedFileCaches;
            AZStd::vector<size_t> m_cachedFileRefCounts;

            u64 m_cacheSize;
            u32 m_blockSize;
            bool m_onlyEpilogWrites;
        };
    } // namespace IO
} // namespace AZ