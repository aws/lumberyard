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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Math/IntersectSegment.h>

using namespace AZ;
using namespace AZ::Intersect;

//=========================================================================
// IntersectSegmentTriangleCCW
// [10/21/2009]
//=========================================================================
int IntersectSegmentTriangleCCW(const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
    /*float &u, float &v, float &w,*/ Vector3& normal, VectorFloat& t)
{
    VectorFloat v, w; // comment this and enable input params if we need the barycentric coordinates
    const VectorFloat zero = VectorFloat::CreateZero();

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 qp = p - q;

    // Compute triangle normal. Can be pre-calculated/cached if
    // intersecting multiple segments against the same triangle
    normal = ab.Cross(ac);  // Right hand CCW

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    VectorFloat d = qp.Dot(normal);
    if (d.IsLessEqualThan(zero))
    {
        return 0;
    }

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    Vector3 ap = p - a;
    t = ap.Dot(normal);

    // range segment check t[0,1] (it this case [0,d])
    if (t.IsLessThan(zero) || t.IsGreaterThan(d))
    {
        return 0;
    }

    // Compute barycentric coordinate components and test if within bounds
    Vector3 e = qp.Cross(ap);
    v = ac.Dot(e);
    if (v.IsLessThan(zero) || v.IsGreaterThan(d))
    {
        return 0;
    }
    w = -ab.Dot(e);
    if (w.IsLessThan(zero) || (v + w).IsGreaterThan(d))
    {
        return 0;
    }

    // Segment/ray intersects triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    VectorFloat ood = d.GetReciprocal();
    t *= ood;
    /*v *= ood;
    w *= ood;
    u = 1.0f - v - w;*/

    normal.Normalize();

    return 1;
}

//=========================================================================
// IntersectSegmentTriangle
// [10/21/2009]
//=========================================================================
int
AZ::Intersect::IntersectSegmentTriangle(const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
    /*float &u, float &v, float &w,*/ Vector3& normal, VectorFloat& t)
{
    VectorFloat v, w; // comment this and enable input params if we need the barycentric coordinates
    const VectorFloat zero = VectorFloat::CreateZero();

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 qp = p - q;
    Vector3 ap = p - a;

    // Compute triangle normal. Can be pre-calculated or cached if
    // intersecting multiple segments against the same triangle
    normal = ab.Cross(ac);  // Right hand CCW

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    VectorFloat d = qp.Dot(normal);
    Vector3 e;
    if (d.IsGreaterThan(g_fltEps))
    {
        // the normal is on the right side
        e = qp.Cross(ap);
    }
    else
    {
        normal = -normal;

        // so either have a parallel ray or our normal is flipped
        if (d.IsGreaterEqualThan(-g_fltEps))
        {
            return 0; // parallel
        }
        d = -d;
        e = ap.Cross(qp);
    }

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    t = ap.Dot(normal);

    // range segment check t[0,1] (it this case [0,d])
    if (t.IsLessThan(zero) || t.IsGreaterThan(d))
    {
        return 0;
    }

    // Compute barycentric coordinate components and test if within bounds
    v = ac.Dot(e);
    if (v.IsLessThan(zero) || v.IsGreaterThan(d))
    {
        return 0;
    }
    w = -ab.Dot(e);
    if (w.IsLessThan(zero) || (v + w).IsGreaterThan(d))
    {
        return 0;
    }

    // Segment/ray intersects the triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    VectorFloat ood = d.GetReciprocal();
    t *= ood;
    //v *= ood;
    //w *= ood;
    //u = 1.0f - v - w;

    normal.Normalize();

    return 1;
}

//=========================================================================
// TestSegmentAABBOrigin
// [10/21/2009]
//=========================================================================
int
AZ::Intersect::TestSegmentAABBOrigin(const Vector3& midPoint, const Vector3& halfVector, const Vector3& aabbExtends)
{
    const Vector3 EPSILON(0.001f); // \todo this is slow load move to a const
    Vector3 absHalfVector = halfVector.GetAbs();
    Vector3 absMidpoint = midPoint.GetAbs();
    Vector3 absHalfMidpoint = absHalfVector + aabbExtends;

    // Try world coordinate axes as separating axes
    if (!absMidpoint.IsLessEqualThan(absHalfMidpoint))
    {
        return 0;
    }

    // Add in an epsilon term to counteract arithmetic errors when segment is
    // (near) parallel to a coordinate axis (see text for detail)
    absHalfVector += EPSILON;

    // Try cross products of segment direction vector with coordinate axes
    Vector3 absMDCross = midPoint.Cross(halfVector).GetAbs();
    //Vector3 eaDCross = absHalfVector.Cross(aabbExtends);
    VectorFloat ex = aabbExtends.GetX();
    VectorFloat ey = aabbExtends.GetY();
    VectorFloat ez = aabbExtends.GetZ();
    VectorFloat adx = absHalfVector.GetX();
    VectorFloat ady = absHalfVector.GetY();
    VectorFloat adz = absHalfVector.GetZ();

    Vector3 ead(ey * adz + ez * ady, ex * adz + ez * adx, ex * ady + ey * adx);
    if (!absMDCross.IsLessEqualThan(ead))
    {
        return 0;
    }

    // No separating axis found; segment must be overlapping AABB
    return 1;
}


//=========================================================================
// IntersectRayAABB
// [10/21/2009]
//=========================================================================
int
AZ::Intersect::IntersectRayAABB(const Vector3& rayStart, const Vector3& dir, const Vector3& dirRCP, const Aabb& aabb, VectorFloat& tStart, VectorFloat& tEnd, Vector3& startNormal /*, Vector3& inter*/)
{
    // we don't need to test with all 6 normals (just 3)
    VectorFloat one = VectorFloat::CreateOne();
    VectorFloat zero = VectorFloat::CreateZero();

    const VectorFloat eps(0.0001f);  // \todo move to constant
    VectorFloat tmin = zero;  // set to -RR_FLT_MAX to get first hit on line
    VectorFloat tmax = g_fltMax;    // set to max distance ray can travel (for segment)

    const Vector3& aabbMin = aabb.GetMin();
    const Vector3& aabbMax = aabb.GetMax();

    // we unroll manually because there is no way to get in efficient way vectors for
    // each axis while getting it as a index
    Vector3 time1 = (aabbMin - rayStart) * dirRCP;
    Vector3 time2 = (aabbMax - rayStart) * dirRCP;

    // X
    if (dir.GetX().GetAbs().IsLessThan(eps))
    {
        // Ray is parallel to slab. No hit if origin not within slab
        if (rayStart.GetX().IsLessThan(aabbMin.GetX()) || rayStart.GetX().IsGreaterThan(aabbMax.GetX()))
        {
            return ISECT_RAY_AABB_NONE;
        }
    }
    else
    {
        // Compute intersection t value of ray with near and far plane of slab
        VectorFloat t1 = time1.GetX();
        VectorFloat t2 = time2.GetX();
        VectorFloat nSign = -one;

        // Make t1 be intersection with near plane, t2 with far plane
        if (t1.IsGreaterThan(t2))
        {
            VectorFloat tmp = t1;
            t1 = t2;
            t2 = tmp;
            nSign = one;
        }

        // Compute the intersection of slab intersections intervals
        if (tmin.IsLessThan(t1))
        {
            tmin = t1;

            startNormal.Set(nSign, zero, zero);
        }

        tmax = tmax.GetMin(t2);

        // Exit with no collision as soon as slab intersection becomes empty
        if (tmin.IsGreaterThan(tmax))
        {
            return ISECT_RAY_AABB_NONE;
        }
    }

    // Y
    if (dir.GetY().GetAbs().IsLessThan(eps))
    {
        // Ray is parallel to slab. No hit if origin not within slab
        if (rayStart.GetY().IsLessThan(aabbMin.GetY()) || rayStart.GetY().IsGreaterThan(aabbMax.GetY()))
        {
            return ISECT_RAY_AABB_NONE;
        }
    }
    else
    {
        // Compute intersection t value of ray with near and far plane of slab
        VectorFloat t1 = time1.GetY();
        VectorFloat t2 = time2.GetY();
        VectorFloat nSign = -one;

        // Make t1 be intersection with near plane, t2 with far plane
        if (t1.IsGreaterThan(t2))
        {
            VectorFloat tmp = t1;
            t1 = t2;
            t2 = tmp;
            nSign = one;
        }

        // Compute the intersection of slab intersections intervals
        if (tmin.IsLessThan(t1))
        {
            tmin = t1;

            startNormal.Set(zero, nSign, zero);
        }

        tmax = tmax.GetMin(t2);

        // Exit with no collision as soon as slab intersection becomes empty
        if (tmin.IsGreaterThan(tmax))
        {
            return ISECT_RAY_AABB_NONE;
        }
    }

    // Z
    if (dir.GetZ().GetAbs().IsLessThan(eps))
    {
        // Ray is parallel to slab. No hit if origin not within slab
        if (rayStart.GetZ().IsLessThan(aabbMin.GetZ()) || rayStart.GetZ().IsGreaterThan(aabbMax.GetZ()))
        {
            return ISECT_RAY_AABB_NONE;
        }
    }
    else
    {
        // Compute intersection t value of ray with near and far plane of slab
        VectorFloat t1 = time1.GetZ();
        VectorFloat t2 = time2.GetZ();
        VectorFloat nSign = -one;

        // Make t1 be intersection with near plane, t2 with far plane
        if (t1.IsGreaterThan(t2))
        {
            VectorFloat tmp = t1;
            t1 = t2;
            t2 = tmp;
            nSign = one;
        }

        // Compute the intersection of slab intersections intervals
        if (tmin.IsLessThan(t1))
        {
            tmin = t1;

            startNormal.Set(zero, zero, nSign);
        }

        tmax = tmax.GetMin(t2);

        // Exit with no collision as soon as slab intersection becomes empty
        if (tmin.IsGreaterThan(tmax))
        {
            return ISECT_RAY_AABB_NONE;
        }
    }

    tStart = tmin;
    tEnd = tmax;

    if (tmin == zero)  // no intersect if the segments starts inside or coincident the aabb
    {
        return ISECT_RAY_AABB_SA_INSIDE;
    }

    // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
    //inter = rayStart + dir * tmin;
    return ISECT_RAY_AABB_ISECT;
}

//=========================================================================
// IntersectRayAABB2
// [2/18/2011]
//=========================================================================
int
AZ::Intersect::IntersectRayAABB2(const Vector3& rayStart, const Vector3& dirRCP, const Aabb& aabb, VectorFloat& start, VectorFloat& end)
{
    VectorFloat tmin, tmax, tymin, tymax, tzmin, tzmax;
    Vector3 vZero = Vector3::CreateZero();

    Vector3 min = (Vector3::CreateSelectCmpGreaterEqual(dirRCP, vZero, aabb.GetMin(), aabb.GetMax()) - rayStart) * dirRCP;
    Vector3 max = (Vector3::CreateSelectCmpGreaterEqual(dirRCP, vZero, aabb.GetMax(), aabb.GetMin()) - rayStart) * dirRCP;

    tmin = min.GetX();
    tmax = max.GetX();
    tymin = min.GetY();
    tymax = max.GetY();

    if (tmin.IsGreaterThan(tymax) || tymin.IsGreaterThan(tmax))
    {
        return ISECT_RAY_AABB_NONE;
    }

    if (tymin.IsGreaterThan(tmin))
    {
        tmin = tymin;
    }

    if (tymax.IsLessThan(tmax))
    {
        tmax = tymax;
    }

    tzmin = min.GetZ();
    tzmax = max.GetZ();

    if (tmin.IsGreaterThan(tzmax) || tzmin.IsGreaterThan(tmax))
    {
        return ISECT_RAY_AABB_NONE;
    }

    if (tzmin.IsGreaterThan(tmin))
    {
        tmin = tzmin;
    }
    if (tzmax.IsLessThan(tmax))
    {
        tmax = tzmax;
    }

    start = tmin;
    end = tmax;

    return ISECT_RAY_AABB_ISECT;
}

//=========================================================================
// IntersectSegmentCylinder
// [10/21/2009]
//=========================================================================
int
AZ::Intersect::IntersectSegmentCylinder(const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const VectorFloat& r, VectorFloat& t)
{
    //const float EPSILON = 0.001f;
    const VectorFloat EPSILON(0.001f); // \todo move a const this loading is slow
    Vector3 d = q - p;  // can be cached
    Vector3 m = sa - p; // -"-
    Vector3 n = /*sb - sa*/ dir; // -"-
    VectorFloat zero = VectorFloat::CreateZero();

    VectorFloat md = m.Dot(d);
    VectorFloat nd = n.Dot(d);
    VectorFloat dd = d.Dot(d);

    // Test if segment fully outside either endcap of cylinder
    if (md.IsLessThan(zero) && (md + nd).IsLessThan(zero))
    {
        return RR_ISECT_RAY_CYL_NONE;                                                    // Segment outside 'p' side of cylinder
    }
    if (md.IsGreaterThan(dd) && (md + nd).IsGreaterThan(dd))
    {
        return RR_ISECT_RAY_CYL_NONE;                                                          // Segment outside 'q' side of cylinder
    }
    VectorFloat nn = n.Dot(n);
    VectorFloat mn = m.Dot(n);
    VectorFloat a = dd * nn - nd * nd;
    VectorFloat k = m.Dot(m) - r * r;
    VectorFloat c = dd * k - md * md;
    if (a.GetAbs().IsLessThan(EPSILON))
    {
        // Segment runs parallel to cylinder axis
        if (c.IsGreaterThan(zero))
        {
            return RR_ISECT_RAY_CYL_NONE;                        // 'a' and thus the segment lie outside cylinder
        }
        // Now known that segment intersects cylinder; figure out how it intersects
        if (md.IsLessThan(zero))
        {
            t = -mn / nn; // Intersect segment against 'p' endcap
            return RR_ISECT_RAY_CYL_P_SIDE;
        }
        else if (md.IsGreaterThan(dd))
        {
            t = (nd - mn) / nn; // Intersect segment against 'q' endcap
            return RR_ISECT_RAY_CYL_Q_SIDE;
        }
        else
        {
            // 'a' lies inside cylinder
            t = zero;
            return RR_ISECT_RAY_CYL_SA_INSIDE;
        }
    }
    VectorFloat b = dd * mn - nd * md;
    VectorFloat discr = b * b - a * c;
    if (discr.IsLessThan(zero))
    {
        return RR_ISECT_RAY_CYL_NONE;                         // No real roots; no intersection
    }
    t = (-b - discr.GetSqrt()) / a;
    int result = RR_ISECT_RAY_CYL_PQ; // default along the PQ segment

    if ((md + t * nd).IsLessThan(zero))
    {
        // Intersection outside cylinder on 'p' side
        if (nd.IsLessEqualThan(zero))
        {
            return RR_ISECT_RAY_CYL_NONE;                           // Segment pointing away from endcap
        }
        float t0 = -md / nd;
        // Keep intersection if Dot(S(t) - p, S(t) - p) <= r^2
        if ((k + t0 * (2.0f * mn + t0 * nn)).IsLessEqualThan(zero))
        {
            //                  if( t0 < 0.0f ) t0 = 0.0f; // it's inside the cylinder
            t = t0;
            result = RR_ISECT_RAY_CYL_P_SIDE;
        }
        else
        {
            return RR_ISECT_RAY_CYL_NONE;
        }
    }
    else if ((md + t * nd).IsGreaterThan(dd))
    {
        // Intersection outside cylinder on 'q' side
        if (nd.IsGreaterEqualThan(zero))
        {
            return RR_ISECT_RAY_CYL_NONE;                              // Segment pointing away from endcap
        }
        VectorFloat t0 = (dd - md) / nd;
        // Keep intersection if Dot(S(t) - q, S(t) - q) <= r^2
        if ((k + dd - 2.0f * md + t0 * (2.0f * (mn - nd) + t0 * nn)).IsLessEqualThan(zero))
        {
            //                  if( t0 < 0.0f ) t0 = 0.0f; // it's inside the cylinder
            t = t0;
            result = RR_ISECT_RAY_CYL_Q_SIDE;
        }
        else
        {
            return RR_ISECT_RAY_CYL_NONE;
        }
    }

    // Segment intersects cylinder between the end-caps; t is correct
    if (t.IsGreaterThan(VectorFloat::CreateOne()))
    {
        return RR_ISECT_RAY_CYL_NONE; // Intersection lies outside segment
    }
    else if (t.IsLessThan(zero))
    {
        if (c.IsLessEqualThan(zero))
        {
            t = zero;
            return RR_ISECT_RAY_CYL_SA_INSIDE; // Segment starts inside
        }
        else
        {
            return RR_ISECT_RAY_CYL_NONE; // Intersection lies outside segment
        }
    }
    else
    {
        return result;
    }
}
//=========================================================================
// IntersectSegmentCapsule
// [10/21/2009]
//=========================================================================
int
AZ::Intersect::IntersectSegmentCapsule(const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const VectorFloat& r, VectorFloat& t)
{
    int result = IntersectSegmentCylinder(sa, dir, p, q, r, t);

    if (result == RR_ISECT_RAY_CYL_SA_INSIDE)
    {
        return ISECT_RAY_CAPSULE_SA_INSIDE;
    }

    if (result == RR_ISECT_RAY_CYL_PQ)
    {
        return ISECT_RAY_CAPSULE_PQ;
    }

    Vector3 dirNorm = dir;
    VectorFloat len = dirNorm.NormalizeWithLength();
    VectorFloat timeLen;

    // check spheres
    VectorFloat timeLenTop, timeLenBottom;
    int resultTop = IntersectRaySphere(sa, dirNorm, p, r, timeLenTop);
    if (resultTop == ISECT_RAY_SPHERE_SA_INSIDE)
    {
        return ISECT_RAY_CAPSULE_SA_INSIDE;
    }
    int resultBottom = IntersectRaySphere(sa, dirNorm, q, r, timeLenBottom);
    if (resultBottom == ISECT_RAY_SPHERE_SA_INSIDE)
    {
        return ISECT_RAY_CAPSULE_SA_INSIDE;
    }

    if (resultTop == ISECT_RAY_SPHERE_ISECT)
    {
        if (resultBottom == ISECT_RAY_SPHERE_ISECT)
        {
            // if we intersect both spheres pick the closest one
            if (timeLenTop.IsLessThan(timeLenBottom))
            {
                t = timeLenTop / len;
                return ISECT_RAY_CAPSULE_P_SIDE;
            }
            else
            {
                t = timeLenBottom / len;
                return ISECT_RAY_CAPSULE_Q_SIDE;
            }
        }
        else
        {
            t = timeLenTop / len;
            return ISECT_RAY_CAPSULE_P_SIDE;
        }
    }

    if (resultBottom == ISECT_RAY_SPHERE_ISECT)
    {
        t = timeLenBottom / len;
        return ISECT_RAY_CAPSULE_Q_SIDE;
    }

    return ISECT_RAY_CAPSULE_NONE;
}

//=========================================================================
// IntersectSegmentPolyhedron
// [10/21/2009]
//=========================================================================
int
AZ::Intersect::IntersectSegmentPolyhedron(const Vector3& sa, const Vector3& sBA, const Plane p[], int numPlanes,
    VectorFloat& tfirst, VectorFloat& tlast, int& iFirstPlane, int& iLastPlane)
{
    // Compute direction vector for the segment
    Vector3 d = /*b - a*/ sBA;
    // Set initial interval to being the whole segment. For a ray, tlast should be
    // set to +RR_FLT_MAX. For a line, additionally tfirst should be set to -RR_FLT_MAX
    VectorFloat zero = VectorFloat::CreateZero();
    tfirst = zero;
    tlast = VectorFloat::CreateOne();
    iFirstPlane = -1;
    iLastPlane = -1;
    // Intersect segment against each plane
    for (int i = 0; i < numPlanes; i++)
    {
        const Vector4& plane = p[i].GetPlaneEquationCoefficients();

        VectorFloat denom = plane.Dot3(d);
        // don't forget we store -D in the plane
        VectorFloat dist = (-plane.GetW()) - plane.Dot3(sa);
        // Test if segment runs parallel to the plane
        if (denom == zero)
        {
            // If so, return "no intersection" if segment lies outside plane
            if (dist.IsLessThan(zero))
            {
                return 0;
            }
        }
        else
        {
            // Compute parameterized t value for intersection with current plane
            VectorFloat t = dist / denom;
            if (denom.IsLessThan(zero))
            {
                // When entering half space, update tfirst if t is larger
                if (t.IsGreaterThan(tfirst))
                {
                    tfirst = t;
                    iFirstPlane = i;
                }
            }
            else
            {
                // When exiting half space, update tlast if t is smaller
                if (t.IsLessThan(tlast))
                {
                    tlast = t;
                    iLastPlane = i;
                }
            }

            // Exit with "no intersection" if intersection becomes empty
            if (tfirst.IsGreaterThan(tlast))
            {
                return 0;
            }
        }
    }

    //DBG_Assert(iFirstPlane!=-1&&iLastPlane!=-1,("We have some bad border case to have only one plane, fix this function!"));
    if (iFirstPlane == -1 && iLastPlane == -1)
    {
        return 0;
    }

    // A nonzero logical intersection, so the segment intersects the polyhedron
    return 1;
}

//=========================================================================
// ClosestSegmentSegment
// [10/21/2009]
//=========================================================================
void
AZ::Intersect::ClosestSegmentSegment(const Vector3& s1, const Vector3& s2, const Vector3& s3, const Vector3& s4, Vector3& v1, Vector3& v2)
{
    VectorFloat ua, ub;

    Vector3 v21 = s2 - s1; // Direction vector of segment S1
    Vector3 v43 = s4 - s3; // Direction vector of segment S2
    Vector3 r = s1 - s3;
    VectorFloat a = v21.Dot(v21); // Squared length of segment S1, always nonnegative
    VectorFloat e = v43.Dot(v43); // Squared length of segment S2, always nonnegative
    VectorFloat f = v43.Dot(r);

    VectorFloat zero = VectorFloat::CreateZero();
    VectorFloat one = VectorFloat::CreateOne();

    AZ_Assert(a >= VectorFloat(1e-5f) && (float)e >= 1e-5f, "We agreed that we should NOT pass invalid segments");

    // Check if either or both segments degenerate into points
    //if (a <= EPSILON && e <= EPSILON) {
    //  // Both segments degenerate into points
    //  s = t = 0.0f;
    //  c1 = p1;
    //  c2 = p2;
    //  return Dot(c1 - c2, c1 - c2);
    //}

    //if (a <= EPSILON) {
    //  // First segment degenerates into a point
    //  s = 0.0f;
    //  t = f / e; // s = 0 => t = (b*s + f) / e = f / e
    //  t = Clamp(t, 0.0f, 1.0f);
    //} else {
    VectorFloat c = v21.Dot(r);
    //  if (e <= EPSILON) {
    //      // Second segment degenerates into a point
    //      t = 0.0f;
    //      s = Clamp(-c / a, 0.0f, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
    //  } else {
    // The general non-degenerate case starts here
    VectorFloat b = v21.Dot(v43);
    VectorFloat denom = a * e - b * b; // Always nonnegative

    // If segments not parallel, compute closest point on L1 to L2, and
    // clamp to segment S1. Else pick arbitrary ua (here 0)
    if (denom != zero)
    {
        ua = ((b * f - c * e) / denom).GetClamp(zero, one);
    }
    else
    {
        ua = zero;
    }

    // Compute point on L2 closest to S1(s) using
    // ub = Dot((s1+v21*ua)-s3,s43) / Dot(v43,v43) = (b*ua + f) / e
    ub = (b * ua + f) / e;

    // If ub in [0,1] done. Else clamp ub, recompute ua for the new value
    // of ub using ua = Dot((s3+v43*ub)-s1,v21) / Dot(v21,v31)= (ub*b - c) / a
    // and clamp ua to [0, 1]
    if (ub.IsLessThan(zero))
    {
        ub = zero;
        ua = (-c / a).GetClamp(zero, one);
    }
    else if (ub.IsGreaterThan(one))
    {
        ub = one;
        ua = ((b - c) / a).GetClamp(zero, one);
    }
    //      }
    //  }

    v1 = s1 + v21 * ua;
    v2 = s3 + v43 * ub;
}

#if 0
//////////////////////////////////////////////////////////////////////////
// TEST AABB/RAY TEST using slopes
namespace test
{
    enum CLASSIFICATION
    {
        MMM, MMP, MPM, MPP, PMM, PMP, PPM, PPP, POO, MOO, OPO, OMO, OOP, OOM,
        OMM, OMP, OPM, OPP, MOM, MOP, POM, POP, MMO, MPO, PMO, PPO
    };

    struct ray
    {
        //common variables
        float x, y, z;      // ray origin
        float i, j, k;      // ray direction
        float ii, ij, ik;   // inverses of direction components

        // ray slope
        int classification;
        float ibyj, jbyi, kbyj, jbyk, ibyk, kbyi; //slope
        float c_xy, c_xz, c_yx, c_yz, c_zx, c_zy;
    };

    struct aabox
    {
        float x0, y0, z0, x1, y1, z1;
    };

    void make_ray(float x, float y, float z, float i, float j, float k, ray* r)
    {
        //common variables
        r->x = x;
        r->y = y;
        r->z = z;
        r->i = i;
        r->j = j;
        r->k = k;

        r->ii = 1.0f / i;
        r->ij = 1.0f / j;
        r->ik = 1.0f / k;

        //ray slope
        r->ibyj = r->i * r->ij;
        r->jbyi = r->j * r->ii;
        r->jbyk = r->j * r->ik;
        r->kbyj = r->k * r->ij;
        r->ibyk = r->i * r->ik;
        r->kbyi = r->k * r->ii;
        r->c_xy = r->y - r->jbyi * r->x;
        r->c_xz = r->z - r->kbyi * r->x;
        r->c_yx = r->x - r->ibyj * r->y;
        r->c_yz = r->z - r->kbyj * r->y;
        r->c_zx = r->x - r->ibyk * r->z;
        r->c_zy = r->y - r->jbyk * r->z;

        //ray slope classification
        if (i < 0)
        {
            if (j < 0)
            {
                if (k < 0)
                {
                    r->classification = MMM;
                }
                else if (k > 0)
                {
                    r->classification = MMP;
                }
                else//(k >= 0)
                {
                    r->classification = MMO;
                }
            }
            else//(j >= 0)
            {
                if (k < 0)
                {
                    r->classification = MPM;
                    if (j == 0)
                    {
                        r->classification = MOM;
                    }
                }
                else//(k >= 0)
                {
                    if ((j == 0) && (k == 0))
                    {
                        r->classification = MOO;
                    }
                    else if (k == 0)
                    {
                        r->classification = MPO;
                    }
                    else if (j == 0)
                    {
                        r->classification = MOP;
                    }
                    else
                    {
                        r->classification = MPP;
                    }
                }
            }
        }
        else//(i >= 0)
        {
            if (j < 0)
            {
                if (k < 0)
                {
                    r->classification = PMM;
                    if (i == 0)
                    {
                        r->classification = OMM;
                    }
                }
                else//(k >= 0)
                {
                    if ((i == 0) && (k == 0))
                    {
                        r->classification = OMO;
                    }
                    else if (k == 0)
                    {
                        r->classification = PMO;
                    }
                    else if (i == 0)
                    {
                        r->classification = OMP;
                    }
                    else
                    {
                        r->classification = PMP;
                    }
                }
            }
            else//(j >= 0)
            {
                if (k < 0)
                {
                    if ((i == 0) && (j == 0))
                    {
                        r->classification = OOM;
                    }
                    else if (i == 0)
                    {
                        r->classification = OPM;
                    }
                    else if (j == 0)
                    {
                        r->classification = POM;
                    }
                    else
                    {
                        r->classification = PPM;
                    }
                }
                else//(k > 0)
                {
                    if (i == 0)
                    {
                        if (j == 0)
                        {
                            r->classification = OOP;
                        }
                        else if (k == 0)
                        {
                            r->classification = OPO;
                        }
                        else
                        {
                            r->classification = OPP;
                        }
                    }
                    else
                    {
                        if ((j == 0) && (k == 0))
                        {
                            r->classification = POO;
                        }
                        else if (j == 0)
                        {
                            r->classification = POP;
                        }
                        else if (k == 0)
                        {
                            r->classification = PPO;
                        }
                        else
                        {
                            r->classification = PPP;
                        }
                    }
                }
            }
        }
    }

    bool slope(ray* r, aabox* b)
    {
        switch (r->classification)
        {
        case MMM:

            if ((r->x < b->x0) || (r->y < b->y0) || (r->z < b->z0)
                || (r->jbyi * b->x0 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z0 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x0 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MMP:

            if ((r->x < b->x0) || (r->y < b->y0) || (r->z > b->z1)
                || (r->jbyi * b->x0 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z1 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x0 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MPM:

            if ((r->x < b->x0) || (r->y > b->y1) || (r->z < b->z0)
                || (r->jbyi * b->x0 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z0 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x0 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MPP:

            if ((r->x < b->x0) || (r->y > b->y1) || (r->z > b->z1)
                || (r->jbyi * b->x0 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z1 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x0 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case PMM:

            if ((r->x > b->x1) || (r->y < b->y0) || (r->z < b->z0)
                || (r->jbyi * b->x1 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z0 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x1 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case PMP:

            if ((r->x > b->x1) || (r->y < b->y0) || (r->z > b->z1)
                || (r->jbyi * b->x1 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z1 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x1 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case PPM:

            if ((r->x > b->x1) || (r->y > b->y1) || (r->z < b->z0)
                || (r->jbyi * b->x1 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z0 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x1 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case PPP:

            if ((r->x > b->x1) || (r->y > b->y1) || (r->z > b->z1)
                || (r->jbyi * b->x1 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z1 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x1 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case OMM:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->z < b->z0)
                || (r->jbyk * b->z0 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z1 + r->c_yz > 0)
                )
            {
                return false;
            }

            return true;

        case OMP:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->z > b->z1)
                || (r->jbyk * b->z1 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z0 + r->c_yz < 0)
                )
            {
                return false;
            }

            return true;

        case OPM:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y > b->y1) || (r->z < b->z0)
                || (r->jbyk * b->z0 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z1 + r->c_yz > 0)
                )
            {
                return false;
            }

            return true;

        case OPP:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y > b->y1) || (r->z > b->z1)
                || (r->jbyk * b->z1 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z0 + r->c_yz < 0)
                )
            {
                return false;
            }

            return true;

        case MOM:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x < b->x0) || (r->z < b->z0)
                || (r->kbyi * b->x0 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MOP:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x < b->x0) || (r->z > b->z1)
                || (r->kbyi * b->x0 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case POM:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x > b->x1) || (r->z < b->z0)
                || (r->kbyi * b->x1 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case POP:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x > b->x1) || (r->z > b->z1)
                || (r->kbyi * b->x1 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case MMO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x < b->x0) || (r->y < b->y0)
                || (r->jbyi * b->x0 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x1 + r->c_yx > 0)
                )
            {
                return false;
            }

            return true;

        case MPO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x < b->x0) || (r->y > b->y1)
                || (r->jbyi * b->x0 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x1 + r->c_yx > 0)
                )
            {
                return false;
            }

            return true;

        case PMO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x > b->x1) || (r->y < b->y0)
                || (r->jbyi * b->x1 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x0 + r->c_yx < 0)
                )
            {
                return false;
            }

            return true;

        case PPO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x > b->x1) || (r->y > b->y1)
                || (r->jbyi * b->x1 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x0 + r->c_yx < 0)
                )
            {
                return false;
            }

            return true;

        case MOO:

            if ((r->x < b->x0)
                || (r->y < b->y0) || (r->y > b->y1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

            return true;

        case POO:

            if ((r->x > b->x1)
                || (r->y < b->y0) || (r->y > b->y1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

            return true;

        case OMO:

            if ((r->y < b->y0)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

        case OPO:

            if ((r->y > b->y1)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

        case OOM:

            if ((r->z < b->z0)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->y > b->y1)
                )
            {
                return false;
            }

        case OOP:

            if ((r->z > b->z1)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->y > b->y1)
                )
            {
                return false;
            }

            return true;
        }

        return false;
    }
}
//////////////////////////////////////////////////////////////////////////
#endif

#endif // #ifndef AZ_UNITY_BUILD