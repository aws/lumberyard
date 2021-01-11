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

#include <AzCore/EBus/EBus.h>

namespace TwitchChatPlay
{
    //!The handler function that the user wishes to provide for a specific keyword or keywords.
    using KeywordMessage = AZStd::function<void(const AZStd::string_view)>;

    //!Provides an interface to connect to chatplay directly.
    class ITwitchChatPlayInterface
    {
    public:
        AZ_RTTI(ITwitchChatPlayInterface, "{36FFA56A-2D55-4C09-99EE-0205DC0A175E}");

        //!Connect to twitch chat with current user, using provided OAuth.
        //!Stores username, not OAuth token.
        virtual void ConnectToChatPlay(const AZStd::string& oAuth, const AZStd::string& username) = 0;

        //!Stores and joins a channel.
        virtual void JoinChannel(const AZStd::string& channel) = 0;
        //!Leaves current channel.
        virtual void LeaveChannel() = 0;  

        virtual void SendChannelMessage(const AZStd::string& message) = 0;
        virtual void SendWhisperMessage(const AZStd::string& message, const AZStd::string& userName) = 0;

        //!For identifying game specific keywords with the results being sent to the generic handler.
        //!Will send message through default notification bus, and automatically pass them to script canvas.
        virtual void SetKeywordWithDefaultHandler(const AZStd::string& keyword) = 0;

        //!For identifying game specific keywords with the results modified.
        //!Sends message through default bus, but modifies formatting.
        virtual void SetKeywordWithFormattedHandler(const AZStd::string& keyword) = 0;

        //!For identifying game specific keywords through a user provided handler.
        virtual void SetKeywordWithSpecificHandler(const AZStd::string& keyword, KeywordMessage messageFunc) = 0;

        //!Removes the keyword from the mapping.
        virtual void RemoveKeyword(const AZStd::string& keyword) = 0;

        //!Turns all keyword matching on or off.
        //!To disable specific keywords, they will need to be removed.
        virtual void ActivateKeywordMatching(const bool activate) = 0;

        //!A common Twitch pattern is to provide keywords in the format: '!<keyword> <value>'
        //!For example, '!vote yes'.
        //!It's recommended to follow this format for user familiarity and overall usability - including the '!' at the front of
        //!your keywords.
        virtual void UseTwitchPattern(const bool usePattern) = 0;

    };

    //!Bus inputs for twitch ChatPlay.
    class TwitchChatPlayRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using TwitchChatPlayRequestBus = AZ::EBus<ITwitchChatPlayInterface, TwitchChatPlayRequests>;
    
} // namespace TwitchChatPlay
