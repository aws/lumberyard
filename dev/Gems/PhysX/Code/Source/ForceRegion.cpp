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
#include "PhysX_precompiled.h"

#include <Source/ForceRegion.h>

#include <PhysX/MeshAsset.h>
#include <PhysX/ColliderShapeBus.h>

#include <AzCore/Math/Color.h>
#include <AzFramework/Physics/RigidBodyBus.h>

namespace PhysX
{
    /// Aggregates the AABB of all trigger collider components in an entity.
    struct TriggerAabbAggregator
    {
        AZ::Aabb operator()(AZ::Aabb& lhs, const AZ::Aabb& rhs) const
        {
            if (rhs == AZ::Aabb::CreateNull()) // Ignore non-trigger colliders that may have null AABB.
            {
                return lhs;
            }
            else
            {
                lhs.AddAabb(rhs);
                return lhs;
            }
        }
    };

    /// Aggregates points on trigger collider components in an entity.
    struct TriggerRandomPointsAggregator
    {
        ForceRegionUtil::PointList operator()(ForceRegionUtil::PointList& left
            , const ForceRegionUtil::PointList& right) const
        {
            ForceRegionUtil::PointList combinedPoints;
            combinedPoints.reserve(left.size() + right.size());
            combinedPoints.insert(combinedPoints.end(), left.begin(), left.end());
            combinedPoints.insert(combinedPoints.end(), right.begin(), right.end());
            return combinedPoints;
        }
    };

    void ForceRegion::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            BaseForce::Reflect(*serializeContext);

            serializeContext->Class<ForceRegion>()
                ->Version(1)
                ->Field("Forces", &ForceRegion::m_forces)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ForceRegion>(
                    "Force Region", "Applies forces on entities within a region")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ForceRegion::m_forces, "Forces", "Forces acting in the region")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ForceRegion::ForceRegion(const ForceRegion& forceRegion)
    {
        // Force region must be deep copied as it contains pointers
        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        context->CloneObjectInplace<ForceRegion>(*this, &forceRegion);
    }

    void ForceRegion::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_regionParams = ForceRegionUtil::CreateRegionParams(m_entityId);
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_entityId);
        LmbrCentral::SplineComponentNotificationBus::Handler::BusConnect(m_entityId);
        ForceRegionRequestBus::Handler::BusConnect(m_entityId);
        PhysX::ColliderComponentEventBus::Handler::BusConnect(m_entityId);
        for (auto& force : m_forces)
        {
            force->Activate(m_entityId);
        }

        AZ::TransformBus::EventResult(m_worldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
    }

    void ForceRegion::Deactivate()
    {
        m_entityId.SetInvalid();
        for (auto& force : m_forces)
        {
            force->Deactivate();
        }
        PhysX::ColliderComponentEventBus::Handler::BusDisconnect();
        ForceRegionRequestBus::Handler::BusDisconnect();
        LmbrCentral::SplineComponentNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
    }

    AZ::Vector3 ForceRegion::CalculateNetForce(const EntityParams& entity) const
    {
        auto totalForce = AZ::Vector3::CreateZero();
        for (auto& force : m_forces)
        {
            totalForce += force->CalculateForce(entity, m_regionParams);
        }
        PhysX::ForceRegionNotificationBus::Broadcast(&ForceRegionNotificationBus::Events::OnCalculateNetForce
            , m_regionParams.m_id
            , entity.m_id
            , totalForce.GetNormalized()
            , totalForce.GetLength());
        return totalForce;
    }

    void ForceRegion::ClearForces()
    {
        for (auto& force : m_forces)
        {
            force->Deactivate();
        }
        m_forces.clear();
    }

    PhysX::RegionParams ForceRegion::GetRegionParams() const
    {
        return m_regionParams;
    }

    void ForceRegion::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_worldTransform = world;
        m_regionParams.m_position = world.GetPosition();
        AZ::Transform rotate = world;
        m_regionParams.m_scale = rotate.ExtractScaleExact();
        m_regionParams.m_rotation = AZ::Quaternion::CreateFromTransform(rotate);
        AZ::EBusReduceResult<AZ::Aabb, PhysX::TriggerAabbAggregator> triggerAabb;
        triggerAabb.value = AZ::Aabb::CreateNull();

        ColliderShapeRequestBus::EventResult(triggerAabb
            , m_entityId
            , &ColliderShapeRequestBus::Events::GetColliderShapeAabb);

        m_regionParams.m_aabb = triggerAabb.value;
    }

    void ForceRegion::OnColliderChanged()
    {
        m_regionParams = ForceRegionUtil::CreateRegionParams(m_entityId);
    }

    void ForceRegion::AddForceWorldSpace(const AZ::Vector3& direction, float magnitude)
    {
        AddAndActivateForce(AZStd::make_unique<ForceWorldSpace>(direction, magnitude));
    }

    void ForceRegion::AddForceLocalSpace(const AZ::Vector3& direction, float magnitude)
    {
        AddAndActivateForce(AZStd::make_unique<ForceLocalSpace>(direction, magnitude));
    }

    void ForceRegion::AddForcePoint(float magnitude)
    {
        AddAndActivateForce(AZStd::make_unique<ForcePoint>(magnitude));
    }

    void ForceRegion::AddForceSplineFollow(float dampingRatio, float frequency, float targetSpeed, float lookAhead)
    {
        AddAndActivateForce(AZStd::make_unique<ForceSplineFollow>(dampingRatio, frequency, targetSpeed, lookAhead));
    }

    void ForceRegion::AddForceSimpleDrag(float dragCoefficient, float volumeDensity)
    {
        AddAndActivateForce(AZStd::make_unique<ForceSimpleDrag>(dragCoefficient, volumeDensity));
    }

    void ForceRegion::AddForceLinearDamping(float damping)
    {
        AddAndActivateForce(AZStd::make_unique<ForceLinearDamping>(damping));
    }

    void ForceRegion::AddAndActivateForce(AZStd::unique_ptr<BaseForce> force)
    {
        AZ_Assert(force, "Failed to add and activate null force.");
        if (nullptr == force)
        {
            return;
        }
        m_forces.push_back(AZStd::move(force));
        m_forces.back()->Activate(m_entityId);
    }

    void ForceRegion::OnSplineChanged()
    {
        LmbrCentral::SplineComponentRequestBus::EventResult(m_regionParams.m_spline
            , m_entityId
            , &LmbrCentral::SplineComponentRequestBus::Events::GetSpline);
    }

    RegionParams ForceRegionUtil::CreateRegionParams(const AZ::EntityId& entityId)
    {
        RegionParams regionParams;
        regionParams.m_id = entityId;
        AZ::TransformBus::EventResult(regionParams.m_rotation
            , entityId
            , &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        AZ::TransformBus::EventResult(regionParams.m_position
            , entityId
            , &AZ::TransformBus::Events::GetWorldTranslation);
        AZ::TransformBus::EventResult(regionParams.m_scale
            , entityId
            , &AZ::TransformBus::Events::GetWorldScale);
        LmbrCentral::SplineComponentRequestBus::EventResult(regionParams.m_spline
            , entityId
            , &LmbrCentral::SplineComponentRequestBus::Events::GetSpline);
        AZ::EBusReduceResult<AZ::Aabb, PhysX::TriggerAabbAggregator> triggerAabb;
        triggerAabb.value = AZ::Aabb::CreateNull();
        ColliderShapeRequestBus::EventResult(triggerAabb
            , entityId
            , &ColliderShapeRequestBus::Events::GetColliderShapeAabb);
        regionParams.m_aabb = triggerAabb.value;
        return regionParams;
    }

    EntityParams ForceRegionUtil::CreateEntityParams(const AZ::EntityId& entityId)
    {
        EntityParams entityParams;
        entityParams.m_id = entityId;
        AZ::TransformBus::EventResult(entityParams.m_position
            , entityId
            , &AZ::TransformBus::Events::GetWorldTranslation);
        Physics::RigidBodyRequestBus::EventResultReverse(entityParams.m_velocity
            , entityId
            , &Physics::RigidBodyRequestBus::Events::GetLinearVelocity);
        Physics::RigidBodyRequestBus::EventResultReverse(entityParams.m_mass
            , entityId
            , &Physics::RigidBodyRequestBus::Events::GetMass);
        AZ::EBusReduceResult<AZ::Aabb, PhysX::TriggerAabbAggregator> triggerAabb;
        triggerAabb.value = AZ::Aabb::CreateNull();
        ColliderShapeRequestBus::EventResult(triggerAabb
            , entityId
            , &ColliderShapeRequestBus::Events::GetColliderShapeAabb);
        entityParams.m_aabb = triggerAabb.value;
        return entityParams;
    }

    ForceRegionUtil::PointList ForceRegionUtil::GenerateBoxPoints(const AZ::Vector3& min, const AZ::Vector3& max)
    {
        ForceRegionUtil::PointList pointList;

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

    ForceRegionUtil::PointList ForceRegionUtil::GenerateSpherePoints(float radius)
    {
        ForceRegionUtil::PointList points;

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

    ForceRegionUtil::PointList ForceRegionUtil::GenerateCylinderPoints(float height, float radius)
    {
        ForceRegionUtil::PointList points;
        AZ::Vector3 base(0.f, 0.f, -height * 0.5f);
        AZ::Vector3 radiusVector(radius, 0.f, 0.f);

        const auto sides = AZ::GetClamp(radius, 3.f, 8.f);
        const auto segments = AZ::GetClamp(height * 0.5f, 2.f, 8.f);
        const auto angleDelta = AZ::Quaternion::CreateRotationZ(AZ::Constants::TwoPi / sides);
        const auto segmentDelta = height / (segments - 1);
        for (auto segment = 0; segment < segments; ++segment)
        {
            for (auto side = 0; side < sides; ++side)
            {
                auto point = base + radiusVector;
                points.emplace_back(point);
                radiusVector = angleDelta * radiusVector;
            }
            base += AZ::Vector3(0, 0, segmentDelta);
        }
        return points;
    }

    ForceRegionUtil::PointList ForceRegionUtil::GenerateMeshPoints(const AZ::Data::Asset<PhysX::Pipeline::MeshAsset>& meshAsset
        , const AZ::Vector3& assetScale)
    {
        AZStd::vector<AZ::Vector3> randomPoints;
        const AZ::u32 minPointCount = 2;
        const AZ::u32 maxPointCount = 50;

        AZ::Data::Asset<PhysX::Pipeline::MeshAsset> meshColliderAsset = meshAsset;
        if (meshColliderAsset && meshColliderAsset.IsReady())
        {
            physx::PxBase* meshData = meshColliderAsset.Get()->GetMeshData();

            if (meshData)
            {
                const physx::PxVec3* vertices = nullptr;
                AZ::u32 vertexCount = 0;
                physx::PxMeshScale meshScale;
                if (meshData->is<physx::PxTriangleMesh>())
                {
                    physx::PxTriangleMeshGeometry mesh = physx::PxTriangleMeshGeometry(reinterpret_cast<physx::PxTriangleMesh*>(meshData));
                    meshScale = mesh.scale;
                    const physx::PxTriangleMesh* triangleMesh = mesh.triangleMesh;
                    vertexCount = triangleMesh->getNbVertices();
                    vertices = triangleMesh->getVertices();
                }
                else
                {
                    physx::PxConvexMeshGeometry mesh = physx::PxConvexMeshGeometry(reinterpret_cast<physx::PxConvexMesh*>(meshData));
                    meshScale = mesh.scale;
                    const physx::PxConvexMesh* convexMesh = mesh.convexMesh;
                    vertexCount = convexMesh->getNbVertices();
                    vertices = convexMesh->getVertices();
                }

                AZ::u32 randomPointCount = vertexCount / 10;
                randomPointCount = AZ::GetMax(randomPointCount, minPointCount);
                randomPointCount = AZ::GetMin(randomPointCount, maxPointCount);

                randomPoints.reserve(randomPointCount);

                AZ::u32 step = vertexCount / randomPointCount;
                for (AZ::u32 vert = 0; vert < vertexCount; vert += step)
                {
                    if (vert >= vertexCount)
                    {
                        vert = vert % (vertexCount);
                    }
                    AZ::Vector3 vertex = PxMathConvert(meshScale.transform(vertices[vert]));
                    vertex = vertex * assetScale;
                    randomPoints.push_back(vertex);
                }
            }
        }

        return randomPoints;
    }
}