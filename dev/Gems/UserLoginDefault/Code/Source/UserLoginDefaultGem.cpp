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
#include "StdAfx.h"
#include <platform_impl.h>
#include "UserLoginDefaultGem.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <IPlayerProfiles.h>
#include <ILevelSystem.h>

void UserLoginDefault::UserLoginDefaultGem::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
{
    CryHooksModule::OnCrySystemInitialized(system, systemInitParams);

    if (system.GetPlatformOS())
    {
        system.GetPlatformOS()->AddListener(this, "UserLoginDefaultGem");
    }
}
void UserLoginDefault::UserLoginDefaultGem::OnCrySystemShutdown(ISystem& system)
{
    if (system.GetPlatformOS())
    {
        system.GetPlatformOS()->RemoveListener(this);
    }

    CryHooksModule::OnCrySystemShutdown(system);
}

void UserLoginDefault::UserLoginDefaultGem::OnPlatformEvent(const IPlatformOS::SPlatformEvent& event)
{
    switch (event.m_eEventType)
    {
    case IPlatformOS::SPlatformEvent::eET_SignIn:
        if (!event.m_uParams.m_storageMounted.m_bOnlyUpdateMediaState
            && event.m_uParams.m_signIn.m_signedInState != IPlatformOS::SPlatformEvent::eSIS_NotSignedIn)
        {
            UserLogin(event.m_user);
        }
        break;
    }
}

void UserLoginDefault::UserLoginDefaultGem::UserLogin(unsigned int user)
{
    auto pGameFramework = GetISystem()->GetIGame()->GetIGameFramework();
    auto profileManager = pGameFramework->GetIPlayerProfileManager();
    if (!profileManager)
    {
        GameWarning("[GameProfiles]: PlayerProfileManager not available. Running without.");
        return;
    }

    IPlatformOS::TUserName tUserName;
    bool ok = GetISystem()->GetPlatformOS()->UserGetName(user, tUserName);
    if (!ok)
    {
        GameWarning("[GameProfiles]: Cannot get user name from platform.");
        return;
    }
    const char* userName = tUserName.c_str();

    bool bIsFirstTime = false;
    ok = profileManager->LoginUser(tUserName, bIsFirstTime);
    if (!ok)
    {
        GameWarning("[GameProfiles]: Cannot login user '%s'", userName);
        return;
    }

    // activate the always present profile "default"
    int profileCount = profileManager->GetProfileCount(userName);
    if (profileCount == 0)
    {
        GameWarning("[GameProfiles]: User '%s' has no profiles", userName);
        return;
    }

    bool handled = false;
    IPlayerProfileManager::SProfileDescription profDesc;

    if (gEnv->IsDedicated())
    {
        for (int i = 0; i < profileCount; ++i)
        {
            ok = profileManager->GetProfileInfo(userName, i, profDesc);
            if (ok)
            {
                const IPlayerProfile* preview = profileManager->PreviewProfile(userName, profDesc.name);
                int iActive = 0;
                if (preview)
                {
                    preview->GetAttribute("Activated", iActive);
                }
                if (iActive > 0)
                {
                    profileManager->ActivateProfile(userName, profDesc.name);
                    CryLogAlways("[GameProfiles]: Successfully activated profile '%s' for user '%s'", profDesc.name, userName);
                    pGameFramework->GetILevelSystem()->LoadRotation();
                    handled = true;
                    break;
                }
            }
        }
        profileManager->PreviewProfile(userName, NULL);
    }

    if (!handled)
    {
        // Get default profile info
        ok = profileManager->GetProfileInfo(userName, 0, profDesc);

        if (!ok)
        {
            GameWarning("[GameProfiles]: Cannot get profile info for user '%s'", userName);
        }
        else
        {
            // Set last login time to now
            time_t lastLoginTime;
            time(&lastLoginTime);
            profileManager->SetProfileLastLoginTime(userName, 0, lastLoginTime);

            // Try to active the user's profile
            IPlayerProfile* pProfile = profileManager->ActivateProfile(userName, profDesc.name);

            // If it fails, try to recreate the profile
            if (!pProfile)
            {
                GameWarning("[GameProfiles]: Cannot activate profile '%s' for user '%s'. Trying to re-create.", profDesc.name, userName);

                IPlayerProfileManager::EProfileOperationResult profileResult;
                profileManager->CreateProfile(userName, profDesc.name, true, profileResult); // override if present!
                pProfile = profileManager->ActivateProfile(userName, profDesc.name);

                if (!pProfile)
                {
                    GameWarning("[GameProfiles]: Cannot activate profile '%s' for user '%s'.", profDesc.name, userName);
                    return;
                }
                else
                {
                    GameWarning("[GameProfiles]: Successfully re-created profile '%s' for user '%s'.", profDesc.name, userName);
                }
            }

            // Reset profile if forcing it
            const bool bResetProfile = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "ResetProfile") != 0;
            if (bResetProfile)
            {
                pProfile->Reset();
                gEnv->pCryPak->RemoveFile("@user@/game.cfg");
                CryLogAlways("[GameProfiles]: Successfully reset and activated profile '%s' for user '%s'", profDesc.name, userName);
            }

            CryLogAlways("[GameProfiles]: Successfully activated profile '%s' for user '%s'", profDesc.name, userName);

            pGameFramework->GetILevelSystem()->LoadRotation();

            if (bIsFirstTime || bResetProfile)
            {
                pProfile->LoadGamerProfileDefaults();
            }
        }
    }
}

AZ_DECLARE_MODULE_CLASS(UserLoginDefault_c9c25313197f489a8e9e8e6037fc62eb, UserLoginDefault::UserLoginDefaultGem)
