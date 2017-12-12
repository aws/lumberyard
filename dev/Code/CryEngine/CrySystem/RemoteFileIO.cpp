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
#include <stdio.h>
#include <sys/stat.h>
#include <AzCore/std/time.h>
#include <string>
#include "RemoteFileIO.h"
#include "ProjectDefines.h"
#include "CryAssert.h"
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzCore/Serialization/Utils.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/std/algorithm.h> // for GetMin()
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.

namespace AZ
{
    namespace IO
    {
        using namespace AzFramework::AssetSystem;

        const int REFRESH_FILESIZE_TIME = 500;// ms

        RemoteFileIO::RemoteFileIO(FileIOBase* excludedFileIO)
        {
            m_excludedFileIO = excludedFileIO;
        }

        RemoteFileIO::~RemoteFileIO()
        {
            delete m_excludedFileIO; //for now delete it, when we change to always create local file io we wont

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            while (!m_remoteFiles.empty())
            {
                HandleType fileHandle = m_remoteFiles.begin()->first;
                Close(fileHandle);
            }

    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            while (!m_excludedFiles.empty())
            {
                HandleType fileHandle = m_excludedFiles.begin()->first;
                Close(fileHandle);
            }
    #endif
        }

    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
        void RemoteFileIO::AddExclusion(const char* exclusion_regex)
        {
            if (!exclusion_regex)
            {
                return;
            }

            m_excludes.push_back(std::regex(exclusion_regex, std::regex_constants::ECMAScript | std::regex_constants::icase));
        }

        bool RemoteFileIO::IsFileExcluded(const char* filePath)
        {
            if (!m_excludedFileIO)
            {
                return false;
            }

            if (!filePath)
            {
                return false;
            }

            for (auto excludedFileIter = m_excludes.begin(); excludedFileIter != m_excludes.end(); ++excludedFileIter)
            {
                if (std::regex_search(filePath, *excludedFileIter))
                {
                    return true;
                }
            }

            return false;
        }

        bool RemoteFileIO::IsFileRemote(const char* filePath)
        {
            if (!m_excludedFileIO)
            {
                return true;
            }
            else
            {
                return !IsFileExcluded(filePath);
            }
        }
    #endif

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
        void RemoteFileIO::RemapFile(const char* filePath, const char* remappedFilePath)
        {
            //error checks
            if (!filePath)
            {
                return;
            }

            if (!remappedFilePath)
            {
                return;
            }

            auto fileRemapIter = m_fileRemap.find(filePath);
            if (fileRemapIter != m_fileRemap.end())
            {
                fileRemapIter->second = remappedFilePath;
            }
            else
            {
                m_fileRemap.insert(std::make_pair(filePath, remappedFilePath));
            }
        }

        const char* RemoteFileIO::GetFileRemap(const char* filePath, bool returnInputIfNotRemapped) const
        {
            //error checks
            if (!filePath)
            {
                return nullptr;
            }

            auto fileRemapIter = m_fileRemap.find(filePath);
            if (fileRemapIter != m_fileRemap.end())
            {
                return fileRemapIter->second.c_str();
            }

            if (returnInputIfNotRemapped)
            {
                return filePath;
            }

            return nullptr;
        }
    #endif

    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
        bool RemoteFileIO::IsFileExcluded(HandleType fileHandle)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_excludedFileIOGuard);
            return m_excludedFiles.find(fileHandle) != m_excludedFiles.end();
        }

        bool RemoteFileIO::IsFileRemote(HandleType fileHandle)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            return m_remoteFiles.find(fileHandle) != m_remoteFiles.end();
        }
    #endif

        Result RemoteFileIO::Open(const char* filePath, OpenMode openMode, HandleType& fileHandle)
        {
            //error checks
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if (openMode == OpenMode::Invalid)
            {
                return ResultCode::Error;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            //if we have excluded io, see if this filePath is excluded, if so route this op to excluded io
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    Result returnValue = m_excludedFileIO->Open(filePath, openMode, fileHandle);
                    //if we succeeded add this file handle to the excluded map
                    if (returnValue == ResultCode::Success)
                    {
                        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_excludedFileIOGuard);
                        m_excludedFiles.insert(std::make_pair(fileHandle, filePath));
                    }

                    return returnValue;
                }
            }
    #endif
            //build a request
            uint32_t mode = static_cast<uint32_t>(openMode);
            FileOpenRequest request(filePath, mode);
            FileOpenResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Open: unable to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_returnCode);
            if (returnValue == ResultCode::Success)
            {
                fileHandle = response.m_fileHandle;
                //track the open file handles
            
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);

                m_remoteFiles.insert(AZStd::make_pair(fileHandle, filePath));
                m_remoteFileCache.insert(fileHandle); // initialize the file cache too
            }

            return returnValue;
        }

        Result RemoteFileIO::Close(HandleType fileHandle)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_excludedFileIOGuard);
                auto excludedFileIter = m_excludedFiles.find(fileHandle);
                if (excludedFileIter != m_excludedFiles.end())
                {
                    Result returnValue = m_excludedFileIO->Close(fileHandle);
                    if (returnValue == ResultCode::Success)
                    {
                        m_excludedFiles.erase(excludedFileIter);
                    }

                    return returnValue;
                }
            }
    #endif
            //build a request
            FileCloseRequest request(fileHandle);
            if (!SendRequest(request))
            {
                AZ_Error("RemoteFileIO", false, "Failed to send request, handle=%u", fileHandle);
                return ResultCode::Error;
            }

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            m_remoteFiles.erase(fileHandle);
            m_remoteFileCache.erase(fileHandle);

            return ResultCode::Success;
        }

        Result RemoteFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(fileHandle))
                {
                    return m_excludedFileIO->Tell(fileHandle, offset);
                }
            }
    #endif
            // tell requests/responses are 100% cache predicted.
            VFSCacheLine& myCacheLine = GetCacheLine(fileHandle);

            offset = myCacheLine.m_position;
            offset -= (myCacheLine.m_cacheLookaheadBuffer.size() - myCacheLine.m_cacheLookaheadPos);
            return Result(ResultCode::Success);
        }

        Result RemoteFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(fileHandle))
                {
                    return m_excludedFileIO->Seek(fileHandle, offset, type);
                }
            }
    #endif
            VFSCacheLine& myCacheLine = GetCacheLine(fileHandle);
            if (type == AZ::IO::SeekType::SeekFromCurrent)
            {
                if (offset == 0)
                {
                    // no point in proceeding.
                    return Result(ResultCode::Success);
                }

                AZ::u64 remainingBytesInCache = (myCacheLine.m_cacheLookaheadBuffer.size() - myCacheLine.m_cacheLookaheadPos);

                if (offset < 0)
                {
                    // we want to seek backwards which might land us still within the cached area.
                    if ((-offset) <= myCacheLine.m_cacheLookaheadPos)
                    {
                        // we want to seek backwards less than we have in the cache.
                        // this does not change the physical location.
                        myCacheLine.m_cacheLookaheadPos += offset;
                            
                        return Result(ResultCode::Success);
                    }
                }
                else if (offset > 0)
                {
                    // we want to seek ahead, but we may yet land within the cached area.
                    if (offset < remainingBytesInCache)
                    {
                        myCacheLine.m_cacheLookaheadPos += static_cast<AZ::u64>(offset);
                        // this does not change the physical location.
                        return Result(ResultCode::Success);
                    }

                }

                if (remainingBytesInCache > 0)
                {
                    // the cache is ahead of the file by aheadBy bytes.
                    offset -= static_cast<AZ::s64>(remainingBytesInCache);
                }
                // if we get here, its outside the actual bounds of seek and we are going to perform a physical seek.
            }
            else if (type == AZ::IO::SeekType::SeekFromStart)
            {
                // maybe we can still do this, assuming that the position actually lies within our buffer
                AZ::u64 bufferStartPos = myCacheLine.m_position - myCacheLine.m_cacheLookaheadBuffer.size();
                if ((offset < myCacheLine.m_position) && (offset >= bufferStartPos))
                {
                    // the offset we wish to seek to is actually still within the buffer.
                    // so for example, our buffer starts at offset 1000 and has 2000 bytes in it (so it covers the range of 1000-2999)
                    // if we seek to 1500 from the beginning of the file, we can just move the cache lookahead pos (which tracks how far it is into the buffer)
                    // to 500
                    myCacheLine.m_cacheLookaheadPos = offset - bufferStartPos;
                    return Result(ResultCode::Success);
                }

                // there's also another thing that happens - the code sometimes calls to seek to 0 immediately after opening
                // there's also no point in issuing any kind of invalidation or seek after that
                if (offset == myCacheLine.m_position) // we are being asked to seek to the place the file is already at, physically
                {
                    myCacheLine.m_cacheLookaheadPos = myCacheLine.m_cacheLookaheadBuffer.size();
                    return Result(ResultCode::Success);
                }
            }

            InvalidateReadaheadCache(fileHandle);
            return SeekInternal(fileHandle, myCacheLine, offset, type);
        }

        Result RemoteFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_excludedFileIOGuard);
                auto excludedIter = m_excludedFiles.find(fileHandle);
                if (excludedIter != m_excludedFiles.end())
                {
                    return Size(excludedIter->second.c_str(), size);
                }
            }
    #endif
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
                VFSCacheLine& myCacheLine = GetCacheLine(fileHandle);

                // do we even have to check?
                AZ::u64 msNow = AZStd::GetTimeUTCMilliSecond();
                if (msNow - myCacheLine.m_fileSizeSnapshotTime > REFRESH_FILESIZE_TIME)
                {
                    auto remoteIter = m_remoteFiles.find(fileHandle);
                    if (remoteIter != m_remoteFiles.end())
                    {
                        Result rc = Size(remoteIter->second.c_str(), size);
                        myCacheLine.m_fileSizeSnapshotTime = msNow;
                        myCacheLine.m_fileSizeSnapshot = size;
                        return rc;
                    }
                }
                else
                {
                    size = myCacheLine.m_fileSizeSnapshot;
                    return ResultCode::Success;
                }
            }

            return ResultCode::Error;
        }

        Result RemoteFileIO::Size(const char* filePath, AZ::u64& size)
        {
            //error checks
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                return ResultCode::Error;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->Size(filePath, size);
                }
            }
    #endif

            FileSizeRequest request(filePath);
            FileSizeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Size: failed to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            if (returnValue == ResultCode::Success)
            {
                size = response.m_size;
            }
            return returnValue;
        }

        void RemoteFileIO::InvalidateReadaheadCache(HandleType handle)
        {
            VFSCacheLine& line = GetCacheLine(handle);
            line.m_cacheLookaheadPos = 0;
            line.m_cacheLookaheadBuffer.clear();
        }

        AZ::u64 RemoteFileIO::ReadFromCache(HandleType handle, void* buffer, AZ::u64 requestedSize)
        {
            auto found = m_remoteFileCache.find(handle);
            if (found == m_remoteFileCache.end())
            {
                return 0;
            }

            VFSCacheLine& cacheLine = found->second;

            AZ::u64 bytesRead = 0;
            
            AZ::u64 remainingBytesInCache = cacheLine.m_cacheLookaheadBuffer.size() - cacheLine.m_cacheLookaheadPos;
            AZ::u64 remainingBytesToRead = requestedSize;
            if (remainingBytesInCache > 0)
            {
                AZ::u64 bytesToReadFromCache = AZStd::GetMin(remainingBytesInCache, remainingBytesToRead);
                memcpy(buffer, cacheLine.m_cacheLookaheadBuffer.data() + cacheLine.m_cacheLookaheadPos, bytesToReadFromCache);
                bytesRead = bytesToReadFromCache;
                remainingBytesToRead -= bytesToReadFromCache;
                cacheLine.m_cacheLookaheadPos += bytesToReadFromCache;
            }
            // note that readinf from the cache does not alter the physical position of the remote file, so we do not alter
            // the m_position value. 
            return bytesRead;
        }

        Result RemoteFileIO::SeekInternal(HandleType handle, VFSCacheLine& cacheLine, AZ::s64 offset, SeekType type)
        {
            // note that seeking in a file does not need to invalidate its size.
            FileSeekRequest request(handle, static_cast<AZ::u32>(type), offset);
            // note, seeks are predicted, and do not ask for a response.
            if (!SendRequest(request))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::SeekInternal: Failed to send request, handle=%u", handle);
                return Result(ResultCode::Error);
            }

            if (type == AZ::IO::SeekType::SeekFromCurrent)
            {
                cacheLine.m_position += offset;
            }
            else if (type == AZ::IO::SeekType::SeekFromStart)
            {
                cacheLine.m_position = offset;
            }
            else if (type == AZ::IO::SeekType::SeekFromEnd)
            {
                AZ::u64 sizeValue = 0;
                Size(handle, sizeValue);

                if (offset >= 0)
                {
                    // seek beyond end is not allowed
                    cacheLine.m_position = sizeValue;
                }
                else // (offset < 0)
                {
                    offset = -offset;
                    if (offset >= sizeValue)
                    {
                        cacheLine.m_position = 0; // seek beyond start
                    }
                    else
                    {
                        cacheLine.m_position = sizeValue - offset; // seek some place after start.
                    }
                }
            }

            return Result(ResultCode::Success);
        }

        Result RemoteFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
#ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(fileHandle))
                {
                    return m_excludedFileIO->Read(fileHandle, buffer, size, failOnFewerThanSizeBytesRead, bytesRead);
                }
            }
#endif
          
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            size_t remainingBytesToRead = size;
            size_t bytesReadFromCache = ReadFromCache(fileHandle, buffer, size);
            remainingBytesToRead -= bytesReadFromCache;

            if (bytesRead)
            {
                *bytesRead = bytesReadFromCache;
            }

            if (remainingBytesToRead == 0)
            {
                return ResultCode::Success;
            }

            buffer = reinterpret_cast<char*>(buffer) + bytesReadFromCache;

            // if we get here, our cache is dry and we still have data to read!

            if (remainingBytesToRead >= CACHE_LOOKAHEAD_SIZE) // no point in caching big reads.
            {
                FileReadRequest request(fileHandle, remainingBytesToRead, failOnFewerThanSizeBytesRead);
                FileReadResponse response;
                if (SendRequest(request, response))
                {
                    // regardless of the result, we still have to move our cached physical pointer.
                    cacheLine.m_position += response.m_data.size();
                    ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
                    if (returnValue == ResultCode::Success)
                    {
                        if (bytesRead)
                        {
                            *bytesRead = (*bytesRead) + response.m_data.size();
                        }
                        AZ_Assert(remainingBytesToRead >= response.m_data.size(), "RemoteFileIO::Read: Response will overflow request buffer");
                        memcpy(buffer, response.m_data.data(), AZStd::GetMin<AZ::u64>(response.m_data.size(), remainingBytesToRead));
                    }
                    return returnValue;
                }
            }
            else
            {
                // if we get here, the cache is now empty.
                // its quite possible for the cache to have been drained simply becuase there is no more data in the file!
                if (!cacheLine.m_cacheLookaheadBuffer.empty())
                {
                    // we had data in the cache.  It wasn't enough to satisfy the request, so we've consumed all the data in our cache at this point
                    // if we're at the end of the file there is no point in proceeding.
                    if (Eof(fileHandle))
                    {
                        // there is no more data to be had.
                        if ((failOnFewerThanSizeBytesRead) && (remainingBytesToRead > 0))
                        {
                            return ResultCode::Error;
                        }
                        return ResultCode::Success;
                    }
                }

                FileReadRequest request(fileHandle, CACHE_LOOKAHEAD_SIZE, false);
                FileReadResponse response;
                if (SendRequest(request, response))
                {
                    ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
                    
                    // regardless of the result, we still have to move our cached physical pointer.
                    cacheLine.m_position += response.m_data.size();

                    if (returnValue == ResultCode::Success)
                    {
                        AZ_Assert(response.m_data.size() <= CACHE_LOOKAHEAD_SIZE, "RemoteFileIO::Read: Response will overflow request buffer (cache)");
                        cacheLine.m_cacheLookaheadBuffer.resize(response.m_data.size());
                        memcpy(cacheLine.m_cacheLookaheadBuffer.data(), response.m_data.data(), response.m_data.size());
                        cacheLine.m_cacheLookaheadPos = 0;
                        bytesReadFromCache = ReadFromCache(fileHandle, buffer, remainingBytesToRead);
                        remainingBytesToRead -= bytesReadFromCache;
                        if (bytesRead)
                        {
                            *bytesRead = (*bytesRead) + bytesReadFromCache;
                        }

                        if ((failOnFewerThanSizeBytesRead) && (remainingBytesToRead > 0))
                        {
                            return ResultCode::Error;
                        }
                    }
                    return returnValue;
                }
            }

            return ResultCode::Error;
        }

        Result RemoteFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(fileHandle))
                {
                    return m_excludedFileIO->Write(fileHandle, buffer, size, bytesWritten);
                }
            }
    #endif

            // We need to seek back to where we should be in the file before we commit a write.
            // This is unnecessary if the cache is empty, or we're at the end of the cache.
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            AZ::u64 remainingBytesInCache = (cacheLine.m_cacheLookaheadBuffer.size() - cacheLine.m_cacheLookaheadPos);

            if (cacheLine.m_cacheLookaheadBuffer.size() && remainingBytesInCache)
            {
                // find out where we are 
                AZ::u64 seekPosition = cacheLine.m_position - remainingBytesInCache;
                Result seekResult = SeekInternal(fileHandle, cacheLine, seekPosition, AZ::IO::SeekType::SeekFromStart);
                if (!seekResult)
                {
                    AZ_Error("RemoteFileIO", false, "RemoteFileIO::Write: failed to seek request, handle=%u", fileHandle);
                    return ResultCode::Error;
                }
            }

            InvalidateReadaheadCache(fileHandle);
            cacheLine.m_fileSizeSnapshotTime = 0; // invalidate file size after write.
            FileWriteRequest request(fileHandle, buffer, size);
            //always async and just return success unless bytesWritten is set then synchronous
            if (bytesWritten)
            {
                FileWriteResponse response;
                if (!SendRequest(request, response))
                {
                    //shouldn't get here... something went wrong...
                    AZ_Error("RemoteFileIO", false, "RemoteFileIO::Write: failed to send request, handle=%u", fileHandle);
                    return ResultCode::Error;
                }
                ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
                if (returnValue == ResultCode::Success)
                {
                    *bytesWritten = response.m_bytesWritten;
                    cacheLine.m_position += response.m_bytesWritten; // assume we moved on by that many bytes.
                }
            }
            else
            {
                // just send the message
                if (!SendRequest(request))
                {
                    //shouldn't get here... something went wrong...
                    AZ_Error("RemoteFileIO", false, "RemoteFileIO::Write: failed to send request, handle=%u", fileHandle);
                    return ResultCode::Error;
                }
                cacheLine.m_position += size; // can only assume we wrote it all.
            }

            return ResultCode::Success;
        }

        Result RemoteFileIO::Flush(HandleType fileHandle)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(fileHandle))
                {
                    return m_excludedFileIO->Flush(fileHandle);
                }
            }
    #endif
            // just send the message, no need to wait for flush response.
            FileFlushRequest request(fileHandle);
            if (!SendRequest(request))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Flush: failed to send request, handle=%u", fileHandle);
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        bool RemoteFileIO::Eof(HandleType fileHandle)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(fileHandle))
                {
                    return m_excludedFileIO->Eof(fileHandle);
                }
            }
    #endif
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            AZ::u64 sizeValue = 0;
            Size(fileHandle, sizeValue);
            AZ::u64 offset = 0;
            Tell(fileHandle, offset);
            return offset >= sizeValue;
        }

        bool RemoteFileIO::Exists(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return false;
            }

            if (!strlen(filePath))
            {
                return false;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->Exists(filePath);
                }
            }
    #endif

            FileExistsRequest request(filePath);
            FileExistsResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Exists: failed to send request for %s", filePath);
                return false;
            }
            return response.m_exists;
        }

        AZ::u64 RemoteFileIO::ModificationTime(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return 0;
            }

            if (!strlen(filePath))
            {
                return 0;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->ModificationTime(filePath);
                }
            }
    #endif

            FileModTimeRequest request(filePath);
            FileModTimeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::ModificationTime: failed to send request for %s", filePath);
                return 0;
            }
            return response.m_modTime;
        }

        AZ::u64 RemoteFileIO::ModificationTime(HandleType fileHandle)
        {
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_excludedFileIOGuard);
                auto excludedIter = m_excludedFiles.find(fileHandle);
                if (excludedIter != m_excludedFiles.end())
                {
                    return ModificationTime(excludedIter->second.c_str());
                }
            }
    #endif
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
                auto remoteIter = m_remoteFiles.find(fileHandle);
                if (remoteIter != m_remoteFiles.end())
                {
                    return ModificationTime(remoteIter->second.c_str());
                }
            }

            return 0;
        }

        bool RemoteFileIO::IsDirectory(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return false;
            }

            if (!strlen(filePath))
            {
                return false;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->IsDirectory(filePath);
                }
            }
    #endif
            PathIsDirectoryRequest request(filePath);
            PathIsDirectoryResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::IsDirectory: failed to send request for %s", filePath);
                return false;
            }
            return response.m_isDir;
        }

        bool RemoteFileIO::IsReadOnly(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return false;
            }

            if (!strlen(filePath))
            {
                return false;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->IsReadOnly(filePath);
                }
            }
    #endif

            FileIsReadOnlyRequest request(filePath);
            FileIsReadOnlyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::IsReadOnly: failed to send request for %s", filePath);
                return false;
            }

            return response.m_isReadOnly;
        }

        Result RemoteFileIO::CreatePath(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                return ResultCode::Error;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->CreatePath(filePath);
                }
            }
    #endif

            PathCreateRequest request(filePath);
            PathCreateResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::CreatePath: failed to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            return returnValue;
        }

        Result RemoteFileIO::DestroyPath(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                return ResultCode::Error;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->DestroyPath(filePath);
                }
            }
    #endif
            PathDestroyRequest request(filePath);
            PathDestroyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::DestroyPath: failed to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            return returnValue;
        }

        Result RemoteFileIO::Remove(const char* filePath)
        {
            //error checks
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                return ResultCode::Error;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->Remove(filePath);
                }
            }
    #endif

            FileRemoveRequest request(filePath);
            FileRemoveResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Remove: failed to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            return returnValue;
        }

        Result RemoteFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            //error checks
            if (!sourceFilePath)
            {
                return ResultCode::Error;
            }

            if (!destinationFilePath)
            {
                return ResultCode::Error;
            }

            if (!strlen(sourceFilePath))
            {
                return ResultCode::Error;
            }

            //fail if the source doesn't exist
            if (!Exists(sourceFilePath))
            {
                return ResultCode::Error;
            }

            bool bSourceExcluded = false;
            bool bDestinationExcluded = false;
    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            sourceFilePath = GetFileRemap(sourceFilePath);
            destinationFilePath = GetFileRemap(destinationFilePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                bSourceExcluded = IsFileExcluded(sourceFilePath);
                bDestinationExcluded = IsFileExcluded(destinationFilePath);
            }

            //if both are excluded then just issue the excluded copy
            if (bSourceExcluded && bDestinationExcluded)
            {
                return m_excludedFileIO->Copy(sourceFilePath, destinationFilePath);
            }
            //else if the only the source or destination is excluded we have to implement the steps of copying ourselves
            else if (bSourceExcluded || bDestinationExcluded)
            {
                //open the source and destination files
                HandleType sourceHandle = InvalidHandle;
                Result res = Open(sourceFilePath, OpenMode::ModeRead | OpenMode::ModeBinary, sourceHandle);
                if (res != ResultCode::Success)
                {
                    return ResultCode::Error;
                }
                if (sourceHandle == InvalidHandle)
                {
                    return ResultCode::Error_HandleInvalid;
                }

                HandleType destHandle = InvalidHandle;
                res = Open(destinationFilePath, OpenMode::ModeWrite | OpenMode::ModeBinary, destHandle);
                if (res != ResultCode::Success)
                {
                    Close(sourceHandle);
                    return ResultCode::Success;
                }
                if (destHandle == InvalidHandle)
                {
                    Close(sourceHandle);
                    return ResultCode::Error_HandleInvalid;
                }

                //get the size of the source file
                uint64_t sourceSize = 0;
                res = Size(sourceHandle, sourceSize);
                if (res != ResultCode::Success)
                {
                    Close(sourceHandle);
                    Close(destHandle);
                    return ResultCode::Error;
                }

                //shortcut 0 length files
                if (sourceSize == 0)
                {
                    Close(sourceHandle);
                    Close(destHandle);
                    return ResultCode::Success;
                }

                const int bufferSize = 64 * 1024;
                char buffer[bufferSize];
                uint64_t totalBytesRead = 0;
                uint64_t bytesRead = 0;
                uint64_t totalBytesWritten = 0;
                uint64_t bytesWritten = 0;
                do
                {
                    Result res = Read(sourceHandle, buffer, bufferSize, false, &bytesRead);
                    if (res != ResultCode::Success)
                    {
                        Close(sourceHandle);
                        Close(destHandle);
                        return ResultCode::Error_HandleInvalid;
                    }
                    totalBytesRead += bytesRead;

                    uint64_t totalBytesWrittenThisPass = 0;
                    do
                    {
                        res = Write(destHandle, buffer + totalBytesWrittenThisPass, bytesRead - totalBytesWrittenThisPass, &bytesWritten);
                        if (res != ResultCode::Success)
                        {
                            Close(sourceHandle);
                            Close(destHandle);
                            return ResultCode::Error_HandleInvalid;
                        }
                        totalBytesWrittenThisPass += bytesWritten;
                        totalBytesWritten += bytesWritten;
                    } while (totalBytesWrittenThisPass < bytesRead);

                    CRY_ASSERT(totalBytesWrittenThisPass == bytesRead);
                } while (totalBytesRead < sourceSize);

                CRY_ASSERT(totalBytesRead == sourceSize);
                CRY_ASSERT(totalBytesWritten == sourceSize);

                return ResultCode::Success;
            }
    #endif
            //else both are remote so just issue the remote copy command
            FileCopyRequest request(sourceFilePath, destinationFilePath);
            FileCopyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Copy: failed to send request for %s -> %s", sourceFilePath, destinationFilePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            return returnValue;
        }

        Result RemoteFileIO::Rename(const char* sourceFilePath, const char* destinationFilePath)
        {
            //error checks
            if (!sourceFilePath)
            {
                return ResultCode::Error;
            }

            if (!destinationFilePath)
            {
                return ResultCode::Error;
            }

            if (!strlen(sourceFilePath))
            {
                return ResultCode::Error;
            }

            //fail if the source doesn't exist
            if (!Exists(sourceFilePath))
            {
                return ResultCode::Error;
            }

            if (!strlen(destinationFilePath))
            {
                return ResultCode::Error;
            }

            //we are going to access shared memory so lock and copy the results into our memory
            bool bSourceExcluded = false;
            bool bDestinationExcluded = false;

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            sourceFilePath = GetFileRemap(sourceFilePath);
            destinationFilePath = GetFileRemap(destinationFilePath);
    #endif
            //if the source and destination are the same, shortcut
            if (!strcmp(sourceFilePath, destinationFilePath))
            {
                return ResultCode::Error;
            }
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                bSourceExcluded = IsFileExcluded(sourceFilePath);
                bDestinationExcluded = IsFileExcluded(destinationFilePath);
            }
    #endif
            //if the destination exists
            if (Exists(destinationFilePath))
            {
                //if its read only fail
                if (IsReadOnly(destinationFilePath))
                {
                    return ResultCode::Error;
                }
            }

    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            //if both are excluded then just issue the excluded command
            if (bSourceExcluded && bDestinationExcluded)
            {
                return m_excludedFileIO->Rename(sourceFilePath, destinationFilePath);
            }
            //else if the only the source or destination is excluded we have to implement the steps ourselves
            else if (bSourceExcluded || bDestinationExcluded)
            {
                Result res = Copy(sourceFilePath, destinationFilePath);
                if (res == ResultCode::Success)
                {
                    return Remove(sourceFilePath);
                }

                return ResultCode::Error;
            }
    #endif
            //else both are remote so just issue the remote command
            FileRenameRequest request(sourceFilePath, destinationFilePath);
            FileRenameResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::Rename: failed to send request for %s -> %s", sourceFilePath, destinationFilePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            return returnValue;
        }

        Result RemoteFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            //error checks
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if (!filter)
            {
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                return ResultCode::Error;
            }

    #ifdef REMOTEFILEIO_USE_FILE_REMAPS
            filePath = GetFileRemap(filePath);
    #endif
    #ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            if (m_excludedFileIO)
            {
                if (IsFileExcluded(filePath))
                {
                    return m_excludedFileIO->FindFiles(filePath, filter, callback);
                }
            }
    #endif

            FindFilesRequest request(filePath, filter);
            FindFilesResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Error("RemoteFileIO", false, "RemoteFileIO::FindFiles: could not send request for %s, %s", filePath, filter);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            if (returnValue == ResultCode::Success)
            {
                // callbacks
                const uint64_t numFiles = response.m_files.size();
                for (uint64_t fileIdx = 0; fileIdx < numFiles; ++fileIdx)
                {
                    const char* fileName = response.m_files[fileIdx].c_str();
                    if (!callback(fileName))
                    {
                        fileIdx = numFiles;//we are done
                    }
                }
            }

            return returnValue;
        }

        void RemoteFileIO::SetAlias(const char* alias, const char* path)
        {
        }

        const char* RemoteFileIO::GetAlias(const char* alias)
        {
            return nullptr;
        }

        void RemoteFileIO::ClearAlias(const char* alias)
        {
        }

        AZ::u64 RemoteFileIO::ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const
        {
            return strlen(inOutBuffer);
        }

        bool RemoteFileIO::ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize)
        {
            return false;
        }

        bool RemoteFileIO::GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            const auto fileIt = m_remoteFiles.find(fileHandle);
            if (fileIt != m_remoteFiles.end())
            {
                AZ_Assert(filenameSize >= fileIt->second.length(), "RemoteFileIO::GetFilename: Result buffer is too small to store filename: %u vs %s", filenameSize, fileIt->second.length());
                if (filenameSize >= fileIt->second.length())
                {
                    strncpy(filename, fileIt->second.c_str(), fileIt->second.length());
                    return true;
                }
            }
            return false;
        }

        // (assumes you're inside a remote files guard lock already for creation, which is true since we create one on open)
        VFSCacheLine& RemoteFileIO::GetCacheLine(HandleType handle)
        {
            auto found = m_remoteFileCache.find(handle);
            if (found != m_remoteFileCache.end())
            {
                return found->second;
            }

            // this is a serious error since it may be that you're in an unguarded access to a non-existant handle.
            AZ_ErrorOnce("FILEIO", false, "Attempt to Create or Get cache line missed, did something go wrong with open?");

            auto pairIterBool = m_remoteFileCache.insert(handle);
            return pairIterBool.first->second;
        }

    } // namespace IO
}//namespace AZ