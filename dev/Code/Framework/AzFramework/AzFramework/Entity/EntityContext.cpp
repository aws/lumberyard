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
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/containers/stack.h>

#include "EntityContext.h"

namespace AzFramework
{
    //=========================================================================
    // ReflectSerialize
    //=========================================================================
    void EntityContext::ReflectSerialize(AZ::SerializeContext& serialize)
    {
        // EntityContext entity data is serialized through streams / Ebus messages.
        serialize.Class<EntityContext>()
            ->Version(1)
            ->SerializerForEmptyClass()
        ;
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
            EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve application serialization context.");
        }

        EntityContextRequestBus::Handler::BusConnect(m_contextId);
        EntityContextEventBus::Bind(m_eventBusPtr, m_contextId);
        AZ::ComponentApplicationEventBus::Handler::BusConnect();
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

        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::Entity* rootEntity = new AZ::Entity();
        rootEntity->CreateComponent<AZ::SliceComponent>();

        // Manually create an asset to hold the root slice.
        m_rootAsset.Get()->SetData(rootEntity, rootEntity->FindComponent<AZ::SliceComponent>());
        m_rootAsset.Get()->GetComponent()->SetMyAsset(m_rootAsset.Get());
        m_rootAsset.Get()->GetComponent()->SetSerializeContext(m_serializeContext);
        m_rootAsset.Get()->GetComponent()->ListenForAssetChanges();

        // Editor context slice is always dynamic by default. Whether it's a "level",
        // or something else, it can be instantiated at runtime.
        m_rootAsset.Get()->GetComponent()->SetIsDynamic(true);

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
            m_rootAsset = nullptr;
        }
    }

    //=========================================================================
    // ResetContext
    //=========================================================================
    void EntityContext::ResetContext()
    {
        if (m_rootAsset)
        {
            EntityIdList entityIds = GetRootSliceEntityIds();

            for (AZ::EntityId entityId : entityIds)
            {
                DestroyEntity(entityId);
            }

            // Re-create fresh root slice asset.
            CreateRootSlice();

            OnContextReset();

            // Notify listeners.
            EBUS_EVENT_ID(m_contextId, EntityContextEventBus, OnEntityContextReset);
        }
    }

    //=========================================================================
    // GetLoadedEntityIdMap
    //=========================================================================
    const AZ::SliceComponent::EntityIdToEntityIdMap& EntityContext::GetLoadedEntityIdMap()
    {
        return m_loadedEntityIdMap;
    }

    //=========================================================================
    // HandleEntitiesAdded
    //=========================================================================
    void EntityContext::HandleEntitiesAdded(const EntityList& entities)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        for (AZ::Entity* entity : entities)
        {
            EntityIdContextQueryBus::MultiHandler::BusConnect(entity->GetId());
            EBUS_EVENT_PTR(m_eventBusPtr, EntityContextEventBus, OnEntityContextCreateEntity, *entity);
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

        EBUS_EVENT_PTR(m_eventBusPtr, EntityContextEventBus, OnEntityContextDestroyEntity, id);
        EntityIdContextQueryBus::MultiHandler::BusDisconnect(id);
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
        EBUS_EVENT_ID_RESULT(owningContextId, entityId, EntityIdContextQueryBus, GetOwningContextId);
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
    // DestroyEntity
    //=========================================================================
    bool EntityContext::DestroyEntity(AZ::Entity* entity)
    {
        AZ_Assert(entity, "Invalid entity passed to DestroyEntity");
        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::SliceAsset* rootSlice = m_rootAsset.Get();

        EntityContextId owningContextId = EntityContextId::CreateNull();
        EBUS_EVENT_ID_RESULT(owningContextId, entity->GetId(), EntityIdContextQueryBus, GetOwningContextId);
        AZ_Assert(owningContextId == m_contextId, "Entity does not belong to this context, and therefore can not be safely destroyed by this context.");

        if (owningContextId == m_contextId)
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
    bool EntityContext::DestroyEntity(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

        if (entity)
        {
            return DestroyEntity(entity);
        }

        return false;
    }

    //=========================================================================
    // CloneEntity
    //=========================================================================
    AZ::Entity* EntityContext::CloneEntity(const AZ::Entity& sourceEntity)
    {
        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
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
    SliceInstantiationTicket EntityContext::InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZ::EntityUtils::EntityIdMapper& customIdMapper)
    {
        if (asset.GetId().IsValid())
        {
            const SliceInstantiationTicket ticket(m_contextId, ++m_nextSliceTicket);
            m_queuedSliceInstantiations.emplace_back(asset, ticket, customIdMapper);
            m_queuedSliceInstantiations.back().m_asset.QueueLoad();

            AZ::Data::AssetBus::MultiHandler::BusConnect(asset.GetId());

            return ticket;
        }

        return SliceInstantiationTicket();
    }

    //=========================================================================
    // CloneSliceInstance
    //=========================================================================
    AZ::SliceComponent::SliceInstanceAddress EntityContext::CloneSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress sourceInstance, AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(sourceInstance.second, "Source slice instance is invalid.");

        AZ::SliceComponent::SliceInstance* newInstance = sourceInstance.first->CloneInstance(sourceInstance.second, sourceToCloneEntityIdMap);

        return AZ::SliceComponent::SliceInstanceAddress(sourceInstance.first, newInstance);
    }

    //=========================================================================
    // LoadFromStream
    //=========================================================================
    bool EntityContext::LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds, AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(m_rootAsset, "The context has not been initialized.");

        AZ::Entity* newRootEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(stream, m_serializeContext, filterDesc);

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

        AZ::SliceComponent::EntityList entities;
        newRootSlice->GetEntities(entities);

        if (remapIds)
        {
            AZ::SliceComponent::EntityIdToEntityIdMap entityIdMap;
            AZ::SliceComponent::InstantiatedContainer entityContainer;
            entityContainer.m_entities = AZStd::move(entities);
            AZ::EntityUtils::GenerateNewIdsAndFixRefs(&entityContainer, idRemapTable ? *idRemapTable : entityIdMap, m_serializeContext);
            entities = AZStd::move(entityContainer.m_entities);
            m_loadedEntityIdMap = AZStd::move(entityIdMap);
        }
        else
        {
            m_loadedEntityIdMap.clear();
        }

        EBUS_EVENT_ID(m_contextId, EntityContextEventBus, OnEntityContextLoadedFromStream, entities);

        HandleEntitiesAdded(entities);

        AZ::Data::AssetBus::MultiHandler::BusConnect(m_rootAsset.GetId());
        return true;
    }

    //=========================================================================
    // ComponentApplicationEventBus::OnEntityRemoved
    //=========================================================================
    void EntityContext::OnEntityRemoved(const AZ::EntityId& entityId)
    {
        EntityContextId owningContextId = EntityContextId::CreateNull();
        EBUS_EVENT_ID_RESULT(owningContextId, entityId, EntityIdContextQueryBus, GetOwningContextId);
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

        EBUS_EVENT_PTR(m_eventBusPtr, EntityContextEventBus, OnSliceInstantiationFailed, asset.GetId());

        for (auto iter = m_queuedSliceInstantiations.begin(); iter != m_queuedSliceInstantiations.end(); )
        {
            const InstantiatingSliceInfo& instantiating = *iter;

            if (instantiating.m_asset.GetId() == asset.GetId())
            {
                AZStd::function<void()> notifyCallback =
                    [instantiating, this]()
                    {
                        const AZ::Data::Asset<AZ::Data::AssetData>& asset = instantiating.m_asset;
                        const SliceInstantiationTicket& ticket = instantiating.m_ticket;
                        EBUS_EVENT_ID(ticket, SliceInstantiationResultBus, OnSliceInstantiationFailed, asset.GetId());
                    };

                EBUS_QUEUE_FUNCTION(AZ::TickBus, notifyCallback);

                iter = m_queuedSliceInstantiations.erase(iter);
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
    void EntityContext::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzFramework);

        AZ_Assert(asset.GetAs<AZ::SliceAsset>(), "Asset is not a slice!");

        if (asset == m_rootAsset)
        {
            return;
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

        for (auto iter = m_queuedSliceInstantiations.begin(); iter != m_queuedSliceInstantiations.end(); )
        {
            const InstantiatingSliceInfo& instantiating = *iter;

            if (instantiating.m_asset.GetId() == asset.GetId())
            {                
                AZStd::function<void()> instantiateCallback =
                    [instantiating, this]()
                    {
                        const AZ::Data::Asset<AZ::Data::AssetData>& asset = instantiating.m_asset;
                        const SliceInstantiationTicket& ticket = instantiating.m_ticket;

                        bool isSliceInstantiated = false;
                        AZ::SliceComponent::SliceInstanceAddress instance = m_rootAsset.Get()->GetComponent()->AddSlice(asset, instantiating.m_customMapper);
                        if (instance.second)
                        {
                            AZ_Assert(instance.second->GetInstantiated(), "Failed to instantiate root slice!");

                            if (instance.second->GetInstantiated() &&
                                ValidateEntitiesAreValidForContext(instance.second->GetInstantiated()->m_entities))
                            {
                                EBUS_EVENT_PTR(m_eventBusPtr, EntityContextEventBus, OnSlicePreInstantiate, asset.GetId(), instance);
                                EBUS_EVENT_ID(ticket, SliceInstantiationResultBus, OnSlicePreInstantiate, asset.GetId(), instance);

                                HandleEntitiesAdded(instance.second->GetInstantiated()->m_entities);

                                EBUS_EVENT_PTR(m_eventBusPtr, EntityContextEventBus, OnSliceInstantiated, asset.GetId(), instance);
                                EBUS_EVENT_ID(ticket, SliceInstantiationResultBus, OnSliceInstantiated, asset.GetId(), instance);

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
                            EBUS_EVENT_PTR(m_eventBusPtr, EntityContextEventBus, OnSliceInstantiationFailed, asset.GetId());
                            EBUS_EVENT_ID(ticket, SliceInstantiationResultBus, OnSliceInstantiationFailed, asset.GetId());
                        }
                    };

                // Instantiation is queued against the tick bus. This ensures we're not holding the AssetBus lock
                // while the instantiation is handled, which may be costly. This also guarantees callers can
                // jump on the SliceInstantiationResultBus for their ticket before the events are fired.
                EBUS_QUEUE_FUNCTION(AZ::TickBus, instantiateCallback);

                iter = m_queuedSliceInstantiations.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
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

            AZ::SliceComponent::EntityList entities;
            m_rootAsset.Get()->GetComponent()->GetEntities(entities);

            HandleEntitiesAdded(entities);
        }
    }
} // namespace AzFramework
