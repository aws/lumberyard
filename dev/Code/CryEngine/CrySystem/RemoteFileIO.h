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

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/RTTI.h>

//ColinB - There is a bug in caching on VFS, so I'm turning it off until I can fix it
//#define VFS_CACHING

namespace AZ
{
    namespace IO
    {

#ifdef VFS_CACHING
        struct VFSCacheLine
        {
            AZ_CLASS_ALLOCATOR(VFSCacheLine, OSAllocator, 0);
            AZStd::vector<char, AZ::OSStdAllocator> m_cacheLookaheadBuffer;
            AZ::u64 m_cacheLookaheadPos = 0;
            AZ::u64 m_fileSizeSnapshot = 0;
            AZ::u64 m_fileSizeSnapshotTime = 0;

            // note that m_filePosition caches the actual physical pointer location of the file - not the cache pos.
            // the actual 'tell position' should be this number minus the number of bytes its ahead by in the cache.
            AZ::u64 m_filePosition = 0;
            
            VFSCacheLine() = default;
            VFSCacheLine(const VFSCacheLine& other) = default;
            
            VFSCacheLine(VFSCacheLine&& other)
            {
                *this = AZStd::move(other);
            }
            
            VFSCacheLine& operator=(VFSCacheLine&& other)
            {
                if (this != &other)
                {
                    m_cacheLookaheadBuffer = AZStd::move(other.m_cacheLookaheadBuffer);
                    m_cacheLookaheadPos = other.m_cacheLookaheadPos;
                    m_fileSizeSnapshot = other.m_fileSizeSnapshot;
                    m_fileSizeSnapshotTime = other.m_fileSizeSnapshotTime;
                    m_filePosition = other.m_filePosition;
                }
                return *this;
            }

            AZ::u64 Read(void* buffer,  AZ::u64 requestedSize)
            {
                AZ::u64 bytesRead = 0;
                AZ::u64 remainingBytesInCache = RemainingBytes();
                AZ::u64 remainingBytesToRead = requestedSize;
                if (remainingBytesInCache > 0)
                {
                    AZ::u64 bytesToReadFromCache = AZStd::GetMin(remainingBytesInCache, remainingBytesToRead);
                    memcpy(buffer, m_cacheLookaheadBuffer.data() + m_cacheLookaheadPos, bytesToReadFromCache);
                    bytesRead = bytesToReadFromCache;
                    remainingBytesToRead -= bytesToReadFromCache;
                    m_cacheLookaheadPos += bytesToReadFromCache;
                }

                return bytesRead;
            }

            void Write(void* buffer,  AZ::u64 size)
            {
                AZ_Assert(!RemainingBytes(),"RemoteFileIO::WriteToCacheLine: Failed to write to a cache that it not empty!");

                OffsetFilePosition(size);
                m_cacheLookaheadBuffer.resize(size);
                memcpy(m_cacheLookaheadBuffer.data(), buffer, size);
                m_cacheLookaheadPos = 0;
            }

            inline void Invalidate()
            {
                m_cacheLookaheadPos = 0;
                m_cacheLookaheadBuffer.clear();
            }

            inline AZ::u64 RemainingBytes()
            {
                return m_cacheLookaheadBuffer.size() - m_cacheLookaheadPos;
            }

            inline AZ::u64 CacheFilePosition()
            {
                return m_filePosition - RemainingBytes();
            }

            inline AZ::u64 CacheStartFilePosition()
            {
                return m_filePosition - m_cacheLookaheadBuffer.size();
            }

            inline AZ::u64 CacheEndFilePosition()
            {
                return m_filePosition;
            }

            inline bool IsFilePositionInCache(AZ::u64 filePosition)
            {
                return filePosition >= CacheStartFilePosition() && filePosition < CacheEndFilePosition();
            }

            inline void SetCachePositionFromFilePosition(AZ::u64 filePosition)
            {
                m_cacheLookaheadPos = filePosition - CacheStartFilePosition();
            }

            inline void SetFilePosition(AZ::u64 filePosition)
            {
                m_filePosition = filePosition;
            }

            inline void OffsetFilePosition(AZ::s64 offset)
            {
                m_filePosition += offset;
            }
        };
#endif

        class RemoteFileIO
            : public FileIOBase
        {
        public:
            AZ_RTTI(RemoteFileIO, "{A863335E-9330-44E2-AD89-B5309F3B8B93}");
            AZ_CLASS_ALLOCATOR(RemoteFileIO, OSAllocator, 0);

            RemoteFileIO();
            ~RemoteFileIO();

            ////////////////////////////////////////////////////////////////////////////////////////
            //implementation of FileIOBase

            Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) override;
            Result Close(HandleType fileHandle) override;
            Result Tell(HandleType fileHandle, AZ::u64& offset) override;
            Result Seek(HandleType fileHandle, AZ::s64 offset, SeekType type) override;
            Result Size(HandleType fileHandle, AZ::u64& size) override;
            Result Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false, AZ::u64* bytesRead = nullptr) override;
            Result Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) override;
            Result Flush(HandleType fileHandle) override;
            bool Eof(HandleType fileHandle) override;
            AZ::u64 ModificationTime(HandleType fileHandle) override;
            bool Exists(const char* filePath) override;
            Result Size(const char* filePath, AZ::u64& size) override;
            AZ::u64 ModificationTime(const char* filePath) override;
            bool IsDirectory(const char* filePath) override;
            bool IsReadOnly(const char* filePath) override;
            Result CreatePath(const char* filePath) override;
            Result DestroyPath(const char* filePath) override;
            Result Remove(const char* filePath) override;
            Result Copy(const char* sourceFilePath, const char* destinationFilePath) override;
            Result Rename(const char* sourceFilePath, const char* destinationFilePath) override;
            Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) override;
            void SetAlias(const char* alias, const char* path) override;
            void ClearAlias(const char* alias) override;
            AZ::u64 ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const override;
            const char* GetAlias(const char* alias) override;
            bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) override;
            bool GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const override;
            ////////////////////////////////////////////////////////////////////////////////////////////

        private:
            mutable AZStd::recursive_mutex m_remoteFilesGuard;
            AZStd::unordered_map<HandleType, AZ::OSString, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_remoteFiles;
#ifdef VFS_CACHING
            AZStd::unordered_map<HandleType, VFSCacheLine, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_remoteFileCache;

            // (assumes you're inside a remote files guard lock already for creation, which is true since we create one on open)
            inline VFSCacheLine& GetCacheLine(HandleType handle)
            {
                auto found = m_remoteFileCache.find(handle);
                // check this because it is a serious error since it may be that you're in an unguarded access to a non-existant handle.
                AZ_Assert(found != m_remoteFileCache.end(), "RemoteFileIO::GetCacheLine() Attempt to Get cache line missed, did something go wrong with open?");
                return found->second;
            }
#endif
        };
    } // namespace IO
} // namespace AZ
