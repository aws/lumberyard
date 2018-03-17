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
#include <Multiplayer_precompiled.h>
#include <ISystem.h>
#include <Multiplayer/FlowMultiplayerNodes.h>
#include <GridMate/GridMate.h>
#include <GridMate/NetworkGridMate.h>

namespace
{
    // EVENTS
    const char* g_multiplayerOnConnectedNodePath = "Multiplayer:Events:OnConnected";
    const char* g_multiplayerOnDisconnectedNodePath = "Multiplayer:Events:OnDisconnected";
    const char* g_multiplayerOnPlayerConnectedNodePath = "Multiplayer:Events:OnPlayerConnected";
    const char* g_multiplayerOnPlayerDisconnectedNodePath = "Multiplayer:Events:OnPlayerDisconnected";
    const char* g_multiplayerOnLocalPlayerReadyNodePath = "Multiplayer:Events:OnLocalPlayerReady";
    const char* g_multiplayerOnPlayerReadyNodePath = "Multiplayer:Events:OnPlayerReady";
}

/*!
* FlowGraph node base class for listening to GridMate session events.
*/
class FlowNode_MultiplayerSessionEvent
    : public CFlowBaseNode<eNCT_Instanced>
    , public GridMate::SessionEventBus::Handler
{
public:
    FlowNode_MultiplayerSessionEvent(SActivationInfo* activationInfo, string description)
        : CFlowBaseNode()
        , m_activationInfo(*activationInfo)
        , m_description(description)
    {
    }

    ~FlowNode_MultiplayerSessionEvent() override
    {
        UnRegisterGridMateEventHandler();
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Initialize)
        {
            RegisterGridMateEventHandler();
        }
    }

    void RegisterGridMateEventHandler()
    {
        if (false == GridMate::SessionEventBus::Handler::BusIsConnected())
        {
            if (gEnv->pNetwork && gEnv->pNetwork->GetGridMate())
            {
                GridMate::SessionEventBus::Handler::BusConnect(gEnv->pNetwork->GetGridMate());
            }
        }
    }

    void UnRegisterGridMateEventHandler()
    {
        if (true == GridMate::SessionEventBus::Handler::BusIsConnected())
        {
            if (gEnv->pNetwork && gEnv->pNetwork->GetGridMate())
            {
                GridMate::SessionEventBus::Handler::BusDisconnect(gEnv->pNetwork->GetGridMate());
            }
        }
    }

protected:

    GridMate::GridSession* GetSession()
    {
        if (gEnv->pNetwork)
        {
            GridMate::Network* gridmate = static_cast<GridMate::Network*>(gEnv->pNetwork);
            if (gridmate)
            {
                return gridmate->GetCurrentSession();
            }
        }

        return nullptr;
    }


    SActivationInfo m_activationInfo;
    string m_description;
};


/*!
* FlowGraph node base class for events with the common Activate input and True / False output
*/
class FlowNode_MultiplayerSessionBoolEvent
    : public FlowNode_MultiplayerSessionEvent
{
public:
    FlowNode_MultiplayerSessionBoolEvent(SActivationInfo* activationInfo, string description)
        : FlowNode_MultiplayerSessionEvent(activationInfo, description)
        , m_activateHelp("Forces an update of the output")
        , m_falseHelp("")
        , m_trueHelp("")
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP(m_activateHelp)),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("True", _HELP(m_trueHelp)),
            OutputPortConfig_Void("False", _HELP(m_falseHelp)),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP(m_description);
        config.SetCategory(EFLN_APPROVED);
    }

protected:
    string m_activateHelp;
    string m_trueHelp;
    string m_falseHelp;

    enum InputPorts
    {
        InputPortActivate = 0,
    };

    enum OutputPorts
    {
        OutputPortTrue = 0,
        OutputPortFalse,
    };
};


/*!
* FlowGraph node that activates when a new session is joined (connected).
* NOTE: though the network session may be connected, the local player's actor / entity may not have been created.
* Use the FlowNode_OnLocalPlayerReady if you require the local player's actor exist.
*/
class FlowNode_OnConnected
    : public FlowNode_MultiplayerSessionBoolEvent
{
public:
    FlowNode_OnConnected(SActivationInfo* activationInfo)
        : FlowNode_MultiplayerSessionBoolEvent(activationInfo, "Indicates if the multiplayer session is connected")
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_OnConnected(activationInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        FlowNode_MultiplayerSessionBoolEvent::ProcessEvent(event, activationInfo);

        if (event == eFE_Activate)
        {
            if (IsPortActive(activationInfo, InputPortActivate))
            {
                int port = OutputPortFalse;
                GridMate::GridSession* session = GetSession();
                if (session && session->IsReady())
                {
                    port = OutputPortTrue;
                }

                ActivateOutput(&m_activationInfo, port, true);
            }
        }
    }

    void OnSessionJoined(GridMate::GridSession* session) override
    {
        ActivateOutput(&m_activationInfo, OutputPortTrue, true);
    }
};


/*!
* FlowGraph node that activates when GridMate disconnects from the current session.
*/
class FlowNode_OnDisconnected
    : public FlowNode_MultiplayerSessionBoolEvent
{
public:
    FlowNode_OnDisconnected(SActivationInfo* activationInfo)
        : FlowNode_MultiplayerSessionBoolEvent(activationInfo, "Indicates if the multiplayer session is disconnected")
    {
    }

    void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_OnDisconnected(activationInfo);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        FlowNode_MultiplayerSessionBoolEvent::ProcessEvent(event, activationInfo);

        if (event == eFE_Activate)
        {
            if (IsPortActive(activationInfo, InputPortActivate))
            {
                int port = OutputPortTrue;
                GridMate::GridSession* session = GetSession();
                if (session && session->IsReady())
                {
                    port = OutputPortFalse;
                }

                ActivateOutput(&m_activationInfo, port, true);
            }
        }
    }

    void OnSessionEnd(GridMate::GridSession* session) override
    {
        ActivateOutput(&m_activationInfo, OutputPortTrue, true);
    }
};

/*!
* FlowGraph node that activates when a new player connects.
* NOTE: the actor / entity for the new player may not exist yet.  Use FlowNode_OnPlayerReady
* instead of this node if you need access to the new player's actor / entity
*/
class FlowNode_OnPlayerConnected
    : public FlowNode_MultiplayerSessionEvent
{
public:
    FlowNode_OnPlayerConnected(SActivationInfo* activationInfo)
        : FlowNode_MultiplayerSessionEvent(activationInfo, "Activates when a new player connects. The player may not have an entity yet.")
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_OnPlayerConnected(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("MemberId", _HELP("Member Id of the player that connected.")),
            OutputPortConfig<string>("Name", _HELP("Name of the player that connected.")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP(m_description);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        FlowNode_MultiplayerSessionEvent::ProcessEvent(event, activationInfo);

        if (event == eFE_Initialize)
        {
            GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);
            if (network)
            {
                GridMate::GridSession* session = network->GetCurrentSession();
                if (session)
                {
                    GridMate::GridMember* member = session->GetMyMember();
                    if (member)
                    {
                        ChannelId channelId = network->GetChannelIdForSessionMember(member);
                        if (channelId != kInvalidChannelId)
                        {
                            ActivateOutput(&m_activationInfo, OutputPortMemberId, (int)channelId);
                            ActivateOutput(&m_activationInfo, OutputPortMemberName, string(member->GetName().c_str()));
                        }
                    }
                }
                else
                {
                    ChannelId channelId = network->GetLocalChannelId();
                    ActivateOutput(&m_activationInfo, OutputPortMemberId, (int)channelId);
                }
            }
        }
    }

    void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override
    {
        if (member)
        {
            ActivateOutput(&m_activationInfo, OutputPortMemberId, member->GetIdCompact());
            ActivateOutput(&m_activationInfo, OutputPortMemberName, string(member->GetName().c_str()));
        }
    }
protected:

    enum OutputPorts
    {
        OutputPortMemberId = 0,
        OutputPortMemberName
    };
};

/*!
* FlowGraph node that activates when a new player connects and their actor / entity is valid.
*/
class FlowNode_OnPlayerReady
    : public FlowNode_MultiplayerSessionEvent
{
public:
    FlowNode_OnPlayerReady(SActivationInfo* activationInfo)
        : FlowNode_MultiplayerSessionEvent(activationInfo, "Activates when a new player has a valid actor entity.")
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_OnPlayerReady(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("MemberId", _HELP("Member Id of the player that connected.")),
            OutputPortConfig<FlowEntityId>("EntityId", _HELP("Id of the entity controlled by the player.")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP(m_description);
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        FlowNode_MultiplayerSessionEvent::ProcessEvent(event, activationInfo);

        if (eFE_Update == event)
        {
            m_connectingClients.remove_if(
                [activationInfo](ChannelId channelId)
                {
                    IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActorByChannelId(channelId);
                    if (pActor && pActor->GetEntityId() != kInvalidEntityId)
                    {
                        ActivateOutput(activationInfo, OutputPortMemberId, channelId);
                        ActivateOutput(activationInfo, OutputPortEntityId, FlowEntityId(pActor->GetEntityId()));
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                );

            if (m_connectingClients.empty())
            {
                m_activationInfo.pGraph->SetRegularlyUpdated(m_activationInfo.myID, false);
            }
        }
    }

    void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override
    {
        if (member)
        {
            ChannelId channelId = member->GetIdCompact();
            stl::push_back_unique(m_connectingClients, channelId);
            m_activationInfo.pGraph->SetRegularlyUpdated(m_activationInfo.myID, true);
        }
    }

protected:
    enum OutputPorts
    {
        OutputPortMemberId = 0,
        OutputPortEntityId
    };

private:
    std::list<ChannelId> m_connectingClients;
};

/*!
* FlowGraph node that activates when a player disconnects.
*/
class FlowNode_OnPlayerDisconnected
    : public FlowNode_MultiplayerSessionEvent
{
public:
    FlowNode_OnPlayerDisconnected(SActivationInfo* activationInfo)
        : FlowNode_MultiplayerSessionEvent(activationInfo, "Activates when a player disconnects.")
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_OnPlayerDisconnected(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<int>("MemberId", _HELP("Member Id of the player that disconnected.")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP(m_description);
        config.SetCategory(EFLN_APPROVED);
    }

    void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member) override
    {
        if (member)
        {
            ActivateOutput(&m_activationInfo, OutputPortMemberId, member->GetIdCompact());
        }
    }
protected:

    enum InputPorts
    {
        InputPortActivate = 0,
    };

    enum OutputPorts
    {
        OutputPortMemberId = 0
    };
};

/*!
 * FlowGraph node that activates when the local player has connected and their actor / entity is valid.
 */
class FlowNode_OnLocalPlayerReady
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    FlowNode_OnLocalPlayerReady(SActivationInfo* activationInfo)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] = {
            { 0 }
        };
        static const SOutputPortConfig outputs[] = {
            OutputPortConfig<FlowEntityId>("EntityId", _HELP("The local player's entity id")),
            OutputPortConfig<int>("MemberId", _HELP("The local player's network member id")),
            { 0 }
        };
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Outputs the local player's entity id when ready");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            activationInfo->pGraph->SetRegularlyUpdated(activationInfo->myID, true);
            break;
        }

        case eFE_Update:
        {
            if (UpdateEntityIdOutput(activationInfo))
            {
                activationInfo->pGraph->SetRegularlyUpdated(activationInfo->myID, false);
            }
            break;
        }
        }
    }

    bool UpdateEntityIdOutput(SActivationInfo* activationInfo)
    {
        IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
        if (pActor && pActor->GetEntityId() != kInvalidEntityId)
        {
            ActivateOutput(activationInfo, OutputPortEntityId, FlowEntityId(pActor->GetEntityId()));

            GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);
            if (network)
            {
                ActivateOutput(activationInfo, OutputPortMemberId, (int)network->GetLocalChannelId());
            }
            return true;
        }

        return false;
    }

    enum OutputPorts
    {
        OutputPortEntityId = 0,
        OutputPortMemberId
    };
};

REGISTER_FLOW_NODE(g_multiplayerOnConnectedNodePath, FlowNode_OnConnected);
REGISTER_FLOW_NODE(g_multiplayerOnDisconnectedNodePath, FlowNode_OnDisconnected);
REGISTER_FLOW_NODE(g_multiplayerOnPlayerConnectedNodePath, FlowNode_OnPlayerConnected);
REGISTER_FLOW_NODE(g_multiplayerOnPlayerDisconnectedNodePath, FlowNode_OnPlayerDisconnected);
REGISTER_FLOW_NODE(g_multiplayerOnLocalPlayerReadyNodePath, FlowNode_OnLocalPlayerReady);
REGISTER_FLOW_NODE(g_multiplayerOnPlayerReadyNodePath, FlowNode_OnPlayerReady);