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
#ifndef AZCORE_MATH_SEGMENT_INTERSECTION_H
#define AZCORE_MATH_SEGMENT_INTERSECTION_H

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Plane.h>

/// \file isect_segment.h

namespace AZ
{
    namespace Intersect
    {
        /**
         * LineToPointDistancTime computes the time of the shortest distance from point 'p' to segment (s1,s2)
         * To calculate the point of intersection:
         * P = s1 + u (s2 - s1)
         * \param s1 segment start point
         * \param s2 segment end point
         * \param p point to find the closest time to.
         * \return time (on the segment) for the shortest distance from 'p' to (s1,s2)  [0.0f (s1),1.0f (s2)]
         */
        AZ_MATH_FORCE_INLINE VectorFloat    LineToPointDistanceTime(const Vector3& s1, const Vector3& s21, const Vector3& p)
        {
            Vector3 ps1 = p - s1;
            // so u = (p.x - s1.x)*(s2.x - s1.x) + (p.y - s1.y)*(s2.y - s1.y) + (p.z-s1.z)*(s2.z-s1.z) /  |s2-s1|^2
            return s21.Dot(ps1) / s21.Dot(s21);
        }

        /**
         * LineToPointDistance computes the closest point to 'p' from a segment (s1,s2).
         * \param s1 segment start point
         * \param s2 segment end point
         * \param p point to find the closest time to.
         * \param u time (on the segment) for the shortest distance from 'p' to (s1,s2)  [0.0f (s1),1.0f (s2)]
         * \return the closest point
         */
        AZ_MATH_FORCE_INLINE Vector3 LineToPointDistance(const Vector3& s1, const Vector3& s2, const Vector3& p, VectorFloat& u)
        {
            Vector3 s21 = s2 - s1;
            // we assume seg1 and seg2 are NOT coincident
            AZ_Assert(!s21.IsClose(Vector3(0.0f), 1e-4f), "OK we agreed that we will pass valid segments! (s1 != s2)");

            u = LineToPointDistanceTime(s1, s21, p);

            return s1 + u * s21;
        }

        //enum SegmentTriangleIsectTypes
        //{
        //  ISECT_TRI_SEGMENT_NONE = 0, // no intersection
        //  ISECT_TRI_SEGMENT_PARALLEL, // The line is parallel to the triangle.
        //  ISECT_TRI_SEGMENT_ISECT,    // along the PQ segment
        //};

        /**
         * Given segment pq and triangle abc (CCW), returns whether segment intersects
         * triangle and if so, also returns the barycentric coordinates (u,v,w)
         * of the intersection point
         * \param p segment start point
         * \param q segment end point
         * \param a triangle point 1
         * \param b triangle point 2
         * \param c triangle point 3
         * \param normal at the intersection point.
         * \param t time of intersection along the segment [0.0 (p), 1.0 (q)]
         * \return 1 if the segment intersects the triangle otherwise 0
         */
        int IntersectSegmentTriangleCCW(const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
            /*float &u, float &v, float &w,*/ Vector3& normal, VectorFloat& t);

        /**
         * Same as \ref IntersectSegmentTriangleCCW without respecting the trianlge (a,b,c) vertex order (double sided).
         */
        int IntersectSegmentTriangle(const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
            /*float &u, float &v, float &w,*/ Vector3& normal, VectorFloat& t);

        /// Ray aabb intersection result types.
        enum RayAABBIsectTypes
        {
            ISECT_RAY_AABB_NONE = 0,    ///< no intersection
            ISECT_RAY_AABB_SA_INSIDE,   ///< the ray starts inside the aabb
            ISECT_RAY_AABB_ISECT,       ///< intersects along the PQ segment
        };

        /**
         * Intersect ray R(t) = rayStart + t*d against AABB a. When intersecting,
         * return intersection distance tmin and point q of intersection.
         * \param rayStart ray starting point
         * \param dir ray direction and length (dir = rayEnd - rayStart)
         * \param dirRCP 1/dir (reciprocal direction - we cache this result very often so we don't need to compute it multiple times, otherwise just use dir.GetReciprocal())
         * \param aabb Axis aligned bounding box to intersect against
         * \param tStart time on ray of the first intersection [0,1] or 0 if the ray starts inside the aabb - check the return value
         * \param tEnd time of the of the second intersection [0,1] (it can be > 1 if intersects after the rayEnd)
         * \param startNormal normal at the start point.
         * \return \ref RayAABBIsectTypes
         */
        int IntersectRayAABB(const Vector3& rayStart, const Vector3& dir, const Vector3& dirRCP, const Aabb& aabb, VectorFloat& tStart, VectorFloat& tEnd, Vector3& startNormal /*, Vector3& inter*/);

        /**
         * Intersect ray against AABB.
         * \param rayStart ray starting point.
         * \param dir ray reciprocal direction.
         * \param aabb Axis aligned bounding box to intersect against.
         * \param start length on ray of the first intersection.
         * \param end length of the of the second intersection.
         * \return \ref RayAABBIsectTypes In this faster version than IntersectRayAABB we return only ISECT_RAY_AABB_NONE and ISECT_RAY_AABB_ISECT.
         * You can check yourself for that case.
         */
        int IntersectRayAABB2(const Vector3& rayStart, const Vector3& dirRCP, const Aabb& aabb, VectorFloat& start, VectorFloat& end);

        /**
         * Clip a ray to an aabb. return true if ray was clipped. The ray
         * can be inside so don't use the result if the ray intersect the box.
         */
        AZ_MATH_FORCE_INLINE int ClipRayWithAabb(const Aabb& aabb, Vector3& rayStart, Vector3& rayEnd, VectorFloat& tClipStart, VectorFloat& tClipEnd)
        {
            Vector3 startNormal;
            VectorFloat tStart, tEnd;
            Vector3 dirLen = rayEnd - rayStart;
            if (IntersectRayAABB(rayStart, dirLen, dirLen.GetReciprocal(), aabb, tStart, tEnd, startNormal) != ISECT_RAY_AABB_NONE)
            {
                // clip the ray with the box
                if (tStart.IsGreaterThan(VectorFloat::CreateZero()))
                {
                    rayStart = rayStart + tStart * dirLen;
                    tClipStart = tStart;
                }
                if (tEnd.IsLessThan(VectorFloat::CreateOne()))
                {
                    rayEnd = rayStart + tEnd * dirLen;
                    tClipEnd = tEnd;
                }

                return 1;
            }

            return 0;
        }

        /**
         * Test segment and aabb where the segment is defined by midpoint
         * midPoint = (p1-p0) * 0.5f and half vector halfVector = p1 - midPoint
         * the aabb is at the origin and defined by half extents only.
         * \return 1 if the intersect, otherwise 0
         */
        int TestSegmentAABBOrigin(const Vector3& midPoint, const Vector3& halfVector, const Vector3& aabbExtends);

        /**
         * Test if segment specified by points p0 and p1 intersects AABB. \ref TestSegmentAABBOrigin
         * \return 1 if the intersect, otherwise 0
         */
        AZ_MATH_FORCE_INLINE int TestSegmentAABB(const Vector3& p0, const Vector3& p1, const Aabb& aabb)
        {
            Vector3 e = aabb.GetExtents();
            Vector3 d = p1 - p0;
            Vector3 m = p0 + p1 - aabb.GetMin() - aabb.GetMax();

            return TestSegmentAABBOrigin(m, d, e);
        }

        /// Ray sphere intersection result types.
        enum SphereIsectTypes
        {
            ISECT_RAY_SPHERE_SA_INSIDE = -1, // the ray starts inside the cylinder
            ISECT_RAY_SPHERE_NONE,      // no intersection
            ISECT_RAY_SPHERE_ISECT,     // along the PQ segment
        };

        /**
         * IntersectRaySphereOrigin
         * return time t>=0  but not limited, so if you check a segment make sure
         * t <= segmentLen
         * \param rayStart ray start point
         * \param rayDirNormalized ray direction normalized.
         * \param shereRadius sphere radius
         * \param time of closest intersection [0,+INF] in relation to the normalized direction.
         * \return \ref SphereIsectTypes
         */
        AZ_INLINE int IntersectRaySphereOrigin(const Vector3& rayStart, const Vector3& rayDirNormalized, const VectorFloat& sphereRadius, VectorFloat& t)
        {
            Vector3 m = rayStart;
            VectorFloat b = m.Dot(rayDirNormalized);
            VectorFloat c = m.Dot(m) - sphereRadius * sphereRadius;
            VectorFloat zero = VectorFloat::CreateZero();

            // Exit if r's origin outside s (c > 0)and r pointing away from s (b > 0)
            if (c.IsGreaterThan(zero) && b.IsGreaterThan(zero))
            {
                return ISECT_RAY_SPHERE_NONE;
            }
            VectorFloat discr = b * b - c;
            // A negative discriminant corresponds to ray missing sphere
            if (discr.IsLessThan(zero))
            {
                return ISECT_RAY_SPHERE_NONE;
            }

            // Ray now found to intersect sphere, compute smallest t value of intersection
            t = -b - sqrtf(discr);

            // If t is negative, ray started inside sphere so clamp t to zero
            if (t.IsLessThan(zero))
            {
                //  t = 0.0f;
                return ISECT_RAY_SPHERE_SA_INSIDE; // no hit if inside
            }
            //q = p + t * d;
            return ISECT_RAY_SPHERE_ISECT;
        }

        /// Intersect ray (rayStart,rayDirNormalized) and sphere (sphereCenter,sphereRadius) \ref IntersectRaySphereOrigin
        AZ_MATH_FORCE_INLINE int IntersectRaySphere(const Vector3& rayStart, const Vector3& rayDirNormalized, const Vector3& sphereCenter, const VectorFloat& sphereRadius, VectorFloat& t)
        {
            Vector3 m = rayStart - sphereCenter;

            return IntersectRaySphereOrigin(m, rayDirNormalized, sphereRadius, t);
        }

        /// Ray cylinder intersection types.
        enum CylinderIsectTypes
        {
            RR_ISECT_RAY_CYL_SA_INSIDE = -1, // the ray starts inside the cylinder
            RR_ISECT_RAY_CYL_NONE,  // no intersection
            RR_ISECT_RAY_CYL_PQ,        // along the PQ segment
            RR_ISECT_RAY_CYL_P_SIDE,    // on the P side
            RR_ISECT_RAY_CYL_Q_SIDE,    // on the Q side
        };

        /// Intersect segment S(t)=sa+t(dir), 0<=t<=1 against cylinder specified by p, q and r
        int IntersectSegmentCylinder(const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const VectorFloat& r, VectorFloat& t);

        /// Capsule ray intersect types.
        enum CapsuleIsectTypes
        {
            ISECT_RAY_CAPSULE_SA_INSIDE = -1, // the ray starts inside the cylinder
            ISECT_RAY_CAPSULE_NONE, // no intersection
            ISECT_RAY_CAPSULE_PQ,       // along the PQ segment
            ISECT_RAY_CAPSULE_P_SIDE,   // on the P side
            ISECT_RAY_CAPSULE_Q_SIDE,   // on the Q side
        };

        /**
        *  This is quick implementation of segment capsule based on segment cylinder \ref IntersectSegmentCylinder
        *  segment sphere intersection. We can optimize it a lot once we fix the ray
        *  cylinder intersection.
        */
        int IntersectSegmentCapsule(const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const VectorFloat& r, VectorFloat& t);

        /**
         * Intersect segment S(t)=A+t(B-A), 0<=t<=1 against convex polyhedron specified
         * by the n halfspaces defined by the planes p[]. On exit tfirst and tlast
         * define the intersection, if any.
         */
        int IntersectSegmentPolyhedron(const Vector3& sa, const Vector3& sBA, const Plane p[], int numPlanes,
            VectorFloat& tfirst, VectorFloat& tlast, int& iFirstPlane, int& iLastPlane);

        /**
        * Calculate the line segment PaPb that is the shortest route between
        * two segments s1s2 and s3s4. Calculate also the values of ua and ub where
        * Pa = s1 + ua (s2 - s1)
        * Pb = s3 + ub (s4 - s3)
        * If segments are parallel returns a solution.
        */
        void ClosestSegmentSegment(const Vector3& s1, const Vector3& s2, const Vector3& s3, const Vector3& s4, Vector3& v1, Vector3& v2);
    }
}

#endif // AZCORE_MATH_SEGMENT_INTERSECTION_H
#pragma once