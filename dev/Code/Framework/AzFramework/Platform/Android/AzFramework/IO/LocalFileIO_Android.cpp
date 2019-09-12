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
#include <dirent.h>
#include <fstream>

#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/Utils.h>

#include <android/api-level.h>

#if __ANDROID_API__ == 19
    // The following were apparently introduced in API 21, however in earlier versions of the 
    // platform specific headers they were defines.  In the move to unified headers, the following
    // defines were removed from stat.h
    #ifndef stat64
        #define stat64 stat
    #endif

    #ifndef fstat64
        #define fstat64 fstat
    #endif

    #ifndef lstat64
        #define lstat64 lstat
    #endif
#endif // __ANDROID_API__ == 19


namespace AZ
{
    namespace IO
    {
        Result LocalFileIO::Open(const char* filePath, OpenMode mode, HandleType& fileHandle)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("%s", filePath);

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);
            FILE* filePointer;
            bool readingAPK = false;
            AZ::u64 size = 0;

            if (!AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                filePointer = fopen(resolvedPath, GetStringModeFromOpenMode(mode));
            }
            else
            {
                //try opening the file in the APK
                readingAPK = true;
                filePointer = AZ::Android::APKFileHandler::Open(resolvedPath, GetStringModeFromOpenMode(mode), size);
            }

            if (filePointer != nullptr)
            {
                fileHandle = GetNextHandle();
                {
                    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
                    FileDescriptor file;
                    file.m_filename = resolvedPath;
                    file.m_handle = filePointer;
                    file.m_isPackaged = readingAPK;
                    file.m_size = size;
                    m_openFiles[fileHandle] = file;
                }
                return ResultCode::Success;
            }

            // On failure, ensure the fileHandle returned is invalid
            //  some code does not check return but handle value (equivalent to checking for nullptr FILE*)
            fileHandle = InvalidHandle;
            return ResultCode::Error;
        }

        Result LocalFileIO::Close(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            if (fclose(filePointer))
            {
                return ResultCode::Error;
            }

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
                m_openFiles.erase(fileHandle);
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
            ANDROID_IO_PROFILE_SECTION;

        #if defined(AZ_DEBUG_BUILD)
            // detect portability issues in debug (only)
            // so that the static_cast below doesn't trap us.
            if ((size > static_cast<uint64_t>(UINT32_MAX)) && (sizeof(size_t) < 8))
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

            AZ::Android::APKFileHandler::SetNumBytesToRead(static_cast<size_t>(size));
            size_t readResult = fread(buffer, 1, static_cast<size_t>(size), filePointer);
            if (static_cast<uint64_t>(readResult) != size)
            {
                if (ferror(filePointer) || failOnFewerThanSizeBytesRead)
                {
                    return ResultCode::Error;
                }

                // Reading less than desired is valid if ferror is not set
                AZ_Assert(feof(filePointer), "End of file unexpectedly reached before all data was read");
            }

            if (bytesRead)
            {
                *bytesRead = static_cast<uint64_t>(readResult);
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
            ANDROID_IO_PROFILE_SECTION;

        #if defined(AZ_DEBUG_BUILD)
            // detect portability issues in debug (only)
            // so that the static_cast below doesn't trap us.
            if ((size > static_cast<uint64_t>(UINT32_MAX)) && (sizeof(size_t) < 8))
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

            size_t writeResult = fwrite(buffer, 1, static_cast<size_t>(size), filePointer);

            if (bytesWritten)
            {
                *bytesWritten = writeResult;
            }

            if (static_cast<uint64_t>(writeResult) != size)
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        Result LocalFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
            ANDROID_IO_PROFILE_SECTION;

            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            offset = static_cast<AZ::u64>(ftello(filePointer));
            return ResultCode::Success;
        }

        bool LocalFileIO::Eof(HandleType fileHandle)
        {
            ANDROID_IO_PROFILE_SECTION;

            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return false;
            }

            return feof(filePointer) != 0;
        }

        Result LocalFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
            ANDROID_IO_PROFILE_SECTION;

            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            int operatingSystemSeekType = GetFSeekModeFromSeekType(type);
            int ret = fseeko(filePointer, static_cast<off_t>(offset), operatingSystemSeekType);

            if (ret == -1)
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        Result LocalFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            ANDROID_IO_PROFILE_SECTION;

            auto fileDescriptor = GetFileDescriptorFromHandle(fileHandle);
            if (fileDescriptor == nullptr)
            {
                return ResultCode::Error_HandleInvalid;
            }

            auto filePointer = fileDescriptor->m_handle;

            if (!fileDescriptor->m_isPackaged)
            {
                if (!filePointer)
                {
                    return ResultCode::Error_HandleInvalid;
                }

                struct stat64 statResult;
                if (fstat64(fileno(filePointer), &statResult) != 0)
                {
                    return ResultCode::Error;
                }

                size = static_cast<AZ::u64>(statResult.st_size);
            }
            else
            {
                size = fileDescriptor->m_size;
            }

            return ResultCode::Success;
        }


        Result LocalFileIO::Size(const char* filePath, AZ::u64& size)
        {
            ANDROID_IO_PROFILE_SECTION;

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                size = AZ::Android::APKFileHandler::FileLength(resolvedPath);
                if (!size)
                {
                    return Exists(resolvedPath) ? ResultCode::Success : ResultCode::Error;
                }
            }
            else
            {
                struct stat64 statResult;
                if (stat64(resolvedPath, &statResult) != 0)
                {
                    return ResultCode::Error;
                }

                size = static_cast<AZ::u64>(statResult.st_size);
            }

            return ResultCode::Success;
        }

        bool LocalFileIO::IsDirectory(const char* filePath)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("IsDir:%s", filePath);

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                return AZ::Android::APKFileHandler::IsDirectory(AZ::Android::Utils::StripApkPrefix(resolvedPath));
            }

            struct stat result;
            if (stat(resolvedPath, &result) == 0)
            {
                return S_ISDIR(result.st_mode);
            }
            return false;
        }

        bool LocalFileIO::IsReadOnly(const char* filePath)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("IsReadOnly:%s", filePath);
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(filePath))
            {
                return true;
            }

            return access(resolvedPath, W_OK) != 0;
        }

        Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            char resolvedSourcePath[AZ_MAX_PATH_LEN];
            char resolvedDestPath[AZ_MAX_PATH_LEN];
            ResolvePath(sourceFilePath, resolvedSourcePath, AZ_MAX_PATH_LEN);
            ResolvePath(destinationFilePath, resolvedDestPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(sourceFilePath) || AZ::Android::Utils::IsApkPath(destinationFilePath))
            {
                return ResultCode::Error; //copy from APK still to be implemented (and of course you can't copy to an APK)
            }

            // note:  Android, without root, has no reliable way to update modtimes
            //        on files on internal storage - this includes "emulated" SDCARD storage
            //        that actually resides on internal, and thus we can't depend on modtimes.
            {
                std::ifstream  sourceFile(resolvedSourcePath, std::ios::binary);
                if (sourceFile.fail())
                {
                    return ResultCode::Error;
                }

                std::ofstream  destFile(resolvedDestPath, std::ios::binary);
                if (destFile.fail())
                {
                    return ResultCode::Error;
                }
                destFile << sourceFile.rdbuf();
            }
            return ResultCode::Success;
        }


        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, LocalFileIO::FindFilesCallbackType callback)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("FindFiles:%s", filePath);

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::OSString pathWithoutSlash = RemoveTrailingSlash(resolvedPath);
            bool isInAPK = AZ::Android::Utils::IsApkPath(pathWithoutSlash.c_str());

            if (isInAPK)
            {
                AZ::OSString strippedPath = AZ::Android::Utils::StripApkPrefix(pathWithoutSlash.c_str());

                char tempBuffer[AZ_MAX_PATH_LEN] = {0};

                AZ::Android::APKFileHandler::ParseDirectory(strippedPath.c_str(), [&](const char* name)
                    {
                        if (NameMatchesFilter(name, filter))
                        {
                            AZ::OSString foundFilePath = CheckForTrailingSlash(resolvedPath);
                            foundFilePath += name;
                            // if aliased, de-alias!
                            azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                            ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                            if (!callback(tempBuffer))
                            {
                                return false;
                            }
                        }
                        return true;
                    });
            }
            else
            {
                DIR* dir = opendir(pathWithoutSlash.c_str());

                if (dir != nullptr)
                {
                    // because the absolute path might actually be SHORTER than the alias ("c:/r/dev" -> "@devroot@"), we need to
                    // use a static buffer here.
                    char tempBuffer[AZ_MAX_PATH_LEN];

                    // clear the errno state so we can distinguish between errors and end of stream
                    errno = 0;
                    struct dirent* entry = readdir(dir);

                    // List all the other files in the directory.
                    while (entry != nullptr)
                    {
                        if (NameMatchesFilter(entry->d_name, filter))
                        {
                            AZ::OSString foundFilePath = CheckForTrailingSlash(resolvedPath);
                            foundFilePath += entry->d_name;
                            // if aliased, de-alias!
                            azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                            ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                            if (!callback(tempBuffer))
                            {
                                break;
                            }
                        }
                        entry = readdir(dir);
                    }

                    if (errno != 0)
                    {
                        closedir(dir);
                        return ResultCode::Error;
                    }

                    closedir(dir);
                    return ResultCode::Success;
                }
                else
                {
                    return ResultCode::Error;
                }
            }
            return ResultCode::Success;
        }

        AZ::u64 LocalFileIO::ModificationTime(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            struct stat result;
            if (stat(resolvedPath, &result))
            {
                return 0;
            }
            return static_cast<AZ::u64>(result.st_mtime);
        }

        AZ::u64 LocalFileIO::ModificationTime(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return 0;
            }
            struct stat64 statResult;
            if (fstat64(fileno(filePointer), &statResult) != 0)
            {
                return 0;
            }
            return static_cast<AZ::u64>(statResult.st_mtime);
        }


        Result LocalFileIO::CreatePath(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                return ResultCode::Error; //you can't write to the APK
            }

            // create all paths up to that directory.
            // its not an error if the path exists.
            if ((Exists(resolvedPath)) && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error; // that path exists, but is not a directory.
            }

            // make directories from bottom to top.
            AZ::OSString pathBuffer;
            size_t pathLength = strlen(resolvedPath);
            pathBuffer.reserve(pathLength);
            for (size_t pathPos = 0; pathPos < pathLength; ++pathPos)
            {
                if ((resolvedPath[pathPos] == '\\') || (resolvedPath[pathPos] == '/'))
                {
                    if (pathPos > 0)
                    {
                        mkdir(pathBuffer.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        if (!IsDirectory(pathBuffer.c_str()))
                        {
                            return ResultCode::Error;
                        }
                    }
                }
                pathBuffer.push_back(resolvedPath[pathPos]);
            }

            mkdir(pathBuffer.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            return IsDirectory(resolvedPath) ? ResultCode::Success : ResultCode::Error;
        }

        bool LocalFileIO::IsAbsolutePath(const char* path) const
        {
            return path && path[0] == '/';
        }

        bool LocalFileIO::ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const
        {
            if (AZ::Android::Utils::IsApkPath(path))
            {
                azstrncpy(absolutePath, maxLength, path, maxLength);
                return true;
            }
            AZ_Assert(maxLength >= AZ_MAX_PATH_LEN, "Path length is larger than AZ_MAX_PATH_LEN");
            if (!IsAbsolutePath(path))
            {
                // note that realpath fails if the path does not exist and actually changes the return value
                // to be the actual place that FAILED, which we don't want.
                // if we fail, we'd prefer to fall through and at least use the original path.
                const char* result = realpath(path, absolutePath);
                if (result)
                {
                    return true;
                }
            }
            azstrcpy(absolutePath, maxLength, path);
            return IsAbsolutePath(absolutePath);
        }

        bool LocalFileIO::Exists(const char* filePath)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("Exists:%s", filePath);

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);
            if (AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                return AZ::Android::APKFileHandler::DirectoryOrFileExists(resolvedPath);
            }
            return SystemFile::Exists(resolvedPath);
        }

        bool LocalFileIO::GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            auto fileIt = m_openFiles.find(fileHandle);
            if (fileIt != m_openFiles.end())
            {
                azstrncpy(filename, filenameSize, fileIt->second.m_filename.c_str(), filenameSize);
                return true;
            }
            return false;
        }

        Result LocalFileIO::Flush(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            if (fflush(filePointer))
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }
    } // namespace IO
}//namespace AZ
