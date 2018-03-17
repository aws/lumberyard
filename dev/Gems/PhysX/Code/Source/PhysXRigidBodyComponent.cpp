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
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <PhysXRigidBodyComponent.h>
#include <PhysXMathConversion.h>
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
        AZ::TransformBus::EventResult(lyTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        m_rigidBody.SetScale(lyTransform.ExtractScaleExact());

        Physics::Ptr<Physics::ShapeConfiguration> shapeConfig = nullptr;
        PhysXColliderComponentRequestBus::EventResult(shapeConfig, GetEntityId(),
            &PhysXColliderComponentRequests::GetShapeConfigFromEntity);

        m_rigidBody.SetShape(shapeConfig);
        m_rigidBody.SetTransform(lyTransform);
        m_rigidBody.SetEntity(GetEntity());
        auto pxRigidActor = m_rigidBody.CreatePhysXActor();

        // Add actor to scene
        PhysXSystemRequestBus::Broadcast(&PhysXSystemRequests::AddActor, *pxRigidActor);

        // Listen to the PhysX system for events concerning this entity.
        EntityPhysXEventBus::Handler::BusConnect(GetEntityId());

        AzFramework::PhysicsComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PhysXRigidBodyComponent::Deactivate()
    {
        AzFramework::PhysicsComponentRequestBus::Handler::BusDisconnect();
        EntityPhysXEventBus::Handler::BusDisconnect();
        m_rigidBody.ReleasePhysXActor();
    }

    void PhysXRigidBodyComponent::OnPostStep()
    {
        // Inform the TransformComponent that we've been moved by the PhysX system
        m_rigidBody.GetTransformUpdateFromPhysX();
        AZ::Transform transform = m_rigidBody.GetTransform();

        // Maintain scale (this must be precise).
        AZ::Transform entityTransform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(entityTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        transform.MultiplyByScale(entityTransform.ExtractScaleExact());

        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformInterface::SetWorldTM, transform);
    }

    void PhysXRigidBodyComponent::EnablePhysics()
    {
        AZ_Warning("PhysXRigidBodyComponent", false, "EnablePhysics() not implemented.");
    }

    void PhysXRigidBodyComponent::DisablePhysics()
    {
        AZ_Warning("PhysXRigidBodyComponent", false, "DisablePhysics() not implemented.");
    }

    bool PhysXRigidBodyComponent::IsPhysicsEnabled()
    {
        AZ_Warning("PhysXRigidBodyComponent", false, "IsPhysicsEnabled() not implemented.");
        return true;
    }

    void PhysXRigidBodyComponent::AddImpulse(const AZ::Vector3& impulse)
    {
        m_rigidBody.ApplyLinearImpulse(impulse);
    }

    void PhysXRigidBodyComponent::AddImpulseAtPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint)
    {
        m_rigidBody.ApplyLinearImpulseAtWorldPoint(impulse, worldSpacePoint);
    }

    void PhysXRigidBodyComponent::AddAngularImpulse(const AZ::Vector3& impulse)
    {
        m_rigidBody.ApplyAngularImpulse(impulse);
    }

    AZ::Vector3 PhysXRigidBodyComponent::GetVelocity()
    {
        return m_rigidBody.GetLinearVelocity();
    }

    void PhysXRigidBodyComponent::SetVelocity(const AZ::Vector3& velocity)
    {
        m_rigidBody.SetLinearVelocity(velocity);
    }

    AZ::Vector3 PhysXRigidBodyComponent::GetAngularVelocity()
    {
        return m_rigidBody.GetAngularVelocity();
    }

    void PhysXRigidBodyComponent::SetAngularVelocity(const AZ::Vector3& angularVelocity)
    {
        m_rigidBody.SetAngularVelocity(angularVelocity);
    }

    float PhysXRigidBodyComponent::GetMass()
    {
        return m_rigidBody.GetMass();
    }

    void PhysXRigidBodyComponent::SetMass(float mass)
    {
        m_rigidBody.SetMass(mass);
    }

    float PhysXRigidBodyComponent::GetLinearDamping()
    {
        return m_rigidBody.GetLinearDamping();
    }

    void PhysXRigidBodyComponent::SetLinearDamping(float damping)
    {
        m_rigidBody.SetLinearDamping(damping);
    }

    float PhysXRigidBodyComponent::GetAngularDamping()
    {
        return m_rigidBody.GetAngularDamping();
    }

    void PhysXRigidBodyComponent::SetAngularDamping(float damping)
    {
        m_rigidBody.SetAngularDamping(damping);
    }

    float PhysXRigidBodyComponent::GetSleepThreshold()
    {
        return m_rigidBody.GetSleepThreshold();
    }

    void PhysXRigidBodyComponent::SetSleepThreshold(float threshold)
    {
        m_rigidBody.SetSleepThreshold(threshold);
    }

    AZ::Aabb PhysXRigidBodyComponent::GetAabb()
    {
        return m_rigidBody.GetAabb();
    }
} // namespace PhysX
