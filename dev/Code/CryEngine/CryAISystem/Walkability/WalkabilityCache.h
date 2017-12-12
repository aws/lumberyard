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

#ifndef CRYINCLUDE_CRYAISYSTEM_WALKABILITY_WALKABILITYCACHE_H
#define CRYINCLUDE_CRYAISYSTEM_WALKABILITY_WALKABILITYCACHE_H
#pragma once


#include "FloorHeightCache.h"


class WalkabilityCache
{
public:
    typedef StaticDynArray<IPhysicalEntity*, 768> Entities;
    typedef StaticDynArray<AABB, 768> AABBs;

    WalkabilityCache(tAIObjectID actorID);

    void Reset(bool resetFloorCache = true);
    bool Cache(const AABB& aabb);
    void Draw();

    const AABB& GetAABB() const;
    bool FullyContaints(const AABB& aabb) const;

    size_t GetOverlapping(const AABB& aabb, Entities& entities) const;
    size_t GetOverlapping(const AABB& aabb, Entities& entities, AABBs& aabbs) const;

    bool IsFloorCached(const Vec3& position, Vec3& floor) const;

    bool FindFloor(const Vec3& position, Vec3& floor);
    bool FindFloor(const Vec3& position, Vec3& floor, IPhysicalEntity** entities, AABB* aabbs, size_t entityCount,
        const AABB& enclosingAABB);
    bool CheckWalkability(const Vec3& origin, const Vec3& target, float radius, Vec3* finalFloor = 0, bool* flatFloor = 0);
    bool OverlapTorsoSegment(const Vec3& start, const Vec3& end, float radius, IPhysicalEntity** entities, size_t entityCount);

    size_t GetMemoryUsage() const;

private:
    Vec3 m_center;
    AABB m_aabb;

    size_t m_entititesHash;
    tAIObjectID m_actorID;

    Entities m_entities;
    AABBs m_aabbs;

    FloorHeightCache m_floorCache;
};

#endif // CRYINCLUDE_CRYAISYSTEM_WALKABILITY_WALKABILITYCACHE_H
