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
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Vector3.h>
#include <AtomCore/Math/Sphere.h>
#include <AtomCore/Math/Frustum.h>
#include <AtomCore/Math/ShapeIntersection.h>

namespace AZ
{
    namespace ShapeIntersection
    {
        PlaneClassification Classify(Plane plane, Sphere sphere)
        {
            float distance = plane.GetPointDist(sphere.GetCenter());
            float radius = sphere.GetRadius();
            if(distance < -radius)
            {
                return Behind;
            }
            else if(distance > radius)
            {
                return InFront;
            }
            else
            {
                return Intersects;
            }
        }

        PlaneClassification Classify(Plane plane, Obb obb)
        {
            //compute the projection interval radius onto the plane normal, then compute the distance to the plane and see if it's outside the interval.
            //d = distance of box center to the plane
            //r = projection interval radius of the obb (projected onto the plane normal)

            float d = plane.GetPointDist(obb.GetPosition());
            float r = obb.GetHalfLengthX() * plane.GetNormal().Dot(obb.GetAxisX()).GetAbs() +
                obb.GetHalfLengthY() * plane.GetNormal().Dot(obb.GetAxisY()).GetAbs() +
                obb.GetHalfLengthZ() * plane.GetNormal().Dot(obb.GetAxisZ()).GetAbs();
            if (d < -r)
            {
                return Behind;
            }
            else if(d > r)
            {
                return InFront;
            }
            else
            {
                return Intersects;
            }
        }

        FrustumClassification Classify(Frustum frustum, Sphere sphere)
        {
            bool touchesAnyPlanes = false;
            PlaneClassification c;            
            c = Classify(frustum.GetNearPlane(), sphere);
            if(c == Behind)
            {
                return Outside;
            }
            else if(c == Intersects)
            {
                touchesAnyPlanes = true;
            }

            c = Classify(frustum.GetFarPlane(), sphere);
            if (c == Behind)
            {
                return Outside;
            }
            else if (c == Intersects)
            {
                touchesAnyPlanes = true;
            }

            c = Classify(frustum.GetLeftPlane(), sphere);
            if (c == Behind)
            {
                return Outside;
            }
            else if (c == Intersects)
            {
                touchesAnyPlanes = true;
            }

            c = Classify(frustum.GetRightPlane(), sphere);
            if (c == Behind)
            {
                return Outside;
            }
            else if (c == Intersects)
            {
                touchesAnyPlanes = true;
            }

            c = Classify(frustum.GetTopPlane(), sphere);
            if (c == Behind)
            {
                return Outside;
            }
            else if (c == Intersects)
            {
                touchesAnyPlanes = true;
            }

            c = Classify(frustum.GetBottomPlane(), sphere);
            if (c == Behind)
            {
                return Outside;
            }
            else if (c == Intersects)
            {
                touchesAnyPlanes = true;
            }

            if(touchesAnyPlanes)
            {
                return Touches;
            }
            else
            {
                return Inside;
            }
        }

        bool Overlaps(Sphere sphere, Aabb aabb)
        {
            AZ::VectorFloat distSq = aabb.GetDistanceSq(sphere.GetCenter());
            AZ::VectorFloat radiusSq = sphere.GetRadius() * sphere.GetRadius();
            return distSq <= radiusSq;
        }

        bool Overlaps(Sphere sphere, Frustum frustum)
        {
            AZ::Vector4 radius = AZ::Vector4(sphere.GetRadius());

            AZ::Vector4 dists1 = AZ::Vector4(
                frustum.GetNearPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetFarPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetLeftPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetRightPlane().GetPointDist(sphere.GetCenter())
            ) + radius;

            AZ::Vector4 dists2 = AZ::Vector4(
                frustum.GetTopPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetBottomPlane().GetPointDist(sphere.GetCenter()),
                AZ::VectorFloat::CreateZero(),
                AZ::VectorFloat::CreateZero()
            ) + radius;

            return  dists1.IsGreaterEqualThan(AZ::Vector4::CreateZero()) &&
                    dists2.IsGreaterEqualThan(AZ::Vector4::CreateZero());
        }

        bool Overlaps(Sphere sphere, Plane plane)
        {
            AZ::VectorFloat dist = plane.GetPointDist(sphere.GetCenter());
            return dist * dist <= sphere.GetRadius() * sphere.GetRadius();
        }

        bool Overlaps(Sphere sphere1, Sphere sphere2)
        {
            AZ::VectorFloat radiusSum = sphere1.GetRadius() + sphere2.GetRadius();
            return sphere1.GetCenter().GetDistanceSq(sphere2.GetCenter()) <= (radiusSum * radiusSum);
        }

        bool Overlaps(Frustum frustum, Aabb aabb)
        {
            //For an AABB, extents.Dot(planeAbs) computes the projection interval radius of the AABB onto the plane normal.
            //So for each plane, we can test compare the center-to-plane distance to this interval to see which side of the plane the AABB is on.
            //The AABB is not overlapping if it is fully behind any of the planes, otherwise it is overlapping.

            AZ::Vector3 center          = aabb.GetCenter();
            AZ::Vector3 extents         = 0.5f * aabb.GetExtents();

            AZ::Vector4 results1 = AZ::Vector4::CreateZero();
            AZ::Vector4 results2 = AZ::Vector4::CreateOne();

            AZ::Plane plane = frustum.GetNearPlane();
            AZ::Vector3 planeAbs = plane.GetNormal().GetAbs();
            results1.SetX(plane.GetPointDist(center) + extents.Dot(planeAbs));

            plane = frustum.GetFarPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results1.SetY(plane.GetPointDist(center) + extents.Dot(planeAbs));

            plane = frustum.GetLeftPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results1.SetZ(plane.GetPointDist(center) + extents.Dot(planeAbs));

            plane = frustum.GetRightPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results1.SetW(plane.GetPointDist(center) + extents.Dot(planeAbs));

            plane = frustum.GetTopPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results2.SetX(plane.GetPointDist(center) + extents.Dot(planeAbs));

            plane = frustum.GetBottomPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results2.SetY(plane.GetPointDist(center) + extents.Dot(planeAbs));
            
            return  results1.IsGreaterThan(AZ::Vector4::CreateZero()) &&
                    results2.IsGreaterThan(AZ::Vector4::CreateZero());
        }        

        bool Overlaps(Frustum frustum, Obb obb)
        {
            //Note: the order of these tests can affect overall frustum culling performance.
            if(Classify(frustum.GetNearPlane(), obb) == Behind)
            {
                return false;
            }

            if(Classify(frustum.GetFarPlane(), obb) == Behind)
            {
                return false;
            }

            if(Classify(frustum.GetLeftPlane(), obb) == Behind)
            {
                return false;
            }

            if(Classify(frustum.GetRightPlane(), obb) == Behind)
            {
                return false;
            }

            if(Classify(frustum.GetTopPlane(), obb) == Behind)
            {
                return false;
            }

            if(Classify(frustum.GetBottomPlane(), obb) == Behind)
            {
                return false;
            }

            return true;
        }

        /// Tests to see if Arg1 contains Arg2. Non Symmetric.
        bool Contains(Sphere sphere, Aabb aabb)
        {
            AZ::VectorFloat maxDistSq = sphere.GetCenter().GetDistanceSq(aabb.GetMax());
            AZ::VectorFloat minDistSq = sphere.GetCenter().GetDistanceSq(aabb.GetMin());
            AZ::VectorFloat radiusSq = sphere.GetRadius() * sphere.GetRadius();

            return maxDistSq <= radiusSq && minDistSq <= radiusSq;
        }

        bool Contains(Sphere sphere, Vector3 point)
        {
            AZ::VectorFloat distSq = sphere.GetCenter().GetDistanceSq(point);
            AZ::VectorFloat radiusSq = sphere.GetRadius() * sphere.GetRadius();
            return distSq <= radiusSq;
        }

        bool Contains(Sphere sphere1, Sphere sphere2)
        {
            AZ::VectorFloat radiusDiff = sphere1.GetRadius() - sphere2.GetRadius();
            return sphere1.GetCenter().GetDistanceSq(sphere2.GetCenter()) <= (radiusDiff * radiusDiff);
        }

        //P/N testing with center/extents
        bool Contains(Frustum frustum, Aabb aabb)
        {
            //For an AABB, extents.Dot(planeAbs) computes the projection interval radius of the AABB onto the plane normal.
            //So for each plane, we can test compare the center-to-plane distance to this interval to see which side of the plane the AABB is on.
            //The AABB is contained if it is fully in front of all of the planes.

            AZ::Vector3 center = aabb.GetCenter();
            AZ::Vector3 extents = 0.5f * aabb.GetExtents();

            AZ::Vector4 results1 = AZ::Vector4::CreateZero();
            AZ::Vector4 results2 = AZ::Vector4::CreateZero();

            AZ::Plane plane = frustum.GetNearPlane();
            AZ::Vector3 planeAbs = plane.GetNormal().GetAbs();
            results1.SetX(plane.GetPointDist(center) - extents.Dot(planeAbs));

            plane = frustum.GetFarPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results1.SetY(plane.GetPointDist(center) - extents.Dot(planeAbs));

            plane = frustum.GetLeftPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results1.SetZ(plane.GetPointDist(center) - extents.Dot(planeAbs));

            plane = frustum.GetRightPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results1.SetW(plane.GetPointDist(center) - extents.Dot(planeAbs));

            plane = frustum.GetTopPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results2.SetX(plane.GetPointDist(center) - extents.Dot(planeAbs));

            plane = frustum.GetBottomPlane();
            planeAbs = plane.GetNormal().GetAbs();
            results2.SetY(plane.GetPointDist(center) - extents.Dot(planeAbs));

            return  results1.IsGreaterEqualThan(AZ::Vector4::CreateZero()) &&
                    results2.IsGreaterEqualThan(AZ::Vector4::CreateZero());
        }

        bool Contains(Frustum frustum, Sphere sphere)
        {
            AZ::Vector4 radius = AZ::Vector4(sphere.GetRadius());

            AZ::Vector4 dists1 = AZ::Vector4(
                frustum.GetNearPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetFarPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetLeftPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetRightPlane().GetPointDist(sphere.GetCenter())
            ) - radius;

            AZ::Vector4 dists2 = AZ::Vector4(
                frustum.GetTopPlane().GetPointDist(sphere.GetCenter()),
                frustum.GetBottomPlane().GetPointDist(sphere.GetCenter()),
                sphere.GetRadius(),
                sphere.GetRadius()
            ) - radius;

            return  dists1.IsGreaterEqualThan(AZ::Vector4::CreateZero()) &&
                    dists2.IsGreaterEqualThan(AZ::Vector4::CreateZero());
        }

        bool Contains(Frustum frustum, Vector3 point)
        {
            AZ::Vector4 dists1 = AZ::Vector4(
                frustum.GetNearPlane().GetPointDist(point),
                frustum.GetFarPlane().GetPointDist(point),
                frustum.GetLeftPlane().GetPointDist(point),
                frustum.GetRightPlane().GetPointDist(point)
            );

            AZ::Vector4 dists2 = AZ::Vector4(
                frustum.GetTopPlane().GetPointDist(point),
                frustum.GetBottomPlane().GetPointDist(point),
                AZ::VectorFloat::CreateZero(),
                AZ::VectorFloat::CreateZero()
            );

            return  dists1.IsGreaterEqualThan(AZ::Vector4::CreateZero()) &&
                dists2.IsGreaterEqualThan(AZ::Vector4::CreateZero());
        }

    }//ShapeIntersection
}//AZ
