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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_PROXIMITYTRIGGERSYSTEM_H
#define CRYINCLUDE_CRYENTITYSYSTEM_PROXIMITYTRIGGERSYSTEM_H
#pragma once

#include <IProximityTriggerSystem.h>

struct ProximityElementBounds
{
    float m_aabbMinVal;
    float m_aabbMaxVal;
    int m_itemIndex;
};

struct SProximityEvent
{
    bool bEnter; // Enter/Leave.
    AZ::EntityId entity; // Cry EntityId or AZ::EntityId.
    SProximityElement* pTrigger;
    void GetMemoryUsage(ICrySizer* pSizer) const{}
};

//////////////////////////////////////////////////////////////////////////
class CProximityTriggerSystem
    : public ProximityTriggerSystemRequestBus::Handler
{
public:
    CProximityTriggerSystem();
    ~CProximityTriggerSystem();

    //////////////////////////////////////////////////////////////////////////
    // ProximityTriggerSystemRequests::Handler
    SProximityElement* CreateTrigger(SProximityElement::NarrowPassCheckFunction narrowPassChecker = nullptr) override;
    void RemoveTrigger(SProximityElement* pTrigger) override;
    void MoveTrigger(SProximityElement* pTrigger, const AABB& aabb, bool invalidateCachedAABB = false) override;

    SProximityElement* CreateEntity(AZ::EntityId id) override;
    void MoveEntity(SProximityElement* pEntity, const Vec3& pos, const AABB& aabb = AABB(Vec3(0))) override;
    void RemoveEntity(SProximityElement* pEntity, bool instantEvent = false) override;
    //////////////////////////////////////////////////////////////////////////

    void Update();
    void Reset();
    void BeginReset();

    void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    void CheckIfLeavingTrigger(SProximityElement* pEntity);
    void ProcessOverlap(SProximityElement* pEntity, SProximityElement* pTrigger);
    void RemoveFromTriggers(SProximityElement* pEntity, bool instantEvent = false);
    void PurgeRemovedTriggers();
    void SortTriggers();
    void SendEvents();
    void SendEvent(EEntityEvent eventId, EntityId triggerId, EntityId entityId);

private:
    typedef std::vector<SProximityElement*> Elements;

    Elements m_triggers;
    Elements m_triggersToRemove;
    Elements m_entitiesToRemove;
    bool m_triggerArrayChanged;
    bool m_bResetting;

    std::vector<SProximityElement*> m_entities;
    std::vector<AABB> m_triggersAABB;

    std::vector<SProximityEvent> m_events;

    std::vector<ProximityElementBounds> m_aabbListEntities;
    std::vector<ProximityElementBounds> m_aabbListTriggers;

public:
    typedef stl::PoolAllocatorNoMT<sizeof(SProximityElement)> ProximityElement_PoolAlloc;
    static ProximityElement_PoolAlloc* g_pProximityElement_PoolAlloc;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_PROXIMITYTRIGGERSYSTEM_H
