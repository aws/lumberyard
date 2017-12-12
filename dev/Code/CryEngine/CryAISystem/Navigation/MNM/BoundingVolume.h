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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_BOUNDINGVOLUME_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_BOUNDINGVOLUME_H
#pragma once


namespace MNM
{
    struct BoundingVolume
    {
        BoundingVolume()
            : aabb(AABB::RESET)
            , height(0.0f)
        {
        }

        typedef std::vector<Vec3> Boundary;
        Boundary vertices;
        AABB aabb;

        float height;

        void Swap(BoundingVolume& other);

        bool Overlaps(const AABB& _aabb) const;

        enum ExtendedOverlap
        {
            NoOverlap = 0,
            PartialOverlap = 1,
            FullOverlap = 2,
        };

        ExtendedOverlap Contains(const AABB& _aabb) const;
        bool Contains(const Vec3& point) const;

        std::pair<bool, float> IntersectLineSeg(const Vec3& segP0, const Vec3& segP1) const;
    };

    struct HashBoundingVolume
    {
        BoundingVolume vol;
        uint32 hash;
    };
}

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_MNM_BOUNDINGVOLUME_H
