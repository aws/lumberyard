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
#include <AzCore/Interface/Interface.h>

#include <TwitchChatPlaySystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <TwitchChatPlay/TwitchChatPlayNotificationsBus.h>
#include <AzCore/Component/TickBus.h>

namespace TwitchChatPlay
{
    class TwitchChatPlayNotificationBusHandler
        : public TwitchChatPlayNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(TwitchChatPlayNotificationBusHandler, "{DB2A446B-251F-4C01-BB4D-4DC86146D416}", AZ::SystemAllocator
            , OnTwitchChatPlayMessages
            , OnTwitchChatPlayFormattedMessages
        );

        void OnTwitchChatPlayMessages(const AZStd::string_view message) override
        {
            Call(FN_OnTwitchChatPlayMessages, message);
        }

        void OnTwitchChatPlayFormattedMessages(const AZStd::string_view username, const AZStd::string_view message) override
        {
            Call(FN_OnTwitchChatPlayFormattedMessages, username, message);
        }
    };

    void TwitchChatPlaySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TwitchChatPlaySystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TwitchChatPlaySystemComponent>("TwitchChatPlay", "Enables Games To Connect With Twitch IRC")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<TwitchChatPlayNotificationBus>("TwitchChatPlayNotificationBus")
                ->Attribute(AZ::Edit::Attributes::Category, "TwitchChatPlay")
                ->Handler<TwitchChatPlayNotificationBusHandler>()
                ;

            behaviorContext->EBus<TwitchChatPlayRequestBus>("TwitchChatPlayRequestBus")
                ->Attribute(AZ::Edit::Attributes::Category, "TwitchChatPlay")
                ->Event("ConnectToChatPlay", &TwitchChatPlayRequestBus::Events::ConnectToChatPlay
                    , { { {  "Oauth Token" , "The Oauth token for the respective username" } , { "Username" , "The username for the bot account" } } })
                ->Event("JoinChannel", &TwitchChatPlayRequestBus::Events::JoinChannel
                    , { { { "Channel name" , "The name of the channel to connect to" } } })
                ->Event("LeaveChannel", &TwitchChatPlayRequestBus::Events::LeaveChannel)
                ->Event("SendChannelMessage", &TwitchChatPlayRequestBus::Events::SendChannelMessage)
                ->Event("SendWhisperMessage", &TwitchChatPlayRequestBus::Events::SendWhisperMessage)
                ->Event("SetHandlerForKeyword", &TwitchChatPlayRequestBus::Events::SetKeywordWithDefaultHandler
                    , { { { "Keyword" , "The keyword to search for" } } })
                ->Event("SetHandlerForKeywordToReturnFormattedResponse", &TwitchChatPlayRequestBus::Events::SetKeywordWithFormattedHandler
                    , { { { "Keyword" , "The keyword to search for" } } })
                ->Event("RemoveKeyword", &TwitchChatPlayRequestBus::Events::RemoveKeyword)
                ->Event("ActivateKeywordMatching", &TwitchChatPlayRequestBus::Events::ActivateKeywordMatching)
                ->Event("UseTwitchPattern", &TwitchChatPlayRequestBus::Events::UseTwitchPattern
                    , "Turnon twitch pattern matching, will check only the first word of message for keyword, recommended format '!<keyword>'")
                ;
        }
    }

    void TwitchChatPlaySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TwitchChatPlayService"));
    }

    void TwitchChatPlaySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TwitchChatPlayService"));
    }

    void TwitchChatPlaySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("WebsocketsService"));
    }

    void TwitchChatPlaySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TwitchChatPlaySystemComponent::Init()
    {
        m_pingCheck = AZStd::string("PING :") + twitchTMI + "\r\n";
        m_pongResponse = AZStd::string("PONG :") + twitchTMI;
    }

    void TwitchChatPlaySystemComponent::Activate()
    {
        AZ::Interface<ITwitchChatPlayInterface>::Register(this);
        TwitchChatPlayRequestBus::Handler::BusConnect();
    }

    void TwitchChatPlaySystemComponent::Deactivate()
    {
        TwitchChatPlayRequestBus::Handler::BusDisconnect();
        AZ::Interface<ITwitchChatPlayInterface>::Unregister(this);
    }

    void TwitchChatPlaySystemComponent::ConnectToChatPlay(const AZStd::string& oAuth, const AZStd::string& userName)
    {
        //create a websocket and setup response handler
        Websockets::OnMessage messageFunc = [this](const AZStd::string_view message)
        {
            HandleResponses(message);
        };

        Websockets::WebsocketsRequestBus::BroadcastResult(m_websocket, &Websockets::WebsocketsRequests::CreateClient,
            twitchSSLWebsocket,
            messageFunc,
            Websockets::IWebsocketRequests::Secure
        );

        m_userName = userName;
        m_userNamePreface = ConstructUserMessagePreface();

        SetTime();
        while (!m_websocket->IsConnectionOpen() && !HasTimedOut())
        {
            AZStd::this_thread::sleep_for(s_milliSecondDelay);
        }

        if (!m_websocket->IsConnectionOpen())
        {
            AZ_Error("Twitch Chatplay", false, "Websocket connection failed to open.")
            return;
        }

        m_websocket->SendWebsocketMessage("PASS oauth:" + oAuth);
        m_websocket->SendWebsocketMessage("NICK " + m_userName);
    }

    void TwitchChatPlaySystemComponent::JoinChannel(const AZStd::string& channel)
    {
        if (!m_currentChannel.empty())
        {
            LeaveChannel();
        }

        m_currentChannel = channel;
        m_websocket->SendWebsocketMessage(ConstructChannelMessage("JOIN"));
    }

    void TwitchChatPlaySystemComponent::LeaveChannel()
    {
        if (m_currentChannel.empty())
        {
            AZ_Warning("Twitch Chatplay Gem", false, "You are not currently in a channel!");
            return;
        }

        m_websocket->SendWebsocketMessage(ConstructChannelMessage("PART"));
        m_currentChannel = "";
    }

    void TwitchChatPlaySystemComponent::SendChannelMessage(const AZStd::string& message)
    {
        const AZStd::string channelMessagePreface = "PRIVMSG #" + m_currentChannel + " :";
        m_websocket->SendWebsocketMessage(channelMessagePreface + message);
    }

    void TwitchChatPlaySystemComponent::SendWhisperMessage(const AZStd::string& message, const AZStd::string& userName)
    {
        const AZStd::string channelMessagePreface = "PRIVMSG #" + m_currentChannel + " :/w ";
        m_websocket->SendWebsocketMessage(channelMessagePreface + userName + " " + message);
    }

    void TwitchChatPlaySystemComponent::SetKeywordWithDefaultHandler(const AZStd::string& keyword)
    {
        m_keywords[keyword] = [](const AZStd::string_view message)
        {
            AZStd::string stringMessage = message;
            AZ::TickBus::QueueFunction(
                [stringMessage]()
            {
                TwitchChatPlayNotificationBus::Broadcast(&TwitchChatPlayNotifications::OnTwitchChatPlayMessages, stringMessage);
            });
        };
    }

    void TwitchChatPlaySystemComponent::SetKeywordWithFormattedHandler(const AZStd::string& keyword)
    {
        m_keywords[keyword] = [](const AZStd::string_view message)
        {
            const size_t messagePos = message.find_first_of(':');  //Find the break between username and message

            AZStd::string userSubstr = message.substr(0, messagePos);

            AZStd::string messageSubstr = message.substr(messagePos + 1);

            AZ::TickBus::QueueFunction(
                [userSubstr, messageSubstr]()
            {
                TwitchChatPlayNotificationBus::Broadcast(&TwitchChatPlayNotifications::OnTwitchChatPlayFormattedMessages,
                    userSubstr, messageSubstr);
            });
        };
    }


    void TwitchChatPlaySystemComponent::SetKeywordWithSpecificHandler(const AZStd::string& keyword, KeywordMessage messageFunc)
    {
        m_keywords[keyword] = messageFunc;
    }

    void TwitchChatPlaySystemComponent::RemoveKeyword(const AZStd::string& keyword)
    {
        if (m_keywords.count(keyword))
        {
            m_keywords.erase(keyword);
        }
    }

    void TwitchChatPlaySystemComponent::ActivateKeywordMatching(bool activate)
    {
        m_keywordMatching = activate;
    }

    void TwitchChatPlaySystemComponent::UseTwitchPattern(const bool usePattern)
    {
        m_useTwitchPattern = usePattern;
    }

    void TwitchChatPlaySystemComponent::HandleResponses(const AZStd::string_view message)
    {
        if (message.compare(m_pingCheck) == 0)
        {
            PongResponse();
            return;
        }

        if (m_keywordMatching)
        {
            IdentifyMessageKeywords(message);
        }

        else
        {
            //if keywords aren't used, provide no stripping of data and send message directly
            AZStd::string stringMessage = message;
            AZ::TickBus::QueueFunction(
                [stringMessage]()
            {
                TwitchChatPlayNotificationBus::Broadcast(&TwitchChatPlayNotifications::OnTwitchChatPlayMessages, stringMessage);
            });
        }
    }

    void TwitchChatPlaySystemComponent::PongResponse() const
    {
        m_websocket->SendWebsocketMessage(m_pongResponse);
    }

    void TwitchChatPlaySystemComponent::IdentifyMessageKeywords(const AZStd::string_view message)
    {
        if (m_useTwitchPattern)
        {
            TwitchFormatMatching(message);
        }

        else
        {
            //much, much slower because it has to search the entire message for the keyword
            DefaultFormatMatching(message);
        }
    }

    void TwitchChatPlaySystemComponent::DefaultFormatMatching(const AZStd::string_view message)
    {
        //get the username
        size_t userPos = message.find_first_of('@');
        if (userPos == AZStd::string::npos)
        {
            return;
        }

        AZStd::string_view userSubstr = message.substr(userPos);

        size_t channelPos = message.find_first_of('#');
        if (channelPos == AZStd::string::npos)
        {
            return;
        }


        AZStd::string_view channelSubstr = message.substr(channelPos);

        size_t messagePos = channelSubstr.find_first_of(':');
        if (messagePos == AZStd::string::npos)
        {
            return;
        }

        AZStd::string_view messageBody;

        messageBody = channelSubstr.substr(messagePos + 1);

        while (!messageBody.empty())
        {
            size_t nextSpace = channelSubstr.find_first_of(" ", messagePos + 1);
            AZStd::string_view keyword;

            if (nextSpace != AZStd::string_view::npos)
            {
                keyword = channelSubstr.substr(messagePos + 1, nextSpace - (messagePos + 1));
            }

            else
            {
                size_t endPos = channelSubstr.find_first_of("\r", messagePos + 1);
                keyword = channelSubstr.substr(messagePos + 1, endPos - (messagePos + 1));
            }
            
            if (m_keywords.count(keyword))
            {
                m_keywords[keyword](userSubstr);  //sends only the message from the point 'username'
                return;  //only return first keyword
            }

            if (messageBody.length() < (nextSpace + 1) || nextSpace == AZStd::string_view::npos)
            {
                return;
            }

            messageBody = messageBody.substr(nextSpace + 1);
        }
    }

    void TwitchChatPlaySystemComponent::TwitchFormatMatching(const AZStd::string_view message)
    {
        //get the username
        size_t userPos = message.find_first_of('@');
        if (userPos == AZStd::string::npos)
        {
            return;
        }

        AZStd::string_view userSubstr = message.substr(userPos);


        size_t channelPos = message.find_first_of('#');
        if (channelPos == AZStd::string::npos)
        {
            return;
        }


        AZStd::string_view channelSubstr = message.substr(channelPos);
        size_t messagePos = channelSubstr.find_first_of(':');
        if (messagePos == AZStd::string::npos)
        {
            return;
        }
            

        AZStd::string_view keyword;

        size_t valuePos = channelSubstr.find_first_of(" ", messagePos + 1);

        if(valuePos != AZStd::string_view::npos)
        {
            keyword = channelSubstr.substr(messagePos + 1, valuePos - (messagePos + 1));
        }

        else
        {
            size_t endPos = channelSubstr.find_first_of("\r", messagePos + 1);
            keyword = channelSubstr.substr(messagePos + 1, endPos - (messagePos + 1));
        }

        if (m_keywords.count(keyword))
        {
            m_keywords[keyword](userSubstr);  //sends only the message from the point 'username'
        }
    }

    AZStd::string TwitchChatPlaySystemComponent::ConstructUserMessagePreface() const
    {
        return AZStd::string(":" + m_userName + "!" + m_userName + "@" + m_userName
            + twitchTMI);
    }

    AZStd::string TwitchChatPlaySystemComponent::ConstructChannelMessage(const AZStd::string& messageType) const
    {
        return messageType + " #" + m_currentChannel;
    }

    void TwitchChatPlaySystemComponent::SetTime()
    {
        m_timeOut = AZStd::chrono::seconds(2);
        m_time = AZStd::chrono::high_resolution_clock::now();
    }

    bool TwitchChatPlaySystemComponent::HasTimedOut() const
    {
        AZStd::chrono::time_point<> time = AZStd::chrono::high_resolution_clock::now();
        return ((time - m_time) > m_timeOut);
    }
}
