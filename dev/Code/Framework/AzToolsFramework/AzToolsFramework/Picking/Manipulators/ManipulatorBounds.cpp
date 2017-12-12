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
        bool ManipulatorBoundBox::IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t)
        {
            int hits = AZ::Intersect::IntersectRayBox(rayOrigin, rayDir, m_center, m_axis1, m_axis2, m_axis3,
                m_halfExtents.GetX(), m_halfExtents.GetY(), m_halfExtents.GetZ(), t);
            return (hits > 0);
        }

        void ManipulatorBoundBox::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeBox* quadData = azrtti_cast<const BoundShapeBox*>(&shapeData))
            {
                m_center = quadData->m_transform.GetPosition();
                m_axis1 = quadData->m_transform.GetBasisX();
                m_axis2 = quadData->m_transform.GetBasisY();
                m_axis3 = quadData->m_transform.GetBasisZ();
                m_halfExtents = quadData->m_halfExtents;
            }
        }

        bool ManipulatorBoundCylinder::IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t)
        {
            float t1 = 0.0f;
            float t2 = 0.0f;
            int hits = AZ::Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_end1, m_dir, m_height, m_radius, t1, t2);
            if (hits > 0)
            {
                t = t1;
                return true;
            }
            else
            {
                return false;
            }
        }

        void ManipulatorBoundCylinder::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeCylinder* cylinderData = azrtti_cast<const BoundShapeCylinder*>(&shapeData))
            {
                AZ::Vector3 pos = cylinderData->m_transform.GetPosition();
                AZ::Vector3 axisZ = cylinderData->m_transform.GetBasisZ();

                m_end1 = pos;
                m_dir = axisZ;
                m_height = cylinderData->m_height;
                m_radius = cylinderData->m_radius;
            }
        }

        bool ManipulatorBoundCone::IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t)
        {
            float t1 = 0.0f;
            float t2 = 0.0f;
            int hits = AZ::Intersect::IntersectRayCone(rayOrigin, rayDir, m_apexPosition, m_dir, m_height, m_radius, t1, t2);
            if (hits > 0)
            {
                t = t1;
                return true;
            }
            else
            {
                return false;
            }
        }

        void ManipulatorBoundCone::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeCone* coneData = azrtti_cast<const BoundShapeCone*>(&shapeData))
            {
                AZ::Vector3 pos = coneData->m_transform.GetPosition();
                AZ::Vector3 axisZ = coneData->m_transform.GetBasisZ();

                m_apexPosition = pos;
                m_dir = axisZ;
                m_height = coneData->m_height;
                m_radius = coneData->m_radius;
            }
        }

        bool ManipulatorBoundQuad::IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t)
        {
            int hits = AZ::Intersect::IntersectRayQuad(rayOrigin, rayDir, m_corner1, m_corner2, m_corner3, m_corner4, t);
            return (hits > 0);
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

        bool ManipulatorBoundTorus::IntersectRay(const AZ::Vector3 &rayOrigin, const AZ::Vector3 &rayDir, float &t)
        {
            float t1 = 0.0f;
            float t2 = 0.0f;
            int hits = AZ::Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_end1, m_dir, m_height, m_radius, t1, t2);
            if (hits > 0)
            {
                t = t1;

                AZ::Vector3 cylinderCenter = m_end1 + 0.5f * m_height * m_dir;
                AZ::Vector3 intersection1 = rayOrigin + t1 * rayDir;
                float distanceSqr = 0.0f;
                if (hits == 2)
                {
                    AZ::Vector3 intersection2 = rayOrigin + t2 * rayDir;
                    float distance1Sqr = (intersection1 - cylinderCenter).GetLengthSq();
                    float distance2Sqr = (intersection2 - cylinderCenter).GetLengthSq();
                    distanceSqr = (distance1Sqr > distance2Sqr ? distance1Sqr : distance2Sqr);
                }
                else // hits == 1
                {
                    distanceSqr = (intersection1 - cylinderCenter).GetLengthSq();
                }
                float threshold = m_radius - 2.0f * m_height;
                threshold *= threshold;
                if (distanceSqr > threshold)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        void ManipulatorBoundTorus::SetShapeData(const BoundRequestShapeBase& shapeData)
        {
            if (const BoundShapeTorus* torusData = azrtti_cast<const BoundShapeTorus*>(&shapeData))
            {
                AZ::Vector3 pos = torusData->m_transform.GetPosition();
                AZ::Vector3 axisZ = torusData->m_transform.GetBasisZ();

                m_end1 = pos;
                m_dir = axisZ;
                m_height = torusData->m_height;
                m_radius = torusData->m_radius;
            }
        }
    } // namespace Picking
} // namespace AzToolsFramework