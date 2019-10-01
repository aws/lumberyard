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
#include <AzCore/IO/StreamerRequest.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StorageDrive_Windows.h>

namespace AZ
{
    namespace IO
    {
        const AZStd::chrono::microseconds StorageDriveWin::s_averageSeekTime =
            AZStd::chrono::milliseconds(9) + // Common average seek time for desktop hdd drives.
            AZStd::chrono::milliseconds(3); // Rotational latency for a 7200RPM disk

        StorageDriveWin::StorageDriveWin(const AZStd::vector<const char*>& drivePaths, bool hasSeekPenalty, u32 maxFileHandles)
            : m_maxFileHandles(maxFileHandles)
            , m_hasSeekPenalty(hasSeekPenalty)
        {
            AZ_Assert(!drivePaths.empty(), "StorageDrive_win requires at least one drive path to work.");
            
            // Get drive paths
            m_drivePaths.reserve(drivePaths.size());
            for (const char* drivePath : drivePaths)
            {
                AZStd::string path(drivePath);
                // Erase the slash as it's one less character to compare and avoids issues with forward and
                // backward slashes.
                path.erase(path.length() - 1);
                m_drivePaths.push_back(AZStd::move(path));
            }

            // Create name for statistics. The name will include all drive mappings on this physical device
            // for instance "Storage drive (F:,G:,H:)"
            m_name = "Storage drive (";
            m_name += m_drivePaths[0].substr(0, m_drivePaths[0].length()-1);
            for (size_t i = 1; i < m_drivePaths.size(); ++i)
            {
                m_name += ',';
                // Add the path, but don't include the slash as that might cause formating issues with some profilers.
                m_name += m_drivePaths[i].substr(0, m_drivePaths[i].length()-1);
            }
            m_name += ')';
            AZ_Printf("Streamer", "%s created.\n", m_name.c_str());
        }

        StorageDriveWin::~StorageDriveWin()
        {
            for (HANDLE file : m_fileHandles)
            {
                if (file != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(file);
                }
            }
            AZ_Printf("Streamer", "%s destroyed.\n", m_name.c_str());
        }

        void StorageDriveWin::PrepareRequest(FileRequest* request)
        {
            AZ_Assert(request, "PrepareRequest was provided a null request.");

            switch (request->GetOperationType())
            {
            case FileRequest::Operation::Read:
                if (IsServicedByThisDrive(request->GetReadData().m_path->GetAbsolutePath()))
                {
                    m_pendingRequests.push_back(request);
                }
                else
                {
                    StreamStackEntry::PrepareRequest(request);
                }
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

        bool StorageDriveWin::ExecuteRequests()
        {
            if (!m_pendingRequests.empty())
            {
                FileRequest* request = m_pendingRequests.front();
                ReadRequest(request);
                m_pendingRequests.pop_front();
                StreamStackEntry::ExecuteRequests();
                return true;
            }
            else
            {
                return StreamStackEntry::ExecuteRequests();
            }
        }

        s32 StorageDriveWin::GetAvailableRequestSlots() const
        {
            if (m_cachesInitialized)
            {
                s32 availableSlots = s32{ 1 } -aznumeric_cast<s32>(m_pendingRequests.size());
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

        void StorageDriveWin::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            AZStd::deque<AZStd::shared_ptr<Request>>& externalPending)
        {
            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, externalPending);

            AZStd::chrono::system_clock::time_point currentTime = now;
            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTimeUSec = aznumeric_caster(m_readTimeAverage.GetTotal().count());
            AZStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();

            const RequestPath* m_activeFile = nullptr;

            // Requests will be read in the order they are queued so the time they'll complete will be cumulative.
            if (m_hasSeekPenalty)
            {
                u64 currentOffset = m_activeOffset;
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
                        if (!IsServicedByThisDrive(readData.m_path->GetAbsolutePath()))
                        {
                            continue;
                        }
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
                        if (!IsServicedByThisDrive(readData.m_compressionInfo.m_archiveFilename.GetAbsolutePath()))
                        {
                            continue;
                        }
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
                        if (IsServicedByThisDrive(request->m_filename.GetAbsolutePath()))
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
            }
            else
            {
                for (FileRequest* request : m_pendingRequests)
                {
                    FileRequest::ReadData& readData = request->GetReadData();
                    if (!m_activeFile || (*m_activeFile != *readData.m_path && FindFileInCache(*readData.m_path) == s_fileNotFound))
                    {
                        currentTime += fileOpenCloseTimeAverage;
                    }
                    currentTime += AZStd::chrono::microseconds(static_cast<u64>((readData.m_size * totalReadTimeUSec) / totalBytesRead));
                    request->SetEstimatedCompletion(currentTime);
                    m_activeFile = readData.m_path;
                }

                for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
                {
                    u64 readSize = 0;
                    if ((*requestIt)->GetOperationType() == FileRequest::Operation::Read)
                    {
                        FileRequest::ReadData& readData = (*requestIt)->GetReadData();
                        if (!IsServicedByThisDrive(readData.m_path->GetAbsolutePath()))
                        {
                            continue;
                        }
                        readSize = readData.m_size;
                        if (!m_activeFile || *m_activeFile != *readData.m_path)
                        {
                            currentTime += fileOpenCloseTimeAverage;
                        }
                        m_activeFile = readData.m_path;
                    }
                    else if ((*requestIt)->GetOperationType() == FileRequest::Operation::CompressedRead)
                    {
                        FileRequest::CompressedReadData& readData = (*requestIt)->GetCompressedReadData();
                        if (!IsServicedByThisDrive(readData.m_compressionInfo.m_archiveFilename.GetAbsolutePath()))
                        {
                            continue;
                        }
                        readSize = readData.m_compressionInfo.m_compressedSize;
                        m_activeFile = &readData.m_compressionInfo.m_archiveFilename;
                        // Don't add the fileOpenCloseTimeAverage as the archive to read from is going to be open in most cases.
                    }
                    else
                    {
                        continue;
                    }

                    currentTime += AZStd::chrono::microseconds(static_cast<u64>((readSize * totalReadTimeUSec) / totalBytesRead));
                    (*requestIt)->SetEstimatedCompletion(currentTime);
                }

                // The pending requests can be reordered based on various factors so lean towards a worst case prediction.
                for (AZStd::shared_ptr<Request>& request : externalPending)
                {
                    if (request->IsReadOperation())
                    {
                        if (IsServicedByThisDrive(request->m_filename.GetAbsolutePath()))
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
            }
        }

        bool StorageDriveWin::GetFileSize(u64& result, const RequestPath& filePath)
        {
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "StorageDriveWin::GetFileSize %s", m_name.c_str());

            if (!IsServicedByThisDrive(filePath.GetAbsolutePath()))
            {
                return m_next ? m_next->GetFileSize(result, filePath) : false;
            }

            LARGE_INTEGER fileSize;
            
            // If the file is already open, use the file handle which usually is cheaper than asking for the file by name.
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != s_fileNotFound)
            {
                AZ_Assert(m_fileHandles[cacheIndex] != INVALID_HANDLE_VALUE,
                    "File path '%s' doesn't have an associated file handle.", m_filePaths[cacheIndex].GetRelativePath());
                if (GetFileSizeEx(m_fileHandles[cacheIndex], &fileSize) == FALSE)
                {
                    // Failed to read the file size, so let the next entry in the stack try.
                    return m_next ? m_next->GetFileSize(result, filePath) : false;
                }
                result = aznumeric_caster(fileSize.QuadPart);
                return true;
            }
            
            // The file is not open yet, so try to get the file size by name.
            WIN32_FILE_ATTRIBUTE_DATA attributes;
            if (GetFileAttributesEx(filePath.GetAbsolutePath(), GetFileExInfoStandard, &attributes) == FALSE)
            {
                // Failed to read the file size, so let the next entry in the stack try.
                return m_next ? m_next->GetFileSize(result, filePath) : false;
            }
            fileSize.LowPart = attributes.nFileSizeLow;
            fileSize.HighPart = attributes.nFileSizeHigh;
            result = aznumeric_caster(fileSize.QuadPart);
            return true;
        }

        void StorageDriveWin::ReadRequest(FileRequest* request)
        {
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "StorageDriveWin::ReadRequest %s", m_name.c_str());

            FileRequest::ReadData& data = request->GetReadData();

            if (!m_cachesInitialized)
            {
                m_fileLastUsed.resize(m_maxFileHandles, AZStd::chrono::system_clock::time_point::min());
                m_filePaths.resize(m_maxFileHandles);
                m_fileHandles.resize(m_maxFileHandles, INVALID_HANDLE_VALUE);
                
                m_cachesInitialized = true;
            }

            HANDLE file = INVALID_HANDLE_VALUE;

            // If the file is already open, use that file handle and update it's last touched time.
            size_t cacheIndex = FindFileInCache(*data.m_path);
            if (cacheIndex != s_fileNotFound)
            {
                file = m_fileHandles[cacheIndex];
                m_fileLastUsed[cacheIndex] = AZStd::chrono::high_resolution_clock::now();
            }
            
            // If the file is not open, eject the oldest entry from the cache and open the file for reading.
            if (file == INVALID_HANDLE_VALUE)
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
                file = CreateFile(data.m_path->GetAbsolutePath(), FILE_GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (file == INVALID_HANDLE_VALUE)
                {
                    // Failed to open the file, so let the next entry in the stack try.
                    StreamStackEntry::PrepareRequest(request);
                    return;
                }

                if (m_fileHandles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_fileHandles[cacheIndex]);
                }
                m_fileHandles[cacheIndex] = file;
                m_fileLastUsed[cacheIndex] = AZStd::chrono::high_resolution_clock::now();
                m_filePaths[cacheIndex] = *data.m_path;
            }
            
            AZ_Assert(file, "While searching for file '%s' StorageDevice::ReadRequest failed to detect a problem.", data.m_path->GetRelativePath());
            
            OVERLAPPED overlapped;
            memset(&overlapped, 0, sizeof(OVERLAPPED));
            overlapped.Offset = static_cast<DWORD>(data.m_offset);
            overlapped.OffsetHigh = static_cast<DWORD>(data.m_offset >> 32);
            DWORD bytesRead = 0;
            {
                TIMED_AVERAGE_WINDOW_SCOPE(m_readTimeAverage);
                if (::ReadFile(file, data.m_output, aznumeric_cast<DWORD>(data.m_size), &bytesRead, &overlapped) == FALSE)
                {
                    // Failed to open the file, so let the next entry in the stack try.
                    StreamStackEntry::PrepareRequest(request);
                    return;
                }
            }
            m_activeCacheSlot = cacheIndex;
            m_activeOffset = data.m_offset + bytesRead;
            m_readSizeAverage.PushEntry(bytesRead);

            request->SetStatus(bytesRead == data.m_size ? FileRequest::Status::Completed : FileRequest::Status::Failed);
            m_context->MarkRequestAsCompleted(request);
        }

        bool StorageDriveWin::CancelRequest(FileRequest* request)
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

        void StorageDriveWin::FlushCache(const RequestPath& filePath)
        {
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != s_fileNotFound)
            {
                if (m_fileHandles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_fileHandles[cacheIndex]);
                    m_fileHandles[cacheIndex] = INVALID_HANDLE_VALUE;
                }
                m_fileLastUsed[cacheIndex] = AZStd::chrono::system_clock::time_point();
                m_filePaths[cacheIndex].Clear();
            }
            StreamStackEntry::FlushCache(filePath);
        }

        void StorageDriveWin::FlushEntireCache()
        {
            size_t numFiles = m_filePaths.size();
            for (size_t i = 0; i < numFiles; ++i)
            {
                if (m_fileHandles[i] != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(m_fileHandles[i]);
                    m_fileHandles[i] = INVALID_HANDLE_VALUE;
                }
                m_fileLastUsed[i] = AZStd::chrono::system_clock::time_point();
                m_filePaths[i].Clear();
            }
            StreamStackEntry::FlushEntireCache();
        }

        size_t StorageDriveWin::FindFileInCache(const RequestPath& filePath) const
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

        bool StorageDriveWin::IsServicedByThisDrive(const char* filePath) const
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore); 
            
            // This approach doesn't allow paths to be resolved to the correct drive when junctions are used or when a drive
            // is mapped as folder of another drive. To do this correctly "GetVolumePathName" should be used, but this takes
            // about 1 to 1.5 ms per request, so this introduces unacceptably large overhead particularly when the user has
            // multiple disks.
            for (const AZStd::string& drivePath : m_drivePaths)
            {
                if (azstrnicmp(filePath, drivePath.c_str(), drivePath.length()) == 0)
                {
                    return true;
                }
            }
            return false;
        }

        void StorageDriveWin::CollectStatistics(AZStd::vector<Statistic>& statistics) const
        {
            if (m_cachesInitialized)
            {
                constexpr double bytesToMB = (1024.0 * 1024.0);
                using DoubleSeconds = AZStd::chrono::duration<double>;

                double totalBytesReadMB = m_readSizeAverage.GetTotal() / bytesToMB;
                double totalReadTimeSec = AZStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
                if (totalReadTimeSec > 0.0)
                {
                    statistics.push_back(Statistic::CreateFloat(m_name, "Read Speed (avg. mbps)", totalBytesReadMB / totalReadTimeSec));
                }

                statistics.push_back(Statistic::CreateInteger(m_name, "File Open & Close (avg. us)", m_fileOpenCloseTimeAverage.CalculateAverage().count()));
                statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", s64{ 1 } - m_pendingRequests.size()));
            }
            StreamStackEntry::CollectStatistics(statistics);
        }
    } // namespace IO
} // namespace AZ