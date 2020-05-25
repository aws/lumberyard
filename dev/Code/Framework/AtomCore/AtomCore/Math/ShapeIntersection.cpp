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
        /// Tests to see if Arg1 overlaps Arg2. Symmetric.
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

        bool Overlaps(Sphere sphere, Obb obb)
        {
            AZ_Assert(false, "Not implemented yet");
            AZ_UNUSED(sphere);
            AZ_UNUSED(obb);
            return false;
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

        bool Overlaps(Frustum frustum, Plane plane)
        {
            AZ_Assert(false, "Not implemented yet");
            AZ_UNUSED(frustum);
            AZ_UNUSED(plane);
            return false;
        }

        //P/N testing with center/extents
        bool Overlaps(Frustum frustum, Aabb aabb)
        {
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

        bool Overlaps(Frustum frustum1, Frustum frustum2)
        {
            AZ_Assert(false, "Not impemented yet");
            AZ_UNUSED(frustum1);
            AZ_UNUSED(frustum2);
            
            return false;
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

        bool Contains(Sphere sphere, Frustum frustum)
        {
            AZ_Assert(false, "Not implemented yet");
            AZ_UNUSED(sphere);
            AZ_UNUSED(frustum);
            return false;
        }

        bool Contains(Sphere sphere, Obb obb)
        {
            AZ_Assert(false, "Not implemented yet");
            AZ_UNUSED(sphere);
            AZ_UNUSED(obb);
            return false;
        }

        bool Contains(Sphere sphere1, Sphere sphere2)
        {
            AZ::VectorFloat radiusDiff = sphere1.GetRadius() - sphere2.GetRadius();
            return sphere1.GetCenter().GetDistanceSq(sphere2.GetCenter()) <= (radiusDiff * radiusDiff);
        }

        //P/N testing with center/extents
        bool Contains(Frustum frustum, Aabb aabb)
        {
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

        bool Contains(Frustum frustum, Obb obb)
        {
            AZ_Assert(false, "Not implemented yet.");
            AZ_UNUSED(frustum);
            AZ_UNUSED(obb);
            return false;
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

        bool Contains(Frustum frustum1, Frustum frustum2)
        {
            AZ_Assert(false, "Not implemented yet.");
            AZ_UNUSED(frustum1);
            AZ_UNUSED(frustum2);
            return false;
        }


    }//ShapeIntersection
}//AZ