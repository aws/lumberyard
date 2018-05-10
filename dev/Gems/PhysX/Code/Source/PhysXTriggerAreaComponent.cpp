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

#include <PhysX_precompiled.h>
#include <PhysXTriggerAreaComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Scripting/TriggerAreaComponentBus.h>
#include "LmbrCentral/Physics/PhysicsComponentBus.h"
#include <PhysXRigidBody.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    void PhysXTriggerAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysXTriggerAreaComponent, AZ::Component>()
                ->Version(1)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PhysXTriggerAreaComponent>(
                    "PhysX Trigger Area", "Trigger Area component telling PhysX that this rigid body is a trigger")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/TriggerArea.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Trigger.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
            }
        }
    }

    void PhysXTriggerAreaComponent::Activate()
    {
        PhysXTriggerAreaEventBus::Handler::BusConnect(GetEntityId());
        
        Physics::RigidBody* rigidBody = nullptr;
        AzFramework::PhysicsComponentRequestBus::EventResult(rigidBody, GetEntityId(), &AzFramework::PhysicsComponentRequestBus::Events::GetRigidBody);

        if (rigidBody)
        {
            physx::PxRigidActor* pxRigidActor = reinterpret_cast<physx::PxRigidActor*>(rigidBody->GetNativePointer());

            physx::PxU32 nbShapes = pxRigidActor->getNbShapes();
            AZStd::vector<physx::PxShape*> shapes(nbShapes);

            pxRigidActor->getShapes(shapes.data(), nbShapes);
            
            AZ_Warning("PhysX Trigger Area", nbShapes > 0, "No shapes assigned to entity %s. Trigger will not be able to activate.", m_entity->GetName().c_str());

            // Make the shape attached to a rigid body a trigger
            AZStd::for_each(shapes.begin(), shapes.end(), [pxRigidActor](physx::PxShape* shape)
            {
                pxRigidActor->detachShape(*shape);
                shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
                shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
                pxRigidActor->attachShape(*shape);
            });
        }
        else
        {
            AZ_Warning("PhysX Trigger Area", false, "No rigid body assigned to entity %s. Trigger will not be able to activate.", m_entity->GetName().c_str());
        }
    }

    void PhysXTriggerAreaComponent::Deactivate()
    {
        PhysXTriggerAreaEventBus::Handler::BusDisconnect();
    }

    void PhysXTriggerAreaComponent::OnTriggerEnter(AZ::EntityId entityEntering)
    {
        // Notify the trigger entity itself about another entity entering it
        LmbrCentral::TriggerAreaNotificationBus::Event( GetEntityId(), &LmbrCentral::TriggerAreaNotifications::OnTriggerAreaEntered, entityEntering);

        // Notify the entity that has entered the trigger
        LmbrCentral::TriggerAreaEntityNotificationBus::Event(entityEntering, &LmbrCentral::TriggerAreaEntityNotifications::OnEntityEnteredTriggerArea, GetEntityId());
    }

    void PhysXTriggerAreaComponent::OnTriggerExit(AZ::EntityId entityExiting)
    {
        LmbrCentral::TriggerAreaNotificationBus::Event(GetEntityId(), &LmbrCentral::TriggerAreaNotifications::OnTriggerAreaExited, entityExiting);
        LmbrCentral::TriggerAreaEntityNotificationBus::Event(entityExiting, &LmbrCentral::TriggerAreaEntityNotifications::OnEntityExitedTriggerArea, GetEntityId());
    }

} // namespace PhysX
