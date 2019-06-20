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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <PhysX/ColliderComponentBus.h>
#include <Source/Shape.h>
#include <Source/RigidBodyStatic.h>

namespace PhysX
{
    class RigidBodyStatic;

    /// Base class for all runtime collider components.
    class BaseColliderComponent
        : public AZ::Component
        , public AZ::EntityBus::Handler
        , public ColliderComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(BaseColliderComponent, "{D0D48233-DCCA-4125-A6AE-4E5AC5E722D3}");
        static void Reflect(AZ::ReflectContext* context);

        BaseColliderComponent() = default;
        BaseColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration);

        // ColliderComponentRequestBus
        AZStd::shared_ptr<Physics::ShapeConfiguration> GetShapeConfigFromEntity() override;
        const Physics::ColliderConfiguration& GetColliderConfig() override;
        virtual AZStd::shared_ptr<Physics::Shape> GetShape() override;
        virtual void* GetNativePointer() override;
        bool IsStaticRigidBody() override;
        PhysX::RigidBodyStatic* GetStaticRigidBody() override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
            provided.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // Not compatible with cry engine colliders
            incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::EntityBus
        void OnEntityActivated(const AZ::EntityId& parentEntityId) override;


        AZ::Vector3 GetNonUniformScale();
        float GetUniformScale();

    protected:
        // Specific collider components should override this function
        virtual AZStd::shared_ptr<Physics::ShapeConfiguration> CreateScaledShapeConfig();

        Physics::ColliderConfiguration m_configuration;

    private:
        void InitStaticRigidBody();
        void InitShape();

        AZStd::unique_ptr<PhysX::RigidBodyStatic> m_staticRigidBody;
        AZStd::shared_ptr<PhysX::Shape> m_shape;

    };
}
