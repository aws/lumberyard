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

#include "UiSpawnerComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <LyShine/Bus/UiGameEntityContextBus.h>

// BehaviorContext UiSpawnerNotificationBus forwarder
class BehaviorUiSpawnerNotificationBusHandler : public UiSpawnerNotificationBus::Handler, public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiSpawnerNotificationBusHandler, "{95213AF9-F8F4-4D86-8C68-625F5AFE78FA}", AZ::SystemAllocator,
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

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiSpawnerComponent, AZ::Component>()
            ->Version(1)
            ->Field("Slice", &UiSpawnerComponent::m_sliceAsset)
            ->Field("SpawnOnActivate", &UiSpawnerComponent::m_spawnOnActivate);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            auto editInfo = editContext->Class<UiSpawnerComponent>("UiSpawner",
                "The spawner component provides dynamic UI slice spawning support.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Spawner.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Spawner.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiSpawnerComponent::m_sliceAsset, "Dynamic slice", "The slice to spawn");
            editInfo->DataElement(0, &UiSpawnerComponent::m_spawnOnActivate, "Spawn on activate",
                "Should the component spawn the selected slice upon activation?");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiSpawnerBus>("UiSpawnerBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
            ->Event("Spawn", &UiSpawnerBus::Events::Spawn)
            ->Event("SpawnRelative", &UiSpawnerBus::Events::SpawnRelative)
            ->Event("SpawnAbsolute", &UiSpawnerBus::Events::SpawnViewport)
            ;

        behaviorContext->EBus<UiSpawnerNotificationBus>("UiSpawnerNotificationBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
            ->Handler<BehaviorUiSpawnerNotificationBusHandler>()
            ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("SpawnerService", 0xd2f1d7a3));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
{
    dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSpawnerComponent::UiSpawnerComponent()
{
    // Slice asset should load purely on-demand.
    m_sliceAsset.SetFlags(static_cast<AZ::u8>(AZ::Data::AssetFlags::OBJECTSTREAM_NO_LOAD));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Activate()
{
    UiSpawnerBus::Handler::BusConnect(GetEntityId());

    if (m_spawnOnActivate)
    {
        SpawnSliceInternal(m_sliceAsset, AZ::Vector2(0.0f, 0.0f), false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::Deactivate()
{
    UiSpawnerBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::Spawn()
{
    return SpawnSliceInternal(m_sliceAsset, AZ::Vector2(0.0f, 0.0f), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnRelative(const AZ::Vector2& relative)
{
    return SpawnSliceInternal(m_sliceAsset, relative, false);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnViewport(const AZ::Vector2& pos)
{
    return SpawnSliceInternal(m_sliceAsset, pos, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice)
{
    return SpawnSliceInternal(slice, AZ::Vector2(0.0f, 0.0f), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& relative)
{
    return SpawnSliceInternal(slice, relative, false);
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSliceViewport(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& pos)
{
    return SpawnSliceInternal(slice, pos, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzFramework::SliceInstantiationTicket UiSpawnerComponent::SpawnSliceInternal(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& position, bool isViewportPosition)
{
    AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
    EBUS_EVENT_ID_RESULT(contextId, GetEntityId(), AzFramework::EntityIdContextQueryBus, GetOwningContextId);

    AzFramework::SliceInstantiationTicket ticket;
    EBUS_EVENT_ID_RESULT(ticket, contextId, UiGameEntityContextBus, InstantiateDynamicSlice, slice, position, isViewportPosition, GetEntity(), nullptr);

    AzFramework::SliceInstantiationResultBus::MultiHandler::BusConnect(ticket);

    return ticket;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/)
{
    const AzFramework::SliceInstantiationTicket& ticket = (*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());
    EBUS_EVENT_ID(GetEntityId(), UiSpawnerNotificationBus, OnSpawnBegin, ticket);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress)
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
        EBUS_EVENT_ID(GetEntityId(), UiSpawnerNotificationBus, OnEntitySpawned, ticket, currEntity->GetId());
    }

    EBUS_EVENT_ID(GetEntityId(), UiSpawnerNotificationBus, OnSpawnEnd, ticket);

    EBUS_EVENT_ID(GetEntityId(), UiSpawnerNotificationBus, OnEntitiesSpawned, ticket, entityIds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSpawnerComponent::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId)
{
    AzFramework::SliceInstantiationResultBus::MultiHandler::BusDisconnect(*AzFramework::SliceInstantiationResultBus::GetCurrentBusId());

    AZStd::string assetPath;
    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, sliceAssetId);

    if (assetPath.empty())
    {
        assetPath = sliceAssetId.ToString<AZStd::string>();
    }

    AZ_Warning("UiSpawnerComponent", false, "Slice '%s' failed to instantiate. Check that it contains UI elements.", assetPath.c_str());
}
