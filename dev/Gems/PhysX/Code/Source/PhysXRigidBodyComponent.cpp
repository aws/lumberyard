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

#include <StdAfx.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <PhysXRigidBodyComponent.h>
#include <PhysXMathConversion.h>
#include <Include/PhysX/PhysXBus.h>
#include <Include/PhysX/PhysXColliderComponentBus.h>

namespace PhysX
{
    void PhysXRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        PhysXRigidBody::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysXRigidBodyComponent, AZ::Component>()
                ->Version(1)
                ->Field("PhysXRigidBody", &PhysXRigidBodyComponent::m_rigidBody)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
        }
    }

    PhysXRigidBodyComponent::PhysXRigidBodyComponent(const PhysXRigidBodyConfiguration& config)
    {
        m_rigidBody.SetConfig(config);
    }

    void PhysXRigidBodyComponent::Init()
    {
    }

    void PhysXRigidBodyComponent::Activate()
    {
        // Get necessary information from transform and shape buses and create PhysXRigidBody
        AZ::Transform lyTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(lyTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        m_rigidBody.SetScale(lyTransform.ExtractScaleExact());

        Physics::Ptr<Physics::ShapeConfiguration> shapeConfig = nullptr;
        EBUS_EVENT_ID_RESULT(shapeConfig, GetEntityId(), PhysXColliderComponentRequestBus, GetShapeConfigFromEntity);

        m_rigidBody.SetShape(shapeConfig);
        m_rigidBody.SetTransform(lyTransform);
        m_rigidBody.SetEntity(GetEntity());
        auto pxRigidActor = m_rigidBody.CreatePhysXActor();

        // Add actor to scene
        EBUS_EVENT(PhysX::PhysXSystemRequestBus, AddActor, *pxRigidActor);

        // Listen to the PhysX system for events concerning this entity.
        EntityPhysXEventBus::Handler::BusConnect(GetEntityId());
    }

    void PhysXRigidBodyComponent::Deactivate()
    {
        EntityPhysXEventBus::Handler::BusDisconnect();

        // Stop listening for events from self and descendants
        AZ::EntityBus::MultiHandler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();

        m_rigidBody.ReleasePhysXActor();
    }

    void PhysXRigidBodyComponent::AddCollidersFromEntityAndDescendants(const AZ::EntityId& rootEntityId)
    {
    }

    void PhysXRigidBodyComponent::AddCollidersFromEntity(const AZ::EntityId& entityId)
    {
    }

    void PhysXRigidBodyComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
    }

    void PhysXRigidBodyComponent::OnChildAdded(AZ::EntityId childId)
    {
    }

    void PhysXRigidBodyComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ_Assert(entityId != GetEntityId(), "Shouldn't be connected to our own EntityBus");

        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);
    }

    void PhysXRigidBodyComponent::OnPostStep()
    {
        // Inform the TransformComponent that we've been moved by the PhysX system
        m_rigidBody.GetTransformUpdateFromPhysX();
        AZ::Transform transform = m_rigidBody.GetTransform();

        // Maintain scale (this must be precise).
        AZ::Transform entityTransform = AZ::Transform::Identity();
        EBUS_EVENT_ID_RESULT(entityTransform, GetEntityId(), AZ::TransformBus, GetWorldTM);
        transform.MultiplyByScale(entityTransform.ExtractScaleExact());

        EBUS_EVENT_ID(GetEntityId(), AZ::TransformBus, SetWorldTM, transform);
    }
} // namespace PhysX
