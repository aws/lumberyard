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

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(PlatformOS_Base_cpp, AZ_RESTRICTED_PLATFORM)
#endif

#include "PlatformOS_Base.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/conversions_winrt.h>

#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

#include <GridMate/GridMate.h>
#include <GridMate/Online/UserServiceTypes.h>

namespace
{
    IPlatformOS::SPlatformEvent::ESignInState SignInStateGridMateToLumberyard(int state)
    {
        switch (state)
        {
        case GridMate::OLS_NotSignedIn:
            return IPlatformOS::SPlatformEvent::eSIS_NotSignedIn;

        case GridMate::OLS_SignedInOffline:
            return IPlatformOS::SPlatformEvent::eSIS_SignedInLocally;

        case GridMate::OLS_SignedInOnline:
            return IPlatformOS::SPlatformEvent::eSIS_SignedInLive;

        case GridMate::OLS_SigninUnknown:
            return IPlatformOS::SPlatformEvent::eSIS_NotSignedIn;

        default:
            AZ_Assert(false, "Invalid sign in state.");
        }

        return IPlatformOS::SPlatformEvent::eSIS_NotSignedIn;
    }
}

PlatformOS_Base::PlatformOS_Base()
    : m_pendingUserSignIn(Unknown_User)
{
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

PlatformOS_Base::~PlatformOS_Base()
{
    if (gEnv && gEnv->pSystem) // PlatformOS is created after System and gEnv, however it's destroyed after gEnv & System as well, so we might not have them here
    {
        gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    }
}

bool PlatformOS_Base::GetUserProfilePreference(unsigned int user, EUserProfilePreference ePreference, SUserProfileVariant& outResult) const
{
    (void)user;
    (void)ePreference;
    (void)outResult;
    return false;
}

bool PlatformOS_Base::UserSelectStorageDevice(unsigned int userIndex, bool bForceUI)
{
    (void)userIndex;
    (void)bForceUI;
    return true;
}

unsigned int PlatformOS_Base::UserGetMaximumSignedInUsers() const
{
    return 1;
}

bool PlatformOS_Base::UserIsSignedIn(unsigned int userIndex) const
{
    return userIndex < UserGetMaximumSignedInUsers();
}

bool PlatformOS_Base::UserIsSignedIn(const IPlatformOS::TUserName& userName, unsigned int& outUserIndex) const
{
    TUserName name;
    UserGetName(0, name);
    if (strcmp(name.c_str(), userName.c_str()) == 0)
    {
        outUserIndex = 0;
        return true;
    }

    return false;
}

void PlatformOS_Base::TrySignIn(unsigned int userId)
{
    if (UserIsSignedIn(userId))
    {
        IPlatformOS::SPlatformEvent event(userId);
        event.m_eEventType = SPlatformEvent::eET_SignIn;
        event.m_uParams.m_signIn.m_previousSignedInState = SPlatformEvent::eSIS_NotSignedIn;
        event.m_uParams.m_signIn.m_signedInState = IPlatformOS::SPlatformEvent::eSIS_SignedInLive;
        NotifyListeners(event);
    }
}

bool PlatformOS_Base::UserDoSignIn(unsigned int numUsersRequested, unsigned int controllerIndex)
{
    LOADING_TIME_PROFILE_SECTION;
    (void)numUsersRequested;

    TrySignIn(controllerIndex);
    return true;
}

void PlatformOS_Base::UserSignOut(unsigned int userIndex)
{
    (void)userIndex;
}

bool PlatformOS_Base::UserGetName(unsigned int userIndex, IPlatformOS::TUserName& outName) const
{
    if (userIndex >= UserGetMaximumSignedInUsers())
    {
        return false;
    }

    outName = "DefaultUser";
    return true;
}

int PlatformOS_Base::GetFirstSignedInUser() const
{
    return 0;
}

bool PlatformOS_Base::UserGetOnlineName(unsigned int userIndex, IPlatformOS::TUserName& outName) const
{
    return UserGetName(userIndex, outName);
}


void PlatformOS_Base::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT:
        AZ_Assert(gEnv->pNetwork, "Network is not initialized");
        AZ_Assert(gEnv->pNetwork->GetGridMate(), "GridMate is not initialized");

        if (m_pendingUserSignIn != Unknown_User)
        {
            TrySignIn(m_pendingUserSignIn);
            m_pendingUserSignIn = Unknown_User;
        }
        break;
    }
}
