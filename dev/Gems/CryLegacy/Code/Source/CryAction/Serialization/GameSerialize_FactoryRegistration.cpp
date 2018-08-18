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

#include "CryLegacy_precompiled.h"
#include "GameSerialize.h"

#include "XmlSaveGame.h"
#include "XmlLoadGame.h"

#include "IGameFramework.h"
#include "IPlayerProfiles.h"

namespace
{
    ISaveGame* CSaveGameCurrentUser()
    {
        IPlayerProfileManager* pPPMgr = CCryAction::GetCryAction()->GetIPlayerProfileManager();
        if (pPPMgr)
        {
            int userCount = pPPMgr->GetUserCount();
            if (userCount > 0)
            {
                IPlayerProfileManager::SUserInfo info;
                if (pPPMgr->GetUserInfo(pPPMgr->GetCurrentUserIndex(), info))
                {
                    IPlayerProfile* pProfile = pPPMgr->GetCurrentProfile(info.userId);
                    if (pProfile)
                    {
                        return pProfile->CreateSaveGame();
                    }
                    else
                    {
                        GameWarning("CSaveGameCurrentUser: User '%s' has no active profile. No save created.", info.userId);
                    }
                }
                else
                {
                    GameWarning("CSaveGameCurrentUser: Invalid logged in user. No save created.");
                }
            }
            else
            {
                GameWarning("CSaveGameCurrentUser: No User logged in. No save created.");
            }
        }

        // can't save without a profile
        return NULL;
    }

    ILoadGame* CLoadGameCurrentUser()
    {
        IPlayerProfileManager* pPPMgr = CCryAction::GetCryAction()->GetIPlayerProfileManager();
        if (pPPMgr)
        {
            int userCount = pPPMgr->GetUserCount();
            if (userCount > 0)
            {
                IPlayerProfileManager::SUserInfo info;
                if (pPPMgr->GetUserInfo(pPPMgr->GetCurrentUserIndex(), info))
                {
                    IPlayerProfile* pProfile = pPPMgr->GetCurrentProfile(info.userId);
                    if (pProfile)
                    {
                        return pProfile->CreateLoadGame();
                    }
                    else
                    {
                        GameWarning("CLoadGameCurrentUser: User '%s' has no active profile. Can't load game.", info.userId);
                    }
                }
                else
                {
                    GameWarning("CSaveGameCurrentUser: Invalid logged in user. Can't load game.");
                }
            }
            else
            {
                GameWarning("CLoadGameCurrentUser: No User logged in. Can't load game.");
            }
        }

        // can't load without a profile
        return NULL;
    }
};

void CGameSerialize::RegisterFactories(IGameFramework* pFW)
{
    // save/load game factories
    REGISTER_FACTORY(pFW, "xml", CXmlSaveGame, false);
    REGISTER_FACTORY(pFW, "xml", CXmlLoadGame, false);
    //  REGISTER_FACTORY(pFW, "binary", CXmlSaveGame, false);
    //  REGISTER_FACTORY(pFW, "binary", CXmlLoadGame, false);

    pFW->RegisterFactory("xml", CLoadGameCurrentUser, false);
    pFW->RegisterFactory("xml", CSaveGameCurrentUser, false);
}
