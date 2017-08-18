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

#ifndef CRYINCLUDE_CRYAISYSTEM_WALKABILITY_WALKABILITYCACHEMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_WALKABILITY_WALKABILITYCACHEMANAGER_H
#pragma once


#include <PoolAllocator.h>
#include "WalkabilityCache.h"


class WalkabilityCacheManager
{
public:
    WalkabilityCacheManager();
    ~WalkabilityCacheManager();

    void Reset();
    void PreUpdate();
    void PostUpdate();
    void Draw();

    void EnableActor(tAIObjectID actorID, bool enabled);
    void PrepareActor(tAIObjectID actorID, const AABB& aabb);

    bool IsFloorCached(tAIObjectID actorID, const Vec3& position, Vec3& floor);
    bool FindFloor(tAIObjectID actorID, const Vec3& position, Vec3& floor);
    bool CheckWalkability(tAIObjectID actorID, const Vec3& origin, const Vec3& target, float radius, Vec3* finalFloor = 0,
        bool* flatFloor = 0);
    bool CheckWalkability(tAIObjectID actorID, const Vec3& origin, const Vec3& target, float radius,
        const ListPositions& boundary, Vec3* finalFloor = 0, bool* flatFloor = 0, const AABB* boundaryAABB = 0);
private:
    struct ActorWalkabilityCache
    {
        ActorWalkabilityCache()
            : cache(0)
            , frameID(0)
        {
        }

        WalkabilityCache* cache;
        size_t frameID;
    };

    size_t m_currentFrameID;

    typedef VectorMap<tAIObjectID, ActorWalkabilityCache> ActorWalkabilityCaches;
    ActorWalkabilityCaches m_caches;

    stl::PoolAllocatorNoMT<sizeof(WalkabilityCache)> m_alloc;

    size_t m_walkabilityRequestCount;
    size_t m_walkabilityCacheHitCount;
    size_t m_floorRequestCount;
    size_t m_floorCacheHitCount;
    size_t m_preservedFloorCache;
};

#endif // CRYINCLUDE_CRYAISYSTEM_WALKABILITY_WALKABILITYCACHEMANAGER_H
