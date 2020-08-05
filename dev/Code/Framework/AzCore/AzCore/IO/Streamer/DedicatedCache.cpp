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
#include <AzCore/std/parallel/lock.h>

namespace AZ
{
    namespace IO
    {
        using RecursiveLockGuard = AZStd::lock_guard<AZStd::recursive_mutex>;

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

            RecursiveLockGuard lock(m_cacheMutex);

            for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                cache->SetNext(m_next);
            }
        }

        void DedicatedCache::SetContext(StreamerContext& context)
        {
            StreamStackEntry::SetContext(context);

            RecursiveLockGuard lock(m_cacheMutex);

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
                RecursiveLockGuard lock(m_cacheMutex);

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
                RecursiveLockGuard lock(m_cacheMutex);

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
                RecursiveLockGuard lock(m_cacheMutex);

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
                RecursiveLockGuard lock(m_cacheMutex);

                for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
                {
                    cache->FlushEntireCache();
                }
            }

            StreamStackEntry::FlushEntireCache();
        }

        void DedicatedCache::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            {
                RecursiveLockGuard lock(m_cacheMutex);

                DedicatedCache* _this = const_cast<DedicatedCache*>(this);
                _this->CleanupCachedNames();
                size_t count = m_cachedFileNames.size();
                for (size_t i = 0; i < count; ++i)
                {
                    double hitRate = m_cachedFileCaches[i]->CalculateHitRatePercentage();
                    double cacheable = m_cachedFileCaches[i]->CalculateCacheableRatePercentage();
                    s32 slots = m_cachedFileCaches[i]->CalculateAvailableRequestSlots();

                    AZStd::string_view name;

                    if (m_cachedFileRanges[i].IsEntireFile())
                    {
                        auto combinedName = AZStd::string::format("%s/%s", m_name.c_str(), m_cachedFileNames[i].GetRelativePath());
                        name = _this->GetOrAddCachedName(AZStd::hash_string(combinedName.begin(), combinedName.length()), AZStd::move(combinedName));
                    }
                    else
                    {
                        auto combinedName = AZStd::string::format("%s/%s %llu:%llu", m_name.c_str(), m_cachedFileNames[i].GetRelativePath(),
                                                                                                     m_cachedFileRanges[i].GetOffset(), m_cachedFileRanges[i].GetEndPoint());
                        name = _this->GetOrAddCachedName(AZStd::hash_string(combinedName.begin(), combinedName.length()), AZStd::move(combinedName));
                    }

                    statistics.push_back(Statistic::CreatePercentage(name, "Cache Hit Rate", hitRate));
                    statistics.push_back(Statistic::CreatePercentage(name, "Cacheable", cacheable));
                    statistics.push_back(Statistic::CreateInteger(name, "Available slots", slots));
                }
            }

            StreamStackEntry::CollectStatistics(statistics);
        }

        void DedicatedCache::CreateDedicatedCache(const RequestPath& filename, const FileRange& range)
        {
            {
                RecursiveLockGuard lock(m_cacheMutex);

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
                RecursiveLockGuard lock(m_cacheMutex);

                size_t index = FindCache(filename, range);
                if (index != s_fileNotFound)
                {
                    AZ_Assert(m_cachedFileRefCounts[index] > 0, "A dedicated cache entry without references was left.");
                    --m_cachedFileRefCounts[index];
                    if (m_cachedFileRefCounts[index] == 0)
                    {
                        size_t hashToDelete = 0;
                        if (m_cachedFileRanges[index].IsEntireFile())
                        {
                            auto combinedName = AZStd::string::format("%s/%s", m_name.c_str(), m_cachedFileNames[index].GetRelativePath());
                            hashToDelete = AZStd::hash_string(combinedName.begin(), combinedName.length());
                        }
                        else
                        {
                            auto combinedName = AZStd::string::format("%s/%s %llu:%llu", m_name.c_str(), m_cachedFileNames[index].GetRelativePath(),
                                                                                                        m_cachedFileRanges[index].GetOffset(), m_cachedFileRanges[index].GetEndPoint());
                            hashToDelete = AZStd::hash_string(combinedName.begin(), combinedName.length());
                        }
                        m_cachedStatNamesToDelete.emplace_back(hashToDelete);

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

        AZStd::string_view DedicatedCache::GetOrAddCachedName(size_t index, AZStd::string&& name)
        {
            auto iter = AZStd::find_if(m_cachedStatNames.begin(), m_cachedStatNames.end(), [index](const auto& element)
            {
                return element.first == index;
            });

            if (iter != m_cachedStatNames.end())
            {
                return AZStd::string_view(iter->second);
            }
            else
            {
                m_cachedStatNames.push_back(AZStd::make_pair<size_t, AZStd::string>(index, AZStd::forward<AZStd::string>(name)));
                return AZStd::string_view(m_cachedStatNames.back().second);
            }
        }

        void DedicatedCache::CleanupCachedNames()
        {
            for (const auto fileHash : m_cachedStatNamesToDelete)
            {
                auto iter = AZStd::find_if(m_cachedStatNames.begin(), m_cachedStatNames.end(), [fileHash](const auto& element)
                {
                    return element.first == fileHash;
                });

                if (iter != m_cachedStatNames.end())
                {
                    m_cachedStatNames.erase(iter);
                }
            }
            m_cachedStatNamesToDelete.clear();
        }
    } // namespace IO
} // namespace AZ