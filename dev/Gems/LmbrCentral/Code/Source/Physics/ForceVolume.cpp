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
#include "ForceVolume.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/TubeShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include "Shape/SphereShape.h"

namespace LmbrCentral
{
    const AZ::Color ForceVolume::s_arrowColor = AZ::Color(0.f, 0.f, 1.f, 1.f);

    void ForceVolume::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            Force::Reflect(*serializeContext);

            serializeContext->Class<ForceVolume>()
                ->Version(1)
                ->Field("Forces", &ForceVolume::m_forces)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ForceVolume>(
                    "Force Volume", "Applies forces on entities within a volume")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ForceVolume::m_forces, "Forces", "Forces acting in the volume")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ForceVolume::ForceVolume(const ForceVolume& forceVolume)
    {
        // Force volume must be deep copied as it contains pointers
        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        context->CloneObjectInplace<ForceVolume>(*this, &forceVolume);
    }

    ForceVolume::~ForceVolume()
    {
        for (auto force : m_forces)
        {
            delete force;
        }
    }

    void ForceVolume::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_volumeParams = ForceVolumeUtil::CreateVolumeParams(m_entityId);
        AZ::TransformNotificationBus::MultiHandler::BusConnect(entityId);
        SplineComponentNotificationBus::Handler::BusConnect(entityId);
        ShapeComponentNotificationsBus::Handler::BusConnect(entityId);
        AZ::TransformBus::EventResult(m_worldTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
        for (auto force : m_forces)
        {
            force->Activate(entityId);
        }
    }

    void ForceVolume::Deactivate()
    {
        m_entityId.SetInvalid();
        for (auto force : m_forces)
        {
            force->Deactivate();
        }
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        SplineComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
    }

    AZ::Vector3 ForceVolume::CalculateNetForce(const EntityParams& entity) const
    {
        auto totalForce = AZ::Vector3::CreateZero();
        for (auto force : m_forces)
        {
            totalForce += force->CalculateForce(entity, m_volumeParams);
        }
        return totalForce;
    }

    void ForceVolume::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_worldTransform = world;
        m_volumeParams.m_position = world.GetTranslation();
        AZ::Transform rotate = world;
        rotate.ExtractScaleExact();
        m_volumeParams.m_rotation = AZ::Quaternion::CreateFromTransform(rotate);
    }

    void ForceVolume::OnSplineChanged()
    {
        SplineComponentRequestBus::EventResult(m_volumeParams.m_spline, m_entityId, &SplineComponentRequestBus::Events::GetSpline);
    }

    void ForceVolume::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        ShapeComponentRequestsBus::EventResult(m_volumeParams.m_aabb, m_entityId, &ShapeComponentRequestsBus::Events::GetEncompassingAabb);
    }

    void ForceVolume::Display(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::Crc32 shapeType;
        ShapeComponentRequestsBus::EventResult(shapeType, m_entityId, &ShapeComponentRequestsBus::Events::GetShapeType);

        ForceVolumeUtil::PointList points;
        if (shapeType == AZ::Crc32("Tube"))
        {
            float radius = 0.f;
            AZ::SplinePtr spline;
            TubeShapeComponentRequestsBus::EventResult(radius, m_entityId, &TubeShapeComponentRequestsBus::Events::GetRadius);
            SplineComponentRequestBus::EventResult(spline, m_entityId, &SplineComponentRequestBus::Events::GetSpline);
            points = ForceVolumeUtil::GenerateTubePoints(*spline, radius);
        }
        else if (shapeType == AZ::Crc32("Sphere"))
        {
            SphereShapeConfig config;
            SphereShapeComponentRequestsBus::EventResult(config, m_entityId, &SphereShapeComponentRequestsBus::Events::GetSphereConfiguration);
            points =  ForceVolumeUtil::GenerateSpherePoints(config.m_radius);
        }
        else if (shapeType == AZ::Crc32("Box"))
        {
            AZ::Vector3 dimensions;
            BoxShapeComponentRequestsBus::EventResult(dimensions, m_entityId, &BoxShapeComponentRequestsBus::Events::GetBoxDimensions);
            points = ForceVolumeUtil::GenerateBoxPoints(dimensions * -0.5f, dimensions * 0.5f);
        }
        else if (shapeType == AZ::Crc32("Cylinder"))
        {
            LmbrCentral::CylinderShapeConfig cylinder;
            LmbrCentral::CylinderShapeComponentRequestsBus::EventResult(cylinder, m_entityId, &CylinderShapeComponentRequestsBus::Events::GetCylinderConfiguration);
            points = ForceVolumeUtil::GenerateCylinderPoints(cylinder.m_height, cylinder.m_radius);
        }
        else if (shapeType == AZ::Crc32("Capsule"))
        {
            LmbrCentral::CapsuleShapeConfig capsule;
            LmbrCentral::CapsuleShapeComponentRequestsBus::EventResult(capsule, m_entityId, &CapsuleShapeComponentRequestsBus::Events::GetCapsuleConfiguration);
            points = ForceVolumeUtil::GenerateCylinderPoints(capsule.m_height - capsule.m_radius * 2.f, capsule.m_radius);
        }
        else
        {
            AZ_Warning("", false, "ForceVolume:Unknown shape type. Forces won't render");
            return;
        }

        // Display forces
        debugDisplay.SetColor(s_arrowColor);
        for (auto& point : points)
        {
            ForceVolumeUtil::DisplayForceDirection(*this, debugDisplay, m_worldTransform * point);
        }
    }

    VolumeParams ForceVolumeUtil::CreateVolumeParams(const AZ::EntityId& entityId)
    {
        VolumeParams volume;
        volume.m_id = entityId;
        AZ::TransformBus::EventResult(volume.m_rotation, entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        AZ::TransformBus::EventResult(volume.m_position, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        SplineComponentRequestBus::EventResult(volume.m_spline, entityId, &SplineComponentRequestBus::Events::GetSpline);
        ShapeComponentRequestsBus::EventResult(volume.m_aabb, entityId, &ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return volume;
    }

    EntityParams ForceVolumeUtil::CreateEntityParams(const AZ::EntityId& entityId)
    {
        EntityParams entity;
        entity.m_id = entityId;
        AZ::TransformBus::EventResult(entity.m_position, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        AzFramework::PhysicsComponentRequestBus::EventResultReverse(entity.m_velocity, entityId, &AzFramework::PhysicsComponentRequestBus::Events::GetVelocity);
        AzFramework::PhysicsComponentRequestBus::EventResultReverse(entity.m_mass, entityId, &AzFramework::PhysicsComponentRequestBus::Events::GetMass);
        entity.m_aabb = GetMaxAabb(entityId);
        return entity;
    }

    AZ::Aabb ForceVolumeUtil::GetMaxAabb(const AZ::EntityId& entityId)
    {
        AZ::Aabb combinedAabb = AZ::Aabb::CreateNull();
        AZ::Aabb componentAabb = AZ::Aabb::CreateNull();

        LmbrCentral::RenderBoundsRequestBus::EventResult(componentAabb, entityId, &LmbrCentral::RenderBoundsRequests::GetWorldBounds);
        combinedAabb.AddAabb(componentAabb);

        ShapeComponentRequestsBus::EventResult(componentAabb, entityId, &ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        combinedAabb.AddAabb(componentAabb);

        return combinedAabb;
    }

    ForceVolumeUtil::PointList ForceVolumeUtil::GenerateBoxPoints(const AZ::Vector3& min, const AZ::Vector3& max)
    {
        ForceVolumeUtil::PointList pointList;

        auto size = max - min;

        const auto minSamples = 2.f;
        const auto maxSamples = 8.f;
        const auto desiredSampleDelta = 2.f;

        // How many sample in each axis
        int numSamples[] =
        {
            static_cast<int>((size.GetX() / desiredSampleDelta).GetClamp(minSamples, maxSamples)),
            static_cast<int>((size.GetY() / desiredSampleDelta).GetClamp(minSamples, maxSamples)),
            static_cast<int>((size.GetZ() / desiredSampleDelta).GetClamp(minSamples, maxSamples))
        };

        float sampleDelta[] =
        {
            size.GetX() / static_cast<float>(numSamples[0] - 1),
            size.GetY() / static_cast<float>(numSamples[1] - 1),
            size.GetZ() / static_cast<float>(numSamples[2] - 1),
        };

        for (auto i = 0; i < numSamples[0]; ++i)
        {
            for (auto j = 0; j < numSamples[1]; ++j)
            {
                for (auto k = 0; k < numSamples[2]; ++k)
                {
                    pointList.emplace_back(
                        min.GetX() + i * sampleDelta[0],
                        min.GetY() + j * sampleDelta[1],
                        min.GetZ() + k * sampleDelta[2]
                    );
                }
            }
        }
        return pointList;
    }

    ForceVolumeUtil::PointList ForceVolumeUtil::GenerateSpherePoints(float radius)
    {
        ForceVolumeUtil::PointList points;

        int nSamples = static_cast<int>(radius * 5);
        nSamples = AZ::GetClamp(nSamples, 5, 512);

        // Draw arrows using Fibonacci sphere
        float offset = 2.f / nSamples;
        float increment = AZ::Constants::Pi * (3.f - sqrt(5.f));
        for (int i = 0; i < nSamples; ++i)
        {
            float phi = ((i + 1) % nSamples) * increment;
            float y = ((i * offset) - 1) + (offset / 2.f);
            float r = sqrt(1 - pow(y, 2));
            float x = cos(phi) * r;
            float z = sin(phi) * r;
            points.emplace_back(x * radius, y * radius, z * radius);
        }
        return points;
    }

    ForceVolumeUtil::PointList ForceVolumeUtil::GenerateTubePoints(const AZ::Spline& spline, float radius)
    {
        ForceVolumeUtil::PointList points;
        auto address = spline.GetAddressByFraction(0.0f);
        const auto delta = 1.0f / static_cast<float>(spline.GetSegmentGranularity());
        const auto sides = 4;
        while (address.m_segmentIndex < spline.GetSegmentCount())
        {
            for (auto i = 0; i < spline.GetSegmentGranularity(); ++i)
            {
                GenerateCirclePoints(
                    sides, 
                    radius, 
                    spline.GetPosition(address),
                    spline.GetNormal(address),
                    spline.GetTangent(address),
                    points
                );
                
                address.m_segmentFraction += delta;
            }
            address.m_segmentIndex++;
            address.m_segmentFraction = 0.f;
        }

        // Generate final set of points
        address = spline.GetAddressByFraction(1.0f);
        GenerateCirclePoints(
            sides, 
            radius, 
            spline.GetPosition(address),
            spline.GetNormal(address),
            spline.GetTangent(address),
            points
        );

        return points;
    }

    ForceVolumeUtil::PointList ForceVolumeUtil::GenerateCylinderPoints(float height, float radius)
    {
        PointList points;
        AZ::Vector3 base(0.f, 0.f, -height * 0.5f);
        AZ::Vector3 radiusVector(radius, 0.f, 0.f);

        const auto sides = AZ::GetClamp(radius, 3.f, 8.f);
        const auto segments = AZ::GetClamp(height * 0.5f, 2.f, 8.f);
        const auto angleDelta = AZ::Quaternion::CreateRotationZ(AZ::Constants::TwoPi / sides);
        const auto segmentDelta = height / (segments-1);
        for (auto i = 0; i < segments; ++i)
        {
            for (auto j = 0; j < sides; ++j)
            {
                auto point = base + radiusVector;
                points.emplace_back(point);
                radiusVector = angleDelta * radiusVector;
            }
            base += AZ::Vector3(0, 0, segmentDelta);
        }
        return points;
    }

    void ForceVolumeUtil::GenerateCirclePoints(int sides, float radius, const AZ::Vector3& position, const AZ::Vector3& normal, const AZ::Vector3& tangent, ForceVolumeUtil::PointList& points)
    {
        AZ::Vector3 currentNormal = normal;
        auto deltaRotation = AZ::Quaternion::CreateFromAxisAngle(tangent, AZ::Constants::TwoPi / sides);
        for (auto j = 0; j < sides; ++j)
        {
            auto samplePosition = position + currentNormal * radius;
            points.emplace_back(samplePosition);
            currentNormal = deltaRotation * currentNormal;
        }
    }

    void ForceVolumeUtil::DisplayForceDirection(const ForceVolume& forceVolume, AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& worldPoint)
    {
        EntityParams entityParams;
        entityParams.m_id.SetInvalid();
        entityParams.m_position = worldPoint;
        entityParams.m_velocity = AZ::Vector3::CreateZero();
        entityParams.m_mass = 1.f;

        AZ::Vector3 forceDirection = forceVolume.CalculateNetForce(entityParams);
        if (!forceDirection.IsZero())
        {
            forceDirection.Normalize();
            forceDirection *= 0.5f;
            debugDisplay.DrawArrow(worldPoint - forceDirection, worldPoint + forceDirection, 1.5f);
        }
        else
        {
            debugDisplay.DrawBall(worldPoint, 0.05f);
        }
    }
}
