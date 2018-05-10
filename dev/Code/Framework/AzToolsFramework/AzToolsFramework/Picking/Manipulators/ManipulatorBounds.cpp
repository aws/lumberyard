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

#include <AzToolsFramework/Picking/ContextBoundAPI.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBounds.h>
#include <AzCore/Math/IntersectSegment.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        bool ManipulatorBoundSphere::IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            AZ::VectorFloat vecRayIntersectionDistance;
            const int hits = AZ::Intersect::IntersectRaySphere(rayOrigin, rayDirection, m_center, AZ::VectorFloat(m_radius), vecRayIntersectionDistance);
            rayIntersectionDistance = vecRayIntersectionDistance;
            return hits > 0;
        }

        void ManipulatorBoundSphere::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeSphere* sphereData = azrtti_cast<const BoundShapeSphere*>(&shapeData))
            {
                m_center = sphereData->m_center;
                m_radius = sphereData->m_radius;
            }
        }

        bool ManipulatorBoundBox::IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            return AZ::Intersect::IntersectRayBox(rayOrigin, rayDirection, m_center, m_axis1, m_axis2, m_axis3,
                m_halfExtents.GetX(), m_halfExtents.GetY(), m_halfExtents.GetZ(), rayIntersectionDistance) > 0;
        }

        void ManipulatorBoundBox::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeBox* boxData = azrtti_cast<const BoundShapeBox*>(&shapeData))
            {
                m_center = boxData->m_center;
                m_axis1 = boxData->m_orientation * AZ::Vector3::CreateAxisX();
                m_axis2 = boxData->m_orientation * AZ::Vector3::CreateAxisY();
                m_axis3 = boxData->m_orientation * AZ::Vector3::CreateAxisZ();
                m_halfExtents = boxData->m_halfExtents;
            }
        }

        bool ManipulatorBoundCylinder::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            float t1 = 0.0f;
            float t2 = 0.0f;
            if (AZ::Intersect::IntersectRayCappedCylinder(
                rayOrigin, rayDirection, m_base, m_axis, m_height, m_radius, t1, t2) > 0)
            {
                rayIntersectionDistance = t1;
                return true;
            }

            return false;
        }

        void ManipulatorBoundCylinder::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeCylinder* cylinderData = azrtti_cast<const BoundShapeCylinder*>(&shapeData))
            {
                m_base = cylinderData->m_base;
                m_axis = cylinderData->m_axis;
                m_height = cylinderData->m_height;
                m_radius = cylinderData->m_radius;
            }
        }

        bool ManipulatorBoundCone::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            float t1 = 0.0f;
            float t2 = 0.0f;
            if (AZ::Intersect::IntersectRayCone(
                rayOrigin, rayDirection, m_apexPosition, m_dir, m_height, m_radius, t1, t2) > 0)
            {
                rayIntersectionDistance = t1;
                return true;
            }

            return false;
        }

        void ManipulatorBoundCone::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeCone* coneData = azrtti_cast<const BoundShapeCone*>(&shapeData))
            {
                m_apexPosition = coneData->m_base + coneData->m_axis * coneData->m_height;
                m_dir = -coneData->m_axis;
                m_height = coneData->m_height;
                m_radius = coneData->m_radius;
            }
        }

        bool ManipulatorBoundQuad::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            return AZ::Intersect::IntersectRayQuad(
                rayOrigin, rayDirection, m_corner1, m_corner2, m_corner3, m_corner4, rayIntersectionDistance) > 0;
        }

        void ManipulatorBoundQuad::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeQuad* quadData = azrtti_cast<const BoundShapeQuad*>(&shapeData))
            {
                m_corner1 = quadData->m_corner1;
                m_corner2 = quadData->m_corner2;
                m_corner3 = quadData->m_corner3;
                m_corner4 = quadData->m_corner4;
            }
        }

        bool ManipulatorBoundTorus::IntersectRay(
            const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            float t1 = 0.0f;
            float t2 = 0.0f;
            const AZ::Vector3 base = m_center - m_axis * m_minorRadius;
            const int hits = AZ::Intersect::IntersectRayCappedCylinder(
                rayOrigin, rayDirection, base, m_axis, m_minorRadius * 2.0f, m_majorRadius + m_minorRadius, t1, t2);

            if (hits > 0)
            {
                const AZ::Vector3 intersection1 = rayOrigin + t1 * rayDirection;
                float distanceSq;
                if (hits == 2)
                {
                    rayIntersectionDistance = AZ::GetMin(t1, t2);
                    const AZ::Vector3 intersection2 = rayOrigin + t2 * rayDirection;
                    const float distance1Sq = (intersection1 - m_center).GetLengthSq();
                    const float distance2Sq = (intersection2 - m_center).GetLengthSq();
                    distanceSq = distance1Sq > distance2Sq
                        ? distance1Sq
                        : distance2Sq;
                }
                else // hits == 1
                {
                    rayIntersectionDistance = t1;
                    distanceSq = (intersection1 - m_center).GetLengthSq();
                }

                const float thresholdSq =  powf(m_majorRadius - m_minorRadius, 2.0f);
                if (distanceSq < thresholdSq)
                {
                    return false;
                }

                return true;
            }

            return false;
        }

        void ManipulatorBoundTorus::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeTorus* torusData = azrtti_cast<const BoundShapeTorus*>(&shapeData))
            {
                m_center = torusData->m_center;
                m_axis = torusData->m_axis;
                m_minorRadius = torusData->m_minorRadius;
                m_majorRadius = torusData->m_majorRadius;
            }
        }

        bool ManipulatorBoundLineSegment::IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            const AZ::VectorFloat rayLength = AZ::VectorFloat(1000.0f);
            AZ::Vector3 closestPosRay, closestPosLineSegment;
            AZ::VectorFloat rayProportion, lineSegmentProportion;
            AZ::Intersect::ClosestSegmentSegment(rayOrigin, rayOrigin + rayDirection * rayLength, m_worldStart, m_worldEnd, rayProportion, lineSegmentProportion, closestPosRay, closestPosLineSegment);
            AZ::VectorFloat distanceFromLine = (closestPosRay - closestPosLineSegment).GetLength();
            // note: here out param is proportion/percentage of line - rayIntersectionDistance is expected to be distance so we must scale rayProportion by its length.
            rayIntersectionDistance = rayProportion * rayLength + distanceFromLine; // add distance from line to give more accurate rayIntersectionDistance value.
            return distanceFromLine.IsLessEqualThan(AZ::VectorFloat(m_width));
        }

        void ManipulatorBoundLineSegment::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeLineSegment* lineSegmentData = azrtti_cast<const BoundShapeLineSegment*>(&shapeData))
            {
                m_worldStart = lineSegmentData->m_start;
                m_worldEnd = lineSegmentData->m_end;
                m_width = lineSegmentData->m_width;
            }
        }

        bool ManipulatorBoundSpline::IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
        {
            if (const AZStd::shared_ptr<const AZ::Spline> spline = m_spline.lock())
            {
                AZ::RaySplineQueryResult splineQueryResult = AZ::IntersectSpline(m_transform, rayOrigin, rayDirection, *spline);
                rayIntersectionDistance = splineQueryResult.m_rayDistance;
                return splineQueryResult.m_distanceSq.IsLessEqualThan(AZ::VectorFloat(powf(m_width, 2.0f)));
            }

            return false;
        }

        void ManipulatorBoundSpline::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeSpline* splineData = azrtti_cast<const BoundShapeSpline*>(&shapeData))
            {
                m_spline = splineData->m_spline;
                m_transform = splineData->m_transform;
                m_width = splineData->m_width;
            }
        }
    } // namespace Picking
} // namespace AzToolsFramework