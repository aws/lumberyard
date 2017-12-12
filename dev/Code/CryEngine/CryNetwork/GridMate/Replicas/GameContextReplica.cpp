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
#include "StdAfx.h"

#include "ILevelSystem.h"

#include "../NetworkGridMate.h"
#include "../NetworkGridmateDebug.h"
#include "../NetworkGridmateMarshaling.h"
#include "GameContextReplica.h"

namespace GridMate
{
    //-----------------------------------------------------------------------------
    GameContextReplica::GameContextReplica()
        : RPCOnMemberLoadedMap("RPCOnMemberLoadedMap")
        , RPCOnActorReceived("RPCOnActorReceived")
        , RPCHandleServerScriptRMI("RPCHandleServerScriptRMI")
        , RPCHandleClientScriptRMI("RPCHandleClientScriptRMI")
    {
    }

    //-----------------------------------------------------------------------------
    GameContextReplica::~GameContextReplica()
    {
        Network::Get().SetGameContextReplica(nullptr);

        if (GridMate::SessionEventBus::Handler::BusIsConnected())
        {
            GridMate::SessionEventBus::Handler::BusDisconnect();
        }
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::SyncWithGame()
    {
        GM_ASSERT_TRACE(gEnv->mMainThreadId == CryGetCurrentThreadId(),
            "Game context sync should only be called from main thread, and never during map load.");

        // The server needs to notify the game layer of any clients who have reported
        // full readiness (map loaded, etc).
        if (IsMaster())
        {
            if (!m_readyClients.empty())
            {
                if (IGameRules* gameRules = GetGameRules())
                {
                    for (ChannelId memberChannelId : m_readyClients)
                    {
                        ProcessReadyClient(memberChannelId, gameRules);
                    }

                    m_readyClients.clear();
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::OnReplicaActivate(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        GM_DEBUG_TRACE("GameContextReplica::OnActivate");
        SetPriority(k_replicaPriorityRealTime);

        Network& net = Network::Get();

        if (IsMaster())
        {
            if (false == GridMate::SessionEventBus::Handler::BusIsConnected())
            {
                GridMate::SessionEventBus::Handler::BusConnect(net.GetGridMate());
            }
        }

        net.SetGameContextReplica(this);
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::UpdateChunk(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::UpdateFromChunk(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::OnReplicaDeactivate(const GridMate::ReplicaContext& rc)
    {
        (void)rc;
        GM_DEBUG_TRACE("GameContextReplica::OnDeactivate");

        Network::Get().SetGameContextReplica(nullptr);
    }

    //-----------------------------------------------------------------------------
    bool GameContextReplica::IsReplicaMigratable()
    {
        return false;
    }
    //-----------------------------------------------------------------------------
    bool GameContextReplica::IsUpdateFromReplicaEnabled()
    {
        return !static_cast<GridMate::Network&>(GridMate::Network::Get()).IsInMinimalUpdate();
    }
    //-----------------------------------------------------------------------------
    bool GameContextReplica::OnMemberLoadedMap(const GridMate::RpcContext& rc)
    {
        ChannelId memberChannelId = static_cast<ChannelId>(rc.m_sourcePeer);
        GM_DEBUG_TRACE("Client with id %u has reported map load completion.", memberChannelId);

        m_readyClients.push_back(memberChannelId);

        // No need to pass on.
        return false;
    }

    //-----------------------------------------------------------------------------
    bool GameContextReplica::OnActorReceived(const GridMate::RpcContext& rc)
    {
        ChannelId memberChannelId = static_cast<ChannelId>(rc.m_sourcePeer);
        GM_DEBUG_TRACE("Client with id %u has received his actor.", memberChannelId);

        IGameRules* gameRules = GetGameRules();

        GM_ASSERT_TRACE(gameRules, "Server-side game rules should already be created by the time clients have loaded maps.");

        if (gEnv->bServer && gameRules && gEnv->pGame)
        {
            if (gEnv->IsClient() || memberChannelId != Network::Get().GetLocalChannelId())
            {
                gameRules->OnClientEnteredGame(memberChannelId, false);
            }
        }

        // No need to pass on.
        return false;
    }

    //-----------------------------------------------------------------------------
    bool GameContextReplica::HandleServerScriptRMI(RMI::ScriptInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        AZ_Assert(IsMaster(), "HandleServerScriptRMI should only ever execute on the server!");
        if (IsMaster())
        {
            RMI::HandleScript(invocation, rc);
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    bool GameContextReplica::HandleClientScriptRMI(RMI::ScriptInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        return RMI::HandleScript(invocation, rc);
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::ProcessReadyClient(ChannelId memberChannelId, IGameRules* gameRules)
    {
        GM_ASSERT_TRACE(gameRules,
            "Server-side game rules should already be created by the time "
            "we're processing ready clients.");

        // The server shouldn't create an actor for himself unless he too is a client (listen server).
        if (gEnv->IsClient() || memberChannelId != Network::Get().GetLocalChannelId())
        {
            gameRules->OnClientConnect(memberChannelId, false);
            // Client actor should have been created by now,
            // we'll wait for client to ACK his actor creation (OnActorReceived).
        }
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::OnGameParamsChanged()
    {
        GridSession* session = Network::Get().GetCurrentSession();

        // Update the session search parameters with the new values.
        if (session && m_gameContext)
        {
            GridSessionParam param;

            param.m_id = "playerCount";
            param.SetValue(m_gameContext->GetPlayerCount());
            session->SetParam(param);
        }
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::OnMemberJoined(GridSession* session, GridMember* member)
    {
        (void)session;
        (void)member;

        OnGameParamsChanged();
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::OnMemberLeaving(GridSession* session, GridMember* member)
    {
        (void)session;
        (void)member;

        OnGameParamsChanged();
    }

    //-----------------------------------------------------------------------------
    void GameContextReplica::BindGameContext(IGameContext* gameContext)
    {
        m_gameContext = gameContext;
    }
} // namespace GridMate
