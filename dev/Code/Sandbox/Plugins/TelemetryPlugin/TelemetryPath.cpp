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
#include "TelemetryPath.h"
#include "Viewport.h"
#include "TelemetryRepository.h"

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CTelemetryPathObject, CBaseObject)
//////////////////////////////////////////////////////////////////////////

CTelemetryPathObject::CTelemetryPathObject()
{ }

//////////////////////////////////////////////////////////////////////////

void CTelemetryPathObject::FinalConstruct(CTelemetryRepository* repo, CTelemetryTimeline* timeline)
{
    static const COLORREF TELEM_CLR_AI = RGB(0, 0, 255);
    static const COLORREF TELEM_CLR_PLAYER = RGB(255, 0, 0);
    static const COLORREF TELEM_CLR_PLAYER_AI_TEAM = RGB(0, 255, 255);

    m_repository = repo;
    m_path = timeline;

    COLORREF clr;
    switch (m_path->getOwner()->getType())
    {
    case eStatOwner_AI:
        clr = TELEM_CLR_AI;
        break;
    case eStatOwner_Player:
        clr = TELEM_CLR_PLAYER;
        break;
    case eStatOwner_Player_AITeam:
        clr = TELEM_CLR_PLAYER_AI_TEAM;
        break;
    }

    ChangeColor(clr);

    TTelemNodePtr owner = m_path->getOwner();

    typedef STelemetryNode::TStates OwnerStates;
    const OwnerStates& states = owner->getStates();

    if (states.size())
    {
        for (OwnerStates::const_iterator it = states.begin(); it != states.end(); ++it)
        {
            m_variables.push_back(CSmartVariable<CString>());
            m_variables.back() = it->second.c_str();
            AddVariable(m_variables.back(), it->first.c_str());
        }

        OwnerStates::const_iterator it = states.find("name");
        if (it != states.end())
        {
            SetName(it->second.c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////

CTelemetryPathObject::~CTelemetryPathObject()
{
}

//////////////////////////////////////////////////////////////////////////

CTelemetryTimeline& CTelemetryPathObject::getPath()
{
    return *m_path;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryPathObject::DeleteThis()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryPathObject::Done()
{
    m_path->Clear();
    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryPathObject::GetBoundBox(AABB& box)
{
    box.SetTransformedAABB(GetWorldTM(), m_path->getBBox());
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryPathObject::GetLocalBounds(AABB& box)
{
    box = m_path->getBBox();
}

//////////////////////////////////////////////////////////////////////////

bool CTelemetryPathObject::HitTest(HitContext& hc)
{
    static const int PATH_LINE_WIDTH = 5;

    // HACK: Sandbox sometimes calls HitTest on deleted objects without checking this flag.
    if (this->CheckFlags(OBJFLAG_DELETED))
    {
        return false;
    }

    const Matrix34& wtm = GetWorldTM();

    for (size_t i = 1, size = m_path->getEventCount(); i != size; ++i)
    {
        Vec3 p0 = wtm.TransformPoint(m_path->getEvent(i - 1).position) + TELEM_VERTICAL_ADJ;
        Vec3 p1 = wtm.TransformPoint(m_path->getEvent(i).position) + TELEM_VERTICAL_ADJ;

        if (hc.view->HitTestLine(p0, p1, hc.point2d, PATH_LINE_WIDTH))
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////

bool CTelemetryPathObject::HitTestRect(HitContext& hc)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryPathObject::Display(DisplayContext& dc)
{
    static const float POINT_SIZE = 0.003f;

    if (m_path->getEventCount() < 2 || m_repository->getRenderMode() == eTLRM_Density)
    {
        return;
    }

    bool selected = IsSelected() || IsHighlighted();

    dc.SetColor(selected ? dc.GetSelectedColor() : GetColor());

    for (size_t i = 1, size = m_path->getEventCount(); i != size; ++i)
    {
        dc.DrawLine(m_path->getEvent(i - 1).position + TELEM_VERTICAL_ADJ, m_path->getEvent(i).position + TELEM_VERTICAL_ADJ);
    }


    if (selected)
    {
        dc.DepthTestOff();
    }

    for (size_t i = 0, size = m_path->getEventCount(); i != size; ++i)
    {
        Vec3 pnt = m_path->getEvent(i).position;

        float fScale = POINT_SIZE * dc.view->GetScreenScaleFactor(pnt);
        Vec3 sz(fScale, fScale, fScale);

        dc.DrawWireBox(pnt - sz  + TELEM_VERTICAL_ADJ, pnt + sz  + TELEM_VERTICAL_ADJ);
    }

    if (selected)
    {
        dc.DepthTestOn();
    }
}

//////////////////////////////////////////////////////////////////////////
