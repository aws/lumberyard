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
#include "WorldMonitor.h"
#include "Navigation/NavigationSystem/NavigationSystem.h"
#include "DebugDrawContext.h"


//#pragma optimize("", off)
//#pragma inline_depth(0)


WorldMonitor::WorldMonitor()
    : m_callback(0)
    , m_enabled(gEnv->IsEditor())
{
}

WorldMonitor::WorldMonitor(const Callback& callback)
    : m_callback(callback)
    , m_enabled(gEnv->IsEditor())
{
}

void WorldMonitor::Start()
{
    if (IsEnabled())
    {
        if (m_callback)
        {
            gEnv->pPhysicalWorld->AddEventClient(EventPhysStateChange::id, WorldMonitor::StateChangeHandler, 1, 1.0f);
            gEnv->pPhysicalWorld->AddEventClient(EventPhysEntityDeleted::id, WorldMonitor::EntityRemovedHandler, 1, 1.0f);
        }
    }
}

void WorldMonitor::Stop()
{
    if (IsEnabled())
    {
        if (m_callback)
        {
            gEnv->pPhysicalWorld->RemoveEventClient(EventPhysStateChange::id, WorldMonitor::StateChangeHandler, 1);
            gEnv->pPhysicalWorld->RemoveEventClient(EventPhysEntityDeleted::id, WorldMonitor::EntityRemovedHandler, 1);
        }
    }
}

bool WorldMonitor::IsEnabled() const
{
    return m_enabled;
}

int WorldMonitor::StateChangeHandler(const EventPhys* pPhysEvent)
{
    WorldMonitor* pthis = gAIEnv.pNavigationSystem->GetWorldMonitor();

    assert(pthis != NULL);
    assert(pthis->IsEnabled());

    {
        const EventPhysStateChange* event = (EventPhysStateChange*)pPhysEvent;

        bool consider = false;

        if (event->iSimClass[1] == SC_STATIC)
        {
            consider = true;
        }
        else
        {
            IPhysicalEntity* physEnt = event->pEntity;

            if ((event->iSimClass[1] == SC_SLEEPING_RIGID) ||
                (event->iSimClass[1] == SC_ACTIVE_RIGID) ||
                (physEnt->GetType() == PE_RIGID))
            {
                consider = NavigationSystemUtils::IsDynamicObjectPartOfTheMNMGenerationProcess(physEnt);
            }
        }

        if (consider)
        {
            const AABB aabbOld(event->BBoxOld[0], event->BBoxOld[1]);
            const AABB aabbNew(event->BBoxNew[0], event->BBoxNew[1]);

            if (((aabbOld.min - aabbNew.min).len2() + (aabbOld.max - aabbNew.max).len2()) > 0.0f)
            {
                pthis->m_callback(aabbOld);
                pthis->m_callback(aabbNew);

                if (gAIEnv.CVars.DebugDrawNavigationWorldMonitor)
                {
                    CDebugDrawContext dc;

                    dc->DrawAABB(aabbOld, IDENTITY, false, Col_DarkGray, eBBD_Faceted);
                    dc->DrawAABB(aabbNew, IDENTITY, false, Col_White, eBBD_Faceted);
                }
            }
        }
    }

    return 1;
}

int WorldMonitor::EntityRemovedHandler(const EventPhys* pPhysEvent)
{
    WorldMonitor* pthis = gAIEnv.pNavigationSystem->GetWorldMonitor();

    assert(pthis != NULL);
    assert(pthis->IsEnabled());

    {
        const EventPhysEntityDeleted* event = (EventPhysEntityDeleted*)pPhysEvent;

        bool consider = false;

        IPhysicalEntity* physEnt = event->pEntity;
        pe_type type = physEnt->GetType();

        if (type == PE_STATIC)
        {
            consider = true;
        }
        else if (type == PE_RIGID)
        {
            consider = NavigationSystemUtils::IsDynamicObjectPartOfTheMNMGenerationProcess(physEnt);
        }

        if (consider)
        {
            pe_status_pos sp;

            if (physEnt->GetStatus(&sp))
            {
                const AABB aabb(sp.BBox[0], sp.BBox[1]);
                pthis->m_callback(aabb);

                if (gAIEnv.CVars.DebugDrawNavigationWorldMonitor)
                {
                    CDebugDrawContext dc;

                    dc->DrawAABB(aabb, IDENTITY, false, Col_White, eBBD_Faceted);
                }
            }
        }
    }

    return 1;
}
