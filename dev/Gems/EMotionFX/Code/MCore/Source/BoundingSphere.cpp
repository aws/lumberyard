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
#include "BoundingSphere.h"
#include "AABB.h"


namespace MCore
{
    // encapsulate a point in the sphere
    void BoundingSphere::Encapsulate(const Vector3& v)
    {
        // calculate the squared distance from the center to the point
        const Vector3 diff = v - mCenter;
        const float dist = diff.Dot(diff);

        // if the current sphere doesn't contain the point, grow the sphere so that it contains the point
        if (dist > mRadiusSq)
        {
            const Vector3 diff2 = diff.Normalized() * mRadius;
            const Vector3 delta = 0.5f * (diff - diff2);
            mCenter             += delta;
            mRadius             += delta.SafeLength();
            mRadiusSq           = mRadius * mRadius;
        }
    }


    // check if the sphere intersects with a given AABB
    bool BoundingSphere::Intersects(const AABB& b) const
    {
        float distance = 0.0f;

        for (uint32 t = 0; t < 3; ++t)
        {
            const Vector3& minVec = b.GetMin();
            if (mCenter[t] < minVec[t])
            {
                distance += (mCenter[t] - minVec[t]) * (mCenter[t] - minVec[t]);
                if (distance > mRadiusSq)
                {
                    return false;
                }
            }
            else
            {
                const Vector3& maxVec = b.GetMax();
                if (mCenter[t] > maxVec[t])
                {
                    distance += (mCenter[t] - maxVec[t]) * (mCenter[t] - maxVec[t]);
                    if (distance > mRadiusSq)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }


    // check if the sphere completely contains a given AABB
    bool BoundingSphere::Contains(const AABB& b) const
    {
        float distance = 0.0f;
        for (uint32 t = 0; t < 3; ++t)
        {
            const Vector3& maxVec = b.GetMax();
            if (mCenter[t] < maxVec[t])
            {
                distance += (mCenter[t] - maxVec[t]) * (mCenter[t] - maxVec[t]);
            }
            else
            {
                const Vector3& minVec = b.GetMin();
                if (mCenter[t] > minVec[t])
                {
                    distance += (mCenter[t] - minVec[t]) * (mCenter[t] - minVec[t]);
                }
            }

            if (distance > mRadiusSq)
            {
                return false;
            }
        }

        return true;
    }
}   // namespace MCore
