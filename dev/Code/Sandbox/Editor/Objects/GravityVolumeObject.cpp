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

// Description : CGravityVolumeObject implementation.


#include "StdAfx.h"
#include "GravityVolumeObject.h"
#include "../GravityVolumePanel.h"
#include "../Viewport.h"
#include "Util/Triangulate.h"
#include "Material/Material.h"

#include <I3DEngine.h>

#include <IEntityHelper.h>
#include "Components/IComponentArea.h"
#include "Components/IComponentRender.h"

//////////////////////////////////////////////////////////////////////////
// class CGravityVolume Sector


//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

#define RAY_DISTANCE 100000.0f

//////////////////////////////////////////////////////////////////////////
int CGravityVolumeObject::m_rollupId = 0;
CGravityVolumePanel* CGravityVolumeObject::m_panel = 0;

//////////////////////////////////////////////////////////////////////////
CGravityVolumeObject::CGravityVolumeObject()
{
    m_bForce2D = false;
    mv_radius = 4.0f;
    mv_gravity = 10.0f;
    mv_falloff = 0.8f;
    mv_damping = 1.0f;
    mv_dontDisableInvisible = false;
    mv_step = 4.0f;

    m_bIgnoreParamUpdate = false;

    m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    m_selectedPoint = -1;
    m_entityClass = "AreaBezierVolume";

    SetColor(Vec2Rgb(Vec3(0, 0.8f, 1)));
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::Done()
{
    m_points.clear();
    m_bezierPoints.clear();
    CEntityObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CGravityVolumeObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CEntityObject::Init(ie, prev, file);

    if (m_pEntity)
    {
        m_pEntity->GetOrCreateComponent<IComponentArea>();
    }

    if (prev)
    {
        m_points = ((CGravityVolumeObject*)prev)->m_points;
        CalcBBox();
    }

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::InitVariables()
{
    AddVariable(mv_radius, "Radius", functor(*this, &CGravityVolumeObject::OnParamChange));
    AddVariable(mv_gravity, "Gravity", functor(*this, &CGravityVolumeObject::OnParamChange));
    AddVariable(mv_falloff, "Falloff", functor(*this, &CGravityVolumeObject::OnParamChange));
    AddVariable(mv_damping, "Damping", functor(*this, &CGravityVolumeObject::OnParamChange));
    AddVariable(mv_step, "StepSize", functor(*this, &CGravityVolumeObject::OnParamChange));
    AddVariable(mv_dontDisableInvisible, "DontDisableInvisible", functor(*this, &CGravityVolumeObject::OnParamChange));
    mv_step.SetLimits(0.1f, 1000.f);
}

//////////////////////////////////////////////////////////////////////////
QString CGravityVolumeObject::GetUniqueName() const
{
    char prefix[32];
    sprintf_s(prefix, "%p ", this);
    return QString(prefix) + GetName();
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::InvalidateTM(int nWhyFlags)
{
    CEntityObject::InvalidateTM(nWhyFlags);
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::GetBoundBox(AABB& box)
{
    box.SetTransformedAABB(GetWorldTM(), m_bbox);
    float s = 1.0f;
    box.min -= Vec3(s, s, s);
    box.max += Vec3(s, s, s);
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::GetLocalBounds(AABB& box)
{
    box = m_bbox;
}

//////////////////////////////////////////////////////////////////////////
bool CGravityVolumeObject::HitTest(HitContext& hc)
{
    // First check if ray intersect our bounding box.
    float tr = hc.distanceTolerance / 2 + GravityVolume_CLOSE_DISTANCE;

    // Find intersection of line with zero Z plane.
    float minDist = FLT_MAX;
    Vec3 intPnt(hc.raySrc);
    //GetNearestEdge( hc.raySrc,hc.rayDir,p1,p2,minDist,intPnt );

    bool bWasIntersection = false;
    Vec3 ip;
    Vec3 rayLineP1 = hc.raySrc;
    Vec3 rayLineP2 = hc.raySrc + hc.rayDir * RAY_DISTANCE;
    const Matrix34& wtm = GetWorldTM();

    for (int i = 0; i < m_points.size(); i++)
    {
        int kn = 6;
        for (int k = 0; k < kn; k++)
        {
            Vec3 pi = wtm.TransformPoint(GetSplinePos(m_points, i, float(k) / kn));
            Vec3 pj = wtm.TransformPoint(GetSplinePos(m_points, i, float(k) / kn + 1.0f / kn));

            float d = 0;
            if (RayToLineDistance(rayLineP1, rayLineP2, pi, pj, d, ip))
            {
                if (d < minDist)
                {
                    bWasIntersection = true;
                    minDist = d;
                    intPnt = ip;
                }
            }
        }
    }

    float fGravityVolumeCloseDistance = GravityVolume_CLOSE_DISTANCE * hc.view->GetScreenScaleFactor(intPnt) * 0.01f;

    if (bWasIntersection && minDist < fGravityVolumeCloseDistance + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(intPnt);
        hc.object = this;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);

    if (!m_panel)
    {
        m_panel = new CGravityVolumePanel;
        m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "GravityVolume Parameters", m_panel);
    }
    if (m_panel)
    {
        m_panel->SetGravityVolume(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::EndEditParams(IEditor* ie)
{
    if (m_rollupId != 0)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupId);
        CalcBBox();
    }
    m_rollupId = 0;
    m_panel = 0;

    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
int CGravityVolumeObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown || event == eMouseLDblClick)
    {
        Vec3 pos = view->MapViewToCP(point);

        if (m_points.size() < 2)
        {
            SetPos(pos);
        }

        pos.z += GravityVolume_Z_OFFSET;

        if (m_points.size() == 0)
        {
            InsertPoint(-1, Vec3(0, 0, 0));
        }
        else
        {
            SetPoint(m_points.size() - 1, pos - GetWorldPos());
        }

        if (event == eMouseLDblClick)
        {
            if (m_points.size() > GetMinPoints())
            {
                // Remove last unneeded point.
                m_points.pop_back();
                EndCreation();
                return MOUSECREATE_OK;
            }
            else
            {
                return MOUSECREATE_ABORT;
            }
        }

        if (event == eMouseLDown)
        {
            Vec3 vlen = Vec3(pos.x, pos.y, 0) - Vec3(GetWorldPos().x, GetWorldPos().y, 0);
            if (m_points.size() > 2 && vlen.GetLength() < GravityVolume_CLOSE_DISTANCE)
            {
                EndCreation();
                return MOUSECREATE_OK;
            }
            if (GetPointCount() >= GetMaxPoints())
            {
                EndCreation();
                return MOUSECREATE_OK;
            }

            InsertPoint(-1, pos - GetWorldPos());
        }
        return MOUSECREATE_CONTINUE;
    }
    return CEntityObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::EndCreation()
{
    if (m_panel)
    {
        m_panel->SetGravityVolume(this);
    }
}

//////////////////////////////////////////////////////////////////////////
Vec3 CGravityVolumeObject::GetBezierPos(CGravityVolumePointVector& points, int index, float t)
{
    //Vec3 p0 = points[index].pos;
    //Vec3 pf0 = points[index].forw;
    //Vec3 p1 = points[index+1].pos;
    //Vec3 pb1 = points[index+1].back;
    return points[index].pos *      ((1 - t) * (1 - t) * (1 - t)) +
           points[index].forw *     (3 * t * (1 - t) * (1 - t)) +
           points[index + 1].back *   (3 * t * t * (1 - t)) +
           points[index + 1].pos *    (t * t * t);
}



//////////////////////////////////////////////////////////////////////////
Vec3 CGravityVolumeObject::GetSplinePos(CGravityVolumePointVector& points, int index, float t)
{
    //Vec3 p0 = points[index].pos;
    //Vec3 pf0 = points[index].forw;
    //Vec3 p1 = points[index+1].pos;
    //Vec3 pb1 = points[index+1].back;
    //Vec3 p0 = points[index].back;
    //Vec3 p1 = (points[index].forw + points[index+1].back)/2;
    //Vec3 p2 = points[index+1].forw;
    int indprev = index - 1;
    int indnext = index + 1;
    if (indprev < 0)
    {
        indprev = 0;
    }
    if (indnext >= points.size())
    {
        indnext = points.size() - 1;
    }
    Vec3 p0 = points[indprev].pos;
    Vec3 p1 = points[index].pos;
    Vec3 p2 = points[indnext].pos;
    return ((p0 + p2) / 2 - p1) * t * t + (p1 - p0) * t + (p0 + p1) / 2;
}


//////////////////////////////////////////////////////////////////////////
Vec3 CGravityVolumeObject::GetBezierNormal(int index, float t)
{
    float kof = 0.0f;
    int i = index;
    if (index >= m_points.size() - 1)
    {
        kof = 1.0f;
        i = m_points.size() - 2;
        if (i < 0)
        {
            return Vec3(0, 0, 0);
        }
    }
    const Matrix34& wtm = GetWorldTM();
    Vec3 p0 = wtm.TransformPoint(GetSplinePos(m_points, i, t + 0.0001f + kof));
    Vec3 p1 = wtm.TransformPoint(GetSplinePos(m_points, i, t - 0.0001f + kof));

    Vec3 e = p0 - p1;
    Vec3 n = e.Cross(Vec3(0, 0, 1));

    //if(n.GetLength()>0.00001f)
    if (n.x > 0.00001f || n.x < -0.00001f || n.y > 0.00001f || n.y < -0.00001f || n.z > 0.00001f || n.z < -0.00001f)
    {
        n.Normalize();
    }
    return n;
}

//////////////////////////////////////////////////////////////////////////
float CGravityVolumeObject::GetBezierSegmentLength(int index, float t)
{
    const Matrix34& wtm = GetWorldTM();
    int kn = int(t * 20) + 1;
    float fRet = 0.0f;
    for (int k = 0; k < kn; k++)
    {
        Vec3 e = GetSplinePos(m_points, index, t * float(k) / kn) -
            GetSplinePos(m_points, index, t * (float(k) / kn + 1.0f / kn));
        fRet += e.GetLength();
    }
    return fRet;
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::DrawBezierSpline(DisplayContext& dc, CGravityVolumePointVector& points, const QColor& col, bool isDrawJoints, bool isDrawGravityVolume)
{
    const Matrix34& wtm = GetWorldTM();
    float fPointSize = 0.5f;

    dc.SetColor(col);
    for (int i = 0; i < points.size(); i++)
    {
        Vec3 p0 = wtm.TransformPoint(points[i].pos);

        // Draw Bezier joints
        //if (isDrawJoints)
        if (0)
        {
            ColorB colf = dc.GetColor();

            Vec3 pf0 = wtm.TransformPoint(points[i].forw);
            float fScalef = fPointSize * dc.view->GetScreenScaleFactor(pf0) * 0.01f;
            Vec3 szf(fScalef, fScalef, fScalef);
            dc.SetColor(QColor(0, 200, 0));
            dc.DrawLine(p0, pf0);
            dc.SetColor(QColor(0, 255, 0));
            dc.DrawWireBox(pf0 - szf, pf0 + szf);

            Vec3 pb0 = wtm.TransformPoint(points[i].back);
            float fScaleb = fPointSize * dc.view->GetScreenScaleFactor(pb0) * 0.01f;
            Vec3 szb(fScaleb, fScaleb, fScaleb);
            dc.SetColor(QColor(0, 200, 0));
            dc.DrawLine(p0, pb0);
            dc.SetColor(QColor(0, 255, 0));
            dc.DrawWireBox(pb0 - szb, pb0 + szb);

            dc.SetColor(colf);
        }

        if (isDrawGravityVolume && i < points.size() - 1)
        {
            Vec3 p1 = wtm.TransformPoint(points[i + 1].pos);
            ColorB colf = dc.GetColor();

            dc.SetColor(QColor(0, 200, 0));
            dc.DrawLine(p0, p1);
            dc.SetColor(colf);
        }


        //if(i < points.size()-1)
        if (i < points.size())
        {
            int kn = int((GetBezierSegmentLength(i) + 0.5f) / mv_step);
            if (kn == 0)
            {
                kn = 1;
            }
            for (int k = 0; k < kn; k++)
            {
                Vec3 p = wtm.TransformPoint(GetSplinePos(points, i, float(k) / kn));
                Vec3 p1 = wtm.TransformPoint(GetSplinePos(points, i, float(k) / kn + 1.0f / kn));
                dc.DrawLine(p, p1);

                if (isDrawGravityVolume)
                {
                    ColorB colf = dc.GetColor();
                    dc.SetColor(QColor(127, 127, 255));
                    Vec3 a = GetSplinePos(points, i, float(k) / kn);
                    Vec3 a1 = GetSplinePos(points, i, float(k) / kn + 1.0f / kn);
                    Vec3 e1 = GetBezierNormal(i, float(k) / kn);
                    Vec3 e2 = e1.Cross(a1 - a);
                    if (e2.x > 0.00001f || e2.x < -0.00001f || e2.y > 0.00001f || e2.y < -0.00001f || e2.z > 0.00001f || e2.z < -0.00001f)
                    {
                        e2.Normalize();
                        for (int i = 0; i < 16; i++)
                        {
                            float ang = float(i) / 16 * 3.141593f * 2;
                            float ang2 = float(i + 1) / 16 * 3.141593f * 2;
                            Vec3 f = e1 * cos(ang) + e2 * sin(ang);
                            Vec3 f2 = e1 * cos(ang2) + e2 * sin(ang2);
                            Vec3 b = wtm.TransformPoint(mv_radius * f + a);
                            Vec3 b2 = wtm.TransformPoint(mv_radius * f2 + a);
                            dc.DrawLine(b, b2);
                        }
                    }
                    dc.SetColor(colf);
                }
            }
        }

        float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
        Vec3 sz(fScale, fScale, fScale);
        dc.DrawWireBox(p0 - sz, p0 + sz);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::Display(DisplayContext& dc)
{
    //dc.renderer->EnableDepthTest(false);

    const Matrix34& wtm = GetWorldTM();
    QColor col(Qt::black);

    float fPointSize = 0.5f;

    bool bPointSelected = false;
    if (m_selectedPoint >= 0 && m_selectedPoint < m_points.size())
    {
        bPointSelected = true;
    }

    if (m_points.size() > 1)
    {
        if ((IsSelected() || IsHighlighted()))
        {
            col = dc.GetSelectedColor();
            dc.SetColor(col);
        }
        else
        {
            if (IsFrozen())
            {
                dc.SetFreezeColor();
            }
            else
            {
                dc.SetColor(GetColor());
            }
            col = GetColor();
        }

        DrawBezierSpline(dc, m_points, col, false, IsSelected());

        if (IsSelected())
        {
            AABB box;
            GetBoundBox(box);
            dc.DrawWireBox(box.min, box.max);
        }

        // Draw selected point.
        if (bPointSelected)
        {
            dc.SetColor(QColor(0, 0, 255));
            //dc.SetSelectedColor( 0.5f );

            Vec3 selPos = wtm.TransformPoint(m_points[m_selectedPoint].pos);
            //dc.renderer->DrawBall( selPos,1.0f );
            float fScale = fPointSize * dc.view->GetScreenScaleFactor(selPos) * 0.01f;
            Vec3 sz(fScale, fScale, fScale);
            dc.DrawWireBox(selPos - sz, selPos + sz);

            DrawAxis(dc, m_points[m_selectedPoint].pos, 0.15f);
        }
    }

    DrawDefault(dc, GetColor());
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::Serialize(CObjectArchive& ar)
{
    m_bIgnoreParamUpdate = true;
    CEntityObject::Serialize(ar);

    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        m_points.clear();
        XmlNodeRef points = xmlNode->findChild("Points");
        if (points)
        {
            for (int i = 0; i < points->getChildCount(); i++)
            {
                XmlNodeRef pnt = points->getChild(i);
                CGravityVolumePoint pt;
                pnt->getAttr("Pos", pt.pos);
                //pnt->getAttr( "Back",pt.back );
                //pnt->getAttr( "Forw",pt.forw );
                pnt->getAttr("Angle", pt.angle);
                pnt->getAttr("Width", pt.width);
                pnt->getAttr("IsDefaultWidth", pt.isDefaultWidth);
                m_points.push_back(pt);
            }
        }

        CalcBBox();

        // Update UI.
        if (m_panel && m_panel->GetGravityVolume() == this)
        {
            m_panel->SetGravityVolume(this);
        }
    }
    else
    {
        // Saving.

        // Save Points.
        if (!m_points.empty())
        {
            XmlNodeRef points = xmlNode->newChild("Points");
            for (int i = 0; i < m_points.size(); i++)
            {
                XmlNodeRef pnt = points->newChild("Point");
                pnt->setAttr("Pos", m_points[i].pos);
                //pnt->setAttr( "Back",m_points[i].back );
                //pnt->setAttr( "Forw",m_points[i].forw );
                pnt->setAttr("Angle", m_points[i].angle);
                pnt->setAttr("Width", m_points[i].width);
                pnt->setAttr("IsDefaultWidth", m_points[i].isDefaultWidth);
            }
        }
    }
    m_bIgnoreParamUpdate = false;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CGravityVolumeObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CEntityObject::Export(levelPath, xmlNode);
    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::CalcBBox()
{
    if (m_points.empty())
    {
        return;
    }

    // Reposition GravityVolume, so that GravityVolume object position is in the middle of the GravityVolume.
    Vec3 center = m_points[0].pos;
    if (center.x != 0 || center.y != 0 || center.z != 0)
    {
        // May not work correctly if GravityVolume is transformed.
        for (int i = 0; i < m_points.size(); i++)
        {
            m_points[i].pos -= center;
        }
        Matrix34 ltm = GetLocalTM();
        SetPos(GetPos() + ltm.TransformVector(center));
    }

    for (int i = 0; i < m_points.size(); i++)
    {
        SplineCorrection(i);
    }

    // First point must always be 0,0,0.
    m_bbox.Reset();
    for (int i = 0; i < m_points.size(); i++)
    {
        m_bbox.Add(m_points[i].pos);
    }
    if (m_bbox.min.x > m_bbox.max.x)
    {
        m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    }
    //BBox box;
    //box.SetTransformedAABB( GetWorldTM(),m_bbox );

    //m_bbox.max.z = max( m_bbox.max.z,(float)mv_height );
    //m_bbox.max.z = max( m_bbox.max.z, 0.0f);
    m_bbox.min -= Vec3(mv_radius, mv_radius, mv_radius);
    m_bbox.max += Vec3(mv_radius, mv_radius, mv_radius);
}

//////////////////////////////////////////////////////////////////////////
int CGravityVolumeObject::InsertPoint(int index, const Vec3& point)
{
    if (GetPointCount() >= GetMaxPoints())
    {
        return GetPointCount() - 1;
    }
    int newindex;
    StoreUndo("Insert Point");

    newindex = m_points.size();


    if (index < 0 || index >= m_points.size())
    {
        CGravityVolumePoint pt;
        pt.pos = point;
        pt.back = point;
        pt.forw = point;
        m_points.push_back(pt);
        newindex = m_points.size() - 1;
    }
    else
    {
        CGravityVolumePoint pt;
        pt.pos = point;
        pt.back = point;
        pt.forw = point;
        m_points.insert(m_points.begin() + index, pt);
        newindex = index;
    }
    SetPoint(newindex, point);
    CalcBBox();
    return newindex;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::RemovePoint(int index)
{
    if ((index >= 0 || index < m_points.size()) && m_points.size() > 3)
    {
        StoreUndo("Remove Point");
        m_points.erase(m_points.begin() + index);
        CalcBBox();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::SelectPoint(int index)
{
    if (m_selectedPoint == index)
    {
        return;
    }
    m_selectedPoint = index;
    if (m_panel)
    {
        m_panel->SelectPoint(index);
    }
}

//////////////////////////////////////////////////////////////////////////
float CGravityVolumeObject::GetPointAngle()
{
    int index = m_selectedPoint;
    if (0 <= index && index < m_points.size())
    {
        return m_points[index].angle;
    }
    return 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::SetPointAngle(float angle)
{
    int index = m_selectedPoint;
    if (0 <= index && index < m_points.size())
    {
        m_points[index].angle = angle;
    }
}


float CGravityVolumeObject::GetPointWidth()
{
    int index = m_selectedPoint;
    if (0 <= index && index < m_points.size())
    {
        return m_points[index].width;
    }
    return 0.0f;
}

void CGravityVolumeObject::SetPointWidth(float width)
{
    int index = m_selectedPoint;
    if (0 <= index && index < m_points.size())
    {
        m_points[index].width = width;
    }
}

bool CGravityVolumeObject::IsPointDefaultWidth()
{
    int index = m_selectedPoint;
    if (0 <= index && index < m_points.size())
    {
        return m_points[index].isDefaultWidth;
    }
    return true;
}

void CGravityVolumeObject::PointDafaultWidthIs(bool isDefault)
{
    int index = m_selectedPoint;
    if (0 <= index && index < m_points.size())
    {
        m_points[index].isDefaultWidth = isDefault;
        m_points[index].width = mv_radius;
    }
}



//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::BezierAnglesCorrection(CGravityVolumePointVector& points, int index)
{
    int maxindex = points.size() - 1;
    if (index < 0 || index > maxindex)
    {
        return;
    }

    Vec3& p2 = points[index].pos;
    Vec3& back = points[index].back;
    Vec3& forw = points[index].forw;

    if (index == 0)
    {
        back = p2;
        if (maxindex == 1)
        {
            Vec3& p3 = points[index + 1].pos;
            forw = p2 + (p3 - p2) / 3;
        }
        else
        if (maxindex > 0)
        {
            Vec3& p3 = points[index + 1].pos;
            Vec3& pb3 = points[index + 1].back;

            float lenOsn = (pb3 - p2).GetLength();
            float lenb = (p3 - p2).GetLength();

            forw = p2 + (pb3 - p2) / (lenOsn / lenb * 3);
        }
    }

    if (index == maxindex)
    {
        forw = p2;
        if (index > 0)
        {
            Vec3& p1 = points[index - 1].pos;
            Vec3& pf1 = points[index - 1].forw;

            float lenOsn = (pf1 - p2).GetLength();
            float lenf = (p1 - p2).GetLength();

            if (lenOsn > 0.000001f && lenf > 0.000001f)
            {
                back = p2 + (pf1 - p2) / (lenOsn / lenf * 3);
            }
            else
            {
                back = p2;
            }
        }
    }

    if (1 <= index && index <= maxindex - 1)
    {
        Vec3& p1 = points[index - 1].pos;
        Vec3& p3 = points[index + 1].pos;

        float lenOsn = (p3 - p1).GetLength();
        float lenb = (p1 - p2).GetLength();
        float lenf = (p3 - p2).GetLength();

        back = p2 + (p1 - p3) / (lenOsn / lenb * 3);
        forw = p2 + (p3 - p1) / (lenOsn / lenf * 3);
    }
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::SplinePointsCorrection(CGravityVolumePointVector& points, int index)
{
    int maxindex = points.size() - 1;
    if (index < 0 || index > maxindex)
    {
        return;
    }

    Vec3& p2 = points[index].pos;
    Vec3& back = points[index].back;
    Vec3& forw = points[index].forw;

    if (index == 0)
    {
        back = p2;
        if (maxindex == 1)
        {
            Vec3& p3 = points[index + 1].pos;
            forw = p2 + (p3 - p2) / 3;
        }
        else
        if (maxindex > 0)
        {
            Vec3& p3 = points[index + 1].pos;
            Vec3& pb3 = points[index + 1].back;

            float lenOsn = (pb3 - p2).GetLength();
            float lenb = (p3 - p2).GetLength();

            forw = p2 + (pb3 - p2) / (lenOsn / lenb * 3);
        }
    }

    if (index == maxindex)
    {
        forw = p2;
        if (index > 0)
        {
            Vec3& p1 = points[index - 1].pos;
            Vec3& pf1 = points[index - 1].forw;

            float lenOsn = (pf1 - p2).GetLength();
            float lenf = (p1 - p2).GetLength();

            if (lenOsn > 0.000001f && lenf > 0.000001f)
            {
                back = p2 + (pf1 - p2) / (lenOsn / lenf * 3);
            }
            else
            {
                back = p2;
            }
        }
    }

    if (1 <= index && index <= maxindex - 1)
    {
        Vec3& p1 = points[index - 1].pos;
        Vec3& p3 = points[index + 1].pos;

        float lenOsn = (p3 - p1).GetLength();
        float lenb = (p1 - p2).GetLength();
        float lenf = (p3 - p2).GetLength();

        back = p2 + (p1 - p3) / (lenOsn / lenb * 2);
        forw = p2 + (p3 - p1) / (lenOsn / lenf * 2);
    }
}



//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::BezierCorrection(int index)
{
    BezierAnglesCorrection(m_points, index - 1);
    BezierAnglesCorrection(m_points, index);
    BezierAnglesCorrection(m_points, index + 1);
    BezierAnglesCorrection(m_points, index - 2);
    BezierAnglesCorrection(m_points, index + 2);
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::SplineCorrection(int index)
{
    /*
    SplinePointsCorrection(m_points, index-1);
    SplinePointsCorrection(m_points, index);
    SplinePointsCorrection(m_points, index+1);
    SplinePointsCorrection(m_points, index-2);
    SplinePointsCorrection(m_points, index+2);
    */
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::SetPoint(int index, const Vec3& pos)
{
    Vec3 p = pos;
    if (m_bForce2D)
    {
        if (!m_points.empty())
        {
            p.z = m_points[0].pos.z; // Keep original Z.
        }
    }

    if (0 <= index && index < m_points.size())
    {
        m_points[index].pos = p;
        SplineCorrection(index);
    }

    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
int CGravityVolumeObject::GetNearestPoint(const Vec3& raySrc, const Vec3& rayDir, float& distance)
{
    int index = -1;
    float minDist = FLT_MAX;
    Vec3 rayLineP1 = raySrc;
    Vec3 rayLineP2 = raySrc + rayDir * RAY_DISTANCE;
    Vec3 intPnt;
    const Matrix34& wtm = GetWorldTM();
    for (int i = 0; i < m_points.size(); i++)
    {
        float d = PointToLineDistance(rayLineP1, rayLineP2, wtm.TransformPoint(m_points[i].pos), intPnt);
        if (d < minDist)
        {
            minDist = d;
            index = i;
        }
    }
    distance = minDist;
    return index;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::GetNearestEdge(const Vec3& pos, int& p1, int& p2, float& distance, Vec3& intersectPoint)
{
    p1 = -1;
    p2 = -1;
    float minDist = FLT_MAX;
    Vec3 intPnt;

    const Matrix34& wtm = GetWorldTM();

    for (int i = 0; i < m_points.size(); i++)
    {
        int j = (i < m_points.size() - 1) ? i + 1 : 0;

        if (j == 0 && i != 0)
        {
            continue;
        }

        int kn = 6;
        for (int k = 0; k < kn; k++)
        {
            Vec3 a1 = wtm.TransformPoint(GetSplinePos(m_points, i, float(k) / kn));
            Vec3 a2 = wtm.TransformPoint(GetSplinePos(m_points, i, float(k) / kn + 1.0f / kn));

            float d = PointToLineDistance(a1, a2, pos, intPnt);
            if (d < minDist)
            {
                minDist = d;
                p1 = i;
                p2 = j;
                intersectPoint = intPnt;
            }
        }
    }
    distance = minDist;
}

//////////////////////////////////////////////////////////////////////////
bool CGravityVolumeObject::RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt)
{
    Vec3 pa, pb;
    float ua, ub;
    if (!LineLineIntersect(pi, pj, rayLineP1, rayLineP2, pa, pb, ua, ub))
    {
        return false;
    }

    // If before ray origin.
    if (ub < 0)
    {
        return false;
    }

    float d = 0;
    if (ua < 0)
    {
        d = PointToLineDistance(rayLineP1, rayLineP2, pi, intPnt);
    }
    else if (ua > 1)
    {
        d = PointToLineDistance(rayLineP1, rayLineP2, pj, intPnt);
    }
    else
    {
        intPnt = rayLineP1 + ub * (rayLineP2 - rayLineP1);
        d = (pb - pa).GetLength();
    }
    distance = d;

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::GetNearestEdge(const Vec3& raySrc, const Vec3& rayDir, int& p1, int& p2, float& distance, Vec3& intersectPoint)
{
    p1 = -1;
    p2 = -1;
    float minDist = FLT_MAX;
    Vec3 intPnt;
    Vec3 rayLineP1 = raySrc;
    Vec3 rayLineP2 = raySrc + rayDir * RAY_DISTANCE;

    const Matrix34& wtm = GetWorldTM();

    for (int i = 0; i < m_points.size(); i++)
    {
        int j = (i < m_points.size() - 1) ? i + 1 : 0;

        if (j == 0 && i != 0)
        {
            continue;
        }

        int kn = 6;
        for (int k = 0; k < kn; k++)
        {
            Vec3 pi = wtm.TransformPoint(GetSplinePos(m_points, i, float(k) / kn));
            Vec3 pj = wtm.TransformPoint(GetSplinePos(m_points, i, float(k) / kn + 1.0f / kn));

            float d = 0;
            if (!RayToLineDistance(rayLineP1, rayLineP2, pi, pj, d, intPnt))
            {
                continue;
            }

            if (d < minDist)
            {
                minDist = d;
                p1 = i;
                p2 = j;
                intersectPoint = intPnt;
            }
        }
    }
    distance = minDist;
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::OnParamChange(IVariable* var)
{
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
bool CGravityVolumeObject::HitTestRect(HitContext& hc)
{
    //BBox box;
    // Retrieve world space bound box.
    //GetBoundBox( box );

    // Project all edges to viewport.
    const Matrix34& wtm = GetWorldTM();
    for (int i = 0; i < m_points.size(); i++)
    {
        int j = (i < m_points.size() - 1) ? i + 1 : 0;
        if (j == 0 && i != 0)
        {
            continue;
        }

        Vec3 p0 = wtm.TransformPoint(m_points[i].pos);
        Vec3 p1 = wtm.TransformPoint(m_points[j].pos);

        QPoint pnt0 = hc.view->WorldToView(p0);
        QPoint pnt1 = hc.view->WorldToView(p1);

        // See if pnt0 to pnt1 line intersects with rectangle.
        // check see if one point is inside rect and other outside, or both inside.
        bool in0 = hc.rect.contains(pnt0);
        bool in1 = hc.rect.contains(pnt0);
        if ((in0 && in1) || (in0 && !in1) || (!in0 && in1))
        {
            hc.object = this;
            return true;
        }
    }

    return false;
}

void CGravityVolumeObject::SetSelected(bool bSelect)
{
    CEntityObject::SetSelected(bSelect);
}


bool CGravityVolumeObject::CreateGameObject()
{
    bool bRes = CEntityObject::CreateGameObject();

    UpdateGameArea();
    return bRes;
}


//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::UpdateGameArea()
{
    if (!m_pEntity)
    {
        return;
    }

    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
    if (!pArea)
    {
        return;
    }
    const Matrix34& wtm = GetWorldTM();
    m_bezierPoints.resize(m_points.size());
    for (int i = 0; i < m_points.size(); i++)
    {
        m_bezierPoints[i] = wtm.TransformPoint(m_points[i].pos);
    }
    if (m_bezierPoints.size())
    {
        pArea->SetGravityVolume(&m_bezierPoints[0], m_points.size(), mv_radius, mv_gravity, mv_dontDisableInvisible, mv_falloff, mv_damping);
    }

    IComponentRenderPtr pRenderComponent = m_pEntity->GetOrCreateComponent<IComponentRender>();
    m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);
    if (!pRenderComponent)
    {
        return;
    }
    AABB box;
    GetLocalBounds(box);
    pRenderComponent->SetLocalBounds(box, true);
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumeObject::OnEvent(ObjectEvent event)
{
    if (event == EVENT_INGAME)
    {
        bool bEnabled = false;
        IVariable* pVarEn = m_pProperties->FindVariable("bEnabled");
        if (pVarEn)
        {
            pVarEn->Get(bEnabled);
        }

        SEntityEvent ev;
        ev.event = ENTITY_EVENT_SCRIPT_EVENT;
        const char* pComEn = "Enable";
        const char* pComDi = "Disable";
        static const bool cbTrue = true;
        if (bEnabled)
        {
            ev.nParam[0] = (INT_PTR)pComEn;
        }
        else
        {
            ev.nParam[0] = (INT_PTR)pComDi;
        }
        ev.nParam[1] = (INT_PTR)IEntityClass::EVT_BOOL;
        ev.nParam[2] = (INT_PTR)&cbTrue;
        m_pEntity->SendEvent(ev);
    }
}

#include <Objects/GravityVolumeObject.moc>