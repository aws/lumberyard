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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "FileIOHandler_wwise.h"

#include <IAudioInterfacesCommonData.h>
#include <ICryPak.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#define MAX_NUMBER_STRING_SIZE      (10)    // 4G
#define ID_TO_STRING_FORMAT_BANK    AKTEXT("%u.bnk")
#define ID_TO_STRING_FORMAT_WEM     AKTEXT("%u.wem")
#define MAX_EXTENSION_SIZE          (4)     // .xxx
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE + MAX_EXTENSION_SIZE + 1)   // null-terminated

namespace Audio
{
    // AkFileHandle must be able to store our AZ::IO::HandleType
    static_assert(sizeof(AkFileHandle) >= sizeof(AZ::IO::HandleType), "AkFileHandle must be able to store at least the size of a AZ::IO::HandleType");

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AkFileHandle GetAkFileHandle(AZ::IO::HandleType realFileHandle)
    {
        if (realFileHandle == AZ::IO::InvalidHandle)
        {
            return AkFileHandle(INVALID_HANDLE_VALUE);
        }

        return reinterpret_cast<AkFileHandle>(realFileHandle);
    }

    AZ::IO::HandleType GetRealFileHandle(AkFileHandle akFileHandle)
    {
        if (akFileHandle == AkFileHandle(INVALID_HANDLE_VALUE))
        {
            return AZ::IO::InvalidHandle;
        }

#if   defined(AZ_PLATFORM_APPLE)
        // On 64 bit systems, strict compilers throw an error trying to reinterpret_cast
        // from AkFileHandle (a 64 bit pointer) to AZ::IO::HandleType (a uint32_t) because:
        //
        // cast from pointer to smaller type 'AZ::IO::HandleType' (aka 'unsigned int') loses information
        //
        // However, this is safe because AkFileHandle is a "blind" type that serves as a token.
        // We create the token and hand it off, and it is handed back whenever file IO is done.
        return static_cast<AZ::IO::HandleType>(reinterpret_cast<uintptr_t>(akFileHandle));
#else
        return reinterpret_cast<AZ::IO::HandleType>(akFileHandle);
#endif
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CFileIOHandler_wwise::CFileIOHandler_wwise()
        : m_bAsyncOpen(false)
    {
        memset(m_sBankPath, 0, AK_MAX_PATH * sizeof(AkOSChar));
        memset(m_sLanguageFolder, 0, AK_MAX_PATH * sizeof(AkOSChar));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CFileIOHandler_wwise::~CFileIOHandler_wwise()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Init(const AkDeviceSettings& rDeviceSettings, const bool bAsyncOpen /* = false */)
    {
        AKRESULT eResult = AK_Fail;

        if (rDeviceSettings.uSchedulerTypeFlags == AK_SCHEDULER_BLOCKING)
        {
            m_bAsyncOpen = bAsyncOpen;

            // If the Stream Manager's File Location Resolver was not set yet, set this object as the
            // File Location Resolver (this I/O hook is also able to resolve file location).
            if (!AK::StreamMgr::GetFileLocationResolver())
            {
                AK::StreamMgr::SetFileLocationResolver(this);
            }

            // Create a device in the Stream Manager, specifying this as the hook.
            m_nDeviceID = AK::StreamMgr::CreateDevice(rDeviceSettings, this);

            if (m_nDeviceID != AK_INVALID_DEVICE_ID)
            {
                eResult = AK_Success;
            }
        }
        else
        {
            AKASSERT(!"CAkDefaultIOHookBlocking I/O hook only works with AK_SCHEDULER_BLOCKING devices");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::ShutDown()
    {
        if (AK::StreamMgr::GetFileLocationResolver() == this)
        {
            AK::StreamMgr::SetFileLocationResolver(nullptr);
        }

        AK::StreamMgr::DestroyDevice(m_nDeviceID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Open(const AkOSChar* sFileName, AkOpenMode eOpenMode, AkFileSystemFlags* pFlags, bool& rSyncOpen, AkFileDesc& rFileDesc)
    {
        AKRESULT eResult = AK_Fail;

        if (rSyncOpen || !m_bAsyncOpen)
        {
            rSyncOpen = true;
            AkOSChar sFinalFilePath[AK_MAX_PATH] = { '\0' };
            AKPLATFORM::SafeStrCat(sFinalFilePath, m_sBankPath, AK_MAX_PATH);

            if (pFlags && eOpenMode == AK_OpenModeRead)
            {
                // Add language folder if the file is localized.
                if (pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC && pFlags->uCodecID == AKCODECID_BANK && pFlags->bIsLanguageSpecific)
                {
                    AKPLATFORM::SafeStrCat(sFinalFilePath, m_sLanguageFolder, AK_MAX_PATH);
                }
            }

            AKPLATFORM::SafeStrCat(sFinalFilePath, sFileName, AK_MAX_PATH);

            char* sTemp = nullptr;
            CONVERT_OSCHAR_TO_CHAR(sFinalFilePath, sTemp);
            const char* sOpenMode = nullptr;

            switch (eOpenMode)
            {
                case AK_OpenModeRead:
                {
                    sOpenMode = "rbx";
                    break;
                }
                case AK_OpenModeWrite:
                {
                    sOpenMode = "wbx";
                    break;
                }
                case AK_OpenModeWriteOvrwr:
                {
                    sOpenMode = "w+bx";
                    break;
                }
                case AK_OpenModeReadWrite:
                {
                    sOpenMode = "abx";
                    break;
                }
                default:
                {
                    AKASSERT(!"Unknown file open mode!");
                    break;
                }
            }

            AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(sTemp, sOpenMode, ICryPak::FOPEN_HINT_DIRECT_OPERATION);

            if (fileHandle != AZ::IO::InvalidHandle)
            {
                rFileDesc.hFile = GetAkFileHandle(fileHandle);
                rFileDesc.iFileSize = static_cast<AkInt64>(gEnv->pCryPak->FGetSize(sTemp));
                rFileDesc.uSector = 0;
                rFileDesc.deviceID = m_nDeviceID;
                rFileDesc.pCustomParam = nullptr;
                rFileDesc.uCustomParamSize = 0;
                eResult = AK_Success;
            }
        }
        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Open(AkFileID nFileID, AkOpenMode eOpenMode, AkFileSystemFlags* pFlags, bool& rSyncOpen, AkFileDesc& rFileDesc)
    {
        AKRESULT eResult = AK_Fail;

        if (rSyncOpen || !m_bAsyncOpen)
        {
            rSyncOpen = true;
            AkOSChar sFinalFilePath[AK_MAX_PATH] = { '\0' };
            AKPLATFORM::SafeStrCat(sFinalFilePath, m_sBankPath, AK_MAX_PATH);

            if (pFlags && eOpenMode == AK_OpenModeRead)
            {
                // Add language folder if the file is localized.
                if (pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC && pFlags->bIsLanguageSpecific)
                {
                    AKPLATFORM::SafeStrCat(sFinalFilePath, m_sLanguageFolder, AK_MAX_PATH);
                }
            }

            AkOSChar sFileName[MAX_FILETITLE_SIZE] = { '\0' };

            const AkOSChar* const sFilenameFormat = (pFlags->uCodecID == AKCODECID_BANK ? ID_TO_STRING_FORMAT_BANK : ID_TO_STRING_FORMAT_WEM);

            AK_OSPRINTF(sFileName, MAX_FILETITLE_SIZE, sFilenameFormat, static_cast<int unsigned>(nFileID));

            AKPLATFORM::SafeStrCat(sFinalFilePath, sFileName, AK_MAX_PATH);

            char* sTemp = nullptr;
            CONVERT_OSCHAR_TO_CHAR(sFinalFilePath, sTemp);

            const size_t nFileSize = gEnv->pCryPak->FGetSize(sTemp);

            if (nFileSize > 0)
            {
                AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(sTemp, "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);

                if (fileHandle != AZ::IO::InvalidHandle)
                {
                    rFileDesc.iFileSize = static_cast<AkInt64>(nFileSize);
                    rFileDesc.hFile = GetAkFileHandle(fileHandle);
                    rFileDesc.uSector = 0;
                    rFileDesc.deviceID = m_nDeviceID;
                    rFileDesc.pCustomParam = nullptr;
                    rFileDesc.uCustomParamSize = 0;

                    eResult = AK_Success;
                }
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Read(AkFileDesc& rFileDesc, const AkIoHeuristics& rHeuristics, void* pBuffer, AkIOTransferInfo& rTransferInfo)
    {
        AKASSERT(pBuffer && rFileDesc.hFile != AkFileHandle(INVALID_HANDLE_VALUE));

        AZ::IO::HandleType fileHandle = GetRealFileHandle(rFileDesc.hFile);
        const long nCurrentFileReadPos = gEnv->pCryPak->FTell(fileHandle);
        const long nWantedFileReadPos = static_cast<long>(rTransferInfo.uFilePosition);

        if (nCurrentFileReadPos != nWantedFileReadPos)
        {
            gEnv->pCryPak->FSeek(fileHandle, nWantedFileReadPos, SEEK_SET);
        }

        const size_t nBytesRead = gEnv->pCryPak->FReadRaw(pBuffer, 1, rTransferInfo.uRequestedSize, fileHandle);
        AKASSERT(nBytesRead == static_cast<size_t>(rTransferInfo.uRequestedSize));

        return (nBytesRead > 0) ? AK_Success : AK_Fail;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Write(AkFileDesc& rFileDesc, const AkIoHeuristics& rHeuristics, void* pBuffer, AkIOTransferInfo& rTransferInfo)
    {
        AKASSERT(pBuffer && rFileDesc.hFile != AkFileHandle(INVALID_HANDLE_VALUE));

        AZ::IO::HandleType fileHandle = GetRealFileHandle(rFileDesc.hFile);

        const long nCurrentFileWritePos = gEnv->pCryPak->FTell(fileHandle);
        const long nWantedFileWritePos = static_cast<long>(rTransferInfo.uFilePosition);

        if (nCurrentFileWritePos != nWantedFileWritePos)
        {
            gEnv->pCryPak->FSeek(fileHandle, nWantedFileWritePos, SEEK_SET);
        }

        const size_t nBytesWritten = gEnv->pCryPak->FWrite(pBuffer, 1, static_cast<size_t>(rTransferInfo.uRequestedSize), fileHandle);
        AKASSERT(nBytesWritten == static_cast<size_t>(rTransferInfo.uRequestedSize));

        return (nBytesWritten > 0) ? AK_Success : AK_Fail;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Close(AkFileDesc& rFileDesc)
    {
        AKRESULT eResult = AK_Fail;

        if (!gEnv->pCryPak->FClose(GetRealFileHandle(rFileDesc.hFile)))
        {
            eResult = AK_Success;
        }
        else
        {
            AKASSERT(!"Failed to close file handle");
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AkUInt32 CFileIOHandler_wwise::GetBlockSize(AkFileDesc& in_fileDesc)
    {
        // No constraint on block size (file seeking).
        return 1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::GetDeviceDesc(AkDeviceDesc& out_deviceDesc)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AkUInt32 CFileIOHandler_wwise::GetDeviceData()
    {
        return 1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::SetBankPath(const AkOSChar* const sBankPath)
    {
        AKPLATFORM::SafeStrCpy(m_sBankPath, sBankPath, AK_MAX_PATH);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::SetLanguageFolder(const AkOSChar* const sLanguageFolder)
    {
        AKPLATFORM::SafeStrCpy(m_sLanguageFolder, sLanguageFolder, AK_MAX_PATH);
    }
} // namespace Audio
