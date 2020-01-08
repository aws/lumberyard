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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/Debug/Profiler.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/functional.h>

#include <stdio.h>

using namespace AZ::IO; 

namespace Platform
{
    // Forward declaration of platform specific implementations

    using FileHandleType = SystemFile::FileHandleType;

    void Seek(FileHandleType handle, const SystemFile* systemFile, SizeType offset, SystemFile::SeekMode mode);
    SystemFile::SizeType Tell(FileHandleType handle, const SystemFile* systemFile);
    bool Eof(FileHandleType handle, const SystemFile* systemFile);
    AZ::u64 ModificationTime(FileHandleType handle, const SystemFile* systemFile);
    SystemFile::SizeType Read(FileHandleType handle, const SystemFile* systemFile, SizeType byteSize, void* buffer);
    SystemFile::SizeType Write(FileHandleType handle, const SystemFile* systemFile, const void* buffer, SizeType byteSize);
    void Flush(FileHandleType handle, const SystemFile* systemFile );
    SystemFile::SizeType Length(FileHandleType handle, const SystemFile* systemFile);

    bool Exists(const char* fileName);
    void FindFiles(const char* filter, SystemFile::FindFileCB cb);
    AZ::u64 ModificationTime(const char* fileName);
    SystemFile::SizeType Length(const char* fileName);
    bool Delete(const char* fileName);
    bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite);
    bool IsWritable(const char* sourceFileName);
    bool SetWritable(const char* sourceFileName, bool writable);
    bool CreateDir(const char* dirName);
    bool DeleteDir(const char* dirName);
}

void SystemFile::CreatePath(const char* fileName)
{
    char folderPath[AZ_MAX_PATH_LEN] = { 0 };
    const char* delimiter1 = strrchr(fileName, '\\');
    const char* delimiter2 = strrchr(fileName, '/');
    const char* delimiter = delimiter2 > delimiter1 ? delimiter2 : delimiter1;
    if (delimiter > fileName)
    {
        azstrncpy(folderPath, AZ_ARRAY_SIZE(folderPath), fileName, delimiter - fileName);
        if (!Exists(folderPath))
        {
            CreateDir(folderPath);
        }
    }
}

SystemFile::SystemFile()
{
    m_fileName[0] = '\0';
    m_handle = AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE;
}

SystemFile::~SystemFile()
{
    if (IsOpen())
    {
        Close();
    }
}

bool SystemFile::Open(const char* fileName, int mode, int platformFlags)
{
    AZ_PROFILE_INTERVAL_SCOPED(AZ::Debug::ProfileCategory::AzCore, this, "SystemFile::Open - %s", fileName);
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Open - %s", fileName);

    if (fileName)       // If we reopen the file we are allowed to have NULL file name
    {
        if (strlen(fileName) > AZ_ARRAY_SIZE(m_fileName) - 1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, 0);
            return false;
        }

        // store the filename
        azsnprintf(m_fileName, AZ_ARRAY_SIZE(m_fileName), "%s", fileName);
    }

    if (FileIOBus::HasHandlers())
    {
        bool isOpen = false;
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnOpen, *this, m_fileName, mode, platformFlags, isOpen);
        if (isHandled)
        {
            return isOpen;
        }
    }

    AZ_Assert(!IsOpen(), "This file (%s) is already open!", m_fileName);

    return PlatformOpen(mode, platformFlags);
}

bool SystemFile::ReOpen(int mode, int platformFlags)
{
    AZ_Assert(strlen(m_fileName) > 0, "Missing filename. You must call open first!");
    return Open(0, mode, platformFlags);
}

void SystemFile::Close()
{
    AZ_PROFILE_INTERVAL_SCOPED(AZ::Debug::ProfileCategory::AzCore, this, "SystemFile::Close - %s", m_fileName);
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Close - %s", m_fileName);

    if (FileIOBus::HasHandlers())
    {
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnClose, *this);
        if (isHandled)
        {
            return;
        }
    }

    PlatformClose();
}

void SystemFile::Seek(SizeType offset, SeekMode mode)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Seek - %s:%i", m_fileName, offset);

    if (FileIOBus::HasHandlers())
    {
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnSeek, *this, offset, mode);
        if (isHandled)
        {
            return;
        }
    }

    Platform::Seek(m_handle, this, offset, mode);
}

SystemFile::SizeType SystemFile::Tell()
{
    return Platform::Tell(m_handle, this);
}

bool SystemFile::Eof()
{
    return Platform::Eof(m_handle, this);
}

AZ::u64 SystemFile::ModificationTime()
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::ModTime - %s", m_fileName);

    return Platform::ModificationTime(m_handle, this);
}

SystemFile::SizeType SystemFile::Read(SizeType byteSize, void* buffer)
{
    AZ_PROFILE_INTERVAL_SCOPED(AZ::Debug::ProfileCategory::AzCore, this, "SystemFile::Read - %s:%i", m_fileName, byteSize);
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Read - %s:%i", m_fileName, byteSize);

    if (FileIOBus::HasHandlers())
    {
        SizeType numRead = 0;
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnRead, *this, byteSize, buffer, numRead);
        if (isHandled)
        {
            return numRead;
        }
    }

    return Platform::Read(m_handle, this, byteSize, buffer);
}

SystemFile::SizeType SystemFile::Write(const void* buffer, SizeType byteSize)
{
    AZ_PROFILE_INTERVAL_SCOPED(AZ::Debug::ProfileCategory::AzCore, this, "SystemFile::Write - %s:%i", m_fileName, byteSize);
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Write - %s:%i", m_fileName, byteSize);

    if (FileIOBus::HasHandlers())
    {
        SizeType numWritten = 0;
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnWrite, *this, buffer, byteSize, numWritten);
        if (isHandled)
        {
            return numWritten;
        }
    }

    return Platform::Write(m_handle, this, buffer, byteSize);
}

void SystemFile::Flush()
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Flush - %s", m_fileName);

    Platform::Flush(m_handle, this);
}

SystemFile::SizeType SystemFile::Length() const
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Length - %s", m_fileName);

    return Platform::Length(m_handle, this);
}

bool SystemFile::IsOpen() const
{
    return m_handle != AZ_TRAIT_SYSTEMFILE_INVALID_HANDLE;
}

SystemFile::SizeType SystemFile::DiskOffset() const
{
#if AZ_TRAIT_DOES_NOT_SUPPORT_FILE_DISK_OFFSET
    #pragma message("--- File Disk Offset is not available ---")
#endif
    return 0;
}

bool SystemFile::Exists(const char* fileName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Exists(util) - %s", fileName);

    return Platform::Exists(fileName);
}

void SystemFile::FindFiles(const char* filter, FindFileCB cb)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::FindFiles(util) - %s", filter);

    Platform::FindFiles(filter, cb);
}

AZ::u64 SystemFile::ModificationTime(const char* fileName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::ModTime(util) - %s", fileName);

    return Platform::ModificationTime(fileName);
}

SystemFile::SizeType SystemFile::Length(const char* fileName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Length(util) - %s", fileName);

    return Platform::Length(fileName);
}

SystemFile::SizeType SystemFile::Read(const char* fileName, void* buffer, SizeType byteSize, SizeType byteOffset)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Read(util) - %s:[%i,%i]", fileName, byteOffset, byteSize);

    SizeType numBytesRead = 0;
    SystemFile f;
    if (f.Open(fileName, SF_OPEN_READ_ONLY))
    {
        if (byteSize == 0)
        {
            byteSize = f.Length(); // read the entire file
        }
        if (byteOffset != 0)
        {
            f.Seek(byteOffset, SF_SEEK_BEGIN);
        }

        numBytesRead = f.Read(byteSize, buffer);
        f.Close();
    }

    return numBytesRead;
}

bool SystemFile::Delete(const char* fileName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Delete(util) - %s", fileName);

    if (!Exists(fileName))
    {
        return false;
    }

    return Platform::Delete(fileName);
}

bool SystemFile::Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::Rename(util) - %s", sourceFileName);

    if (!Exists(sourceFileName))
    {
        return false;
    }

    return Platform::Rename(sourceFileName, targetFileName, overwrite);
}

bool SystemFile::IsWritable(const char* sourceFileName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::IsWritable(util) - %s", sourceFileName);

    return Platform::IsWritable(sourceFileName);
}

bool SystemFile::SetWritable(const char* sourceFileName, bool writable)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::SetWritable(util) - %s", sourceFileName);

    return Platform::SetWritable(sourceFileName, writable);
}

bool SystemFile::CreateDir(const char* dirName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::CreateDir(util) - %s", dirName);

    return Platform::CreateDir(dirName);
}

bool SystemFile::DeleteDir(const char* dirName)
{
    AZ_PROFILE_SCOPE_STALL_DYNAMIC(AZ::Debug::ProfileCategory::AzCore, "SystemFile::DeleteDir(util) - %s", dirName);

    return Platform::DeleteDir(dirName);
}
