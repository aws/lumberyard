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
#include "StarterGameEntityUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <MathConversion.h>
#include <Scripting/TagComponent.h>


namespace StarterGameGem
{
    // We need this function to get the velocity of the player.
    // The player needs a 'CharacterPhysics' component and a 'Ragdoll' component which both,
    // unfortunately, inherit from the 'PhysicsRequestBus' and therefore they both override
    // 'GetVelocity()'. The 'Ragdoll' is disabled until the player dies which means it defaults to
    // returning a zero vector and, as we can't control the order, it may be queried second which
    // overwrites the 'CharacterPhysics's velocity values.
    // This function finds all the physics objects pertaining to an entity and returns the largest
    // velocity (as that's likely the most significant).
    bool StarterGameEntityUtility::GetEntitysVelocity(const AZ::EntityId& entityId, AZ::Vector3& velocity)
    {
        IPhysicalEntityIt* iter = gEnv->pPhysicalWorld->GetEntitiesIterator();
        IPhysicalEntity* pent;
        PhysicsVars* pVars = gEnv->pPhysicalWorld->GetPhysVars();

        bool found = false;
        for (iter->MoveFirst(); pent = iter->Next(); )
        {
            AZ::EntityId entId = static_cast<AZ::EntityId>(pent->GetForeignData(PHYS_FOREIGN_ID_COMPONENT_ENTITY));
            if (entId == entityId)
            {
                // Found it!
                pe_status_living living;
                pent->GetStatus(&living);

                // Use the newly found velocity if it's larger.
                AZ::Vector3 vel = LYVec3ToAZVec3(living.vel);
                if (vel.GetLengthSq() > velocity.GetLengthSq())
                    velocity = vel;

                found = true;
            }
        }

        return found;
    }

    AZ::EntityId StarterGameEntityUtility::GetParentEntity(const AZ::EntityId& entityId)
    {
        AZ::EntityId parentId;
        EBUS_EVENT_ID_RESULT(parentId, entityId, AZ::TransformBus, GetParentId);
        return parentId;
    }

    AZStd::string StarterGameEntityUtility::GetEntityName(const AZ::EntityId& entityId)
    {
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, entityId);
        return entityName;
    }

    bool StarterGameEntityUtility::EntityHasTag(const AZ::EntityId& entityId, const AZStd::string& tag)
    {
        bool hasTag = false;
        LmbrCentral::TagComponentRequestBus::EventResult(hasTag, entityId, &LmbrCentral::TagComponentRequestBus::Events::HasTag, AZ::Crc32(tag.c_str()));
        return hasTag;
    }

    bool StarterGameEntityUtility::EntityHasComponent(const AZ::EntityId& entityId, const AZStd::string& componentName)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!entity)
        {
            return false;
        }

        const AZ::Entity::ComponentArrayType& components = entity->GetComponents();

        for (size_t i = 0; i < components.size(); ++i)
        {
            AZ::Component* comp = azdynamic_cast<AZ::Component*>(components[i]);
            if (comp != nullptr)
            {
                //AZ_Printf("StarterGame", "[%s] Component: %s", __FUNCTION__, comp->RTTI_GetTypeName());
                if (componentName == comp->RTTI_GetTypeName())
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool StarterGameEntityUtility::AddTagComponentToEntity(const AZ::EntityId& entityId)
    {
        bool res = true; // succeed if component as already there
        if (!StarterGameEntityUtility::EntityHasComponent(entityId, "TagComponent"))
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            res = false; // component is not already there so we need to add it, only succeed if the add succeeds below

            if (entity)
            {
                // For some reason I have to deactivate the entity to add a component to it.
                entity->Deactivate();
                AZ::Component* component;
                AZ::ComponentDescriptorBus::EventResult(component, LmbrCentral::TagComponent::TYPEINFO_Uuid(), &AZ::ComponentDescriptorBus::Events::CreateComponent);
                res = entity->AddComponent(component);
                entity->Activate();
            }
        }

        return res;
    }

	void StarterGameEntityUtility::SetCharacterHalfHeight(const AZ::EntityId& entityId, float halfHeight)
	{
		pe_player_dimensions playerDims;
		LmbrCentral::CryPhysicsComponentRequestBus::Event(entityId, &LmbrCentral::CryPhysicsComponentRequestBus::Events::GetPhysicsParameters, playerDims);
		auto groundOffset = playerDims.heightCollider - playerDims.sizeCollider.z;
		playerDims.heightCollider = halfHeight + groundOffset;
		playerDims.sizeCollider.Set(playerDims.sizeCollider.x, playerDims.sizeCollider.y, halfHeight);
		LmbrCentral::CryPhysicsComponentRequestBus::Event(entityId, &LmbrCentral::CryPhysicsComponentRequestBus::Events::SetPhysicsParameters, playerDims);
	}

    void StarterGameEntityUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameEntityUtility>("StarterGameEntityUtility")
                ->Method("GetEntitysVelocity", &GetEntitysVelocity)
                ->Method("GetParentEntity", &GetParentEntity)
                ->Method("GetEntityName", &GetEntityName)
                ->Method("EntityHasTag", &EntityHasTag)
                ->Method("EntityHasComponent", &EntityHasComponent)
                ->Method("AddTagComponentToEntity", &AddTagComponentToEntity)
                ->Method("SetCharacterHalfHeight", &SetCharacterHalfHeight)
            ;
        }

    }

}
