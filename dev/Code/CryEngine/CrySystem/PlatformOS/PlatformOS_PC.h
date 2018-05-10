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

// Description : Interface to the Platform OS


#pragma once


#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(PlatformOS_PC_h, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

#include "IPlatformOS.h"
#include <CryListenerSet.h>
#include <IGameFramework.h>

#include "PlatformOS_Base.h"

namespace ZipDir
{
    class FileEntryTree;

    class CacheRW;
    TYPEDEF_AUTOPTR(CacheRW);
    typedef CacheRW_AutoPtr CacheRWPtr;
}


class CPlatformOS_PC
    : public PlatformOS_Base
    , public IPlatformOS::IPlatformListener
    , public IGameFrameworkListener
{
public:

    CPlatformOS_PC(const uint8 createParams);

    // ~IPlatformOS

    // Called each frame to update the platform listener
    virtual void Tick(float realFrameTime);

    // Local user profile functions to check/initiate user sign in:
    // See IPlatformOS.h for documentation.
    virtual bool                    MountSaveFile(unsigned int userIndex) { return true; }
    virtual IFileFinderPtr          GetFileFinder(unsigned int user);
    virtual void                    MountDLCContent(IDLCListener* pCallback, unsigned int user, const uint8 keyData[16]);

    virtual bool                    CanRestartTitle() const;
    virtual void                    RestartTitle(const char* pTitle);

    virtual bool UsePlatformSavingAPI() const;
    virtual bool BeginSaveLoad(unsigned int user, bool bSave);
    virtual void EndSaveLoad(unsigned int user);
    virtual IPlatformOS::ISaveReaderPtr SaveGetReader(const char* fileName, unsigned int user);
    virtual IPlatformOS::ISaveWriterPtr SaveGetWriter(const char* fileName, unsigned int user);

    virtual bool StringVerifyStart(const char* inString, IStringVerifyEvents* pInCallback);
    virtual bool IsVerifyingString();

    virtual void AddListener(IPlatformOS::IPlatformListener* pListener, const char* szName);
    virtual void RemoveListener(IPlatformOS::IPlatformListener* pListener);
    virtual void NotifyListeners(SPlatformEvent& event);

    virtual ILocalizationManager::EPlatformIndependentLanguageID GetSystemLanguage();
    virtual const char* GetSKUId();
    virtual ILocalizationManager::TLocalizationBitfield GetSystemSupportedLanguages();

    virtual IPlatformOS::EMsgBoxResult DebugMessageBox(const char* body, const char* title, unsigned int flags = 0) const;

    virtual bool PostLocalizationBootChecks();

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    virtual bool DebugSave(SDebugDump& dump);

    virtual bool ConsoleLoadGame(IConsoleCmdArgs* pArgs);

    virtual void InitEncryptionKey(const char* pMagic, size_t magicLength, const uint8* pKey, size_t keyLength);

    virtual void GetEncryptionKey(const DynArray<char>** pMagic = NULL, const DynArray<uint8>** pKey = NULL);

    virtual EUserPIIStatus GetUserPII(unsigned int inUser, SUserPII* pOutPII);

    virtual IPlatformOS::ECDP_Start StartUsingCachePaks(const int user, bool* outWritesOccurred);
    virtual IPlatformOS::ECDP_End EndUsingCachePaks(const int user);
    virtual IPlatformOS::ECDP_Open DoesCachePakExist(const char* const filename, const size_t size, unsigned char md5[16]);
    virtual IPlatformOS::ECDP_Open OpenCachePak(const char* const filename, const char* const bindRoot, const size_t size, unsigned char md5[16]);
    virtual IPlatformOS::ECDP_Close CloseCachePak(const char* const filename);
    virtual IPlatformOS::ECDP_Delete DeleteCachePak(const char* const filename);
    virtual IPlatformOS::ECDP_Write WriteCachePak(const char* const filename, const void* const pData, const size_t numBytes);

    virtual IPlatformOS::EZipExtractFail ExtractZips(const char* path);

    virtual void SetOpticalDriveIdle(bool bIdle);
    virtual void AllowOpticalDriveUsage(bool bAllow);

    // ~IPlatformOS

    // IPlatformOS::IPlatformListener
    virtual void OnPlatformEvent(const IPlatformOS::SPlatformEvent& _event);
    // ~IPlatformOS::IPlatformListener

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener

    // IGameFrameworkListener
    virtual void OnPostUpdate(float fDeltaTime) {}
    virtual void OnSaveGame(ISaveGame* pSaveGame) {}
    virtual void OnLoadGame(ILoadGame* pLoadGame) {}
    virtual void OnLevelEnd(const char* nextLevel) {}
    virtual void OnActionEvent(const SActionEvent& event);
    // ~IGameFrameworkListener

private:

    void SaveDirtyFiles();
    IPlatformOS::EZipExtractFail RecurseZipContents(ZipDir::FileEntryTree* pSourceDir, const char* currentPath, ZipDir::CacheRWPtr pCache);
    bool SxmlMissingFromHDD(ZipDir::FileEntryTree* pSourceDir, const char* currentPath, ZipDir::CacheRWPtr pCache);

    bool DecryptAndCheckSigning(const char* pInData, int inDataLen, char** pOutData, int* pOutDataLen, const uint8 key[16]);
    bool UseSteamReadWriter() const;

private:
    CStableFPSWatcher m_fpsWatcher;
    CListenerSet<IPlatformOS::IPlatformListener*> m_listeners;
    DynArray<char> m_encryptionMagic;
    DynArray<uint8> m_encryptionKey;
    float m_delayLevelStartIcon;
    int m_cachePakStatus;
    int m_cachePakUser;
    bool m_bSaving;
    bool m_bAllowMessageBox;
    bool m_bLevelLoad;
    bool m_bSaveDuringLevelLoad;
};

#endif
