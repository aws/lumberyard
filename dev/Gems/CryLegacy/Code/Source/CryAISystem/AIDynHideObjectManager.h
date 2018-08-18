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

// Description : Provides to query hide points around entities which
//               are flagged as AI hideable. The manage also caches the objects.


#ifndef CRYINCLUDE_CRYAISYSTEM_AIDYNHIDEOBJECTMANAGER_H
#define CRYINCLUDE_CRYAISYSTEM_AIDYNHIDEOBJECTMANAGER_H
#pragma once

#include "StlUtils.h"


struct SDynamicObjectHideSpot
{
    Vec3 pos, dir;
    EntityId    entityId;
    unsigned int nodeIndex;

    SDynamicObjectHideSpot(const Vec3& _pos = ZERO, const Vec3& _dir = ZERO, EntityId id = 0, unsigned int index = 0)
        : pos(_pos)
        , dir(_dir)
        , entityId(id)
        , nodeIndex(index) {}
};


class CAIDynHideObjectManager
{
public:
    CAIDynHideObjectManager();

    void Reset();

    void GetHidePositionsWithinRange(std::vector<SDynamicObjectHideSpot>& hideSpots, const Vec3& pos, float radius,
        IAISystem::tNavCapMask navCapMask, float passRadius, unsigned int lastNavNodeIndex = 0);

    bool ValidateHideSpotLocation(const Vec3& pos, const SAIBodyInfo& bi, EntityId objectEntId);

    void DebugDraw();

private:

    void ResetCache();
    void FreeCacheItem(int i);
    int GetNewCacheItem();
    unsigned int GetPositionHashFromEntity(IEntity* pEntity);
    void InvalidateHideSpotLocation(const Vec3& pos, EntityId objectEntId);

    struct SCachedDynamicObject
    {
        EntityId id;
        unsigned int positionHash;
        std::vector<SDynamicObjectHideSpot> spots;
        CTimeValue timeStamp;
    };

    typedef VectorMap<EntityId, unsigned int> DynamicOHideObjectMap;

    DynamicOHideObjectMap m_cachedObjects;
    std::vector<SCachedDynamicObject> m_cache;
    std::vector<int> m_cacheFreeList;
};

#endif // CRYINCLUDE_CRYAISYSTEM_AIDYNHIDEOBJECTMANAGER_H
