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
#include "CapsuleShapeComponent.h"

#include <AzCore/std/containers/array.h>
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <MathConversion.h>
#include <Shape/ShapeGeometryUtil.h>

namespace LmbrCentral
{
    const AZ::u32 g_capsuleDebugShapeSides = 16;
    const AZ::u32 g_capsuleDebugShapeCapSegments = 8;

    void CapsuleShape::Reflect(AZ::ReflectContext* context)
    {
        CapsuleShapeConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CapsuleShape>()
                ->Version(1)
                ->Field("Configuration", &CapsuleShape::m_capsuleShapeConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CapsuleShape>("Capsule Shape", "Capsule shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShape::m_capsuleShapeConfig, "Capsule Configuration", "Capsule shape configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
            }
        }
    }

    void CapsuleShape::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        ShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
        CapsuleShapeComponentRequestsBus::Handler::BusConnect(m_entityId);
    }

    void CapsuleShape::Deactivate()
    {
        CapsuleShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void CapsuleShape::InvalidateCache(InvalidateShapeCacheReason reason)
    {
        m_intersectionDataCache.InvalidateCache(reason);
    }

    void CapsuleShape::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::TransformChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void CapsuleShape::SetHeight(float height)
    {
        m_capsuleShapeConfig.m_height = height;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CapsuleShape::SetRadius(float radius)
    {
        m_capsuleShapeConfig.m_radius = radius;
        m_intersectionDataCache.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            m_entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    float CapsuleShape::GetHeight()
    {
        return m_capsuleShapeConfig.m_height;
    }
    
    float CapsuleShape::GetRadius()
    {
        return m_capsuleShapeConfig.m_radius;
    }

    AZ::Aabb CapsuleShape::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig);

        const AZ::Aabb topAabb(AZ::Aabb::CreateCenterRadius(
            m_intersectionDataCache.m_topPlaneCenterPoint, m_intersectionDataCache.m_radius));
        AZ::Aabb baseAabb(AZ::Aabb::CreateCenterRadius(
            m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_radius));
        baseAabb.AddAabb(topAabb);
        return baseAabb;
    }

    bool CapsuleShape::IsPointInside(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig);

        const float radiusSquared = powf(m_intersectionDataCache.m_radius, 2.0f);

        // Check Bottom sphere
        if (AZ::Intersect::PointSphere(m_intersectionDataCache.m_basePlaneCenterPoint, radiusSquared, point))
        {
            return true;
        }

        // If the capsule is infact just a sphere then just stop (because the height of the cylinder <= 2 * radius of the cylinder)
        if (m_intersectionDataCache.m_isSphere)
        {
            return false;
        }

        // Check Top sphere
        if (AZ::Intersect::PointSphere(m_intersectionDataCache.m_topPlaneCenterPoint, radiusSquared, point))
        {
            return true;
        }

        // If its not in either sphere check the cylinder
        return AZ::Intersect::PointCylinder(
            m_intersectionDataCache.m_basePlaneCenterPoint, m_intersectionDataCache.m_axisVector,
            powf(m_intersectionDataCache.m_internalHeight, 2.0f), radiusSquared, point);
    }

    float CapsuleShape::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig);

        const Lineseg lineSeg(
            AZVec3ToLYVec3(m_intersectionDataCache.m_basePlaneCenterPoint),
            AZVec3ToLYVec3(m_intersectionDataCache.m_topPlaneCenterPoint));

        float t = 0.0f;
        float distance = Distance::Point_Lineseg(AZVec3ToLYVec3(point), lineSeg, t);
        distance -= m_intersectionDataCache.m_radius;
        return powf(AZStd::max(distance, 0.0f), 2.0f);
    }

    bool CapsuleShape::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, m_capsuleShapeConfig);

        if (m_intersectionDataCache.m_isSphere)
        {
            return AZ::Intersect::IntersectRaySphere(
                src, dir, m_intersectionDataCache.m_basePlaneCenterPoint,
                m_intersectionDataCache.m_radius, distance) > 0;
        }

        AZ::VectorFloat t;
        const AZ::VectorFloat rayLength = AZ::VectorFloat(1000.0f);
        const bool intersection = AZ::Intersect::IntersectSegmentCapsule(
            src, dir * rayLength, m_intersectionDataCache.m_basePlaneCenterPoint,
            m_intersectionDataCache.m_topPlaneCenterPoint, m_intersectionDataCache.m_radius, t) > 0;
        distance = rayLength * t;
        return intersection;
    }

    void CapsuleShape::CapsuleIntersectionDataCache::UpdateIntersectionParamsImpl(
        const AZ::Transform& currentTransform, const CapsuleShapeConfig& configuration)
    {
        const AZ::VectorFloat entityScale = currentTransform.RetrieveScale().GetMaxElement();
        m_axisVector = currentTransform.GetBasisZ().GetNormalizedSafe() * entityScale;

        const float internalCylinderHeight = configuration.m_height - configuration.m_radius * 2.0f;
        if (internalCylinderHeight > std::numeric_limits<float>::epsilon())
        {
            const AZ::Vector3 currentPositionToPlanesVector = m_axisVector * (internalCylinderHeight * 0.5f);
            m_topPlaneCenterPoint = currentTransform.GetPosition() + currentPositionToPlanesVector;
            m_basePlaneCenterPoint = currentTransform.GetPosition() - currentPositionToPlanesVector;
            m_axisVector = m_axisVector * internalCylinderHeight;
            m_isSphere = false;
        }
        else
        {
            m_basePlaneCenterPoint = currentTransform.GetPosition();
            m_topPlaneCenterPoint = currentTransform.GetPosition();
            m_isSphere = true;
        }

        // scale intersection data cache radius by entity transform for internal calculations
        m_radius = configuration.m_radius * entityScale;
        m_internalHeight = internalCylinderHeight;
    }

    /**
     * Generate vertices for triangles to make up a complete capsule.
     */
    static void GenerateSolidCapsuleMeshVertices(
        const AZ::Transform& worldFromLocal, float radius, float height,
        AZ::u32 sides, AZ::u32 capSegments, Vec3* vertices)
    {
        const float middleHeight = AZ::GetMax(height - radius * 2.0f, 0.0f);
        const float halfMiddleHeight = middleHeight * 0.5f;

        vertices = CapsuleTubeUtil::GenerateSolidStartCap(
            worldFromLocal, AZ::Vector3::CreateAxisZ(-halfMiddleHeight), radius,
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(),
            sides, capSegments, vertices);

        AZStd::array<float, 2> endPositions = {{ -halfMiddleHeight, halfMiddleHeight }};
        for (int i = 0; i < endPositions.size(); ++i)
        {
            const AZ::Vector3 position = AZ::Vector3::CreateAxisZ(endPositions[i]);
            vertices = CapsuleTubeUtil::GenerateSegmentVertices(
                position, AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(),
                radius, sides, worldFromLocal, vertices);
        }

        CapsuleTubeUtil::GenerateSolidEndCap(
            worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight), radius,
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(),
            sides, capSegments, vertices);
    }

    /**
     * Generate vertices (via GenerateSolidCapsuleMeshVertices) and then build index
     * list for capsule shape for solid rendering.
     */
    static void GenerateSolidCapsuleMesh(
        const AZ::Transform& worldFromLocal, float radius, float height,
        AZ::u32 sides, AZ::u32 capSegments, AZStd::vector<Vec3>& vertexBufferOut,
        AZStd::vector<vtx_idx>& indexBufferOut)
    {
        const AZ::u32 segments = 1;
        const AZ::u32 totalSegments = segments + capSegments * 2;

        const size_t numVerts = sides * (totalSegments + 1) + 2;
        const size_t numTriangles = (sides * totalSegments) * 2 + (sides * 2);

        vertexBufferOut.resize(numVerts);
        indexBufferOut.resize(numTriangles * 3);

        GenerateSolidCapsuleMeshVertices(
            worldFromLocal, radius, height, sides, capSegments, &vertexBufferOut[0]);

        CapsuleTubeUtil::GenerateSolidMeshIndices(
            sides, segments, capSegments, &indexBufferOut[0]);
    }

    /**
     * Generate full wire mesh for capsule (end caps, loops, and lines along length).
     */
    static void GenerateWireCapsuleMesh(
        const AZ::Transform& worldFromLocal, float radius, float height,
        AZ::u32 sides, AZ::u32 capSegments, AZStd::vector<Vec3>& lineBufferOut)
    {
        // notes on vert buffer size
        // total end segments
        // 2 verts for each segment
        //  2 * capSegments for one full half arc
        //   2 arcs per end
        //    2 ends
        // total segments
        // 2 verts for each segment
        //  2 lines - top and bottom
        //   2 lines - left and right
        // total loops
        // 2 verts for each segment
        //  loops == sides
        //   1 extra segment needed for last loop
        //    1 extra segment needed for centre loop
        const AZ::u32 segments = 1;
        const AZ::u32 totalEndSegments = capSegments * 2 * 2 * 2 * 2;
        const AZ::u32 totalSegments = segments * 2 * 2 * 2;
        const AZ::u32 totalLoops = sides * 2 * (segments + 2);

        const size_t numVerts = totalEndSegments + totalSegments + totalLoops;
        lineBufferOut.resize(numVerts);

        const float middleHeight = AZ::GetMax(height - radius * 2.0f, 0.0f);
        const float halfMiddleHeight = middleHeight * 0.5f;

        // start cap
        Vec3* vertices = CapsuleTubeUtil::GenerateWireCap(
            worldFromLocal,
            AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
            radius,
            -AZ::Vector3::CreateAxisZ(),
            AZ::Vector3::CreateAxisX(),
            capSegments,
            &lineBufferOut[0]);

        // first loop
        vertices = CapsuleTubeUtil::GenerateWireLoop(
            worldFromLocal, AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), sides, radius, vertices);

        // centre loop
        vertices = CapsuleTubeUtil::GenerateWireLoop(
            worldFromLocal, AZ::Vector3::CreateZero(),
            AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), sides, radius, vertices);

        // body
        // left line
        vertices = WriteVertex(CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                radius, AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), 0.0f),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                radius, AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), 0.0f),
            vertices);
        // right line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                radius, -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), AZ::Constants::Pi),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                radius, -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisX(), AZ::Constants::Pi),
            vertices);
        // top line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                radius, AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), 0.0f),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                radius, AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), 0.0f),
            vertices);
        // bottom line
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(-halfMiddleHeight),
                radius, -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), AZ::Constants::Pi),
            vertices);
        vertices = WriteVertex(
            CapsuleTubeUtil::CalculatePositionOnSphere(
                worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight),
                radius, -AZ::Vector3::CreateAxisZ(), AZ::Vector3::CreateAxisY(), AZ::Constants::Pi),
            vertices);

        // final loop
        vertices = CapsuleTubeUtil::GenerateWireLoop(
            worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight), AZ::Vector3::CreateAxisZ(),
            AZ::Vector3::CreateAxisX(), sides, radius, vertices);

        // end cap
        CapsuleTubeUtil::GenerateWireCap(
            worldFromLocal, AZ::Vector3::CreateAxisZ(halfMiddleHeight),
            radius, AZ::Vector3::CreateAxisZ(),
            AZ::Vector3::CreateAxisX(), capSegments,
            vertices);
    }

    void GenerateCapsuleMesh(
        const AZ::Transform& worldFromLocal, float radius, float height,
        AZ::u32 sides, AZ::u32 capSegments, AZStd::vector<Vec3>& vertexBufferOut,
        AZStd::vector<vtx_idx>& indexBufferOut, AZStd::vector<Vec3>& lineBufferOut)
    {
        AZ::Transform worldFromLocalUniformScale = worldFromLocal;
        const AZ::VectorFloat maxScale = worldFromLocalUniformScale.ExtractScale().GetMaxElement();
        worldFromLocalUniformScale *= AZ::Transform::CreateScale(AZ::Vector3(maxScale));

        GenerateSolidCapsuleMesh(
            worldFromLocalUniformScale, radius, height, sides,
            capSegments, vertexBufferOut, indexBufferOut);

        GenerateWireCapsuleMesh(
            worldFromLocalUniformScale, radius, height, sides, capSegments, lineBufferOut);
    }
} // namespace LmbrCentral
