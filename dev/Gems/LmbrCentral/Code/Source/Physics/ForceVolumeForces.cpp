/*Reflect(AZ::ReflectContext* context)
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

#include "ForceVolumeForces.h"
#include <AzCore/Math/MathUtils.h>

namespace LmbrCentral
{
    void WorldSpaceForce::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WorldSpaceForce, Force>()
                ->Field("Direction", &WorldSpaceForce::m_direction)
                ->Field("Magnitude", &WorldSpaceForce::m_magnitude)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<WorldSpaceForce>(
                    "World Space Force", "Applies a force in world space")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldSpaceForce::m_direction, "Direction", "Direction of the force")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldSpaceForce::m_magnitude, "Magnitude", "Magnitude of the force")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<WorldSpaceForceRequestBus>("WorldSpaceForceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Physics (Legacy)")
                ->Event("SetDirection", &WorldSpaceForceRequestBus::Events::SetDirection)
                ->Event("GetDirection", &WorldSpaceForceRequestBus::Events::GetDirection)
                ->Event("SetMagnitude", &WorldSpaceForceRequestBus::Events::SetMagnitude)
                ->Event("GetMagnitude", &WorldSpaceForceRequestBus::Events::GetMagnitude)
                ;
        }
    }

    AZ::Vector3 WorldSpaceForce::CalculateForce(const EntityParams& entity, const VolumeParams& volume)
    {
        return m_direction * m_magnitude * entity.m_mass;
    }

    void LocalSpaceForce::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LocalSpaceForce, Force>()
                ->Field("Direction", &LocalSpaceForce::m_direction)
                ->Field("Magnitude", &LocalSpaceForce::m_magnitude)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LocalSpaceForce>(
                    "Local Space Force", "Applies a force in the volume's local space")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LocalSpaceForce::m_direction, "Direction", "Direction of the force")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LocalSpaceForce::m_magnitude, "Magnitude", "Magnitude of the force")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LocalSpaceForceRequestBus>("LocalSpaceForceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Physics (Legacy)")
                ->Event("SetDirection", &LocalSpaceForceRequestBus::Events::SetDirection)
                ->Event("GetDirection", &LocalSpaceForceRequestBus::Events::GetDirection)
                ->Event("SetMagnitude", &LocalSpaceForceRequestBus::Events::SetMagnitude)
                ->Event("GetMagnitude", &LocalSpaceForceRequestBus::Events::GetMagnitude)
                ;
        }
    }

    AZ::Vector3 LocalSpaceForce::CalculateForce(const EntityParams& entity, const VolumeParams& volume)
    {
        return volume.m_rotation * m_direction * m_magnitude * entity.m_mass;
    }

    void PointForce::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PointForce, Force>()
                ->Field("Magnitude", &PointForce::m_magnitude)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PointForce>(
                    "Point Force", "Applies a force relative to the center of the volume")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PointForce::m_magnitude, "Magnitude", "The size of the force")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PointForceRequestBus>("PointForceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Physics (Legacy)")
                ->Event("SetMagnitude", &PointForceRequestBus::Events::SetMagnitude)
                ->Event("GetMagnitude", &PointForceRequestBus::Events::GetMagnitude)
                ;
        }
    }

    AZ::Vector3 PointForce::CalculateForce(const EntityParams& entity, const VolumeParams& volume)
    {
        return (entity.m_position - volume.m_aabb.GetCenter()) * m_magnitude;
    }

    void SplineFollowForce::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SplineFollowForce, Force>()
                ->Field("DampingRatio", &SplineFollowForce::m_dampingRatio)
                ->Field("Frequency", &SplineFollowForce::m_frequency)
                ->Field("TargetSpeed", &SplineFollowForce::m_targetSpeed)
                ->Field("Lookahead", &SplineFollowForce::m_lookAhead)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SplineFollowForce>(
                    "Spline Follow Force", "Applies a force to make objects follow a spline at a given speed")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineFollowForce::m_dampingRatio, "Damping Ratio", "Damping Ratio")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineFollowForce::m_frequency, "Frequency", "Frequency")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineFollowForce::m_targetSpeed, "Target Speed", "Target speed")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SplineFollowForce::m_lookAhead, "Lookahead", "Look ahead")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SplineFollowForceRequestBus>("SplineFollowForceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Physics (Legacy)")
                ->Event("SetDampingRatio", &SplineFollowForceRequestBus::Events::SetDampingRatio)
                ->Event("GetDampingRatio", &SplineFollowForceRequestBus::Events::GetDampingRatio)
                ->Event("SetFrequency", &SplineFollowForceRequestBus::Events::SetFrequency)
                ->Event("GetFrequency", &SplineFollowForceRequestBus::Events::GetFrequency)
                ->Event("SetTargetSpeed", &SplineFollowForceRequestBus::Events::SetTargetSpeed)
                ->Event("GetTargetSpeed", &SplineFollowForceRequestBus::Events::GetTargetSpeed)
                ->Event("SetLookAhead", &SplineFollowForceRequestBus::Events::SetLookAhead)
                ->Event("GetLookAhead", &SplineFollowForceRequestBus::Events::GetLookAhead)
                ;
        }
    }

    void SplineFollowForce::Activate(AZ::EntityId entityId)
    {
        BusConnect(entityId);
        m_loggedMissingSplineWarning = false;
    }

    AZ::Vector3 SplineFollowForce::CalculateForce(const EntityParams& entity, const VolumeParams& volume)
    {
        if (volume.m_spline)
        {
            AZ::Vector3 position = entity.m_position + entity.m_velocity * m_lookAhead;
            AZ::SplineAddress address = volume.m_spline->GetNearestAddressPosition(position - volume.m_position).m_splineAddress;
            AZ::Vector3 splinePosition = volume.m_spline->GetPosition(address);
            AZ::Vector3 splineTangent = volume.m_spline->GetTangent(address);

            // http://www.matthewpeterkelly.com/tutorials/pdControl/index.html
            float kp = pow((2 * AZ::Constants::Pi * m_frequency), 2);
            float kd = 2 * m_dampingRatio * (2 * AZ::Constants::Pi * m_frequency);

            AZ::Vector3 targetVelocity = splineTangent * m_targetSpeed;
            AZ::Vector3 currentVelocity = entity.m_velocity;

            AZ::Vector3 targetPosition = splinePosition + volume.m_position;
            AZ::Vector3 currentPosition = entity.m_position;

            return kp * (targetPosition - currentPosition) + kd * (targetVelocity - currentVelocity);
        }
        else
        {
            if (!m_loggedMissingSplineWarning)
            {
                AZ_Warning("ForceVolumeComponent:SplineFollowForce", false, "Attempting to use force without a spline. Please attach a spline");
                m_loggedMissingSplineWarning = true;
            }
            
            return AZ::Vector3::CreateZero();
        }
    }

    void SimpleDragForce::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SimpleDragForce, Force>()
                ->Field("Drag Coefficient", &SimpleDragForce::m_dragCoefficient)
                ->Field("Volume Density", &SimpleDragForce::m_volumeDensity)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SimpleDragForce>(
                    "Simple Drag Force", "Simulates a drag force on objects")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SimpleDragForce::m_volumeDensity, "Volume Density", "Density of the volume")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SimpleDragForceRequestBus>("SimpleDragForceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Physics (Legacy)")
                ->Event("SetDensity", &SimpleDragForceRequestBus::Events::SetDensity)
                ->Event("GetDensity", &SimpleDragForceRequestBus::Events::GetDensity)
                ;
        }
    }

    AZ::Vector3 SimpleDragForce::CalculateForce(const EntityParams& entity, const VolumeParams& volume)
    {
        // Aproximate shape as a sphere
        AZ::Vector3 center;
        AZ::VectorFloat radius;
        entity.m_aabb.GetAsSphere(center, radius);

        AZ::VectorFloat crossSectionalArea = AZ::Constants::Pi * radius * radius;
        AZ::Vector3 velocitySquared = entity.m_velocity * entity.m_velocity;

        // Wikipedia: https://en.wikipedia.org/wiki/Drag_coefficient
        // Fd = 1/2 * p * u^2 * cd * A
        AZ::Vector3 dragForce = 0.5f * m_volumeDensity * velocitySquared * m_dragCoefficient * crossSectionalArea;

        // The drag force is defined as being in the same direction as the flow velocity. Since the entity is moving
        // and the volume flow is stationary, this just becomes opposite to the entity's velocity. Causing the object to slow
        // down.
        AZ::Vector3 direction(-entity.m_velocity.GetX().GetSign(), -entity.m_velocity.GetY().GetSign(), -entity.m_velocity.GetZ().GetSign());

        return dragForce * direction;
    }

    void LinearDampingForce::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LinearDampingForce, Force>()
                ->Field("Damping", &LinearDampingForce::m_damping)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LinearDampingForce>(
                    "Linear Damping Force", "Applies a force that opposes the objects velocity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &LinearDampingForce::m_damping, "Damping", "Amount of damping")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LinearDampingForceRequestBus>("LinearDampingForceRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Physics (Legacy)")
                ->Event("SetDamping", &LinearDampingForceRequestBus::Events::SetDamping)
                ->Event("GetDamping", &LinearDampingForceRequestBus::Events::GetDamping)
                ;
        }
    }

    AZ::Vector3 LinearDampingForce::CalculateForce(const EntityParams& entity, const VolumeParams& volume)
    {
        return entity.m_velocity * -m_damping * entity.m_mass;
    }
}