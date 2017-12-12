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
#ifndef PLATFORMOS_BASE_H
#define PLATFORMOS_BASE_H

#include "IPlatformOS.h"
#include <CryListenerSet.h>
#include <IGameFramework.h>

class PlatformOS_Base
    : public IPlatformOS
    , public ISystemEventListener
{
public:
    PlatformOS_Base();
    ~PlatformOS_Base();

    unsigned int UserGetMaximumSignedInUsers() const override;
    bool UserIsSignedIn(unsigned int userIndex) const override;
    bool UserIsSignedIn(const IPlatformOS::TUserName& userName, unsigned int& outUserIndex) const override;
    bool UserDoSignIn(unsigned int numUsersRequested, unsigned int controllerIndex) override;
    void UserSignOut(unsigned int user) override;
    int GetFirstSignedInUser() const override;
    unsigned int UserGetPlayerIndex(const char* userName) const override { return 0; }
    bool UserGetName(unsigned int userIndex, IPlatformOS::TUserName& outName) const override;
    bool UserGetOnlineName(unsigned int userIndex, IPlatformOS::TUserName& outName) const override;
    bool GetUserProfilePreference(unsigned int user, EUserProfilePreference ePreference, SUserProfileVariant& outResult) const override;
    bool UserSelectStorageDevice(unsigned int userIndex, bool bForceUI = false) override;
    bool AZ_DEPRECATED(KeyboardStart(unsigned int inUserIndex, unsigned int flags, const char* title, const char* initialInput, int maxInputLength, IVirtualKeyboardEvents* pInCallback),
        "IPlatformOS::KeyboardStart has been deprecated, use InputTextEntryRequestBus::TextEntryStart instead") override;
    bool AZ_DEPRECATED(KeyboardIsRunning(), "IPlatformOS::KeyboardIsRunning has been deprecated, use InputTextEntryRequestBus::HasTextEntryStarted instead") override;
    bool AZ_DEPRECATED(KeyboardCancel(), "IPlatformOS::KeyboardCancel has been deprecated, use InputTextEntryRequestBus::TextEntryStop instead") override;

protected:
    virtual void TrySignIn(unsigned int userId);

    // ISystemEventListener
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    unsigned int m_pendingUserSignIn;
};

#endif // PLATFORMOS_BASE_H
