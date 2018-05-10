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
#include <PhysXTriggerAreaEventBus.h>

namespace PhysX
{
    /**
    * Component configuring collision flags to enable PhysX trigger functionality.
    * Adding this component to an entity along with a PhysX Collider and PhysX Rigid Body will mark the actor to be a trigger.
    */
    class PhysXTriggerAreaComponent
        : public AZ::Component
        , private PhysXTriggerAreaEventBus::Handler
    {
    public:
        AZ_COMPONENT(PhysXTriggerAreaComponent, "{41C0FAD4-D5E0-46D5-801D-37D04165FF3F}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
            required.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        }

        static void Reflect(AZ::ReflectContext* context);

        PhysXTriggerAreaComponent() = default;
        ~PhysXTriggerAreaComponent() override = default;

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

    private:
        // PhysXTriggerAreaEvents interface implementation
        void OnTriggerEnter(AZ::EntityId entityEntering) override;
        void OnTriggerExit(AZ::EntityId entityExiting) override;
    };
} // namespace PhysX
