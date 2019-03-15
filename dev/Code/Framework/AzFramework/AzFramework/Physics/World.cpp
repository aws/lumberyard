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

#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/EditContext.h>

namespace Physics
{
    bool WorldConfiguration::VersionConverter(AZ::SerializeContext& /*context*/,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - remove AutoSimulate
        // - remove TerrainGroup
        // - remove TerrainLayer
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("AutoSimulate", 0xcce85fd9));
            classElement.RemoveElementByName(AZ_CRC("TerrainGroup", 0xca808c89));
            classElement.RemoveElementByName(AZ_CRC("TerrainLayer", 0x439be956));
        }

        // conversion from version 2:
        // - remove TerrainMaterials
        if (classElement.GetVersion() <= 2)
        {
            classElement.RemoveElementByName(AZ_CRC("TerrainMaterials", 0x6da24f86));
        }

        if (classElement.GetVersion() <= 3)
        {
            classElement.RemoveElementByName(AZ_CRC("HandleSimulationEvents", 0xba508787));
        }

        return true;
    }

    void WorldConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WorldConfiguration>()
                ->Version(4, &VersionConverter)
                ->Field("WorldBounds", &WorldConfiguration::m_worldBounds)
                ->Field("MaxTimeStep", &WorldConfiguration::m_maxTimeStep)
                ->Field("FixedTimeStep", &WorldConfiguration::m_fixedTimeStep)
                ->Field("Gravity", &WorldConfiguration::m_gravity)
                ->Field("RaycastBufferSize", &WorldConfiguration::m_raycastBufferSize)
                ->Field("SweepBufferSize", &WorldConfiguration::m_sweepBufferSize)
                ->Field("OverlapBufferSize", &WorldConfiguration::m_overlapBufferSize)
                ->Field("EnableCcd", &WorldConfiguration::m_enableCcd)
                ->Field("EnableActiveActors", &WorldConfiguration::m_enableActiveActors)
                ->Field("EnablePcm", &WorldConfiguration::m_enablePcm)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<WorldConfiguration>("World Configuration", "Default world configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_worldBounds, "World Bounds", "World bounds")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_maxTimeStep, "Max Time Step", "Max time step")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_fixedTimeStep, "Fixed Time Step", "Fixed time step")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_gravity, "Gravity", "Gravity")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_raycastBufferSize, "Raycast Buffer Size", "Maximum number of hits from a raycast")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_sweepBufferSize, "Shapecast Buffer Size", "Maximum number of hits from a shapecast")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_overlapBufferSize, "Overlap Query Buffer Size", "Maximum number of hits from a overlap query")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_enableCcd, "Continuous Collision Detection", "Enabled continuous collision detection in the world")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WorldConfiguration::m_enablePcm, "Persistent Contact Manifold", "Enabled the persistent contact manifold narrow-phase algorithm")
                    ;
            }
        }
    }

    AZStd::vector<OverlapHit> World::OverlapSphere(float radius, const AZ::Transform& pose,
        CustomFilterCallback customFilterCallback)
    {
        SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;

        OverlapRequest overlapRequest;
        overlapRequest.m_pose = pose;
        overlapRequest.m_shapeConfiguration = &shapeConfiguration;
        overlapRequest.m_customFilterCallback = customFilterCallback;
        return Overlap(overlapRequest);
    }

    AZStd::vector<OverlapHit> World::OverlapBox(const AZ::Vector3& dimensions, const AZ::Transform& pose,
        CustomFilterCallback customFilterCallback)
    {
        BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimensions;

        OverlapRequest overlapRequest;
        overlapRequest.m_pose = pose;
        overlapRequest.m_shapeConfiguration = &shapeConfiguration;
        overlapRequest.m_customFilterCallback = customFilterCallback;
        return Overlap(overlapRequest);
    }

    AZStd::vector<OverlapHit> World::OverlapCapsule(float height, float radius, const AZ::Transform& pose,
        CustomFilterCallback customFilterCallback)
    {
        CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_height = height;
        shapeConfiguration.m_radius = radius;

        OverlapRequest overlapRequest;
        overlapRequest.m_pose = pose;
        overlapRequest.m_shapeConfiguration = &shapeConfiguration;
        overlapRequest.m_customFilterCallback = customFilterCallback;
        return Overlap(overlapRequest);
    }

    Physics::RayCastHit World::SphereCast(float radius, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
        QueryType queryType, CollisionGroup collisionGroup, CustomFilterCallback filterCallback)
    {
        SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_customFilterCallback = filterCallback;
        return ShapeCast(request);
    }

    AZStd::vector<Physics::RayCastHit> World::SphereCastMultiple(float radius, const AZ::Transform& startPose, const AZ::Vector3& direction, float distance,
        QueryType queryType, CollisionGroup collisionGroup, CustomFilterCallback filterCallback)
    {
        SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = radius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_customFilterCallback = filterCallback;
        return ShapeCastMultiple(request);
    }

    Physics::RayCastHit World::BoxCast(const AZ::Vector3& boxDimensions, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, CollisionGroup collisionGroup, CustomFilterCallback filterCallback)
    {
        BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = boxDimensions;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_customFilterCallback = filterCallback;
        return ShapeCast(request);
    }

    AZStd::vector<Physics::RayCastHit> World::BoxCastMultiple(const AZ::Vector3& boxDimensions, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, CollisionGroup collisionGroup, CustomFilterCallback filterCallback)
    {
        BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = boxDimensions;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_customFilterCallback = filterCallback;
        return ShapeCastMultiple(request);
    }

    Physics::RayCastHit World::CapsuleCast(float capsuleRadius, float capsuleHeight, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, CollisionGroup collisionGroup, CustomFilterCallback filterCallback)
    {
        CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_height = capsuleHeight;
        shapeConfiguration.m_radius = capsuleRadius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_customFilterCallback = filterCallback;
        return ShapeCast(request);
    }

    AZStd::vector<Physics::RayCastHit> World::CapsuleCastMultiple(float capsuleRadius, float capsuleHeight, const AZ::Transform& startPose,
        const AZ::Vector3& direction, float distance, QueryType queryType, CollisionGroup collisionGroup, CustomFilterCallback filterCallback)
    {
        CapsuleShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_height = capsuleHeight;
        shapeConfiguration.m_radius = capsuleRadius;

        ShapeCastRequest request;
        request.m_distance = distance;
        request.m_start = startPose;
        request.m_direction = direction;
        request.m_shapeConfiguration = &shapeConfiguration;
        request.m_queryType = queryType;
        request.m_collisionGroup = collisionGroup;
        request.m_customFilterCallback = filterCallback;
        return ShapeCastMultiple(request);
    }
} // namespace Physics
