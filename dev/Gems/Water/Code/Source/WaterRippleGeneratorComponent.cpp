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
#include "Water_precompiled.h"

#include "WaterRippleGeneratorComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <IRenderAuxGeom.h>

#include "WaterVolumeComponent.h"

namespace Water
{
    void WaterRippleGeneratorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WaterRippleGeneratorComponent, AZ::Component>()
                ->Version(0)
                ->Field("Strength", &WaterRippleGeneratorComponent::m_strength)
                ->Field("Scale", &WaterRippleGeneratorComponent::m_scale)
                ->Field("Interval", &WaterRippleGeneratorComponent::m_rippleIntervalInSeconds)
                ->Field("SpeedThreshold", &WaterRippleGeneratorComponent::m_speedThreshold)
                ;
            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<WaterRippleGeneratorComponent>("Water Ripple Generator", "Configuration for Water Ripple Generator. To use this component requires a PhysX trigger collider component.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WaterVolume.png")
                    ->DataElement(0, &WaterRippleGeneratorComponent::m_strength, "Strength", "The Strength (height scale) of the Ripple being generated.")
                    ->DataElement(0, &WaterRippleGeneratorComponent::m_scale, "Scale", "The Scale (multiplier) of the Ripple being generated.")
                    ->DataElement(0, &WaterRippleGeneratorComponent::m_rippleIntervalInSeconds, "Interval", "The Interval of the Ripple being generated (in seconds).")
                    ->DataElement(0, &WaterRippleGeneratorComponent::m_speedThreshold, "Speed Threshold", "Generate Ripples Continuously while body speed (Meters per second) is greater or equal to the threshold.")
                    ;
            }
        }
    }

    void WaterRippleGeneratorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("WaterRippleGeneratorService", 0xd686c739));
    }

    void WaterRippleGeneratorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void WaterRippleGeneratorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        required.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
    }

    void WaterRippleGeneratorComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void WaterRippleGeneratorComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        Physics::TriggerNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void WaterRippleGeneratorComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        Physics::TriggerNotificationBus::Handler::BusDisconnect(GetEntityId());
    }

    void WaterRippleGeneratorComponent::GenerateWaterRippleAtCurrentLocation() const
    {
        AZ::Vector3 velocity = AZ::Vector3::CreateZero();
        Physics::RigidBodyRequestBus::EventResult(velocity, GetEntityId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);

        if(velocity.GetLength().IsGreaterEqualThan(m_speedThreshold))
        {
            AZ::Vector3 entityLocation = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(entityLocation, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

            gEnv->pRenderer->EF_AddWaterSimHit(Vec3(entityLocation), m_scale, m_strength);
        }
    }

    void WaterRippleGeneratorComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (m_active)
        {
            m_elapsedTimeBetweenRipples += deltaTime;
            if (m_elapsedTimeBetweenRipples >= m_rippleIntervalInSeconds)
            {
                GenerateWaterRippleAtCurrentLocation();
                m_elapsedTimeBetweenRipples = 0.0f;
            }
        }
    }

    bool WaterRippleGeneratorComponent::IsWaterVolumeComponent(const Physics::WorldBody* physicalBody)
    {
        AZ::EntityId entityId = physicalBody->GetEntityId();
        AZ::Entity* triggerEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(triggerEntity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        return triggerEntity && triggerEntity->FindComponent<WaterVolumeComponent>() != nullptr;
    }

    void WaterRippleGeneratorComponent::OnTriggerEnter(const Physics::TriggerEvent& event)
    {
        if (IsWaterVolumeComponent(event.m_otherBody))
        {
            GenerateWaterRippleAtCurrentLocation();
            m_active = true;
        }
    }

    void WaterRippleGeneratorComponent::OnTriggerExit(const Physics::TriggerEvent& event)
    {
        if (IsWaterVolumeComponent(event.m_otherBody))
        {
            m_active = false;
        }
    }
} // namespace Water