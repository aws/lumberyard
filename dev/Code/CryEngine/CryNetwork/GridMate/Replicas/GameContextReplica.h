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
#ifndef INCLUDE_GRIDMATEGAMECONTEXTREPLICA_HEADER
#define INCLUDE_GRIDMATEGAMECONTEXTREPLICA_HEADER

#pragma once

#include "../NetworkGridmateMarshaling.h"
#include "../Compatibility/GridMateNetSerialize.h"
#include "../Compatibility/GridMateRMI.h"
#include "../Replicas/GameContextReplica.h"

#include <SimpleSerialize.h>

namespace GridMate
{
    /*!
     * Replica for synchronizing game context state (current level, data not specific to an entity).
     */
    class GameContextReplica
        : public GridMate::ReplicaChunk
        , public GridMate::SessionEventBus::Handler
    {
    public:

        friend class NetworkGridMate;

        GM_CLASS_ALLOCATOR(GameContextReplica);

        void SyncWithGame();

        GameContextReplica();
        virtual ~GameContextReplica();

        //////////////////////////////////////////////////////////////////////
        //! GridMate::ReplicaChunk overrides.
        static const char* GetChunkName() { return "GameContextReplica"; }
        void UpdateChunk(const GridMate::ReplicaContext& rc) override;
        void UpdateFromChunk(const GridMate::ReplicaContext& rc) override;
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override;
        bool IsReplicaMigratable() override;
        bool IsUpdateFromReplicaEnabled() override;
        //////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////
        //! GridMate::SessionEventBus::Handler
        void OnMemberJoined(GridSession* session, GridMember* member) override;
        void OnMemberLeaving(GridSession* session, GridMember* member) override;
        //////////////////////////////////////////////////////////////////////

        void BindGameContext(IGameContext* gameContext);

        //! Notifies the server that a particular client (w/ channel id) has completed loading a map.
        //! In CryEngine, a map must be loaded for most actions to take place, including spawning a
        //! client's representative actor.
        bool OnMemberLoadedMap(const GridMate::RpcContext& rc);
        GridMate::Rpc<>::BindInterface<GameContextReplica, &GameContextReplica::OnMemberLoadedMap> RPCOnMemberLoadedMap;

        //! Notifies the server that a particular client has received the replica for his actor.
        bool OnActorReceived(const GridMate::RpcContext& rc);
        GridMate::Rpc<>::BindInterface<GameContextReplica, &GameContextReplica::OnActorReceived> RPCOnActorReceived;

        //! RPC for dispatching server ScriptRMIs (shim).
        bool HandleServerScriptRMI(RMI::ScriptInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<RMI::ScriptInvocationWrapper::Ptr, RMI::ScriptInvocationWrapper::Marshaler>>
            ::BindInterface<GameContextReplica, &GameContextReplica::HandleServerScriptRMI> RPCHandleServerScriptRMI;

        //! RPC for dispatching client ScriptRMIs (shim).
        bool HandleClientScriptRMI(RMI::ScriptInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);
        GridMate::Rpc <GridMate::RpcArg<RMI::ScriptInvocationWrapper::Ptr, RMI::ScriptInvocationWrapper::Marshaler>>
            ::BindInterface<GameContextReplica, &GameContextReplica::HandleClientScriptRMI, RMI::ClientRMITraits> RPCHandleClientScriptRMI;

    private:
        //! Notifies game layer that a client is joined and ready.
        void ProcessReadyClient(ChannelId memberChannelId, IGameRules* gameRules);

        //! Callback listener for changing game params.
        void OnGameParamsChanged();

    private:
        //! Clients who have reported readiness to the server (map loaded, ready for game acknowledgement).
        vector<ChannelId> m_readyClients;

        IGameContext* m_gameContext;
    };
} // namespace GridMate

#endif // INCLUDE_GRIDMATEGAMECONTEXTREPLICA_HEADER
