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

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CFileIOHandler_wwise
        : public AK::StreamMgr::IAkFileLocationResolver
        , public AK::StreamMgr::IAkIOHookBlocking
    {
    public:

        CFileIOHandler_wwise();
        ~CFileIOHandler_wwise() override;

        CFileIOHandler_wwise(const CFileIOHandler_wwise&) = delete;         // Copy protection
        CFileIOHandler_wwise& operator=(const CFileIOHandler_wwise&) = delete; // Copy protection

        AKRESULT Init(const AkDeviceSettings& rDeviceSettings, const bool bAsyncOpen = false);
        void ShutDown();

        // IAkFileLocationResolver overrides.
        AKRESULT Open(const AkOSChar* sFileName, AkOpenMode eOpenMode, AkFileSystemFlags* pFlags, bool& rSyncOpen, AkFileDesc& rFileDesc) override;
        AKRESULT Open(AkFileID nFileID, AkOpenMode eOpenMode, AkFileSystemFlags* pFlags, bool& rSyncOpen, AkFileDesc& rFileDesc) override;
        // ~IAkFileLocationResolver overrides.

        // IAkIOHookBlocking overrides.
        AKRESULT Read(AkFileDesc& rFileDesc, const AkIoHeuristics& rHeuristics, void* pBuffer, AkIOTransferInfo& rTransferInfo) override;
        AKRESULT Write(AkFileDesc& in_fileDesc, const AkIoHeuristics& in_heuristics, void* in_pData, AkIOTransferInfo& io_transferInfo) override;
        // ~IAkIOHookBlocking overrides.

        // IAkLowLevelIOHook overrides.
        AKRESULT Close(AkFileDesc& rFileDesc) override;
        AkUInt32 GetBlockSize(AkFileDesc& in_fileDesc) override;
        void GetDeviceDesc(AkDeviceDesc& out_deviceDesc) override;
        AkUInt32 GetDeviceData() override;
        // ~IAkLowLevelIOHook overrides.

        void SetBankPath(const AkOSChar* const sBankPath);
        void SetLanguageFolder(const AkOSChar* const sLanguageFolder);

    private:
        AkOSChar m_sBankPath[AK_MAX_PATH];
        AkOSChar m_sLanguageFolder[AK_MAX_PATH];
        bool m_bAsyncOpen;
        AkDeviceID m_nDeviceID;
    };
} // namespace Audio
