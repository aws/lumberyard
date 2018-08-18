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

#include "IActorSystem.h"

#include "../NetworkGridMate.h"
#include "../NetworkGridmateDebug.h"
#include "../NetworkGridmateMarshaling.h"
#include "../NetworkGridMateEntityEventBus.h"

#include "Components/IComponentPhysics.h"
#include "Components/IComponentScript.h"
#include "Components/IComponentSerialization.h"

#include "EntityReplica.h"
#include "EntityScriptReplicaChunk.h"

#include "GameContextReplica.h"

#include <GridMate/Replica/ReplicaFunctions.h>

namespace GridMate
{
    static const NetworkAspectType kAllEntityAspectBits = (1 << NetSerialize::kNumAspectSlots) - 1;
    //-----------------------------------------------------------------------------
    size_t EntityReplica::SerializedNetSerializeState::s_nextAspectIndex = 0;

    //-----------------------------------------------------------------------------
    EntityReplica::SerializedNetSerializeState::SerializedNetSerializeState()
        : DataSet(Debug::GetAspectNameByBitIndex(s_nextAspectIndex))
    {
        m_aspectIndex = s_nextAspectIndex++;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SerializedNetSerializeState::DispatchChangedEvent(const TimeContext& tc)
    {
        static_cast<EntityReplica*>(m_replicaChunk)->OnAspectChanged(m_aspectIndex);
    }

    //-----------------------------------------------------------------------------
    EntityReplica::EntityReplica()
        : m_gameDirtiedAspects(kAllEntityAspectBits)
        , m_localEntityId(kInvalidEntityId)
        , RPCHandleLegacyServerRMI("RPCHandleLegacyServerRMI")
        , RPCHandleLegacyClientRMI("RPCHandleLegacyClientRMI")
        , RPCHandleActorServerRMI("RPCHandleActorServerRMI")
        , RPCHandleActorClientRMI("RPCHandleActorClientRMI")
        , RPCUploadClientAspect("RPCUploadClientAspect")
        , RPCDelegateAuthorityToOwner("RPCDelegateAuthorityToOwner")
        , m_extraSpawnInfo("ExtraSpawnInfo")
        , m_clientDelegatedAspects("ClientDelegatedAspects", 0)
        , m_aspectProfiles("AspectProfiles")
        , m_modifiedDataSets(0)
        , m_scriptReplicaChunk(nullptr)
        , m_masterAspectScratchBuffer(EndianType::BigEndian)
        , m_isClientAspectAuthority(false)
        , m_flags(0)
    {
        SerializedNetSerializeState::s_nextAspectIndex = 0;
    }

    //-----------------------------------------------------------------------------
    EntityReplica::EntityReplica(const EntitySpawnParamsStorage& paramsStorage)
        : m_gameDirtiedAspects(kAllEntityAspectBits)
        , m_localEntityId(kInvalidEntityId)
        , RPCHandleLegacyServerRMI("RPCHandleLegacyServerRMI")
        , RPCHandleLegacyClientRMI("RPCHandleLegacyClientRMI")
        , RPCHandleActorServerRMI("RPCHandleActorServerRMI")
        , RPCHandleActorClientRMI("RPCHandleActorClientRMI")
        , RPCUploadClientAspect("RPCUploadClientAspect")
        , RPCDelegateAuthorityToOwner("RPCDelegateAuthorityToOwner")
        , m_spawnParams(paramsStorage)
        , m_extraSpawnInfo("ExtraSpawnInfo")
        , m_clientDelegatedAspects("ClientDelegatedAspects", 0)
        , m_aspectProfiles("AspectProfiles")
        , m_modifiedDataSets(0)
        , m_scriptReplicaChunk(nullptr)
        , m_masterAspectScratchBuffer(EndianType::BigEndian)
        , m_isClientAspectAuthority(false)
        , m_flags(0)
    {
        SerializedNetSerializeState::s_nextAspectIndex = 0;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnReplicaActivate(const GridMate::ReplicaContext& rc)
    {
        m_scriptReplicaChunk = GetReplica()->FindReplicaChunk<EntityScriptReplicaChunk>().get();

        Network& net = Network::Get();

        GM_DEBUG_TRACE("EntityReplica::OnActivate - IsMaster:%s EntityId:%u EntityName:%s EntityClass:%s, Address:0x%p",
            IsMaster() ? "yes" : "no",
            m_spawnParams.m_id,
            m_spawnParams.m_entityName.c_str(),
            m_spawnParams.m_className.c_str(),
            this);

        #if GRIDMATE_DEBUG_ENABLED
        for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
        {
            NetSerialize::AspectSerializeState::Marshaler& marshaler =
                m_netSerializeState[ aspectIndex ].GetMarshaler();

            marshaler.m_debugName = GridMate::Debug::GetAspectNameByBitIndex(aspectIndex);
            marshaler.m_debugIndex = aspectIndex;
        }
        #endif // GRIDMATE_DEBUG_ENABLED

        // Objects initially assume all globally-delegatable aspects are delegatable
        // by the object.
        m_clientDelegatedAspects.Set(eEA_All);

        if (IsMaster())
        {
            NetSerialize::EntityAspectProfiles aspectProfiles;
            for (size_t i = 0; i < NetSerialize::kNumAspectSlots; ++i)
            {
                aspectProfiles.SetAspectProfile(i, NetSerialize::kUnsetAspectProfile);
            }

            // The master side can immediately link the local entity to the replica.
            // Clients need to wait for unmarshaling.
            IEntity* entity = EstablishLocalEntity();
            if (entity)
            {
                // Ensure we're serializing fully up to date position/orientation data.
                // Game code often manipulates these after CEntity::Init() returns.
                m_spawnParams.m_position = entity->GetPos();
                m_spawnParams.m_orientation = entity->GetRotation();
                m_spawnParams.m_scale = entity->GetScale();

                IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entity->GetId());
                if (gameObject)
                {
                    for (size_t i = 0; i < NetSerialize::kNumAspectSlots; ++i)
                    {
                        uint8 profile = gameObject->GetAspectProfile(static_cast<EEntityAspects>(BIT(i)));
                        aspectProfiles.SetAspectProfile(i, profile);
                    }
                }
            }

            // Initialize aspect profiles
            m_aspectProfiles.Set(aspectProfiles);
        }
        else
        {
            // Flag replica such that we can establish (create or link) the local entity
            // associated this replica as soon as it's safe to do so.
            net.GetNewProxyEntityMap()[m_spawnParams.m_id] = this;
            m_flags |= kFlag_NewlyReceived;

            SetupAspectCallbacks();
        }
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UpdateChunk(const GridMate::ReplicaContext& rc)
    {
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

        UpdateAspects();
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UpdateFromChunk(const GridMate::ReplicaContext& rc)
    {
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

        GM_ASSERT_TRACE(m_spawnParams.m_id != kInvalidEntityId, "Invalid serialized entity Id for replica.");

        if (kInvalidEntityId != m_localEntityId)
        {
            UpdateAspects();
        }
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnReplicaDeactivate(const GridMate::ReplicaContext& rc)
    {
        GM_DEBUG_TRACE("EntityReplica::OnDeactivate - IsMaster:%s EntityId:%u EntityName:%s EntityClass:%s",
            IsMaster() ? "yes" : "no",
            m_spawnParams.m_id,
            m_spawnParams.m_entityName.c_str(),
            m_spawnParams.m_className.c_str());

        if (!IsMaster())
        {
            if (m_localEntityId != kInvalidEntityId)
            {
                // Destroy the client-side entity.
                if (IEntitySystem* entitySystem = gEnv->pEntitySystem)
                {
                    GM_DEBUG_TRACE("Destroying local entity %s [%u] for replica",
                        m_spawnParams.m_entityName.c_str(), m_localEntityId);

                    entitySystem->RemoveEntity(m_localEntityId);
                }
            }
        }

        EBUS_EVENT_ID(m_localEntityId, NetworkGridMateEntityEventBus, OnEntityUnboundFromNetwork, GetReplica());

        // Remove knowledge of the now-dead replica.
        Network::Get().GetNewProxyEntityMap().erase(m_spawnParams.m_id);

        m_localEntityId = kInvalidEntityId;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::IsReplicaMigratable()
    {
        return false;
    }

    //-----------------------------------------------------------------------------
    EntityId EntityReplica::HandleNewlyReceivedNow()
    {
        if (m_localEntityId == kInvalidEntityId)
        {
            HandleNewlyReceived();

            if (m_localEntityId != kInvalidEntityId)
            {
                UpdateAspects();
            }
        }

        return m_localEntityId;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::HandleNewlyReceived()
    {
        if (m_spawnParams.m_id != kInvalidEntityId &&
            m_localEntityId == kInvalidEntityId)
        {
            Network& net = Network::Get();

            if (!net.AllowEntityCreation())
            {
                return;
            }

            bool isGameRules =
                !!(m_spawnParams.m_paramsFlags & EntitySpawnParamsStorage::kParamsFlag_IsGameRules);

            if (isGameRules)
            {
                EstablishLocalEntity();
                GM_DEBUG_TRACE("Established game rules? %s", m_localEntityId != kInvalidEntityId ? "yes" : "no");
            }
            else
            {
                IGameRules* gameRules = GetGameRules();

                if (gameRules)
                {
                    EstablishLocalEntity();
                    GM_DEBUG_TRACE_LEVEL(2, "Established local entity? %s", m_localEntityId != kInvalidEntityId ? "yes" : "no");
                }
                else
                {
                    GM_DEBUG_TRACE_LEVEL(2, "Waiting for game rules...");
                    return;
                }
            }

            // Pass any server-side aspect profile specifications to the game.
            if (IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(m_localEntityId))
            {
                const NetSerialize::EntityAspectProfiles& aspectProfiles = m_aspectProfiles.Get();

                for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
                {
                    const NetSerialize::AspectProfile aspectProfile = aspectProfiles.GetAspectProfile(aspectIndex);

                    if (aspectProfile != NetSerialize::kUnsetAspectProfile)
                    {
                        gameObject->SetAspectProfile(EEntityAspects(BIT(aspectIndex)), aspectProfile, true);
                    }
                }
            }

            // Flush pending RMIs.
            if (kInvalidEntityId != m_localEntityId)
            {
                GM_DEBUG_TRACE("Flushing pending RMIs (%u / %u)",
                    m_pendingLegacyRMIs.size(), m_pendingActorRMIs.size());

                for (const auto& rmi : m_pendingLegacyRMIs)
                {
                    HandleLegacyClientRMI(rmi.first, rmi.second);
                }

                for (const auto& rmi : m_pendingActorRMIs)
                {
                    HandleActorClientRMI(rmi.first, rmi.second);
                }

                m_pendingLegacyRMIs.clear();
                m_pendingActorRMIs.clear();
            }
        }

        m_flags &= ~kFlag_NewlyReceived;
    }

    //-----------------------------------------------------------------------------
    IEntity* EntityReplica::EstablishLocalEntity()
    {
        IEntity* entity = nullptr;

        GM_DEBUG_TRACE("Handling new server replica - IsMaster:%s EntityId:%u EntityName:%s EntityClass:%s, Address:0x%p",
            IsMaster() ? "yes" : "no",
            m_spawnParams.m_id,
            m_spawnParams.m_entityName.c_str(),
            m_spawnParams.m_className.c_str(),
            this);

        IGameRulesSystem* gameRulesSystem = GetGameRulesSystem();

        if (!IsMaster())
        {
            bool isGameRules =
                !!(m_spawnParams.m_paramsFlags & EntitySpawnParamsStorage::kParamsFlag_IsGameRules);
            bool isLevelEntity =
                !!(m_spawnParams.m_paramsFlags & EntitySpawnParamsStorage::kParamsFlag_IsLevelEntity);

            if (isGameRules)
            {
                if (gameRulesSystem->GetCurrentGameRulesEntity()) // already have game rules
                {
                    const char* gameRulesType = gameRulesSystem->GetCurrentGameRulesEntity()->GetClass()->GetName();
                    GM_ASSERT_TRACE(strcmp(gameRulesType, m_spawnParams.m_className.c_str()) == 0,
                        "Invalid game rules type '%s' instantiated locally. Received '%s'.", gameRulesType, m_spawnParams.m_className.c_str());

                    entity = gameRulesSystem->GetCurrentGameRulesEntity();
                }
                else
                {
                    entity = SpawnLocalEntity();
                }

                GM_ASSERT_TRACE(entity, "Failed to spawn game rules.");
                GM_ASSERT_TRACE(gameRulesSystem->GetCurrentGameRules(), "Failed to register game rules.");
            }
            else
            {
                if (isLevelEntity)
                {
                    // This is a non-gamerules level entity. We should've created it during level load
                    // with the same Id as the server.
                    entity = gEnv->pEntitySystem->GetEntity(m_spawnParams.m_id);
                    AZ_Warning("CryNetworkShim", entity, "Unable to locate level entity with id %u locally.", m_spawnParams.m_id);

                    if (entity)
                    {
                        AZ_Assert(0 == azstricmp(entity->GetName(), m_spawnParams.m_entityName.c_str()),
                            "Entity mismatch detected! Local entity with id %u is named \"%s\", "
                            "but the server's version is named \"%s\".",
                            entity->GetId(),
                            entity->GetName(),
                            m_spawnParams.m_entityName.c_str());
                    }
                }
                else
                {
                    // This is a non-level entity, so we need to spawn our local version of it.
                    entity = SpawnLocalEntity();
                }
            }

            if (entity)
            {
                EBUS_EVENT_ID(entity->GetId(), NetworkGridMateEntityEventBus, OnEntityBoundFromNetwork, GetReplica());
            }
        }
        else
        {
            entity = gEnv->pEntitySystem->GetEntity(m_spawnParams.m_id);

            GM_ASSERT_TRACE(entity, "Unable to find local entity for id %u.",
                m_spawnParams.m_id);
        }

        if (entity)
        {
            // Register in network-global map for lookup and server<->client ID mapping.
            auto insertion = Network::Get().GetEntityReplicaMap().insert_key(m_spawnParams.m_id);
            AZ_Assert(insertion.second, "Entity replica map already has an entry for entity id %u!", m_spawnParams.m_id);
            insertion.first->second = this;

            // If this entity represents the actor for this client, let the server know that we got it.
            const ChannelId channelId = m_spawnParams.m_channelId;
            const ChannelId localChannelId = Network::Get().GetLocalChannelId();
            if (channelId == localChannelId)
            {
                IActorSystem* actorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
                if (actorSystem)
                {
                    IActor* actor = actorSystem->GetActor(entity->GetId());
                    if (actor)
                    {
                        GameContextReplica* contextReplica = Network::Get().GetGameContextReplica();
                        if (contextReplica)
                        {
                            contextReplica->RPCOnActorReceived();
                        }
                    }
                }
            }

            AssignLocalEntity(entity);
        }

        return entity;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::AssignLocalEntity(IEntity* entity)
    {
        GM_ASSERT_TRACE(entity && !entity->IsGarbage(), "Invalid entity passed to AssignEntity()")
        GM_ASSERT_TRACE(m_localEntityId == kInvalidEntityId || m_localEntityId == entity->GetId(), "Entity has already been assigned");

        if (entity && entity->GetId() != m_localEntityId)
        {
            m_localEntityId = entity->GetId();

            if (m_scriptReplicaChunk)
            {
                m_scriptReplicaChunk->SetLocalEntityId(m_localEntityId);
            }

            IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entity->GetId());

            if (IsMaster())
            {
                // SerializeSpawnInfo under CryNetwork was only relevant for client actors, so we'll
                // stick to that rule here.
                // I can certainly see the value for general-purpose 'spawn data', which GridMate
                // has an established pattern for. We can improve SerializeSpawnInfo() functionality
                // over CryNetwork's if some case calls for it.
                if (gameObject && gameObject->GetChannelId() != kInvalidChannelId)
                {
                    using namespace NetSerialize;

                    enum
                    {
                        kSpawnInfoMaxSize = 16 * 1024
                    };
                    char tempStorage[ kSpawnInfoMaxSize ];
                    WriteBufferType writeBuffer(EndianType::BigEndian, tempStorage, sizeof(tempStorage));
                    Network::Get().GetLegacySerializeProvider()->AcquireSerializer(writeBuffer, [&](ISerialize* serializer)
                    {
                        gameObject->SerializeSpawnInfo(serializer, entity);
                    });

                    m_extraSpawnInfo.Set(new EntityExtraSpawnInfo(writeBuffer.Get(), writeBuffer.Size()));
                }
                else
                {
                    m_extraSpawnInfo.Set(nullptr);
                }
            }

            if (IsAspectDelegatedToThisClient())
            {
                if (gameObject)
                {
                    gameObject->SetAuthority(true);
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UnbindLocalEntity()
    {
        m_localEntityId = kInvalidEntityId;
    }

    //-----------------------------------------------------------------------------
    IEntity* EntityReplica::SpawnLocalEntity() const
    {
        GM_ASSERT_TRACE(!IsMaster(), "SpawnLocalEntity() should not be invoked in the replica master.");

        IEntity* entity = nullptr;

        if (IEntitySystem* entitySystem = gEnv->pEntitySystem)
        {
            SEntitySpawnParams spawnParams = GetEngineSpawnParams();

            const ChannelId channelId = m_spawnParams.m_channelId;
            const ChannelId localChannelId = Network::Get().GetLocalChannelId();

            spawnParams.id = 0; // Allow entity ID to be determined locally.
                                // We handle mapping between local and server Id.

            if (channelId != kInvalidChannelId)
            {
                // Entities with channels are actors owned by clients, and need to be spawned
                // through the actor system.

                IActorSystem* actorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();

                GM_ASSERT_TRACE(actorSystem, "Actor system doesn't exist, so we can't create any actors.");

                IActor* actor = nullptr;

                GM_DEBUG_TRACE("Creating replicated actor for entity \"%s\", channel %u, is mine? %s",
                    spawnParams.sName, channelId, channelId == localChannelId ? "yes" : "no");

                EntityExtraSpawnInfo::Ptr extraSpawnInfo = m_extraSpawnInfo.Get();

                if (actorSystem)
                {
                    if (extraSpawnInfo && extraSpawnInfo->m_buffer.GetSize() > 0)
                    {
                        GridMate::ReadBuffer readBuf = extraSpawnInfo->m_buffer.GetReadBuffer();
                        Network::Get().GetLegacySerializeProvider()->AcquireDeserializer(readBuf, [&](ISerialize* serializer)
                        {
                            TSerialize tSer = serializer;

                            actor = actorSystem->CreateActor(
                                channelId,
                                spawnParams.sName,
                                spawnParams.pClass ? spawnParams.pClass->GetName() : "",
                                spawnParams.vPosition,
                                spawnParams.qRotation,
                                spawnParams.vScale,
                                0,
                                &tSer);
                        });
                    }
                    else
                    {
                        actor = actorSystem->CreateActor(
                            channelId,
                            spawnParams.sName,
                            spawnParams.pClass ? spawnParams.pClass->GetName() : "",
                            spawnParams.vPosition,
                            spawnParams.qRotation,
                            spawnParams.vScale,
                            0,
                            nullptr);
                    }
                }

                GM_ASSERT_TRACE(actor, "Failed to create actor for entity \"%s\", channel %u.",
                    spawnParams.sName, channelId);

                if (actor)
                {
                    if (channelId == localChannelId)
                    {
                        // Server delegates control of channel-owned actors to that channel.
                        const_cast<EntityReplica*>(this)->RPCDelegateAuthorityToOwner(channelId);
                    }
                    entity = actor->GetEntity();
                }
            }
            else
            {
                // Otherwise, spawn normally through the entity system.

                GM_DEBUG_TRACE("Creating replicated entity \"%s\".",
                    spawnParams.sName, channelId);

                entity = entitySystem->SpawnEntity(spawnParams);

                GM_ASSERT_TRACE(entity, "Failed to create local entity \"%s\".",
                    spawnParams.sName);
            }
        }

        return entity;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UpdateAspects()
    {
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

        using namespace NetSerialize;

        IEntity* entity = gEnv->pEntitySystem->GetEntity(m_localEntityId);
        if (!entity)
        {
            AZ_Assert(false, "Failed to find entity for replica 0x%x with server id %u (%s) and local id %u.", GetReplicaId(), m_spawnParams.m_id, m_spawnParams.m_entityName.c_str(), m_localEntityId);
            return;
        }

        IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entity->GetId());

        if (IsMaster())
        {
            // Collect aspect profile changes
            NetSerialize::EntityAspectProfiles aspectProfiles = m_aspectProfiles.Get();
            bool profilesChanged = false;

            // Master side - identify changes by invoking NetSerialize on dirtied aspects,
            // and check for changes. Send if a hash of the data has changed.
            for (size_t i = 0; i < kNumAspectSlots; ++i)
            {
                // Propagate aspect data changes requested by the game.
                const NetworkAspectType engineAspectBit = BIT(i);

                if (gameObject)
                {
                    uint8 profile = gameObject->GetAspectProfile(static_cast<EEntityAspects>(engineAspectBit));
                    if (profile != aspectProfiles.GetAspectProfile(i))
                    {
                        aspectProfiles.SetAspectProfile(i, profile);
                        profilesChanged = true;
                    }
                }

                if (!!(m_gameDirtiedAspects & engineAspectBit))
                {
                    // We want to serialize ServerProperties using a special mechansim
                    // while leaving the other aspects of the script serialization alone.
                    if (engineAspectBit == eEA_Script && m_scriptReplicaChunk)
                    {
                        gEnv->pGame->GetIGameFramework()->NetMarshalScript(m_scriptReplicaChunk, entity);
                    }

                    SerializedNetSerializeState& aspectState = m_netSerializeState[i];

                    m_masterAspectScratchBuffer.Clear();

                    // Serialize aspect into scratch buffer.
                    {
                        FRAME_PROFILER(Debug::GetAspectNameByBitIndex(i), GetISystem(), PROFILE_NETWORK);
                        Network::Get().GetLegacySerializeProvider()->AcquireSerializer(m_masterAspectScratchBuffer, [&](ISerialize* serializer)
                        {
                            SerializeAspect(serializer, entity, gameObject, static_cast<EEntityAspects>(engineAspectBit));
                        });

                        CommitAspectData(i, m_masterAspectScratchBuffer.Get(),
                            m_masterAspectScratchBuffer.Size(),
                            HashBuffer(m_masterAspectScratchBuffer.Get(), m_masterAspectScratchBuffer.Size()));
                    }

                    m_gameDirtiedAspects &= ~engineAspectBit;
                }
            }

            if (profilesChanged)
            {
                m_aspectProfiles.Set(aspectProfiles);
            }
        }
        else
        {
            // Proxy side - Handle dispatching changed aspect data via NetSerialize().

            for (size_t i = 0; i < kNumAspectSlots; ++i)
            {
                SerializedNetSerializeState& aspectState = m_netSerializeState[i];

                if (aspectState.GetMarshaler().IsWaitingForDispatch())
                {
                    aspectState.GetMarshaler().MarkDispatchComplete();

                    // Don't dispatch locally if this client uploaded the aspect change.
                    if (false == IsAspectDelegatedToThisClient(i))
                    {
                        const NetworkAspectType engineAspectBit = BIT(i);

                        if (0 == aspectState.GetMarshaler().GetStorageSize())
                        {
                            continue;
                        }

                        GM_DEBUG_TRACE_LEVEL(2, "Aspect %u has %u bytes waiting for dispatch.",
                            i, aspectState.Get().GetWrittenSize());

                        ReadBufferType rb = aspectState.GetMarshaler().GetReadBuffer();

                        GM_ASSERT_TRACE(rb.Get(), "NetSerialize target buffer is not ready. "
                            "This replica cannot be unmarshaled.");

                        {
                            FRAME_PROFILER(Debug::GetAspectNameByBitIndex(i), GetISystem(), PROFILE_NETWORK);
                            Network::Get().GetLegacySerializeProvider()->AcquireDeserializer(rb, [&](ISerialize* serializer)
                            {
                                SerializeAspect(serializer, entity, gameObject, static_cast<EEntityAspects>(engineAspectBit));
                            });
                        }

                        EBUS_EVENT(NetworkSystemEventBus, AspectReceived, m_localEntityId, engineAspectBit, rb.Size().GetSizeInBytesRoundUp());
                    }
                }
            }
        }
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SerializeAspect(TSerialize ser, IEntity* entity, IGameObject* gameObject, EEntityAspects aspect)
    {
        if (gameObject)
        {
            FRAME_PROFILER("SerializeAspect.NetSerialize", GetISystem(), PROFILE_NETWORK);
            gameObject->NetSerialize(ser, aspect, 0, 0);
        }
        else if (aspect == eEA_Physics)
        {
            FRAME_PROFILER("SerializeAspect.eEA_Physics", GetISystem(), PROFILE_NETWORK);
            IComponentSerializationPtr serializationComponent = entity->GetComponent<IComponentSerialization>();
            if (serializationComponent)
            {
                serializationComponent->SerializeOnly(ser, { IComponentPhysics::Type() });
            }
        }
        else if (aspect == eEA_Script)
        {
            FRAME_PROFILER("SerializationAspect.eEA_Script",GetISystem(),PROFILE_NETWORK);
            gEnv->pGame->GetIGameFramework()->SerializeScript(ser, entity);
        }
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SetupAspectCallbacks()
    {
        using namespace NetSerialize;

        for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
        {
            SerializedNetSerializeState& aspectState = m_netSerializeState[ aspectIndex ];
            AspectSerializeState::Marshaler& marshaler = aspectState.GetMarshaler();

            // Trigger initial dispatch of all aspects.
            marshaler.MarkWaitingForDispatch();
        }

        // Setup client-side callback for aspect profile changes.
        using namespace AZStd::placeholders;
        auto aspectProfileCallback =
            AZStd::bind(&EntityReplica::OnAspectProfileChanged, this, _1, _2, _3);
        m_aspectProfiles.GetMarshaler().SetChangeDelegate(aspectProfileCallback);
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::CommitAspectData(size_t aspectIndex, const char* newData, size_t newDataSize, uint32 hash)
    {
        GM_ASSERT_TRACE(newData, "Invalid data buffer.");

        SerializedNetSerializeState& aspectState = m_netSerializeState[ aspectIndex ];

        // Update outgoing storage for marshaling.
        NetSerialize::AspectSerializeState::Marshaler& marshaler = aspectState.GetMarshaler();
        if (marshaler.GetStorageSize() < newDataSize)
        {
            marshaler.AllocateAspectSerializationBuffer(newDataSize);
        }

        if (newDataSize > 0)
        {
            FRAME_PROFILER("AspectBufferCopy", GetISystem(), PROFILE_NETWORK);
            WriteBufferType writeBuffer = marshaler.GetWriteBuffer();
            writeBuffer.Clear();
            writeBuffer.WriteRaw(newData, newDataSize);
        }

        // Store updated contents & hash. Any change will result in a downstream update.
        NetSerialize::AspectSerializeState updatedState = aspectState.Get();
        bool changed = false;

        {
            FRAME_PROFILER("AspectBufferHash", GetISystem(), PROFILE_NETWORK);
            changed = updatedState.UpdateHash(hash, newDataSize);
        }
        {
            FRAME_PROFILER("AspectUpdate", GetISystem(), PROFILE_NETWORK);
            aspectState.Set(updatedState);
        }

        if (changed)
        {
            FRAME_PROFILER("AspectSentEvent", GetISystem(), PROFILE_NETWORK);
            EBUS_EVENT(NetworkSystemEventBus, AspectSent, m_localEntityId, BIT(aspectIndex), newDataSize);
        }

        return changed;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::IsAspectDelegatedToThisClient(size_t aspectIndex) const
    {
        const NetworkAspectType engineAspectBit = BIT(aspectIndex);

        return IsAspectDelegatedToThisClient() &&                                  // Authority over this entity has been delegated to this client.
               !!(engineAspectBit & NetSerialize::GetDelegatableAspectMask()) &&   // This aspect supports client-delegation (globally).
               !!(engineAspectBit & m_clientDelegatedAspects.Get());               // This aspect supports client-delegation (on this object).
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::IsAspectDelegatedToThisClient() const
    {
        return m_isClientAspectAuthority;   // Authority over this entity has been delegated to this client.
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnAspectChanged(size_t aspectIndex)
    {
        GM_ASSERT_TRACE(!IsMaster(), "We shouldn't have unmarshaled on master.");

        SerializedNetSerializeState& aspectState = m_netSerializeState[ aspectIndex ];
        aspectState.GetMarshaler().MarkWaitingForDispatch();
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnAspectProfileChanged(size_t aspectIndex,
        NetSerialize::AspectProfile oldProfile,
        NetSerialize::AspectProfile newProfile)
    {
        GM_ASSERT_TRACE(!IsMaster(), "OnAspectProfileChanged should only be invoked on proxies/clients.");

        if (!IsMaster())
        {
            IGameObject* gameObject = GetGameFramework()->GetGameObject(m_localEntityId);

            if (gameObject)
            {
                gameObject->SetAspectProfile(EEntityAspects(BIT(aspectIndex)), newProfile, true);
            }
        }
    }
    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleLegacyServerRMI(RMI::LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        AZ_Assert(IsMaster(), "Legacy Server RMIs should only ever be processed on the server!");
        if (IsMaster())
        {
            AZ_Assert(kInvalidEntityId != m_localEntityId, "local entity ids should be immediately available on the server!");
            if (kInvalidEntityId != m_localEntityId)
            {
                RMI::HandleLegacy(m_localEntityId, invocation, rc);
            }
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleLegacyClientRMI(RMI::LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        if (kInvalidEntityId != m_localEntityId)
        {
            return RMI::HandleLegacy(m_localEntityId, invocation, rc);
        }

        m_pendingLegacyRMIs.push_back(std::make_pair(invocation, rc));
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleActorServerRMI(RMI::ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        AZ_Assert(IsMaster(), "Legacy Server RMIs should only ever be processed on the server!");
        if (IsMaster())
        {
            AZ_Assert(kInvalidEntityId != m_localEntityId, "local entity ids should be immediately available on the server!");
            if (kInvalidEntityId != m_localEntityId)
            {
                RMI::HandleActor(m_localEntityId, invocation, rc);
            }
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleActorClientRMI(RMI::ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        if (kInvalidEntityId != m_localEntityId)
        {
            return RMI::HandleActor(m_localEntityId, invocation, rc);
        }

        m_pendingActorRMIs.push_back(std::make_pair(invocation, rc));
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::UploadClientAspect(uint32 aspectIndex, AspectUploadBuffer::Ptr buffer, const GridMate::RpcContext& rc)
    {
        GM_ASSERT_TRACE(buffer.get(), "UploadClientAspect: Empty buffer received for client-delegated aspect.");

        if (buffer.get())
        {
            const char* data = buffer->GetData();
            size_t dataSize = buffer->GetSize();
            ReadBufferType rb(EndianType::BigEndian, data, dataSize);

            IEntity* entity = gEnv->pEntitySystem->GetEntity(m_localEntityId);
            AZ_Assert(entity, "Failed to find entity for replica 0x%x with server id %u (%s) and local id %u.", GetReplicaId(), m_spawnParams.m_id, m_spawnParams.m_entityName.c_str(), m_localEntityId);
            if (entity)
            {
                IGameObject* gameObject = GetGameFramework()->GetGameObject(m_localEntityId);

                // Changes will be distributed to other clients via normal downstream aspect behavior.
                CommitAspectData(aspectIndex, data, dataSize, NetSerialize::HashBuffer(data, dataSize));

                FRAME_PROFILER(Debug::GetAspectNameByBitIndex(aspectIndex), GetISystem(), PROFILE_NETWORK);

                using namespace NetSerialize;
                const NetworkAspectType engineAspectBit = BIT(aspectIndex);
                Network::Get().GetLegacySerializeProvider()->AcquireDeserializer(rb, [&](ISerialize* serializer)
                {
                    SerializeAspect(serializer, entity, gameObject, static_cast<EEntityAspects>(engineAspectBit));
                });

                EBUS_EVENT(NetworkSystemEventBus, AspectReceived, m_localEntityId, engineAspectBit, dataSize);
            }
        }

        // No need to pass on - this only occurs on the server, and changes will be marshaled
        // down through aspect states.
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::DelegateAuthorityToOwner(ChannelId ownerChannelId, const GridMate::RpcContext& rc)
    {
        if (!IsMaster() && Network::Get().GetLocalChannelId() == ownerChannelId)
        {
            m_isClientAspectAuthority = true;

            IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(m_localEntityId);
            if (gameObject)
            {
                gameObject->SetAuthority(true);
            }

            // Wipe hash values for client-delegated aspects.
            for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
            {
                m_clientDelegatedAspectHashes[ aspectIndex ] = 0;
            }

            m_gameDirtiedAspects = 0;
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    const EntitySpawnParamsStorage& EntityReplica::GetSerializedSpawnParams() const
    {
        return m_spawnParams;
    }

    //-----------------------------------------------------------------------------
    SEntitySpawnParams EntityReplica::GetEngineSpawnParams() const
    {
        return GetSerializedSpawnParams().ToEntitySpawnParams();
    }

    //-----------------------------------------------------------------------------
    EntityId EntityReplica::GetLocalEntityId() const
    {
        return m_localEntityId;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::MarkAspectsDirty(NetworkAspectType aspects)
    {
        m_gameDirtiedAspects |= aspects;
    }

    //-----------------------------------------------------------------------------
    NetworkAspectType EntityReplica::GetDirtyAspects() const
    {
        return m_gameDirtiedAspects;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UploadClientDelegatedAspects()
    {
        // Client-delegated aspect shim. Any delegated aspects that were dirtied on the client
        // are packaged up and RPC'd across.
        if (m_gameDirtiedAspects && m_isClientAspectAuthority)
        {
            IEntity* entity = gEnv->pEntitySystem->GetEntity(m_localEntityId);
            AZ_Assert(entity, "Failed to find entity for replica 0x%x with server id %u (%s) and local id %u.", GetReplicaId(), m_spawnParams.m_id, m_spawnParams.m_entityName.c_str(), m_localEntityId);
            if (entity)
            {
                IGameObject* gameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(entity->GetId());

                for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
                {
                    const NetworkAspectType engineAspectBit = BIT(aspectIndex);

                    if (!!(m_gameDirtiedAspects & BIT(aspectIndex)) && IsAspectDelegatedToThisClient(aspectIndex))
                    {
                        // We want to serializer ServerProperties using a sepearte mechanism
                        // which we will handle here.
                        //
                        // Everything else will be serialized through the old system
                        if (engineAspectBit == eEA_Script && m_scriptReplicaChunk)
                        {
                            gEnv->pGame->GetIGameFramework()->NetMarshalScript(m_scriptReplicaChunk, entity);
                        }

                        m_masterAspectScratchBuffer.Clear();
                        Network::Get().GetLegacySerializeProvider()->AcquireSerializer(m_masterAspectScratchBuffer, [&](ISerialize* serializer)
                        {
                            SerializeAspect(serializer, entity, gameObject, static_cast<EEntityAspects>(engineAspectBit));
                        });

                        if (m_masterAspectScratchBuffer.Size() > 0)
                        {
                            const uint32 newHash = NetSerialize::HashBuffer(m_masterAspectScratchBuffer.Get(), m_masterAspectScratchBuffer.Size());

                            if (newHash != m_clientDelegatedAspectHashes[aspectIndex])
                            {
                                m_clientDelegatedAspectHashes[aspectIndex] = newHash;

                                // Upload aspect data tot he server via RPC.
                                AspectUploadBuffer::Ptr buffer = new AspectUploadBuffer(m_masterAspectScratchBuffer.Get(), m_masterAspectScratchBuffer.Size());
                                RPCUploadClientAspect(uint32(aspectIndex), buffer);

                                EBUS_EVENT(NetworkSystemEventBus, AspectSent, m_localEntityId, BIT(aspectIndex), m_masterAspectScratchBuffer.Size());
                            }
                        }
                    }
                }
            }
        }
        m_gameDirtiedAspects = 0;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SetClientDelegatedAspectMask(NetworkAspectType aspects)
    {
        m_clientDelegatedAspects.Set(aspects);
    }

    //-----------------------------------------------------------------------------
    NetworkAspectType EntityReplica::GetClientDelegatedAspectMask() const
    {
        return m_clientDelegatedAspects.Get();
    }

    //-----------------------------------------------------------------------------
    NetSerialize::AspectProfile EntityReplica::GetAspectProfile(size_t aspectIndex) const
    {
        GM_ASSERT_TRACE(aspectIndex < NetSerialize::kNumAspectSlots, "Invalid aspect index: %u", aspectIndex);

        return m_aspectProfiles.Get().GetAspectProfile(aspectIndex);
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SetAspectProfile(size_t aspectIndex, NetSerialize::AspectProfile profile)
    {
        GM_ASSERT_TRACE(aspectIndex < NetSerialize::kNumAspectSlots, "Invalid aspect index: %u", aspectIndex);

        if (GetAspectProfile(aspectIndex) != profile)
        {
            NetSerialize::EntityAspectProfiles aspectProfiles = m_aspectProfiles.Get();
            aspectProfiles.SetAspectProfile(aspectIndex, profile);
            m_aspectProfiles.Set(aspectProfiles);
        }
    }
    //-----------------------------------------------------------------------------
    AZ::u32 EntityReplica::CalculateDirtyDataSetMask(MarshalContext& mc)
    {
        if ((mc.m_marshalFlags & ReplicaMarshalFlags::ForceDirty))
        {
            return m_modifiedDataSets;
        }

        return ReplicaChunkBase::CalculateDirtyDataSetMask(mc);
    }
    //-----------------------------------------------------------------------------
    void EntityReplica::OnDataSetChanged(const DataSetBase& dataSet)
    {
        // Keep track of which DataSets have been chagned, so when we initialize
        // we only initialize the DataSets with data.
        int index = GetDescriptor()->GetDataSetIndex(this,&dataSet);
        m_modifiedDataSets |= (1 << index);
    }
} // namespace GridMate
