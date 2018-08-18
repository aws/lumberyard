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
#include "TelemetryEventTimeline.h"
#include "Cry_Camera.h"
#include "I3DEngine.h"
#include "Objects/Entity.h"

float CEntity::m_helperScale = 1;

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CTelemetryEventTimeline, CBaseObject)
//////////////////////////////////////////////////////////////////////////

CTelemetryEventTimeline::CTelemetryEventTimeline()
    : m_repository(0)
    , m_octree(0)
    , m_highlightedEvent(0)
    , m_selectedEvent(0)
{ }

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::FinalConstruct(CTelemetryRepository* repo)
{
    m_repository = repo;
    m_octree = repo->getOctree();
}

//////////////////////////////////////////////////////////////////////////

CTelemetryEventTimeline::~CTelemetryEventTimeline()
{ }

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::DeleteThis()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::GetBoundBox(AABB& box)
{
    box.SetTransformedAABB(GetWorldTM(), m_octree->getWorldBox());
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::GetLocalBounds(AABB& box)
{
    box = AABB(m_octree->getWorldBox().min, m_octree->getWorldBox().max);
}

//////////////////////////////////////////////////////////////////////////

bool CTelemetryEventTimeline::HitTest(HitContext& hc)
{
    COctreeHitTestVisitor htv(hc);
    m_octree->AcceptVisitor(htv);
    m_highlightedEvent = htv.getHit();

    if (m_highlightedEvent)
    {
        SetName(m_highlightedEvent->timeline->getName());
    }

    return m_highlightedEvent != 0;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::SetSelected(bool bSelect)
{
    CBaseObject::SetSelected(bSelect);
    m_selectedEvent = IsSelected() ? m_highlightedEvent : 0;

    if (m_selectedEvent)
    {
        SetVariables(m_selectedEvent);
    }
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::SetVariables(const STelemetryEvent* e)
{
    for (size_t i = 0; i != m_variables.size(); ++i)
    {
        RemoveVariable(m_variables[i].GetVar());
    }

    m_variables.clear();

    typedef STelemetryEvent::TStates TEventStates;
    const TEventStates& states = e->states;

    for (TEventStates::const_iterator it = states.begin(); it != states.end(); ++it)
    {
        m_variables.push_back(CSmartVariable<CString>());
        m_variables.back() = it->second.c_str();
        AddVariable(m_variables.back(), it->first.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////

bool CTelemetryEventTimeline::HitTestRect(HitContext& hc)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////

void CTelemetryEventTimeline::Display(DisplayContext& dc)
{
    m_lightCache.ClearLights();

    if (m_repository->getRenderMode() == eTLRM_Markers)
    {
        CMarkerDrawingVisitor dv(dc.camera->GetPosition(), dc, GetTextureIcon(), IsHighlighted() ? m_highlightedEvent : 0, m_selectedEvent);
        m_octree->AcceptVisitor(dv);
    }
    else
    {
        COctreeDensityVisitor densv;
        m_octree->AcceptVisitor(densv);

        CDensityDrawingVisitor dv(m_lightCache, densv.getMaxDensity(), m_repository->getTelemetryLayer());
        m_octree->AcceptVisitor(dv);
    }
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CDMLightCache
//////////////////////////////////////////////////////////////////////////

CDMLightCache::CDMLightCache()
    : m_lastUsed(0)
{
}

//////////////////////////////////////////////////////////////////////////

void CDMLightCache::SetLight(float intensity, const Vec3& pos, CObjectLayer* pLayer)
{
    static const float DENS_LIGHT_RADIUS = 4.0f;
    static const float DENS_LIGHT_DIFFUSE_MUL = 1.0f;
    static const float DENS_LIGHT_HDR_DYNAMIC = 2.0f;

    if (m_lastUsed == m_lights.size())
    {
        CBaseObject* nl = GetIEditor()->NewObject("Entity", "Light");
        m_lights.push_back(nl);

        CVarBlock* vars = static_cast<CEntity*>(nl)->GetProperties();

        IVariable* v = vars->FindVariable("bDeferredLight");
        v->Set(true);

        v = vars->FindVariable("Radius");
        v->Set(DENS_LIGHT_RADIUS);

        v = vars->FindVariable("clrDiffuse");
        v->Set(Vec3(255 * intensity, 255 * (1.0f - intensity), 0));

        v = vars->FindVariable("fDiffuseMultiplier");
        v->Set(DENS_LIGHT_DIFFUSE_MUL);

        v = vars->FindVariable("fHDRDynamic");
        v->Set(DENS_LIGHT_HDR_DYNAMIC);
    }

    CBaseObject* ls = m_lights[m_lastUsed++];
    ls->SetHidden(false);
    ls->SetPos(pos + TELEM_VERTICAL_ADJ);
    ls->SetLayer(pLayer);

    CVarBlock* vars = static_cast<CEntity*>(ls)->GetProperties();
    IVariable* v = vars->FindVariable("clrDiffuse");
    v->Set(Vec3(255 * intensity, 255 * (1.0f - intensity), 0));
}

//////////////////////////////////////////////////////////////////////////

void CDMLightCache::ClearLights()
{
    size_t last = m_lastUsed;
    m_lastUsed = 0;
    for (size_t i = 0; i != last; ++i)
    {
        m_lights[i]->SetHidden(true);
    }
}

//////////////////////////////////////////////////////////////////////////

CDMLightCache::~CDMLightCache()
{
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// COctreeDrawingVisitor
//////////////////////////////////////////////////////////////////////////

CMarkerDrawingVisitor::CMarkerDrawingVisitor(const Vec3& camera, DisplayContext& dc, int textureId, STelemetryEvent* highlighted, STelemetryEvent* selected)
    : m_camPos(camera)
    , m_dc(dc)
    , m_texture(textureId)
    , m_clampDepth(std::numeric_limits<size_t>::max())
    , m_selected(selected)
    , m_highlighted(highlighted)
{
    m_colors[eStatOwner_AI] = RGB(100, 100, 255);
    m_colors[eStatOwner_Player] = RGB(255, 100, 80);
    m_colors[eStatOwner_Player_AITeam] = RGB(0, 255, 255);
}

//////////////////////////////////////////////////////////////////////////

bool CMarkerDrawingVisitor::EnterBranch(branch_type& branch)
{
    const AABB& box = branch.getBox();
    float dist = box.GetDistanceSqr(m_camPos);

    if (dist > 1000.0f)
    {
        m_clampDepth = branch.getDepth();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////

void CMarkerDrawingVisitor::LeaveBranch(branch_type& branch)
{
    if (m_clampDepth == branch.getDepth())
    {
        m_clampDepth = std::numeric_limits<size_t>::max();
    }
}

//////////////////////////////////////////////////////////////////////////

void CMarkerDrawingVisitor::VisitLeaf(leaf_type& leaf)
{
    if (!leaf.getDataSize())
    {
        return;
    }

    bool clamped = m_clampDepth < leaf.getDepth();

    STelemetryEvent** data = leaf.getData();
    for (size_t i = 0,
         size = clamped ? 1 : leaf.getDataSize();
         i != size; ++i)
    {
        DrawEvent(*data[i]);
    }
}

//////////////////////////////////////////////////////////////////////////

void CMarkerDrawingVisitor::DrawEvent(const STelemetryEvent& event)
{
    bool sel = &event == m_selected || &event == m_highlighted;
    COLORREF clr = sel
        ? m_dc.GetSelectedColor()
        : m_colors[event.timeline->getOwner()->getType()];

    m_dc.SetColor(clr, 1);
    m_dc.DrawTextureLabel(event.position + TELEM_VERTICAL_ADJ, OBJECT_TEXTURE_ICON_SIZEX, OBJECT_TEXTURE_ICON_SIZEY, m_texture);
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// COctreeDrawingVisitor
//////////////////////////////////////////////////////////////////////////

CDensityDrawingVisitor::CDensityDrawingVisitor(CDMLightCache& lights, size_t maxDensity, CObjectLayer* pLayer)
    : m_lights(lights)
    , m_maxDensity(maxDensity)
    , m_pLayer(pLayer)
{
}

//////////////////////////////////////////////////////////////////////////

bool CDensityDrawingVisitor::EnterBranch(branch_type& branch)
{
    return true;
}

//////////////////////////////////////////////////////////////////////////

void CDensityDrawingVisitor::LeaveBranch(branch_type& branch)
{
}

//////////////////////////////////////////////////////////////////////////

void CDensityDrawingVisitor::VisitLeaf(leaf_type& leaf)
{
    if (leaf.getDataSize())
    {
        m_lights.SetLight(
            1.0f - (m_maxDensity - leaf.getDataSize()) / static_cast<float>(m_maxDensity),
            leaf.getBox().GetCenter(), m_pLayer);
    }
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// COctreeDensityVisitor
//////////////////////////////////////////////////////////////////////////

COctreeDensityVisitor::COctreeDensityVisitor()
    : m_maxDensity(0)
{ }

//////////////////////////////////////////////////////////////////////////

bool COctreeDensityVisitor::EnterBranch(branch_type& branch)
{
    return true;
}

//////////////////////////////////////////////////////////////////////////

void COctreeDensityVisitor::LeaveBranch(branch_type& branch)
{ }

//////////////////////////////////////////////////////////////////////////

void COctreeDensityVisitor::VisitLeaf(leaf_type& leaf)
{
    m_maxDensity = std::max(leaf.getDataSize(), m_maxDensity);
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// COctreeHitTestVisitor
//////////////////////////////////////////////////////////////////////////

COctreeHitTestVisitor::COctreeHitTestVisitor(const HitContext& hc)
    : m_hitContext(hc)
    , m_ray(hc.raySrc, hc.rayDir)
    , m_hitEvent(0)
    , m_cachedDistSqr(std::numeric_limits<float>::max())
{ }

//////////////////////////////////////////////////////////////////////////

bool COctreeHitTestVisitor::EnterBranch(branch_type& branch)
{
    Vec3 isc;
    return Intersect::Ray_AABB(m_ray, branch.getBox(), isc) != 0;
}

//////////////////////////////////////////////////////////////////////////

void COctreeHitTestVisitor::LeaveBranch(branch_type& branch)
{ }

//////////////////////////////////////////////////////////////////////////

// Point-Ray distance
// returns distance with negative value if closest point on ray line is behind the direction
float Point_RaySq(const Vec3& point, const Ray& ray, Vec3& closestPt)
{
    float distSq = Distance::Point_LineSq(point, ray.origin, ray.origin + ray.direction, closestPt);
    return ray.direction.dot(closestPt - ray.origin) >= 0 ? distSq : -distSq;
}

//////////////////////////////////////////////////////////////////////////

void COctreeHitTestVisitor::VisitLeaf(leaf_type& leaf)
{
    Vec3 isc;
    if (Intersect::Ray_AABB(m_ray, leaf.getBox(), isc) == 0)
    {
        return;
    }

    STelemetryEvent** events = leaf.getData();
    size_t size = leaf.getDataSize();

    for (size_t i = 0; i != size; ++i)
    {
        Vec3 closest;
        float distSq = Point_RaySq(events[i]->position + TELEM_VERTICAL_ADJ, m_ray, closest);

        if (distSq < 0)
        {
            continue;
        }

        if (!m_hitEvent || distSq < m_cachedDistSqr)
        {
            m_hitEvent = events[i];
            m_cachedDistSqr = distSq;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

STelemetryEvent* COctreeHitTestVisitor::getHit() const
{
    return 0.5f >= m_cachedDistSqr
           ? m_hitEvent
           : 0;
}

//////////////////////////////////////////////////////////////////////////
