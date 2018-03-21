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
#include <AzFramework/IO/LocalFileIO.h>
#include <sys/stat.h>
#include <AzCore/std/functional.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>
#include <cctype>

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(LocalFileIO_cpp)
#elif defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_WINDOWS_X64)
#include "LocalFileIO_win.inl"
#elif defined(AZ_PLATFORM_ANDROID)
#include "LocalFileIO_android.inl"
#elif defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV) || defined(AZ_PLATFORM_APPLE_OSX) || defined(AZ_PLATFORM_LINUX)
#include "LocalFileIO_posix.inl"
#endif

namespace AZ
{
    namespace IO
    {
        const HandleType LocalHandleStartValue = 1000000; //start the local file io handles at 1 million

        LocalFileIO::LocalFileIO()
        {
            m_nextHandle = LocalHandleStartValue;
        }

        LocalFileIO::~LocalFileIO()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            while (!m_openFiles.empty())
            {
                const auto& handlePair = *m_openFiles.begin();
                Close(handlePair.first);
            }

            AZ_Assert(m_openFiles.empty(), "Trying to shutdown filing system with files still open");
        }

        // Android uses PAKs, which SystemFile doesn't support yet
        // Android implementation is in LocalFileIO_android.inl
#if !defined(AZ_PLATFORM_ANDROID)
        Result LocalFileIO::Open(const char* filePath, OpenMode mode, HandleType& fileHandle)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::IO::UpdateOpenModeForReading(mode);

            // Generate open modes for SystemFile
            int systemFileMode = TranslateOpenModeToSystemFileMode(resolvedPath, mode);
            bool write = AnyFlag(mode & (OpenMode::ModeWrite | OpenMode::ModeUpdate | OpenMode::ModeAppend));
            if (write)
            {
                CheckInvalidWrite(resolvedPath);
            }

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);

                fileHandle = GetNextHandle();

                // Construct a new SystemFile in the map (SystemFiles don't copy/move very well).
                auto newPair = m_openFiles.emplace(fileHandle);
                // Check for successful insert
                if (!newPair.second)
                {
                    fileHandle = InvalidHandle;
                    return ResultCode::Error;
                }
                // Attempt to open the newly created file
                if (newPair.first->second.Open(resolvedPath, systemFileMode, 0))
                {
                    return ResultCode::Success;
                }
                else
                {
                    // Remove file, it's not actually open
                    m_openFiles.erase(fileHandle);
                    // On failure, ensure the fileHandle returned is invalid
                    //  some code does not check return but handle value (equivalent to checking for nullptr FILE*)
                    fileHandle = InvalidHandle;
                    return ResultCode::Error;
                }
            }
        }

        Result LocalFileIO::Close(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            filePointer->Close();

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
                m_openFiles.erase(fileHandle);
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
#if defined(AZ_DEBUG_BUILD)
            // detect portability issues in debug (only)
            // so that the static_cast below doesn't trap us.
            if ((size > static_cast<AZ::u64>(UINT32_MAX)) && (sizeof(size_t) < 8))
            {
                // file read too large for platform!
                return ResultCode::Error;
            }
#endif
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SizeType readResult = filePointer->Read(size, buffer);

            if (bytesRead)
            {
                *bytesRead = aznumeric_cast<AZ::u64>(readResult);
            }

            if (static_cast<AZ::u64>(readResult) != size)
            {
                if (failOnFewerThanSizeBytesRead)
                {
                    return ResultCode::Error;
                }

                // Reading less than desired is valid if ferror is not set
                AZ_Assert(Eof(fileHandle), "End of file unexpectedly reached before all data was read");
            }

            return ResultCode::Success;
        }

        Result LocalFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
#if defined(AZ_DEBUG_BUILD)
            // detect portability issues in debug (only)
            // so that the static_cast below doesn't trap us.
            if ((size > static_cast<AZ::u64>(UINT32_MAX)) && (sizeof(size_t) < 8))
            {
                // file read too large for platform!
                return ResultCode::Error;
            }
#endif

            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SizeType writeResult = filePointer->Write(buffer, size);

            if (bytesWritten)
            {
                *bytesWritten = writeResult;
            }

            if (static_cast<AZ::u64>(writeResult) != size)
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        Result LocalFileIO::Flush(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            filePointer->Flush();

            return ResultCode::Success;
        }

        Result LocalFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SizeType resultValue = filePointer->Tell();
            if (resultValue == -1)
            {
                return ResultCode::Error;
            }

            offset = static_cast<AZ::u64>(resultValue);
            return ResultCode::Success;
        }

        Result LocalFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SeekMode mode = SystemFile::SF_SEEK_BEGIN;
            if (type == SeekType::SeekFromCurrent)
            {
                mode = SystemFile::SF_SEEK_CURRENT;
            }
            else if (type == SeekType::SeekFromEnd)
            {
                mode = SystemFile::SF_SEEK_END;
            }

            filePointer->Seek(offset, mode);

            return ResultCode::Success;
        }

        Result LocalFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            size = filePointer->Length();
            return ResultCode::Success;
        }

        Result LocalFileIO::Size(const char* filePath, AZ::u64& size)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            size = SystemFile::Length(resolvedPath);
            if (!size)
            {
                return SystemFile::Exists(resolvedPath) ? ResultCode::Success : ResultCode::Error;
            }

            return ResultCode::Success;
        }

        bool LocalFileIO::IsReadOnly(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            return !SystemFile::IsWritable(resolvedPath);
        }

        bool LocalFileIO::Eof(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return false;
            }

            return filePointer->Eof();
        }

        AZ::u64 LocalFileIO::ModificationTime(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            return SystemFile::ModificationTime(resolvedPath);
        }

        AZ::u64 LocalFileIO::ModificationTime(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return 0;
            }

            return filePointer->ModificationTime();
        }

        bool LocalFileIO::Exists(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            return SystemFile::Exists(resolvedPath);
        }
#endif

        void LocalFileIO::CheckInvalidWrite(const char* path)
        {
            (void)path;

#if defined(AZ_ENABLE_TRACING)
            const char* assetsAlias = GetAlias("@assets@");

            if (((path) && (assetsAlias) && (azstrnicmp(path, assetsAlias, strlen(assetsAlias)) == 0)))
            {
                AZ_Error("FileIO", false, "You may not alter data inside the asset cache.  Please check the call stack and consider writing into the source asset folder instead.\n"
                    "Attempted write location: %s", path);
            }
#endif
        }

        Result LocalFileIO::Remove(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];

            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            CheckInvalidWrite(resolvedPath);

            if (IsDirectory(resolvedPath))
            {
                return ResultCode::Error;
            }

            return SystemFile::Delete(resolvedPath) ? ResultCode::Success : ResultCode::Error;
        }

        Result LocalFileIO::Rename(const char* originalFilePath, const char* newFilePath)
        {
            char resolvedOldPath[AZ_MAX_PATH_LEN];
            char resolvedNewPath[AZ_MAX_PATH_LEN];

            ResolvePath(originalFilePath, resolvedOldPath, AZ_MAX_PATH_LEN);
            ResolvePath(newFilePath, resolvedNewPath, AZ_MAX_PATH_LEN);

            CheckInvalidWrite(resolvedNewPath);

            if (!SystemFile::Rename(resolvedOldPath, resolvedNewPath))
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

#if defined(AZ_PLATFORM_ANDROID)
        OSHandleType LocalFileIO::GetFilePointerFromHandle(HandleType fileHandle)
#else
        SystemFile* LocalFileIO::GetFilePointerFromHandle(HandleType fileHandle)
#endif
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            auto openFileIterator = m_openFiles.find(fileHandle);
            if (openFileIterator != m_openFiles.end())
            {
                FileDescriptor& file = openFileIterator->second;
#if defined(AZ_PLATFORM_ANDROID)
                return file.m_handle;
#else
                return &file;
#endif
            }
            return nullptr;
        }

        const FileDescriptor* LocalFileIO::GetFileDescriptorFromHandle(HandleType fileHandle)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            auto openFileIterator = m_openFiles.find(fileHandle);
            const FileDescriptor* file = nullptr;
            if (openFileIterator != m_openFiles.end())
            {
                file = &(openFileIterator->second);
            }
            AZ_Assert(file != nullptr, "Can't find valid file descriptor from handle");
            return file;
        }

        HandleType LocalFileIO::GetNextHandle()
        {
            return m_nextHandle++;
        }

        static ResultCode DestroyPath_Recurse(LocalFileIO* fileIO, const char* filePath)
        {
            // this is a deltree command.  It needs to eat everything.  Even files.
            ResultCode res = ResultCode::Success;

            fileIO->FindFiles(filePath, "*.*", [&](const char* iterPath) -> bool
                {
                    // depth first recurse into directories!

                    // note:  findFiles returns full path names.

                    if (fileIO->IsDirectory(iterPath))
                    {
                        // recurse.
                        if (DestroyPath_Recurse(fileIO, iterPath) != ResultCode::Success)
                        {
                            res = ResultCode::Error;
                            return false; // stop the find files.
                        }
                    }
                    else
                    {
                        // if its a file, remove it
                        if (fileIO->Remove(iterPath) != ResultCode::Success)
                        {
                            res = ResultCode::Error;
                            return false; // stop the find files.
                        }
                    }
                    return true; // continue the find files
                });

            if (res != ResultCode::Success)
            {
                return res;
            }

            // now that we've finished recursing, rmdir on the folder itself
            return AZ::IO::SystemFile::DeleteDir(filePath) ? ResultCode::Success : ResultCode::Error;
        }

        Result LocalFileIO::DestroyPath(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            bool pathExists = Exists(resolvedPath);
            if (!pathExists)
            {
                return ResultCode::Success;
            }

            if (pathExists && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error;
            }

            return DestroyPath_Recurse(this, resolvedPath);
        }

        bool LocalFileIO::ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize)
        {
            if (path == nullptr)
            {
                return false;
            }

            if (IsAbsolutePath(path))
            {
                size_t pathLen = strlen(path);
                if (pathLen + 1 < resolvedPathSize)
                {
                    azstrncpy(resolvedPath, resolvedPathSize, path, pathLen + 1);

                    //see if the absolute path uses @assets@ or @root@, if it does lowercase the relative part
                    if (!LowerIfBeginsWith(resolvedPath, resolvedPathSize, GetAlias("@assets@")))
                    {
                        LowerIfBeginsWith(resolvedPath, resolvedPathSize, GetAlias("@root@"));
                    }

                    for (AZ::u64 i = 0; i < resolvedPathSize && resolvedPath[i] != '\0'; i++)
                    {
                        if (resolvedPath[i] == '\\')
                        {
                            resolvedPath[i] = '/';
                        }
                    }

                    return true;
                }
                else
                {
                    return false;
                }
            }

            char rootedPathBuffer[AZ_MAX_PATH_LEN] = {0};
            const char* rootedPath = path;
            // if the path does not begin with an alias, then it is assumed to begin with @assets@
            if (path[0] != '@')
            {
                if (GetAlias("@assets@"))
                {
                    const int rootLength = 9;// strlen("@assets@/")
                    azstrncpy(rootedPathBuffer, AZ_MAX_PATH_LEN, "@assets@/", rootLength);
                    size_t pathLen = strlen(path);
                    size_t rootedPathBufferlength = rootLength + pathLen + 1;// +1 for null terminator
                    if (rootedPathBufferlength > resolvedPathSize)
                    {
                        AZ_Assert(rootedPathBufferlength < resolvedPathSize, "Constructed path length is wrong:%s", rootedPathBuffer);//path constructed is wrong
                        size_t remainingSize = resolvedPathSize - rootLength - 1;// - 1 for null terminator
                        azstrncpy(rootedPathBuffer + rootLength, AZ_MAX_PATH_LEN, path, remainingSize);
                        rootedPathBuffer[resolvedPathSize - 1] = '\0';
                    }
                    else
                    {
                        azstrncpy(rootedPathBuffer + rootLength, AZ_MAX_PATH_LEN - rootLength, path, pathLen + 1);
                    }
                }
                else
                {
                    ConvertToAbsolutePath(path, rootedPathBuffer, AZ_MAX_PATH_LEN);
                }
                rootedPath = rootedPathBuffer;
            }

            for (AZ::u64 i = 0; i < resolvedPathSize && resolvedPath[i] != '\0'; i++)
            {
                if (resolvedPath[i] == '\\')
                {
                    resolvedPath[i] = '/';
                }
            }

            return ResolveAliases(rootedPath, resolvedPath, resolvedPathSize);
        }

        void LocalFileIO::SetAlias(const char* key, const char* path)
        {
            ClearAlias(key);
            char fullPath[AZ_MAX_PATH_LEN];
            ConvertToAbsolutePath(path, fullPath, AZ_MAX_PATH_LEN);
            m_aliases.push_back(AZStd::pair<const char*, const char*>(key, fullPath));
        }

        const char* LocalFileIO::GetAlias(const char* key)
        {
            const auto it = AZStd::find_if(m_aliases.begin(), m_aliases.end(), [key](const AliasType& alias)
                    {
                        return alias.first.compare(key) == 0;
                    });

            if (it != m_aliases.end())
            {
                return it->second.c_str();
            }
            return nullptr;
        }

        void LocalFileIO::ClearAlias(const char* key)
        {
            m_aliases.erase(AZStd::remove_if(m_aliases.begin(), m_aliases.end(), [key](const AliasType& alias)
                    {
                        return alias.first.compare(key) == 0;
                    }), m_aliases.end());
        }

        AZ::u64 LocalFileIO::ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const
        {
            size_t longest_match = 0;
            size_t bufStringLength = strlen(inOutBuffer);
            const char* longestAlias = nullptr;

            for (const auto& alias : m_aliases)
            {
                if (((longest_match == 0) || (alias.second.size() > longest_match)) && (alias.second.size() <= bufStringLength))
                {
                    // custom strcmp that ignores slash directions
                    bool allMatch = true;
                    for (size_t pos = 0; pos < bufStringLength && pos < alias.second.size(); ++pos)
                    {
                        char bufValue = inOutBuffer[pos];
                        char aliasValue = alias.second[pos];
                        if (bufValue == '/')
                        {
                            bufValue = '\\';
                        }
                        if (aliasValue == '/')
                        {
                            aliasValue = '\\';
                        }
                        bufValue = static_cast<char>(tolower(bufValue));
                        aliasValue = static_cast<char>(tolower(aliasValue));
                        if (bufValue != aliasValue)
                        {
                            allMatch = false;
                            break;
                        }
                    }
                    if (allMatch)
                    {
                        // There must be a slash after the match for it to be valid, otherwise it will match against
                        // partial folder names (causes issues when switching branches, for instance)
                        size_t matchLen = alias.second.size();
                        if (matchLen < bufStringLength && (inOutBuffer[matchLen] == '/' || inOutBuffer[matchLen] == '\\'))
                        {
                            longest_match = matchLen;
                            longestAlias = alias.first.c_str();
                        }
                    }
                }
            }
            if (longestAlias)
            {
                // rearrange the buffer to have
                // [alias][old path]
                size_t alias_size = strlen(longestAlias);
                size_t charsToAbsorb = longest_match;
                size_t remainingData = bufStringLength - charsToAbsorb;
                size_t finalStringSize = alias_size + remainingData;
                if (finalStringSize >= bufferLength)
                {
                    AZ_Error("FileIO", false, "Buffer overflow avoided for ConverToAlias on %s (len %llu) with alias %s", inOutBuffer, bufferLength, longestAlias);
                    // avoid buffer overflow, return original untouched
                    return bufStringLength;
                }

                // move the remaining piece of the string:
                memmove(inOutBuffer + alias_size, inOutBuffer + charsToAbsorb, remainingData);
                // copy the alias
                memcpy(inOutBuffer, longestAlias, alias_size);
                /// add a null
                inOutBuffer[finalStringSize] = 0;
                return finalStringSize;
            }
            return bufStringLength;
        }

        bool LocalFileIO::ResolveAliases(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const
        {
            AZ_Assert(path != resolvedPath && resolvedPathSize > strlen(path), "Resolved path is incorrect");
            AZ_Assert(path && path[0] != '%', "%% is deprecated, @ is the only valid alias token");

            size_t pathLen = strlen(path) + 1; // account for null
            azstrncpy(resolvedPath, resolvedPathSize, path, pathLen);
            for (const auto& alias : m_aliases)
            {
                const char* key = alias.first.c_str();
                size_t keyLen = alias.first.length();
                if (azstrnicmp(resolvedPath, key, keyLen) == 0) // we only support aliases at the front of the path
                {
                    if(azstrnicmp(key, "@assets@", 8) == 0 || azstrnicmp(key, "@root@", 6) == 0)
                    {
                        for (AZ::u64 i = keyLen; i < resolvedPathSize && resolvedPath[i] != '\0'; i++)
                        {
                            resolvedPath[i] = static_cast<char>(tolower(static_cast<int>(resolvedPath[i])));
                        }
                    }

                    const char* dest = alias.second.c_str();
                    size_t destLen = alias.second.length();
                    char* afterKey = resolvedPath + keyLen;
                    size_t afterKeyLen = pathLen - keyLen;
                    // must ensure that we are replacing the entire folder name, not a partial (e.g. @GAME01@/ vs @GAME0@/)
                    if (*afterKey == '/' || *afterKey == '\\' || *afterKey == 0)
                    {
                        if (afterKeyLen + destLen + 1 < resolvedPathSize)//if after replacing the alias the length is greater than the max path size than skip
                        {
                            // scoot the right hand side of the replacement over to make room
                            memmove(resolvedPath + destLen, afterKey, afterKeyLen + 1); // make sure null is copied
                            memcpy(resolvedPath, dest, destLen); // insert replacement
                            pathLen -= keyLen;
                            pathLen += destLen;

                            for (AZ::u64 i = 0; i < resolvedPathSize && resolvedPath[i] != '\0'; i++)
                            {
                                if (resolvedPath[i] == '\\')
                                {
                                    resolvedPath[i] = '/';
                                }
                            }

                            return true;
                        }
                    }
                }
            }

            AZ_Assert(path[0] != '@', "Failed to resolve an alias:%s", path); // it is a failure to fail to resolve an alias!
            return false;
        }

#if !defined(AZ_PLATFORM_ANDROID)
        bool LocalFileIO::GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            auto fileIt = m_openFiles.find(fileHandle);
            if (fileIt != m_openFiles.end())
            {
                azstrncpy(filename, filenameSize, fileIt->second.Name(), filenameSize);
                return true;
            }
            return false;
        }
#endif // ANDROID

        bool LocalFileIO::LowerIfBeginsWith(char* inOutBuffer, AZ::u64 bufferLen, const char* alias) const
        {
            if (alias)
            {
                AZ::u64 aliasLen = azlossy_caster(strlen(alias));
                if (azstrnicmp(inOutBuffer, alias, aliasLen) == 0)
                {
                    for (AZ::u64 i = aliasLen; i < bufferLen && inOutBuffer[i] != '\0'; ++i)
                    {
                        inOutBuffer[i] = static_cast<char>(std::tolower(static_cast<int>(inOutBuffer[i])));
                    }

                    return true;
                }
            }

            return false;
        }

        AZ::OSString LocalFileIO::RemoveTrailingSlash(const AZ::OSString& pathStr)
        {
            if (pathStr.empty() || (pathStr[pathStr.length() - 1] != '/' && pathStr[pathStr.length() - 1] != '\\'))
            {
                return pathStr;
            }

            return pathStr.substr(0, pathStr.length() - 1);
        }

        AZ::OSString LocalFileIO::CheckForTrailingSlash(const AZ::OSString& pathStr)
        {
            if (pathStr.empty() || pathStr[pathStr.length() - 1] == '/')
            {
                return pathStr;
            }

            if (pathStr[pathStr.length() - 1] == '\\')
            {
                return pathStr.substr(0, pathStr.length() - 1) + "/";
            }

            return pathStr + "/";
        }
    } // namespace IO
} // namespace AZ
