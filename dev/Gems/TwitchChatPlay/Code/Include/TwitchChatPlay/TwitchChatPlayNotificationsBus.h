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

#include <AzCore/RTTI/BehaviorContext.h>

namespace TwitchChatPlay
{
    //!Notification that a twitch message has been received.
    class TwitchChatPlayNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //!Generic messages with no requested formatting.
        virtual void OnTwitchChatPlayMessages(const AZStd::string_view message) = 0;

        //!Messages that are being specifically handled with additional input provided by the user.
        virtual void OnTwitchChatPlayFormattedMessages(const AZStd::string_view username, const AZStd::string_view message) = 0;
    };
    using TwitchChatPlayNotificationBus = AZ::EBus<TwitchChatPlayNotifications>;
} // namespace TwitchChatPlay
