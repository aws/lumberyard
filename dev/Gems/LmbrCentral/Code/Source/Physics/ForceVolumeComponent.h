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
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Component/TickBus.h>
#include <LmbrCentral/Scripting/TriggerAreaComponentBus.h>
#include <LmbrCentral/Physics/ForceVolumeRequestBus.h>

struct IPhysicalEntity;

namespace LmbrCentral
{
    /**
     * Configuration for the ForceVolume
     * @see ForceVolumeRequests
     */
    struct ForceVolumeConfiguration
    {
        AZ_CLASS_ALLOCATOR(ForceVolumeConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ForceVolumeConfiguration, "{B0270667-B52B-4117-974F-035472E9F25A}");
        static void Reflect(AZ::ReflectContext* context);

        bool UsePoint() const { return m_forceMode == ForceMode::ePoint; }
        bool UseDirection() const { return m_forceMode == ForceMode::eDirection; }

        ForceMode m_forceMode = ForceMode::eDirection;
        ForceSpace m_forceSpace = ForceSpace::eWorldSpace;
        bool m_forceMassDependent = true;
        AZ::Vector3 m_forceDirection = AZ::Vector3::CreateAxisX();
        float m_forceMagnitude = 20.0f;
        float m_volumeDamping = 0.0f;
        float m_volumeDensity = 0.0f;
    };

    /**
     * ForceVolumeComponent
     * 
     * /brief Applies a forces to objects within a volume
     * 
     * Three types of forces are applied to entities within the volume:
     * 
     * Force - a linear force in local/world space. Can be mass indepedent.
     * Damping - A force applied opposite to the entities velocity
     * Resistance - Used to simulate air resistance. Each shape is approximated to a sphere
     * 
     * The forces are calculated for each entity in the CalculateForce, CalculateDamping, CalculateResistance methods and then summed together.
     * Game developers should subclass this component and override the behaviour of these functions for further customisation.
     */
    class ForceVolumeComponent
        : public AZ::Component
        , public ForceVolumeRequestBus::Handler
        , protected LmbrCentral::TriggerAreaNotificationBus::Handler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(ForceVolumeComponent, "{810D20F9-AD2D-4A5C-BBD4-0175A3874DD6}");
        static void Reflect(AZ::ReflectContext* context);

        ForceVolumeComponent() = default;
        explicit ForceVolumeComponent(const ForceVolumeConfiguration& configuration);
        ~ForceVolumeComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // ForceVolumeRequests
        void SetForceMode(ForceMode mode) override;
        ForceMode GetForceMode() override;
        void SetForceSpace(ForceSpace space) override;
        ForceSpace GetForceSpace() override;
        void SetForceMassDependent(bool massDependent) override;
        bool GetForceMassDependent() override;
        void SetForceMagnitude(float magnitude) override;
        float GetForceMagnitude() override;
        void SetForceDirection(const AZ::Vector3& direction) override;
        const AZ::Vector3& GetForceDirection() override;
        void SetVolumeDamping(float damping) override;
        float GetVolumeDamping() override;
        void SetVolumeDensity(float density) override;
        float GetVolumeDensity() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        friend class EditorForceVolumeComponent;

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }

        /// TriggerAreaNotifications
        void OnTriggerAreaEntered(AZ::EntityId entityId) override;
        void OnTriggerAreaExited(AZ::EntityId entityId) override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        static AZ::Vector3 GetWorldForceDirectionAtPoint(const ForceVolumeConfiguration& config, const AZ::Vector3& point, const AZ::Vector3& volumeCenter, const AZ::Quaternion& volumeRotation);

        virtual AZ::Vector3 CalculateForce(const AZ::EntityId& entity, float deltaTime);
        virtual AZ::Vector3 CalculateDamping(const AZ::EntityId& entity, float deltaTime);
        virtual AZ::Vector3 CalculateResistance(const AZ::EntityId& entity, float deltaTime);

        void Activate() override;
        void Deactivate() override;

        ForceVolumeConfiguration m_configuration;
        AZStd::vector<AZ::EntityId> m_entities;
    };
} // namespace LmbrCentral
