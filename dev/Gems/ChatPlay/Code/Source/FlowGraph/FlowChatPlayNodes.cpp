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
////////////////////////////////////////////////////////////////////////////
//
//   Twitch ChatPlay Flow Nodes
//
////////////////////////////////////////////////////////////////////////////

#include "ChatPlay_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <ChatPlay/ChatPlayTypes.h>
#include <ChatPlay/ChatPlayBus.h>

namespace ChatPlay
{

    ////////////////////////////////////////////////////////////////////////////
    // Is This Feature Available
    class CFlowNode_ChatPlayAvailable
        : public CFlowBaseNode < eNCT_Singleton >
    {
        enum InputPorts
        {
            InputPorts_Activate,
        };

        enum OutputPorts
        {
            OutputPorts_Available,
            OutputPorts_Unavailable,
        };

    public:

        explicit CFlowNode_ChatPlayAvailable(IFlowNode::SActivationInfo*)
        {
        }

        ~CFlowNode_ChatPlayAvailable() override
        {
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Activate to check the availability of ChatPlay.")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Available", _HELP("The ChatPlay feature is available.")),
                OutputPortConfig_Void("Unavailable", _HELP("The ChatPlay feature is not available.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Checks the availability of the ChatPlay feature.");
            config.SetCategory(EFLN_APPROVED);
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(pActInfo, InputPorts_Activate))
                {
#if defined(CONSOLE)
                    ActivateOutput(pActInfo, OutputPorts::OutputPorts_Unavailable, 1);
#else
                    ActivateOutput(pActInfo, OutputPorts::OutputPorts_Available, 1);
#endif // CONSOLE
                }
                break;
            }
            }
        }
    };


    ////////////////////////////////////////////////////////////////////////////
    // Connects to a specific Twitch Channel
    // Uses registered broadcaster's credentials if possible, else uses defaults
    // Will output any change in the state of connection
    class CFlowNode_ChatPlayChannel
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Channel,
            InputPorts_Connect,
            InputPorts_Disconnect,
        };

        enum OutputPorts
        {
            OutputPorts_Connected,
            OutputPorts_Connecting,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayChannel(IFlowNode::SActivationInfo*);

        ~CFlowNode_ChatPlayChannel() override;

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayChannel(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig<string>("Channel", _HELP("Twitch channel name")),
                InputPortConfig_Void("Connect", _HELP("Initiate connection. No effect if already connected or connecting.")),
                InputPortConfig_Void("Disconnect", _HELP("Initiate disconnection. No effect if already disconnected or disconnecting.")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<bool>("Connected", _HELP("Signalled when the connection state changes")),
                OutputPortConfig<bool>("Connecting", _HELP("Signalled when the connecting state changes")),
                OutputPortConfig<bool>("Error", _HELP("Signalled if an error has occurred. Reset when initiating a connection")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Connects to a Twitch channel to listen for chat commands");
            config.SetCategory(EFLN_APPROVED);
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;

    private:
        SActivationInfo m_actInfo;

        AZStd::string m_channelId;
        CallbackToken m_callbackToken;

        // Cache connection state flags so we can ensure edge-triggered behavior
        bool m_connectedState = false;
        bool m_connectingState = false;
        bool m_errorState = false;

        // Callback used with IChatChannel
        void OnConnectionStateChange(ConnectionState connectionState);

        // Free channel reference (potentially closing the connection)
        void ClearChatChannel();

        // Change the channel the node is currently tracking.
        // Automatically clears old channel reference.
        // Idempotent if the channel parameter is the same as the current channel.
        // Returns true if the channel has been updated.
        bool SetChatChannel(const AZStd::string& channelId);
    };

    CFlowNode_ChatPlayChannel::CFlowNode_ChatPlayChannel(IFlowNode::SActivationInfo*)
        : m_callbackToken(0)
    {
    }

    CFlowNode_ChatPlayChannel::~CFlowNode_ChatPlayChannel()
    {
        // Unset the current channel
        ClearChatChannel();
    }

    void CFlowNode_ChatPlayChannel::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (pActInfo)
        {
            // Needed later to allow callbacks to send signals
            m_actInfo = *pActInfo;
        }

        switch (event)
        {
        case eFE_Initialize:
        {
            // Check/set the channel when initializing the Node
            AZStd::string channelId = GetPortString(pActInfo, InputPorts_Channel).c_str();
            SetChatChannel(channelId);
        }
        break;

        case eFE_Uninitialize:
            break;

        case eFE_Activate:
            if (IsPortActive(pActInfo, InputPorts_Channel))
            {
                AZStd::string channelId = GetPortString(pActInfo, InputPorts_Channel).c_str();
                SetChatChannel(channelId);
            }

            if (IsPortActive(pActInfo, InputPorts_Connect) && !m_channelId.empty())
            {
                ChatPlayChannelRequestBus::Event(m_channelId, &ChatPlayChannelRequestBus::Events::Connect);
            }

            if (IsPortActive(pActInfo, InputPorts_Disconnect) && !m_channelId.empty())
            {
                ChatPlayChannelRequestBus::Event(m_channelId, &ChatPlayChannelRequestBus::Events::Disconnect);
            }

            break;
        }
    }

    void CFlowNode_ChatPlayChannel::ClearChatChannel()
    {
        ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::DestroyChannel, m_channelId);
        m_callbackToken = 0;
    }

    bool CFlowNode_ChatPlayChannel::SetChatChannel(const AZStd::string& _channelId)
    {
        AZStd::string channelId = _channelId;
        AZStd::to_lower(channelId.begin(), channelId.end());

        if (m_channelId == channelId)
        {
            // Do nothing if the channel has not changed
            return false;
        }

        ClearChatChannel();

        m_channelId = channelId;

        if (!ChatPlayUtils::IsValidChannelName(m_channelId))
        {
            // Don't do anything else if the channel name is not valid
            AZ_Warning("ChatPlay", false, "The entered Channel name %s is invalid.", m_channelId.c_str());
            return false;
        }

        bool channelExists = false;
        ChatPlayRequestBus::BroadcastResult(channelExists, &ChatPlayRequestBus::Events::CreateChannel, m_channelId);

        if (!channelExists)
        {
            // Could not create ChatChannel, ChatPlay might not be available
            ActivateOutput<bool>(&m_actInfo, OutputPorts::OutputPorts_Error, true);
            AZ_Warning("ChatPlay", false, "ChatPlay is disabled or not available on this platform.");
        }

        StateCallback cb = std::bind(&CFlowNode_ChatPlayChannel::OnConnectionStateChange, this, std::placeholders::_1);
        ChatPlayChannelRequestBus::EventResult(m_callbackToken, m_channelId, &ChatPlayChannelRequestBus::Events::RegisterConnectionStateChange, cb);

        ConnectionState state = ConnectionState::Disconnected;
        ChatPlayChannelRequestBus::EventResult(state, m_channelId, &ChatPlayChannelRequestBus::Events::GetConnectionState);
        OnConnectionStateChange(state);

        return true;
    }

    void CFlowNode_ChatPlayChannel::OnConnectionStateChange(ConnectionState connectionState)
    {
        bool connected = connectionState == ConnectionState::Connected;
        bool connecting = connectionState == ConnectionState::Connecting;
        bool error = connectionState == ConnectionState::Error;

        // logic to only activate outputs when their respective values change.

        if (m_connectedState != connected)
        {
            if (connected)
            {
                ActivateOutput<bool>(&m_actInfo, OutputPorts::OutputPorts_Connected, connected);
            }
            m_connectedState = connected;
        }

        if (m_connectingState != connecting)
        {
            if (connecting)
            {
                ActivateOutput<bool>(&m_actInfo, OutputPorts::OutputPorts_Connecting, connecting);
            }
            m_connectingState = connecting;

        }

        if (m_errorState != error)
        {
            if (error)
            {
                ActivateOutput<bool>(&m_actInfo, OutputPorts::OutputPorts_Error, error);
            }
            m_errorState = error;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Register keywords to count on a specific channel
    // Able to start and stop counting the keyword
    // Will output the count every time the keyword is said in channel and enabled
    class CFlowNode_ChatPlayKeyword
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Channel,
            InputPorts_Keyword,
            InputPorts_Start,
            InputPorts_Stop,
            InputPorts_Reset,
        };

        enum OutputPorts
        {
            OutputPorts_Signal,
            OutputPorts_Active,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayKeyword(IFlowNode::SActivationInfo*);

        ~CFlowNode_ChatPlayKeyword() override;

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayKeyword(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig<string>("Channel", _HELP("Twitch channel name")),
                InputPortConfig<string>("Keyword", _HELP("Chat command to listen for")),
                InputPortConfig_Void("Start", _HELP("Start counting and outputing the amount each time this keyword is said")),
                InputPortConfig_Void("Stop", _HELP("Stop counting and outputing the amount for this keyword")),
                InputPortConfig<int>("Reset", _HELP("Reset the count for this keyword")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig<int>("Signal", _HELP("Each time the keyword is typed in chat, this pin outputs the amount of times this keyword has been counted (including this time).")),
                OutputPortConfig<bool>("Active", _HELP("Outputs if this keyword is actively being counted or if it is stopped")),
                OutputPortConfig<bool>("Error", _HELP("Signalled if an error has occurred.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Listens for chat commands on a connected Twitch channel");
            config.SetCategory(EFLN_APPROVED);
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;

    private:
        SActivationInfo m_actInfo;

        AZStd::string m_channelId;
        AZStd::string m_keyword;
        CallbackToken m_callbackToken;
        bool m_started;
        int m_signalCount;

        // Callback triggered from IChatChannel indicating a keyword was found
        void OnKeywordSignal(const AZStd::string& match, const AZStd::string& username);

        // Change the channel the node is currently tracking.
        // Automatically clears old channel reference.
        // Idempotent if the channel parameter is the same as the current channel.
        // Returns true if the channel has been updated.
        bool SetChatChannel(const AZStd::string& channelId);

        // Change the keyword the node is currently tracking
        // Idempotent if the keyword parameter is the same as the current keyword.
        // Returns true if the keyword has been updated
        bool SetKeyword(const AZStd::string& keyword);

        // Register the current keyword with IChatChannel
        // Idempotent if already registered
        // No effect if the channel is currently unset
        void RegisterKeyword();

        // Unregister a registered keyword from IChatChannel
        // Idempotent if already unregistered
        // No effect if the channel is currently unset
        void UnregisterKeyword();

        // Make the node listen for its configured keyword
        // Idempotent if already started
        // Takes effect as soon as both channel and keyword are set
        // Remains in effect even if the channel or keyword change
        void StartKeyword();

        // Stops the node listening for the keyword
        // Idempotent if already stopped
        void StopKeyword();

        // Clear the current channel reference (potentially disconnecting the channel)
        void ClearChatChannel();
    };

    CFlowNode_ChatPlayKeyword::CFlowNode_ChatPlayKeyword(IFlowNode::SActivationInfo*)
        : m_callbackToken(0)
        , m_started(false)
        , m_signalCount(0)
    {
    }

    CFlowNode_ChatPlayKeyword::~CFlowNode_ChatPlayKeyword()
    {
        UnregisterKeyword();
    }

    void CFlowNode_ChatPlayKeyword::OnKeywordSignal(const AZStd::string& /*match*/, const AZStd::string& /*username*/)
    {
        if (m_started)
        {
            // assumption: one signal call per keyword matched.
            m_signalCount++;

            // activate the output pin with the updated value.
            ActivateOutput<int>(&m_actInfo, OutputPorts_Signal, m_signalCount);
        }
    }

    void CFlowNode_ChatPlayKeyword::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (pActInfo)
        {
            m_actInfo = *pActInfo;
        }

        switch (event)
        {
        case eFE_Initialize:
        {
            m_started = false;

            AZStd::string channelId = GetPortString(pActInfo, InputPorts_Channel).c_str();
            AZStd::string keyword = GetPortString(pActInfo, InputPorts_Keyword).c_str();

            // Fix for keyword nodes creating an empty channel when they initialise
            SetChatChannel(channelId);
            SetKeyword(keyword);

            m_signalCount = GetPortInt(pActInfo, InputPorts_Reset);

            // Set initial output pin states
            ActivateOutput<bool>(&m_actInfo, OutputPorts_Active, m_started);
            ActivateOutput<int>(&m_actInfo, OutputPorts_Signal, m_signalCount);
        }
        break;

        case eFE_Uninitialize:
            ClearChatChannel();
            m_started = false;
            break;

        case eFE_Activate:

            // Fix for keyword nodes creating an empty channel when they initialise
            // Check if the ChatChannel exists
            //bool channelExists = false;
            //ChatPlayRequestBus::BroadcastResult(channelExists, &ChatPlayRequestBus::Events::CreateChannel, m_channelId);

            if (IsPortActive(pActInfo, InputPorts_Channel))
            {
                AZStd::string channelId = GetPortString(pActInfo, InputPorts_Channel).c_str();
                SetChatChannel(channelId);
            }

            if (IsPortActive(pActInfo, InputPorts_Keyword))
            {
                AZStd::string keyword = GetPortString(pActInfo, InputPorts_Keyword).c_str();
                SetKeyword(keyword);
            }

            if (IsPortActive(pActInfo, InputPorts_Start))
            {
                StartKeyword();
            }

            if (IsPortActive(pActInfo, InputPorts_Stop))
            {
                StopKeyword();
            }

            if (IsPortActive(pActInfo, InputPorts_Reset))
            {
                // sets the keyword signal count to the value passed in.
                m_signalCount = GetPortInt(pActInfo, InputPorts_Reset);
                if (m_started)
                {
                    ActivateOutput<int>(&m_actInfo, OutputPorts_Signal, m_signalCount);
                }
            }

            break;
        }
    }

    bool CFlowNode_ChatPlayKeyword::SetChatChannel(const AZStd::string& _channelId)
    {
        AZStd::string channelId = _channelId;
        AZStd::to_lower(channelId.begin(), channelId.end());

        if (m_channelId == channelId)
        {
            // Do nothing if the channel has not changed
            return false;
        }

        ClearChatChannel();

        m_channelId = channelId;

        if (m_channelId.empty())
        {
            // Don't do anything else if the channel name is empty
            return false;
        }

        // If this node was previously started, stay started by registering with the new channel
        if (m_started)
        {
            RegisterKeyword();
        }

        return true;
    }

    bool CFlowNode_ChatPlayKeyword::SetKeyword(const AZStd::string& keyword)
    {
        if (m_keyword == keyword)
        {
            // Do nothing if the keyword has not changed
            return false;
        }

        UnregisterKeyword();

        // Reset the signal count and store the new keyword
        m_signalCount = GetPortInt(&m_actInfo, InputPorts_Reset);
        ActivateOutput<int>(&m_actInfo, OutputPorts_Signal, m_signalCount);

        // Set the new keyword
        m_keyword = keyword;
        if (m_started)
        {
            RegisterKeyword();
        }
        return true;
    }

    void CFlowNode_ChatPlayKeyword::RegisterKeyword()
    {
        if (!m_channelId.empty())
        {
            UnregisterKeyword();
            KeywordCallback cb = std::bind(&CFlowNode_ChatPlayKeyword::OnKeywordSignal, this, std::placeholders::_1, std::placeholders::_2);
            ChatPlayChannelRequestBus::EventResult(m_callbackToken, m_channelId, &ChatPlayChannelRequestBus::Events::RegisterKeyword, m_keyword, cb);
        }
    }

    void CFlowNode_ChatPlayKeyword::UnregisterKeyword()
    {
        if (!m_channelId.empty() && m_callbackToken)
        {
            ChatPlayChannelRequestBus::Event(m_channelId, &ChatPlayChannelRequestBus::Events::UnregisterKeyword, m_callbackToken);
            m_callbackToken = 0;
        }
    }

    void CFlowNode_ChatPlayKeyword::StartKeyword()
    {
        if (!m_started)
        {
            RegisterKeyword();
            m_started = true;

            ActivateOutput<bool>(&m_actInfo, OutputPorts_Active, true);
        }
    }

    void CFlowNode_ChatPlayKeyword::StopKeyword()
    {
        if (m_started)
        {
            UnregisterKeyword();
            m_started = false;

            ActivateOutput<bool>(&m_actInfo, OutputPorts_Active, false);
        }
    }

    void CFlowNode_ChatPlayKeyword::ClearChatChannel()
    {
        m_callbackToken = 0;
    }

    /******************************************************************************/
    // Send private message from one authenticated user to any other user
    // This could be used to whisper game invitations to viewers from a broadcaster
    // Will output success if whisper succeeded, error if failed
    class CFlowNode_ChatPlayWhisper
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Activate,
            InputPorts_Sender,
            InputPorts_Recipient,
            InputPorts_Message,
        };

        enum OutputPorts
        {
            OutputPorts_Success,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayWhisper(IFlowNode::SActivationInfo*)
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayWhisper(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Send whisper")),
                InputPortConfig<string>("Sender", _HELP("Registered user to send the message")),
                InputPortConfig<string>("Recipient", _HELP("Any user to receive the message")),
                InputPortConfig<string>("Message", _HELP("Message to send")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Success", _HELP("Signalled if the whipser was sent successfully")),
                OutputPortConfig<bool>("Error", _HELP("Signalled with true if an error occurred")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Send a whisper from one registered user to any other user.");
            config.SetCategory(EFLN_APPROVED);
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;

    private:
        SActivationInfo m_actInfo;

        // Callback from IChatPlay with the whisper result
        void OnCallback(WhisperResult result);
    };

    void CFlowNode_ChatPlayWhisper::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            m_actInfo = *pActInfo;
            break;

        case eFE_Activate:
            m_actInfo = *pActInfo;

            if (IsPortActive(pActInfo, InputPorts_Activate))
            {
                AZStd::string message = GetPortString(pActInfo, InputPorts_Message).c_str();
                AZStd::string sender = GetPortString(pActInfo, InputPorts_Sender).c_str();
                AZStd::string recipient = GetPortString(pActInfo, InputPorts_Recipient).c_str();

                const WhisperCallback& callback = std::bind(&CFlowNode_ChatPlayWhisper::OnCallback, this, std::placeholders::_1);

                // Send the whisper
                ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::SendWhisperWithCallback, sender, recipient, message, callback);
            }
        }
    }

    void CFlowNode_ChatPlayWhisper::OnCallback(WhisperResult result)
    {
        if (result == WhisperResult::Success)
        {
            ActivateOutput(&m_actInfo, OutputPorts_Success, true);
        }
        else
        {
            ActivateOutput<bool>(&m_actInfo, OutputPorts_Error, true);
        }
    }

    /******************************************************************************/
    // Disconnects from every currently connected ChatPlay Channel
    class CFlowNode_ChatPlayDisconnectAll
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Disconnect,
        };

    public:

        explicit CFlowNode_ChatPlayDisconnectAll(IFlowNode::SActivationInfo*)
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayDisconnectAll(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("DisconnectAll", _HELP("Initiate disconnection of all channels.")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Disconnect all ChatPlay Channels");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Initialize:
                break;

            case eFE_Activate:

                if (IsPortActive(pActInfo, InputPorts_Disconnect))
                {
                    ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::DisconnectAll);
                }

                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    class CFlowNode_ChatPlayRegisterCredentials
        : public CFlowBaseNode < eNCT_Singleton >
    {
        enum InputPorts
        {
            InputPorts_Activate,
            InputPorts_Username,
            InputPorts_OAuthToken,
        };

        enum OutputPorts
        {
            OutputPorts_Out,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayRegisterCredentials(IFlowNode::SActivationInfo*)
        {
        }

        ~CFlowNode_ChatPlayRegisterCredentials() override
        {
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Registers the OAuth token for the given username")),
                InputPortConfig<string>("Username", _HELP("Twitch username")),
                InputPortConfig<string>("OAuth_Token", _HELP("User's Twitch Chat OAuth token")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Out", _HELP("Signalled when done")),
                OutputPortConfig<bool>("Error", _HELP("Signalled if an error has occurred.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Registers Twitch IRC credentials for use when whispering");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Initialize:
                break;

            case eFE_Activate:
                if (IsPortActive(pActInfo, InputPorts_Activate))
                {
                    string username = GetPortString(pActInfo, InputPorts_Username);
                    string token = GetPortString(pActInfo, InputPorts_OAuthToken);

                    ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::RegisterCredentials, username.c_str(), token.c_str());
                }

                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    class CFlowNode_ChatPlayUnregisterCredentials
        : public CFlowBaseNode < eNCT_Singleton >
    {
        enum InputPorts
        {
            InputPorts_Activate,
            InputPorts_Username,
        };

        enum OutputPorts
        {
            OutputPorts_Out,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayUnregisterCredentials(IFlowNode::SActivationInfo*)
        {
        }

        ~CFlowNode_ChatPlayUnregisterCredentials() override
        {
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Unregisters the OAuth token for the given username")),
                InputPortConfig<string>("Username", _HELP("Twitch username")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Out", _HELP("Signalled when done")),
                OutputPortConfig<bool>("Error", _HELP("Signalled if an error has occurred.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Unregisters Twitch IRC credentials");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Initialize:
                break;

            case eFE_Activate:

                if (IsPortActive(pActInfo, InputPorts_Activate))
                {
                    string username = GetPortString(pActInfo, InputPorts_Username);

                    ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::UnregisterCredentials, username.c_str());
                }

                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    class CFlowNode_ChatPlayUnregisterAllCredentials
        : public CFlowBaseNode < eNCT_Instanced >
    {
        enum InputPorts
        {
            InputPorts_Activate,
        };

        enum OutputPorts
        {
            OutputPorts_Out,
            OutputPorts_Error,
        };

    public:

        explicit CFlowNode_ChatPlayUnregisterAllCredentials(IFlowNode::SActivationInfo*)
        {
        }

        ~CFlowNode_ChatPlayUnregisterAllCredentials() override
        {
        }

        IFlowNodePtr Clone(SActivationInfo* pActInfo)
        {
            return new CFlowNode_ChatPlayUnregisterAllCredentials(pActInfo);
        }

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate", _HELP("Unregister all credentials")),
                { 0 }
            };
            static const SOutputPortConfig outputs[] =
            {
                OutputPortConfig_Void("Out", _HELP("Signalled when done")),
                OutputPortConfig<bool>("Error", _HELP("Signalled if an error has occurred.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.pOutputPorts = outputs;
            config.sDescription = _HELP("Unregisters all Twitch IRC credentials");
            config.SetCategory(EFLN_APPROVED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
        {
            switch (event)
            {
            case eFE_Initialize:
                break;

            case eFE_Activate:

                if (IsPortActive(pActInfo, InputPorts_Activate))
                {
                    ChatPlayRequestBus::Broadcast(&ChatPlayRequestBus::Events::UnregisterAllCredentials);
                }

                break;
            }
        }

        void GetMemoryUsage(ICrySizer*) const override
        {
        }
    };

    REGISTER_FLOW_NODE("Twitch:ChatPlay:Available", CFlowNode_ChatPlayAvailable);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:Channel", CFlowNode_ChatPlayChannel);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:Keyword", CFlowNode_ChatPlayKeyword);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:Whisper", CFlowNode_ChatPlayWhisper);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:DisconnectAll", CFlowNode_ChatPlayDisconnectAll);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:RegisterCredentials", CFlowNode_ChatPlayRegisterCredentials);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:UnregisterCredentials", CFlowNode_ChatPlayUnregisterCredentials);
    REGISTER_FLOW_NODE("Twitch:ChatPlay:UnregisterAllCredentials", CFlowNode_ChatPlayUnregisterAllCredentials);

}