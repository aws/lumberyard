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

// Description : Player profile implementation for consoles which manage
//               the profile data via the OS and not via a file system
//               which may not be present.


#ifndef __PLAYERPROFILECONSOLE_H__
#define __PLAYERPROFILECONSOLE_H__

#if _MSC_VER > 1000
#   pragma once
#endif

#include "PlayerProfileImplFS.h"

class CPlayerProfileImplConsole
    : public CPlayerProfileManager::IPlatformImpl
    , public ICommonProfileImpl
{
public:
    CPlayerProfileImplConsole();

    // CPlayerProfileManager::IPlatformImpl
    virtual bool Initialize(CPlayerProfileManager* pMgr);
    virtual void Release();
    virtual bool LoginUser(SUserEntry* pEntry);
    virtual bool LogoutUser(SUserEntry* pEntry);
    virtual bool SaveProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name, bool initialSave = false, int /*reason*/ = ePR_All);
    virtual bool LoadProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name);
    virtual bool DeleteProfile(SUserEntry* pEntry, const char* name);
    virtual bool RenameProfile(SUserEntry* pEntry, const char* newName);
    virtual bool GetSaveGames(SUserEntry* pEntry, CPlayerProfileManager::TSaveGameInfoVec& outVec, const char* altProfileName);
    virtual ISaveGame* CreateSaveGame(SUserEntry* pEntry);
    virtual ILoadGame* CreateLoadGame(SUserEntry* pEntry);
    virtual bool DeleteSaveGame(SUserEntry* pEntry, const char* name);
    virtual ILevelRotationFile* GetLevelRotationFile(SUserEntry* pEntry, const char* name);
    virtual bool GetSaveGameThumbnail(SUserEntry* pEntry, const char* saveGameName, SThumbnail& thumbnail);
    virtual void GetMemoryStatistics(ICrySizer* s);
    // ~CPlayerProfileManager::IPlatformImpl

    // ICommonProfileImpl
    virtual void InternalMakeFSPath(SUserEntry* pEntry, const char* profileName, string& outPath);
    virtual void InternalMakeFSSaveGamePath(SUserEntry* pEntry, const char* profileName, string& outPath, bool bNeedFolder);
    virtual CPlayerProfileManager* GetManager() { return m_pMgr; }
    // ~ICommonProfileImpl

protected:
    virtual ~CPlayerProfileImplConsole();

private:
    CPlayerProfileManager* m_pMgr;
};

#endif
