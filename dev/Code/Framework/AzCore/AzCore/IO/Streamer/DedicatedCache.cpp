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
#include <AzCore/IO/Streamer/DedicatedCache.h>
#include <AzCore/std/string/string.h>

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
#include <AzCore/std/parallel/lock.h>
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

namespace AZ
{
    namespace IO
    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        using RecursiveLockGuard = AZStd::lock_guard<AZStd::recursive_mutex>;
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

        DedicatedCache::DedicatedCache(u64 cacheSize, u32 blockSize, bool onlyEpilogWrites)
            : StreamStackEntry("Dedicated cache")
            , m_cacheSize(cacheSize)
            , m_blockSize(blockSize)
            , m_onlyEpilogWrites(onlyEpilogWrites)
        {
        }

        void DedicatedCache::SetNext(AZStd::shared_ptr<StreamStackEntry> next)
        {
            m_next = AZStd::move(next);

            // lock not needed - SetNext only run during configuration, before request processing
            for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                cache->SetNext(m_next);
            }
        }

        void DedicatedCache::SetContext(StreamerContext& context)
        {
            StreamStackEntry::SetContext(context);

            // lock not needed - SetContext only run during configuration, before request processing
            for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                cache->SetContext(context);
            }
        }

        void DedicatedCache::PrepareRequest(FileRequest* request)
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

        bool DedicatedCache::ExecuteRequests()
        {
            bool hasProcessedRequest = false;

            {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                RecursiveLockGuard lock(m_cacheMutex);
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
                {
                    hasProcessedRequest = cache->ExecuteRequests() || hasProcessedRequest;
                }
            }

            return StreamStackEntry::ExecuteRequests() || hasProcessedRequest;
        }

        void DedicatedCache::ReadFile(FileRequest* request)
        {
            FileRequest::ReadData& data = request->GetReadData();

            {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                RecursiveLockGuard lock(m_cacheMutex);
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                size_t index = FindCache(*data.m_path, data.m_offset);
                if (index != s_fileNotFound)
                {
                    m_cachedFileCaches[index]->PrepareRequest(request);
                    return;
                }
            }

            if (m_next)
            {
                m_next->PrepareRequest(request);
            }
        }

        void DedicatedCache::FlushCache(const RequestPath& filePath)
        {
            {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                RecursiveLockGuard lock(m_cacheMutex);
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                size_t count = m_cachedFileNames.size();
                for (size_t i = 0; i < count; ++i)
                {
                    if (m_cachedFileNames[i] == filePath)
                    {
                        m_cachedFileCaches[i]->FlushCache(filePath);
                    }
                }
            }

            StreamStackEntry::FlushCache(filePath);
        }

        void DedicatedCache::FlushEntireCache()
        {
            {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                RecursiveLockGuard lock(m_cacheMutex);
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
                {
                    cache->FlushEntireCache();
                }
            }

            StreamStackEntry::FlushEntireCache();
        }

        void DedicatedCache::CollectStatistics(AZStd::vector<Statistic>& statistics)
        {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            {
                RecursiveLockGuard lock(m_cacheMutex);

                size_t count = m_cachedFileNames.size();
                for (size_t i = 0; i < count; ++i)
                {
                    double hitRate = m_cachedFileCaches[i]->CalculateHitRatePercentage();
                    double cacheable = m_cachedFileCaches[i]->CalculateCacheableRatePercentage();
                    s32 slots = m_cachedFileCaches[i]->CalculateAvailableRequestSlots();

                    AZStd::string_view name;
                    if (m_cachedFileRanges[i].IsEntireFile())
                    {
                        name = AddOrUpdateCachedName(i, AZStd::string::format("%s/%s", m_name.c_str(), m_cachedFileNames[i].GetRelativePath()));
                    }
                    else
                    {
                        name = AddOrUpdateCachedName(i, AZStd::string::format("%s/%s %llu:%llu", m_name.c_str(), m_cachedFileNames[i].GetRelativePath(),
                            m_cachedFileRanges[i].GetOffset(), m_cachedFileRanges[i].GetEndPoint()));
                    }

                    statistics.push_back(Statistic::CreatePercentage(name, "Cache Hit Rate", hitRate));
                    statistics.push_back(Statistic::CreatePercentage(name, "Cacheable", cacheable));
                    statistics.push_back(Statistic::CreateInteger(name, "Available slots", slots));
                }
            }
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

            StreamStackEntry::CollectStatistics(statistics);
        }

        void DedicatedCache::CreateDedicatedCache(const RequestPath& filename, const FileRange& range)
        {
            {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                RecursiveLockGuard lock(m_cacheMutex);
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                size_t index = FindCache(filename, range);
                if (index == s_fileNotFound)
                {
                    index = m_cachedFileCaches.size();
                    m_cachedFileNames.push_back(filename);
                    m_cachedFileRanges.push_back(range);
                    m_cachedFileCaches.push_back(AZStd::make_unique<BlockCache>(m_cacheSize, m_blockSize, m_onlyEpilogWrites));
                    m_cachedFileCaches[index]->SetNext(m_next);
                    m_cachedFileCaches[index]->SetContext(*m_context);
                    m_cachedFileRefCounts.push_back(1);
                }
                else
                {
                    ++m_cachedFileRefCounts[index];
                }
            }

            StreamStackEntry::CreateDedicatedCache(filename, range);
        }

        void DedicatedCache::DestroyDedicatedCache(const RequestPath& filename, const FileRange& range)
        {
            {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                RecursiveLockGuard lock(m_cacheMutex);
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                size_t index = FindCache(filename, range);
                if (index != s_fileNotFound)
                {
                    AZ_Assert(m_cachedFileRefCounts[index] > 0, "A dedicated cache entry without references was left.");
                    --m_cachedFileRefCounts[index];
                    if (m_cachedFileRefCounts[index] == 0)
                    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                        if (m_cachedStatNames.size() > index)
                        {
                            m_cachedStatNames.erase(m_cachedStatNames.begin() + index);
                        }
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                        m_cachedFileNames.erase(m_cachedFileNames.begin() + index);
                        m_cachedFileRanges.erase(m_cachedFileRanges.begin() + index);
                        m_cachedFileCaches.erase(m_cachedFileCaches.begin() + index);
                        m_cachedFileRefCounts.erase(m_cachedFileRefCounts.begin() + index);
                    }
                }
                else
                {
                    AZ_Assert(false, "Attempting to destroy a dedicated cache that doesn't exist or was already destroyed.");
                }
            }

            StreamStackEntry::DestroyDedicatedCache(filename, range);
        }

        size_t DedicatedCache::FindCache(const RequestPath& filename, FileRange range)
        {
            size_t count = m_cachedFileNames.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (m_cachedFileNames[i] == filename && m_cachedFileRanges[i] == range)
                {
                    return i;
                }
            }
            return s_fileNotFound;
        }

        size_t DedicatedCache::FindCache(const RequestPath& filename, u64 offset)
        {
            size_t count = m_cachedFileNames.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (m_cachedFileNames[i] == filename && m_cachedFileRanges[i].IsInRange(offset))
                {
                    return i;
                }
            }
            return s_fileNotFound;
        }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZStd::string_view DedicatedCache::AddOrUpdateCachedName(size_t index, AZStd::string&& name)
        {
            if (m_cachedStatNames.size() <= index)
            {
                m_cachedStatNames.push_back(AZStd::move(name));
                return m_cachedStatNames.back();
            }
            else
            {
                AZStd::string& storedName = m_cachedStatNames[index];
                storedName = AZStd::move(name);
                return storedName;
            }
        }
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

    } // namespace IO
} // namespace AZ
