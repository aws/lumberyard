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

//#define REMOTEFILEIO_USE_EXCLUDED_FILEIO
//#define REMOTEFILEIO_USE_FILE_REMAPS

#ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
#include <regex>
#endif

namespace AZ
{
    namespace IO
    {
        const size_t CACHE_LOOKAHEAD_SIZE = 1024 * 64;
        struct VFSCacheLine
        {
            AZ_CLASS_ALLOCATOR(VFSCacheLine, OSAllocator, 0);
            AZStd::vector<char, AZ::OSStdAllocator> m_cacheLookaheadBuffer;
            AZ::u64 m_cacheLookaheadPos = 0;
            AZ::u64 m_fileSizeSnapshot = 0;
            AZ::u64 m_fileSizeSnapshotTime = 0;

            // note that m_position caches the actual physical pointer location of the file - not the cache pos.
            // the actual 'tell position' should be this number minus the number of bytes its ahead by in the cache.
            AZ::u64 m_position = 0;
            
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
                    m_position = other.m_position;
                }
                return *this;
            }

        };

        class RemoteFileIO
            : public FileIOBase
        {
        public:
            AZ_RTTI(RemoteFileIO, "{A863335E-9330-44E2-AD89-B5309F3B8B93}");
            AZ_CLASS_ALLOCATOR(RemoteFileIO, OSAllocator, 0);

            RemoteFileIO(FileIOBase* excludedFileIO = nullptr);
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

#ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            void AddExclusion(const char* exclusion_regex);
            bool IsFileExcluded(const char* filePath);
            bool IsFileRemote(const char* filePath);
            bool IsFileExcluded(HandleType fileHandle);
            bool IsFileRemote(HandleType fileHandle);
#endif

#ifdef REMOTEFILEIO_USE_FILE_REMAPS
            void RemapFile(const char* filePath, const char* remappedToFileName);
            const char* GetFileRemap(const char* filePath, bool returnFileNameIfNotRemapped = true) const;
#endif

        private:
            FileIOBase* m_excludedFileIO;
#ifdef REMOTEFILEIO_USE_EXCLUDED_FILEIO
            AZStd::mutex m_excludedFileIOGuard;
            std::vector<std::regex> m_excludes;
            std::map<HandleType, std::string> m_excludedFiles;
#endif

#ifdef REMOTEFILEIO_USE_FILE_REMAPS
            std::map<std::string, std::string> m_fileRemap;
#endif
            mutable AZStd::recursive_mutex m_remoteFilesGuard;
            AZStd::unordered_map<HandleType, AZ::OSString, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_remoteFiles;
            AZStd::unordered_map<HandleType, VFSCacheLine, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_remoteFileCache;

            VFSCacheLine& GetCacheLine(HandleType handle);
            void InvalidateReadaheadCache(HandleType handle);
            AZ::u64 ReadFromCache(HandleType handle, void* buffer,  AZ::u64 requestedSize);
            Result SeekInternal(HandleType handle, VFSCacheLine& cacheLine, AZ::s64 offset, SeekType type);
        };
    } // namespace IO
} // namespace AZ
