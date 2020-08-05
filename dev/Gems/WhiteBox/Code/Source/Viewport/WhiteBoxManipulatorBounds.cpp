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

#include "WhiteBox_precompiled.h"

#include "Util/WhiteBoxMathUtil.h"
#include "WhiteBoxManipulatorBounds.h"

#include <AzCore/Math/IntersectSegment.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(ManipulatorBoundPolygon, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ManipulatorBoundEdge, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BoundShapePolygon, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BoundShapeEdge, AZ::SystemAllocator, 0)

    bool IntersectRayVertex(
        const VertexBound& vertexBound, const float vertexScreenRadius, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        AZ::VectorFloat distance;
        if (AZ::Intersect::IntersectRaySphere(
                rayOrigin, rayDirection, vertexBound.m_center, vertexScreenRadius, distance))
        {
            rayIntersectionDistance = distance;
            return true;
        }

        return false;
    }

    bool IntersectRayPolygon(
        const PolygonBound& polygonBound, const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        float& rayIntersectionDistance)
    {
        AZ_Assert(polygonBound.m_triangles.size() % 3 == 0, "Invalid number of points to represent triangles");

        for (size_t i = 0; i < polygonBound.m_triangles.size(); i += 3)
        {
            AZ::Vector3 p0 = polygonBound.m_triangles[i];
            AZ::Vector3 p1 = polygonBound.m_triangles[i + 1];
            AZ::Vector3 p2 = polygonBound.m_triangles[i + 2];

            AZ::VectorFloat time;
            AZ::Vector3 normal;
            const float rayLength = 1000.0f;
            const int intersected = AZ::Intersect::IntersectSegmentTriangleCCW(
                rayOrigin, rayOrigin + rayDirection * rayLength, p0, p1, p2, normal, time);

            if (intersected != 0)
            {
                rayIntersectionDistance = time * rayLength;
                return true;
            }
        }

        return false;
    }

    bool ManipulatorBoundPolygon::IntersectRay(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        return WhiteBox::IntersectRayPolygon(m_polygonBound, rayOrigin, rayDirection, rayIntersectionDistance);
    }

    void ManipulatorBoundPolygon::SetShapeData(const AzToolsFramework::Picking::BoundRequestShapeBase& shapeData)
    {
        if (const auto* polygonData = azrtti_cast<const BoundShapePolygon*>(&shapeData))
        {
            m_polygonBound.m_triangles = polygonData->m_triangles;
        }
    }

    AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> BoundShapePolygon::MakeShapeInterface(
        AzToolsFramework::Picking::RegisteredBoundId id) const
    {
        AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> quad =
            AZStd::make_shared<ManipulatorBoundPolygon>(id);
        quad->SetShapeData(*this);
        return quad;
    }

    bool IntersectRayEdge(
        const EdgeBound& edgeBound, const float edgeScreenWidth, const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        const float rayLength = 1000.0f; // this is arbitrary to turn it into a ray
        const AZ::Vector3 sa = rayOrigin;
        const AZ::Vector3 sb = rayOrigin + rayDirection * rayLength;
        const AZ::Vector3 p = edgeBound.m_start;
        const AZ::Vector3 q = edgeBound.m_end;
        const float r = edgeScreenWidth;

        float t = 0.0f;
        if (IntersectSegmentCylinder(sa, sb, p, q, r, t) != 0)
        {
            // intersected, t is normalized distance along the ray
            rayIntersectionDistance = t * rayLength;
            return true;
        }

        return false;
    }

    bool ManipulatorBoundEdge::IntersectRay(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        return WhiteBox::IntersectRayEdge(
            m_edgeBound, m_edgeBound.m_radius, rayOrigin, rayDirection, rayIntersectionDistance);
    }

    void ManipulatorBoundEdge::SetShapeData(const AzToolsFramework::Picking::BoundRequestShapeBase& shapeData)
    {
        if (const auto* edgeData = azrtti_cast<const BoundShapeEdge*>(&shapeData))
        {
            m_edgeBound = {edgeData->m_start, edgeData->m_end, edgeData->m_radius};
        }
    }

    AZStd::shared_ptr<AzToolsFramework::Picking::BoundShapeInterface> BoundShapeEdge::MakeShapeInterface(
        AzToolsFramework::Picking::RegisteredBoundId id) const
    {
        AZStd::shared_ptr<ManipulatorBoundEdge> edge = AZStd::make_shared<ManipulatorBoundEdge>(id);
        edge->SetShapeData(*this);
        return edge;
    }
} // namespace WhiteBox
