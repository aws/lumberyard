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

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include "SpawnerComponent.h"

namespace LmbrCentral
{
    // BehaviorContext SpawnerComponentNotificationBus forwarder
    class BehaviorSpawnerComponentNotificationBusHandler : public SpawnerComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorSpawnerComponentNotificationBusHandler, "{AC202871-2522-48A6-9B62-5FDAABB302CD}", AZ::SystemAllocator,
            OnSpawnBegin, OnSpawnEnd, OnEntitySpawned, OnSpawnedSliceDestroyed);

        void OnSpawnBegin(const AzFramework::SliceInstantiationTicket& ticket) override
        {
            Call(FN_OnSpawnBegin, ticket);
        }

        void OnSpawnEnd(const AzFramework::SliceInstantiationTicket& ticket) override
        {
            Call(FN_OnSpawnEnd, ticket);
        }

        void OnEntitySpawned(const AzFramework::SliceInstantiationTicket& ticket, const AZ::EntityId& id) override
        {
            Call(FN_OnEntitySpawned, ticket, id);
        }

        void OnSpawnedSliceDestroyed(const AzFramework::SliceInstantiationTicket& ticket) override
        {
            Call(FN_OnSpawnedSliceDestroyed, ticket);
        }
    };

    //=========================================================================
    void SpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Slice", &SpawnerComponent::m_sliceAsset)
                ->Field("SpawnOnActivate", &SpawnerComponent::m_spawnOnActivate)
                ->Field("DestroyOnDeactivate", &SpawnerComponent::m_destroyOnDeactivate)
                ;
            
            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SpawnerComponent>("Spawner", "The Spawner component allows an entity to spawn a design-time or run-time dynamic slice (*.dynamicslice) at the entity's location with an optional offset")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Spawner.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Spawner.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-spawner.html")
                    ->DataElement(0, &SpawnerComponent::m_sliceAsset, "Dynamic slice", "The slice to spawn")
                    ->DataElement(0, &SpawnerComponent::m_spawnOnActivate, "Spawn on activate", "Should the component spawn the selected slice upon activation?")
                    ->DataElement(0, &SpawnerComponent::m_destroyOnDeactivate, "Destroy on deactivate", "Upon deactivation, should the component destroy any slices it spawned?")
                    ;
            }
        }
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<SpawnerComponentRequestBus>("SpawnerComponentRequestBus")
                ->Event("Spawn", &SpawnerComponentRequestBus::Events::Spawn)
                ->Event("SpawnRelative", &SpawnerComponentRequestBus::Events::SpawnRelative)
                ->Event("SpawnAbsolute", &SpawnerComponentRequestBus::Events::SpawnAbsolute)
                ->Event("DestroySpawnedSlice", &SpawnerComponentRequestBus::Events::DestroySpawnedSlice)
                ->Event("DestroyAllSpawnedSlices", &SpawnerComponentRequestBus::Events::DestroyAllSpawnedSlices)
                ->Event("GetCurrentlySpawnedSlices", &SpawnerComponentRequestBus::Events::GetCurrentlySpawnedSlices)
                ->Event("HasAnyCurrentlySpawnedSlices", &SpawnerComponentRequestBus::Events::HasAnyCurrentlySpawnedSlices)
                ->Event("GetCurrentEntitiesFromSpawnedSlice", &SpawnerComponentRequestBus::Events::GetCurrentEntitiesFromSpawnedSlice)
                ->Event("GetAllCurrentlySpawnedEntities", &SpawnerComponentRequestBus::Events::GetAllCurrentlySpawnedEntities)
                ;

            behaviorContext->EBus<SpawnerComponentNotificationBus>("SpawnerComponentNotificationBus")
                ->Handler<BehaviorSpawnerComponentNotificationBusHandler>()
                ;

            behaviorContext->Constant("SpawnerComponentTypeId", BehaviorConstant(SpawnerComponentTypeId));

            behaviorContext->Class<SpawnerConfig>()
                //->Property("sliceAsset", BehaviorValueProperty(&SpawnerConfig::m_sliceAsset))
                ->Property("spawnOnActivate", BehaviorValueProperty(&SpawnerConfig::m_spawnOnActivate))
                ->Property("destroyOnDeactivate", BehaviorValueProperty(&SpawnerConfig::m_destroyOnDeactivate))
                ;
        }
    }

    //=========================================================================
    void SpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SpawnerService", 0xd2f1d7a3));
    }


    //=========================================================================
    void SpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    //=========================================================================
    SpawnerComponent::SpawnerComponent()
    {
        // Slice asset should load purely on-demand.
        m_sliceAsset.SetFlags(static_cast<AZ::u8>(AZ::Data::AssetFlags::OBJECTSTREAM_NO_LOAD));
    }

    //=========================================================================
    void SpawnerComponent::Activate()
    {
        SpawnerComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_spawnOnActivate)
        {
            SpawnSliceInternal(m_sliceAsset, AZ::Transform::Identity());
        }
    }

    //=========================================================================
    void SpawnerComponent::Deactivate()
    {
        SpawnerComponentRequestBus::Handler::BusDisconnect();
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::MultiHandler::BusDisconnect();

        if (m_destroyOnDeactivate)
        {
            DestroyAllSpawnedSlices();
        }

        m_activeTickets.clear();
        m_entityToTicketMap.clear();
        m_ticketToEntitiesMap.clear();
    }

    bool SpawnerComponent::ReadInConfig(const AZ::ComponentConfig* spawnerConfig)
    {
        if (auto config = azrtti_cast<const SpawnerConfig*>(spawnerConfig))
        {
            m_sliceAsset = config->m_sliceAsset;
            m_spawnOnActivate = config->m_spawnOnActivate;
            m_destroyOnDeactivate = config->m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    bool SpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outSpawnerConfig) const
    {
        if (auto config = azrtti_cast<SpawnerConfig*>(outSpawnerConfig))
        {
            config->m_sliceAsset = m_sliceAsset;
            config->m_spawnOnActivate = m_spawnOnActivate;
            config->m_destroyOnDeactivate = m_destroyOnDeactivate;
            return true;
        }
        return false;
    }

    //=========================================================================
    void SpawnerComponent::SetDynamicSlice(const AZ::Data::Asset<AZ::Data::AssetData>& dynamicSliceAsset)
    {
        m_sliceAsset = dynamicSliceAsset;
    }

    //=========================================================================
    void SpawnerComponent::SetSpawnOnActivate(bool spawnOnActivate)
    {
        m_spawnOnActivate = spawnOnActivate;
    }

    //=========================================================================
    bool SpawnerComponent::GetSpawnOnActivate()
    {
        return m_spawnOnActivate;
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::Spawn()
    {
        return SpawnSliceInternal(m_sliceAsset, AZ::Transform::Identity());
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnRelative(const AZ::Transform& relative)
    {
        return SpawnSliceInternal(m_sliceAsset, relative);
    }
    
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnAbsolute(const AZ::Transform& world)
    {
        return SpawnSliceInternalAbsolute(m_sliceAsset, world);
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice)
    {
        return SpawnSliceInternal(slice, AZ::Transform::Identity());
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative)
    {
        return SpawnSliceInternal(slice, relative);
    }
    
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world)
    {
        return SpawnSliceInternalAbsolute(slice, world);
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceInternalAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world)
    {
        AzFramework::SliceInstantiationTicket ticket;
        EBUS_EVENT_RESULT(ticket, AzFramework::GameEntityContextRequestBus, InstantiateDynamicSlice, slice, world, nullptr);

        if (ticket)
        {
            m_activeTickets.emplace_back(ticket);
            m_ticketToEntitiesMap.emplace(ticket); // create entry for ticket, with no entities listed yet

            AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
        }
        return ticket;
    }

    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceInternal(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative)
    {
        AZ::Transform transform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        transform *= relative;

        AzFramework::SliceInstantiationTicket ticket;
        EBUS_EVENT_RESULT(ticket, AzFramework::GameEntityContextRequestBus, InstantiateDynamicSlice, slice, transform, nullptr);

        if (ticket)
        {
            m_activeTickets.emplace_back(ticket);
            m_ticketToEntitiesMap.emplace(ticket); // create entry for ticket, with no entities listed yet

            AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);
        }

        return ticket;
    }

    //=========================================================================
    void SpawnerComponent::DestroySpawnedSlice(const AzFramework::SliceInstantiationTicket& sliceTicket)
    {
        auto foundTicketEntities = m_ticketToEntitiesMap.find(sliceTicket);
        if (foundTicketEntities != m_ticketToEntitiesMap.end())
        {
            AZStd::unordered_set<AZ::EntityId>& entitiesInSlice = foundTicketEntities->second;

            AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(sliceTicket); // don't care anymore about events from this ticket

            if (entitiesInSlice.empty())
            {
                AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::CancelDynamicSliceInstantiation, sliceTicket);
            }
            else
            {
                for (AZ::EntityId entity : entitiesInSlice)
                {
                    AZ::EntityBus::MultiHandler::BusDisconnect(entity); // don't care anymore about events from this entity
                    m_entityToTicketMap.erase(entity);
                }

                AzFramework::GameEntityContextRequestBus::Broadcast(&AzFramework::GameEntityContextRequestBus::Events::DestroyDynamicSliceByEntity, *entitiesInSlice.begin());
            }

            m_ticketToEntitiesMap.erase(foundTicketEntities);
            m_activeTickets.erase(AZStd::remove(m_activeTickets.begin(), m_activeTickets.end(), sliceTicket), m_activeTickets.end());

            // slice destruction is queued, so queue the notification as well.
            AZ::EntityId entityId = GetEntityId();
            AZ::TickBus::QueueFunction(
                [entityId, sliceTicket]() // use copies, in case 'this' is destroyed
                {
                    SpawnerComponentNotificationBus::Event(entityId, &SpawnerComponentNotificationBus::Events::OnSpawnedSliceDestroyed, sliceTicket);
                });
        }
    }

    //=========================================================================
    void SpawnerComponent::DestroyAllSpawnedSlices()
    {
        AZStd::vector<AzFramework::SliceInstantiationTicket> activeTickets = m_activeTickets; // iterate over a copy of the vector
        for (AzFramework::SliceInstantiationTicket& ticket : activeTickets)
        {
            DestroySpawnedSlice(ticket);
        }

        AZ_Assert(m_activeTickets.empty(), "SpawnerComponent::DestroyAllSpawnedSlices - tickets still listed");
        AZ_Assert(m_entityToTicketMap.empty(), "SpawnerComponent::DestroyAllSpawnedSlices - entities still listed");
        AZ_Assert(m_ticketToEntitiesMap.empty(), "SpawnerComponent::DestroyAllSpawnedSlices - ticket entities still listed");
    }

    //=========================================================================
    AZStd::vector<AzFramework::SliceInstantiationTicket> SpawnerComponent::GetCurrentlySpawnedSlices()
    {
        return m_activeTickets;
    }

    //=========================================================================
    bool SpawnerComponent::HasAnyCurrentlySpawnedSlices()
    {
        return !m_activeTickets.empty();
    }

    //=========================================================================
    AZStd::vector<AZ::EntityId> SpawnerComponent::GetCurrentEntitiesFromSpawnedSlice(const AzFramework::SliceInstantiationTicket& ticket)
    {
        AZStd::vector<AZ::EntityId> entities;
        auto foundTicketEntities = m_ticketToEntitiesMap.find(ticket);
        if (foundTicketEntities != m_ticketToEntitiesMap.end())
        {
            const AZStd::unordered_set<AZ::EntityId>& ticketEntities = foundTicketEntities->second;

            AZ_Warning("SpawnerComponent", !ticketEntities.empty(), "SpawnerComponent::GetCurrentEntitiesFromSpawnedSlice - Spawn has not completed, its entities are not available.");

            entities.reserve(ticketEntities.size());
            entities.insert(entities.end(), ticketEntities.begin(), ticketEntities.end());

            // Sort entities so that results are stable.
            AZStd::sort(entities.begin(), entities.end());
        }
        return entities;
    }

    //=========================================================================
    AZStd::vector<AZ::EntityId> SpawnerComponent::GetAllCurrentlySpawnedEntities()
    {
        AZStd::vector<AZ::EntityId> entities;
        entities.reserve(m_entityToTicketMap.size());

        // Return entities in the order their tickets spawned.
        // It's not a requirement, but it seems nice to do.
        for (const AzFramework::SliceInstantiationTicket& ticket : m_activeTickets)
        {
            const AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];
            entities.insert(entities.end(), ticketEntities.begin(), ticketEntities.end());

            // Sort entities from a given ticket, so that results are stable.
            AZStd::sort(entities.end() - ticketEntities.size(), entities.end());
        }

        return entities;
    }

    //=========================================================================
    void SpawnerComponent::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AzFramework::SliceInstantiationTicket& ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnSpawnBegin, ticket);
    }

    //=========================================================================
    void SpawnerComponent::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AzFramework::SliceInstantiationTicket& ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        // Stop listening for this ticket (since it's done). We can have have multiple tickets in flight.
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        const AZ::SliceComponent::EntityList& entities = sliceAddress.second->GetInstantiated()->m_entities;

        AZStd::vector<AZ::EntityId> entityIds;
        entityIds.reserve(entities.size());

        AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];

        for (AZ::Entity* currEntity : entities)
        {
            AZ::EntityId currEntityId = currEntity->GetId();

            entityIds.emplace_back(currEntityId);

            // update internal slice tracking data
            ticketEntities.emplace(currEntityId);
            m_entityToTicketMap.emplace(AZStd::make_pair(currEntityId, ticket));
            AZ::EntityBus::MultiHandler::BusConnect(currEntityId);

            EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnEntitySpawned, ticket, currEntityId);
        }

        EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnSpawnEnd, ticket);

        EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnEntitiesSpawned, ticket, entityIds);

        // If slice had no entities, clean it up
        if (entities.empty())
        {
            DestroySpawnedSlice(ticket);
        }
    }

    //=========================================================================
    void SpawnerComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
    {
        const AzFramework::SliceInstantiationTicket& ticket = *AzFramework::SliceInstantiationResultBus::GetCurrentBusId();

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        // clean it up
        DestroySpawnedSlice(ticket);

        // error msg
        if (sliceAssetId == m_sliceAsset.GetId())
        {
            AZ_Error("SpawnerComponent", false, "Slice [id:'%s' hint:'%s'] failed to instantiate", sliceAssetId.ToString<AZStd::string>().c_str(), m_sliceAsset.GetHint().c_str());
        }
        else
        {
            AZ_Error("SpawnerComponent", false, "Slice [id:'%s'] failed to instantiate", sliceAssetId.ToString<AZStd::string>().c_str());
        }
    }

    //=========================================================================
    void SpawnerComponent::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

        auto entityToTicketIter = m_entityToTicketMap.find(entityId);
        if (entityToTicketIter != m_entityToTicketMap.end())
        {
            AzFramework::SliceInstantiationTicket ticket = entityToTicketIter->second;
            m_entityToTicketMap.erase(entityToTicketIter);

            AZStd::unordered_set<AZ::EntityId>& ticketEntities = m_ticketToEntitiesMap[ticket];
            ticketEntities.erase(entityId);

            // If this was last entity in the spawn, clean it up
            if (ticketEntities.empty())
            {
                DestroySpawnedSlice(ticket);
            }
        }
    }
} // namespace LmbrCentral
