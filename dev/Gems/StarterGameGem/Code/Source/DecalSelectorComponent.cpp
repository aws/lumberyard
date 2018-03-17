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
#include "StarterGameGem_precompiled.h"
#include "DecalSelectorComponent.h"

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <GameplayEventBus.h>

namespace StarterGameGem
{

    DecalPool::DecalPool()
        : m_index(0)
    {}

    void DecalPool::AddDecal(AZ::EntityId& decal)
    {
        // Make sure this decal doesn't already exist in this pool.
        for (int i = 0; i < m_pool.size(); ++i)
            if (m_pool[i] == decal)
                return;

        m_pool.push_back(decal);
    }

    AZ::EntityId DecalPool::GetAvailableDecal()
    {
        AZ::EntityId decal = m_pool[m_index];

        // Increment the index and reset it if it's trying to go too far.
        ++m_index;
        if (m_index >= m_pool.size())
        {
            m_index = 0;
        }

        return decal;
    }

    //--------------------------------------------------------------

    void DecalSelectorComponent::Init()
    {
    }

    void DecalSelectorComponent::Activate()
    {
        DecalSelectorComponentRequestsBus::Handler::BusConnect(GetEntityId());

        // Initialise the vector based on the maximum number of materials.
        // MAX_SURFACE_ID says there are only 255... but we can't access that constant here.
        m_decalPools.resize(255 + 1);
    }

    void DecalSelectorComponent::Deactivate()
    {
        DecalSelectorComponentRequestsBus::Handler::BusDisconnect();
    }

    void DecalSelectorComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<DecalSelectorComponent>()
                ->Version(1)
                ->Field("DecalPools", &DecalSelectorComponent::m_decalPools)
                ->Field("FallBackToDefault", &DecalSelectorComponent::m_useDefaultMat)
                //->Field("CurrentWaypoint", &DecalSelectorComponent::m_currentWaypoint)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DecalSelectorComponent>("Decal Selector", "Selects the decal to spawn based on the surface type")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(0, &DecalSelectorComponent::m_useDefaultMat, "FallBackToDefault", "If a decal hasn't been specified for a surface, use the default one.")
                    //->DataElement(0, &DecalSelectorComponent::m_eventName, "EventName", "The event to listen for.")
                    ;
            }
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behavior->Class<DecalSpawnerParams>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("event", BehaviorValueProperty(&DecalSpawnerParams::m_eventName))
                ->Property("transform", BehaviorValueProperty(&DecalSpawnerParams::m_transform))
                ->Property("targetId", BehaviorValueProperty(&DecalSpawnerParams::m_target))
                ->Property("attachToEntity", BehaviorValueProperty(&DecalSpawnerParams::m_attachToTargetEntity))
                ->Property("surfaceType", BehaviorValueProperty(&DecalSpawnerParams::m_surfaceType))
                ;

            behavior->EBus<DecalSelectorComponentRequestsBus>("DecalSelectorComponentRequestsBus")
                ->Event("SpawnDecal", &DecalSelectorComponentRequestsBus::Events::SpawnDecal)
                ->Event("AddDecalToPool", &DecalSelectorComponentRequestsBus::Events::AddDecalToPool)
                ;
        }
    }

    void DecalSelectorComponent::SpawnDecal(const DecalSpawnerParams& params)
    {
        AZ::EntityId entityId;
        short surfaceType = params.m_surfaceType;
        if (surfaceType < 0 || surfaceType > 255)
        {
            if (m_useDefaultMat)
            {
                surfaceType = 0;	// default decal
            }
        }

        if (m_decalPools[surfaceType].m_pool.size() == 0)
        {
            if (m_useDefaultMat)
            {
                if (m_decalPools[0].m_pool.size() == 0)
                {
                    // That surface shouldn't spawn a decal and there's no default decal set to be used.
                    return;
                }
                else
                {
                    // Use a default decal.
                    entityId = m_decalPools[0].GetAvailableDecal();
                }
            }
            else
            {
                // The surface doesn't have any decals for it and it's not been told to use the default.
                return;
            }
        }
        else
        {
            entityId = m_decalPools[surfaceType].GetAvailableDecal();
        }

        if (!entityId.IsValid())
        {
            // The entity given to us by the pool was invalid.
            AZ_Warning("StarterGame", false, "DecalSelectorComponent: We received an invalid entity ID from a decal pool.");
            return;
        }

        // Reset the decal in case it's a repurposed one.
        ResetDecalEntity(entityId);

        // Move to its new position, ensuring the original scale is maintained.
        AZ::Transform tm = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(tm, entityId, &AZ::TransformInterface::GetWorldTM);
        AZ::Transform scale = AZ::Transform::CreateScale(AZ::Vector3(tm.GetColumn(0).GetLength(), tm.GetColumn(1).GetLength(), tm.GetColumn(2).GetLength()));
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldTM, params.m_transform * scale);

        // Parent it, if required.
        if (params.m_attachToTargetEntity && params.m_target.IsValid())
        {
            AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetParent, params.m_target);
        }
    }

    void DecalSelectorComponent::AddDecalToPool(AZ::EntityId decal, int surfaceIndex)
    {
        if (surfaceIndex < 0 || surfaceIndex > 255)
        {
            AZ_Warning("StarterGame", false, "DecalSelectorComponent: Something attempted to add a decal to a pool that shouldn't exist.");
            return;
        }

        m_decalPools[surfaceIndex].AddDecal(decal);
    }

    void DecalSelectorComponent::ResetDecalEntity(AZ::EntityId& decal)
    {
        //EBUS_EVENT_ID(m_entityId, AZ::TransformBus, SetParent, AZ::EntityId());
        AZ::TransformBus::Event(decal, &AZ::TransformInterface::SetParent, AZ::EntityId());

        // Shouldn't be necessary as every decal should get its transform set.
        //AZ::TransformBus::Event(decal, &AZ::TransformInterface::SetWorldTM, AZ::Transform::CreateIdentity());
    }


}
