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

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)
#include <AzCore/Memory/Memory.h>
#include <GridMate/Session/LANSession.h>
#include <IFlowSystem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <Multiplayer/FlowMultiplayerNodes.h>
#include <GridMate/NetworkGridMate.h>

#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftClientServiceBus.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftSessionRequest.h>

namespace
{
    const char* g_GameLiftStartNodePath = "Multiplayer:GameLift:Start";
    const char* g_GameLiftCreateGameSessionNodePath = "Multiplayer:GameLift:CreateGameSession";
    const char* g_GameLiftListServersNodePath = "Multiplayer:GameLift:ListServers";
}

/*!
 * FlowGraph node that starts the GameLift service.
 * NOTE: all inputs will default to their corresponding cvar values if left empty.
 */
class FlowNode_GameLift_Start
    : public CFlowBaseNode<eNCT_Singleton>
    , public GridMate::GameLiftClientServiceEventsBus::Handler
{
public:
    FlowNode_GameLift_Start(SActivationInfo* activationInfo)
        : m_activationInfo(*activationInfo)
    {
    }

    ~FlowNode_GameLift_Start() override
    {
        EBUS_EVENT(GameLift::GameLiftRequestBus, StopClientService);
    }

    virtual void GetMemoryUsage(ICrySizer* sizer) const
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Start GameLift")),
            InputPortConfig<string>("AWSAccessKey", _HELP("AWS Access Key")),
            InputPortConfig<string>("AWSSecretKey", _HELP("AWS Secret Key")),
            InputPortConfig<string>("AWSRegion", _HELP("AWS Region")),
            InputPortConfig<string>("GameLiftEndpoint", _HELP("GameLift Endpoint (optional)")),
            InputPortConfig<string>("FleetID", _HELP("Fleet ID")),
            InputPortConfig<string>("AliasID", _HELP("Alias ID (required if no Fleet ID provided)")),
            InputPortConfig<string>("PlayerID", _HELP("Player ID (optional)")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Success", _HELP("Successfully started GameLift")),
            OutputPortConfig_Void("Failed", _HELP("Failed to start GameLift")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Start GameLift");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate))
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
            AZ_Assert(gridMate, "GridMate does not exist");

            if (GridMate::GameLiftClientServiceBus::GetNumOfEventHandlers(gridMate))
            {
                CryLogAlways("GameLift session service is already running.");
                ActivateOutput(&m_activationInfo, OutputPortFailed, true);
                return;
            }

            BusConnect(gridMate);

            GridMate::GameLiftClientServiceDesc serviceDesc;
            serviceDesc.m_accessKey = GetPortValueOrDefault(InputPortAWSAccessKey, "gamelift_aws_access_key");
            serviceDesc.m_secretKey = GetPortValueOrDefault(InputPortAWSSecretKey, "gamelift_aws_secret_key");
            serviceDesc.m_fleetId = GetPortValueOrDefault(InputPortFleetID, "gamelift_fleet_id");
            serviceDesc.m_endpoint = GetPortValueOrDefault(InputPortGameLiftEndpoint, "gamelift_endpoint");
            serviceDesc.m_region = GetPortValueOrDefault(InputPortAWSRegion, "gamelift_aws_region");
            serviceDesc.m_aliasId = GetPortValueOrDefault(InputPortAliasID, "gamelift_alias_id");
            serviceDesc.m_playerId = GetPortValueOrDefault(InputPortPlayerID, "gamelift_player_id");
            EBUS_EVENT(GameLift::GameLiftRequestBus, StartClientService, serviceDesc);
        }
    }

private:
    void OnGameLiftSessionServiceReady(GridMate::GameLiftClientService*) override
    {
        BusDisconnect();
        ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
    }

    void OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService*, const AZStd::string&) override
    {
        BusDisconnect();
        ActivateOutput(&m_activationInfo, OutputPortFailed, true);
    }

    const string GetPortValueOrDefault(int port, string cvar)
    {
        const string& portValue = GetPortString(&m_activationInfo, port);
        if (!portValue.empty())
        {
            return portValue;
        }

        return gEnv->pConsole->GetCVar(cvar)->GetString();
    }

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortAWSAccessKey,
        InputPortAWSSecretKey,
        InputPortAWSRegion,
        InputPortGameLiftEndpoint,
        InputPortFleetID,
        InputPortAliasID,
        InputPortPlayerID
    };

    enum OutputPorts
    {
        OutputPortSuccess = 0,
        OutputPortFailed
    };

    SActivationInfo m_activationInfo;
};

/*!
* FlowGraph node to search for servers.
* The corresponding FlowNode_ListServersResult converts the search results into values like
* server name, map, number of players etc.
*/
class FlowNode_GameLift_ListServers
    : public CFlowBaseNode<eNCT_Instanced>
    , public GridMate::SessionEventBus::Handler
{
public:
    FlowNode_GameLift_ListServers(SActivationInfo* activationInfo)
        : CFlowBaseNode()
        , m_activationInfo(nullptr)
        , m_search(nullptr)
    {
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_GameLift_ListServers(activationInfo);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Activate the service")),
            InputPortConfig<int>("MaxResults", _HELP("The maximum number of results to retrieve")),
            InputPortConfig<int>("TestNode", _HELP("The maximum number of results to retrieve")),
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

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (event == eFE_Activate && IsPortActive(activationInfo, InputPortActivate))
        {
            GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);
            if (network)
            {
                GridMate::IGridMate* gridmate = network->GetGridMate();
                if (gridmate)
                {
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
                        EBUS_EVENT_ID_RESULT(m_search, gridmate, GridMate::GameLiftClientServiceBus, StartSearch, GridMate::GameLiftSearchParams());
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

    void OnGridSearchComplete(GridMate::GridSearch* search) override
    {
        if (search == m_search)
        {
            GridMate::Network* network = static_cast<GridMate::Network*>(gEnv->pNetwork);
            if (network)
            {
                BusDisconnect();

                if (search->GetNumResults() > 0)
                {
                    ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                    ActivateOutput(&m_activationInfo, OutputPortNumResults, (int)search->GetNumResults());

                    // activate the OutputPortResult once for every result in the search
                    for (int i = 0; i < search->GetNumResults(); ++i)
                    {
                        FlowMultiplayerNodes::SearchInfoCustomData customData(m_search, i, false);
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


/*!
* FlowGraph node that sends a GameLift request to create a new game session.
*/
class FlowNode_GameLift_CreateGameSession
    : public CFlowBaseNode<eNCT_Instanced>
    , public GridMate::SessionEventBus::Handler
{
public:
    FlowNode_GameLift_CreateGameSession(SActivationInfo* activationInfo)
        : m_activationInfo(*activationInfo)
        , m_search(nullptr)
    {
    }

    IFlowNodePtr Clone(SActivationInfo* activationInfo) override
    {
        return new FlowNode_GameLift_CreateGameSession(activationInfo);
    }

    void GetMemoryUsage(ICrySizer* sizer) const override
    {
        sizer->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Activate", _HELP("Create a GameLift game session")),
            InputPortConfig<string>("ServerName", _HELP("Server name")),
            InputPortConfig<string>("Map", _HELP("Map name")),
            InputPortConfig<int>("MaxPlayers", _HELP("Maximum number of players allowed")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            OutputPortConfig_Void("Success", _HELP("Successfully created a GameLift game session")),
            OutputPortConfig_Void("Failed", _HELP("Failed to start a GameLift game session")),
            OutputPortConfig_CustomData<FlowMultiplayerNodes::SearchInfoCustomData>("Results", _HELP("The created GameLift game session result")),
            { 0 }
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Create a GameLift game session");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
    {
        m_activationInfo = *activationInfo;

        if (eFE_Activate == event && IsPortActive(activationInfo, InputPortActivate))
        {
            GridMate::IGridMate* gridmate = gEnv->pNetwork->GetGridMate();
            if (gridmate)
            {
                if (m_search)
                {
                    // invalidate old results by sending along empty results
                    FlowMultiplayerNodes::SearchInfoCustomData customData;
                    ActivateCustomDataOutput<FlowMultiplayerNodes::SearchInfoCustomData>(&m_activationInfo, OutputPortResult, customData);

                    m_search->AbortSearch();
                    m_search->Release();
                    m_search = nullptr;
                }
                else
                {
                    BusConnect(gridmate);
                }

                // Request a new GameLift game
                GridMate::GameLiftSessionRequestParams reqParams;
                reqParams.m_instanceName = GetPortString(activationInfo, InputPortServerName).c_str();
                reqParams.m_numPublicSlots = GetPortInt(activationInfo, InputPortMaxPlayers);
                reqParams.m_params[0].m_id = "sv_name";
                reqParams.m_params[0].m_value = GetPortString(activationInfo, InputPortServerName).c_str();
                reqParams.m_numParams++;
                reqParams.m_params[1].m_id = "sv_map";
                reqParams.m_params[1].m_value = GetPortString(activationInfo, InputPortMapName).c_str();
                reqParams.m_numParams++;
                EBUS_EVENT_ID_RESULT(m_search, gridmate, GridMate::GameLiftClientServiceBus, RequestSession, reqParams);
            }

            if (!m_search)
            {
                CryLogAlways("Failed to create a new GameLift game");
                ActivateOutput(&m_activationInfo, OutputPortFailed, true);
            }
        }
    }

    void OnGridSearchComplete(GridMate::GridSearch* search) override
    {
        if (search == m_search)
        {
            BusDisconnect();

            if (search->GetNumResults() > 0)
            {
                CryLogAlways("GameLift session found ");
                ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
                FlowMultiplayerNodes::SearchInfoCustomData customData(m_search, 0);
                ActivateCustomDataOutput<FlowMultiplayerNodes::SearchInfoCustomData>(&m_activationInfo, OutputPortResult, customData);
            }
            else
            {
                CryLogAlways("No GameLift sessions found.");
                ActivateOutput(&m_activationInfo, OutputPortSuccess, true);
            }
        }
    }

private:
    GridMate::GridSearch* m_search;

    enum InputPorts
    {
        InputPortActivate = 0,
        InputPortServerName,
        InputPortMapName,
        InputPortMaxPlayers
    };

    enum OutputPorts
    {
        OutputPortSuccess = 0,
        OutputPortFailed,
        OutputPortResult
    };

    SActivationInfo m_activationInfo;
};

REGISTER_FLOW_NODE(g_GameLiftStartNodePath, FlowNode_GameLift_Start);
REGISTER_FLOW_NODE(g_GameLiftCreateGameSessionNodePath, FlowNode_GameLift_CreateGameSession);
REGISTER_FLOW_NODE(g_GameLiftListServersNodePath, FlowNode_GameLift_ListServers);


#endif //BUILD_GAMELIFT_CLIENT
