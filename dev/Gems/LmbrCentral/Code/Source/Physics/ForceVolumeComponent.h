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
#include "ForceVolume.h"

#include <AzCore/Math/Spline.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TickBus.h>
#include <LmbrCentral/Scripting/TriggerAreaComponentBus.h>
#include <LmbrCentral/Physics/ForceVolumeRequestBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    /**
     * ForceVolumeComponent
     * 
     * /brief Applies a forces to objects within a volume.
     * 
     * Uses a trigger area and a shape to receive notifications about entities entering and exiting
     * the volume. Each tick a net force will be calculated per entity by summing all the attached forces.
     */
    class ForceVolumeComponent
        : public AZ::Component
        , protected LmbrCentral::TriggerAreaNotificationBus::Handler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(ForceVolumeComponent, "{810D20F9-AD2D-4A5C-BBD4-0175A3874DD6}");
        static void Reflect(AZ::ReflectContext* context);

        ForceVolumeComponent() = default;
        explicit ForceVolumeComponent(const ForceVolume& forceVolume, bool debug);
        ~ForceVolumeComponent() override = default;

    protected:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            required.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }

        // TriggerAreaNotifications
        void OnTriggerAreaEntered(AZ::EntityId entityId) override;
        void OnTriggerAreaExited(AZ::EntityId entityId) override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        void DrawDebugForceForEntity(const EntityParams&, const AZ::Vector3& netForce);

        AZStd::vector<AZ::EntityId> m_entities; ///< List of entitiy ids contained within the volume.
        ForceVolume m_forceVolume; ///< Calculates the net force.
        bool m_debugForces = false; ///< Draws debug lines for entities in the volume
    };
} // namespace LmbrCentral
