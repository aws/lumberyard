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

#include "IGameRulesSystem.h"
#include "ILevelSystem.h"

#include "NetworkGridMate.h"
#include "NetworkGridmateDebug.h"

#include "Replicas/EntityReplica.h"
#include "Replicas/EntityScriptReplicaChunk.h"
#include "Replicas/GameContextReplica.h"
#include "Compatibility/GridMateNetSerialize.h"
#include "Compatibility/GridMateRMI.h"

#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/BasicHostChunkDescriptor.h>

#include "Components/IComponentUser.h"

#include "NetworkGridMateEntityEventBus.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define NETWORKGRIDMATE_CPP_SECTION_1 1
#define NETWORKGRIDMATE_CPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION NETWORKGRIDMATE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(NetworkGridMate_cpp, AZ_RESTRICTED_PLATFORM)
#endif

namespace GridMate
{
    //-----------------------------------------------------------------------------
    Network* Network::s_instance = nullptr;

    int Network::s_StatsIntervalMS          = 1000;     // 1 second by default.
    int Network::s_DumpStatsEnabled         = 0;

    FILE* Network::s_DumpStatsFile          = nullptr;

    //-----------------------------------------------------------------------------
    Network::Network()
        : m_localChannelId(kOfflineChannelId)
        , m_gameContext(nullptr)
        , m_gridMate(nullptr)
        , m_session(nullptr)
        , m_gameContextReplica(nullptr)
        , m_levelLoadState(LevelLoadState_None)
        , m_allowMinimalUpdate(false)
    {
        s_instance = this;
        m_legacySerializeProvider = this;

        m_postFrameTasks.reserve(32);
    }

    //-----------------------------------------------------------------------------
    Network::~Network()
    {
#if GRIDMATE_DEBUG_ENABLED
        Debug::UnregisterCVars();
#endif

        if (GetLevelSystem())
        {
            GetLevelSystem()->RemoveListener(this);
        }

        if (gEnv && gEnv->pEntitySystem)
        {
            gEnv->pEntitySystem->RemoveSink(this);
        }

        m_activeEntityReplicaMap.clear();
        m_newProxyEntities.clear();

        ShutdownGridMate();

        if (s_DumpStatsFile)
        {
            fclose(s_DumpStatsFile);
            s_DumpStatsFile = nullptr;
        }

        s_instance = nullptr;
    }

    //-----------------------------------------------------------------------------
    bool Network::Init(int ncpu)
    {
        // Register for entity system callbacks.
        GM_ASSERT_TRACE(gEnv && gEnv->pEntitySystem, "Entity system should already be initialized.");
        if (gEnv && gEnv->pEntitySystem)
        {
            gEnv->pEntitySystem->AddSink(this, IEntitySystem::OnSpawn | IEntitySystem::OnRemove, 0);
        }

#if GRIDMATE_DEBUG_ENABLED
        Debug::RegisterCVars();
#endif

        StartGridMate();
        MarkAsLocalOnly();

        return true;
    }

    //-----------------------------------------------------------------------------
    Network& Network::Get()
    {
        GM_ASSERT_TRACE(s_instance, "Network interface has not yet been created.");
        return *s_instance;
    }

    //-----------------------------------------------------------------------------
    void Network::Release()
    {
        delete this;
    }

    //-----------------------------------------------------------------------------
    void Network::SetGameContext(IGameContext* gameContext)
    {
        if (GetLevelSystem())
        {
            // try to add ourselves (in case we are not already)
            GetLevelSystem()->AddListener(this);
        }

        m_gameContext = gameContext;

        if (m_gameContextReplica)
        {
            m_gameContextReplica->BindGameContext(gameContext);
        }
    }

    //-----------------------------------------------------------------------------
    bool Network::AllowEntityCreation() const
    {
        // Disallow entity creation during level transitions.
        if (m_gameContextReplica)
        {
            return m_levelLoadState == LevelLoadState_Loaded;
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    void Network::SetGameContextReplica(GameContextReplica* contextReplica)
    {
        m_gameContextReplica = contextReplica;
        if (contextReplica)
        {
            contextReplica->BindGameContext(m_gameContext);
        }
    }

    //-----------------------------------------------------------------------------
    bool Network::IsInMinimalUpdate() const
    {
        return m_allowMinimalUpdate;
    }

    //-----------------------------------------------------------------------------
    void Network::SyncWithGame(ENetworkGameSync syncType)
    {
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

        switch (syncType)
        {
        case eNGS_FrameStart:
        {
            UpdateGridMate(syncType);
        }
        break;

        case eNGS_FrameEnd:
        {
            FlushPostFrameTasks();

            UpdateGridMate(syncType);

            if (m_gameContextReplica)
            {
                FRAME_PROFILER("GameContextReplica Update", GetISystem(), PROFILE_NETWORK);
                m_gameContextReplica->SyncWithGame();
            }

            UpdateNetworkStatistics();

            DebugDraw();
        }
        break;

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Inherited from CryNetwork, this mechanism is required for safe updating during loading.
        // During such time, the network is pumped via the NetworkStallerTicker thread, and this flag
        // basically describes when it's safe for network messages to be distributed to the game.
        case eNGS_AllowMinimalUpdate:
        {
            m_allowMinimalUpdate = true;
            m_levelLoadState = LevelLoadState_Loading;
        }
        break;

        case eNGS_DenyMinimalUpdate:
        {
            m_allowMinimalUpdate = false;
            m_levelLoadState = LevelLoadState_Loaded;
        }
        break;

        case eNGS_MinimalUpdateForLoading:
        {
            if (m_allowMinimalUpdate)
            {
                UpdateGridMate(syncType);
            }
        }
        break;
            /////////////////////////////////////////////////////////////////////////////////////////////
        }
    }

    //-----------------------------------------------------------------------------
    void Network::FlushPostFrameTasks()
    {
        BindNewEntitiesToNetwork();

        for (const Task& task : m_postFrameTasks)
        {
            task();
        }

        RMI::FlushQueue();

        m_postFrameTasks.clear();
    }

    //-----------------------------------------------------------------------------
    void Network::UpdateGridMate(ENetworkGameSync syncType)
    {
        if (m_gridMate && m_mutexUpdatingGridMate.try_lock() )
        {
            FRAME_PROFILER("GridMate Update", GetISystem(), PROFILE_NETWORK);

            GridMate::ReplicaManager* replicaManager = GetCurrentSession() ? GetCurrentSession()->GetReplicaMgr() : nullptr;
            if (replicaManager)
            {
                switch (syncType)
                {
                    case eNGS_MinimalUpdateForLoading:
                    case eNGS_FrameStart:
                    {
                        if (replicaManager)
                        {
                            replicaManager->Unmarshal();
                            replicaManager->UpdateFromReplicas();
                        }

                        // When called from the network stall ticker thread, marshalling should be performed as well.
                        if (syncType != eNGS_MinimalUpdateForLoading)
                        {
                            break;
                        }
                    }

                    case eNGS_FrameEnd:
                    {
                        if (replicaManager)
                        {
                            replicaManager->UpdateReplicas();
                            replicaManager->Marshal();
                        }
                        break;
                    }
                    default: break;
                }
            }

            m_gridMate->Update();
            m_mutexUpdatingGridMate.unlock();
        }
    }

    //-----------------------------------------------------------------------------
    ChannelId Network::GetChannelIdForSessionMember(GridMate::GridMember* member) const
    {
        return member ? ChannelId(member->GetIdCompact()) : kInvalidChannelId;
    }

    //-----------------------------------------------------------------------------
    void Network::ChangedAspects(EntityId entityId, NetworkAspectType aspectBits)
    {
        if (aspectBits == 0)
        {
            return; // nothing to do
        }

#ifndef _RELEASE
        for (size_t i = NetSerialize::kNumAspectSlots; i < NUM_ASPECTS; ++i)
        {
            if (BIT64(i) & aspectBits)
            {
                GM_ASSERT_TRACE(0, "Any aspects >= %u can not be serialized through this layer, until support for > 32 data sets is enabled.", static_cast<uint32>(NetSerialize::kNumAspectSlots));
                break;
            }
        }
#endif

        EntityReplica* replica = FindEntityReplica(entityId);
        if (replica)
        {
            if (replica->IsMaster() || replica->IsAspectDelegatedToThisClient())
            {
                NetworkAspectType oldDirtyAspects = replica->GetDirtyAspects();
                replica->MarkAspectsDirty(aspectBits);

                if (replica->IsAspectDelegatedToThisClient())
                {
                    // Only add the task if these are the first aspects being dirtied.
                    if (oldDirtyAspects == 0)
                    {
                        m_postFrameTasks.push_back(
                            [=]()
                            {
                                EntityReplica* rep = FindEntityReplica(entityId);
                                if (rep)
                                {
                                    rep->UploadClientDelegatedAspects();
                                }
                            }
                            );
                    }
                }
            }
        }
        else
        {
            GM_DEBUG_TRACE("Failed to mark aspects dirty because replica for "
                "entity id %u could not be found.", entityId);
        }
    }

    //-----------------------------------------------------------------------------
    ChannelId Network::GetLocalChannelId() const
    {
        return m_localChannelId;
    }

    //-----------------------------------------------------------------------------
    ChannelId Network::GetServerChannelId() const
    {
        if (m_session)
        {
            return GetChannelIdForSessionMember(m_session->GetHost());
        }

        return m_localChannelId;
    }

    //-----------------------------------------------------------------------------
    EntityId Network::LocalEntityIdToServerEntityId(EntityId localId) const
    {
        if (!gEnv->bServer)
        {
            // \todo - Optimize. Keep a local->server id map locally. We already have server->local
            // via m_activeEntityReplicaMap.
            for (auto& replicaEntry : m_activeEntityReplicaMap)
            {
                if (replicaEntry.second->GetLocalEntityId() == localId)
                {
                    return replicaEntry.first;
                }
            }

            return kInvalidEntityId;
        }

        return localId;
    }

    //-----------------------------------------------------------------------------
    EntityId Network::ServerEntityIdToLocalEntityId(EntityId serverId, bool allowForcedEstablishment /*= false*/) const
    {
        EntityId localId = kInvalidEntityId;

        if (gEnv->bServer)
        {
            localId = serverId;
        }
        else
        {
            auto foundAt = m_activeEntityReplicaMap.find(serverId);
            if (foundAt != m_activeEntityReplicaMap.end())
            {
                EntityReplicaPtr replica = foundAt->second;

                localId = replica->GetLocalEntityId();
            }
            else if (allowForcedEstablishment)
            {
                AZ_Assert(AllowEntityCreation(), "Entity creation is not allowed during level loads! Forcing creation is going to cause problems!");

                // If we're deserializing this entity Id via the 'eid' policy, but the local entity is not
                // yet established, expedite establishment. This is to ensure we can properly map/decode
                // the entity Id mid-serialization.
                auto newProxy = m_newProxyEntities.find(serverId);
                if (newProxy != m_newProxyEntities.end())
                {
                    EntityReplicaPtr replica = newProxy->second;
                    localId = replica->HandleNewlyReceivedNow();
                }
            }
        }

        return localId;
    }

    //-----------------------------------------------------------------------------
    void Network::InvokeRMI(IGameObject* gameObject, IRMIRep& rep, void* params, ChannelId targetChannelFilter, uint32 where)
    {
        RMI::InvokeLegacy(gameObject, rep, params, targetChannelFilter, where);
    }

    //-----------------------------------------------------------------------------
    void Network::InvokeActorRMI(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep)
    {
        RMI::InvokeActor(entityId, actorExtensionId, targetChannelFilter, rep);
    }

    //-----------------------------------------------------------------------------
    void Network::InvokeScriptRMI(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId, ChannelId avoidChannelId)
    {
        RMI::InvokeScript(serializable, isServerRMI, toChannelId, avoidChannelId);
    }

    //-----------------------------------------------------------------------------
    void Network::RegisterActorRMI(IActorRMIRep* rep)
    {
        RMI::RegisterActorRMI(rep);
    }

    //-----------------------------------------------------------------------------
    void Network::UnregisterActorRMI(IActorRMIRep* rep)
    {
        RMI::UnregisterActorRMI(rep);
    }

    //-----------------------------------------------------------------------------
    void Network::SetDelegatableAspectMask(NetworkAspectType aspectBits)
    {
        NetSerialize::SetDelegatableAspects(aspectBits);
    }

    //-----------------------------------------------------------------------------
    void Network::SetObjectDelegatedAspectMask(EntityId entityId, NetworkAspectType aspects, bool set)
    {
        m_postFrameTasks.push_back(
            [=]()
            {
                if (EntityReplica* entityReplica = FindEntityReplica(entityId))
                {
                    NetworkAspectType mask = entityReplica->GetClientDelegatedAspectMask();

                    if (set)
                    {
                        mask |= aspects;
                    }
                    else
                    {
                        mask &= ~aspects;
                    }

                    entityReplica->SetClientDelegatedAspectMask(mask);
                }
                else
                {
                    GM_DEBUG_TRACE("Failed to update aspect delegation mask because replica"
                        "for entity id %u could not be found.", entityId);
                }
            }
            );
    }

    //-----------------------------------------------------------------------------
    void Network::DelegateAuthorityToClient(EntityId entityId, ChannelId clientChannelId)
    {
        GridMate::EntityReplica* replica = FindEntityReplica(entityId);
        if (replica)
        {
            replica->RPCDelegateAuthorityToOwner(clientChannelId);
        }
    }

    void Network::ShutdownGridMate()
    {
        if (m_gridMate)
        {
            GM_DEBUG_TRACE("Shutting down GridMate network.");

            m_postFrameTasks.clear();
            RMI::EmptyQueue();

            GridMateDestroy(m_gridMate);
            m_gridMate = nullptr;

            if (m_sessionEvents.IsConnected())
            {
                m_sessionEvents.Disconnect();
            }

            if (m_systemEvents.IsConnected())
            {
                m_systemEvents.Disconnect();
            }

            // TEMP: Moved GridMateAllocatorMP creation into AzFramework::Application due to a shutdown issue
            // with allocator environment variables. AzFramework::Application will create GridMateAllocatorMP from
            // the main process.
            // See LMBR-20396 for more details.
            //AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
        }
    }

    //-----------------------------------------------------------------------------
    EntityReplica* Network::FindEntityReplica(EntityId id) const
    {
        if (!gEnv->bServer)
        {
            // Replicas are mapped by server-side entity Id, and we map back and forth
            // to reconcile across server and clients.
            // Upon deserializing via 'eid' policy, server-side Ids are converted back
            // to local.
            id = LocalEntityIdToServerEntityId(id);
        }

        auto replicaIter = m_activeEntityReplicaMap.find(id);

        if (replicaIter != m_activeEntityReplicaMap.end())
        {
            return replicaIter->second.get();
        }

        return nullptr;
    }

    //-----------------------------------------------------------------------------
    void Network::StartGridMate()
    {
        if (nullptr != m_gridMate)
        {
            return;
        }

        GridMateDesc desc;
        m_gridMate = GridMateCreate(desc);

        // Initializing GridMate multiplayer service allocator
        // TEMP: Moved GridMateAllocatorMP creation into AzFramework::Application due to a shutdown issue
        // with allocator environment variables. AzFramework::Application will create GridMateAllocatorMP from
        // the main process.
        // See LMBR-20396 for more details.
        //GridMate::GridMateAllocatorMP::Descriptor allocDesc;
        //allocDesc.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        //AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create(allocDesc);

        // Monitor session events.
        GM_ASSERT_TRACE(!m_sessionEvents.IsConnected(), "Session events bus should not be connected yet.");
        m_sessionEvents.Connect(m_gridMate);

        // Monitor internal system events.
        GM_ASSERT_TRACE(!m_systemEvents.IsConnected(), "System events bus should not be connected yet.");
        m_systemEvents.Connect();

        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(EntityReplica::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<EntityReplica, EntityReplica::EntityReplicaDesc>();
        }

        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(EntityScriptReplicaChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<EntityScriptReplicaChunk>();
        }

        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(GameContextReplica::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameContextReplica, GridMate::BasicHostChunkDescriptor<GameContextReplica>>();
        }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION NETWORKGRIDMATE_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(NetworkGridMate_cpp, AZ_RESTRICTED_PLATFORM)
#endif
    }

    //-----------------------------------------------------------------------------
    void Network::OnLoadingComplete(ILevel* level)
    {
        if (level != nullptr)
        {
            if (m_gameContextReplica)
            {
                // Notify the server we've successfully loaded the map, so our representative
                // actor can be created.
                m_gameContextReplica->RPCOnMemberLoadedMap();
            }
            else if (gEnv->IsClient() || gEnv->IsEditor())
            {
                // This is the single player case. Trigger local actor spawn.
                if (IGameRules* gameRules = GetGameRules())
                {
                    gameRules->OnClientConnect(m_localChannelId, false);    // Can become part of GameContext once GameContext is directly bound to its replica (so it can tell if it is in MP or SP).
                    gameRules->OnClientEnteredGame(m_localChannelId, false);    // Should be triggered by actor creation on the client, and forwarded by the local GameContext to the server
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    void Network::OnUnloadComplete(ILevel* level)
    {
        m_levelLoadState = LevelLoadState_None;
        m_activeEntityReplicaMap.clear();
        m_newServerEntities.clear();
    }

    //-----------------------------------------------------------------------------
    CTimeValue Network::GetSessionTime()
    {
        CTimeValue t = gEnv->pTimer->GetFrameStartTime();
        if (m_session)
        {
            t.SetMilliSeconds(m_session->GetTime());
        }
        return t;
    }
    //-----------------------------------------------------------------------------
    void Network::UpdateNetworkStatistics()
    {
        static float s_lastUpdate = 0.f;

        const float time = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);
        if (time >= s_lastUpdate + (s_StatsIntervalMS * 0.001f))
        {
            FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

            s_lastUpdate = time;

            if (m_session)
            {
                Carrier* carrier = m_session->GetCarrier();

                for (unsigned int i = 0; i < m_session->GetNumberOfMembers(); ++i)
                {
                    GridMember* member = m_session->GetMemberByIndex(i);

                    if (member == m_session->GetMyMember())
                    {
                        continue;
                    }

                    const ConnectionID connId = member->GetConnectionId();

                    if (connId != InvalidConnectionID)
                    {
                        TrafficControl::Statistics stats;
                        carrier->QueryStatistics(connId, &stats);

                        CarrierStatistics& memberStats =
                            m_statisticsPerChannel[ GetChannelIdForSessionMember(member) ];

                        memberStats.m_rtt = stats.m_rtt;
                        memberStats.m_packetLossRate = stats.m_packetLoss;
                        memberStats.m_totalReceivedBytes = stats.m_dataReceived;
                        memberStats.m_totalSentBytes = stats.m_dataSend;
                        memberStats.m_packetsLost = stats.m_packetLost;
                        memberStats.m_packetsReceived = stats.m_packetReceived;
                        memberStats.m_packetsSent = stats.m_packetSend;
                    }
                }
            }

            #if GRIDMATE_DEBUG_ENABLED
            if (s_DumpStatsEnabled > 0)
            {
                DumpNetworkStatistics();
                m_gameStatistics = GameStatistics();
            }
            #endif // GRIDMATE_DEBUG_ENABLED
        }
    }

    //-----------------------------------------------------------------------------
    void Network::ClearNetworkStatistics()
    {
        m_gameStatistics = GameStatistics();
        m_statisticsPerChannel.clear();
    }

    //-----------------------------------------------------------------------------
    GameStatistics& Network::GetGameStatistics()
    {
        return m_gameStatistics;
    }

    //-----------------------------------------------------------------------------
    CarrierStatistics Network::GetCarrierStatistics()
    {
        if (!m_statisticsPerChannel.empty())
        {
            return m_statisticsPerChannel.begin()->second;
        }

        return CarrierStatistics();
    }

    //-----------------------------------------------------------------------------
    // IEntitySystemSink callbacks.
    //-----------------------------------------------------------------------------
    void Network::OnSpawn(IEntity* pEntity, SEntitySpawnParams& params)
    {
        // Every time the game/engine spawns an entity, this is where we hear about it.
        // This is essentially where we handle "binding to the network."
        // If it's a networked entity, we simply spawn a replica for it.

        GM_DEBUG_TRACE("Entity Spawned: \"%s\" [%u]",
            pEntity ? pEntity->GetName() : "<null>",
            pEntity ? pEntity->GetId() : 0);

        if (gEnv->bServer)
        {
            // Ignore the entity if it doesn't need to be bound.
            if (!!(pEntity->GetFlags() & (ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_SERVER_ONLY)))
            {
                GM_DEBUG_TRACE("Skipping replica creation for entity \"%s\" [%d] because it's not marked for networking.",
                    pEntity ? pEntity->GetName() : "<null>",
                    pEntity ? pEntity->GetId() : 0);

                return;
            }

            const EntityId id = pEntity->GetId();

            // Pull the channel Id from the actor, if this is indeed an actor. We need to serialize
            // the owning channel Id, so when clients replicate the entity on their end, they know
            // if it's theirs or another remote client's representative actor.
            IActorSystem* actorSystem = GetActorSystem();
            IGameRulesSystem* gameRulesSystem = GetGameRulesSystem();
            IActor* actor = actorSystem ? actorSystem->GetActor(id) : nullptr;
            const ChannelId channelId = actor ? actor->GetChannelId() : kInvalidChannelId;

            if (channelId != kInvalidChannelId)
            {
                GM_DEBUG_TRACE("Entity %s is bound to channel Id %u",
                    pEntity->GetName(), channelId);
            }

            uint32 paramsFlags = EntitySpawnParamsStorage::kParamsFlag_None;

            if (gameRulesSystem && gameRulesSystem->GetCurrentGameRulesEntity() == pEntity)
            {
                paramsFlags |= EntitySpawnParamsStorage::kParamsFlag_IsGameRules;
            }
            else if (m_levelLoadState == LevelLoadState_Loading && channelId == kInvalidChannelId)
            {
                paramsFlags |= EntitySpawnParamsStorage::kParamsFlag_IsLevelEntity;
            }

            // Undocumented behavior maintained from old network spawn logic.
            // Evidently clients should not inherit this flag from the server.
            params.nFlags = (params.nFlags & ~ENTITY_FLAG_TRIGGER_AREAS);

            EntitySpawnParamsStorage paramsStorage(params, channelId, paramsFlags);

            if (m_session && m_session->IsHost())
            {
                GM_DEBUG_TRACE("Replica created for entity \"%s\" [%u]",
                    pEntity ? pEntity->GetName() : "<null>",
                    pEntity ? pEntity->GetId() : 0);

                m_newServerEntities[ paramsStorage.m_id ] = paramsStorage;
            }
            else if (!gEnv->IsEditor())
            {
                GM_DEBUG_TRACE("Replica creation skipped for entity \"%s\" [%u] because we're not in a session",
                    pEntity ? pEntity->GetName() : "<null>",
                    pEntity ? pEntity->GetId() : 0);
            }
        }
    }

    //-----------------------------------------------------------------------------
    bool Network::OnRemove(IEntity* pEntity)
    {
        GM_DEBUG_TRACE("Entity Removed: \"%s\" [%u]",
            pEntity ? pEntity->GetName() : "<null>",
            pEntity ? pEntity->GetId() : 0);

        const EntityId id = pEntity->GetId();

        // Ensure any deferred adds are canceled.
        m_newServerEntities.erase(id);

        for (auto iter = m_activeEntityReplicaMap.begin(); iter != m_activeEntityReplicaMap.end(); ++iter)
        {
            EntityReplicaPtr entityChunk = iter->second;
            if (entityChunk->GetLocalEntityId() == pEntity->GetId())
            {
                // Destroy the replica if we own it.
                if (entityChunk->IsMaster())
                {
                    GM_DEBUG_TRACE("Destroyed owned replica for removed entity \"%s\" [%u]",
                        pEntity ? pEntity->GetName() : "<null>",
                        pEntity ? pEntity->GetId() : 0);

                    entityChunk->GetReplica()->Destroy();
                }

                break;
            }
        }

        // Don't disable removal; we just wanted to intercept it.
        return true;
    }

    //-----------------------------------------------------------------------------
    void Network::BindNewEntitiesToNetwork()
    {
        if (m_session && m_session->IsHost())
        {
            // Create replicas for new spawned during level load.
            // Only create replica if the entity is still relevant.
            for (auto& newEntity : m_newServerEntities)
            {
                if (gEnv->pEntitySystem->GetEntity(newEntity.first))
                {
                    ReplicaPtr replica = Replica::CreateReplica(newEntity.second.m_entityName.c_str());
                    EntityReplica* entityChunk = CreateReplicaChunk<EntityReplica>(newEntity.second);
                    replica->AttachReplicaChunk(entityChunk);                    

                    EntityScriptReplicaChunk* scriptEntityChunk = CreateReplicaChunk<EntityScriptReplicaChunk>();
                    replica->AttachReplicaChunk(scriptEntityChunk);

                    EBUS_EVENT_ID(newEntity.first, NetworkGridMateEntityEventBus, OnEntityBoundToNetwork, replica);

                    m_session->GetReplicaMgr()->AddMaster(replica);
                }
            }
        }
        m_newServerEntities.clear();

        for (EntityReplicaMap::iterator iterNewProxy = m_newProxyEntities.begin(); iterNewProxy != m_newProxyEntities.end(); )
        {
            EntityReplicaPtr entityChunk = iterNewProxy->second;
            entityChunk->HandleNewlyReceivedNow();

            if ((entityChunk->GetFlags() & EntityReplica::kFlag_NewlyReceived) == 0)
            {
                iterNewProxy = m_newProxyEntities.erase(iterNewProxy);
            }
            else
            {
                ++iterNewProxy;
            }
        }
    }

    //-----------------------------------------------------------------------------
    void Network::GetBandwidthStatistics(SBandwidthStats* const pStats)
    {
        pStats->m_numChannels = m_statisticsPerChannel.size();

        if (!m_statisticsPerChannel.empty())
        {
            const auto& carrierStats = m_statisticsPerChannel.begin()->second;
            pStats->m_1secAvg.m_totalPacketsDropped = carrierStats.m_packetsLost;
            pStats->m_1secAvg.m_totalPacketsRecvd = carrierStats.m_packetsReceived;
            pStats->m_1secAvg.m_totalPacketsSent = carrierStats.m_packetsSent;
            pStats->m_1secAvg.m_totalBandwidthRecvd = carrierStats.m_totalReceivedBytes;
            pStats->m_1secAvg.m_totalBandwidthSent = carrierStats.m_totalSentBytes;
        }
    }

    //-----------------------------------------------------------------------------
    void Network::GetPerformanceStatistics(SNetworkPerformance* pSizer)
    {
        // Network Cpu stats.
    }

    //-----------------------------------------------------------------------------
    void Network::GetProfilingStatistics(SNetworkProfilingStats* const pStats)
    {
        pStats->m_maxBoundObjects = uint(~0);
        pStats->m_numBoundObjects = m_activeEntityReplicaMap.size();
        // pStats->m_ProfileInfoStats Part of NET_PROFILE macros
    }
    //! Called when need serializer for the legacy aspect
    void Network::AcquireSerializer(WriteBuffer& wb, NetSerialize::AcquireSerializeCallback callback)
    {
        NetSerialize::EntityNetSerializerCollectState serializerImpl(wb);
        CSimpleSerialize<NetSerialize::EntityNetSerializerCollectState> serializer(serializerImpl);
        callback(&serializer);
    }
    //! Called when need deserializer for the legacy aspect
    void Network::AcquireDeserializer(ReadBuffer& rb, NetSerialize::AcquireSerializeCallback callback)
    {
        NetSerialize::EntityNetSerializerDispatchState serializerImpl(rb);
        CSimpleSerialize<NetSerialize::EntityNetSerializerDispatchState> serializer(serializerImpl);
        callback(&serializer);
    }
    //-----------------------------------------------------------------------------
    void Network::MarkAsConnectedServer()
    {
        CryLog("Marked as hosting server.");

        gEnv->bServer = true;
        gEnv->bMultiplayer = true;

        if (m_gameContext)
        {
            uint32 flags = m_gameContext->GetContextFlags();
            flags &= ~(eGSF_LocalOnly);
            flags |= (eGSF_Server);
            m_gameContext->SetContextFlags(flags);
        }
    }
    //-----------------------------------------------------------------------------
    void Network::MarkAsConnectedClient()
    {
        CryLog("Marked as connected client.");

        gEnv->bServer = false;
        gEnv->bMultiplayer = true;

        if (m_gameContext)
        {
            uint32 flags = m_gameContext->GetContextFlags();
            flags &= ~(eGSF_Server | eGSF_LocalOnly);
            m_gameContext->SetContextFlags(flags);
        }
    }
    //-----------------------------------------------------------------------------
    void Network::MarkAsLocalOnly()
    {
        CryLog("Marked as local only.");

        gEnv->bServer = true;
        gEnv->bMultiplayer = false;

        if (m_gameContext)
        {
            uint32 flags = m_gameContext->GetContextFlags();
            flags |= (eGSF_Server | eGSF_LocalOnly);
            m_gameContext->SetContextFlags(flags);
        }
    }
} // namespace GridMate
