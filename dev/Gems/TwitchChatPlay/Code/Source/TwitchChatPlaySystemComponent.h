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

#include <AzCore/Component/Component.h>

#include <TwitchChatPlay/TwitchChatPlayBus.h>

#include <Websockets/WebsocketsBus.h>

namespace TwitchChatPlay
{
    constexpr const char* twitchSSLWebsocket = "wss://irc-ws.chat.twitch.tv:443";
    constexpr const char* twitchTMI = "tmi.twitch.tv";
    const static AZStd::chrono::milliseconds s_milliSecondDelay(50);

    class TwitchChatPlaySystemComponent
        : public AZ::Component
        , protected TwitchChatPlayRequestBus::Handler
    {
    public:
        AZ_COMPONENT(TwitchChatPlaySystemComponent, "{A01B7D9A-6C6A-4FE2-9355-F4F1EA36A698}", ITwitchChatPlayInterface);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //--Bus calls
        void ConnectToChatPlay(const AZStd::string& oAuth, const AZStd::string& userName) override;
        void JoinChannel(const AZStd::string& channel) override;
        void LeaveChannel() override;

        void SendChannelMessage(const AZStd::string& message) override;

        //Only works when users are subscribed to the bot
        void SendWhisperMessage(const AZStd::string& message, const AZStd::string& userName) override;

        //--Keywords
        void SetKeywordWithDefaultHandler(const AZStd::string& keyword) override;
        void SetKeywordWithFormattedHandler(const AZStd::string& keyword) override;
        void SetKeywordWithSpecificHandler(const AZStd::string& keyword, KeywordMessage messageFunc) override;
        void RemoveKeyword(const AZStd::string& keyword) override;
        void ActivateKeywordMatching(const bool activate) override;
        void UseTwitchPattern(const bool usePattern) override;  //Assumes Twitch pattern and adjust keyword matching accordingly

    private:
        //--Internal functions
        void HandleResponses(const AZStd::string_view message);
        void PongResponse() const;
        void IdentifyMessageKeywords(const AZStd::string_view message);
        void DefaultFormatMatching(const AZStd::string_view message);
        void TwitchFormatMatching(const AZStd::string_view message);

        AZStd::string ConstructUserMessagePreface() const;
        AZStd::string ConstructChannelMessage(const AZStd::string& messageType) const;

        //--Timeout
        void SetTime();
        bool HasTimedOut() const;

        AZStd::unique_ptr<Websockets::IWebsocketClient> m_websocket;

        //--Twitch info
        AZStd::string m_userName;
        AZStd::string m_currentChannel = "";

        //Stored values for later use
        AZStd::string m_pingCheck;
        AZStd::string m_pongResponse;
        AZStd::string m_userNamePreface;

        //Keyword list active keywords
        AZStd::unordered_map<AZStd::string, KeywordMessage> m_keywords;
        bool m_keywordMatching = false;
        bool m_useTwitchPattern = true;

        //Timeouts
        AZStd::chrono::time_point<> m_time;
        AZStd::chrono::microseconds m_timeOut;
    };
}
