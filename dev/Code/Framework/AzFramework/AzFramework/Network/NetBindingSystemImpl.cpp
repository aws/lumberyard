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

#include <AzFramework/Network/NetBindingSystemImpl.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAsset.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Serialize/CompressionMarshal.h>

namespace AzFramework
{
    namespace
    {
        NetBindingHandlerInterface* GetNetBindingHandler(AZ::Entity* entity)
        {
            NetBindingHandlerInterface* handler = nullptr;
            for (AZ::Component* component : entity->GetComponents())
            {
                handler = azrtti_cast<NetBindingHandlerInterface*>(component);
                if (handler)
                {
                    break;
                }
            }
            return handler;
        }
    }

    void NetBindingSliceInstantiationHandler::InstantiateEntities()
    {
        if (m_sliceAssetId.IsValid())
        {
            auto RemapFunc = [this](AZ::EntityId originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>&) -> AZ::EntityId
            {
                auto iter = m_bindingQueue.find(originalId);
                if (iter != m_bindingQueue.end())
                {
                    return iter->second.m_desiredRuntimeEntityId;
                }
                return AZ::Entity::MakeId();
            };
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::DynamicSliceAsset>(m_sliceAssetId, false);
            EBUS_EVENT_RESULT(m_ticket, GameEntityContextRequestBus, InstantiateDynamicSlice, asset, AZ::Transform::Identity(), RemapFunc);
            AzFramework::SliceInstantiationResultBus::Handler::BusConnect(m_ticket);
        }
    }

    bool NetBindingSliceInstantiationHandler::IsInstantiated()
    {
        return !m_sliceAssetId.IsValid() || m_ticket;
    }

    bool NetBindingSliceInstantiationHandler::IsBindingComplete()
    {
        return !SliceInstantiationResultBus::Handler::BusIsConnected() && m_bindingQueue.empty();
    }

    void NetBindingSliceInstantiationHandler::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AZ::SliceComponent::EntityList& sliceEntities = sliceAddress.second->GetInstantiated()->m_entities;
        for (AZ::Entity *sliceEntity : sliceEntities)
        {
            auto it = sliceAddress.second->GetEntityIdToBaseMap().find(sliceEntity->GetId());
            AZ_Assert(it != sliceAddress.second->GetEntityIdToBaseMap().end(), "Failed to retrieve static entity id for a slice entity!");
            AZ::EntityId staticEntityId = it->second;

            // We instantiate slices once for each replicated entity, so if the newly spawned entity is not already in the binding map,
            // then we don't want to add it.
            auto itBindRecord = m_bindingQueue.find(staticEntityId);
            if (itBindRecord != m_bindingQueue.end())
            {
                AZ_Assert(GetNetBindingHandler(sliceEntity), "Slice entity matched the static id of replicated entity, but there is no valid NetBindingHandlerInterface on it!");
 
                itBindRecord->second.m_actualRuntimeEntityId = sliceEntity->GetId();
            }

            sliceEntity->SetRuntimeActiveByDefault(false);
        }
    }

    void NetBindingSliceInstantiationHandler::OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        SliceInstantiationResultBus::Handler::BusDisconnect();

        const AZ::SliceComponent::EntityList sliceEntities = sliceAddress.second->GetInstantiated()->m_entities;
        for (AZ::Entity *sliceEntity : sliceEntities)
        {
            auto it = sliceAddress.second->GetEntityIdToBaseMap().find(sliceEntity->GetId());
            AZ_Assert(it != sliceAddress.second->GetEntityIdToBaseMap().end(), "Failed to retrieve static entity id for a slice entity!");
            AZ::EntityId staticEntityId = it->second;
            auto itUnbound = m_bindingQueue.find(staticEntityId);
            if (itUnbound == m_bindingQueue.end())
            {
                EBUS_EVENT(GameEntityContextRequestBus, DestroyGameEntity, sliceEntity->GetId());
            }
        }
    }

    void NetBindingSliceInstantiationHandler::OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/)
    {
        SliceInstantiationResultBus::Handler::BusDisconnect();
    }
    
    class NetBindingSystemContextData
        : public GridMate::ReplicaChunk
    {
    public:
        AZ_CLASS_ALLOCATOR(NetBindingSystemContextData, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "NetBindingSystemContextData"; }

        NetBindingSystemContextData()
            : m_bindingContextSequence("BindingContextSequence", UnspecifiedNetBindingContextSequence)
        {
        }

        bool IsReplicaMigratable() override { return true; }
        bool IsBroadcast() override { return true; }

        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override
        {
            (void)rc;
            NetBindingSystemImpl* system = static_cast<NetBindingSystemImpl*>(NetBindingSystemBus::FindFirstHandler());
            AZ_Assert(system, "NetBindingSystemContextData requires a valid NetBindingSystemComponent to function!");
            system->OnContextDataActivated(this);
        }

        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override
        {
            (void)rc;
            NetBindingSystemImpl* system = static_cast<NetBindingSystemImpl*>(NetBindingSystemBus::FindFirstHandler());
            if (system)
            {
                system->OnContextDataDeactivated(this);
            }
        }

        GridMate::DataSet<AZ::u32, GridMate::VlqU32Marshaler> m_bindingContextSequence;
    };

    NetBindingSystemImpl::NetBindingSystemImpl()
        : m_bindingSession(nullptr)
        , m_currentBindingContextSequence(UnspecifiedNetBindingContextSequence)
        , m_isAuthoritativeRootSliceLoad(false)
        , m_overrideRootSliceLoadAuthoritative(false)
    {
    }

    NetBindingSystemImpl::~NetBindingSystemImpl()
    {
    }

    void NetBindingSystemImpl::Init()
    {
        NetBindingSystemBus::Handler::BusConnect();
        NetBindingSystemEventsBus::Handler::BusConnect();

        // Start listening for game context events
        EntityContextId gameContextId = EntityContextId::CreateNull();
        EBUS_EVENT_RESULT(gameContextId, GameEntityContextRequestBus, GetGameEntityContextId);
        if (!gameContextId.IsNull())
        {
            EntityContextEventBus::Handler::BusConnect(gameContextId);
        }
    }

    void NetBindingSystemImpl::Shutdown()
    {
        EntityContextEventBus::Handler::BusDisconnect();
        NetBindingSystemEventsBus::Handler::BusDisconnect();
        NetBindingSystemBus::Handler::BusDisconnect();

        m_contextData.reset();
    }

    bool NetBindingSystemImpl::ShouldBindToNetwork()
    {
        return m_contextData
               && m_contextData->GetReplica()
               && m_contextData->GetReplica()->IsActive();
    }

    NetBindingContextSequence NetBindingSystemImpl::GetCurrentContextSequence()
    {
        return m_currentBindingContextSequence;
    }

    bool NetBindingSystemImpl::ReadyToAddReplica() const
    {
        return m_bindingSession && m_bindingSession->GetReplicaMgr();
    }

    void NetBindingSystemImpl::AddReplicaMaster(AZ::Entity* entity, GridMate::ReplicaPtr replica)
    {
        bool addReplica = ShouldBindToNetwork();
        AZ_Assert(addReplica, "Entities shouldn't be binding to the network right now!");
        if (addReplica)
        {
            if (ReadyToAddReplica())
            {
                m_bindingSession->GetReplicaMgr()->AddMaster(replica);
            }
            else
            {
            m_addMasterRequests.push_back(AZStd::make_pair(entity->GetId(), replica));
        }
    }
    }


    AZ::EntityId NetBindingSystemImpl::GetStaticIdFromEntityId(AZ::EntityId entityId)
    {
        AZ::EntityId staticId = entityId; // if no static id mapping is found, then the static id is the same as the runtime id
        
        // If entity came from a slice, try to get the mapping from it
        AZ::SliceComponent::SliceInstanceAddress sliceInfo;
        EBUS_EVENT_ID_RESULT(sliceInfo, entityId, EntityIdContextQueryBus, GetOwningSlice);
        AZ::SliceComponent::SliceInstance* sliceInstance = sliceInfo.second;
        if (sliceInstance)
        {
            auto it = sliceInstance->GetEntityIdToBaseMap().find(entityId);
            if (it != sliceInstance->GetEntityIdToBaseMap().end())
            {
                staticId = it->second;
            }
        }

        return staticId;
    }

    AZ::EntityId NetBindingSystemImpl::GetEntityIdFromStaticId(AZ::EntityId staticEntityId)
    {
        AZ::EntityId runtimeId = AZ::EntityId();
        
        // if we can find an entity with the static id, then the static id is the same as the runtime id.
        AZ::Entity* entity = nullptr;
        EBUS_EVENT(AZ::ComponentApplicationBus, FindEntity, staticEntityId);
        if (entity)
        {
            runtimeId = staticEntityId;
        }
        
        return runtimeId;
    }

    void NetBindingSystemImpl::SpawnEntityFromSlice(GridMate::ReplicaId bindTo, const NetBindingSliceContext& bindToContext)
    {
        auto& sliceQueue = m_bindRequests[bindToContext.m_contextSequence];
        auto iterSliceRequest = sliceQueue.insert_key(bindToContext.m_runtimeEntityId);
        NetBindingSliceInstantiationHandler& sliceHandler = iterSliceRequest.first->second;
        sliceHandler.m_sliceAssetId = bindToContext.m_sliceAssetId;
        BindRequest& request = sliceHandler.m_bindingQueue[bindToContext.m_staticEntityId];
        request.m_bindTo = bindTo;
        request.m_desiredRuntimeEntityId = bindToContext.m_runtimeEntityId;
        request.m_requestTime = AZStd::chrono::system_clock::now();
    }

    void NetBindingSystemImpl::SpawnEntityFromStream(AZ::IO::GenericStream& spawnData, AZ::EntityId useEntityId, GridMate::ReplicaId bindTo, NetBindingContextSequence addToContext)
    {
        auto& requestQueue = m_spawnRequests[addToContext];
        requestQueue.push_back();
        SpawnRequest& request = requestQueue.back();
        request.m_bindTo = bindTo;
        request.m_useEntityId = useEntityId;
        request.m_spawnDataBuffer.resize(spawnData.GetLength());
        spawnData.Read(request.m_spawnDataBuffer.size(), request.m_spawnDataBuffer.data());
    }

    void NetBindingSystemImpl::OnNetworkSessionActivated(GridMate::GridSession* session)
    {
        AZ_Assert(!m_bindingSession, "We already have an active session! Was the previous session deactivated?");
        if (!m_bindingSession)
        {
            m_bindingSession = session;

            if (m_bindingSession->IsHost())
            {
                GridMate::Replica* replica = CreateSystemReplica();
                session->GetReplicaMgr()->AddMaster(replica);
            }
        }
    }

    void NetBindingSystemImpl::OnNetworkSessionDeactivated(GridMate::GridSession* session)
    {
        if (session == m_bindingSession)
        {
            m_bindingSession = nullptr;
        }
    }

    void NetBindingSystemImpl::OnEntityContextReset()
    {
        bool isContextOwner = m_contextData && m_contextData->IsMaster() && m_bindingSession && m_bindingSession->IsHost();
        if (isContextOwner)
        {
            ++m_currentBindingContextSequence;
            NetBindingSystemContextData* context = static_cast<NetBindingSystemContextData*>(m_contextData.get());
            context->m_bindingContextSequence.Set(m_currentBindingContextSequence);
        }
    }

    bool NetBindingSystemImpl::IsAuthoritateLoad() const
    {
        if (m_overrideRootSliceLoadAuthoritative)
        {
            return m_isAuthoritativeRootSliceLoad;
        }

        return !m_bindingSession || m_bindingSession->IsHost();
    }

    void NetBindingSystemImpl::OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList& contextEntities)
    {
        bool isAuthoritativeLoad = IsAuthoritateLoad();

        for (AZ::Entity* entity : contextEntities)
        {
            NetBindingHandlerInterface* netBinder = GetNetBindingHandler(entity);
            if (netBinder)
            {
                netBinder->MarkAsLevelSliceEntity();
            }

            if (!isAuthoritativeLoad && netBinder)
            {
                entity->SetRuntimeActiveByDefault(false);

                auto& slicesQueue = m_bindRequests[GetCurrentContextSequence()];
                auto& sliceHandler = slicesQueue[entity->GetId()];
                BindRequest& request = sliceHandler.m_bindingQueue[entity->GetId()];
                request.m_actualRuntimeEntityId = entity->GetId();
                request.m_requestTime = AZStd::chrono::system_clock::now();
            }
        }
    }

    void NetBindingSystemImpl::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        (void)deltaTime;
        (void)time;
        UpdateContextSequence();
        ProcessBindRequests();
        ProcessSpawnRequests();
    }

    int NetBindingSystemImpl::GetTickOrder()
    {
        return AZ::TICK_PLACEMENT + 1;
    }

    void NetBindingSystemImpl::UpdateContextSequence()
    {
        NetBindingSystemContextData* contextChunk = static_cast<NetBindingSystemContextData*>(m_contextData.get());
        if (m_currentBindingContextSequence != contextChunk->m_bindingContextSequence.Get())
        {
            m_currentBindingContextSequence = contextChunk->m_bindingContextSequence.Get();
        }
    }

    GridMate::Replica* NetBindingSystemImpl::CreateSystemReplica()
    {
        AZ_Assert(m_bindingSession->IsHost(), "CreateSystemReplica should only be called on the host!");
        GridMate::Replica* replica = GridMate::Replica::CreateReplica("NetBindingSystem");
        NetBindingSystemContextData* contextChunk = GridMate::CreateReplicaChunk<NetBindingSystemContextData>();
        replica->AttachReplicaChunk(contextChunk);

        return replica;
    }

    void NetBindingSystemImpl::OnContextDataActivated(GridMate::ReplicaChunkPtr contextData)
    {
        AZ_Assert(!m_contextData, "We already have our context!");
        m_contextData = contextData;

        // Make sure we always have the unspecified entry. This should also
        // be the lower_bound in the map and assuming it is always there
        // makes things simpler.
        m_spawnRequests.insert(UnspecifiedNetBindingContextSequence);
        m_bindRequests.insert(UnspecifiedNetBindingContextSequence);

        if (contextData->IsMaster())
        {
            ++m_currentBindingContextSequence;
            static_cast<NetBindingSystemContextData*>(contextData.get())->m_bindingContextSequence.Set(m_currentBindingContextSequence);
        }
        else
        {
            UpdateContextSequence();
        }
        AZ::TickBus::Handler::BusConnect();
        EBUS_EVENT(AzFramework::NetBindingHandlerBus, BindToNetwork, nullptr);
    }

    void NetBindingSystemImpl::OnContextDataDeactivated(GridMate::ReplicaChunkPtr contextData)
    {
        AZ_Assert(m_contextData == contextData, "This is not our context!");
        m_contextData = nullptr;

        AZ::TickBus::Handler::BusDisconnect();
        m_spawnRequests.clear();
        m_bindRequests.clear();
        m_addMasterRequests.clear();
        m_currentBindingContextSequence = UnspecifiedNetBindingContextSequence;
    }

    void NetBindingSystemImpl::ProcessSpawnRequests()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "NetBindingSystemComponent requires a valid SerializeContext in order to spawn entities!");
        auto SpawnFunc = [=](SpawnRequest& spawnData, AZ::EntityId useEntityId, bool addToContext)
        {
            AZ::Entity* proxyEntity = nullptr;
            AZ::ObjectStream::ClassReadyCB readyCB([&](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* sc)
            {
                (void)classId;
                (void)sc;
                proxyEntity = static_cast<AZ::Entity*>(classPtr);
            });
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > stream(&spawnData.m_spawnDataBuffer);
            AZ::ObjectStream::LoadBlocking(&stream, *serializeContext, readyCB);

            if (proxyEntity)
            {
                proxyEntity->SetId(useEntityId);
                BindAndActivate(proxyEntity, spawnData.m_bindTo, addToContext);
            }
        };

        if (!m_spawnRequests.empty())
        {
            SpawnRequestContextContainerType::iterator itContextQueue = m_spawnRequests.lower_bound(UnspecifiedNetBindingContextSequence);
            AZ_Assert(itContextQueue->first == UnspecifiedNetBindingContextSequence, "We should always have the unspecified (aka global entity) spawn queue!");

            // Process requests for global entities (not part of any context)
            SpawnRequestContainerType& globalQueue = itContextQueue->second;
            for (SpawnRequest& request : globalQueue)
            {
                SpawnFunc(request, request.m_useEntityId, false);
            }
            globalQueue.clear();

            if (GetCurrentContextSequence() != UnspecifiedNetBindingContextSequence)
            {
                ++itContextQueue;

                // Clear any obsolete requests (any contexts below the current context sequence)
                SpawnRequestContextContainerType::iterator itCurrentContextQueue = m_spawnRequests.lower_bound(GetCurrentContextSequence());
                if (itContextQueue != itCurrentContextQueue)
                {
                    m_spawnRequests.erase(itContextQueue, itCurrentContextQueue);
                }

                // Spawn any entities for the current context
                if (itCurrentContextQueue != m_spawnRequests.end())
                {
                    if (itCurrentContextQueue->first == GetCurrentContextSequence())
                    {
                        for (SpawnRequest& request : itCurrentContextQueue->second)
                        {
                            SpawnFunc(request, request.m_useEntityId, true);
                        }
                        itCurrentContextQueue->second.clear();
                    }
                }
            }
        }
    }

    void NetBindingSystemImpl::ProcessBindRequests()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "NetBindingSystemComponent requires a valid SerializeContext in order to spawn entities!");

        if (!m_bindRequests.empty())
        {
            BindRequestContextContainerType::iterator itContextQueue = m_bindRequests.lower_bound(UnspecifiedNetBindingContextSequence);
            AZ_Assert(itContextQueue->first == UnspecifiedNetBindingContextSequence, "We should always have the unspecified/global spawn queue!");

            if (GetCurrentContextSequence() != UnspecifiedNetBindingContextSequence)
            {
                ++itContextQueue;

                // Clear any obsolete requests (any contexts below the current context sequence)
                BindRequestContextContainerType::iterator itCurrentContextQueue = m_bindRequests.lower_bound(GetCurrentContextSequence());
                if (itContextQueue != itCurrentContextQueue)
                {
                    m_bindRequests.erase(itContextQueue, itCurrentContextQueue);
                }

                // Spawn any proxy entities for the current context
                AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
                if (itCurrentContextQueue != m_bindRequests.end())
                {
                    if (itCurrentContextQueue->first == GetCurrentContextSequence())
                    {
                        for (auto itSliceHandler = itCurrentContextQueue->second.begin(); itSliceHandler != itCurrentContextQueue->second.end(); /*++itSliceHandler*/)
                        {
                            NetBindingSliceInstantiationHandler& sliceHandler = itSliceHandler->second;

                            // If this is a new slice request, instantiate it
                            if (!sliceHandler.IsInstantiated())
                            {
                                sliceHandler.InstantiateEntities();
                            }

                            // If the entity is ready to be bound to the network, bind it.
                            // NOTE: It is possible for entities spawned from a slice containing multiple entities with net binding
                            // to never receive their replica counterpart, either because the replica was destroyed, or was interest
                            // filtered. We don't have a very good pipeline to prevent these slices from being authored, so if we
                            // encounter them, we will delete them after 5000ms for now.
                            for (auto itRequest = sliceHandler.m_bindingQueue.begin(); itRequest != sliceHandler.m_bindingQueue.end(); /*++itRequest*/)
                            {
                                BindRequest& request = itRequest->second;
                                if (request.m_bindTo != GridMate::InvalidReplicaId && request.m_actualRuntimeEntityId.IsValid())
                                {
                                    AZ_Warning("NetBindingSystemImpl", request.m_actualRuntimeEntityId == request.m_desiredRuntimeEntityId, "Entity id does not match desired id. Binding may not work as intended!");
                                    AZ::Entity* proxyEntity = nullptr;
                                    EBUS_EVENT_RESULT(proxyEntity, AZ::ComponentApplicationBus, FindEntity, request.m_actualRuntimeEntityId);
                                    if (proxyEntity)
                                    {
                                        BindAndActivate(proxyEntity, request.m_bindTo, false);
                                    }
                                    itRequest = sliceHandler.m_bindingQueue.erase(itRequest);
                                }
                                else if (AZStd::chrono::milliseconds(now - request.m_requestTime).count() > 5000)
                                {
                                    AZ_TracePrintf("NetBindingSystemImpl", "Entity with static id %llu is still unbound after 5000ms. Discarding unbound entity.", static_cast<AZ::u64>(itRequest->first));
                                    AZ::Entity* unboundEntity = nullptr;
                                    EBUS_EVENT_RESULT(unboundEntity, AZ::ComponentApplicationBus, FindEntity, request.m_actualRuntimeEntityId);
                                    if (unboundEntity)
                                    {
                                        EBUS_EVENT(GameEntityContextRequestBus, DestroyGameEntity, unboundEntity->GetId());
                                    }
                                    itRequest = sliceHandler.m_bindingQueue.erase(itRequest);
                                }
                                else
                                {
                                    ++itRequest;
                                }
                            }

                            if (sliceHandler.IsBindingComplete())
                            {
                                itSliceHandler = itCurrentContextQueue->second.erase(itSliceHandler);
                            }
                            else
                            {
                                ++itSliceHandler;
                            }
                        }
                    }
                }
            }
        }

        // Spawn replicas for any local entities that are still valid
        for (auto& addRequest : m_addMasterRequests)
        {
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, addRequest.first);
            if (entity)
            {
                m_bindingSession->GetReplicaMgr()->AddMaster(addRequest.second);
            }
        }
        m_addMasterRequests.clear();
    }

    void NetBindingSystemImpl::BindAndActivate(AZ::Entity* entity, GridMate::ReplicaId replicaId, bool addToContext)
    {
        bool success = false;
        if (ShouldBindToNetwork())
        {
            GridMate::ReplicaPtr bindTo = m_contextData->GetReplicaManager()->FindReplica(replicaId);
            if (bindTo)
            {
                if (addToContext)
                {
                    EBUS_EVENT(GameEntityContextRequestBus, AddGameEntity, entity);
                }

                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }

                NetBindingHandlerInterface* binding = GetNetBindingHandler(entity);
                AZ_Assert(binding, "Can't find NetBindingHandlerInterface!");
                binding->BindToNetwork(bindTo);

                entity->Activate();
                success = true;
            }
        }

        if (!success)
        {
            // If binding failed for whatever reason, destroy the entity that was spawned.
            AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
            EBUS_EVENT_ID_RESULT(contextId, entity->GetId(), AzFramework::EntityIdContextQueryBus, GetOwningContextId);
            if (contextId.IsNull())
            {
                delete entity;
            }
            else
            {
                EBUS_EVENT(GameEntityContextRequestBus, DestroyGameEntity, entity->GetId());
            }
        }
    }

    void NetBindingSystemImpl::Reflect(AZ::ReflectContext* context)
    {
        if (context)
        {
            // We need to register the chunk type, and this would be a good time to do so.
            if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(NetBindingSystemContextData::GetChunkName())))
            {
                GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<AzFramework::NetBindingSystemContextData>();
            }
        }
    }
} // namespace AzFramework
