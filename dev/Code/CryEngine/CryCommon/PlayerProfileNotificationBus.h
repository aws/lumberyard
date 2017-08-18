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
#pragma once
#include "AzCore/EBus/EBus.h"

namespace AZ
{
    enum class ProfileSaveLoadError
    {
        SystemNotInitialized,
        UserNotLoggedIn,
        ProfileNotActive,
        AnotherOperationInProgress,
        Unknown,
    };
    //////////////////////////////////////////////////////////////////////////
    /// This bus allows you to respond to game saving and loading
    //////////////////////////////////////////////////////////////////////////
    class PlayerProfileNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        /// This event is sent before anything is saved
        virtual void OnProfileSaving() {}

        //////////////////////////////////////////////////////////////////////////
        /// This event is sent when saving fails
        virtual void OnProfileSaveFailure(const ProfileSaveLoadError&) {}

        //////////////////////////////////////////////////////////////////////////
        /// This event is sent after the game has been saved
        virtual void OnProfileSaved() {}

        //////////////////////////////////////////////////////////////////////////
        /// This event is sent just before a saved game is loaded
        virtual void OnProfileLoading() {}

        //////////////////////////////////////////////////////////////////////////
        /// This event is sent when saving fails
        virtual void OnProfileLoadFailure(const ProfileSaveLoadError&) {}

        //////////////////////////////////////////////////////////////////////////
        /// This event is sent once a saved game is loaded
        virtual void OnProfileLoaded() {}
    };
    using PlayerProfileNotificationBus = AZ::EBus<PlayerProfileNotifications>;
} // namespace AZ