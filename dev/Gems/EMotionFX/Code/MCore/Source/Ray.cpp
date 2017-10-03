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

// include required headers
#include "Ray.h"
#include "AABB.h"
#include "BoundingSphere.h"
#include "PlaneEq.h"
#include "FastMath.h"
#include "Algorithms.h"


namespace MCore
{
    // ray-boundingsphere
    bool Ray::Intersects(const BoundingSphere& s, Vector3* intersectA, Vector3* intersectB) const
    {
        Vector3 rayOrg = mOrigin - s.GetCenter(); // ray in space of the sphere

        float b, c, t1, t2, delta;

        b = rayOrg.Dot(mDirection);
        c = rayOrg.SquareLength() - s.GetRadiusSquared();

        delta = ((b * b) - c);

        if (delta < 0.0f)
        {
            return false; // no intersection
        }
        if (intersectA == nullptr && intersectB == nullptr)
        {
            return true;
        }

        if (delta < Math::epsilon)
        {
            delta = Math::Sqrt(delta);

            t1 = (-b + delta);
            if (t1 < 0.0f)
            {
                return false;
            }
            t2 = (-b - delta);
            if (t2 < 0.0f)
            {
                return false;
            }

            if (t1 < t2)
            {
                if (intersectA)
                {
                    (*intersectA) = mOrigin + mDirection * t1;
                }
                if (intersectB)
                {
                    (*intersectB) = mOrigin + mDirection * t2;
                }
            }
            else
            {
                if (intersectA)
                {
                    (*intersectA) = mOrigin + mDirection * t2;
                }
                if (intersectB)
                {
                    (*intersectB) = mOrigin + mDirection * t1;
                }
            }
        }
        else
        {
            // if we are here it means that delta equals zero and we have only one solution
            t1 = (-b);
            if (intersectA)
            {
                (*intersectA) = mOrigin + mDirection * t1;
            }
            if (intersectB)
            {
                (*intersectB) = (*intersectA);
            }
        }

        return true;
    }


    // ray-plane
    bool Ray::Intersects(const PlaneEq& p, Vector3* intersect) const
    {
        // check if ray is parallel to plane (no intersection) or ray pointing away from plane (no intersection)
        float dot1 = p.GetNormal().Dot(mDirection);
        //if (dot1 >= 0) return false;  // backface cull

        // calc second dot product
        float dot2 = -(p.GetNormal().Dot(mOrigin) + p.GetDist());

        // calc t value
        float t = dot2 / dot1;

        // if t<0 then the line defined by the ray, intersects the plane behind the rays origin and so no
        // intersection occurs. else we can calculate the intersection point
        if (t < 0.0f)
        {
            return false;
        }
        if (t > Length())
        {
            return false;
        }

        // calc intersection point
        if (intersect)
        {
            intersect->x = mOrigin.x + (mDirection.x * t);
            intersect->y = mOrigin.y + (mDirection.y * t);
            intersect->z = mOrigin.z + (mDirection.z * t);
        }

        return true;
    }


    // ray-triangle intersection test
    bool Ray::Intersects(const Vector3& p1, const Vector3& p2, const Vector3& p3, Vector3* intersect, float* baryU, float* baryV) const
    {
        // calculate two vectors of the polygon
        const Vector3 edge1 = p2 - p1;
        const Vector3 edge2 = p3 - p1;

        // begin calculating determinant - also used to calculate U parameter
        const Vector3 dir = mDest - mOrigin;
        const Vector3 pvec = dir.Cross(edge2);

        // if determinant is near zero, ray lies in plane of triangle
        const float det = edge1.Dot(pvec);
        if (det > -Math::epsilon && det < Math::epsilon)
        {
            return false;
        }

        // calculate distance from vert0 to ray origin
        const Vector3 tvec = mOrigin - p1;

        // calculate U parameter and test bounds
        const float inv_det = 1.0f / det;
        const float u = tvec.Dot(pvec) * inv_det;
        if (u < 0.0f || u > 1.0f)
        {
            return false;
        }

        // prepare to test V parameter
        const Vector3 qvec = tvec.Cross(edge1);

        // calculate V parameter and test bounds
        const float v = dir.Dot(qvec) * inv_det;
        if (v < 0.0f || u + v > 1.0f)
        {
            return false;
        }

        // calculate t, ray intersects triangle
        const float t = edge2.Dot(qvec) * inv_det;
        if (t < 0.0f || t > 1.0f)
        {
            return false;
        }

        // output the results
        if (baryU)
        {
            *baryU = u;
        }
        if (baryV)
        {
            *baryV = v;
        }
        if (intersect)
        {
            *intersect = mOrigin + t * dir;
        }

        // yes, there was an intersection
        return true;
    }


    // ray-axis aligned bounding box
    bool Ray::Intersects(const AABB& b, Vector3* intersectA, Vector3* intersectB) const
    {
        float tNear = -FLT_MAX, tFar = FLT_MAX;

        const Vector3& minVec = b.GetMin();
        const Vector3& maxVec = b.GetMax();

        // For all three axes, check the near and far intersection point on the two slabs
        for (int32 i = 0; i < 3; i++)
        {
            if (Math::Abs(mDirection[i]) < Math::epsilon)
            {
                // direction is parallel to this plane, check if we're somewhere between min and max
                if ((mOrigin[i] < minVec[i]) || (mOrigin[i] > maxVec[i]))
                {
                    return false;
                }
            }
            else
            {
                // calculate t's at the near and far slab, see if these are min or max t's
                float t1 = (minVec[i] - mOrigin[i]) / mDirection[i];
                float t2 = (maxVec[i] - mOrigin[i]) / mDirection[i];
                if (t1 > t2)
                {
                    float temp = t1;
                    t1 = t2;
                    t2 = temp;
                }
                if (t1 > tNear)
                {
                    tNear = t1;                                 // accept nearest value
                }
                if (t2 < tFar)
                {
                    tFar  = t2;                                 // accept farthest value
                }
                if ((tNear > tFar) || (tFar < 0.0f))
                {
                    return false;
                }
            }
        }

        if (intersectA)
        {
            *intersectA = mOrigin + mDirection * tNear;
        }

        if (intersectB)
        {
            *intersectB = mOrigin + mDirection * tFar;
        }

        return true;
    }
}   // namespace MCore
