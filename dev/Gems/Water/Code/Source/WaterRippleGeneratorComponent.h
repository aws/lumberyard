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
#include <AzCore/Component/TickBus.h>
#include <Water/WaterEffectsBus.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/TriggerBus.h>

namespace Water
{
    /// Component for generating water ripples against a water volume component attached to an entity with a Physx Trigger interacting with a PhysX collider.
    class WaterRippleGeneratorComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private Physics::TriggerNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(WaterRippleGeneratorComponent, "{31A9997E-9D9D-4F07-8344-E0050C4C984C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        WaterRippleGeneratorComponent() = default;
        virtual ~WaterRippleGeneratorComponent() = default;

        bool IsCurrentlyActive() const { return m_active; }

    private:

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override { return AZ::ComponentTickBus::TICK_DEFAULT; }

        // TriggerNotificationBus
        void OnTriggerEnter(const Physics::TriggerEvent& event) override;
        void OnTriggerExit(const Physics::TriggerEvent& event) override;

        void GenerateWaterRippleAtCurrentLocation() const;
        static bool IsWaterVolumeComponent(const Physics::WorldBody* physicalBody);

        bool m_active = false; ///< Ripples are generated while True, activated when PhysX triggers and colliders interact on a Water Volume.
        float m_strength = 1.0f; ///< The Strength (height scale) of the Ripple being generated.
        float m_scale = 1.0f; ///< The Scale (multiplier) of the Ripple being generated.
        float m_rippleIntervalInSeconds = 0.3f; ///< The interval between ripples in seconds.
        float m_speedThreshold = 1.0f; ///< Threshold required to generate ripples in meters per second.
        float m_elapsedTimeBetweenRipples = 0.0f; ///< Accumulator for delta time tracking variable.
    };
}