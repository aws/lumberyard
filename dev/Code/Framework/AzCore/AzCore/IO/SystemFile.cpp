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
#ifndef AZ_UNITY_BUILD

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/functional.h>

#include <stdio.h>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <errno.h>
    #include <dirent.h>
#endif

#if defined(AZ_PLATFORM_ANDROID)
    #include <AzCore/Android/APKFileHandler.h>
    #include <AzCore/Android/Utils.h>
#endif 

using namespace AZ::IO;

namespace
{
    //=========================================================================
    // GetAttributes
    // [1/16/2013]
    //  Internal utility to avoid code duplication. Returns result of win32
    //  GetFileAttributes
    //=========================================================================
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    DWORD
    GetAttributes(const char* fileName)
    {
#   ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            return GetFileAttributesW(fileNameW);
        }
        else
        {
            return INVALID_FILE_ATTRIBUTES;
        }
#   else //!_UNICODE
        return GetFileAttributes(fileName);
#   endif // !_UNICODE
    }
#endif

    //=========================================================================
    // SetAttributes
    // [1/20/2013]
    //  Internal utility to avoid code duplication. Returns result of win32
    //  SetFileAttributes
    //=========================================================================
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    BOOL
    SetAttributes(const char* fileName, DWORD fileAttributes)
    {
#   ifdef _UNICODE
        wchar_t fileNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
        {
            return SetFileAttributesW(fileNameW, fileAttributes);
        }
        else
        {
            return FALSE;
        }
#   else //!_UNICODE
        return SetFileAttributes(fileName, fileAttributes);
#   endif // !_UNICODE
    }
#endif

    //=========================================================================
    // CreateDirRecursive
    // [2/3/2013]
    //  Internal utility to create a folder hierarchy recursively without
    //  any additional string copies.
    //  If this function fails (returns false), the error will be available via:
    //   * GetLastError() on Windows-like platforms
    //   * errno on Unix platforms
    //=========================================================================
#if AZ_TRAIT_USE_WINDOWS_FILE_API
#   if defined(_UNICODE)
    bool
    CreateDirRecursive(wchar_t* dirPath)
    {
        if (CreateDirectoryW(dirPath, nullptr))
        {
            return true;    // Created without error
        }
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
        {
            // try to create our parent hierarchy
            for (size_t i = wcslen(dirPath); i > 0; --i)
            {
                if (dirPath[i] == L'/' || dirPath[i] == L'\\')
                {
                    wchar_t delimiter = dirPath[i];
                    dirPath[i] = 0; // null-terminate at the previous slash
                    bool ret = CreateDirRecursive(dirPath);
                    dirPath[i] = delimiter; // restore slash
                    if (ret)
                    {
                        // now that our parent is created, try to create again
                        return CreateDirectoryW(dirPath, nullptr) != 0;
                    }
                    return false;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (error == ERROR_ALREADY_EXISTS)
        {
            DWORD attributes = GetFileAttributesW(dirPath);
            return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        return false;
    }
#   else
    bool
    CreateDirRecursive(char* dirPath)
    {
        if (CreateDirectory(dirPath, nullptr))
        {
            return true;    // Created without error
        }
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
        {
            // try to create our parent hierarchy
            for (size_t i = strlen(dirPath); i > 0; --i)
            {
                if (dirPath[i] == '/' || dirPath[i] == '\\')
                {
                    char delimiter = dirPath[i];
                    dirPath[i] = 0; // null-terminate at the previous slash
                    bool ret = CreateDirRecursive(dirPath);
                    dirPath[i] = delimiter; // restore slash
                    if (ret)
                    {
                        // now that our parent is created, try to create again
                        return CreateDirectory(dirPath, nullptr) != 0;
                    }
                    return false;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (error == ERROR_ALREADY_EXISTS)
        {
            DWORD attributes = GetFileAttributes(dirPath);
            return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        return false;
    }
#   endif
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    bool CreateDirRecursive(char* dirPath)
    {
    #if defined(AZ_PLATFORM_ANDROID)
        if (AZ::Android::Utils::IsApkPath(dirPath))
        {
            return false;
        }
    #endif // defined(AZ_PLATFORM_ANDROID)

        int result = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (result == 0)
        {
            return true;    // Created without error
        }
        else if (result == -1)
        {
            // If result == -1, the error is stored in errno
            // http://pubs.opengroup.org/onlinepubs/007908799/xsh/mkdir.html
            result = errno;
        }

        if (result == ENOTDIR || result == ENOENT)
        {
            // try to create our parent hierarchy
            for (size_t i = strlen(dirPath); i > 0; --i)
            {
                if (dirPath[i] == '/' || dirPath[i] == '\\')
                {
                    char delimiter = dirPath[i];
                    dirPath[i] = 0; // null-terminate at the previous slash
                    bool ret = CreateDirRecursive(dirPath);
                    dirPath[i] = delimiter; // restore slash
                    if (ret)
                    {
                        // now that our parent is created, try to create again
                        return mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
                    }
                    return false;
                }
            }
            // if we reach here then there was no parent folder to create, so we failed for other reasons
        }
        else if (result == EEXIST)
        {
            struct stat s;
            if (stat(dirPath, &s) == 0)
            {
                return s.st_mode & S_IFDIR;
            }
        }
        return false;
    }
#endif

#if AZ_TRAIT_USE_WINDOWS_FILE_API
    static const SystemFile::FileHandleType PlatformSpecificInvalidHandle = INVALID_HANDLE_VALUE;
#elif AZ_TRAIT_USE_SYSTEMFILE_HANDLE
    static const SystemFile::FileHandleType PlatformSpecificInvalidHandle = (SystemFile::FileHandleType) -1;
#elif defined(AZ_PLATFORM_ANDROID)
    static const SystemFile::FileHandleType PlatformSpecificInvalidHandle = nullptr;
#endif
}

//=========================================================================
// SystemFile
// [9/30/2009]
//=========================================================================
SystemFile::SystemFile()
{
    m_fileName[0] = '\0';
    m_handle = PlatformSpecificInvalidHandle;
}

//=========================================================================
// ~SystemFile
// [9/30/2009]
//=========================================================================
SystemFile::~SystemFile()
{
    if (IsOpen())
    {
        Close();
    }
}

//=========================================================================
// Open
// [9/30/2009]
//=========================================================================
bool
SystemFile::Open(const char* fileName, int mode, int platformFlags)
{
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

#if AZ_TRAIT_USE_WINDOWS_FILE_API
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = FILE_SHARE_READ;
    DWORD dwFlagsAndAttributes = platformFlags;
    DWORD dwCreationDisposition = OPEN_EXISTING;

    bool createPath = false;
    if (mode & SF_OPEN_READ_ONLY)
    {
        dwDesiredAccess |= GENERIC_READ;
        // Always open files for reading with file shared write flag otherwise it may result in a sharing violation if 
        // either some process has already opened the file for writing or if some process will later on try to open the file for writing. 
        dwShareMode |= FILE_SHARE_WRITE;
    }
    if (mode & SF_OPEN_READ_WRITE)
    {
        dwDesiredAccess |= GENERIC_READ;
    }
    if ((mode & SF_OPEN_WRITE_ONLY) || (mode & SF_OPEN_READ_WRITE) || (mode & SF_OPEN_APPEND))
    {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    if ((mode & SF_OPEN_CREATE_NEW))
    {
        dwCreationDisposition = CREATE_NEW;
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }
    else if ((mode & SF_OPEN_CREATE))
    {
        dwCreationDisposition = CREATE_ALWAYS;
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }
    else if ((mode & SF_OPEN_TRUNCATE))
    {
        dwCreationDisposition = TRUNCATE_EXISTING;
    }

    if (createPath)
    {
        char folderPath[AZ_MAX_PATH_LEN] = { 0 };
        const char* delimiter1 = strrchr(m_fileName, '\\');
        const char* delimiter2 = strrchr(m_fileName, '/');
        const char* delimiter = delimiter2 > delimiter1 ? delimiter2 : delimiter1;
        if (delimiter > m_fileName)
        {
            azstrncpy(folderPath, m_fileName, delimiter - m_fileName);
            if (!Exists(folderPath))
            {
                CreateDir(folderPath);
            }
        }
    }

#   ifdef _UNICODE
    wchar_t fileNameW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    m_handle = INVALID_HANDLE_VALUE;
    if (mbstowcs_s(&numCharsConverted, fileNameW, m_fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
    {
        m_handle = CreateFileW(fileNameW, dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, dwFlagsAndAttributes, 0);
    }
#   else //!_UNICODE
    m_handle = CreateFile(m_fileName, dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, dwFlagsAndAttributes, 0);
#   endif // !_UNICODE

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
        return false;
    }
    else
    {
        if (mode & SF_OPEN_APPEND)
        {
            SetFilePointer(m_handle, 0, NULL, FILE_END);
        }
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    int desiredAccess = 0;
    int permissions = S_IRWXU | S_IRGRP | S_IROTH;

    bool createPath = false;
    if ((mode & SF_OPEN_READ_WRITE) == SF_OPEN_READ_WRITE)
    {
        desiredAccess = O_RDWR;
    }
    else if ((mode & SF_OPEN_READ_ONLY) == SF_OPEN_READ_ONLY)
    {
        desiredAccess = O_RDONLY;
    }
    else if ((mode & SF_OPEN_WRITE_ONLY) == SF_OPEN_WRITE_ONLY || (mode & SF_OPEN_APPEND))
    {
        desiredAccess = O_WRONLY;
    }
    else
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, 0);
        return false;
    }

    if ((mode & SF_OPEN_CREATE_NEW) == SF_OPEN_CREATE_NEW)
    {
        desiredAccess |= O_CREAT | O_EXCL;
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }
    else if ((mode & SF_OPEN_CREATE) ==  SF_OPEN_CREATE)
    {
        desiredAccess |= O_CREAT | O_TRUNC;
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }
    else if ((mode & SF_OPEN_TRUNCATE))
    {
        desiredAccess |= O_TRUNC;
    }

    if (createPath)
    {
        char folderPath[AZ_MAX_PATH_LEN] = {0};
        const char* delimiter1 = strrchr(m_fileName, '\\');
        const char* delimiter2 = strrchr(m_fileName, '/');
        const char* delimiter = delimiter2 > delimiter1 ? delimiter2 : delimiter1;
        if (delimiter > m_fileName)
        {
            azstrncpy(folderPath, AZ_ARRAY_SIZE(folderPath), m_fileName, delimiter - m_fileName);
            if (!Exists(folderPath))
            {
                CreateDir(folderPath);
            }
        }
    }
    m_handle = open(m_fileName, desiredAccess, permissions);

    if (m_handle == PlatformSpecificInvalidHandle)
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
        return false;
    }
    else
    {
        if (mode & SF_OPEN_APPEND)
        {
            lseek(m_handle, 0, SEEK_END);
        }
    }
#elif defined(AZ_PLATFORM_ANDROID)
    const char* openMode = nullptr;
    if ((mode & SF_OPEN_READ_ONLY) == SF_OPEN_READ_ONLY)
    {
        openMode = "r";
    }
    else if ((mode & SF_OPEN_WRITE_ONLY) == SF_OPEN_WRITE_ONLY)
    {
        if ((mode & SF_OPEN_APPEND) == SF_OPEN_APPEND)
        {
            openMode = "a+";
        }
        else if (mode & (SF_OPEN_TRUNCATE | SF_OPEN_CREATE_NEW | SF_OPEN_CREATE))
        {
            openMode = "w+";
        }
        else
        {
            openMode = "w";
        }
    }
    else if ((mode & SF_OPEN_READ_WRITE) == SF_OPEN_READ_WRITE)
    {
        if ((mode & SF_OPEN_APPEND) == SF_OPEN_APPEND)
        {
            openMode = "a+";
        }
        else if (mode & (SF_OPEN_TRUNCATE | SF_OPEN_CREATE_NEW | SF_OPEN_CREATE))
        {
            openMode = "w+";
        }
        else
        {
            openMode = "r+";
        }
    }
    else
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, EINVAL);
        return false;
    }

    bool createPath = false;
    if (mode & (SF_OPEN_CREATE_NEW | SF_OPEN_CREATE))
    {
        createPath = (mode & SF_OPEN_CREATE_PATH) == SF_OPEN_CREATE_PATH;
    }

    bool isApkFile = AZ::Android::Utils::IsApkPath(m_fileName);

    if (createPath)
    {
        if (isApkFile)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, ENOSPC);
            return false;
        }

        char folderPath[AZ_MAX_PATH_LEN] = {0};
        const char* delimiter1 = strrchr(m_fileName, '\\');
        const char* delimiter2 = strrchr(m_fileName, '/');
        const char* delimiter = delimiter2 > delimiter1 ? delimiter2 : delimiter1;
        if (delimiter > m_fileName)
        {
            azstrncpy(folderPath, AZ_ARRAY_SIZE(folderPath), m_fileName, delimiter - m_fileName);
            if (!Exists(folderPath))
            {
                CreateDir(folderPath);
            }
        }
    }

    int errorCode = 0;
    if (isApkFile)
    {
        AZ::u64 size = 0;
        m_handle = AZ::Android::APKFileHandler::Open(m_fileName, openMode, size);
        errorCode = EACCES; // general error when a file can't be opened from inside the APK
    }
    else
    {
        m_handle = fopen(m_fileName, openMode);
        errorCode = errno;
    }

    if (m_handle == PlatformSpecificInvalidHandle)
    {
        EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errorCode);
        return false;
    }
#else
    return false;
#endif

    return true;
}

//=========================================================================
// Reopen
// [9/30/2009]
//=========================================================================
bool
SystemFile::ReOpen(int mode, int platformFlags)
{
    AZ_Assert(strlen(m_fileName) > 0, "Missing filename. You must call open first!");
    return Open(0, mode, platformFlags);
}

//=========================================================================
// Close
// [9/30/2009]
//=========================================================================
void
SystemFile::Close()
{
    if (FileIOBus::HasHandlers())
    {
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnClose, *this);
        if (isHandled)
        {
            return;
        }
    }

#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        if (!CloseHandle(m_handle))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
        }
        m_handle = INVALID_HANDLE_VALUE;
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        close(m_handle);
        m_handle = PlatformSpecificInvalidHandle;
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        fclose(m_handle);
        m_handle = PlatformSpecificInvalidHandle;
    }
#endif
}

//=========================================================================
// Seek
// [9/30/2009]
//=========================================================================
void
SystemFile::Seek(SizeType offset, SeekMode mode)
{
    if (FileIOBus::HasHandlers())
    {
        bool isHandled = false;
        EBUS_EVENT_RESULT(isHandled, FileIOBus, OnSeek, *this, offset, mode);
        if (isHandled)
        {
            return;
        }
    }

#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        DWORD dwMoveMethod = mode;
        LARGE_INTEGER distToMove;
        distToMove.QuadPart = offset;

        if (!SetFilePointerEx(m_handle, distToMove, 0, dwMoveMethod))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
        }
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        int result = lseek(m_handle, offset, mode);
        if (result == -1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
        }
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        off_t result = fseeko(m_handle, static_cast<off_t>(offset), mode);
        if (result != 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
        }
    }
#endif
}

//=========================================================================
// Tell
//=========================================================================
SystemFile::SizeType SystemFile::Tell()
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        LARGE_INTEGER distToMove;
        distToMove.QuadPart = 0;

        LARGE_INTEGER newFilePtr;
        if (!SetFilePointerEx(m_handle, distToMove, &newFilePtr, FILE_CURRENT))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return 0;
        }

        return aznumeric_cast<SizeType>(newFilePtr.QuadPart);
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        off_t result = lseek(m_handle, 0, SEEK_CUR);
        if (result == (off_t) -1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
        }
        return aznumeric_cast<SizeType>(result);
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        off_t result = ftello(m_handle);
        if (result != -1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
            return 0;
        }
        return aznumeric_cast<SizeType>(result);
    }
#endif

    return 0;
}

//=========================================================================
// Eof
//=========================================================================
bool SystemFile::Eof()
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        LARGE_INTEGER zero;
        zero.QuadPart = 0;

        LARGE_INTEGER currentFilePtr;
        if (!SetFilePointerEx(m_handle, zero, &currentFilePtr, FILE_CURRENT))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return false;
        }

        FILE_STANDARD_INFO fileInfo;
        if (!GetFileInformationByHandleEx(m_handle, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return false;
        }

        return currentFilePtr.QuadPart == fileInfo.EndOfFile.QuadPart;
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        off_t current = lseek(m_handle, 0, SEEK_CUR);
        if (current == (off_t)-1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, current);
            return false;
        }

        off_t end = lseek(m_handle, 0, SEEK_END);
        if (end == (off_t)-1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, end);
            return false;
        }

        // Reset file pointer back to from whence it came
        lseek(m_handle, current, SEEK_SET);

        return current == end;
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        return (feof(m_handle) != 0);
    }
#endif

    return false;
}

//=========================================================================
// ModificationTime
//=========================================================================
AZ::u64 SystemFile::ModificationTime()
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        FILE_BASIC_INFO fileInfo;
        if (!GetFileInformationByHandleEx(m_handle, FileBasicInfo, &fileInfo, sizeof(fileInfo)))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return 0;
        }

        return aznumeric_cast<AZ::u64>(fileInfo.ChangeTime.QuadPart);
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        struct stat statResult;
        if (fstat(m_handle, &statResult) != 0)
        {
            return 0;
        }
        return aznumeric_cast<AZ::u64>(statResult.st_mtime);
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        struct stat statResult;
        if (stat(m_fileName, &statResult) != 0)
        {
            return 0;
        }
        return aznumeric_cast<AZ::u64>(statResult.st_mtime);
    }
#endif

    return 0;
}

//=========================================================================
// Read
// [9/30/2009]
//=========================================================================
SystemFile::SizeType
SystemFile::Read(SizeType byteSize, void* buffer)
{
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

#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        DWORD dwNumBytesRead = 0;
        DWORD nNumberOfBytesToRead = (DWORD)byteSize;
        if (!ReadFile(m_handle, buffer, nNumberOfBytesToRead, &dwNumBytesRead, 0))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return 0;
        }
        return static_cast<SizeType>(dwNumBytesRead);
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        ssize_t bytesRead = read(m_handle, buffer, byteSize);
        if (bytesRead == -1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
            return 0;
        }
        return bytesRead;
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        size_t bytesToRead = static_cast<size_t>(byteSize);
        AZ::Android::APKFileHandler::SetNumBytesToRead(bytesToRead);
        size_t bytesRead = fread(buffer, 1, bytesToRead, m_handle);

        if (bytesRead != bytesToRead && ferror(m_handle))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
            return 0;
        }

        return bytesRead;
    }
#endif

    return 0;
}

//=========================================================================
// Write
// [9/30/2009]
//=========================================================================
SystemFile::SizeType
SystemFile::Write(const void* buffer, SizeType byteSize)
{
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

#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        DWORD dwNumBytesWritten = 0;
        DWORD nNumberOfBytesToWrite = (DWORD)byteSize;
        if (!WriteFile(m_handle, buffer, nNumberOfBytesToWrite, &dwNumBytesWritten, 0))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return 0;
        }
        return static_cast<SizeType>(dwNumBytesWritten);
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        ssize_t result = write(m_handle, buffer, byteSize);
        if (result == -1)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
            return 0;
        }
        return result;
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        size_t bytesToWrite = static_cast<size_t>(byteSize);
        size_t bytesWritten = fwrite(buffer, 1, bytesToWrite, m_handle);

        if (bytesWritten != bytesToWrite && ferror(m_handle))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
            return 0;
        }

        return bytesWritten;
    }
#endif

    return 0;
}

//=========================================================================
// Flush
//=========================================================================
void SystemFile::Flush()
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        if (!FlushFileBuffers(m_handle))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
        }
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        if (fsync(m_handle) != 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
        }
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        if (fflush(m_handle) != 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, errno);
        }
    }
#endif
}

//=========================================================================
// Length
// [9/30/2009]
//=========================================================================
SystemFile::SizeType
SystemFile::Length() const
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        LARGE_INTEGER size;
        if (!GetFileSizeEx(m_handle, &size))
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, (int)GetLastError());
            return 0;
        }

        return static_cast<SizeType>(size.QuadPart);
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        struct stat stat;
        if (fstat(m_handle, &stat) < 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, 0);
            return 0;
        }
        return stat.st_size;
    }
#elif defined(AZ_PLATFORM_ANDROID)
    if (m_handle != PlatformSpecificInvalidHandle)
    {
        if (AZ::Android::Utils::IsApkPath(m_fileName))
        {
            int fileSize = AZ::Android::APKFileHandler::FileLength(m_fileName);
            return static_cast<SizeType>(fileSize);
        }
        else
        {
            struct stat fileStat;
            if (stat(m_fileName, &fileStat) < 0)
            {
                EBUS_EVENT(FileIOEventBus, OnError, this, nullptr, 0);
                return 0;
            }
            return static_cast<SizeType>(fileStat.st_size);
        }
    }
#endif

    return 0;
}

//=========================================================================
// IsOpen
// [9/30/2009]
//=========================================================================
bool
SystemFile::IsOpen() const
{
    return m_handle != PlatformSpecificInvalidHandle;
}

//=========================================================================
// DiskOffset
// [9/30/2009]
//=========================================================================
SystemFile::SizeType
SystemFile::DiskOffset() const
{
#if AZ_TRAIT_DOES_NOT_SUPPORT_FILE_DISK_OFFSET
    #pragma message("--- File Disk Offset is not available ---")
#endif
    return 0;
}

//=========================================================================
// Exists
// [9/30/2009]
//=========================================================================
bool
SystemFile::Exists(const char* fileName)
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    return GetAttributes(fileName) != INVALID_FILE_ATTRIBUTES;
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_APPLE)
    return access(fileName, F_OK) == 0;
#elif defined(AZ_PLATFORM_ANDROID)
    if (AZ::Android::Utils::IsApkPath(fileName))
    {
        return AZ::Android::APKFileHandler::DirectoryOrFileExists(fileName);
    }
    else
    {
        return access(fileName, F_OK) == 0;
    }
#else
    return false;
#endif
}



//=========================================================================
// FindFiles
// [9/30/2009]
//=========================================================================
void
SystemFile::FindFiles(const char* filter, FindFileCB cb)
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    int lastError;

#   ifdef _UNICODE
    wchar_t filterW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    hFile = INVALID_HANDLE_VALUE;
    if (mbstowcs_s(&numCharsConverted, filterW, filter, AZ_ARRAY_SIZE(filterW) - 1) == 0)
    {
        hFile = FindFirstFile(filterW, &fd);
    }
#   else // !_UNICODE
    hFile = FindFirstFile(filter, &fd);
#   endif // !_UNICODE

    if (hFile != INVALID_HANDLE_VALUE)
    {
        const char* fileName;

#   ifdef _UNICODE
        char fileNameA[AZ_MAX_PATH_LEN];
        fileName = NULL;
        if (wcstombs_s(&numCharsConverted, fileNameA, fd.cFileName, AZ_ARRAY_SIZE(fileNameA) - 1) == 0)
        {
            fileName = fileNameA;
        }
#   else // !_UNICODE
        fileName = fd.cFileName;
#   endif // !_UNICODE

        cb(fileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);

        // List all the other files in the directory.
        while (FindNextFile(hFile, &fd) != 0)
        {
#   ifdef _UNICODE
            fileName = NULL;
            if (wcstombs_s(&numCharsConverted, fileNameA, fd.cFileName, AZ_ARRAY_SIZE(fileNameA) - 1) == 0)
            {
                fileName = fileNameA;
            }
#   else // !_UNICODE
            fileName = fd.cFileName;
#   endif // !_UNICODE

            cb(fileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
        }

        lastError = (int)GetLastError();
        FindClose(hFile);
        if (lastError != ERROR_NO_MORE_FILES)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, lastError);
        }
    }
    else
    {
        lastError = (int)GetLastError();
        if (lastError != ERROR_FILE_NOT_FOUND)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, lastError);
        }
    }
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    //TODO: Linux implementation does not correctly handle filters
    DIR* dir = opendir(filter);

    if (dir != NULL)
    {
        struct dirent entry;
        struct dirent* result = NULL;

        int lastError = readdir_r(dir, &entry, &result);
        // List all the other files in the directory.
        while (result != NULL && lastError == 0)
        {
            cb(entry.d_name, (entry.d_type & DT_DIR) == 0);
            lastError = readdir_r(dir, &entry, &result);
        }

        if (lastError != 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, lastError);
        }

        closedir(dir);
    }
    else
    {
        int lastError = errno;
        if (lastError != ENOENT)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, filter, 0);
        }
    }
#endif
}

//=========================================================================
// ModificationTime
//=========================================================================
AZ::u64 SystemFile::ModificationTime(const char* fileName)
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    HANDLE handle = nullptr;

#   ifdef _UNICODE
    wchar_t fileNameW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
    {
        handle = CreateFileW(fileNameW, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    }
#   else // !_UNICODE
    handle = CreateFileA(fileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
#   endif // !_UNICODE

    if (handle == INVALID_HANDLE_VALUE)
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
        return 0;
    }

    FILE_BASIC_INFO fileInfo;
    fileInfo.ChangeTime.QuadPart = 0;
    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &fileInfo, sizeof(fileInfo)))
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
    }

    CloseHandle(handle);

    return aznumeric_cast<AZ::u64>(fileInfo.ChangeTime.QuadPart);
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    struct stat statResult;
    if (stat(fileName, &statResult) != 0)
    {
        return 0;
    }
    return aznumeric_cast<AZ::u64>(statResult.st_mtime);
#else
    AZ::u64 time = 0;
    SystemFile f;
    if (f.Open(fileName, SF_OPEN_READ_ONLY))
    {
        time = f.ModificationTime();
        f.Close();
    }
    return time;
#endif
}

//=========================================================================
// Length
// [9/30/2009]
//=========================================================================
SystemFile::SizeType
SystemFile::Length(const char* fileName)
{
    SizeType len = 0;
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    WIN32_FILE_ATTRIBUTE_DATA data = { 0 };
    BOOL result = FALSE;

#   ifdef _UNICODE
    wchar_t fileNameW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
    {
        result = GetFileAttributesExW(fileNameW, GetFileExInfoStandard, &data);
    }
#   else // !_UNICODE
    result = GetFileAttributesExA(fileName, GetFileExInfoStandard, &data);
#   endif // !_UNICODE

    if (result)
    {
        // Convert hi/lo parts to a SizeType
        LARGE_INTEGER fileSize;
        fileSize.LowPart = data.nFileSizeLow;
        fileSize.HighPart = data.nFileSizeHigh;
        len = aznumeric_cast<SizeType>(fileSize.QuadPart);
    }
    else
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
    }
#else
    // generic solution
    SystemFile f;
    if (f.Open(fileName, SF_OPEN_READ_ONLY))
    {
        len = f.Length();
        f.Close();
    }
#endif
    return len;
}

//=========================================================================
// Read
// [9/30/2009]
//=========================================================================
SystemFile::SizeType
SystemFile::Read(const char* fileName, void* buffer, SizeType byteSize, SizeType byteOffset)
{
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

//=========================================================================
// Delete
// [11/5/2012]
//=========================================================================
bool
SystemFile::Delete(const char* fileName)
{
    if (!Exists(fileName))
    {
        return false;
    }

#if AZ_TRAIT_USE_WINDOWS_FILE_API

#   ifdef _UNICODE
    wchar_t fileNameW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    if (mbstowcs_s(&numCharsConverted, fileNameW, fileName, AZ_ARRAY_SIZE(fileNameW) - 1) == 0)
    {
        if (DeleteFileW(fileNameW) == 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
            return false;
        }
    }
    else
    {
        return false;
    }
#   else // !_UNICODE
    if (DeleteFile(fileName) == 0)
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, (int)GetLastError());
        return false;
    }
#   endif // !_UNICODE

#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    int result = remove(fileName);
    if (result != 0)
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, fileName, result);
        return false;
    }
#else
    return false;
#endif
    return true;
}

//=========================================================================
// Delete
// [11/5/2012]
//=========================================================================
bool
SystemFile::Rename(const char* sourceFileName, const char* targetFileName, bool overwrite)
{
    if (!Exists(sourceFileName))
    {
        return false;
    }

#if AZ_TRAIT_USE_WINDOWS_FILE_API

#   ifdef _UNICODE
    wchar_t sourceFileNameW[AZ_MAX_PATH_LEN];
    wchar_t targetFileNameW[AZ_MAX_PATH_LEN];
    size_t numCharsConverted;
    if (mbstowcs_s(&numCharsConverted, sourceFileNameW, sourceFileName, AZ_ARRAY_SIZE(sourceFileNameW) - 1) == 0 &&
        mbstowcs_s(&numCharsConverted, targetFileNameW, targetFileName, AZ_ARRAY_SIZE(targetFileNameW) - 1) == 0)
    {
        if (MoveFileExW(sourceFileNameW, targetFileNameW, overwrite ? MOVEFILE_REPLACE_EXISTING : 0) == 0)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, (int)GetLastError());
            return false;
        }
    }
    else
    {
        return false;
    }
#   else // !_UNICODE
    if (MoveFileEx(sourceFileName, targetFileName, overwrite ? MOVEFILE_REPLACE_EXISTING : 0) == 0)
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, (int)GetLastError());
        return false;
    }
#   endif // !_UNICODE

#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    int result = rename(sourceFileName, targetFileName);
    if (result)
    {
        EBUS_EVENT(FileIOEventBus, OnError, nullptr, sourceFileName, result);
        return false;
    }
#else
    return false;
#endif
    return true;
}

//=========================================================================
// IsWritable
// [1/16/2014]
//=========================================================================
bool
SystemFile::IsWritable(const char* sourceFileName)
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    auto fileAttr = GetAttributes(sourceFileName);
    return !((fileAttr == INVALID_FILE_ATTRIBUTES) || (fileAttr & FILE_ATTRIBUTE_DIRECTORY) || (fileAttr & FILE_ATTRIBUTE_READONLY));
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
    return (access (sourceFileName, W_OK) == 0);
#else
    return false;
#endif
}

//=========================================================================
// SetWritable
// [1/20/2014]
//=========================================================================
bool
SystemFile::SetWritable(const char* sourceFileName, bool writable)
{
#if AZ_TRAIT_USE_WINDOWS_FILE_API
    auto fileAttr = GetAttributes(sourceFileName);
    if ((fileAttr == INVALID_FILE_ATTRIBUTES) || (fileAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return false;
    }

    if (writable)
    {
        fileAttr = fileAttr & (~FILE_ATTRIBUTE_READONLY);
    }
    else
    {
        fileAttr = fileAttr | (FILE_ATTRIBUTE_READONLY);
    }
    auto success = SetAttributes(sourceFileName, fileAttr);
    return success != FALSE;
#elif  defined(AZ_PLATFORM_APPLE)
    struct stat s;
    if (stat(sourceFileName, &s) == 0)
    {
        int permissions = (s.st_mode & S_IRWXU) | (s.st_mode & S_IRWXO) | (s.st_mode & S_IRWXG);
        if (writable)
        {
            if (s.st_mode & S_IWUSR)
            {
                return true;
            }
            return chmod(sourceFileName,  permissions | S_IWUSR) == 0;
        }
        else
        {
            if (s.st_mode & S_IRUSR)
            {
                return true;
            }
            return chmod(sourceFileName,  permissions | S_IRUSR) == 0;
        }
    }
    return false;
#elif defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_LINUX)
    AZ_Assert(false, "SetWritable is not supported on Linux");
    struct stat s;
    if (stat(sourceFileName, &s) == 0)
    {
        int permissions = (s.st_mode & S_IRWXU) | (s.st_mode & S_IRWXO) | (s.st_mode & S_IRWXG);
        if (s.st_mode & S_IWUSR)
        {
            return true;
        }
        return chmod(sourceFileName,  permissions | S_IWUSR) == 0;
    }
    return false;
#else
    return false;
#endif
}

//=========================================================================
// CreateDir
// [2/3/2014]
//=========================================================================
bool
SystemFile::CreateDir(const char* dirName)
{
    if (dirName)
    {
#if AZ_TRAIT_USE_WINDOWS_FILE_API
#   if defined(_UNICODE)
        wchar_t dirPath[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, dirPath, dirName, AZ_ARRAY_SIZE(dirPath) - 1) == 0)
        {
            bool success = CreateDirRecursive(dirPath);
            if (!success)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, (int)GetLastError());
            }
            return success;
        }
#   else
        char dirPath[AZ_MAX_PATH_LEN];
        if (azstrcpy(dirPath, AZ_ARRAY_SIZE(dirPath), dirName) == 0)
        {
            bool success = CreateDirRecursive(dirPath);
            if (!success)
            {
                EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, (int)GetLastError());
            }
            return success;
        }
#   endif
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        char dirPath[AZ_MAX_PATH_LEN];
        if (strlen(dirName) > AZ_MAX_PATH_LEN)
        {
            return false;
        }
        azstrcpy(dirPath, AZ_MAX_PATH_LEN, dirName);
        bool success = CreateDirRecursive(dirPath);
        if (!success)
        {
            EBUS_EVENT(FileIOEventBus, OnError, nullptr, dirName, errno);
        }
        return success;
#else
        AZ_Assert(false, "CreateDir not implemented on this platform!");
#endif
    }
    return false;
}
//=========================================================================
// DeleteDir
// [2/3/2014]
//=========================================================================
bool
SystemFile::DeleteDir(const char* dirName)
{
    if (dirName)
    {
#if defined(AZ_PLATFORM_WINDOWS)
#   if defined(_UNICODE)
        wchar_t dirNameW[AZ_MAX_PATH_LEN];
        size_t numCharsConverted;
        if (mbstowcs_s(&numCharsConverted, dirNameW, dirName, AZ_ARRAY_SIZE(dirNameW) - 1) == 0)
        {
            return RemoveDirectory(dirNameW) != 0;
        }
#   else
        return RemoveDirectory(dirName) != 0;
#   endif
#elif defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
        return rmdir(dirName) != 0;
#else
        AZ_Assert(false, "CreateDir not implemented on this platform!");
#endif
    }
    return false;
}

#endif // #ifndef AZ_UNITY_BUILD
