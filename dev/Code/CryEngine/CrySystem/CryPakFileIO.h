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
#ifndef CRYSYSTEM_CRYPAKFILEIO_H
#define CRYSYSTEM_CRYPAKFILEIO_H

#include <AzCore/IO/FileIO.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/osstring.h>

#define CRYPAK_FILEIO_MAX_BUFFERSIZE (16 * 1024)

namespace AZ
{
    namespace IO
    {
        //! CryPakFileIO
        //! An implementation of the FileIOBase which pipes all operations via CryPak,
        //! which itself pipes all operations via Local or RemoteFileIO.
        //! this allows us to talk to files inside packfiles, without having to change the interface.

        class CryPakFileIO
            : public FileIOBase
        {
        public:
            AZ_RTTI(CryPakFileIO, "{679F8DB8-CC61-4BC8-ADDB-170E3D428B5D}", FileIOBase);
            AZ_CLASS_ALLOCATOR(CryPakFileIO, OSAllocator, 0);

            CryPakFileIO();
            ~CryPakFileIO();

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

        protected:
            // we keep a list of file names ever opened so that we can easily return it.
            mutable AZStd::recursive_mutex m_operationGuard;
            AZStd::unordered_map<HandleType, AZ::OSString, AZStd::hash<HandleType>, AZStd::equal_to<HandleType>, AZ::OSStdAllocator> m_trackedFiles;
            AZStd::fixed_vector<char, CRYPAK_FILEIO_MAX_BUFFERSIZE> m_copyBuffer;
        };
    } // namespace IO
} // namespace AZ
#endif // #ifndef CRYSYSTEM_CRYPAKFILEIO_H