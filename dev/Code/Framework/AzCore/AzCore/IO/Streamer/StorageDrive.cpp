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
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StorageDrive.h>

namespace AZ
{
    namespace IO
    {
        const AZStd::chrono::microseconds StorageDrive::s_averageSeekTime =
            AZStd::chrono::milliseconds(9) + // Common average seek time for desktop hdd drives.
            AZStd::chrono::milliseconds(3); // Rotational latency for a 7200RPM disk

        StorageDrive::StorageDrive(u32 maxFileHandles)
            : StreamStackEntry("Storage drive (generic)")
        {
            m_fileLastUsed.resize(maxFileHandles, AZStd::chrono::system_clock::time_point::min());
            m_filePaths.resize(maxFileHandles);
            m_fileHandles.resize(maxFileHandles);
        }

        void StorageDrive::SetNext(AZStd::shared_ptr<StreamStackEntry> /*next*/)
        {
            AZ_Assert(false, "StorageDrive isn't allowed to have a node to forward requests to.");
        }

        void StorageDrive::PrepareRequest(FileRequest* request)
        {
            AZ_Assert(request, "PrepareRequest was provided a null request.");

            switch (request->GetOperationType())
            {
            case FileRequest::Operation::Read:
                m_pendingRequests.push_back(request);
                break;
            case FileRequest::Operation::Cancel:
                CancelRequest(request);
                break;
            default:
                AZ_Assert(false, "Request wasn't handled by any entry on the streaming stack.");
                break;
            }
        }

        bool StorageDrive::ExecuteRequests()
        {
            if (!m_pendingRequests.empty())
            {
                FileRequest* request = m_pendingRequests.front();
                ReadFile(request);
                m_pendingRequests.pop_front();
                return true;
            }
            else
            {
                return false;
            }
        }

        s32 StorageDrive::GetAvailableRequestSlots() const
        {
            // Only participate if there are actually any reads done.
            if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
            {
                s32 availableSlots = s_maxRequests -aznumeric_cast<s32>(m_pendingRequests.size());
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
                return std::numeric_limits<s32>::max();
            }
        }

        void StorageDrive::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            AZStd::deque<AZStd::shared_ptr<Request>>& externalPending)
        {
            AZStd::chrono::system_clock::time_point currentTime = now;
            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTimeUSec = aznumeric_caster(m_readTimeAverage.GetTotal().count());
            AZStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();

            // Requests will be read in the order they are queued so the time they'll complete will be cumulative.
            u64 currentOffset = m_activeOffset;
            const RequestPath* m_activeFile = nullptr;

            for (FileRequest* request : m_pendingRequests)
            {
                FileRequest::ReadData& readData = request->GetReadData();
                if (m_activeFile)
                {
                    if (*m_activeFile != *readData.m_path)
                    {
                        if (FindFileInCache(*readData.m_path) == s_fileNotFound)
                        {
                            currentTime += fileOpenCloseTimeAverage;
                        }
                        currentOffset = static_cast<u64>(-1);
                    }
                }
                else
                {
                    currentTime += fileOpenCloseTimeAverage;
                }

                if (readData.m_offset != currentOffset)
                {
                    currentTime += s_averageSeekTime;
                }

                m_activeFile = readData.m_path;
                currentTime += AZStd::chrono::microseconds(static_cast<u64>((readData.m_size * totalReadTimeUSec) / totalBytesRead));
                request->SetEstimatedCompletion(currentTime);

                currentOffset = readData.m_offset + readData.m_size;
            }

            for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
            {
                u64 readSize = 0;
                u64 offset = 0;
                if ((*requestIt)->GetOperationType() == FileRequest::Operation::Read)
                {
                    FileRequest::ReadData& readData = (*requestIt)->GetReadData();
                    readSize = readData.m_size;
                    offset = readData.m_offset;
                    if (m_activeFile && *m_activeFile != *readData.m_path)
                    {
                        currentTime += fileOpenCloseTimeAverage;
                        currentOffset = static_cast<u64>(-1);
                    }
                    m_activeFile = readData.m_path;
                }
                else if ((*requestIt)->GetOperationType() == FileRequest::Operation::CompressedRead)
                {
                    FileRequest::CompressedReadData& readData = (*requestIt)->GetCompressedReadData();
                    m_activeFile = &readData.m_compressionInfo.m_archiveFilename;
                    readSize = readData.m_compressionInfo.m_compressedSize;
                    offset = readData.m_compressionInfo.m_offset;
                    // Don't add the fileOpenCloseTimeAverage as the archive to read from is going to be open in most cases.
                }
                else
                {
                    continue;
                }

                if (currentOffset != offset)
                {
                    currentTime += s_averageSeekTime;
                }
                currentTime += AZStd::chrono::microseconds(static_cast<u64>((readSize * totalReadTimeUSec) / totalBytesRead));
                (*requestIt)->SetEstimatedCompletion(currentTime);

                currentOffset = offset + readSize;
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
                    estimation += s_averageSeekTime;
                    estimation += AZStd::chrono::microseconds(static_cast<u64>((readSize * totalReadTimeUSec) / totalBytesRead));
                    request->m_estimatedCompletion = estimation;
                }
            }
        }

        bool StorageDrive::GetFileSize(u64& result, const RequestPath& filePath)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            // If the file is already open, use the file handle which usually is cheaper than asking for the file by name.
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != s_fileNotFound)
            {
                AZ_Assert(m_fileHandles[cacheIndex], 
                    "File path '%s' doesn't have an associated file handle.", m_filePaths[cacheIndex].GetRelativePath());
                result = m_fileHandles[cacheIndex]->Length();
                return true;
            }
            
            // The file is not open yet, so try to get the file size by name.
            u64 size = SystemFile::Length(filePath.GetAbsolutePath());
            if (size != 0) // SystemFile::Length doesn't allow telling a zero-sized file apart from a invalid path.
            {
                result = size;
                return true;
            }

            // No luck finding the file size so let the next entry in the stack try.
            return false;
        }

        void StorageDrive::ReadFile(FileRequest* request)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            FileRequest::ReadData& data = request->GetReadData();
            
            SystemFile* file = nullptr;

            // If the file is already open, use that file handle and update it's last touched time.
            size_t cacheIndex = FindFileInCache(*data.m_path);
            if (cacheIndex != s_fileNotFound)
            {
                file = m_fileHandles[cacheIndex].get();
                m_fileLastUsed[cacheIndex] = AZStd::chrono::high_resolution_clock::now();
            }
            
            // If the file is not open, eject the entry from the cache that hasn't been used for the longest time 
            // and open the file for reading.
            if (!file)
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
                AZStd::unique_ptr<SystemFile> newFile = AZStd::make_unique<SystemFile>();
                bool isOpen = newFile->Open(data.m_path->GetAbsolutePath(), SystemFile::OpenMode::SF_OPEN_READ_ONLY);
                if (!isOpen)
                {
                    request->SetStatus(FileRequest::Status::Failed);
                    m_context->MarkRequestAsCompleted(request);
                    return;
                }

                file = newFile.get();
                m_fileLastUsed[cacheIndex] = AZStd::chrono::high_resolution_clock::now();
                m_fileHandles[cacheIndex] = AZStd::move(newFile);
                m_filePaths[cacheIndex] = *data.m_path;
            }
            
            AZ_Assert(file, "While searching for file '%s' StorageDevice::ReadFile failed to detect a problem.", data.m_path->GetRelativePath());
            u64 bytesRead = 0;
            {
                TIMED_AVERAGE_WINDOW_SCOPE(m_readTimeAverage);
                if (file->Tell() != data.m_offset)
                {
                    file->Seek(data.m_offset, SystemFile::SeekMode::SF_SEEK_BEGIN);
                }
                bytesRead = file->Read(data.m_size, data.m_output);
            }
            m_readSizeAverage.PushEntry(bytesRead);

            m_activeCacheSlot = cacheIndex;
            m_activeOffset = data.m_offset + bytesRead;

            request->SetStatus(bytesRead == data.m_size ? FileRequest::Status::Completed : FileRequest::Status::Failed);
            m_context->MarkRequestAsCompleted(request);
        }

        void StorageDrive::CancelRequest(FileRequest* request)
        {
            for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
            {
                if ((*it)->IsChildOf(request))
                {
                    (*it)->SetStatus(FileRequest::Status::Canceled);
                    m_context->MarkRequestAsCompleted(*it);
                    it = m_pendingRequests.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void StorageDrive::FlushCache(const RequestPath& filePath)
        {
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != s_fileNotFound)
            {
                m_fileLastUsed[cacheIndex] = AZStd::chrono::system_clock::time_point();
                m_fileHandles[cacheIndex].reset();
                m_filePaths[cacheIndex].Clear();
            }
        }

        void StorageDrive::FlushEntireCache()
        {
            size_t numFiles = m_filePaths.size();
            for (size_t i = 0; i < numFiles; ++i)
            {
                m_fileLastUsed[i] = AZStd::chrono::system_clock::time_point();
                m_fileHandles[i].reset();
                m_filePaths[i].Clear();
            }
        }

        size_t StorageDrive::FindFileInCache(const RequestPath& filePath) const
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

        void StorageDrive::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            constexpr double bytesToMB = (1024.0 * 1024.0);
            using DoubleSeconds = AZStd::chrono::duration<double>;
            
            double totalBytesReadMB = m_readSizeAverage.GetTotal() / bytesToMB;
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
        }
    } // namespace IO
} // namespace AZ