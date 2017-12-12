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
#undef GetUserName

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    /// Give a reason for wanting to save the profile
    enum class ProfileSaveReason
    {
        SavingGame = 0x01,
        OptionsUpdated = 0x02,
        All = 0xff
    };


    //////////////////////////////////////////////////////////////////////////
    /// This bus allows you to store information along with a player profile
    //////////////////////////////////////////////////////////////////////////
    class PlayerProfileRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        /// Gets the current profile name for the current user
        virtual const char* GetCurrentProfileForCurrentUser() = 0;

        //////////////////////////////////////////////////////////////////////////
        /// Gets the current profile name for the given user
        virtual const char* GetCurrentProfileName(const char* userName) = 0;

        //////////////////////////////////////////////////////////////////////////
        /// Gets the current user's name.  A user can have multiple profiles
        virtual const char* GetCurrentUserName() = 0;

        //////////////////////////////////////////////////////////////////////////
        /// Save the current user's profile for the given reason
        virtual bool RequestProfileSave(ProfileSaveReason reason) = 0;

        //////////////////////////////////////////////////////////////////////////
        /// Use this method to store reflected data, such as custom input bindings, into the current user's profile
        virtual bool StoreData(const char* dataKey, const void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* serializeContext = nullptr) = 0;

        //////////////////////////////////////////////////////////////////////////
        /// Use this method to retrieve reflected data, such as custom input bindings, from the current user's profile
        virtual bool RetrieveData(const char* dataKey, void** outClassPtr, AZ::SerializeContext* serializeContext = nullptr) = 0;
    };
    using PlayerProfileRequestBus = AZ::EBus<PlayerProfileRequests>;
} // namespace AZ