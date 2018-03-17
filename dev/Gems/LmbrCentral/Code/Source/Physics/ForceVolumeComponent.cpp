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

#include "LmbrCentral_precompiled.h"
#include "ForceVolumeComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <IPhysics.h>
#include <MathConversion.h>
#include "I3DEngine.h"
#include "LmbrCentral/Shape/ShapeComponentBus.h"

namespace LmbrCentral
{
    using AzFramework::PhysicsComponentRequestBus;

    //Wikipedia: https://en.wikipedia.org/wiki/Drag_coefficient
    static const auto s_sphereDragCoefficient = 0.47f;

    void ForceVolumeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ForceVolumeConfiguration>()
                ->Version(1)
                ->Field("ForceMode", &ForceVolumeConfiguration::m_forceMode)
                ->Field("ForceSpace", &ForceVolumeConfiguration::m_forceSpace)
                ->Field("UseMass", &ForceVolumeConfiguration::m_useMass)
                ->Field("ForceMagnitude", &ForceVolumeConfiguration::m_forceMagnitude)
                ->Field("ForceDirection", &ForceVolumeConfiguration::m_forceDirection)
                ->Field("VolumeDamping", &ForceVolumeConfiguration::m_volumeDamping)
                ->Field("VolumeDensity", &ForceVolumeConfiguration::m_volumeDensity)
            ;
        }
    }

    void ForceVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        ForceVolumeConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ForceVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &ForceVolumeComponent::m_configuration)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ForceVolumeRequestBus>("ForceVolumeRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Event("SetForceMode", &ForceVolumeRequestBus::Events::SetForceMode)
                ->Event("GetForceMode", &ForceVolumeRequestBus::Events::GetForceMode)
                ->Event("SetForceMagnitude", &ForceVolumeRequestBus::Events::SetForceMagnitude)
                ->Event("GetForceMagnitude", &ForceVolumeRequestBus::Events::GetForceMagnitude)
                ->Event("SetUseMass", &ForceVolumeRequestBus::Events::SetUseMass)
                ->Event("GetUseMass", &ForceVolumeRequestBus::Events::GetUseMass)
                ->Event("SetForceDirection", &ForceVolumeRequestBus::Events::SetForceDirection)
                ->Event("GetForceDirection", &ForceVolumeRequestBus::Events::GetForceDirection)
                ->Event("SetAirResistance", &ForceVolumeRequestBus::Events::SetVolumeDamping)
                ->Event("GetAirResistance", &ForceVolumeRequestBus::Events::GetVolumeDamping)
                ->Event("SetAirDensity", &ForceVolumeRequestBus::Events::SetVolumeDensity)
                ->Event("GetAirDensity", &ForceVolumeRequestBus::Events::GetVolumeDensity)
            ;
        }
    }

    ForceVolumeComponent::ForceVolumeComponent(const ForceVolumeConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void ForceVolumeComponent::SetForceMode(ForceMode mode)
    {
        m_configuration.m_forceMode = mode;
    }

    ForceMode ForceVolumeComponent::GetForceMode()
    {
        return m_configuration.m_forceMode;
    }

    void ForceVolumeComponent::SetForceSpace(ForceSpace space)
    {
        m_configuration.m_forceSpace = space;
    }

    ForceSpace ForceVolumeComponent::GetForceSpace()
    {
        return m_configuration.m_forceSpace;
    }

    void ForceVolumeComponent::SetUseMass(bool useMass)
    {
        m_configuration.m_useMass = useMass;
    }

    bool ForceVolumeComponent::GetUseMass()
    {
        return m_configuration.m_useMass;
    }

    void ForceVolumeComponent::SetForceMagnitude(float forceMagnitude)
    {
        m_configuration.m_forceMagnitude = forceMagnitude;
    }

    float ForceVolumeComponent::GetForceMagnitude()
    {
        return m_configuration.m_forceMagnitude;
    }

    void ForceVolumeComponent::SetForceDirection(const AZ::Vector3& direction)
    {
        m_configuration.m_forceDirection = direction;
    }

    const AZ::Vector3& ForceVolumeComponent::GetForceDirection()
    {
        return m_configuration.m_forceDirection;
    }

    void ForceVolumeComponent::SetVolumeDamping(float damping)
    {
        m_configuration.m_volumeDamping = damping;
    }

    float ForceVolumeComponent::GetVolumeDamping()
    {
        return m_configuration.m_volumeDamping;
    }

    void ForceVolumeComponent::SetVolumeDensity(float density)
    {
        m_configuration.m_volumeDensity = density;
    }

    float ForceVolumeComponent::GetVolumeDensity()
    {
        return m_configuration.m_volumeDensity;
    }

    AZ::Vector3 ForceVolumeComponent::GetWorldForceDirectionAtPoint(const ForceVolumeConfiguration& config, const AZ::Vector3& point, const AZ::Vector3& volumeCenter, const AZ::Quaternion& volumeRotation)
    {
        switch (config.m_forceMode)
        {
        case ForceMode::ePoint:
            return point - volumeCenter;
            break;
        case ForceMode::eDirection:            
            if (config.m_forceSpace == ForceSpace::eWorldSpace)
            {
                return config.m_forceDirection;
            }
            else
            {                
                return volumeRotation * config.m_forceDirection;
            }
            break;
        default:
            AZ_Error("ForceVolumeComponent", false, "Unhandled force mode:%d", config.m_forceMode);
            return AZ::Vector3::CreateZero();
        }
    }

    void ForceVolumeComponent::OnTriggerAreaEntered(AZ::EntityId entityId)
    {        
        auto newEntityHasPhysics = PhysicsComponentRequestBus::FindFirstHandler(entityId) != nullptr;
        if (newEntityHasPhysics)
        {
            m_entities.emplace_back(entityId);
        }
    }

    void ForceVolumeComponent::OnTriggerAreaExited(AZ::EntityId entityId)
    {        
        auto found = AZStd::find(m_entities.begin(), m_entities.end(), entityId);
        if (found != m_entities.end())
        {
            m_entities.erase(found);
        }
    }

    void ForceVolumeComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        for (auto entityId : m_entities)
        {
            auto totalForce = AZ::Vector3::CreateZero();

            // Force
            if (!AZ::IsClose(m_configuration.m_forceMagnitude, 0.f, AZ_FLT_EPSILON))
            {
                totalForce += CalculateForce(entityId, deltaTime);
            }

            // Linear damping
            if (m_configuration.m_volumeDamping > 0.f)
            {
                totalForce += CalculateDamping(entityId, deltaTime);
            }

            // Air resistance
            if (m_configuration.m_volumeDensity > 0.f)
            {
                totalForce += CalculateResistance(entityId, deltaTime);
            }

            if (!totalForce.IsZero())
            {
                PhysicsComponentRequestBus::Event(entityId, &AzFramework::PhysicsComponentRequests::AddImpulse, totalForce);
            }
        }
    }

    AZ::Vector3 ForceVolumeComponent::CalculateForce(const AZ::EntityId& entityId, float deltaTime)
    {
        AZ::Vector3 entityPos;
        AZ::Aabb volumeAabb;
        AZ::Quaternion volumeRotation = AZ::Quaternion::CreateIdentity();
        float entityMass = 1.f;

        AZ::TransformBus::EventResult(entityPos, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        ShapeComponentRequestsBus::EventResult(volumeAabb, GetEntityId(), &ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        AZ::TransformBus::EventResult(volumeRotation, GetEntityId(), &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        if (m_configuration.m_useMass)
        {
            PhysicsComponentRequestBus::EventResult(entityMass, entityId, &AzFramework::PhysicsComponentRequestBus::Events::GetMass);
        }        

        auto force = GetWorldForceDirectionAtPoint(m_configuration, entityPos, volumeAabb.GetCenter(), volumeRotation);
        force.SetLength(m_configuration.m_forceMagnitude * entityMass);

        return force;
    }

    AZ::Vector3 ForceVolumeComponent::CalculateDamping(const AZ::EntityId& entityId, float deltaTime)
    {
        AZ::Vector3 damping;
        PhysicsComponentRequestBus::EventResult(damping, entityId, &AzFramework::PhysicsComponentRequestBus::Events::GetVelocity);

        float speed = damping.NormalizeWithLength();
        damping *= -m_configuration.m_volumeDamping * speed;
        return damping;
    }

    AZ::Vector3 ForceVolumeComponent::CalculateResistance(const AZ::EntityId& entityId, float deltaTime)
    {
        AZ::Vector3 velocity;
        AZ::Aabb aabb;
        PhysicsComponentRequestBus::EventResult(velocity, entityId, &AzFramework::PhysicsComponentRequestBus::Events::GetVelocity);
        PhysicsComponentRequestBus::EventResult(aabb, entityId, &AzFramework::PhysicsComponentRequestBus::Events::GetAabb);

        // Aproximate shape as a sphere
        AZ::Vector3 center;
        AZ::VectorFloat radius;
        aabb.GetAsSphere(center, radius);
        float r = radius;

        //Wikipedia: https://en.wikipedia.org/wiki/Drag_coefficient
        auto crossSectionalArea = AZ::Constants::Pi * radius * radius;
        
        AZ::Vector3 dragForce = -0.5f * m_configuration.m_volumeDensity * velocity * velocity * s_sphereDragCoefficient * crossSectionalArea;
        return dragForce;
    }

    void ForceVolumeComponent::Activate()
    {
        ForceVolumeRequestBus::Handler::BusConnect(m_entity->GetId());
        TriggerAreaNotificationBus::Handler::BusConnect(m_entity->GetId());
        AZ::TickBus::Handler::BusConnect();
    }

    void ForceVolumeComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        TriggerAreaNotificationBus::Handler::BusDisconnect();
        ForceVolumeRequestBus::Handler::BusDisconnect();
    }
} // namespace LmbrCentral
