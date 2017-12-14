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
#pragma once

#include <PxPhysicsAPI.h>
#include <PhysXSystemComponent.h>
#include <PhysXRigidBody.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/RigidBody.h>

namespace PhysX
{
    /**
    * Component used to register an entity as a rigid body in the PhysX simulation.
    */
    class PhysXRigidBodyComponent
        : public AZ::Component
        , protected AZ::TransformNotificationBus::MultiHandler
        , protected AZ::EntityBus::MultiHandler
        , protected EntityPhysXEventBus::Handler
    {
    public:
        AZ_COMPONENT(PhysXRigidBodyComponent, "{D4E52A70-BDE1-4819-BD3C-93AB3F4F3BE3}");

        static void Reflect(AZ::ReflectContext* context);

        PhysXRigidBodyComponent() = default;
        explicit PhysXRigidBodyComponent(const PhysXRigidBodyConfiguration& config);
        ~PhysXRigidBodyComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

    protected:
        // ColliderComponentEvents
        //void OnColliderChanged() override;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // EntityPhysXEvents
        void OnPostStep() override;

        // TransformNotifications (listening to self and descendants)
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        void OnChildAdded(AZ::EntityId childId) override;

        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        void AddCollidersFromEntityAndDescendants(const AZ::EntityId& rootEntityId);
        void AddCollidersFromEntity(const AZ::EntityId& entityId);

        PhysXRigidBody m_rigidBody;

    private:
        AZ_DISABLE_COPY_MOVE(PhysXRigidBodyComponent);
    };
} // namespace PhysX
