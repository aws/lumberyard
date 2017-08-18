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

#ifndef CRYINCLUDE_CRYACTION_PLAYERPROFILES_PLAYERPROFILEIMPLRSFHELPER_H
#define CRYINCLUDE_CRYACTION_PLAYERPROFILES_PLAYERPROFILEIMPLRSFHELPER_H
#pragma once

#include "PlayerProfileImplFS.h"

class CRichSaveGameHelper
{
public:
    CRichSaveGameHelper(ICommonProfileImpl* pImpl)
        : m_pImpl(pImpl) {}
    bool GetSaveGames(CPlayerProfileManager::SUserEntry* pEntry, CPlayerProfileManager::TSaveGameInfoVec& outVec, const char* profileName);
    ISaveGame* CreateSaveGame(CPlayerProfileManager::SUserEntry* pEntry);
    ILoadGame* CreateLoadGame(CPlayerProfileManager::SUserEntry* pEntry);
    bool DeleteSaveGame(CPlayerProfileManager::SUserEntry* pEntry, const char* name);
    bool GetSaveGameThumbnail(CPlayerProfileManager::SUserEntry* pEntry, const char* saveGameName, CPlayerProfileManager::SThumbnail& thumbnail);
    bool MoveSaveGames(CPlayerProfileManager::SUserEntry* pEntry, const char* oldProfileName, const char* newProfileName);

protected:
    bool FetchMetaData(XmlNodeRef& root, CPlayerProfileManager::SSaveGameMetaData& metaData);
    ICommonProfileImpl* m_pImpl;
};

#endif // CRYINCLUDE_CRYACTION_PLAYERPROFILES_PLAYERPROFILEIMPLRSFHELPER_H
