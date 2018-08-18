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
#include "CryPakFileIO.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/lock.h>
#include <ICryPak.h>
#include <AzCore/std/functional.h> // for function<> in the find files callback.

namespace AZ
{
    namespace IO
    {
        CryPakFileIO::CryPakFileIO()
        {
        }

        CryPakFileIO::~CryPakFileIO()
        {
            // close all files.  This makes it mimic the behavior of base FileIO even though it sits on crypak.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);
            while (!m_trackedFiles.empty())
            {
                HandleType fileHandle = m_trackedFiles.begin()->first;
                AZ_Warning("File IO", "File handle still open while CryPakFileIO being closed: %s", m_trackedFiles.begin()->second.c_str());
                Close(fileHandle);
            }
        }

        Result CryPakFileIO::Open(const char* filePath, OpenMode openMode, HandleType& fileHandle)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Open(filePath, openMode, fileHandle);
                }

                return ResultCode::Error;
            }

            fileHandle = gEnv->pCryPak->FOpen(filePath, GetStringModeFromOpenMode(openMode));
            if (fileHandle == InvalidHandle)
            {
                return ResultCode::Error;
            }

            //track the open file handles
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);
            m_trackedFiles.insert(AZStd::make_pair(fileHandle, filePath));

            return ResultCode::Success;
        }

        Result CryPakFileIO::Close(HandleType fileHandle)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Close(fileHandle);
                }
                return ResultCode::Error; //  we are likely shutting down and crypak has already dropped all its handles already.
            }

            if (gEnv->pCryPak->FClose(fileHandle) == 0)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);

                auto remoteFileIter = m_trackedFiles.find(fileHandle);
                if (remoteFileIter != m_trackedFiles.end())
                {
                    m_trackedFiles.erase(remoteFileIter);
                }

                return ResultCode::Success;
            }
            return ResultCode::Error;
        }

        Result CryPakFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Tell(fileHandle, offset);
                }
                return ResultCode::Error;
            }

            offset = gEnv->pCryPak->FTell(fileHandle);
            return ResultCode::Success;
        }

        Result CryPakFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Seek(fileHandle, offset, type);
                }
                return ResultCode::Error;
            }

            int cryPakResult = gEnv->pCryPak->FSeek(fileHandle, static_cast<long>(offset), GetFSeekModeFromSeekType(type));
            return cryPakResult == 0 ? ResultCode::Success : ResultCode::Error;
        }

        Result CryPakFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Size(fileHandle, size);
                }
                return ResultCode::Error;
            }

            size = gEnv->pCryPak->FGetSize(fileHandle);
            if (size || gEnv->pCryPak->IsInPak(fileHandle))
            {
                return ResultCode::Success;
            }

            if (GetDirectInstance())
            {
                return GetDirectInstance()->Size(fileHandle, size);
            }

            return ResultCode::Error;
        }

        Result CryPakFileIO::Size(const char* filePath, AZ::u64& size)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Size(filePath, size);
                }
                return ResultCode::Error;
            }

            size = gEnv->pCryPak->FGetSize(filePath, true);
            if (!size)
            {
                return gEnv->pCryPak->IsFileExist(filePath, ICryPak::eFileLocation_Any) ? ResultCode::Success : ResultCode::Error;
            }

            return ResultCode::Success;
        }

        Result CryPakFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Read(fileHandle, buffer, size, failOnFewerThanSizeBytesRead, bytesRead);
                }

                return ResultCode::Error;
            }

            size_t result = gEnv->pCryPak->FReadRaw(buffer, 1, size, fileHandle);
            if (bytesRead)
            {
                *bytesRead = static_cast<AZ::u64>(result);
            }

            if (failOnFewerThanSizeBytesRead)
            {
                return result != size ? ResultCode::Error : ResultCode::Success;
            }
            return result == 0 ? ResultCode::Error : ResultCode::Success;
        }

        Result CryPakFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Write(fileHandle, buffer, size, bytesWritten);
                }

                return ResultCode::Error;
            }

            size_t result = gEnv->pCryPak->FWrite(buffer, 1, size, fileHandle);
            if (bytesWritten)
            {
                *bytesWritten = static_cast<AZ::u64>(result);
            }

            return result != size ? ResultCode::Error : ResultCode::Success;
        }

        Result CryPakFileIO::Flush(HandleType fileHandle)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Flush(fileHandle);
                }
                return ResultCode::Error;
            }

            return gEnv->pCryPak->FFlush(fileHandle) == 0 ? ResultCode::Success : ResultCode::Error;
        }

        bool CryPakFileIO::Eof(HandleType fileHandle)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Eof(fileHandle);
                }
                return false;
            }

            return (gEnv->pCryPak->FEof(fileHandle) != 0);
        }

        bool CryPakFileIO::Exists(const char* filePath)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Exists(filePath);
                }
                return false;
            }

            return gEnv->pCryPak->IsFileExist(filePath);
        }

        AZ::u64 CryPakFileIO::ModificationTime(const char* filePath)
        {
            HandleType openFile = InvalidHandle;
            if (!Open(filePath, OpenMode::ModeRead | OpenMode::ModeBinary, openFile))
            {
                return 0;
            }

            AZ::u64 result = ModificationTime(openFile);

            Close(openFile);

            return result;
        }

        AZ::u64 CryPakFileIO::ModificationTime(HandleType fileHandle)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->ModificationTime(fileHandle);
                }
                return 0;
            }

            return gEnv->pCryPak->GetModificationTime(fileHandle);
        }

        bool CryPakFileIO::IsDirectory(const char* filePath)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->IsDirectory(filePath);
                }
                return false;
            }

            return gEnv->pCryPak->IsFolder(filePath);
        }

        Result CryPakFileIO::CreatePath(const char* filePath)
        {
            // since you can't create a path inside a pak file
            // we will pass this to the underlying real fileio!
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return ResultCode::Error;
            }

            return realUnderlyingFileIO->CreatePath(filePath);
        }

        Result CryPakFileIO::DestroyPath(const char* filePath)
        {
            // since you can't destroy a path inside a pak file
            // we will pass this to the underlying real fileio
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return ResultCode::Error;
            }

            return realUnderlyingFileIO->DestroyPath(filePath);
        }

        Result CryPakFileIO::Remove(const char* filePath)
        {
            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->Remove(filePath);
                }
                return ResultCode::Error;
            }

            return (gEnv->pCryPak->RemoveFile(filePath) ? ResultCode::Success : ResultCode::Error);
        }

        Result CryPakFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            // you can actually copy a file from inside a pak to a destination path if you want to...
            HandleType sourceFile = InvalidHandle;
            HandleType destinationFile = InvalidHandle;
            if (!Open(sourceFilePath, OpenMode::ModeRead | OpenMode::ModeBinary, sourceFile))
            {
                return ResultCode::Error;
            }

            // avoid using AZStd::string if possible - use OSString instead of StringFunc
            AZ::OSString destPath(destinationFilePath);

            AZ::OSString::size_type pos = destPath.find_last_of("\\/");
            if (pos != AZ::OSString::npos)
            {
                destPath.resize(pos);
            }
            CreatePath(destPath.c_str());

            if (!Open(destinationFilePath, OpenMode::ModeWrite | OpenMode::ModeBinary, destinationFile))
            {
                Close(sourceFile);
                return ResultCode::Error;
            }

            // standard buffered copy.
            bool failureEncountered = false;
            AZ::u64 bytesRemaining = 0;

            if (!Size(sourceFilePath, bytesRemaining))
            {
                Close(destinationFile);
                Close(sourceFile);
                return ResultCode::Error;
            }

            while (bytesRemaining > 0)
            {
                size_t bytesThisTime = AZStd::GetMin<size_t>(bytesRemaining, CRYPAK_FILEIO_MAX_BUFFERSIZE);

                if (!Read(sourceFile, m_copyBuffer.data(), bytesThisTime, true))
                {
                    failureEncountered = true;
                    break;
                }

                AZ::u64 actualBytesWritten = 0;

                if ((!Write(destinationFile, m_copyBuffer.data(), bytesThisTime, &actualBytesWritten)) || (actualBytesWritten == 0))
                {
                    failureEncountered = true;
                    break;
                }
                bytesRemaining -= actualBytesWritten;
            }

            Close(sourceFile);
            Close(destinationFile);
            return (failureEncountered || (bytesRemaining > 0)) ? ResultCode::Error : ResultCode::Success;
        }


        bool CryPakFileIO::IsReadOnly(const char* filePath)
        {
            // a tricky one!  files inside a pack are technically readonly...
            HandleType openedHandle = InvalidHandle;

            if (!Open(filePath, OpenMode::ModeRead | OpenMode::ModeBinary, openedHandle))
            {
                return false; // this will also return false if there is no crypak, so no need to check it again
            }

            bool inPak = gEnv->pCryPak->IsInPak(openedHandle);
            Close(openedHandle);

            if (inPak)
            {
                return true; // things inside packfiles are read only by default since you cannot modify them while pak is mounted.
            }

            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return false;
            }

            return realUnderlyingFileIO->IsReadOnly(filePath);
        }

        Result CryPakFileIO::Rename(const char* sourceFilePath, const char* destinationFilePath)
        {
            // since you cannot perform this opearation inside a pak file, we do it on the real file
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return ResultCode::Error;
            }

            return realUnderlyingFileIO->Rename(sourceFilePath, destinationFilePath);
        }

        Result CryPakFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            // note that the underlying findFiles takes both path and filter.
            if (!filePath)
            {
                return ResultCode::Error;
            }

            if ((!gEnv) || (!gEnv->pCryPak))
            {
                if (GetDirectInstance())
                {
                    return GetDirectInstance()->FindFiles(filePath, filter, callback);
                }
                return ResultCode::Error;
            }

            OSString total = filePath;
            if (total.empty())
            {
                return ResultCode::Error;
            }

            if ((total[total.size() - 1] != '\\') && (total[total.size() - 1] != '/'))
            {
                total.append("/");
            }

            total.append(filter);

            char tempBuffer[AZ_MAX_PATH_LEN];

            _finddata_t fileinfo;
            intptr_t handle = gEnv->pCryPak->FindFirst(total.c_str(), &fileinfo);
            if (handle == -1)
            {
                return ResultCode::Success; // its not an actual fatal error to not find anything.
            }
            do
            {
                azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, filePath);
                azstrcat(tempBuffer, AZ_MAX_PATH_LEN, "/");
                azstrcat(tempBuffer, AZ_MAX_PATH_LEN, fileinfo.name);
                ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);
                if (!callback(tempBuffer))
                {
                    break;
                }
            } while (gEnv->pCryPak->FindNext(handle, &fileinfo) != -1);

            gEnv->pCryPak->FindClose(handle);

            return ResultCode::Success;
        }

        bool CryPakFileIO::GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
        {
            // because we sit on crypak we need to keep track of crypak files too.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_operationGuard);
            const auto fileIt = m_trackedFiles.find(fileHandle);
            if (fileIt != m_trackedFiles.end())
            {
                CRY_ASSERT(filenameSize >= fileIt->second.length());
                azstrncpy(filename, filenameSize, fileIt->second.c_str(), fileIt->second.length());
                return true;
            }

            if (GetDirectInstance())
            {
                return GetDirectInstance()->GetFilename(fileHandle, filename, filenameSize);
            }
            return false;
        }

        // the rest of these functions just pipe through to the underlying real file IO, since
        // crypak doesn't know about any of this.
        void CryPakFileIO::SetAlias(const char* alias, const char* path)
        {
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return;
            }
            realUnderlyingFileIO->SetAlias(alias, path);
        }

        const char* CryPakFileIO::GetAlias(const char* alias)
        {
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return nullptr;
            }
            return realUnderlyingFileIO->GetAlias(alias);
        }

        void CryPakFileIO::ClearAlias(const char* alias)
        {
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return;
            }
            realUnderlyingFileIO->GetAlias(alias);
        }

        AZ::u64 CryPakFileIO::ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const
        {
            if ((!inOutBuffer) || (bufferLength == 0))
            {
                return 0;
            }
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                inOutBuffer[0] = 0;
                return 0;
            }
            return realUnderlyingFileIO->ConvertToAlias(inOutBuffer, bufferLength);
        }

        bool CryPakFileIO::ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize)
        {
            FileIOBase* realUnderlyingFileIO = FileIOBase::GetDirectInstance();
            if (!realUnderlyingFileIO)
            {
                return false;
            }
            return realUnderlyingFileIO->ResolvePath(path, resolvedPath, resolvedPathSize);
        }
    } // namespace IO
}//namespace AZ