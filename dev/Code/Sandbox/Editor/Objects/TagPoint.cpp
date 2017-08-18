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

// Description : CTagPoint implementation.


#include "StdAfx.h"
#include "TagPoint.h"
#include <IAgent.h>

#include "../Viewport.h"

#include <IAISystem.h>
#include "../AI/AIManager.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTagPoint::CTagPoint()
{
    m_entityClass = "TagPoint";
}

//////////////////////////////////////////////////////////////////////////
bool CTagPoint::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    SetColor(QColor(0, 0, 255));
    SetTextureIcon(GetClassDesc()->GetTextureIconId());

    bool res = CEntityObject::Init(ie, prev, file);

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CTagPoint::InitVariables()
{
}

//////////////////////////////////////////////////////////////////////////
float CTagPoint::GetRadius()
{
    return GetHelperScale() * gSettings.gizmo.helpersScale * gSettings.gizmo.tagpointScaleMulti;
}

//////////////////////////////////////////////////////////////////////////
void CTagPoint::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);
}

//////////////////////////////////////////////////////////////////////////
void CTagPoint::EndEditParams(IEditor* ie)
{
    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
int CTagPoint::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos;
        if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
        {
            pos = view->MapViewToCP(point);
        }
        else
        {
            // Snap to terrain.
            bool hitTerrain;
            pos = view->ViewToWorld(point, &hitTerrain);
            if (hitTerrain)
            {
                pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y) + 1.0f;
            }
            pos = view->SnapToGrid(pos);
        }

        pos = view->SnapToGrid(pos);
        SetPos(pos);
        if (event == eMouseLDown)
        {
            return MOUSECREATE_OK;
        }
        return MOUSECREATE_CONTINUE;
    }
    return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CTagPoint::Display(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();

    float fHelperScale = 1 * m_helperScale * gSettings.gizmo.helpersScale;
    Vec3 dir = wtm.TransformVector(Vec3(0, fHelperScale, 0));
    Vec3 wp = wtm.GetTranslation();
    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(1, 1, 0);
    }
    dc.DrawArrow(wp, wp + dir * 2, fHelperScale);

    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(GetColor(), 0.8f);
    }

    dc.DrawBall(wp, GetRadius());

    if (IsSelected())
    {
        dc.SetSelectedColor(0.6f);
        dc.DrawBall(wp, GetRadius() + 0.02f);
    }

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
bool CTagPoint::HitTest(HitContext& hc)
{
    // Must use icon..

    Vec3 origin = GetWorldPos();
    float radius = GetRadius();

    Vec3 w = origin - hc.raySrc;
    Vec3 wcross = hc.rayDir.Cross(w);
    float d = wcross.GetLengthSquared();

    if (d < radius * radius + hc.distanceTolerance &&
        w.GetLengthSquared() > radius * radius)             // Check if we inside TagPoint, then do not select!
    {
        Vec3 i0;
        hc.object = this;
        if (Intersect::Ray_SphereFirst(Ray(hc.raySrc, hc.rayDir), Sphere(origin, radius), i0))
        {
            hc.dist = hc.raySrc.GetDistance(i0);
            return true;
        }
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CTagPoint::GetBoundBox(AABB& box)
{
    Vec3 pos = GetWorldPos();
    float r = GetRadius();
    box.min = pos - Vec3(r, r, r);
    box.max = pos + Vec3(r, r, r);
}

//////////////////////////////////////////////////////////////////////////
void CTagPoint::GetLocalBounds(AABB& box)
{
    float r = GetRadius();
    box.min = -Vec3(r, r, r);
    box.max = Vec3(r, r, r);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CRespawnPoint::CRespawnPoint()
{
    m_entityClass = "RespawnPoint";
}

CSpawnPoint::CSpawnPoint()
{
    m_entityClass = "SpawnPoint";
}

//////////////////////////////////////////////////////////////////////////
CNavigationSeedPoint::CNavigationSeedPoint()
{
    m_entityClass = "NavigationSeedPoint";
}

void CNavigationSeedPoint::Display(DisplayContext& dc)
{
    DrawDefault(dc);
}

int CNavigationSeedPoint::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    const int retValue = CTagPoint::MouseCreateCallback(view, event, point, flags);

    if (event == eMouseLDown)
    {
        GetIEditor()->GetAI()->CalculateNavigationAccessibility();
    }

    return retValue;
}

void CNavigationSeedPoint::Done()
{
    CTagPoint::Done();
    GetIEditor()->GetAI()->CalculateNavigationAccessibility();
}

void CNavigationSeedPoint::SetModified(bool boModifiedTransformOnly)
{
    CTagPoint::SetModified(boModifiedTransformOnly);
    GetIEditor()->GetAI()->CalculateNavigationAccessibility();
}

#include <Objects/TagPoint.moc>