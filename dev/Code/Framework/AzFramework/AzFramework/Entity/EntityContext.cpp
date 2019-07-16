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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>

#include "EntityContext.h"

namespace AzFramework
{
    //=========================================================================
    // Reflect
    //=========================================================================
    void EntityContext::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // EntityContext entity data is serialized through streams / Ebus messages.
            serializeContext->Class<EntityContext>()
                ->Version(1)
                ;
                
            serializeContext->Class<AzFramework::SliceInstantiationTicket>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::SliceInstantiationTicket>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("Equal", &AzFramework::SliceInstantiationTicket::operator==)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("IsValid", &AzFramework::SliceInstantiationTicket::operator bool)
                ;
        }
    }

    //=========================================================================
    // EntityContext ctor
    //=========================================================================
    EntityContext::EntityContext(AZ::SerializeContext* serializeContext /*= nullptr*/)
        : EntityContext(EntityContextId::CreateRandom(), serializeContext)
    {
        EntityContextRequestBus::Handler::BusConnect(m_contextId);
    }

    //=========================================================================
    // EntityContext ctor
    //=========================================================================
    EntityContext::EntityContext(const AZ::Uuid& contextId, AZ::SerializeContext* serializeContext /*= nullptr*/)
        : m_serializeContext(serializeContext)
        , m_contextId(contextId)
        , m_nextSliceTicket(0)
    {
        if (nullptr == serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve application serialization context.");
        }

        EntityContextRequestBus::Handler::BusConnect(m_contextId);
        EntityContextEventBus::Bind(m_eventBusPtr, m_contextId);
    }

    //=========================================================================
    // EntityContext dtor
    //=========================================================================
    EntityContext::~EntityContext()
    {
        m_eventBusPtr = nullptr;
        DestroyContext();
    }

    //=========================================================================
    // InitContext
    //=========================================================================
    void EntityContext::InitContext()
    {
        if (!m_rootAsset)
        {
            m_rootAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), false);
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_rootAsset.GetId());
        }

        CreateRootSlice();
    }

    //=========================================================================
    // CreateRootSlice
    //=========================================================================
    void EntityContext::CreateRootSlice()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(m_rootAsset && m_rootAsset.Get(), "The context has not been initialized.");

        AZ::Entity* rootEntity = new AZ::Entity();
        rootEntity->CreateComponent<AZ::SliceComponent>();

        if (m_rootAsset.Get()->GetEntity() != nullptr)
        {
            OnRootSlicePreDestruction();
        }

        AZ::EntityBus::MultiHandler::BusConnect(rootEntity->GetId());
        // Manually create an asset to hold the root slice.
        m_rootAsset.Get()->SetData(rootEntity, rootEntity->FindComponent<AZ::SliceComponent>());
        auto* rootSliceComponent = m_rootAsset.Get()->GetComponent();
        rootSliceComponent->InitMetadata();
        rootSliceComponent->SetMyAsset(m_rootAsset.Get());
        rootSliceComponent->SetSerializeContext(m_serializeContext);
        rootSliceComponent->ListenForAssetChanges();

        // Editor context slice is always dynamic by default. Whether it's a "level",
        // or something else, it can be instantiated at runtime.
        rootSliceComponent->SetIsDynamic(true);

        // Make sure the root slice metadata entity is marked as persistent.
        AZ::Entity* metadataEntity = rootSliceComponent->GetMetadataEntity();

        if (metadataEntity)
        {
            AZ::SliceMetadataInfoComponent* infoComponent = metadataEntity->FindComponent<AZ::SliceMetadataInfoComponent>();

            if (infoComponent)
            {
                infoComponent->MarkAsPersistent(true);
            }

            HandleNewMetadataEntitiesCreated(*rootSliceComponent);
        }

        OnRootSliceCreated();
    }

    //=========================================================================
    // DestroyContext
    //=========================================================================
    void EntityContext::DestroyContext()
    {
        if (m_rootAsset)
        {
            ResetContext();

            AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_rootAsset.GetId());
            AZ::EntityBus::MultiHandler::BusDisconnect(m_rootAsset.Get()->GetEntity()->GetId());
            m_rootAsset = nullptr;
        }
    }

    //=========================================================================
    // ResetContext
    //=========================================================================
    void EntityContext::ResetContext()
    {
        if (!m_rootAsset)
        {
            return;
        }

        PrepareForContextReset();
        while (!m_queuedSliceInstantiations.empty())
        {
            // clear out the remaining instantiations in a conservative manner, assuming that callbacks such as OnSliceInstantiationFailed
            // will call back into us and potentially mutate this list.
            const InstantiatingSliceInfo& instantiating = m_queuedSliceInstantiations.back();

            // 'instantiating' is deleted during this loop, so capture the asset Id and Ticket before we continue and destroy it.
            AZ::Data::AssetId idToNotify = instantiating.m_asset.GetId();
            AzFramework::SliceInstantiationTicket ticket = instantiating.m_ticket;

            // this will decrement the refcount of the asset, which could mean its invalid by the next line.
            // the above line also ensures that our list no longer contains this particular instantiation.
            // its important to do that, before calling any callbacks, because some listeners on the following functions
            // may call additional functions on this context, and we could get into a situation
            // where we end up iterating over this list again (before returning from the below bus calls).
            m_queuedSliceInstantiations.pop_back();

            AZ::Data::AssetBus::MultiHandler::BusDisconnect(idToNotify);
            EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnSliceInstantiationFailed, idToNotify);
            DispatchOnSliceInstantiationFailed(ticket, idToNotify, true);
        }

        DestroyRootSliceEntities();

        // Re-create fresh root slice asset.
        CreateRootSlice();

        OnContextReset();

        // Notify listeners.
        EntityContextEventBus::Event(m_contextId, &EntityContextEventBus::Events::OnEntityContextReset);
    }

    //=========================================================================
    // GetLoadedEntityIdMap
    //=========================================================================
    const AZ::SliceComponent::EntityIdToEntityIdMap& EntityContext::GetLoadedEntityIdMap()
    {
        return m_loadedEntityIdMap;
    }

    AZ::EntityId EntityContext::FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const
    {
        auto idIter = m_loadedEntityIdMap.find(staticId);

        if (idIter == m_loadedEntityIdMap.end())
        {
            return AZ::EntityId();
        }

        return idIter->second;
    }

    //=========================================================================
    // HandleEntitiesAdded
    //=========================================================================
    void EntityContext::HandleEntitiesAdded(const EntityList& entities)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        for (AZ::Entity* entity : entities)
        {
            AZ::EntityBus::MultiHandler::BusConnect(entity->GetId());
            EntityIdContextQueryBus::MultiHandler::BusConnect(entity->GetId());
            EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnEntityContextCreateEntity, *entity);
        }

        OnContextEntitiesAdded(entities);
    }

    //=========================================================================
    // HandleEntityRemoved
    //=========================================================================
    void EntityContext::HandleEntityRemoved(const AZ::EntityId& id)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        OnContextEntityRemoved(id);

        EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnEntityContextDestroyEntity, id);
        EntityIdContextQueryBus::MultiHandler::BusDisconnect(id);
        AZ::EntityBus::MultiHandler::BusDisconnect(id);
    }

    //=========================================================================
    // ValidateEntitiesAreValidForContext
    //=========================================================================
    bool EntityContext::ValidateEntitiesAreValidForContext(const EntityList&)
    {
        return true;
    }

    //=========================================================================
    // IsOwnedByThisContext
    //=========================================================================
    bool EntityContext::IsOwnedByThisContext(const AZ::EntityId& entityId)
    {
        // Get ID of the owning context of the incoming entity ID and compare it to
        // the id of this context.
        EntityContextId owningContextId = EntityContextId::CreateNull();
        EntityIdContextQueryBus::EventResult(owningContextId, entityId, &EntityIdContextQueryBus::Events::GetOwningContextId);
        return owningContextId == m_contextId;
    }

    //=========================================================================
    // GetRootSlice
    //=========================================================================
    AZ::SliceComponent* EntityContext::GetRootSlice()
    {
        return m_rootAsset ? m_rootAsset.Get()->GetComponent() : nullptr;
    }

    //=========================================================================
    // CurrentlyInstantiatingSlice
    //=========================================================================
    AZ::Data::AssetId EntityContext::CurrentlyInstantiatingSlice()
    {
        return m_instantiatingAssetId;
    }

    //=========================================================================
    // CreateEntity
    //=========================================================================
    AZ::Entity* EntityContext::CreateEntity(const char* name)
    {
        AZ::Entity* entity = aznew AZ::Entity(name);

        AddEntity(entity);

        return entity;
    }

    //=========================================================================
    // AddEntity
    //=========================================================================
    void EntityContext::AddEntity(AZ::Entity* entity)
    {
        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ_Assert(!EntityIdContextQueryBus::FindFirstHandler(entity->GetId()), "Entity already belongs to a context.");

        AZ::SliceAsset* rootSlice = m_rootAsset.Get();
        rootSlice->GetComponent()->AddEntity(entity);

        HandleEntitiesAdded(EntityList { entity });
    }

    //=========================================================================
    // ActivateEntity
    //=========================================================================
    void EntityContext::ActivateEntity(AZ::EntityId entityId)
    {
        // Verify that this context has the right to perform operations on the entity
        bool validEntity = IsOwnedByThisContext(entityId);
        AZ_Warning("GameEntityContext", validEntity, "Entity with id %llu does not belong to the game context.", entityId);

        if (validEntity)
        {
            // Look up the entity and activate it.
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            if (entity)
            {
                // Safety Check: Is the entity initialized?
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    AZ_Warning("GameEntityContext", false, "Entity with id %llu was not initialized before activation requested.", entityId);
                    entity->Init();
                }

                if (entity->GetState() == AZ::Entity::ES_INIT)
                {
                    entity->Activate();
                }
            }
        }
    }

    //=========================================================================
    // DeactivateEntity
    //=========================================================================
    void EntityContext::DeactivateEntity(AZ::EntityId entityId)
    {
        // Verify that this context has the right to perform operations on the entity
        bool validEntity = IsOwnedByThisContext(entityId);
        AZ_Warning("GameEntityContext", validEntity, "Entity with id %llu does not belong to the game context.", entityId);

        if (validEntity)
        {
            // Then look up the entity and deactivate it.
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            if (entity)
            {
                switch (entity->GetState())
                {
                case AZ::Entity::ES_ACTIVATING:
                    // Queue deactivate to trigger next frame
                    AZ::TickBus::QueueFunction(&AZ::Entity::Deactivate, entity);
                    break;

                case AZ::Entity::ES_ACTIVE:
                    // Deactivate immediately
                    entity->Deactivate();
                    break;

                default:
                    // Don't do anything, it's not even active.
                    break;
                }
            }
        }
    }

    //=========================================================================
    // DestroyEntity
    //=========================================================================
    bool EntityContext::DestroyEntity(AZ::Entity* entity)
    {
        AZ_Assert(entity, "Invalid entity passed to DestroyEntity");
        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::SliceAsset* rootSlice = m_rootAsset.Get();

        bool isOwnedByThisContext = IsOwnedByThisContext(entity->GetId());
        AZ_Assert(isOwnedByThisContext, "Entity does not belong to this context, and therefore can not be safely destroyed by this context.");

        if (isOwnedByThisContext)
        {
            HandleEntityRemoved(entity->GetId());

            rootSlice->GetComponent()->RemoveEntity(entity);
            return true;
        }

        return false;
    }

    //=========================================================================
    // DestroyEntity
    //=========================================================================
    bool EntityContext::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        if (entity)
        {
            return DestroyEntity(entity);
        }

        return false;
    }

    //=========================================================================
    // DestroyRootSliceEntities
    //=========================================================================
    void EntityContext::DestroyRootSliceEntities()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(m_rootAsset, "The context has not been initialized.");
        EntityIdList entityIds = GetRootSliceEntityIds();

        for (auto entityId : entityIds)
        {
            AZ_Assert(IsOwnedByThisContext(entityId), "Entity does not belong to this context, and therefore can not be safely destroyed by this context.");
            HandleEntityRemoved(entityId);
        }

        AZ::SliceAsset* rootSlice = m_rootAsset.Get();
        rootSlice->GetComponent()->RemoveAllEntities();
    }

    //=========================================================================
    // CloneEntity
    //=========================================================================
    AZ::Entity* EntityContext::CloneEntity(const AZ::Entity& sourceEntity)
    {
        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialization context.");

        AZ::Entity* entity = serializeContext->CloneObject(&sourceEntity);
        AZ_Error("EntityContext", entity != nullptr, "Failed to clone source entity.");

        if (entity)
        {
            entity->SetId(AZ::Entity::MakeId());
            AddEntity(entity);
        }

        return entity;
    }

    //=========================================================================
    // GetRootSliceEntityIds
    //=========================================================================
    EntityContext::EntityIdList EntityContext::GetRootSliceEntityIds()
    {
        EntityIdList entityIds;

        const AZ::SliceComponent* rootSliceComponent = m_rootAsset.Get()->GetComponent();
        if (!rootSliceComponent->IsInstantiated())
        {
            return entityIds;
        }

        const EntityList& looseEntities = rootSliceComponent->GetNewEntities();
        entityIds.reserve(looseEntities.size());
        for (const AZ::Entity* entity : looseEntities)
        {
            entityIds.push_back(entity->GetId());
        }

        const AZ::SliceComponent::SliceList& subSlices = rootSliceComponent->GetSlices();
        for (const AZ::SliceComponent::SliceReference& subSlice : subSlices)
        {
            for (const AZ::SliceComponent::SliceInstance& instance : subSlice.GetInstances())
            {
                for (const AZ::Entity* entity : instance.GetInstantiated()->m_entities)
                {
                    entityIds.push_back(entity->GetId());
                }
            }
        }

        return entityIds;
    }

    //=========================================================================
    // InstantiateSlice
    //=========================================================================
    SliceInstantiationTicket EntityContext::InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper, const AZ::Data::AssetFilterCB& assetLoadFilter)
    {
        if (asset.GetId().IsValid())
        {
            const SliceInstantiationTicket ticket(m_contextId, ++m_nextSliceTicket);
            m_queuedSliceInstantiations.emplace_back(asset, ticket, customIdMapper);
            m_queuedSliceInstantiations.back().m_asset.QueueLoad(assetLoadFilter);

            AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());

            return ticket;
        }

        return SliceInstantiationTicket();
    }

    //=========================================================================
    // CancelSliceInstantiation
    //=========================================================================
    void EntityContext::CancelSliceInstantiation(const SliceInstantiationTicket& ticket)
    {
        auto iter = AZStd::find_if(m_queuedSliceInstantiations.begin(), m_queuedSliceInstantiations.end(),
            [ticket](const InstantiatingSliceInfo& instantiating)
            {
                return instantiating.m_ticket == ticket;
            });

        if (iter != m_queuedSliceInstantiations.end())
        {
            const AZ::Data::AssetId assetId = iter->m_asset.GetId();

            // Erase ticket, but stay connected to AssetBus in case asset is used by multiple tickets.
            m_queuedSliceInstantiations.erase(iter);
            iter = m_queuedSliceInstantiations.end(); // clear the iterator so that code inserted after this point to operate on iter will raise issues.

            // No need to queue this notification.
            // (It's queued in other circumstances, to avoid holding the AssetBus lock any longer than necessary)
            DispatchOnSliceInstantiationFailed(ticket, assetId, true);
        }
    }

    //=========================================================================
    // CloneSliceInstance
    //=========================================================================
    AZ::SliceComponent::SliceInstanceAddress EntityContext::CloneSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress sourceInstance, AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(sourceInstance.IsValid(), "Source slice instance is invalid.");

        AZ::SliceComponent::SliceInstance* newInstance = sourceInstance.GetReference()->CloneInstance(sourceInstance.GetInstance(), sourceToCloneEntityIdMap);

        return AZ::SliceComponent::SliceInstanceAddress(sourceInstance.GetReference(), newInstance);
    }

    //=========================================================================
    // LoadFromStream
    //=========================================================================
    bool EntityContext::LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::Entity* newRootEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(stream, m_serializeContext, filterDesc);

        // make sure that PRE_NOTIFY assets get their notify before we activate, so that we can preserve the order of 
        // (load asset) -> (notify) -> (init) -> (activate)
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // for other kinds of instantiations, like slice instantiations, becuase they use the queued slice instantiation mechanism, they will always
        // be instantiated after their asset is already ready.

        return HandleLoadedRootSliceEntity(newRootEntity, remapIds, idRemapTable);
    }

    //=========================================================================
    // HandleLoadedRootSliceEntity
    //=========================================================================
    bool EntityContext::HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        if (!rootEntity)
        {
            return false;
        }

        // Flush asset database events after serialization, so all loaded asset statuses are updated.
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().DispatchEvents();
        }

        AZ::SliceComponent* newRootSlice = rootEntity->FindComponent<AZ::SliceComponent>();

        if (!newRootSlice)
        {
            AZ_Error("EntityContext", false, "Loaded root entity is not a slice.");
            return false;
        }

        ResetContext();

        m_rootAsset.Get()->SetData(rootEntity, newRootSlice, true);
        newRootSlice->SetMyAsset(m_rootAsset.Get());
        newRootSlice->SetSerializeContext(m_serializeContext);
        newRootSlice->ListenForAssetChanges();

        m_loadedEntityIdMap.clear();
        if (remapIds)
        {
            newRootSlice->GenerateNewEntityIds(&m_loadedEntityIdMap);
            if (idRemapTable)
            {
                *idRemapTable = m_loadedEntityIdMap;
            }
        }
        

        AZ::SliceComponent::EntityList entities;
        newRootSlice->GetEntities(entities);

        if (!remapIds)
        {
            for (AZ::Entity* entity : entities)
            {
                m_loadedEntityIdMap.emplace(entity->GetId(), entity->GetId());
            }
        }

        // Make sure the root slice metadata entity is marked as persistent.
        AZ::Entity* metadataEntity = newRootSlice->GetMetadataEntity();

        if (!metadataEntity)
        {
            AZ_Error("EntityContext", false, "Root entity must have a metadata entity");
            return false;
        }

        AZ::SliceMetadataInfoComponent* infoComponent = metadataEntity->FindComponent<AZ::SliceMetadataInfoComponent>();

        if (!infoComponent)
        {
            AZ_Error("EntityContext", false, "Root metadata entity must have a valid info component");
            return false;
        }

        infoComponent->MarkAsPersistent(true);

        EntityContextEventBus::Event(m_contextId, &EntityContextEventBus::Events::OnEntityContextLoadedFromStream, entities);

        HandleEntitiesAdded(entities);

        HandleNewMetadataEntitiesCreated(*newRootSlice);

        AZ::Data::AssetBus::MultiHandler::BusConnect(m_rootAsset.GetId());
        return true;
    }

    //=========================================================================
    // EntityBus::OnEntityDestruction
    //=========================================================================
    void EntityContext::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        EntityContextId owningContextId = EntityContextId::CreateNull();
        EntityIdContextQueryBus::EventResult(owningContextId, entityId, &EntityIdContextQueryBus::Events::GetOwningContextId);
        if (owningContextId == m_contextId)
        {
            HandleEntityRemoved(entityId);

            if (m_rootAsset)
            {
                // Entities removed through the application (as in via manual 'delete'),
                // should be removed from the root slice, but not again deleted.
                GetRootSlice()->RemoveEntity(entityId, false);
            }
        }
    }

    //=========================================================================
    // EntityContextQueryBus::GetOwningSlice
    //=========================================================================
    AZ::SliceComponent::SliceInstanceAddress EntityContext::GetOwningSlice()
    {
        const AZ::EntityId entityId = *EntityIdContextQueryBus::GetCurrentBusId();
        return GetOwningSliceForEntity(entityId);
    }

    //=========================================================================
    // GetOwningSliceForEntity
    //=========================================================================
    AZ::SliceComponent::SliceInstanceAddress EntityContext::GetOwningSliceForEntity(AZ::EntityId entityId) const
    {
        AZ_Assert(m_rootAsset, "The context has not been initialized.");
        return m_rootAsset.Get()->GetComponent()->FindSlice(entityId);
    }

    //=========================================================================
    // OnAssetError - Slice asset failed
    //=========================================================================
    void EntityContext::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_rootAsset)
        {
            return;
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

        EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnSliceInstantiationFailed, asset.GetId());

        for (auto iter = m_queuedSliceInstantiations.begin(); iter != m_queuedSliceInstantiations.end(); )
        {
            const InstantiatingSliceInfo& instantiating = *iter;

            if (instantiating.m_asset.GetId() == asset.GetId())
            {
                // grab a refcount on the asset and copy the ticket, as 'instantiating' is about to be destroyed!
                AZ::Data::AssetId cachedId = instantiating.m_asset.GetId();
                SliceInstantiationTicket ticket = instantiating.m_ticket; 

                AZStd::function<void()> notifyCallback =
                    [cachedId, ticket]() // capture these by value since we're about to leave the scope in which these variables exist.
                    {
                        DispatchOnSliceInstantiationFailed(ticket, cachedId, false);
                    };

                // Instantiation is queued against the tick bus. This ensures we're not holding the AssetBus lock
                // while the instantiation is handled, which may be costly.
                AZ::TickBus::QueueFunction(notifyCallback);

                iter = m_queuedSliceInstantiations.erase(iter); // this invalidates the instantiating data.
            }
            else
            {
                ++iter;
            }
        }
    }

    //=========================================================================
    // OnAssetReady - Slice asset available for instantiation
    //=========================================================================
    void EntityContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> readyAsset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(readyAsset.GetAs<AZ::SliceAsset>(), "Asset is not a slice!");

        if (readyAsset == m_rootAsset)
        {
            return;
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(readyAsset.GetId());

        AZStd::function<void()> instantiateCallback =
            [this, readyAsset]() // we intentionally capture readyAsset by value here, so that its refcount doesn't hit 0 by the time this call happens.
            {
                const AZ::Data::AssetId readyAssetId = readyAsset.GetId();
                for (auto iter = m_queuedSliceInstantiations.begin(); iter != m_queuedSliceInstantiations.end(); )
                {
                    const InstantiatingSliceInfo& instantiating = *iter;

                    if (instantiating.m_asset.GetId() == readyAssetId)
                    {
                        // here we actually refcount / copy by value the internals of 'instantiating' since we will destroy it later
                        // but still wish to send bus messages based on ticket/asset.
                        AZ::Data::Asset<AZ::Data::AssetData> asset = instantiating.m_asset;
                        SliceInstantiationTicket ticket = instantiating.m_ticket;
                        m_instantiatingAssetId = instantiating.m_asset.GetId();
                        AZ::SliceComponent::SliceInstanceAddress instance = m_rootAsset.Get()->GetComponent()->AddSlice(asset, instantiating.m_customMapper);

                        // its important to remove this instantiation from the instantiation list
                        // as soon as possible, before we call these below notification functions, because they might result in our own functions
                        // that search this list being called again.
                        iter = m_queuedSliceInstantiations.erase(iter);
                        // --------------------------- do not refer to 'instantiating' after the above call, it has been destroyed ------------
                        
                        bool isSliceInstantiated = false;

                        if (instance.IsValid())
                        {
                            AZ_Assert(instance.GetInstance()->GetInstantiated(), "Failed to instantiate root slice!");

                            if (instance.GetInstance()->GetInstantiated() &&
                                ValidateEntitiesAreValidForContext(instance.GetInstance()->GetInstantiated()->m_entities))
                            {
                                EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnSlicePreInstantiate, m_instantiatingAssetId, instance);
                                SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSlicePreInstantiate, m_instantiatingAssetId, instance);

                                HandleEntitiesAdded(instance.GetInstance()->GetInstantiated()->m_entities);

                                EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnSliceInstantiated, m_instantiatingAssetId, instance);
                                SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSliceInstantiated, m_instantiatingAssetId, instance);

                                isSliceInstantiated = true;
                            }
                            else
                            {
                                // The prefab has already been added to the root slice. But we are disallowing the
                                // instantiation. So we need to remove it
                                m_rootAsset.Get()->GetComponent()->RemoveSlice(asset);
                            }
                        }
                      
                        if (!isSliceInstantiated)
                        {
                            EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnSliceInstantiationFailed, m_instantiatingAssetId);
                            DispatchOnSliceInstantiationFailed(ticket, m_instantiatingAssetId, false);
                        }

                        // clear the Asset ID cache
                        m_instantiatingAssetId.SetInvalid();
                    }
                    else
                    {
                        ++iter;
                    }
                }
            };

        // Instantiation is queued against the tick bus. This ensures we're not holding the AssetBus lock
        // while the instantiation is handled, which may be costly. This also guarantees callers can
        // jump on the SliceInstantiationResultBus for their ticket before the events are fired.
        AZ::TickBus::QueueFunction(instantiateCallback);
    }

    //=========================================================================
    // OnAssetReloaded - Root slice (or its dependents) has been reloaded.
    //=========================================================================
    void EntityContext::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        if (asset == m_rootAsset && asset.Get() != m_rootAsset.Get())
        {
            ResetContext();

            m_rootAsset = asset;

            auto* rootSliceComponent = m_rootAsset.Get()->GetComponent();

            // because cloned components don't listen for changes by default as they are usually discarded, we need to manually listen here - root is special in this way
            rootSliceComponent->ListenForAssetChanges(); 

            HandleNewMetadataEntitiesCreated(*m_rootAsset.Get()->GetComponent());

            AZ::SliceComponent::EntityList entities;
            m_rootAsset.Get()->GetComponent()->GetEntities(entities);

            HandleEntitiesAdded(entities);

            m_rootAsset.Get()->GetComponent()->ListenForDependentAssetChanges();
        }
    }

    //=========================================================================
    // DispatchOnSliceInstantiationFailed - Helper function to send OnSliceInstantiationFailed events.
    //=========================================================================
    void EntityContext::DispatchOnSliceInstantiationFailed(const SliceInstantiationTicket& ticket, const AZ::Data::AssetId& assetId, bool canceled)
    {
        SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSliceInstantiationFailed, assetId);
        SliceInstantiationResultBus::Event(ticket, &SliceInstantiationResultBus::Events::OnSliceInstantiationFailedOrCanceled, assetId, canceled);
    }

} // namespace AzFramework
