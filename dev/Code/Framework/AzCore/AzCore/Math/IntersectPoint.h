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
#ifndef AZCORE_MATH_POINT_TESTS_H
#define AZCORE_MATH_POINT_TESTS_H

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Plane.h>

/// \file isect_point.h

namespace AZ
{
    /**
    * Intersect
    *
    * This namespace contains, primitive intersections and overlapping tests.
    * reference RealTimeCollisionDetection, CollisionDetection in interactive 3D environments, etc.
    */
    namespace Intersect
    {
        /**
         * Compute barycentric coordinates (u,v,w) for a point P with respect
         * to triangle (a,b,c)
         */
        AZ_MATH_FORCE_INLINE Vector3
        Barycentric(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& p)
        {
            Vector3 v0 = b - a;
            Vector3 v1 = c - a;
            Vector3 v2 = p - a;

            VectorFloat d00 = v0.Dot(v0);
            VectorFloat d01 = v0.Dot(v1);
            VectorFloat d11 = v1.Dot(v1);
            VectorFloat d20 = v2.Dot(v0);
            VectorFloat d21 = v2.Dot(v1);

            VectorFloat denom = d00 * d11 - d01 * d01;
            VectorFloat denomRCP = denom.GetReciprocal();
            VectorFloat v = (d11 * d20 - d01 * d21) * denomRCP;
            VectorFloat w = (d00 * d21 - d01 * d20) * denomRCP;
            VectorFloat u = VectorFloat::CreateOne() - v - w;
            return Vector3(u, v, w);
        }

        /// Test if point p lies inside the triangle (a,b,c).
        AZ_MATH_FORCE_INLINE int
        TestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            Vector3 uvw = Barycentric(a, b, c, p);
            //VectorFloat zero = VectorFloat::CreateZero();
            //return uvw.GetY().IsGreaterEqualThan(zero) && uvw.GetZ().IsGreaterEqualThan(zero) && /*(uvw.GetY()+uvw.GetZ()).IsLessEqualThan(VectorFloat::CreateOne())*/;
            return uvw.IsGreaterEqualThan(Vector3::CreateZero());
        }

        /// Test if point p lies inside the counter clock wise triangle (a,b,c).
        AZ_MATH_FORCE_INLINE int
        TestPointTriangleCCW(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            // Translate the triangle so it lies in the origin p.
            Vector3 pA = a - p;
            Vector3 pB = b - p;
            Vector3 pC = c - p;
            VectorFloat ab = pA.Dot(pB);
            VectorFloat ac = pA.Dot(pC);
            VectorFloat bc = pB.Dot(pC);
            VectorFloat cc = pC.Dot(pC);
            VectorFloat zero = VectorFloat::CreateZero();

            // Make sure plane normals for pab and pac point in the same direction.
            if ((bc * ac - cc * ab).IsLessThan(zero))
            {
                return 0;
            }

            VectorFloat bb = pB.Dot(pB);

            // Make sure plane normals for pab and pca point in the same direction.
            if ((ab * bc - ac * bb).IsLessThan(zero))
            {
                return 0;
            }

            // Otherwise p must be in the triangle.
            return 1;
        }

        /**
         * Find the closest point to 'p' on a plane 'plane'.
         * \return Distance from the 'p' to 'ptOnPlane'
         * \param ptOnPlane closest point on the plane.
         */
        AZ_MATH_FORCE_INLINE VectorFloat
        ClosestPointPlane(const Vector3& p, const Plane& plane, Vector3& ptOnPlane)
        {
            VectorFloat dist = plane.GetPointDist(p);
            // \todo add Vector3 from Vector4 quick function.
            ptOnPlane = p - dist * plane.GetNormal();

            return dist;
        }

        /**
         * Find the closest point to 'p' on a triangle (a,b,c).
         * \return closest point to 'p' on the triangle (a,b,c)
         */
        inline Vector3 ClosestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            // Check if p in vertex region is outside a.
            Vector3 ab = b - a;
            Vector3 ac = c - a;
            Vector3 ap = p - a;
            VectorFloat d1 = ab.Dot(ap);
            VectorFloat d2 = ac.Dot(ap);

            VectorFloat fltZero = VectorFloat::CreateZero();

            if (d1.IsLessEqualThan(fltZero) && d2.IsLessEqualThan(fltZero))
            {
                return a; // barycentric coordinates (1,0,0)
            }
            // Check if p in vertex region is outside b.
            Vector3 bp = p - b;
            VectorFloat d3 = ab.Dot(bp);
            VectorFloat d4 = ac.Dot(bp);
            if (d3.IsGreaterEqualThan(fltZero) && d4.IsLessEqualThan(d3))
            {
                return b; // barycentric coordinates (0,1,0)
            }
            // Check if p in edge region of ab, if so return projection of p onto ab.
            VectorFloat vc = d1 * d4 - d3 * d2;
            if (vc.IsLessEqualThan(fltZero) && d1.IsGreaterEqualThan(fltZero) && d3.IsLessEqualThan(fltZero))
            {
                VectorFloat v = d1 / (d1 - d3);
                return a + v * ab; // barycentric coordinates (1-v,v,0)
            }

            // Check if p in vertex region outside C.
            Vector3 cp = p - c;
            VectorFloat d5 = ab.Dot(cp);
            VectorFloat d6 = ac.Dot(cp);
            if (d6.IsGreaterEqualThan(fltZero) && d5.IsLessEqualThan(d6))
            {
                return c; // barycentric coordinates (0,0,1)
            }

            // Check if P in edge region of ac, if so return projection of p onto ac.
            VectorFloat vb = d5 * d2 - d1 * d6;
            if (vb.IsLessEqualThan(fltZero) && d2.IsGreaterEqualThan(fltZero) && d6.IsLessEqualThan(fltZero))
            {
                VectorFloat w = d2 / (d2 - d6);
                return a + w * ac; // barycentric coordinates (1-w,0,w)
            }

            // Check if p in edge region of bc, if so return projection of p onto bc.
            VectorFloat va = d3 * d6 - d5 * d4;
            if (va.IsLessEqualThan(fltZero) && (d4 - d3).IsGreaterEqualThan(fltZero) && (d5 - d6).IsGreaterEqualThan(fltZero))
            {
                VectorFloat w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                return b + w * (c - b);  // barycentric coordinates (0,1-w,w)
            }

            // p inside face region. Compute q trough its barycentric coordinates (u,v,w)
            VectorFloat denomRCP = (va + vb + vc).GetReciprocal();
            VectorFloat v = vb * denomRCP;
            VectorFloat w = vc * denomRCP;
            return a + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0 - v - w
        }

        /**
        * \brief This method checks if a point is inside a sphere or outside it
        * \param centerPosition Position of the center of the sphere
        * \param radiusSquared square of cylinder radius
        * \param testPoint Point to be tested
        * \return boolean value indicating if the point is inside or not
        */
        AZ_INLINE bool PointSphere(const Vector3& centerPosition,
            float radiusSquared, const Vector3& testPoint)
        {
            return testPoint.GetDistanceSq(centerPosition) < radiusSquared;
        }

        /**
        * \brief This method checks if a point is inside a cylinder or outside it
        * \param baseCenterPoint Vector to the base of the cylinder
        * \param axisVector Non normalized vector from the base of the cylinder to the other end
        * \param axisLengthSquared square of length of "axisVector"
        * \param radiusSquared square of cylinder radius
        * \param testPoint Point to be tested
        * \return boolean value indicating if the point is inside or not
        */
        AZ_INLINE bool PointCylinder(const AZ::Vector3& baseCenterPoint, const AZ::Vector3& axisVector,
            float axisLengthSquared, float radiusSquared, const AZ::Vector3& testPoint)
        {
            AZ::Vector3 baseCenterPointToTestPoint = testPoint - baseCenterPoint;
            float dotProduct = baseCenterPointToTestPoint.Dot(axisVector);

            // If the dot is < 0, the point is below the base cap of the cylinder, if it's > lengthSquared then it's beyond the other cap.
            if (dotProduct < 0.0f || dotProduct > axisLengthSquared)
            {
                return false;
            }
            else
            {
                float distanceSquared = (baseCenterPointToTestPoint.GetLengthSq()) - (dotProduct * dotProduct / axisLengthSquared);
                return distanceSquared <= radiusSquared;
            }
        }
    }
}

#endif // AZCORE_MATH_POINT_TESTS_H
#pragma once