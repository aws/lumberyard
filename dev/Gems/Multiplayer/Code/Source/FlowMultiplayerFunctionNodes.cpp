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
#include <CryString.h>
#include <INetwork.h>
#include <Multiplayer/FlowMultiplayerNodes.h>
#include <GridMate/GridMate.h>
#include <GridMate/Carrier/Driver.h>
#include <GridMate/Session/Session.h>
#include <GridMate/Session/LANSession.h>

#include "MultiplayerGem.h"
#include "Multiplayer/MultiplayerUtils.h"


#if defined(AZ_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_1 1
#define FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_2 2
#define FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_LANPATH 3
#define FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_FLOWNODES 4
#define FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_REGISTERNODES 5
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, AZ_RESTRICTED_PLATFORM)
#endif

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)

#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftClientServiceBus.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftSessionRequest.h>

#endif

namespace
{
    static int g_simulateServerResults = 0;

    // FUNCTIONS
    const char* g_multiplayerListServersResultNodePath = "Multiplayer:Functions:ListServersResult";
    const char* g_multiplayerSetOwnerNodePath = "Multiplayer:Functions:SetOwner";

    const char* g_multiplayerConnectNodePath = "Multiplayer:Functions:Connect";
    const char* g_multiplayerHostNodePath = "Multiplayer:Functions:Host";
    const char* g_multiplayerListServersNodePath = "Multiplayer:Functions:ListServers";

    // LAN
    const char* g_multiplayerHostLANNodePath = "Multiplayer:Functions:LAN:Host";
    const char* g_multiplayerConnectLANNodePath = "Multiplayer:Functions:LAN:Connect";
    const char* g_multiplayerListServersLANNodePath = "Multiplayer:Functions:LAN:ListServers";

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XBONE)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_LANPATH
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, TOOLS_SUPPORT_XBONE)
#endif
#if defined(TOOLS_SUPPORT_PS4)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_LANPATH
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, TOOLS_SUPPORT_PS4)
#endif
#endif
}

namespace FlowMultiplayerNodes
{
    /*!
     * Implementation of simulated search for editor display and testing
     */
    class SimulatedSearch : public ISimulatedSearch
    {
    public:
        SimulatedSearch()
        {
            string serverNames[] = { "Server1", "Server2", "Server3", "Server4" };
            string mapNames[] = { "MapName1", "MapName2", "MapName3", "MapName4" };
            string serverIps[] = { "1.1.1.1", "2.2.2.2", "3.3.3.3", "4.4.4.4" };

            GridMate::LANSearchInfo lanSearchInfo;
            lanSearchInfo.m_numPlayers = 10;
            lanSearchInfo.m_numFreePublicSlots = 6;
            lanSearchInfo.m_numUsedPublicSlots = 10;
            lanSearchInfo.m_sessionId = "000000000000";
            lanSearchInfo.m_numParams = 2;

            for (int i = 0; i < 4; ++i)
            {
                lanSearchInfo.m_params[0].SetValue(serverNames[i]);
                lanSearchInfo.m_params[1].SetValue(mapNames[i]);
                lanSearchInfo.m_serverIP = serverIps[i];
                m_results.push_back(lanSearchInfo);
            }

        }

        virtual unsigned int GetNumResults() const
        {
            return static_cast<unsigned int>(m_results.size());
        }

        virtual const GridMate::LANSearchInfo*	GetResult(unsigned int index) const
        {
            return &m_results[index];
        }

        virtual void AbortSearch() {};
    private:
        std::vector<GridMate::LANSearchInfo> m_results;
    };
}

// Shared Flow Nodes

/*!
* FlowGraph node that converts results from the ListServers FlowGraph node into actual values like
* server name, map, number of players etc.
*/
class FlowNode_ListServersResult
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    FlowNode_ListServersResult(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(nullptr)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_ListServersResult(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_CustomData<FlowMultiplayerNodes::SearchInfoCustomData>("Result", FlowMultiplayerNodes::SearchInfoCustomData(), _HELP("The result of a ListServer action")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig<string>("SessionId", _HELP("The session Id for this server")),
            OutputPortConfig<string>("ServerName", _HELP("The server name")),
            OutputPortConfig<string>("MapName", _HELP("The current map this server is running")),
            OutputPortConfig<int>("MaxPlayers", _HELP("Maximum number of players allowed on this server")),
            OutputPortConfig<int>("NumPlayers", _HELP("Number of players connected to this server")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("List available servers");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Activate && IsPortActive(activationInfo, InputPortResult))
        {
            if (gEnv->pNetwork)
            {
                GridMate::IGridMate* gridmate = gEnv->pNetwork->GetGridMate();
                if (gridmate)
                {
                    FlowMultiplayerNodes::SearchInfoCustomData customData = GetPortCustomData<FlowMultiplayerNodes::SearchInfoCustomData>(activationInfo, InputPortResult);
                    const GridMate::SearchInfo* searchInfo = customData.GetSearchInfo();
                    if (searchInfo)
                    {
                        string serverName = "(unknown)";
                        string mapName = "(unknown)";
                        if (searchInfo->m_numParams > 0)
                        {
                            serverName = searchInfo->m_params[0].m_value.c_str();
                        }
                        if (searchInfo->m_numParams > 1)
                        {
                            mapName = searchInfo->m_params[1].m_value.c_str();
                        }

                        ActivateOutput(&m_activationInfo, OutputPortSessionId, string(searchInfo->m_sessionId.c_str()));
                        ActivateOutput(&m_activationInfo, OutputPortServerName, serverName);
                        ActivateOutput(&m_activationInfo, OutputPortMapName, mapName);
                        ActivateOutput(&m_activationInfo, OutputPortMaxPlayers, searchInfo->m_numFreePublicSlots + searchInfo->m_numUsedPublicSlots);
                        ActivateOutput(&m_activationInfo, OutputPortNumPlayers, searchInfo->m_numUsedPublicSlots);
                    }
                }
            }
        }
    }

protected:

    SActivationInfo m_activationInfo;

    enum InputPorts
    {
        InputPortResult
    };

    enum OutputPorts
    {
        OutputPortSessionId = 0,
        OutputPortServerName,
        OutputPortMapName,
        OutputPortMaxPlayers,
        OutputPortNumPlayers
    };
};

/*!
* FlowGraph node that delegates network authority to a client for a particular entity.
* Delegation includes network aspects, physics and sets a script property for Lua scripts.
*/
class FlowNode_SetOwner
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    FlowNode_SetOwner(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(nullptr)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo * activationInfo) override
    {
        return new FlowNode_SetOwner(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Set the owner")),
            InputPortConfig<FlowEntityId>("EntityId", _HELP("The Entity Id of the entity to pwn")),
            InputPortConfig<int>("MemberId", _HELP("The pwner's Member Id")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Success", _HELP("Successfully set the owner")),
            OutputPortConfig_Void("Failed", _HELP("Failed to set the owner")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Set the owner of an entity");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo *activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Activate && IsPortActive(activationInfo, InputPortActivate))
        {
            if (gEnv->pNetwork)
            {
                FlowEntityId flowEntityId = GetPortFlowEntityId(activationInfo, InputPortEntityId);
                IEntity* entity = gEnv->pEntitySystem->GetEntity(flowEntityId);
                ChannelId channelId = GetPortInt(activationInfo, InputPortOwnerMemberId);
                if (entity != nullptr && channelId != kInvalidChannelId)
                {
                    EntityId entityId = entity->GetId();
                    if (gEnv->bServer)
                    {
                        CryLogAlways("Delegating network authority of entity %u to client %u", entityId, channelId);
                        gEnv->pNetwork->DelegateAuthorityToClient(entityId, channelId);
                    }

                    bool isAuthority = channelId == gEnv->pNetwork->GetLocalChannelId();

                    IPhysicalEntity* physicalEntity = entity->GetPhysics();
                    if (physicalEntity)
                    {
                        if (isAuthority)
                        {
                            CryLogAlways("Delegating physics network authority of entity %u to client %u", entityId, channelId);
                        }
                        physicalEntity->SetNetworkAuthority(isAuthority ? 1 : 0, 0);
                    }

                    IScriptTable* scriptTable = entity->GetScriptTable();
                    if (scriptTable)
                    {
                        if (isAuthority)
                        {
                            CryLogAlways("Delegating script authority of entity %u to client %u", entityId, channelId);
                        }
                        SmartScriptTable properties;
                        if (scriptTable->GetValue("Properties", properties))
                        {
                            properties->SetValue("IsAuthority", isAuthority);
                        }

                    }

                    ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                    return;
                }
            }

            ActivateOutput(&m_activationInfo, OutputPortFailed, true);
        }
    }

protected:
    SActivationInfo m_activationInfo;

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortEntityId,
        InputPortOwnerMemberId
    };

    enum OutputPorts
    {
        OutputPortSuccess = 0,
        OutputPortFailed
    };
};


// BaseFlowNodes

/*!
 * FlowGraph node to connect to a server.  The ServerAddress and Result inputs are optional and
 * if not provided, a connection to localhost will be attempted.
 */
class BaseFlowNode_Connect
    : public CFlowBaseNode<eNCT_Instanced>
    , public GridMate::SessionEventBus::Handler
{
public:
    BaseFlowNode_Connect(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(nullptr)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Connect to a server")),
            InputPortConfig<string>("ServerAddress", _HELP("The network address of the server")),
            InputPortConfig<int>("Port", -1, _HELP("The network port to use.  If -1 is specified, use value of the cl_serverport cvar.")),
            InputPortConfig_CustomData<FlowMultiplayerNodes::SearchInfoCustomData>("Result", FlowMultiplayerNodes::SearchInfoCustomData(), _HELP("The result of a ListServer node")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Success", _HELP("Successfully connected to the server.")),
            OutputPortConfig_Void("Failed", _HELP("Failed to connect to the server.")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Connect to a server");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void StartSessionService(GridMate::IGridMate* gridMate) = 0;
    virtual GridMate::GridSession* JoinSession(const GridMate::SearchInfo* searchInfo, GridMate::CarrierDesc& carrierDesc) = 0;

    void ProcessEvent(EFlowEvent event, SActivationInfo *activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Activate && IsPortActive(activationInfo, InputPortActivate))
        {
            if (gEnv->pNetwork && !gEnv->IsEditor())
            {
                GridMate::IGridMate *gridmate = gEnv->pNetwork->GetGridMate();
                if (gridmate)
                {
                    StartSessionService(gridmate);

                    GridMate::GridSession* session = nullptr;
                    EBUS_EVENT_RESULT(session, Multiplayer::MultiplayerRequestBus, GetSession);
                    if (!session)
                    {
                        FlowMultiplayerNodes::SearchInfoCustomData customData = GetPortCustomData<FlowMultiplayerNodes::SearchInfoCustomData>(activationInfo, InputPortListServersResult);
                        const GridMate::SearchInfo* searchInfo = customData.GetSearchInfo();
                        if (searchInfo)
                        {
                            GridMate::CarrierDesc carrierDesc;

                            Multiplayer::Utils::InitCarrierDesc(carrierDesc);
                            Multiplayer::NetSec::ConfigureCarrierDescForJoin(carrierDesc);

                            session = JoinSession(searchInfo,carrierDesc);
                            if (session != nullptr)
                            {
                                CryLog("Successfully joined a game session.");

                                EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
                                ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                            }
                            else
                            {
                                CryLog("Found a game session, but failed to join.");

                                ActivateOutput(&m_activationInfo, OutputPortFailed, true);
                            }
                        }
                        else
                        {
                            CryLog("Can't join session: No search result selected.");

                            ActivateOutput(&m_activationInfo, OutputPortFailed, true);
                        }
                    }
                    else
                    {
                        CryLog("Can't join session: We are already in a multiplayer session.");

                        ActivateOutput(&m_activationInfo, OutputPortFailed, true);
                    }
                }
            }
            else
            {
                CryLog("Unsupported Action Inside of Editor.");
                ActivateOutput(&m_activationInfo, OutputPortFailed, true);
            }
        }
    }

protected:
    SActivationInfo m_activationInfo;

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortServerAddress,
        InputPortServerPort,
        InputPortListServersResult
    };

    enum OutputPorts
    {
        OutputPortSuccess = 0,
        OutputPortFailed
    };
};

/*!
 * FlowGraph node to start hosting.  If ServerName or MaxPlayers inputs are empty, their corresponding
 * sv_ cvars will be used.
 */
class BaseFlowNode_Host
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    BaseFlowNode_Host(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(nullptr)
    {
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Host a server")),
            InputPortConfig<string>("ServerName", _HELP("The server name")),
            InputPortConfig<int>("MaxPlayers", _HELP("The maximum number of players allowed")),
            InputPortConfig<string>("Map", _HELP("The name of the map to load (optional, if empty will remain on current map)")),
            InputPortConfig<int>("Port", -1, _HELP("The network port to use.  If -1 is specified, use value of the sv_port cvar.")),
            InputPortConfig<bool>("DisconnectDetection", true, _HELP("Whether or not the host should detect disconnections. The gm_disconnectDetection cvar(true by default) and this port must both be true to detect disconnections")),
            InputPortConfig<int>("ConnectionTimeout", 10000, _HELP("How long a connection must remain idle(in MilliSeconds) before disconnecting")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Success", _HELP("Successfully started hosting")),
            OutputPortConfig_Void("Failed", _HELP("Failed to start hosting")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Host a server");
        config.SetCategory(EFLN_APPROVED);
    }

protected:

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortServerName,
        InputPortMaxPlayers,
        InputPortMap,
        InputPortPortNumber,
        InputPortDisconnectDetection,
        InputPortConnectionTimeout
    };

    enum OutputPorts
    {
        OutputPortSuccess = 0,
        OutputPortFailed
    };

public:

    virtual ICVar* FindServerPortCVar() = 0;
    virtual void StartSessionService(GridMate::IGridMate* gridMate) = 0;

    virtual GridMate::SessionParams* GetSessionParams() = 0;
    virtual GridMate::GridSession* HostSession(GridMate::CarrierDesc& carrierDesc) = 0;

    void ProcessEvent(EFlowEvent event, SActivationInfo *activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Activate && IsPortActive(activationInfo, InputPortActivate))
        {
            if (gEnv->pNetwork && !gEnv->IsEditor())
            {
                GridMate::IGridMate *gridmate = gEnv->pNetwork->GetGridMate();
                GridMate::GridSession* gridSession = nullptr;
                EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);
                if (gridmate && !gridSession)
                {
                    StartSessionService(gridmate);

                    string serverName = GetPortString(activationInfo, InputPortServerName);
                    if (serverName == "")
                    {
                        serverName = gEnv->pConsole->GetCVar("sv_servername")->GetString();
                    }
                    else
                    {
                        gEnv->pConsole->GetCVar("sv_servername")->Set(serverName);
                    }

                    int maxPlayers = GetPortInt(activationInfo, InputPortMaxPlayers);
                    if (maxPlayers <= 0)
                    {
                        maxPlayers = gEnv->pConsole->GetCVar("sv_maxplayers")->GetIVal();
                    }
                    else
                    {
                        gEnv->pConsole->GetCVar("sv_maxplayers")->Set(maxPlayers);
                    }

                    int port = GetPortInt(activationInfo,InputPortPortNumber);

                    ICVar* serverPort = FindServerPortCVar();

                    if (serverPort != nullptr)
                    {
                        if (port < 0)
                        {
                            port = serverPort->GetIVal();
                        }
                        else
                        {
                            serverPort->Set(port);
                        }
                    }

                    if (port < 0)
                    {
                        AZ_Assert(false, "Trying to create host at unknown port.");
                        port = 0;
                    }

                    bool enableDisconnectDetection = GetPortBool(activationInfo,InputPortDisconnectDetection);

                    GridMate::CarrierDesc carrierDesc;

                    Multiplayer::Utils::InitCarrierDesc(carrierDesc);
                    Multiplayer::NetSec::ConfigureCarrierDescForHost(carrierDesc);

                    /////
                    // Common Carrier configuration
                    carrierDesc.m_port = port;

                    ICVar* disconnectDetection = gEnv->pConsole->GetCVar("gm_disconnectDetection");
                    if (disconnectDetection)
                    {
                        carrierDesc.m_enableDisconnectDetection = enableDisconnectDetection && !!(disconnectDetection->GetIVal());
                    }
                    else
                    {
                        carrierDesc.m_enableDisconnectDetection = enableDisconnectDetection;
                    }

                    carrierDesc.m_connectionTimeoutMS = GetPortInt(activationInfo,InputPortConnectionTimeout);
                    carrierDesc.m_threadUpdateTimeMS = 30;
                    /////

                    GridMate::SessionParams* sessionParams = GetSessionParams();

                    /////
                    // Common Session configuration
                    sessionParams->m_topology = GridMate::ST_CLIENT_SERVER;
                    sessionParams->m_numPublicSlots = maxPlayers + (gEnv->IsDedicated() ? 1 : 0); // One slot for server member.
                    sessionParams->m_numPrivateSlots = 0;
                    sessionParams->m_peerToPeerTimeout = 60000;
                    sessionParams->m_flags = 0;
                    /////

                    sessionParams->m_numParams = 0;
                    sessionParams->m_params[sessionParams->m_numParams].m_id = "sv_name";
                    sessionParams->m_params[sessionParams->m_numParams].SetValue(serverName);
                    sessionParams->m_numParams++;

                    string mapName = GetPortString(activationInfo, InputPortMap);
                    if (!mapName.empty())
                    {
                        sessionParams->m_params[sessionParams->m_numParams].m_id = "sv_map";
                        sessionParams->m_params[sessionParams->m_numParams].SetValue(mapName);
                        sessionParams->m_numParams++;
                    }

                    GridMate::GridSession* session = HostSession(carrierDesc);

                    if (session != nullptr)
                    {
                        EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
                        ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                        return;
                    }
                }
                else
                {
                    CryLog("Can't host session: We are already in a multiplayer session.");
                }
            }
            else
            {
                if (gEnv->IsEditor())
                {
                    CryLog("Can't host session: Multiplayer functionality is not available in the editor.");
                }
                else
                {
                    CryLog("Can't host session: We don't have a valid GridMate instance.");
                }
            }

            ActivateOutput(&m_activationInfo, OutputPortFailed, true);
        }
    }

protected:
    SActivationInfo m_activationInfo;
};


/*!
 * FlowGraph node to search for servers.  In editor mode this will return simulated results.
 * The corresponding FlowNode_ListServersResult converts the search results into values like
 * server name, map, number of players etc.
 */
class BaseFlowNode_ListServers
    : public CFlowBaseNode<eNCT_Instanced>
    , public GridMate::SessionEventBus::Handler
{
public:
    BaseFlowNode_ListServers(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(nullptr)
        , m_search(nullptr)
        , m_simulatedSearch(nullptr)
    {
        ICVar* simulateServerResults = gEnv->pConsole->GetCVar("cl_simulate_server_results");

        if (simulateServerResults)
        {
            g_simulateServerResults = !!simulateServerResults->GetIVal();
        }
        else
        {
            REGISTER_CVAR2("cl_simulate_server_results", &g_simulateServerResults, gEnv->IsEditor(), VF_NULL, "Display simulated server results from the ListServers FlowGraph node.");
        }
    }

    virtual ~BaseFlowNode_ListServers()
    {
        if (m_simulatedSearch)
        {
            delete m_simulatedSearch;
        }
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Activate the service")),
            InputPortConfig<int>("MaxResults", _HELP("The maximum number of results to retrieve")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Success", _HELP("Successfully got the server list")),
            OutputPortConfig_Void("Failed", _HELP("Failed to connect to the server")),
            OutputPortConfig<int>("NumResults", _HELP("Number of results returned by server")),
            OutputPortConfig_CustomData<FlowMultiplayerNodes::SearchInfoCustomData>("Results", _HELP("This port activates once for every search result")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("List available servers");
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void StartSessionService(GridMate::IGridMate* gridMate) = 0;
    virtual GridMate::GridSearch* StartGridSearch() = 0;

    void ProcessEvent(EFlowEvent event, SActivationInfo *activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Activate && IsPortActive(activationInfo, InputPortActivate))
        {
            GridMate::GridSession* gridSession = nullptr;
            EBUS_EVENT_RESULT(gridSession, Multiplayer::MultiplayerRequestBus, GetSession);
            if (!gridSession)
            {
                bool simulateSearchResult = !!g_simulateServerResults;
                ICVar* simulateCVar = gEnv->pConsole->GetCVar("cl_simulate_server_results");

                if (simulateCVar)
                {
                    simulateSearchResult = !!simulateCVar->GetIVal();
                }

                if (simulateSearchResult)
                {
                    if (!m_simulatedSearch)
                    {
                        m_simulatedSearch = new FlowMultiplayerNodes::SimulatedSearch();
                    }

                    if (m_simulatedSearch)
                    {
                        ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                        ActivateOutput(&m_activationInfo, OutputPortNumResults, 4);
                        for (int i = 0; i < m_simulatedSearch->GetNumResults(); ++i)
                        {
                            FlowMultiplayerNodes::SearchInfoCustomData customData(m_simulatedSearch, i, simulateSearchResult);
                            ActivateCustomDataOutput<FlowMultiplayerNodes::SearchInfoCustomData>(&m_activationInfo, OutputPortResult, customData);
                        }
                    }
                }
                else
                {
                    if (gEnv->pNetwork)
                    {
                        GridMate::IGridMate *gridmate = gEnv->pNetwork->GetGridMate();
                        if (gridmate)
                        {
                            StartSessionService(gridmate);

                            if (m_search && m_search->IsDone())
                            {
                                // invalidate old results by sending along empty results
                                for (int i = 0; i < m_search->GetNumResults(); ++i)
                                {
                                    FlowMultiplayerNodes::SearchInfoCustomData customData;
                                    ActivateCustomDataOutput<FlowMultiplayerNodes::SearchInfoCustomData>(&m_activationInfo, OutputPortResult, customData);
                                }

                                m_search->Release();
                                m_search = nullptr;
                            }

                            if (nullptr == m_search)
                            {
                                m_search = StartGridSearch();

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
                                GridMate::LANSearchParams searchParams;
                                searchParams.m_serverPort = gEnv->pConsole->GetCVar("cl_serverport")->GetIVal() + 1;
                                searchParams.m_listenPort = 0;

                                EBUS_EVENT_ID_RESULT(m_search,gridmate,GridMate::LANSessionServiceBus,StartGridSearch,searchParams);
    #endif
                                if (m_search)
                                {
                                    BusConnect(gridmate);
                                }
                            }

                            if (!m_search)
                            {
                                CryLog("ListServers failed to sart a GridSearch.");
                                ActivateOutput(&m_activationInfo, OutputPortFailed, true);
                                ActivateOutput(&m_activationInfo, OutputPortNumResults, 0);
                            }
                        }
                    }
                }
            }
            else
            {
                CryLog("Can't start a search because we are already in a multiplayer session.");
                ActivateOutput(&m_activationInfo, OutputPortFailed, true);
                ActivateOutput(&m_activationInfo, OutputPortNumResults, 0);
            }
        }
    }

    void OnGridSearchComplete(GridMate::GridSearch* search) override
    {
        if (search == m_search)
        {
            if (gEnv->pNetwork)
            {
                BusDisconnect();

                if (search->GetNumResults() > 0)
                {
                    ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                    ActivateOutput(&m_activationInfo, OutputPortNumResults, (int)search->GetNumResults());

                    bool simulateSearchResult = !!g_simulateServerResults;
                    ICVar* simulateCVar = gEnv->pConsole->GetCVar("cl_simulate_server_results");

                    if (simulateCVar)
                    {
                        simulateSearchResult = !!simulateCVar->GetIVal();
                    }

                    // activate the OutputPortResult once for every result in the search
                    for (int i = 0; i < search->GetNumResults(); ++i)
                    {
                        FlowMultiplayerNodes::SearchInfoCustomData customData(m_search, i, simulateSearchResult);
                        ActivateCustomDataOutput<FlowMultiplayerNodes::SearchInfoCustomData>(&m_activationInfo, OutputPortResult, customData);
                    }
                }
                else
                {
                    CryLog("No game sessions found.");
                    ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                    ActivateOutput(&m_activationInfo, OutputPortNumResults, 0);
                }
            }
        }
    }

protected:

    GridMate::GridSearch* m_search;
    FlowMultiplayerNodes::SimulatedSearch* m_simulatedSearch;
    SActivationInfo m_activationInfo;

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortServerAddress,
        InputPortSessionId,
    };

    enum OutputPorts
    {
        OutputPortSuccess = 0,
        OutputPortFailed,
        OutputPortNumResults,
        OutputPortResult
    };
};



// LAN Flow Nodes

class LANFlowNode_Connect
    : public BaseFlowNode_Connect
{
public:
    LANFlowNode_Connect(SActivationInfo* activationInfo)
        : BaseFlowNode_Connect(activationInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo * activationInfo) override
    {
        return new LANFlowNode_Connect(activationInfo);
    }

    void StartSessionService(GridMate::IGridMate* gridMate) override
    {
        Multiplayer::LAN::StartSessionService(gridMate);
    }

    GridMate::GridSession* JoinSession(const GridMate::SearchInfo* searchInfo,GridMate::CarrierDesc& carrierDesc) override
    {
        GridMate::GridSession* session = nullptr;
        GridMate::JoinParams joinParams;

        const GridMate::LANSearchInfo& lanSearchInfo = static_cast<const GridMate::LANSearchInfo&>(*searchInfo);
        EBUS_EVENT_ID_RESULT(session,gEnv->pNetwork->GetGridMate(),GridMate::LANSessionServiceBus,JoinSessionBySearchInfo,lanSearchInfo,joinParams,carrierDesc);

        return session;
    }
};

class LANFlowNode_Host
    : public BaseFlowNode_Host
{
public:
    LANFlowNode_Host(SActivationInfo* activationInfo)
        : BaseFlowNode_Host(activationInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo * activationInfo) override
    {
        return new LANFlowNode_Host(activationInfo);
    }

    virtual ICVar* FindServerPortCVar()
    {
        return gEnv->pConsole->GetCVar("sv_port");
    }

    void StartSessionService(GridMate::IGridMate* gridMate) override
    {
        Multiplayer::LAN::StartSessionService(gridMate);
    }

    GridMate::SessionParams* GetSessionParams() override
    {
        return &m_sessionParams;
    }

    GridMate::GridSession* HostSession(GridMate::CarrierDesc& carrierDesc) override
    {
        // Attempt to start a hosted LAN session. If we do, we're now a server and in multiplayer mode.

        // Listen for searches on port + 1
        m_sessionParams.m_port = carrierDesc.m_port + 1;

        GridMate::GridSession* retVal = nullptr;

        EBUS_EVENT_ID_RESULT(retVal,gEnv->pNetwork->GetGridMate(),GridMate::LANSessionServiceBus,HostSession,m_sessionParams,carrierDesc);

        return retVal;
    }

private:
    GridMate::LANSessionParams m_sessionParams;
};

/*!
 * FlowGraph node to search for servers.  In editor mode this will return simulated results.
 * The corresponding FlowNode_ListServersResult converts the search results into values like
 * server name, map, number of players etc.
 */
class LANFlowNode_ListServers
    : public BaseFlowNode_ListServers

{
public:
    LANFlowNode_ListServers(SActivationInfo* activationInfo)
        : BaseFlowNode_ListServers(activationInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo * activationInfo) override
    {
        return new LANFlowNode_ListServers(activationInfo);
    }

    void StartSessionService(GridMate::IGridMate* gridMate) override
    {
        Multiplayer::LAN::StartSessionService(gridMate);
    }

    GridMate::GridSearch* StartGridSearch() override
    {
        GridMate::GridSearch* gridSearch = nullptr;

        GridMate::LANSearchParams searchParams;
        searchParams.m_serverPort = gEnv->pConsole->GetCVar("cl_serverport")->GetIVal() + 1;
        searchParams.m_listenPort = 0;

        EBUS_EVENT_ID_RESULT(gridSearch,gEnv->pNetwork->GetGridMate(),GridMate::LANSessionServiceBus,StartGridSearch,searchParams);

        return gridSearch;
    }
};
// </LAN>

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XBONE)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_FLOWNODES
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, TOOLS_SUPPORT_XBONE)
#endif
#if defined(TOOLS_SUPPORT_PS4)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_FLOWNODES
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, TOOLS_SUPPORT_PS4)
#endif
#endif

// GameLift Flow Nodes
#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)

class GameLiftFlowNode_Connect
    : public BaseFlowNode_Connect
{
public:
    GameLiftFlowNode_Connect(SActivationInfo* activationInfo)
        : BaseFlowNode_Connect(activationInfo)
    {
    }

    IFlowNodePtr Clone(SActivationInfo * activationInfo) override
    {
        return new GameLiftFlowNode_Connect(activationInfo);
    }

    void StartSessionService(GridMate::IGridMate* gridMate) override
    {
        (void)gridMate;

        GridMate::GameLiftClientService* clientService = nullptr;
        EBUS_EVENT_RESULT(clientService, GameLift::GameLiftRequestBus, GetClientService);

        if (clientService == nullptr)
        {
            GridMate::GameLiftClientServiceDesc desc;
            desc.m_region = gEnv->pConsole->GetCVar("gamelift_aws_region")->GetString();

            desc.m_accessKey = gEnv->pConsole->GetCVar("gamelift_aws_access_key")->GetString(); //< AWS Access key
            desc.m_secretKey = gEnv->pConsole->GetCVar("gamelift_aws_secret_key")->GetString(); //< AWS Secret key
            desc.m_fleetId = gEnv->pConsole->GetCVar("gamelift_fleet_id")->GetString(); //< GameLift fleetId

            EBUS_EVENT(GameLift::GameLiftRequestBus, StartClientService, desc);
        }
    }

    GridMate::GridSession* JoinSession(const GridMate::SearchInfo* searchInfo, GridMate::CarrierDesc& carrierDesc) override
    {
        GridMate::GridSession* session = nullptr;

        const GridMate::GameLiftSearchInfo& gameLiftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo&>(*searchInfo);
        EBUS_EVENT_ID_RESULT(session, gEnv->pNetwork->GetGridMate(), GridMate::GameLiftClientServiceBus, JoinSessionBySearchInfo, gameLiftSearchInfo, carrierDesc);

        return session;
    }
};

const char* g_multiplayerConnectGameLiftNodePath = "Multiplayer:Functions:GameLift:Connect";

REGISTER_FLOW_NODE(g_multiplayerConnectGameLiftNodePath, GameLiftFlowNode_Connect);

#endif

REGISTER_FLOW_NODE(g_multiplayerHostLANNodePath,LANFlowNode_Host);
REGISTER_FLOW_NODE(g_multiplayerConnectLANNodePath,LANFlowNode_Connect);
REGISTER_FLOW_NODE(g_multiplayerListServersLANNodePath,LANFlowNode_ListServers);

#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XBONE)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_REGISTERNODES
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, TOOLS_SUPPORT_XBONE)
#endif
#if defined(TOOLS_SUPPORT_PS4)
#define AZ_RESTRICTED_SECTION FLOWMULTIPLAYERFUNCTIONNODES_CPP_SECTION_REGISTERNODES
#include AZ_RESTRICTED_FILE(FlowMultiplayerFunctionNodes_cpp, TOOLS_SUPPORT_PS4)
#endif
#endif

REGISTER_FLOW_NODE(g_multiplayerListServersResultNodePath, FlowNode_ListServersResult);
REGISTER_FLOW_NODE(g_multiplayerSetOwnerNodePath, FlowNode_SetOwner);
