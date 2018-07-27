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
#include "ProximityTriggerSystem.h"
#include "Components/ComponentTrigger.h"

#include <MathConversion.h>

#include <AzCore/Casting/numeric_cast.h>

#define AXIS_0 0
#define AXIS_1 1
#define AXIS_2 2

#define ENTITY_RADIUS (0.5f)

#define GET_ENTITY_NAME(eid) gEnv->pEntitySystem->GetEntity(eid)->GetName()

namespace
{
    struct InOut
    {
        float pos;
        // the following fields can take the value -1, 0, or 1, and are used to increment/decrement/keep-the-same
        // the inA and inB arrays in the routine below.
        int a;
        int b;
        int test;

        bool operator<(const InOut& rhs) const
        {
            return pos < rhs.pos;
        }
    };
}
// find the remainder from a-b
static const int MAX_AABB_DIFFERENCES = 4 * 4 * 4;
static void AABBDifference(const AABB& a, const AABB& b, AABB* differences, int* ndifferences)
{
    *ndifferences = 0;

    static const float SMALL_SIZE = 5;

    if (b.GetVolume() < SMALL_SIZE * SMALL_SIZE * SMALL_SIZE || !a.IsIntersectBox(b))
    {
        differences[(*ndifferences)++] = b;
    }
    else
    {
        InOut inout[3][4];
        for (int i = 0; i < 3; i++)
        {
#define FILL_INOUT(n, p, aiv, aov, biv, bov) inout[i][n].pos = p[i]; inout[i][n].a = aiv - aov; inout[i][n].b = biv - bov
            FILL_INOUT(0, a.min, 1, 0, 0, 0);
            FILL_INOUT(1, a.max, 0, 1, 0, 0);
            FILL_INOUT(2, b.min, 0, 0, 1, 0);
            FILL_INOUT(3, b.max, 0, 0, 0, 1);
#undef FILL_INOUT
            std::sort(inout[i], inout[i] + 4);
            for (int j = 0; j < 3; j++)
            {
                inout[i][j].test = (inout[i][j].pos != inout[i][j + 1].pos);
            }
            inout[i][3].test = 0;
        }
        // tracks whether we're inside or outside the AABB's a and b (1 if we're inside)
        int inA[3] = {0, 0, 0};
        int inB[3] = {0, 0, 0};
        for (int ix = 0; ix < 4; ix++)
        {
            inA[0] += inout[0][ix].a;
            inB[0] += inout[0][ix].b;
            if (inout[0][ix].test)
            {
                for (int iy = 0; iy < 4; iy++)
                {
                    inA[1] += inout[1][iy].a;
                    inB[1] += inout[1][iy].b;
                    if (inout[1][iy].test)
                    {
                        for (int iz = 0; iz < 4; iz++)
                        {
                            inA[2] += inout[2][iz].a;
                            inB[2] += inout[2][iz].b;
                            if (inout[2][iz].test)
                            {
                                if (inB[0] && inB[1] && inB[2])
                                {
                                    if (!(inA[0] && inA[1] && inA[2]))
                                    {
                                        differences[*ndifferences] = AABB(
                                                Vec3(inout[0][ix].pos, inout[1][iy].pos, inout[2][iz].pos),
                                                Vec3(inout[0][ix + 1].pos, inout[1][iy + 1].pos, inout[2][iz + 1].pos));
                                        if (differences[*ndifferences].GetVolume())
                                        {
                                            (*ndifferences)++;
                                        }
                                        else
                                        {
                                            assert(false);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool IndexedAABBCompare(ProximityElementBounds i, ProximityElementBounds j)
{
    return (i.m_aabbMinVal < j.m_aabbMinVal);
}

//////////////////////////////////////////////////////////////////////////
CProximityTriggerSystem::ProximityElement_PoolAlloc* CProximityTriggerSystem::g_pProximityElement_PoolAlloc = 0;

//////////////////////////////////////////////////////////////////////////
CProximityTriggerSystem::CProximityTriggerSystem()
{
    m_triggerArrayChanged = false;
    m_bResetting = false;

    g_pProximityElement_PoolAlloc = new ProximityElement_PoolAlloc;

    ProximityTriggerSystemRequestBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CProximityTriggerSystem::~CProximityTriggerSystem()
{
    delete g_pProximityElement_PoolAlloc;
}

//////////////////////////////////////////////////////////////////////////
SProximityElement* CProximityTriggerSystem::CreateTrigger(SProximityElement::NarrowPassCheckFunction narrowPassChecker)
{
    // Use pool allocator here.
    void* elementMem = static_cast<SProximityElement*>(CProximityTriggerSystem::g_pProximityElement_PoolAlloc->Allocate());
    SProximityElement* pTrigger = new (elementMem) SProximityElement;
    pTrigger->m_narrowPassChecker = narrowPassChecker;
    m_triggers.push_back(pTrigger);
    m_triggerArrayChanged = true;
    return pTrigger;
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::RemoveTrigger(SProximityElement* pTrigger)
{
    m_triggersToRemove.push_back(pTrigger);
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::MoveTrigger(SProximityElement* pTrigger, const AABB& aabb, bool invalidateCachedAABB)
{
    pTrigger->aabb = aabb;
    pTrigger->bActivated = true;
    m_triggerArrayChanged = true;

    if (invalidateCachedAABB)
    {
        // notify each entity which is inside
        uint32 num = (uint32)pTrigger->inside.size();
        for (uint32 i = 0; i < num; ++i)
        {
            pTrigger->inside[i]->RemoveInside(pTrigger);
        }

        pTrigger->inside.clear();
        // marcok: find the trigger (not ideal, but I don't have an index I can access here)
        num = (uint32)m_triggers.size();
        for (uint32 i = 0; i < num; ++i)
        {
            if (m_triggers[i] == pTrigger && m_triggersAABB.size() > i)
            {
                m_triggersAABB.push_back(AABB(ZERO));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::SortTriggers()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

    if (m_triggerArrayChanged)
    {
        m_triggerArrayChanged = false;
        m_aabbListTriggers.clear();
        m_aabbListTriggers.reserve(m_triggers.size());
        m_triggersAABB.clear();
        m_triggersAABB.reserve(m_triggers.size());
        if (!m_triggers.empty())
        {
            uint32 num = (uint32)m_triggers.size();
            for (uint32 i = 0; i < num; ++i)
            {
                SProximityElement* pTrigger = m_triggers[i];
                if (pTrigger->bActivated && m_triggersAABB.size() > i)
                {
                    pTrigger->bActivated = false;
                    // Check if any entity inside this trigger is leaving.
                    for (int j = 0; j < (int)pTrigger->inside.size(); )
                    {
                        SProximityElement* pEntity = pTrigger->inside[j];
                        if (!pTrigger->aabb.IsIntersectBox(pEntity->aabb))
                        {
                            // Entity Leaving this trigger.
                            pTrigger->inside.erase(pTrigger->inside.begin() + j);
                            if (pEntity->RemoveInside(pTrigger))
                            {
                                // Add leave event.
                                SProximityEvent event;
                                event.bEnter = false;
                                event.entity = pEntity->id;
                                event.pTrigger = pTrigger;
                                m_events.push_back(event);
                            }
                            else
                            {
                                assert(0); // Should never happen.
                            }
                        }
                        else
                        {
                            j++;
                        }
                    }
                    // check if anything new needs to be included
                    AABB differences[MAX_AABB_DIFFERENCES];
                    int ndifferences;
                    AABBDifference(m_triggersAABB[i], m_triggers[i]->aabb, differences, &ndifferences);
                    for (int j = 0; j < ndifferences; j++)
                    {
                        SEntityProximityQuery q;
                        q.box = differences[j];
                        gEnv->pEntitySystem->QueryProximity(q);
                        for (int k = 0; k < q.nCount; k++)
                        {
                            IEntity* pEnt = q.pEntities[k];
                            if (pEnt)
                            {
                                if (SProximityElement* pProxElem = pEnt->GetProximityElement())
                                {
                                    Vec3 pos = pEnt->GetWorldPos();
                                    MoveEntity(pProxElem, pos);
                                }
                            }
                        }
                    }
                }
                m_triggersAABB.push_back(m_triggers[i]->aabb);
                ProximityElementBounds tempBounds = { m_triggers[i]->aabb.min[AXIS_0], m_triggers[i]->aabb.max[AXIS_0], aznumeric_cast<int>(i) };
                m_aabbListTriggers.push_back(tempBounds);
            }
            std::sort(m_aabbListTriggers.begin(), m_aabbListTriggers.end(), IndexedAABBCompare);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
inline bool IsIntersectInAxis(const AABB& a, const AABB& b, uint32 axis)
{
    if (a.max[axis] < b.min[axis] || b.max[axis] < a.min[axis])
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::ProcessOverlap(SProximityElement* pEntity, SProximityElement* pTrigger)
{
    if (pEntity->AddInside(pTrigger))
    {
        pTrigger->AddInside(pEntity);

        // Add enter event.
        SProximityEvent event;
        event.bEnter = true;
        event.entity = pEntity->id;
        event.pTrigger = pTrigger;
        m_events.push_back(event);
    }

    //CryLogAlways( "[Trigger] Overlap %s and %s",GET_ENTITY_NAME(pEntity->id),GET_ENTITY_NAME(pTrigger->id) );
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::Update()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);

    if (!m_triggersToRemove.empty())
    {
        PurgeRemovedTriggers();
    }

    SortTriggers();

    if (!m_entities.empty())
    {
        uint32 num = (uint32)m_entities.size();
        m_aabbListEntities.clear();
        m_aabbListEntities.reserve(num);
        for (uint32 i = 0; i < num; ++i)
        {
            m_entities[i]->bActivated = false;
            CheckIfLeavingTrigger(m_entities[i]);
            ProximityElementBounds tempBounds = { m_entities[i]->aabb.min[AXIS_0], m_entities[i]->aabb.max[AXIS_0], aznumeric_cast<int>(i) };
            m_aabbListEntities.push_back(tempBounds);
        }

        std::sort(m_aabbListEntities.begin(), m_aabbListEntities.end(), IndexedAABBCompare);

        auto currentEntity = m_aabbListEntities.begin();
        for (auto currentTrigger = m_aabbListTriggers.begin(); currentTrigger != m_aabbListTriggers.end(); ++currentTrigger)
        {
            // Early cache friendly invalidation check using the sorted array data
            while (currentEntity != m_aabbListEntities.end() && (*currentEntity).m_aabbMaxVal < (*currentTrigger).m_aabbMinVal)
            {
                currentEntity++;
            }

            for (auto checkEntity = currentEntity; checkEntity != m_aabbListEntities.end(); ++checkEntity)
            {
                // This is also a cache friendly check, using the sorted array
                if ((*checkEntity).m_aabbMinVal < (*currentTrigger).m_aabbMaxVal)
                {
                    // These are more expensive checks where we fetch entity data that may cause some cache stomping
                    if (IsIntersectInAxis(m_entities[(*checkEntity).m_itemIndex]->aabb, m_triggersAABB[(*currentTrigger).m_itemIndex], AXIS_1))
                    {
                        if (IsIntersectInAxis(m_entities[(*checkEntity).m_itemIndex]->aabb, m_triggersAABB[(*currentTrigger).m_itemIndex], AXIS_2))
                        {
                            SProximityElement::NarrowPassCheckFunction narrowPassChecker = m_triggers[(*currentTrigger).m_itemIndex]->m_narrowPassChecker;
                            // narrow pass entry check
                            if (narrowPassChecker)
                            {
                                Vec3 entityPosition = m_entities[(*checkEntity).m_itemIndex]->aabb.GetCenter();
                                if (!narrowPassChecker(LYVec3ToAZVec3(entityPosition)))
                                {
                                    continue;
                                }
                            }
                            ProcessOverlap(m_entities[(*checkEntity).m_itemIndex], m_triggers[(*currentTrigger).m_itemIndex]);
                        }
                    }
                }
                else
                {
                    // Once we have found an invalid entity, no other entities will be valid for this trigger
                    break;
                }
            }
        }

        // We repopulate the entity list every frame when entities are moved, so we clear it now
        m_entities.clear();
    }

    if (!m_events.empty())
    {
        SendEvents();
    }
}



//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::SendEvent(EEntityEvent eventId, EntityId triggerId, EntityId entityId)
{
    IEntity* pTriggerEntity = gEnv->pEntitySystem->GetEntity(triggerId);
    if (pTriggerEntity)
    {
        SEntityEvent event;

        event.event = eventId;
        event.nParam[0] = entityId;
        event.nParam[1] = 0;
        event.nParam[2] = triggerId;
        pTriggerEntity->SendEvent(event);
    }
}


//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::SendEvents()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_ENTITY, g_bProfilerEnabled);
    for (uint32 i = 0; i < m_events.size(); i++)
    {
        // Legacy notifications.
        EEntityEvent eventId = m_events[i].bEnter ? ENTITY_EVENT_ENTERAREA : ENTITY_EVENT_LEAVEAREA;
        if (IsLegacyEntityId(m_events[i].pTrigger->id) &&
            IsLegacyEntityId(m_events[i].entity))
        {
            SendEvent(eventId, GetLegacyEntityId(m_events[i].pTrigger->id), GetLegacyEntityId(m_events[i].entity));
        }

        // Ebus-based notifications.
        if (eventId == ENTITY_EVENT_ENTERAREA)
        {
            EBUS_EVENT_ID(m_events[i].pTrigger->id, ProximityTriggerEventBus, OnTriggerEnter, m_events[i].entity);
        }
        else if (eventId == ENTITY_EVENT_LEAVEAREA)
        {
            EBUS_EVENT_ID(m_events[i].pTrigger->id, ProximityTriggerEventBus, OnTriggerExit, m_events[i].entity);
        }
    }
    m_events.clear();
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::PurgeRemovedTriggers()
{
    for (int i = 0; i < (int)m_triggersToRemove.size(); i++)
    {
        SProximityElement* pTrigger = m_triggersToRemove[i];
        if (pTrigger)
        {
            PREFAST_ASSUME(pTrigger);
            for (int j = 0; j < (int)pTrigger->inside.size(); j++)
            {
                SProximityElement* pEntity = pTrigger->inside[j];
                pEntity->RemoveInside(pTrigger);
            }

            stl::find_and_erase(m_triggers, pTrigger);
            CProximityTriggerSystem::g_pProximityElement_PoolAlloc->Deallocate(pTrigger);
            m_triggerArrayChanged =  true;
            m_triggersToRemove[i] = nullptr;
        }
    }
    m_triggersToRemove.clear();
}

//////////////////////////////////////////////////////////////////////////
SProximityElement* CProximityTriggerSystem::CreateEntity(AZ::EntityId id)
{
    if (m_bResetting)
    {
        return nullptr;
    }

    SProximityElement* pEntity = new SProximityElement;
    pEntity->id = id;
    return pEntity;
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::RemoveEntity(SProximityElement* pEntity, bool instantEvent)
{
    if (m_bResetting)
    {
        m_entitiesToRemove.push_back(pEntity);
        return;
    }

    // Remove it from all triggers.
    RemoveFromTriggers(pEntity, instantEvent);
    if (!m_entities.empty())
    {
        stl::find_and_erase(m_entities, pEntity);
    }
    delete pEntity;
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::MoveEntity(SProximityElement* pEntity, const Vec3& pos, const AABB &aabb)
{
    if (m_bResetting)
    {
        return;
    }

    if (aabb.IsEmpty())
    {
        // Entity 1 meter
        pEntity->aabb.min = pos - Vec3(ENTITY_RADIUS, ENTITY_RADIUS, 0);
        pEntity->aabb.max = pos + Vec3(ENTITY_RADIUS, ENTITY_RADIUS, ENTITY_RADIUS);
    }
    else
    {
        pEntity->aabb = aabb;
    }
    
    if (!pEntity->bActivated)
    {
        pEntity->bActivated = true;
        m_entities.push_back(pEntity);
    }
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::RemoveFromTriggers(SProximityElement* pEntity, bool instantEvent)
{
    int num = (int)pEntity->inside.size();
    for (int i = 0; i < num; ++i)
    {
        SProximityElement* pTrigger = pEntity->inside[i];
        if (pTrigger && pTrigger->RemoveInside(pEntity))
        {
            if (instantEvent)
            {
                SendEvent(ENTITY_EVENT_LEAVEAREA, aznumeric_cast<EntityId>((AZ::u64)pTrigger->id), aznumeric_cast<EntityId>((AZ::u64)pEntity->id));
            }
            else
            {
                // Add leave event.
                SProximityEvent event;
                event.bEnter = false;
                event.entity = pEntity->id;
                event.pTrigger = pTrigger;
                m_events.push_back(event);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::CheckIfLeavingTrigger(SProximityElement* pEntity)
{
    // Check if we are still inside stored triggers.
    for (int i = 0; i < (int)pEntity->inside.size(); )
    {
        SProximityElement* pTrigger = pEntity->inside[i];

        bool narrowPass = true;
        if (pTrigger->m_narrowPassChecker)
        {
            Vec3 entityPosition = pEntity->aabb.GetCenter();
            if (!pTrigger->m_narrowPassChecker(LYVec3ToAZVec3(entityPosition)))
            {
                narrowPass = false;
            }
        }

        if (!narrowPass || !pTrigger->aabb.IsIntersectBox(pEntity->aabb))
        {
            // Leaving this trigger.
            pEntity->inside.erase(pEntity->inside.begin() + i);
            if (pTrigger->RemoveInside(pEntity))
            {
                // Add leave event.
                SProximityEvent event;
                event.bEnter = false;
                event.entity = pEntity->id;
                event.pTrigger = pTrigger;
                m_events.push_back(event);
            }
            else
            {
                assert(0); // Should not happen that we wasn't inside trigger.
            }
        }
        else
        {
            i++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CProximityTriggerSystem::Reset()
{
    // by this stage nothing else should care about being notified about anything, so just delete the entities and triggers
    for (uint32 i = 0; i < (uint32)m_entitiesToRemove.size(); i++)
    {
        delete m_entitiesToRemove[i];
    }

    for (uint32 i = 0; i < (uint32)m_triggersToRemove.size(); i++)
    {
        CProximityTriggerSystem::g_pProximityElement_PoolAlloc->Deallocate(m_triggersToRemove[i]);
    }

    m_entities.clear();
    m_triggersAABB.clear();
    m_events.clear();
    m_aabbListEntities.clear();
    m_aabbListTriggers.clear();
    m_triggers.clear();
    m_triggersToRemove.clear();
    m_entitiesToRemove.clear();

    // use swap trick to deallocate memory in vector
    std::vector<SProximityElement*>(m_entities).swap(m_entities);
    std::vector<AABB>(m_triggersAABB).swap(m_triggersAABB);
    std::vector<SProximityEvent>(m_events).swap(m_events);
    std::vector<ProximityElementBounds>(m_aabbListEntities).swap(m_aabbListEntities);
    std::vector<ProximityElementBounds>(m_aabbListTriggers).swap(m_aabbListTriggers);
    Elements(m_triggers).swap(m_triggers);
    Elements(m_triggersToRemove).swap(m_triggersToRemove);
    Elements(m_entitiesToRemove).swap(m_entitiesToRemove);

    g_pProximityElement_PoolAlloc->FreeMemory();

    m_bResetting = false;
}

void CProximityTriggerSystem::BeginReset()
{
    // When we begin resetting we ignore all calls to the trigger system until call to the Reset.
    m_bResetting = true;
}

void CProximityTriggerSystem::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(g_pProximityElement_PoolAlloc);
    pSizer->AddContainer(m_triggers);
    pSizer->AddContainer(m_triggersToRemove);
    pSizer->AddContainer(m_entities);
    pSizer->AddContainer(m_triggersAABB);
    pSizer->AddContainer(m_events);
    pSizer->AddContainer(m_aabbListEntities);
    pSizer->AddContainer(m_aabbListTriggers);
}