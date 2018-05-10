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
#include "ParticleManagerComponent.h"

#include "StarterGameUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <Physics/RigidPhysicsComponent.h>
#include <Physics/CharacterPhysicsComponent.h>

#include <GameplayEventBus.h>

namespace StarterGameGem
{
	void ParticleManagerComponent::Init()
	{
	}

	void ParticleManagerComponent::Activate()
	{
		ParticleManagerComponentRequestsBus::Handler::BusConnect(GetEntityId());
	}

	void ParticleManagerComponent::Deactivate()
	{
		ParticleManagerComponentRequestsBus::Handler::BusDisconnect();
	}

	void ParticleManagerComponent::Reflect(AZ::ReflectContext* reflection)
	{
		AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
		if (serializeContext)
		{
			serializeContext->Class<ParticleManagerComponent, AZ::Component>()
				->Version(1)
				//->Field("Waypoints", &ParticleManagerComponent::m_waypoints)
				//->Field("CurrentWaypoint", &ParticleManagerComponent::m_currentWaypoint)
			;

			AZ::EditContext* editContext = serializeContext->GetEditContext();
			if (editContext)
			{
				editContext->Class<ParticleManagerComponent>("Particle Manager", "Provides an interface to spawn particles")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
					->Attribute(AZ::Edit::Attributes::Category, "Rendering")
					->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
					->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
					->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
					//->DataElement(0, &ParticleManagerComponent::m_waypoints, "Waypoints", "A list of waypoints.")
				;
			}
		}

		if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(reflection))
		{
			// ParticleSpawner return type
			behavior->Class<ParticleSpawnerParams>()
				->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
				->Property("transform", BehaviorValueProperty(&ParticleSpawnerParams::m_transform))
				->Property("event", BehaviorValueProperty(&ParticleSpawnerParams::m_eventName))
				->Property("ownerId", BehaviorValueProperty(&ParticleSpawnerParams::m_owner))
				->Property("targetId", BehaviorValueProperty(&ParticleSpawnerParams::m_target))
                ->Property("attachToEntity", BehaviorValueProperty(&ParticleSpawnerParams::m_attachToTargetEntity))
				->Property("impulse", BehaviorValueProperty(&ParticleSpawnerParams::m_impulse))
				;

			behavior->EBus<ParticleManagerComponentRequestsBus>("ParticleManagerComponentRequestsBus")
                ->Event("SpawnParticle", &ParticleManagerComponentRequestsBus::Events::SpawnParticle)
                ->Event("SpawnDecal", &ParticleManagerComponentRequestsBus::Events::SpawnDecal)
                ->Event("AcceptsDecals", &ParticleManagerComponentRequestsBus::Events::AcceptsDecals)
				;
		}
	}

	void ParticleManagerComponent::SpawnParticle(const ParticleSpawnerParams& params)
	{
		AZ::EntityId id = GetEntityId();
		AZStd::vector<AZ::EntityId> children;
		EBUS_EVENT_ID_RESULT(children, id, AZ::TransformBus, GetAllDescendants);

		AZStd::any paramToBus(params);

		for (int i = 0; i < children.size(); ++i)
		{
			// Broadcast this message to all of the particle manager's children.
			// One of them should be able to match the event name and the one that does spawns
			// their particle.
			AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(children[i], params.m_eventName.c_str(), StarterGameUtility::GetUuid("float"));
			AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
		}
	}

    void ParticleManagerComponent::SpawnDecal(const DecalSelectorComponentRequests::DecalSpawnerParams& params)
    {
        AZ::EntityId id = GetEntityId();
        AZStd::vector<AZ::EntityId> children;
        EBUS_EVENT_ID_RESULT(children, id, AZ::TransformBus, GetAllDescendants);

        AZStd::any paramToBus(params);

        for (int i = 0; i < children.size(); ++i)
        {
            // Broadcast this message to all of the particle manager's children.
            // One of them should be able to match the event name and the one that does spawns
            // their particle.
            AZ::GameplayNotificationId gameplayId = AZ::GameplayNotificationId(children[i], params.m_eventName.c_str(), StarterGameUtility::GetUuid("float"));
            AZ::GameplayNotificationBus::Event(gameplayId, &AZ::GameplayNotificationBus::Events::OnEventBegin, paramToBus);
        }
    }

    bool ParticleManagerComponent::AcceptsDecals(const DecalSelectorComponentRequests::DecalSpawnerParams& params)
    {
        bool accepted = true;
        if (params.m_attachToTargetEntity && params.m_target.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, params.m_target);
            const AZ::Entity::ComponentArrayType& components = entity->GetComponents();
            
            for (size_t i = 0; i < components.size(); ++i)
            {
                if (azdynamic_cast<LmbrCentral::RigidPhysicsComponent*>(components[i]) != nullptr ||
                    azdynamic_cast<LmbrCentral::CharacterPhysicsComponent*>(components[i]) != nullptr)
                {
                    //AZ_Printf("StarterGame", "%s: -- FOUND THE RIGID PHYSICS COMPONENT", __FUNCTION__);
                    accepted = false;
                    break;
                }
            }
        }

        return accepted;
    }

}
