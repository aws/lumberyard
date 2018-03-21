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
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Include/PhysX/PhysXColliderComponentBus.h>

namespace PhysX
{
    /**
    * Component allowing primitive or mesh shapes to be used for collision in PhysX.
    * Adding this component to an entity along with a primitive shape or PhysX Mesh Shape component will interpret it as collision geometry.
    */
    class PhysXColliderComponent
        : public AZ::Component
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , public PhysXColliderComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PhysXColliderComponent, "{C53C7C88-7131-4EEB-A602-A7DF5B47898E}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void Reflect(AZ::ReflectContext* context);

        PhysXColliderComponent() = default;
        ~PhysXColliderComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // ShapeComponentNotificationsBus::Handler
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons) override;

        // PhysXColliderComponentBus
        Physics::Ptr<Physics::ShapeConfiguration> GetShapeConfigFromEntity() override;
    private:
        physx::PxMaterial* m_mat;
    };
} // namespace PhysX
