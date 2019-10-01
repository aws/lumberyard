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

#include <StdAfx.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include "RemoteStorageDrive.h"

namespace AZ
{
    namespace IO
    {
        RemoteStorageDrive::RemoteStorageDrive(u32 maxFileHandles)
            : StreamStackEntry("Storage drive(VFS)")
        {
            m_fileLastUsed.resize(maxFileHandles, AZStd::chrono::system_clock::time_point::min());
            m_filePaths.resize(maxFileHandles);
            m_fileHandles.resize(maxFileHandles, InvalidHandle);
        }

        RemoteStorageDrive::~RemoteStorageDrive()
        {
            for (HandleType handle : m_fileHandles)
            {
                if (handle != InvalidHandle)
                {
                    m_fileIO.Close(handle);
                }
            }
        }

        void RemoteStorageDrive::PrepareRequest(FileRequest* request)
        {
            AZ_Assert(request, "PrepareRequest was provided a null request.");

            switch (request->GetOperationType())
            {
            case FileRequest::Operation::Read:
                m_pendingRequests.push_back(request);
                break;
            case FileRequest::Operation::Cancel:
                if (!CancelRequest(request))
                {
                    // Only forward if this isn't part of the request chain, otherwise the storage device should
                    // be the last step as it doesn't forward any (sub)requests.
                    StreamStackEntry::PrepareRequest(request);
                }
                break;
            default:
                StreamStackEntry::PrepareRequest(request);
                break;
            }
        }

        bool RemoteStorageDrive::ExecuteRequests()
        {
            if (!m_pendingRequests.empty())
            {
                FileRequest* request = m_pendingRequests.front();
                ReadFile(request);
                m_pendingRequests.pop_front();
                StreamStackEntry::ExecuteRequests();
                return true;
            }
            else
            {
                return StreamStackEntry::ExecuteRequests();
            }
        }

        s32 RemoteStorageDrive::GetAvailableRequestSlots() const
        {
            if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
            {
                s32 availableSlots = s_maxRequests - aznumeric_cast<s32>(m_pendingRequests.size());
                if (m_next)
                {
                    return AZStd::min(m_next->GetAvailableRequestSlots(), availableSlots);
                }
                else
                {
                    return availableSlots;
                }
            }
            else
            {
                return StreamStackEntry::GetAvailableRequestSlots();
            }
        }

        void RemoteStorageDrive::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            AZStd::deque<AZStd::shared_ptr<Request>>& externalPending)
        {
            // The RemoteStorageDrive can't tell upfront if the request will be serviced by the virtual file system or needs to happen locally. For
            // this reason it assumed that it will service all requests and only fall down to local read in rare situations which means the
            // pending requests (requests not queued up in stream stack by the scheduler) are always considered to be handled by this drive.
            // To avoid other entries in the stack wasting time calculating the completion estimates an empty queue is passed in so the following
            // nodes do no work.
            AZStd::deque<AZStd::shared_ptr<Request>> emptyPending;
            
            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, emptyPending);

            AZStd::chrono::system_clock::time_point currentTime = now;
            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTimeUSec = aznumeric_caster(m_readTimeAverage.GetTotal().count());
            AZStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();

            // Requests will be read in the order they are queued so the time they'll complete will be cumulative.
            size_t activeCacheSlot = m_activeCacheSlot;

            for (FileRequest* request : m_pendingRequests)
            {
                FileRequest::ReadData& readData = request->GetReadData();
                size_t index = FindFileInCache(*readData.m_path);
                if (index != s_fileNotFound)
                {
                    activeCacheSlot = index;
                }
                else
                {
                    currentTime += fileOpenCloseTimeAverage;
                    activeCacheSlot = s_fileNotFound;
                }

                currentTime += AZStd::chrono::microseconds(static_cast<u64>((readData.m_size * totalReadTimeUSec) / totalBytesRead));
                request->SetEstimatedCompletion(currentTime);
            }

            for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
            {
                u64 readSize = 0;
                if ((*requestIt)->GetOperationType() == FileRequest::Operation::Read)
                {
                    FileRequest::ReadData& readData = (*requestIt)->GetReadData();
                    readSize = readData.m_size;
                }
                else if ((*requestIt)->GetOperationType() == FileRequest::Operation::CompressedRead)
                {
                    FileRequest::CompressedReadData& readData = (*requestIt)->GetCompressedReadData();
                    readSize = readData.m_compressionInfo.m_compressedSize;
                }
                else
                {
                    continue;
                }

                currentTime += fileOpenCloseTimeAverage;
                currentTime += AZStd::chrono::microseconds(static_cast<u64>((readSize * totalReadTimeUSec) / totalBytesRead));
                (*requestIt)->SetEstimatedCompletion(currentTime);
            }

            // The pending requests can be reordered based on various factors so lean towards a worst case prediction.
            for (AZStd::shared_ptr<Request>& request : externalPending)
            {
                if (request->IsReadOperation())
                {
                    bool needsDecompression = request->NeedsDecompression();
                    size_t readSize = needsDecompression ? request->m_compressionInfo.m_compressedSize : request->m_byteSize;
                    AZStd::chrono::system_clock::time_point estimation = currentTime;
                    estimation += fileOpenCloseTimeAverage;
                    estimation += AZStd::chrono::microseconds(static_cast<u64>((readSize * totalReadTimeUSec) / totalBytesRead));
                    request->m_estimatedCompletion = estimation;
                }
            }
        }

        bool RemoteStorageDrive::GetFileSize(u64& result, const RequestPath& filePath)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            // If the file is already open, use the file handle which usually is cheaper than asking for the file by name.
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != s_fileNotFound)
            {
                AZ_Assert(m_fileHandles[cacheIndex] != InvalidHandle,
                    "File path '%s' doesn't have an associated file handle.", m_filePaths[cacheIndex].GetRelativePath());
                return m_fileIO.Size(m_fileHandles[cacheIndex], result);
            }
            
            // The file is not open yet, so try to get the file size by name.
            if (m_fileIO.Size(filePath.GetRelativePath(), result))
            {
                return true;
            }

            // No luck finding the file size so let the next entry in the stack try.
            return m_next ? m_next->GetFileSize(result, filePath) : false;
        }

        void RemoteStorageDrive::ReadFile(FileRequest* request)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore); 
            
            FileRequest::ReadData& data = request->GetReadData();

            HandleType file = InvalidHandle;

            // If the file is already open, use that file handle and update it's last touched time.
            size_t cacheIndex = FindFileInCache(*data.m_path);
            if (cacheIndex != s_fileNotFound)
            {
                file = m_fileHandles[cacheIndex];
                m_fileLastUsed[cacheIndex] = AZStd::chrono::high_resolution_clock::now();
            }
            
            // If the file is not open, eject the oldest entry from the cache and open the file for reading.
            if (file == InvalidHandle)
            {
                AZStd::chrono::system_clock::time_point oldest = m_fileLastUsed[0];
                cacheIndex = 0;
                size_t numFiles = m_filePaths.size();
                for (size_t i = 1; i < numFiles; ++i)
                {
                    if (m_fileLastUsed[i] < oldest)
                    {
                        oldest = m_fileLastUsed[i];
                        cacheIndex = i;
                    }
                }

                TIMED_AVERAGE_WINDOW_SCOPE(m_fileOpenCloseTimeAverage);
                if (!m_fileIO.Open(data.m_path->GetRelativePath(), OpenMode::ModeRead, file))
                {
                    StreamStackEntry::PrepareRequest(request);
                    return;
                }

                m_fileLastUsed[cacheIndex] = AZStd::chrono::high_resolution_clock::now();
                if (m_fileHandles[cacheIndex] != InvalidHandle)
                {
                    m_fileIO.Close(m_fileHandles[cacheIndex]);
                }
                m_fileHandles[cacheIndex] = file;
                m_filePaths[cacheIndex] = *data.m_path;
            }
            m_activeCacheSlot = cacheIndex;

            AZ_Assert(file != InvalidHandle, 
                "While searching for file '%s' RemoteStorageDevice::ReadFile encountered a problem that wasn't reported.", data.m_path->GetRelativePath());
            {
                TIMED_AVERAGE_WINDOW_SCOPE(m_readTimeAverage);
                u64 currentOffset = 0;
                if (!m_fileIO.Tell(file, currentOffset))
                {
                    AZ_Warning("IO", false, "RemoteIO failed to tell the offset for a valid file handle for file '%s'.", data.m_path->GetRelativePath());
                    m_readSizeAverage.PushEntry(0);
                    request->SetStatus(FileRequest::Status::Failed);
                    m_context->MarkRequestAsCompleted(request);
                }
                if (currentOffset != data.m_offset)
                {
                    if (!m_fileIO.Seek(file, data.m_offset, SeekType::SeekFromStart))
                    {
                        AZ_Warning("IO", false, "RemoteIO failed to tell to seek to %i in '%s'.", data.m_offset, data.m_path->GetRelativePath());
                        m_readSizeAverage.PushEntry(0);
                        request->SetStatus(FileRequest::Status::Failed);
                        m_context->MarkRequestAsCompleted(request);
                    }
                }
                if (!m_fileIO.Read(file, data.m_output, data.m_size, true))
                {
                    AZ_Warning("IO", false, "RemoteIO failed to read %i bytes at offset %i from '%s'.", 
                        data.m_size, data.m_offset, data.m_path->GetRelativePath());
                    m_readSizeAverage.PushEntry(0);
                    request->SetStatus(FileRequest::Status::Failed);
                    m_context->MarkRequestAsCompleted(request);
                }
            }
            m_readSizeAverage.PushEntry(data.m_size);
            
            request->SetStatus(FileRequest::Status::Completed);
            m_context->MarkRequestAsCompleted(request);
        }

        bool RemoteStorageDrive::CancelRequest(FileRequest* request)
        {
            bool ownsRequestChain = false;
            for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
            {
                if ((*it)->IsChildOf(request))
                {
                    (*it)->SetStatus(FileRequest::Status::Canceled);
                    m_context->MarkRequestAsCompleted(*it);
                    it = m_pendingRequests.erase(it);
                    ownsRequestChain = true;
                }
                else
                {
                    ++it;
                }
            }
            return ownsRequestChain;
        }

        void RemoteStorageDrive::FlushCache(const RequestPath& filePath)
        {
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != s_fileNotFound)
            {
                m_fileLastUsed[cacheIndex] = AZStd::chrono::system_clock::time_point();
                m_filePaths[cacheIndex].Clear();
                AZ_Assert(m_fileHandles[cacheIndex] != InvalidHandle,
                    "File path '%s' doesn't have an associated file handle.", m_filePaths[cacheIndex].GetRelativePath());
                m_fileIO.Close(m_fileHandles[cacheIndex]);
                m_fileHandles[cacheIndex] = InvalidHandle;
            }

            StreamStackEntry::FlushCache(filePath);
        }

        void RemoteStorageDrive::FlushEntireCache()
        {
            size_t numFiles = m_filePaths.size();
            for (size_t i = 0; i < numFiles; ++i)
            {
                m_fileLastUsed[i] = AZStd::chrono::system_clock::time_point();
                m_filePaths[i].Clear();
                if (m_fileHandles[i] != InvalidHandle)
                {
                    m_fileIO.Close(m_fileHandles[i]);
                    m_fileHandles[i] = InvalidHandle;
                }
            }

            StreamStackEntry::FlushEntireCache();
        }

        size_t RemoteStorageDrive::FindFileInCache(const RequestPath& filePath) const
        {
            size_t numFiles = m_filePaths.size();
            for (size_t i = 0; i < numFiles; ++i)
            {
                if (m_filePaths[i] == filePath)
                {
                    return i;
                }
            }
            return s_fileNotFound;
        }

        void RemoteStorageDrive::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            using DoubleSeconds = AZStd::chrono::duration<double>;
            
            double totalBytesReadMB = m_readSizeAverage.GetTotal() / (1024.0 * 1024.0);
            double totalReadTimeSec = AZStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
            if (totalReadTimeSec > 0.0)
            {
                statistics.push_back(Statistic::CreateFloat(m_name, "Read Speed (avg. mbps)", totalBytesReadMB / totalReadTimeSec));
            }

            if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
            {
                statistics.push_back(Statistic::CreateInteger(m_name, "File Open & Close (avg. us)", m_fileOpenCloseTimeAverage.CalculateAverage().count()));
                statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", s64{ s_maxRequests } - m_pendingRequests.size()));
            }

            StreamStackEntry::CollectStatistics(statistics);
        }
    } // namespace IO
} // namespace AZ