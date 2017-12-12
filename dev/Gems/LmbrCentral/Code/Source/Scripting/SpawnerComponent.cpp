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

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>

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
            OnSpawnBegin, OnSpawnEnd, OnEntitySpawned);

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
    };

    //=========================================================================
    // SpawnerComponent::Reflect
    //=========================================================================
    void SpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SpawnerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Slice", &SpawnerComponent::m_sliceAsset)
                ->Field("SpawnOnActivate", &SpawnerComponent::m_spawnOnActivate);

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
                    ->DataElement(0, &SpawnerComponent::m_spawnOnActivate, "Spawn on activate", "Should the component spawn the selected slice upon activation?");
            }
        }
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<SpawnerComponentRequestBus>("SpawnerComponentRequestBus")
                ->Event("Spawn", &SpawnerComponentRequestBus::Events::Spawn)
                ->Event("SpawnRelative", &SpawnerComponentRequestBus::Events::SpawnRelative)
                ->Event("SpawnAbsolute", &SpawnerComponentRequestBus::Events::SpawnAbsolute)
                ;

            behaviorContext->EBus<SpawnerComponentNotificationBus>("SpawnerComponentNotificationBus")
                ->Handler<BehaviorSpawnerComponentNotificationBusHandler>()
                ;
        }
    }

    //=========================================================================
    // SpawnerComponent::GetProvidedServices
    //=========================================================================
    void SpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SpawnerService", 0xd2f1d7a3));
    }


    //=========================================================================
    // SpawnerComponent::GetDependentServices
    //=========================================================================
    void SpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    //=========================================================================
    // SpawnerComponent::SpawnerComponent
    //=========================================================================
    SpawnerComponent::SpawnerComponent()
    {
        // Slice asset should load purely on-demand.
        m_sliceAsset.SetFlags(static_cast<AZ::u8>(AZ::Data::AssetFlags::OBJECTSTREAM_NO_LOAD));
    }

    //=========================================================================
    // SpawnerComponent::Activate
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
    // SpawnerComponent::Deactivate
    //=========================================================================
    void SpawnerComponent::Deactivate()
    {
        SpawnerComponentRequestBus::Handler::BusDisconnect();
    }

    //=========================================================================
    // SpawnerComponent::Spawn
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::Spawn()
    {
        return SpawnSliceInternal(m_sliceAsset, AZ::Transform::Identity());
    }

    //=========================================================================
    // SpawnerComponent::Spawn
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnRelative(const AZ::Transform& relative)
    {
        return SpawnSliceInternal(m_sliceAsset, relative);
    }
    
    //=========================================================================
    // SpawnerComponent::SpawnAbsolute
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnAbsolute(const AZ::Transform& world)
    {
        AZ::Transform entityWorldTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(entityWorldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        const AZ::Transform relative = entityWorldTransform.GetInverseFast() * world;
        return SpawnSliceInternal(m_sliceAsset, relative);
    }

    //=========================================================================
    // SpawnerComponent::SpawnSlice
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice)
    {
        return SpawnSliceInternal(slice, AZ::Transform::Identity());
    }

    //=========================================================================
    // SpawnerComponent::SpawnSliceRelative
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative)
    {
        return SpawnSliceInternal(slice, relative);
    }
    
    //=========================================================================
    // SpawnerComponent::SpawnSliceAbsolute
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceAbsolute(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& world)
    {
        AZ::Transform entityWorldTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(entityWorldTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        const AZ::Transform relative = entityWorldTransform.GetInverseFast() * world;
        return SpawnSliceInternal(slice, relative);
    }

    //=========================================================================
    // SpawnerComponent::SpawnSliceInternal
    //=========================================================================
    AzFramework::SliceInstantiationTicket SpawnerComponent::SpawnSliceInternal(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Transform& relative)
    {
        AZ::Transform transform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(transform, GetEntityId(), AZ::TransformBus, GetWorldTM);

        transform *= relative;

        AzFramework::SliceInstantiationTicket ticket;
        EBUS_EVENT_RESULT(ticket, AzFramework::GameEntityContextRequestBus, InstantiateDynamicSlice, slice, transform, nullptr);

        AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);

        return ticket;
    }

    //=========================================================================
    // SpawnerComponent::OnSlicePreInstantiate
    //=========================================================================
    void SpawnerComponent::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/)
    {
        const AzFramework::SliceInstantiationTicket& ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());
        EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnSpawnBegin, ticket);
    }

    //=========================================================================
    // SpawnerComponent::OnSliceInstantiated
    //=========================================================================
    void SpawnerComponent::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
    {
        const AzFramework::SliceInstantiationTicket& ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        // Stop listening for this ticket (since it's done). We can have have multiple tickets in flight.
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(ticket);

        const AZ::SliceComponent::EntityList& entities = sliceAddress.second->GetInstantiated()->m_entities;

        AZStd::vector<AZ::EntityId> entityIds;
        entityIds.reserve(entities.size());
        for (AZ::Entity* currEntity : entities)
        {
            entityIds.push_back(currEntity->GetId());
            EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnEntitySpawned, ticket, currEntity->GetId());
        }

        EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnSpawnEnd, ticket);

        EBUS_EVENT_ID(GetEntityId(), SpawnerComponentNotificationBus, OnEntitiesSpawned, ticket, entityIds);
    }

    //=========================================================================
    // SpawnerComponent::OnSliceInstantiationFailed
    //=========================================================================
    void SpawnerComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
    {
        AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

        AZ_Error("SpawnerComponent", false, "Slice '%s' failed to instantiate", sliceAssetId.ToString<AZStd::string>().c_str());
    }
} // namespace LmbrCentral
