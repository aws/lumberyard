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

#ifdef VFS_CACHING
        const int REFRESH_FILESIZE_TIME = 500;// ms
        const size_t CACHE_LOOKAHEAD_SIZE = 1024 * 64;
#endif
        const size_t READ_CHUNK_SIZE = 1024 * 64;

        RemoteFileIO::RemoteFileIO()
        {
        }

        RemoteFileIO::~RemoteFileIO()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            while (!m_remoteFiles.empty())
            {
                HandleType fileHandle = m_remoteFiles.begin()->first;
                Close(fileHandle);
            }
        }

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

            //build a request
            uint32_t mode = static_cast<uint32_t>(openMode);
            FileOpenRequest request(filePath, mode);
            FileOpenResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::Open: unable to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_returnCode);
            if (returnValue == ResultCode::Success)
            {
                fileHandle = response.m_fileHandle;

                //track the open file handles
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);

                m_remoteFiles.insert(AZStd::make_pair(fileHandle, filePath));

#ifdef VFS_CACHING
                m_remoteFileCache.insert(fileHandle); // initialize the file cache too
                
                VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
                
                AZ::u64 fileSize = 0;
                Size(fileHandle, fileSize);
                
                //if we successfully opened the file for reading fill the read ahead cache
                if(AnyFlag(openMode & OpenMode::ModeRead))
                {
                    FileReadRequest readRequest(fileHandle, CACHE_LOOKAHEAD_SIZE, false);
                    FileReadResponse readResponse;
                    if (!SendRequest(readRequest, readResponse))
                    {
                        AZ_Assert(false, "Failed to send read request, handle=%u", fileHandle);
                    }
                    //note the response could be ANY size, could be more or less so be careful
                    AZ::u64 readResponseDataSize = readResponse.m_data.size();
                    cacheLine.Write(readResponse.m_data.data(), readResponseDataSize);
                }
#endif
            }

            return returnValue;
        }

        Result RemoteFileIO::Close(HandleType fileHandle)
        {
            //build a request
            FileCloseRequest request(fileHandle);
            if (!SendRequest(request))
            {
                AZ_Assert(false, "Failed to send request, handle=%u", fileHandle);
                return ResultCode::Error;
            }

            //clean up the handle and cache
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            m_remoteFiles.erase(fileHandle);
#ifdef VFS_CACHING
            m_remoteFileCache.erase(fileHandle);
#endif
            return ResultCode::Success;
        }

        Result RemoteFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
#ifdef VFS_CACHING
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            offset = cacheLine.CacheFilePosition();
            return ResultCode::Success;
#else
            FileTellRequest request(fileHandle);
            FileTellResponse responce;
            if(!SendRequest(request, responce))
            {
                AZ_Assert(false, "RemoteFileIO::Tell: Failed to send tell request, fileHandle=%u", fileHandle);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(responce.m_resultCode);
            if (returnValue == ResultCode::Error)
            {
                AZ_Assert(false, "RemoteFileIO::Tell: tell request failed, fileHandle=%u", fileHandle);
                return ResultCode::Error;
            }

            offset = responce.m_offset;

            return ResultCode::Success;
#endif
        }

        Result RemoteFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
#ifdef VFS_CACHING
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);

            //if we land in the cache all we need to do is adjust the cache position and return
            //calculate the new position in the file
            //VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            AZ::u64 fileSize = cacheLine.m_fileSizeSnapshot;
            AZ::s64 newFilePosition = 0;
            if (type == AZ::IO::SeekType::SeekFromCurrent)
            {
                newFilePosition = cacheLine.CacheFilePosition() + offset;
            }
            else if (type == AZ::IO::SeekType::SeekFromStart)
            {
                newFilePosition = offset;
            }
            else if (type == AZ::IO::SeekType::SeekFromEnd)
            {
                Result res = Size(fileHandle, fileSize);
                if( res == ResultCode::Error )
                {
                    AZ_Assert(false, "RemoteFileIO::Seek: Failed to get file size, fileHandle=%u", fileHandle);
                }
                newFilePosition = fileSize + offset;
            }
            else
            {
                AZ_Assert(false, "RemoteFileIO::Seek: unknown seektype, fileHandle=%u", fileHandle);
                return ResultCode::Error;
            }

            //bound check
            //note that seeking beyond end or before begining is system dependant
            //therefore we will define that on all platforms it is not allowed
            if(newFilePosition < 0)
            {
                AZ_Assert(false, "RemoteFileIO::Seek: seek to a position before the begining of a file?!, fileHandle=%u", fileHandle);
                newFilePosition = 0;
            }
            else
            {
                Result res = Size(fileHandle, fileSize);
                if(res == ResultCode::Error)
                {
                    AZ_Assert(false, "RemoteFileIO::Seek: Failed to get file size, fileHandle=%u", fileHandle);
                }

                if(newFilePosition > fileSize)
                {
                    AZ_Assert(false, "RemoteFileIO::Seek: seek to a position after the end of a file?!, fileHandle=%u", fileHandle);
                    newFilePosition = fileSize;
                }
            }

            //see if the new calculated position is in the cache
            if(cacheLine.IsFilePositionInCache(newFilePosition))
            {
                //it is, so calculate what the cache position should be given the new file position and
                //set it and return success
                cacheLine.SetCachePositionFromFilePosition(newFilePosition);
                return ResultCode::Success;
            }
            else if(newFilePosition == cacheLine.m_filePosition)
            {
                return ResultCode::Success;
            }

            //we didnt land in the cache
            //perform the seek for real, invalidate and set new file position
            //note when setting a new absolute position we always use SeekFromStart, not the passed in seek type
            FileSeekRequest request(fileHandle, static_cast<AZ::u32>(AZ::IO::SeekType::SeekFromStart), newFilePosition);
            if(!SendRequest(request))
            {
                AZ_Assert(false, "RemoteFileIO::Seek: Failed to send request, fileHandle=%u", fileHandle);
                return ResultCode::Error;
            }

            cacheLine.Invalidate();
            cacheLine.SetFilePosition(newFilePosition);

            return ResultCode::Success;
#else
            FileSeekRequest request(fileHandle, static_cast<AZ::u32>(type), offset);
            if(!SendRequest(request))
            {
                AZ_Assert(false, "RemoteFileIO::Seek: Failed to send request, fileHandle=%u", fileHandle);
                return ResultCode::Error;
            }
            return ResultCode::Success;
#endif
        }

        Result RemoteFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
#ifdef VFS_CACHING
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);

            // do we even have to check?
            AZ::u64 msNow = AZStd::GetTimeUTCMilliSecond();
            if (msNow - cacheLine.m_fileSizeSnapshotTime > REFRESH_FILESIZE_TIME)
            {
                auto remoteIter = m_remoteFiles.find(fileHandle);
                if (remoteIter != m_remoteFiles.end())
                {
                    Result rc = Size(remoteIter->second.c_str(), size);
                    cacheLine.m_fileSizeSnapshotTime = msNow;
                    cacheLine.m_fileSizeSnapshot = size;
                    return rc;
                }
            }
            else
            {
                size = cacheLine.m_fileSizeSnapshot;
                return ResultCode::Success;
            }
#else
            auto remoteIter = m_remoteFiles.find(fileHandle);
            if (remoteIter != m_remoteFiles.end())
            {
               return Size(remoteIter->second.c_str(), size);
            }
#endif
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

            FileSizeRequest request(filePath);
            FileSizeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::Size: failed to send request for %s", filePath);
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            if (returnValue == ResultCode::Error)
            {
                AZ_Assert(false, "RemoteFileIO::Size: size request failed for %s", filePath);
                return ResultCode::Error;
            }
            
            size = response.m_size;
            return ResultCode::Success;
        }

        Result RemoteFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
            size_t remainingBytesToRead = size;

#ifdef VFS_CACHING
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            size_t bytesReadFromCache = cacheLine.Read(buffer, size);
            remainingBytesToRead -= bytesReadFromCache;

            if (bytesRead)
            {
                *bytesRead = bytesReadFromCache;
            }

            buffer = reinterpret_cast<char*>(buffer) + bytesReadFromCache;

            while(remainingBytesToRead)
            {
                // if we get here, our cache is dry and we still have data to read!
                FileReadRequest request(fileHandle, READ_CHUNK_SIZE, false);
                FileReadResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "RemoteFileIO::Read: request failed, fileHandle=%u", fileHandle);
                    return ResultCode::Error;
                }

                //note the response could be ANY size, could be more or less so be careful
                AZ::u64 responseDataSize = response.m_data.size();
                AZ::u64 maxBufferSize = AZStd::GetMin<AZ::u64>(responseDataSize, remainingBytesToRead);

                // regardless of the result, if the request was sent we still have to move our cached physical pointer.
                cacheLine.OffsetFilePosition(responseDataSize);

                ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);

                //only copy as much as we can
                memcpy(buffer, response.m_data.data(), maxBufferSize);
                buffer = reinterpret_cast<char*>(buffer) + maxBufferSize;

                //if we recieved more than we asked for, there might not be enough buffer, if not cache the over-read
                if(responseDataSize > remainingBytesToRead)
                {
                    cacheLine.Write(response.m_data.data() + remainingBytesToRead, responseDataSize - remainingBytesToRead);
                }

                //only reduce by what should have come back
                remainingBytesToRead -= maxBufferSize;

                //only record read bytes of what is going into the buffer and not cached over-read
                if(bytesRead)
                {
                    *bytesRead += maxBufferSize;
                }

                //if we get an error, we only return an error if failOnFewerThanSizeBytesRead
                if(returnValue == ResultCode::Error)
                {
                    AZ_Assert(false, "RemoteFileIO::Read: request failed, fileHandle=%u", fileHandle);
                    if(failOnFewerThanSizeBytesRead && remainingBytesToRead)
                    {
                        return ResultCode::Error;
                    }
                    return ResultCode::Success;
                }
                else if(!responseDataSize)
                {
                    break;
                }
            }

            //they could have asked for more bytes than there is in the file
            if(failOnFewerThanSizeBytesRead && remainingBytesToRead && Eof(fileHandle))
            {
                return ResultCode::Error;
            }

            //if we get here we have satisfied the read request.
            //if the cache is empty try to refill it if not eof.
            if (!cacheLine.RemainingBytes() && !Eof(fileHandle))
            {
                FileReadRequest request(fileHandle, CACHE_LOOKAHEAD_SIZE, false);
                FileReadResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "RemoteFileIO::Read: read ahead request failed, fileHandle=%u", fileHandle);
                    return ResultCode::Error;
                }

                //note the response could be ANY size, could be more or less so be careful
                AZ::u64 responseDataSize = response.m_data.size();
                ResultCode res = static_cast<ResultCode>(response.m_resultCode);
                AZ_Assert(res == ResultCode::Success, "RemoteFileIO::Read: read ahead failed, fileHandle=%u", fileHandle);
                cacheLine.Write(response.m_data.data(), responseDataSize);
            }
#else
            while(remainingBytesToRead)
            {
                AZ::u64 readSize = GetMin<AZ::u64>(remainingBytesToRead, READ_CHUNK_SIZE);
                FileReadRequest request(fileHandle, readSize, false);
                FileReadResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "RemoteFileIO::Read: request failed, fileHandle=%u", fileHandle);
                    return ResultCode::Error;
                }

                //note the response could be ANY size, could be less so be careful
                AZ::u64 responseDataSize = response.m_data.size();
                AZ_Assert(responseDataSize <= remainingBytesToRead, "RemoteFileIO::Read: responseDataSize too large!!!");

                ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);

                //only copy as much as we can
                memcpy(buffer, response.m_data.data(), responseDataSize);
                buffer = reinterpret_cast<char*>(buffer) + responseDataSize;

                //only reduce by what should have come back
                remainingBytesToRead -= responseDataSize;

                //only record read bytes of what is going into the buffer and not cached over-read
                if(bytesRead)
                {
                    *bytesRead += responseDataSize;
                }

                //if we get an error, we only return an error if failOnFewerThanSizeBytesRead
                if(returnValue == ResultCode::Error)
                {
                    AZ_Assert(false, "RemoteFileIO::Read: request failed, fileHandle=%u", fileHandle);
                    if(failOnFewerThanSizeBytesRead && remainingBytesToRead)
                    {
                        return ResultCode::Error;
                    }
                    return ResultCode::Success;
                }
                else if(!responseDataSize)
                {
                    break;
                }
            }

            if(failOnFewerThanSizeBytesRead && remainingBytesToRead)
            {
                return ResultCode::Error;
            }
#endif
            return ResultCode::Success;
        }

        Result RemoteFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
#ifdef VFS_CACHING
            // We need to seek back to where we should be in the file before we commit a write.
            // This is unnecessary if the cache is empty, or we're at the end of the cache.
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            if (cacheLine.m_cacheLookaheadBuffer.size() && cacheLine.RemainingBytes())
            {
                // find out where we are 
                AZ::u64 seekPosition = cacheLine.CacheFilePosition();
                
                // note, seeks are predicted, and do not ask for a response.
                FileSeekRequest request(fileHandle, static_cast<AZ::u32>(AZ::IO::SeekType::SeekFromStart), seekPosition);
                if (!SendRequest(request))
                {
                    AZ_Assert(false, "RemoteFileIO::SeekInternal: Failed to send request, handle=%u", fileHandle);
                    return ResultCode::Error;
                }

                cacheLine.SetFilePosition(seekPosition);
            }

            cacheLine.Invalidate();
            cacheLine.m_fileSizeSnapshotTime = 0; // invalidate file size after write.
#endif
            FileWriteRequest request(fileHandle, buffer, size);
            //always async and just return success unless bytesWritten is set then synchronous
            if (bytesWritten)
            {
                FileWriteResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "RemoteFileIO::Write: failed to send sync write request, handle=%u", fileHandle);
                    return ResultCode::Error;
                }
                ResultCode res = static_cast<ResultCode>(response.m_resultCode);
                AZ_Assert(res == ResultCode::Success, "RemoteFileIO::Write: sync write request failed, handle=%u", fileHandle);
                *bytesWritten = response.m_bytesWritten;
#ifdef VFS_CACHING
                cacheLine.OffsetFilePosition(response.m_bytesWritten);
#endif
            }
            else
            {
                // just send the message and assume we wrote it all successfully.
                if (!SendRequest(request))
                {
                    AZ_Assert(false, "RemoteFileIO::Write: failed to send async write request, handle=%u", fileHandle);
                    return ResultCode::Error;
                }
#ifdef VFS_CACHING
                cacheLine.OffsetFilePosition(size); 
#endif
            }

            return ResultCode::Success;
        }

        Result RemoteFileIO::Flush(HandleType fileHandle)
        {
            // just send the message, no need to wait for flush response.
            FileFlushRequest request(fileHandle);
            if (!SendRequest(request))
            {
                AZ_Assert(false, "RemoteFileIO::Flush: failed to send request, handle=%u", fileHandle);
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        bool RemoteFileIO::Eof(HandleType fileHandle)
        {
#ifdef VFS_CACHING
            VFSCacheLine& cacheLine = GetCacheLine(fileHandle);
            if(cacheLine.RemainingBytes())
            {
                return false;
            }

            AZ::u64 sizeValue = cacheLine.m_fileSizeSnapshot;
            Result res = Size(fileHandle, sizeValue);
            AZ_Assert(res == ResultCode::Success, "RemoteFileIO::Eof: failed to get Size, handle=%u", fileHandle);
            AZ::u64 offset = cacheLine.m_filePosition;
            res = Tell(fileHandle, offset);
            AZ_Assert(res == ResultCode::Success, "RemoteFileIO::Eof: failed to get Tell, handle=%u", fileHandle);
            return offset >= sizeValue;
#else
            AZ::u64 sizeValue = 0;
            Result res = Size(fileHandle, sizeValue);
            AZ_Assert(res == ResultCode::Success, "RemoteFileIO::Eof: failed to get Size, handle=%u", fileHandle);
            AZ::u64 offset = 0;
            res = Tell(fileHandle, offset);
            AZ_Assert(res == ResultCode::Success, "RemoteFileIO::Eof: failed to get Tell, handle=%u", fileHandle);
            return offset >= sizeValue;
#endif
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

            FileExistsRequest request(filePath);
            FileExistsResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::Exists: failed to send request for %s", filePath);
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

            FileModTimeRequest request(filePath);
            FileModTimeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::ModificationTime: failed to send request for %s", filePath);
                return 0;
            }
            return response.m_modTime;
        }

        AZ::u64 RemoteFileIO::ModificationTime(HandleType fileHandle)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            auto remoteIter = m_remoteFiles.find(fileHandle);
            if (remoteIter != m_remoteFiles.end())
            {
                return ModificationTime(remoteIter->second.c_str());
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

            PathIsDirectoryRequest request(filePath);
            PathIsDirectoryResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::IsDirectory: failed to send request for %s", filePath);
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

            FileIsReadOnlyRequest request(filePath);
            FileIsReadOnlyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::IsReadOnly: failed to send request for %s", filePath);
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

            PathCreateRequest request(filePath);
            PathCreateResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::CreatePath: failed to send request for %s", filePath);
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

            PathDestroyRequest request(filePath);
            PathDestroyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::DestroyPath: failed to send request for %s", filePath);
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

            FileRemoveRequest request(filePath);
            FileRemoveResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::Remove: failed to send request for %s", filePath);
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

            //else both are remote so just issue the remote copy command
            FileCopyRequest request(sourceFilePath, destinationFilePath);
            FileCopyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::Copy: failed to send request for %s -> %s", sourceFilePath, destinationFilePath);
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

            //if the source and destination are the same, shortcut
            if (!strcmp(sourceFilePath, destinationFilePath))
            {
                return ResultCode::Error;
            }

            //if the destination exists
            if (Exists(destinationFilePath))
            {
                //if its read only fail
                if (IsReadOnly(destinationFilePath))
                {
                    return ResultCode::Error;
                }
            }

            //else both are remote so just issue the remote command
            FileRenameRequest request(sourceFilePath, destinationFilePath);
            FileRenameResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::Rename: failed to send request for %s -> %s", sourceFilePath, destinationFilePath);
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

            FindFilesRequest request(filePath, filter);
            FindFilesResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::FindFiles: could not send request for %s, %s", filePath, filter);
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
                    azstrncpy(filename, filenameSize, fileIt->second.c_str(), fileIt->second.length());
                    return true;
                }
            }
            return false;
        }
    } // namespace IO
}//namespace AZ