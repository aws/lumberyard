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

#if BUILD_GAMELIFT_SERVER

#include <GameLift/Session/GameLiftServerSession.h>
#include <GameLift/Session/GameLiftServerService.h>
#include <GridMate/Online/UserServiceTypes.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>

#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Carrier/Utils.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>

#include <aws/gamelift/server/GameLiftServerAPI.h>

namespace GridMate
{
    class GameLiftMember;

    //-----------------------------------------------------------------------------
    // GameLiftSessionReplica
    //-----------------------------------------------------------------------------
    class GameLiftSessionReplica
        : public Internal::GridSessionReplica
    {
    public:
        class GameLiftSessionReplicaDesc
            : public ReplicaChunkDescriptor
        {
        public:
            GameLiftSessionReplicaDesc()
                : ReplicaChunkDescriptor(GameLiftSessionReplica::GetChunkName(), sizeof(GameLiftSessionReplica))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext&) override
            {
                AZ_Assert(false, "GameLiftSessionReplica should never be created from stream on the server!");
                return nullptr;
            }

            void DiscardCtorStream(UnmarshalContext&) override {}
            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }
            void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override {}
        };

        GM_CLASS_ALLOCATOR(GameLiftSessionReplica);

        static const char* GetChunkName() { return "GridMate::GameLiftSessionReplica"; }

        GameLiftSessionReplica(GameLiftServerSession* session)
            : GridSessionReplica(session)
        {
        }
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftMemberID
    //-----------------------------------------------------------------------------
    class GameLiftMemberID
        : public MemberID
    {
    public:
        //-----------------------------------------------------------------------------
        // Marshaler
        //-----------------------------------------------------------------------------
        class Marshaler
        {
        public:
            void Marshal(WriteBuffer& wb, const GameLiftMemberID& id) const
            {
                wb.Write(id.m_id);
            }

            void Unmarshal(GameLiftMemberID& id, ReadBuffer& rb) const
            {
                rb.Read(id.m_id);
            }
        };
        //-----------------------------------------------------------------------------

        GameLiftMemberID()
            : m_id(0)
        {
        }

        GameLiftMemberID(const string& address, AZ::u32 memberId)
            : m_address(address)
            , m_id(memberId)
        {
            AZ_Assert(m_id != 0, "Invalid member id");
        }

        string ToString() const override { return string::format("%08X", m_id); }
        string ToAddress() const override { return m_address; }
        MemberIDCompact Compact() const override { return m_id; }
        bool IsValid() const override { return !m_address.empty() && m_id != 0; }

    private:
        string m_address;
        AZ::u32 m_id;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // GameLiftMemberInfoCtorContext
    //-----------------------------------------------------------------------------
    struct GameLiftMemberInfoCtorContext
        : public CtorContextBase
    {
        CtorDataSet<GameLiftMemberID, GameLiftMemberID::Marshaler> m_memberId;
        CtorDataSet<RemotePeerMode> m_peerMode;
        CtorDataSet<bool> m_isHost;
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftMemberState
    //-----------------------------------------------------------------------------
    class GameLiftMemberState
        : public Internal::GridMemberStateReplica
    {
    public:
        GM_CLASS_ALLOCATOR(GameLiftMemberState);
        static const char* GetChunkName() { return "GameLiftMemberState"; }

        explicit GameLiftMemberState(GridMember* member = nullptr)
            : GridMemberStateReplica(member)
        {
        }
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftMember
    //-----------------------------------------------------------------------------
    class GameLiftMember
        : public GridMember
    {
        friend class GameLiftServerSession;

    public:
        class GameLiftMemberDesc
            : public ReplicaChunkDescriptor
        {
        public:
            GameLiftMemberDesc()
                : ReplicaChunkDescriptor(GameLiftMember::GetChunkName(), sizeof(GameLiftMember))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext&) override
            {
                AZ_Assert(false, "GameLiftMemberDesc should never be created from stream on the server!");
                return nullptr;
            }

            void DiscardCtorStream(UnmarshalContext& mc) override
            {
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);
            }

            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override
            {
                if (!static_cast<GameLiftMember*>(chunkInstance)->IsLocal())
                {
                    delete chunkInstance;
                }
            }

            void MarshalCtorData(ReplicaChunkBase* chunkInstance, WriteBuffer& wb) override
            {
                GameLiftMember* member = static_cast<GameLiftMember*>(chunkInstance);
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.m_memberId.Set(member->m_memberId);
                ctorContext.m_peerMode.Set(member->m_peerMode.Get());
                ctorContext.m_isHost.Set(member->IsHost());
                ctorContext.Marshal(wb);
            }
        };

        GM_CLASS_ALLOCATOR(GameLiftMember);

        static const char* GetChunkName() { return "GridMate::GameLiftMember"; }

        const PlayerId* GetPlayerId() const override
        {
            return nullptr;
        }

        const MemberID& GetId() const override
        {
            return m_memberId;
        }

        void SetPlayerSessionId(const char* playerSessionId) { AZ_Assert(playerSessionId, "Invalid player session id"); m_playerSessionId = playerSessionId; }
        const char* GetPlayerSessionId() const { return m_playerSessionId.c_str(); }

        /// Remote member ctor.
        GameLiftMember(ConnectionID connId, const GameLiftMemberID& memberId, GameLiftServerSession* session)
            : GridMember(memberId.Compact())
            , m_memberId(memberId)
        {
            m_session = session;
            m_connectionId = connId;
        }

        /// Local member ctor.
        GameLiftMember(const GameLiftMemberID& memberId, GameLiftServerSession* session)
            : GridMember(memberId.Compact())
            , m_memberId(memberId)
        {
            m_session = session;

            m_clientState = CreateReplicaChunk<GameLiftMemberState>(this);
            m_clientState->m_name.Set(memberId.ToString());

            m_clientStateReplica = Replica::CreateReplica(memberId.ToString().c_str());
            m_clientStateReplica->AttachReplicaChunk(m_clientState);
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            if (IsMaster() && !IsLocal())
            {
                Aws::GameLift::GenericOutcome outcome = Aws::GameLift::Server::RemovePlayerSession(m_playerSessionId.c_str());
                if (!outcome.IsSuccess())
                {
                    AZ_TracePrintf("GameLift", "[SERVER SESSION] Failed to disconnect a master non-local GameLift player:'%s' with id=%s\n", outcome.GetError().GetErrorName().c_str(), GetId().ToString().c_str());
                }
                else
                {
                    AZ_TracePrintf("GameLift", "[SERVER SESSION] Sucessfully disconnected a master non-local GameLift player with id=%s\n", GetId().ToString().c_str());
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "[SERVER SESSION] Deactivating a gridmember, memberid %d", rc.m_peer ? rc.m_peer->GetId() : 0);
            }

            GridMember::OnReplicaDeactivate(rc);
        }

        using GridMember::SetHost;

        GameLiftMemberID m_memberId;
        string m_playerSessionId;
    };
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftServerSession
    //-----------------------------------------------------------------------------
    GameLiftServerSession::GameLiftServerSession(GameLiftServerService* service)
        : GridSession(service)
    {
    }

    bool GameLiftServerSession::Initialize(const GameLiftSessionParams& params, const CarrierDesc& carrierDesc)
    {
        AZ_Assert(params.m_gameSession, "No game session instance specified.");

        if (!GridSession::Initialize(carrierDesc))
        {
            return false;
        }

        m_myMember = CreateLocalMember(true, true, Mode_Peer);
        m_sessionParams = params;
        m_sessionParams.m_gameSession = nullptr;

        const auto& properties = params.m_gameSession->GetGameProperties();
        GameLiftSessionReplica::ParamContainer sessionProperties;
        for (AZStd::size_t i = 0; i < properties.size(); ++i)
        {
            GridSessionParam param;
            param.m_id = properties[i].GetKey().c_str();
            param.m_value = properties[i].GetValue().c_str();
            sessionProperties.push_back(param);
        }

        m_sessionParams.m_numPublicSlots = params.m_gameSession->GetMaximumPlayerSessionCount() + 1;    //+1 for dedicated server

        //////////////////////////////////////////////////////////////////////////
        // start up the session state we will bind it later
        AZ_Assert(m_sessionParams.m_numPublicSlots < 0xff && m_sessionParams.m_numPrivateSlots < 0xff, "Can't have more than 255 slots!");
        AZ_Assert(m_sessionParams.m_numPublicSlots > 0 || m_sessionParams.m_numPrivateSlots > 0, "You don't have any slots open!");

        GameLiftSessionReplica* state = CreateReplicaChunk<GameLiftSessionReplica>(this);
        state->m_numFreePrivateSlots.Set(static_cast<unsigned char>(m_sessionParams.m_numPrivateSlots));
        state->m_numFreePublicSlots.Set(static_cast<unsigned char>(m_sessionParams.m_numPublicSlots));
        state->m_peerToPeerTimeout.Set(m_sessionParams.m_peerToPeerTimeout);
        state->m_flags.Set(m_sessionParams.m_flags);
        state->m_topology.Set(m_sessionParams.m_topology);
        state->m_params.Set(sessionProperties);
        m_state = state;
        //////////////////////////////////////////////////////////////////////////

        m_sessionId = params.m_gameSession->GetGameSessionId().c_str();

        SetUpStateMachine();
        RequestEvent(SE_HOST);

        return true;
    }

    GridMember* GameLiftServerSession::CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode)
    {
        (void)isInvited;
        AZ_Assert(isHost, "GameLiftServerSession can only run as host!");
        AZ_Assert(!m_myMember, "We already have added a local member!");

        string ip = Utils::GetMachineAddress(m_carrierDesc.m_familyType);
        string address = SocketDriverCommon::IPPortToAddressString(ip.c_str(), m_carrierDesc.m_port);

        GameLiftMemberID myId(address, AZ::Crc32("GameLiftServer"));

        GameLiftMember* member = CreateReplicaChunk<GameLiftMember>(myId, this);
        member->SetHost(isHost);
        member->m_peerMode.Set(peerMode);
        return member;
    }

    void GameLiftServerSession::Shutdown()
    {
        GridSession::Shutdown();

        Aws::GameLift::GenericOutcome outcome = Aws::GameLift::Server::TerminateGameSession();
        AZ_Warning("GridMate", outcome.IsSuccess(), "GameLift session failed to terminate: %s:%s\n",
            outcome.GetError().GetErrorName().c_str(),
            outcome.GetError().GetErrorMessage().c_str());
    }

    GridMember* GameLiftServerSession::CreateRemoteMember(const string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId)
    {
        string playerSessionId;
        data.Read(playerSessionId);

        Aws::GameLift::GenericOutcome outcome = Aws::GameLift::Server::AcceptPlayerSession(playerSessionId.c_str());
        if (!outcome.IsSuccess())
        {
            AZ_TracePrintf("GameLift", "Failed to connect GameLift player:'%s' with id=%s\n", outcome.GetError().GetErrorName().c_str(), playerSessionId.c_str());
            m_carrier->Disconnect(connId);
            return nullptr;
        }

        GameLiftMemberID memberId(address, AZ::Crc32(playerSessionId.c_str()));
        GameLiftMember* member = CreateReplicaChunk<GameLiftMember>(connId, memberId, this);
        member->m_peerMode.Set(peerMode);
        member->SetPlayerSessionId(playerSessionId.c_str());
        return member;
    }

    bool GameLiftServerSession::OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        bool isProcessed = GridSession::OnStateCreate(sm, e);

        switch (e.id)
        {
        case AZ::HSM::EnterEventId:
        {
            Aws::GameLift::GenericOutcome activationOutcome = Aws::GameLift::Server::ActivateGameSession();
            if (!activationOutcome.IsSuccess())
            {
                AZ_TracePrintf("GridMate", "GameLift session activation failed: %s\n", activationOutcome.GetError().GetErrorMessage().c_str());
                RequestEvent(SE_DELETE);
            }
            else
            {
                RequestEvent(SE_CREATED);
            }
        }
        return true;
        }

        return isProcessed;
    }

    bool GameLiftServerSession::OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        bool isProcessed = GridSession::OnStateDelete(sm, e);

        switch (e.id)
        {
            case AZ::HSM::EnterEventId:
            {
                RequestEvent(SE_DELETED);
                return true;
            }
        }

        return isProcessed;
    }

    bool GameLiftServerSession::OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        (void)sm;
        (void)e;

        AZ_Assert(false, "Host migration is not supported for GameLift sessions.");
        return false;
    }

    void GameLiftServerSession::RegisterReplicaChunks()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftSessionReplica, GameLiftSessionReplica::GameLiftSessionReplicaDesc>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftMember, GameLiftMember::GameLiftMemberDesc>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftMemberState>();
    }
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER
