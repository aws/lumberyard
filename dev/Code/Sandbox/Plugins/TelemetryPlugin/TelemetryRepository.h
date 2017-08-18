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

// Description : Storage class for all telemetry data


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYREPOSITORY_H
#define CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYREPOSITORY_H
#pragma once


#include "CrySizer.h"
#include "IGameStatistics.h"
#include "TelemetryObjects.h"
#include "PoolAllocator.h"
#include "Octree.h"

class CObjectLayer;
class CTelemetryPathObject;
class CTelemetryEventTimeline;

//////////////////////////////////////////////////////////////////////////

enum ETelemRenderMode
{
    eTLRM_Markers,
    eTLRM_Density,
};

#define TELEM_VERTICAL_ADJ Vec3(0, 0, 1)

//////////////////////////////////////////////////////////////////////////

struct STelemOctreeAgent
{
    Vec3 getPosition(STelemetryEvent* evnt) const
    {
        return evnt->position;
    }
};

//////////////////////////////////////////////////////////////////////////

class CTelemetryRepository
{
public:
    typedef COctree<STelemetryEvent*, STelemOctreeAgent> TTelemOctree;

    CTelemetryRepository();
    ~CTelemetryRepository();

    TTelemOctree* getOctree() { return &m_octree; }

    ETelemRenderMode getRenderMode() const { return m_renderMode; }
    void setRenderMode(ETelemRenderMode mode) { m_renderMode = mode; }

    CTelemetryPathObject* CreatePathObject(CTelemetryTimeline* timeline);
    void ClearData(bool clearEditorObjs);

    CTelemetryTimeline* AddTimeline(CTelemetryTimeline& toAdd);

    CObjectLayer* getTelemetryLayer();

    STelemetryEvent* newEvent() { return new(m_eventAllocator.Allocate())STelemetryEvent(); }
    STelemetryEvent* newEvent(STelemetryEvent& e)
    {
        STelemetryEvent* ne = newEvent();
        std::swap(*ne, e);
        return ne;
    }

    void deleteEvent(STelemetryEvent* e)
    {
        if (e)
        {
            e->~STelemetryEvent();
            m_eventAllocator.Deallocate(e);
        }
    }

private:
    CTelemetryRepository(const CTelemetryRepository& other);
    CTelemetryRepository& operator=(const CTelemetryRepository& rhs);

private:
    typedef std::vector<CTelemetryTimeline*> TTimelineContainer;
    typedef stl::TPoolAllocator<STelemetryEvent, stl::PSyncNone, 16> TEventAllocator;

    typedef std::vector<CTelemetryPathObject*> TViewPathContainer;

    TTelemOctree m_octree;
    TTimelineContainer m_timelines;
    TViewPathContainer m_viewPaths;
    CTelemetryEventTimeline* m_eventView;
    TEventAllocator m_eventAllocator;
    ETelemRenderMode m_renderMode;
    CObjectLayer* m_telemLayer;
};

//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYREPOSITORY_H
