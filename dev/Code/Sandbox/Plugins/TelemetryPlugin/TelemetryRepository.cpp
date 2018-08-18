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

#include "stdafx.h"
#include "TelemetryRepository.h"
#include "TelemetryPath.h"
#include "TelemetryEventTimeline.h"
#include "I3DEngine.h"
#include "Objects/ObjectLayerManager.h"

//////////////////////////////////////////////////////////////////////////

CTelemetryRepository::CTelemetryRepository()
    : m_octree(STelemOctreeAgent()
        , AABB(AABB::RESET), 10, 15, 5.0f)
    , m_eventView(0)
    , m_renderMode(eTLRM_Markers)
    , m_telemLayer(0)
{
    GetIEditor()->GetClassFactory()->RegisterClass(new CTelemetryPathObjectClassDesc);
    GetIEditor()->GetClassFactory()->RegisterClass(new CTelemetryEventTimelineClassDesc);

    ClearData(false);
}

//////////////////////////////////////////////////////////////////////////

CTelemetryRepository::~CTelemetryRepository()
{
}

//////////////////////////////////////////////////////////////////////////

CObjectLayer* CTelemetryRepository::getTelemetryLayer()
{
    if (!m_telemLayer)
    {
        CObjectLayerManager* pLayerMgr = GetIEditor()->GetObjectManager()->GetLayersManager();
        m_telemLayer = pLayerMgr->FindLayerByName("Telemetry");

        if (!m_telemLayer)
        {
            m_telemLayer = pLayerMgr->CreateLayer();
            m_telemLayer->SetName("Telemetry");
            m_telemLayer->SetExportable(false);
        }
    }
    return m_telemLayer;
}

//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline* CTelemetryRepository::AddTimeline(CTelemetryTimeline& toAdd)
{
    CTelemetryTimeline* ntl = new CTelemetryTimeline();
    ntl->Swap(toAdd);
    m_timelines.push_back(ntl);

    if (CONST_TEMP_STRING(ntl->getName()) != "position")
    {
        for (size_t i = 0, size = ntl->getEventCount(); i != size; ++i)
        {
            m_octree.Insert(&ntl->getEvent(i));
        }

        if (!m_eventView)
        {
            m_eventView = static_cast<CTelemetryEventTimeline*>(GetIEditor()->NewObject("TelemetryEventTimeline"));
            m_eventView->FinalConstruct(this);
            m_eventView->SetLayer(getTelemetryLayer());
        }
    }

    return ntl;
}

CTelemetryPathObject* CTelemetryRepository::CreatePathObject(CTelemetryTimeline* timeline)
{
    CTelemetryPathObject* path = static_cast<CTelemetryPathObject*>(GetIEditor()->NewObject("TelemetryPath"));
    path->FinalConstruct(this, timeline);
    path->SetLayer(getTelemetryLayer());

    m_viewPaths.push_back(path);
    return path;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryRepository::ClearData(bool clearEditorObjs)
{
    if (clearEditorObjs)
    {
        for (size_t i = 0, size = m_viewPaths.size(); i != size; ++i)
        {
            GetIEditor()->DeleteObject(m_viewPaths[i]);
        }
    }
    m_viewPaths.clear();

    if (GetIEditor()->GetDocument())
    {
        float ts = (float)GetIEditor()->Get3DEngine()->GetTerrainSize();
        m_octree.setWorldBox(AABB(Vec3(0, 0, 0), Vec3(ts, ts, ts)));
    }
    m_octree.Clear();

    for (size_t i = 0, size = m_timelines.size(); i != size; ++i)
    {
        delete m_timelines[i];
    }
    m_timelines.clear();
}

//////////////////////////////////////////////////////////////////////////
