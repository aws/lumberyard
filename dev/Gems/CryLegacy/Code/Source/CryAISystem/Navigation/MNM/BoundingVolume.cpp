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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include "BoundingVolume.h"


namespace MNM
{
    void BoundingVolume::Swap(BoundingVolume& other)
    {
        vertices.swap(other.vertices);
        std::swap(aabb, other.aabb);
        std::swap(height, other.height);
    }

    bool BoundingVolume::Overlaps(const AABB& _aabb) const
    {
        if (!Overlap::AABB_AABB(aabb, _aabb))
        {
            return false;
        }

        const size_t vertexCount = vertices.size();

        for (size_t i = 0; i < vertexCount; ++i)
        {
            const Vec3& v0 = vertices[i];

            if (Overlap::Point_AABB2D(v0, _aabb))
            {
                return true;
            }
        }

        float midz = aabb.min.z + (aabb.max.z - aabb.min.z) * 0.5f;

        if (Contains(Vec3(_aabb.min.x, _aabb.min.y, midz)))
        {
            return true;
        }

        if (Contains(Vec3(_aabb.min.x, _aabb.max.y, midz)))
        {
            return true;
        }

        if (Contains(Vec3(_aabb.max.x, _aabb.max.y, midz)))
        {
            return true;
        }

        if (Contains(Vec3(_aabb.max.x, _aabb.min.y, midz)))
        {
            return true;
        }

        size_t ii = vertexCount - 1;
        for (size_t i = 0; i < vertexCount; ++i)
        {
            const Vec3& v0 = vertices[ii];
            const Vec3& v1 = vertices[i];
            ii = i;

            if (Overlap::Lineseg_AABB2D(Lineseg(v0, v1), _aabb))
            {
                return true;
            }
        }

        return false;
    }

    BoundingVolume::ExtendedOverlap BoundingVolume::Contains(const AABB& _aabb) const
    {
        if (!Overlap::AABB_AABB(aabb, _aabb))
        {
            return NoOverlap;
        }

        const size_t vertexCount = vertices.size();
        size_t inCount = 0;

        if (Contains(Vec3(_aabb.min.x, _aabb.min.y, _aabb.min.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.min.x, _aabb.min.y, _aabb.max.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.min.x, _aabb.max.y, _aabb.min.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.min.x, _aabb.max.y, _aabb.max.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.max.x, _aabb.max.y, _aabb.min.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.max.x, _aabb.max.y, _aabb.max.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.max.x, _aabb.min.y, _aabb.max.z)))
        {
            ++inCount;
        }

        if (Contains(Vec3(_aabb.max.x, _aabb.min.y, _aabb.max.z)))
        {
            ++inCount;
        }

        if (inCount != 8)
        {
            return PartialOverlap;
        }

        size_t ii = vertexCount - 1;

        for (size_t i = 0; i < vertexCount; ++i)
        {
            const Vec3& v0 = vertices[ii];
            const Vec3& v1 = vertices[i];
            ii = i;

            if (Overlap::Lineseg_AABB2D(Lineseg(v0, v1), _aabb))
            {
                return PartialOverlap;
            }
        }

        return FullOverlap;
    }

    bool BoundingVolume::Contains(const Vec3& point) const
    {
        if (!Overlap::Point_AABB(point, aabb))
        {
            return false;
        }

        const size_t vertexCount = vertices.size();
        bool count = false;

        size_t ii = vertexCount - 1;
        for (size_t i = 0; i < vertexCount; ++i)
        {
            const Vec3& v0 = vertices[ii];
            const Vec3& v1 = vertices[i];
            ii = i;

            if ((((v1.y <= point.y) && (point.y < v0.y)) || ((v0.y <= point.y) && (point.y < v1.y))) &&
                (point.x < (v0.x - v1.x) * (point.y - v1.y) / (v0.y - v1.y) + v1.x))
            {
                count = !count;
            }
        }

        return count != 0;
    }

    // Separating axis test
    // http://www.geometrictools.com/Documentation/MethodOfSeparatingAxes.pdf
    bool DoesAxisOverlap(float segMin, float segMax, float boxMin, float boxMax, float& t0, float& t1)
    {
        float rayDir = segMax - segMin;

        if (!fcmp(rayDir, 0.0f))
        {
            float s0 = (boxMin - segMin) / rayDir;
            float s1 = (boxMax - segMin) / rayDir;

            if (s1 < s0)
            {
                std::swap(s0, s1);
            }

            if (s0 < t1 && s1 > t0)
            {
                t0 = max(t0, s0);
                t1 = min(t1, s1);
                return true;
            }
        }

        return false;
    }

    std::pair<bool, float> BoundingVolume::IntersectLineSeg(const Vec3& segP0, const Vec3& segP1) const
    {
        float t0 = 0.0f;
        float t1 = 1.0f;

        const auto noIntersect = std::make_pair(false, -1.0f);

        // We overlap if there is overlap on all three axes
        if (!DoesAxisOverlap(segP0.x, segP1.x, this->aabb.min.x, this->aabb.max.x, t0, t1))
        {
            return noIntersect;
        }
        if (!DoesAxisOverlap(segP0.y, segP1.y, this->aabb.min.y, this->aabb.max.y, t0, t1))
        {
            return noIntersect;
        }
        if (!DoesAxisOverlap(segP0.z, segP1.z, this->aabb.min.z, this->aabb.max.z, t0, t1))
        {
            return noIntersect;
        }
        return std::make_pair(true, t0);
    }
}
