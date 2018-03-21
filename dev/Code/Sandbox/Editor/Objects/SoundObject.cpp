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

// Description : CSoundObject implementation.


#include "StdAfx.h"
#include "SoundObject.h"

#include "../Viewport.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CSoundObject::CSoundObject()
{
    //m_ITag = 0;

    m_innerRadius = 1;
    m_outerRadius = 10;
}

//////////////////////////////////////////////////////////////////////////
void CSoundObject::Done()
{
    //if (m_ITag)
    {
        //  GetIEditor()->GetGame()->RemoveSoundObject( m_ITag );
    }
    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CSoundObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    m_ie = ie;

    SetColor(QColor(255, 255, 0));
    bool res = CBaseObject::Init(ie, prev, file);

    // Create Tag point in game.
    //m_ITag = GetIEditor()->GetGame()->CreateSoundObject( (const char*)GetName(),GetPos(),GetAngles() );

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CSoundObject::SetName(const QString& name)
{
    CBaseObject::SetName(name);
    //if (m_ITag)
    //m_ITag->SetName( name );
}

//////////////////////////////////////////////////////////////////////////
void CSoundObject::GetBoundSphere(Vec3& pos, float& radius)
{
    pos = GetPos();
    radius = m_outerRadius;
}

//////////////////////////////////////////////////////////////////////////
void CSoundObject::BeginEditParams(IEditor* ie, int flags)
{
    m_ie = ie;
    CBaseObject::BeginEditParams(ie, flags);
}

//////////////////////////////////////////////////////////////////////////
void CSoundObject::EndEditParams(IEditor* ie)
{
    CBaseObject::EndEditParams(ie);
}

/*
//////////////////////////////////////////////////////////////////////////
void CSoundObject::OnPropertyChange( const CString &property )
{
    CBaseObject::OnPropertyChange(property);

    if (!GetParams())
        return;

    GetParams()->getAttr( "InnerRadius",m_innerRadius );
    GetParams()->getAttr( "OuterRadius",m_outerRadius );
}
*/

//////////////////////////////////////////////////////////////////////////
int CSoundObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown)
    {
        Vec3 pos;
        // Position 1 meter above ground when creating.
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
bool CSoundObject::HitTest(HitContext& hc)
{
    Vec3 origin = GetPos();
    float radius = 1;

    Vec3 w = origin - hc.raySrc;
    w = hc.rayDir.Cross(w);
    float d = w.GetLength();

    if (d < radius + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(origin);
        hc.object = this;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSoundObject::Display(DisplayContext& dc)
{
    QColor color = GetColor();

    dc.SetColor(color, 0.8f);
    dc.DrawBall(GetPos(), 1);

    dc.SetColor(0, 1, 0, 0.3f);
    dc.DrawWireSphere(GetPos(), m_innerRadius);

    dc.SetColor(color, 1);
    dc.DrawWireSphere(GetPos(), m_outerRadius);
    //dc.renderer->DrawBall( GetPos(),m_outerRadius );

    if (IsSelected())
    {
        dc.SetSelectedColor(0.5f);
        dc.DrawBall(GetPos(), 1.3f);
    }
    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSoundObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CBaseObject::Export(levelPath, xmlNode);
    return objNode;
}

#include <Objects/SoundObject.moc>
