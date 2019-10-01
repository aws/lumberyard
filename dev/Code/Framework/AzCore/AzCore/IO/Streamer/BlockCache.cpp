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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace IO
    {
        void BlockCache::Section::Prefix(const Section& section)
        {
            if (!section.m_used)
            {
                return;
            }

            AZ_Assert(!m_wait && !section.m_wait, "Can't merge two section that are already waiting for data to be loaded.");

            if (m_used)
            {
                AZ_Assert(m_blockOffset == 0, "Unable to add a block cache to this one as this block requires an offset upon completion.");
                
                AZ_Assert(section.m_readOffset < m_readOffset, "The block that's being merged needs to come before this block.");
                m_readOffset = section.m_readOffset + section.m_blockOffset; // Remove any alignment that might have been added.
                m_readSize += section.m_readSize - section.m_blockOffset;
                
                AZ_Assert(section.m_output < m_output, "The block that's being merged needs to come before this block.");
                m_output = section.m_output;
                m_copySize += section.m_copySize;
            }
            else
            {
                m_used = true;
                m_readOffset = section.m_readOffset + section.m_blockOffset;
                m_readSize = section.m_readSize - section.m_blockOffset;
                m_output = section.m_output;
                m_copySize = section.m_copySize;
            }
            m_blockOffset = 0; // Two merged sections do not support caching.
        }
        
        BlockCache::BlockCache(u64 cacheSize, u32 blockSize, bool onlyEpilogWrites)
            : StreamStackEntry("Block cache")
            , m_onlyEpilogWrites(onlyEpilogWrites)
        {
            AZ_Assert(blockSize > 0 && (blockSize & (blockSize - 1)) == 0, "Block size needs to be a power of 2.");
            
            m_numBlocks = aznumeric_caster(cacheSize / blockSize);
            m_cacheSize = cacheSize - (cacheSize % blockSize); // Only use the amount needed for the cache.
            m_blockSize = blockSize;
            if (m_numBlocks == 1)
            {
                m_onlyEpilogWrites = true;
            }
            
            m_cache = AZStd::unique_ptr<u8[]>(new u8[m_cacheSize]);
            m_cachedPaths = AZStd::unique_ptr<RequestPath[]>(new RequestPath[m_numBlocks]);
            m_cachedOffsets = AZStd::unique_ptr<u64[]>(new u64[m_numBlocks]);
            m_blockLastTouched = AZStd::unique_ptr<TimePoint[]>(new TimePoint[m_numBlocks]);
            m_inFlightRequests = AZStd::unique_ptr<FileRequest*[]>(new FileRequest*[m_numBlocks]);
            
            ResetCache();
        }

        void BlockCache::PrepareRequest(FileRequest* request)
        {
            AZ_Assert(request, "PrepareRequest was provided a null request.");

            switch (request->GetOperationType())
            {
            case FileRequest::Operation::Read:
                ReadFile(request);
                break;
            default:
                StreamStackEntry::PrepareRequest(request);
                break;
            }
        }

        bool BlockCache::ExecuteRequests()
        {
            size_t delayedCount = m_delayedSections.size();
            
            bool delayedRequestProcessed = false;
            for (size_t i = 0; i < delayedCount; ++i)
            {
                Section& pending = m_delayedSections.front();
                AZ_Assert(pending.m_parent, "Pending section doesn't have a reference to the original request.");
                FileRequest::ReadData& data = pending.m_parent->GetReadData();
                // This call can add the same section to the back of the queue if there's not
                // enough space, so no limit the loop to just the currently know delayed request
                // not the new ones that are later. Because of this the entry needs to be removed
                // from the delayed list no matter what the result is of ServiceFromCache.
                if (ServiceFromCache(pending.m_parent, pending, *data.m_path) != CacheResult::Delayed)
                {
                    delayedRequestProcessed = true;
                }
                m_delayedSections.pop_front();
            }
            bool nextResult = StreamStackEntry::ExecuteRequests();
            return nextResult || delayedRequestProcessed;
        }

        void BlockCache::FinalizeRequest(FileRequest* request)
        {
            auto requestInfo = m_pendingRequests.equal_range(request);
            AZ_Assert(requestInfo.first != requestInfo.second, "Block cache was asked to complete a file request it never queued.");
            
            FileRequest::Status requestStatus = request->GetStatus();
            bool requestWasSuccessful = requestStatus == FileRequest::Status::Completed;
            u32 cacheBlockIndex = requestInfo.first->second.m_cacheBlockIndex;

            for(auto it = requestInfo.first; it != requestInfo.second; ++it)
            {
                Section& section = it->second;
                AZ_Assert(section.m_cacheBlockIndex == cacheBlockIndex,
                    "Section associated with the file request is referencing the incorrect cache block (%i vs %i).", cacheBlockIndex, section.m_cacheBlockIndex);
                if (section.m_wait)
                {
                    section.m_wait->SetStatus(requestStatus);
                    m_context->MarkRequestAsCompleted(section.m_wait);
                }

                if (requestWasSuccessful)
                {
                    memcpy(section.m_output, GetCacheBlockData(cacheBlockIndex) + section.m_blockOffset, section.m_copySize);
                }
            }

            if (requestWasSuccessful)
            {
                TouchBlock(cacheBlockIndex);
                m_inFlightRequests[cacheBlockIndex] = nullptr;
            }
            else
            {
                ResetCacheEntry(cacheBlockIndex);
            }
            AZ_Assert(m_numInFlightRequests > 0, "Clearing out an in-flight request, but there shouldn't be any in flight according to records.");
            m_numInFlightRequests--;
            m_pendingRequests.erase(request);
        }

        s32 BlockCache::GetAvailableRequestSlots() const
        {
            s32 next = StreamStackEntry::GetAvailableRequestSlots();
            return AZStd::min(next, CalculateAvailableRequestSlots());
        }

        void BlockCache::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            AZStd::deque<AZStd::shared_ptr<Request>>& externalPending)
        {
            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, externalPending);

            // The in-flight requests don't have to be updated because the 1, 2 or 3 children they can have will already been updated
            // as the estimations of those children will bubble up to the parent request. Because the storage drives also execute the
            // requests in order, the largest time will be written last resulting in the parent request always having the largest time.
            // Requests that have a wait on another request though will need to be update as the estimation of the in-flight request
            // needs to be copied to the wait request to get an accurate prediction.
            
            for (auto it : m_pendingRequests)
            {
                Section& section = it.second;
                AZ_Assert(section.m_cacheBlockIndex != s_fileNotCached, "An in-flight cache section doesn't have a cache block associated wit it.");
                AZ_Assert(m_inFlightRequests[section.m_cacheBlockIndex], 
                    "Cache block %i is reported as being in-flight but has no request.", section.m_cacheBlockIndex);
                if (section.m_wait)
                {
                    AZ_Assert(section.m_parent, "A cache section with a wait request pending is missing a parent to wait on.");
                    auto largestTime = AZStd::max(section.m_parent->GetEstimatedCompletion(), it.first->GetEstimatedCompletion());
                    section.m_wait->SetEstimatedCompletion(largestTime);
                }
            }
        }

        void BlockCache::ReadFile(FileRequest* request)
        {
            if (!m_next)
            {
                request->SetStatus(FileRequest::Status::Failed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            Section prolog;
            Section main;
            Section epilog;

            FileRequest::ReadData& data = request->GetReadData();

            if (!SplitRequest(prolog, main, epilog, *data.m_path, data.m_offset, data.m_size, reinterpret_cast<u8*>(data.m_output)))
            {
                request->SetStatus(FileRequest::Status::Failed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            if (prolog.m_used || epilog.m_used)
            {
                m_cacheableStatistic.PushEntry(1);
            }
            else
            {
                // Nothing to cache so simply forward the call to the next entry in the stack for direct reading.
                m_cacheableStatistic.PushEntry(0);
                m_next->PrepareRequest(request);
                return;
            }
            
            bool fullyCached = true;
            if (prolog.m_used)
            {
                if (m_onlyEpilogWrites && (main.m_used || epilog.m_used))
                {
                    // Only the epilog is allowed to write to the cache, but a previous read could
                    // still have cached the prolog, so check the cache and use the data if it's there
                    // otherwise merge the section with the main section to have the data read.
                    if (ReadFromCache(request, prolog, *data.m_path) == CacheResult::CacheMiss)
                    {
                        // The data isn't cached so put the prolog in front of the main section
                        // so it's read in one read request. If main wasn't used, prefixing the prolog 
                        // will cause it to be filled in and used.
                        main.Prefix(prolog);
                        m_hitRateStatistic.PushEntry(0);
                    }
                    else
                    {
                        m_hitRateStatistic.PushEntry(1);
                    }
                }
                else
                {
                    // If m_onlyEpilogWrites is set but main and epilog are not filled in, it means that 
                    // the request was so small it fits in one cache block, in which case the prolog and
                    // epilog are practically the same. Or this code is reached because both prolog and
                    // epilog are allowed to write.
                    bool readFromCache = (ServiceFromCache(request, prolog, *data.m_path) == CacheResult::ReadFromCache);
                    fullyCached = readFromCache && fullyCached;
                    m_hitRateStatistic.PushEntry(readFromCache ? 1 : 0);
                }
            }

            if (main.m_used)
            {
                FileRequest* mainRequest = m_context->GetNewInternalRequest();
                // Don't assign ourselves as the owner as there's nothing to do on callback.
                mainRequest->CreateRead(nullptr, request, main.m_output, *data.m_path, main.m_readOffset, main.m_readSize);
                m_next->PrepareRequest(mainRequest);
                fullyCached = false;
            }

            if (epilog.m_used)
            {
                bool readFromCache = (ServiceFromCache(request, epilog, *data.m_path) == CacheResult::ReadFromCache);
                fullyCached = readFromCache && fullyCached;
                m_hitRateStatistic.PushEntry(readFromCache ? 1 : 0);
            }

            if (fullyCached)
            {
                request->SetStatus(FileRequest::Status::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
        }

        void BlockCache::FlushCache(const RequestPath& filePath)
        {
            StreamStackEntry::FlushCache(filePath);
        }

        void BlockCache::FlushEntireCache()
        {
            ResetCache();
            
            StreamStackEntry::FlushEntireCache();
        }

        void BlockCache::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            statistics.push_back(Statistic::CreatePercentage(m_name, "Cache Hit Rate", CalculateHitRatePercentage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, "Cacheable", CalculateCacheableRatePercentage()));
            statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", CalculateAvailableRequestSlots()));

            StreamStackEntry::CollectStatistics(statistics);
        }

        double BlockCache::CalculateHitRatePercentage() const
        {
            return m_hitRateStatistic.CalculateAverage();
        }

        double BlockCache::CalculateCacheableRatePercentage() const
        {
            return m_cacheableStatistic.CalculateAverage();
        }

        s32 BlockCache::CalculateAvailableRequestSlots() const
        {
            return  aznumeric_cast<s32>(m_numBlocks) - m_numInFlightRequests - aznumeric_cast<s32>(m_delayedSections.size());
        }

        BlockCache::CacheResult BlockCache::ReadFromCache(FileRequest* request, Section& section, const RequestPath& filePath)
        {
            u32 cacheLocation = FindInCache(filePath, section.m_readOffset);
            if (cacheLocation != s_fileNotCached)
            {
                return ReadFromCache(request, section, cacheLocation);
            }
            else
            {
                return CacheResult::CacheMiss;
            }
        }

        BlockCache::CacheResult BlockCache::ReadFromCache(FileRequest* request, Section& section, u32 cacheBlock)
        {
            if (!IsCacheBlockInFlight(cacheBlock))
            {
                TouchBlock(cacheBlock);
                memcpy(section.m_output, GetCacheBlockData(cacheBlock) + section.m_blockOffset, section.m_copySize);
                return CacheResult::ReadFromCache;
            }
            else
            {
                FileRequest* wait = m_context->GetNewInternalRequest();
                wait->CreateWait(nullptr, request);
                section.m_cacheBlockIndex = cacheBlock;
                section.m_parent = request;
                section.m_wait = wait;
                m_pendingRequests.emplace(m_inFlightRequests[cacheBlock], section);
                return CacheResult::Queued;
            }
        }

        BlockCache::CacheResult BlockCache::ServiceFromCache(FileRequest* request, Section& section, const RequestPath& filePath)
        {
            AZ_Assert(m_next, "ServiceFromCache in BlockCache was called when the cache doesn't have a way to read files.");

            u32 cacheLocation = FindInCache(filePath, section.m_readOffset);
            if (cacheLocation == s_fileNotCached)
            {
                m_hitRateStatistic.PushEntry(0);

                section.m_parent = request;
                cacheLocation = RecycleOldestBlock(filePath, section.m_readOffset);
                if (cacheLocation != s_fileNotCached)
                {
                    FileRequest* readRequest = m_context->GetNewInternalRequest();
                    readRequest->CreateRead(this, request, GetCacheBlockData(cacheLocation), filePath, section.m_readOffset, section.m_readSize);
                    section.m_cacheBlockIndex = cacheLocation;
                    m_inFlightRequests[cacheLocation] = readRequest;
                    m_numInFlightRequests++;
                    m_pendingRequests.emplace(readRequest, section);
                    m_next->PrepareRequest(readRequest);
                    return CacheResult::Queued;
                }
                else
                {
                    // There's no more space in the cache to store this request to. This is because there are more in-flight requests than 
                    // there are slot in the cache. Create a placeholder request and delay it's queuing until there's a slot available.
                    m_delayedSections.push_back(section);
                    return CacheResult::Delayed;
                }
            }
            else
            {
                m_hitRateStatistic.PushEntry(1);
                return ReadFromCache(request, section, cacheLocation);
            }
        }

        bool BlockCache::SplitRequest(Section& prolog, Section& main, Section& epilog, const RequestPath& filePath, u64 offset, u64 size, u8* buffer) const
        {
            u64 fileLength = 0;
            if (!m_next->GetFileSize(fileLength, filePath))
            {
                return false;
            }
            AZ_Assert(offset + size <= fileLength, "File at path '%s' is being read past the end of the file.", filePath.GetRelativePath());

            //
            // Prolog
            // This looks at the request and sees if there's anything in front of the file that should be cached. This also
            // deals with the situation where the entire file request fits inside the cache which could mean there's data
            // left after the file as well that could be cached.
            //
            u64 roundedOffsetStart = offset & ~(static_cast<u64>(m_blockSize) - 1);
            
            u64 blockReadSizeStart = AZStd::min(fileLength - roundedOffsetStart, static_cast<u64>(m_blockSize));
            // Check if the request is on the left edge of the cache block, which means there's nothing in front of it
            // that could be cached.
            if (roundedOffsetStart == offset)
            {
                if (offset + size >= fileLength)
                {
                    // The entire (remainder) of the file is read so there's nothing to cache
                    main.m_readOffset = offset;
                    main.m_readSize = size;
                    main.m_output = buffer;
                    main.m_used = true;
                    return true;
                }
                else if (size < blockReadSizeStart)
                {
                    // The entire request fits inside a single cache block, but there's more file to read.
                    prolog.m_readOffset = offset;
                    prolog.m_readSize = blockReadSizeStart;
                    prolog.m_blockOffset = 0;
                    prolog.m_output = buffer;
                    prolog.m_copySize = size;
                    prolog.m_used = true;
                    return true;
                }
                // In any other case it means that the entire block would be read so caching has no effect.
            }
            else
            {
                // There is a portion of the file before that's not requested so always cache this block.
                const u64 blockOffset = offset - roundedOffsetStart;
                prolog.m_readOffset = roundedOffsetStart;
                prolog.m_blockOffset = blockOffset;
                prolog.m_output = buffer;
                prolog.m_used = true;

                const bool isEntirelyInCache = blockOffset + size <= blockReadSizeStart;
                if (isEntirelyInCache)
                {
                    if (roundedOffsetStart + blockReadSizeStart > fileLength)
                    {
                        prolog.m_readSize = fileLength - roundedOffsetStart;
                    }
                    else
                    {
                        prolog.m_readSize = blockReadSizeStart;
                    }
                    prolog.m_copySize = size;

                    // There won't be anything else coming after this so continue reading.
                    return true;
                }
                else
                {
                    prolog.m_readSize = blockReadSizeStart;
                    prolog.m_copySize = blockReadSizeStart - blockOffset;
                }
            }


            //
            // Epilog
            // Since the prolog already takes care of the situation where the file fits entirely in the cache the epilog is
            // much simpler as it only has to look at the case where there is more file after the request to read for caching.
            //
            u64 roundedOffsetEnd = (offset + size) & ~(static_cast<u64>(m_blockSize) - 1);
            u64 copySize = offset + size - roundedOffsetEnd;
            u64 blockReadSizeEnd = m_blockSize;
            if ((roundedOffsetEnd + blockReadSizeEnd) > fileLength)
            {
                blockReadSizeEnd = fileLength - roundedOffsetEnd;
            }

            // If the read doesn't align with the edge of the cache
            if (copySize != 0 && copySize < blockReadSizeEnd)
            {
                epilog.m_readOffset = roundedOffsetEnd;
                epilog.m_readSize = blockReadSizeEnd;
                epilog.m_blockOffset = 0;
                epilog.m_output = buffer + (roundedOffsetEnd - offset);
                epilog.m_copySize = copySize;
                epilog.m_used = true;
            }

            //
            // Main
            // If this point is reached there's potentially a block between the prolog and epilog that can be directly read.
            //
            u64 adjustedOffset = offset;
            if (prolog.m_used)
            {
                adjustedOffset += prolog.m_copySize;
                size -= prolog.m_copySize;
            }
            if (epilog.m_used)
            {
                size -= epilog.m_copySize;
            }
            AZ_Assert((adjustedOffset & (m_blockSize - 1)) == 0, "The adjustments made by the prolog should guarantee the offset is aligned to a cache block.");
            if (size != 0)
            {
                main.m_readOffset = adjustedOffset;
                main.m_readSize = size;
                main.m_output = buffer + (adjustedOffset - offset);
                main.m_used = true;
            }

            return true;
        }

        u8* BlockCache::GetCacheBlockData(u32 index)
        {
            AZ_Assert(index < m_numBlocks, "Index for touch a cache entry in the BlockCache is out of bounds.");
            return m_cache.get() + (index * m_blockSize);
        }

        void BlockCache::TouchBlock(u32 index)
        {
            AZ_Assert(index < m_numBlocks, "Index for touch a cache entry in the BlockCache is out of bounds.");
            m_blockLastTouched[index] = AZStd::chrono::high_resolution_clock::now();
        }

        u32 BlockCache::RecycleOldestBlock(const RequestPath& filePath, u64 offset)
        {
            AZ_Assert((offset & (m_blockSize - 1)) == 0, "The offset used to recycle a block cache needs to be a multiple of the block size.");

            // Find the oldest cache block.
            TimePoint oldest = m_blockLastTouched[0];
            u32 oldestIndex = 0;
            for (u32 i = 1; i < m_numBlocks; ++i)
            {
                if (m_blockLastTouched[i] < oldest && !m_inFlightRequests[i])
                {
                    oldest = m_blockLastTouched[i];
                    oldestIndex = i;
                }
            }

            if (!m_inFlightRequests[oldestIndex])
            {
                // Recycle the block.
                m_cachedPaths[oldestIndex] = filePath;
                m_cachedOffsets[oldestIndex] = offset;
                TouchBlock(oldestIndex);
                return oldestIndex;
            }
            else
            {
                return s_fileNotCached;
            }
        }

        u32 BlockCache::FindInCache(const RequestPath& filePath, u64 offset) const
        {
            AZ_Assert((offset & (m_blockSize - 1)) == 0, "The offset used to find a block in the block cache needs to be a multiple of the block size.");
            for (u32 i = 0; i < m_numBlocks; ++i)
            {
                if (m_cachedPaths[i] == filePath && m_cachedOffsets[i] == offset)
                {
                    return i;
                }
            }

            return s_fileNotCached;
        }

        bool BlockCache::IsCacheBlockInFlight(u32 index) const
        {
            AZ_Assert(index < m_numBlocks, "Index for checking if a cache block is in flight is out of bounds.");
            return m_inFlightRequests[index] != nullptr;
        }

        void BlockCache::ResetCacheEntry(u32 index)
        {
            AZ_Assert(index < m_numBlocks, "Index for resetting a cache entry in the BlockCache is out of bounds.");

            m_cachedPaths[index].Clear();
            m_cachedOffsets[index] = 0;
            m_blockLastTouched[index] = TimePoint::min();
            m_inFlightRequests[index] = nullptr;
        }

        void BlockCache::ResetCache()
        {
            for (u32 i = 0; i < m_numBlocks; ++i)
            {
                ResetCacheEntry(i);
            }
            m_numInFlightRequests = 0;
        }
    } // namespace IO
} // namespace AZ