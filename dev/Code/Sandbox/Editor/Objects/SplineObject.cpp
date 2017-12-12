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

#include "StdAfx.h"
#include "Viewport.h"

#include "SplineObject.h"
#include "SplinePanel.h"
#include "Include/ITransformManipulator.h"

CSplinePanel* CSplineObject::m_pSplinePanel = nullptr;
int CSplineObject::m_splineRollupID = 0;

//////////////////////////////////////////////////////////////////////////
CSplineObject::CSplineObject()
    : m_selectedPoint(-1)
    , m_mergeIndex(-1)
    , m_isEditMode(false)
{
    m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::SelectPoint(int index)
{
    if (m_selectedPoint == index)
    {
        return;
    }

    m_selectedPoint = index;
    UpdateTransformManipulator();
    OnUpdateUI();
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::SetPoint(int index, const Vec3& pos)
{
    if (0 <= index && index < GetPointCount())
    {
        m_points[index].pos = pos;
        BezierCorrection(index);
        CalcBBox();
        OnUpdate();
        UpdateTransformManipulator();
    }
}


//////////////////////////////////////////////////////////////////////////
int CSplineObject::InsertPoint(int index, const Vec3& point)
{
    int newindex = GetPointCount();
    if (newindex >= GetMaxPoints())
    {
        return newindex - 1;
    }

    // Don't allow to create 2 points on the same position
    if (newindex >= 2)
    {
        if (m_points[newindex - 2].pos == m_points[newindex - 1].pos)
        {
            return newindex - 1;
        }
    }

    StoreUndo("Insert Point");

    if (index < 0 || index >= GetPointCount())
    {
        CSplinePoint pt;
        pt.pos = point;
        pt.back = point;
        pt.forw = point;
        m_points.push_back(pt);
        newindex = GetPointCount() - 1;
    }
    else
    {
        CSplinePoint pt;
        pt.pos = point;
        pt.back = point;
        pt.forw = point;
        m_points.insert(m_points.begin() + index, pt);
        newindex = index;
    }
    SetPoint(newindex, point);
    return newindex;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::RemovePoint(int index)
{
    if ((index >= 0 || index < GetPointCount()) && GetPointCount() > GetMinPoints())
    {
        StoreUndo("Remove Point");
        m_points.erase(m_points.begin() + index);
        CalcBBox();
        OnUpdate();
    }
}


//////////////////////////////////////////////////////////////////////////
Vec3 CSplineObject::GetBezierPos(int index, float t) const
{
    float invt = 1.0f - t;
    if (index == GetPointCount() - 1)
    {
        return m_points[index].pos *        (invt * invt * invt) +
            m_points[index].forw *       (3 * t * invt * invt);
    }
    return m_points[index].pos *        (invt * invt * invt) +
           m_points[index].forw *       (3 * t * invt * invt) +
           m_points[index + 1].back * (3 * t * t * invt) +
           m_points[index + 1].pos *  (t * t * t);
}


//////////////////////////////////////////////////////////////////////////
Vec3 CSplineObject::GetBezierTangent(int index, float t) const
{
    float invt = 1.0f - t;
    if (index == GetPointCount() - 1)
    {
        Vec3 tan = -m_points[index].pos *   (invt * invt)
            + m_points[index].forw *  (invt * (invt - 2 * t));
        if (!tan.IsZero())
        {
            tan.Normalize();
        }

        return tan;
    }
    Vec3 tan = -m_points[index].pos *   (invt * invt)
        + m_points[index].forw *  (invt * (invt - 2 * t))
        + m_points[index + 1].back * (t * (2 * invt - t))
        + m_points[index + 1].pos *  (t * t);

    if (!tan.IsZero())
    {
        tan.Normalize();
    }

    return tan;
}


//////////////////////////////////////////////////////////////////////////
float CSplineObject::GetBezierSegmentLength(int index, float t) const
{
    const int kSteps = 32;
    float kn = t * kSteps + 1.0f;
    float fRet = 0.0f;

    Vec3 pos = GetBezierPos(index, 0.0f);
    for (float k = 1.0f; k <= kn; k += 1.0f)
    {
        Vec3 nextPos = GetBezierPos(index, t * k / kn);
        fRet += (nextPos - pos).GetLength();
        pos = nextPos;
    }
    return fRet;
}


//////////////////////////////////////////////////////////////////////////
float CSplineObject::GetSplineLength() const
{
    float fRet = 0.f;
    for (int i = 0, numSegments = GetPointCount() - 1; i < numSegments; ++i)
    {
        fRet += GetBezierSegmentLength(i);
    }
    return fRet;
}


//////////////////////////////////////////////////////////////////////////
float CSplineObject::GetPosByDistance(float distance, int& outIndex) const
{
    float lenPos = 0.0f;
    int index = 0;
    float segmentLength = 0.0f;
    for (int numSegments = GetPointCount() - 1; index < numSegments; ++index)
    {
        segmentLength = GetBezierSegmentLength(index);
        if (lenPos + segmentLength > distance)
        {
            break;
        }
        lenPos += segmentLength;
    }

    outIndex = index;
    return segmentLength > 0.0f ? (distance - lenPos) / segmentLength : 0.0f;
}


//////////////////////////////////////////////////////////////////////////
Vec3 CSplineObject::GetBezierNormal(int index, float t) const
{
    float kof = 0.0f;
    int i = index;

    if (index >= GetPointCount() - 1)
    {
        kof = 1.0f;
        i = GetPointCount() - 2;
        if (i < 0)
        {
            return Vec3(0, 0, 0);
        }
    }

    const Matrix34& wtm = GetWorldTM();
    Vec3 p0 = wtm.TransformPoint(GetBezierPos(i, t + 0.0001f + kof));
    Vec3 p1 = wtm.TransformPoint(GetBezierPos(i, t - 0.0001f + kof));

    Vec3 e = p0 - p1;
    Vec3 n = e.Cross(Vec3(0, 0, 1));

    if (n.x > 0.00001f || n.x < -0.00001f || n.y > 0.00001f || n.y < -0.00001f || n.z > 0.00001f || n.z < -0.00001f)
    {
        n.Normalize();
    }
    return n;
}


//////////////////////////////////////////////////////////////////////////
Vec3 CSplineObject::GetLocalBezierNormal(int index, float t) const
{
    float kof = t;
    int i = index;

    if (index >= GetPointCount() - 1)
    {
        kof = 1.0f + t;
        i = GetPointCount() - 2;
        if (i < 0)
        {
            return Vec3(0, 0, 0);
        }
    }

    Vec3 e = GetBezierTangent(i, kof);
    if (e.x < 0.00001f && e.x > -0.00001f && e.y < 0.00001f && e.y > -0.00001f && e.z < 0.00001f && e.z > -0.00001f)
    {
        return Vec3(0, 0, 0);
    }

    Vec3 n;

    float an1 = m_points[i].angle;
    float an2 = m_points[i + 1].angle;

    if (-0.00001f > an1 || an1 > 0.00001f || -0.00001f > an2 || an2 > 0.00001f)
    {
        float af = kof * 2 - 1.0f;
        float ed = 1.0f;
        if (af < 0.0f)
        {
            ed = -1.0f;
        }
        af = ed - af;
        af = af * af * af;
        af = ed - af;
        af = (af + 1.0f) / 2;
        float angle = ((1.0f - af) * an1 + af * an2) * 3.141593f / 180.0f;

        e.Normalize();
        n = Vec3(0, 0, 1).Cross(e);
        n = n.GetRotated(e, angle);
    }
    else
    {
        n = Vec3(0, 0, 1).Cross(e);
    }

    if (n.x > 0.00001f || n.x < -0.00001f || n.y > 0.00001f || n.y < -0.00001f || n.z > 0.00001f || n.z < -0.00001f)
    {
        n.Normalize();
    }
    return n;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::BezierCorrection(int index)
{
    BezierAnglesCorrection(index - 1);
    BezierAnglesCorrection(index);
    BezierAnglesCorrection(index + 1);
    BezierAnglesCorrection(index - 2);
    BezierAnglesCorrection(index + 2);
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::BezierAnglesCorrection(int index)
{
    int maxindex = GetPointCount() - 1;
    if (index < 0 || index > maxindex)
    {
        return;
    }

    Vec3& p2 = m_points[index].pos;
    Vec3& back = m_points[index].back;
    Vec3& forw = m_points[index].forw;

    if (index == 0)
    {
        back = p2;
        if (maxindex == 1)
        {
            Vec3& p3 = m_points[index + 1].pos;
            forw = p2 + (p3 - p2) / 3;
        }
        else if (maxindex > 0)
        {
            Vec3& p3 = m_points[index + 1].pos;
            Vec3& pb3 = m_points[index + 1].back;

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
            Vec3& p1 = m_points[index - 1].pos;
            Vec3& pf1 = m_points[index - 1].forw;

            float lenOsn = (pf1 - p2).GetLength();
            float lenf = (p1 - p2).GetLength();

            if (!fcmp(lenOsn, 0.f) && !fcmp(lenf, 0.f))
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
        Vec3& p1 = m_points[index - 1].pos;
        Vec3& p3 = m_points[index + 1].pos;

        float lenOsn = (p3 - p1).GetLength();
        float lenb = (p1 - p2).GetLength();
        float lenf = (p3 - p2).GetLength();

        if (!fcmp(lenOsn, 0.f))
        {
            back = p2 + (p1 - p3) * (lenb / lenOsn / 3);
            forw = p2 + (p3 - p1) * (lenf / lenOsn / 3);
        }
        else
        {
            back = p2;
            forw = p2;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
float CSplineObject::GetPointAngle() const
{
    int index = m_selectedPoint;
    if (0 <= index && index < GetPointCount())
    {
        return m_points[index].angle;
    }
    return 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void CSplineObject::SetPointAngle(float angle)
{
    int index = m_selectedPoint;
    if (0 <= index && index < GetPointCount())
    {
        m_points[index].angle = angle;
    }
    OnUpdate();
}


//////////////////////////////////////////////////////////////////////////
float CSplineObject::GetPointWidth() const
{
    int index = m_selectedPoint;
    if (0 <= index && index < GetPointCount())
    {
        return m_points[index].width;
    }
    return 0.0f;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::SetPointWidth(float width)
{
    int index = m_selectedPoint;
    if (0 <= index && index < GetPointCount())
    {
        m_points[index].width = width;
    }
    OnUpdate();
}


//////////////////////////////////////////////////////////////////////////
bool CSplineObject::IsPointDefaultWidth() const
{
    int index = m_selectedPoint;
    if (0 <= index && index < GetPointCount())
    {
        return m_points[index].isDefaultWidth;
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::PointDafaultWidthIs(bool isDefault)
{
    int index = m_selectedPoint;
    if (0 <= index && index < GetPointCount())
    {
        m_points[index].isDefaultWidth = isDefault;
        m_points[index].width = GetWidth();
    }
    OnUpdate();
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::ReverseShape()
{
    StoreUndo("Reverse Shape");
    std::reverse(m_points.begin(), m_points.end());
    OnUpdate();
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::Split(int index, const Vec3& point)
{
    // do not change the order of things here
    int in0 = InsertPoint(index, point);

    CSplineObject* pNewShape = (CSplineObject*) GetObjectManager()->CloneObject(this);

    for (int i = in0 + 1, cnt = GetPointCount(); i < cnt; ++i)
    {
        RemovePoint(in0 + 1);
    }
    if (pNewShape)
    {
        for (int i = 0; i < in0; ++i)
        {
            pNewShape->RemovePoint(0);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::Merge(CSplineObject* pSpline)
{
    if (!pSpline || m_mergeIndex < 0 || pSpline->m_mergeIndex < 0)
    {
        return;
    }

    const Matrix34& tm = GetWorldTM();
    const Matrix34& splineTM = pSpline->GetWorldTM();

    if (!m_mergeIndex)
    {
        ReverseShape();
    }

    if (pSpline->m_mergeIndex)
    {
        pSpline->ReverseShape();
    }

    for (int i = 0; i < pSpline->GetPointCount(); ++i)
    {
        Vec3 pnt = splineTM.TransformPoint(pSpline->GetPoint(i));
        pnt = tm.GetInverted().TransformPoint(pnt);
        InsertPoint(GetPointCount(), pnt);
    }

    m_mergeIndex = -1;
    GetObjectManager()->DeleteObject(pSpline);
}


//////////////////////////////////////////////////////////////////////////
bool CSplineObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CBaseObject::Init(ie, prev, file);

    if (prev)
    {
        m_points = ((CSplineObject*)prev)->m_points;
        CalcBBox();
        OnUpdate();
    }

    return res;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::Done()
{
    m_points.clear();
    CBaseObject::Done();
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::BeginEditParams(IEditor* pEditor, int flags)
{
    CBaseObject::BeginEditParams(pEditor, flags);

    if (!m_pSplinePanel)
    {
        m_pSplinePanel = new CSplinePanel();
        m_splineRollupID = pEditor->AddRollUpPage(ROLLUP_OBJECTS, "Spline Parameters", m_pSplinePanel);
    }
    if (m_pSplinePanel)
    {
        m_pSplinePanel->SetSpline(this);
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::EndEditParams(IEditor* pEditor)
{
    if (m_splineRollupID != 0)
    {
        pEditor->RemoveRollUpPage(ROLLUP_OBJECTS, m_splineRollupID);
    }
    m_splineRollupID = 0;
    m_pSplinePanel = 0;

    CBaseObject::EndEditParams(pEditor);
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::GetBoundBox(AABB& box)
{
    box.SetTransformedAABB(GetWorldTM(), m_bbox);
    float s = 1.0f;
    box.min -= Vec3(s, s, s);
    box.max += Vec3(s, s, s);
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::GetLocalBounds(AABB& box)
{
    box = m_bbox;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::CalcBBox()
{
    if (m_points.empty())
    {
        return;
    }

    // Reposition spline, so that .
    Vec3 center = m_points[0].pos;
    if (center.x != 0 || center.y != 0 || center.z != 0)
    {
        for (int i = 0, n = GetPointCount(); i < n; ++i)
        {
            m_points[i].pos -= center;
        }

        SetPos(GetPos() + GetLocalTM().TransformVector(center));
    }

    for (int i = 0, n = GetPointCount(); i < n; ++i)
    {
        BezierCorrection(i);
    }

    // First point must always be 0,0,0.
    m_bbox.Reset();
    for (int i = 0, n = GetPointCount(); i < n; ++i)
    {
        m_bbox.Add(m_points[i].pos);
    }

    if (m_bbox.min.x > m_bbox.max.x)
    {
        m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::Display(DisplayContext& dc)
{
    bool isSelected = IsSelected();

    float fPointSize = isSelected ? 0.005f : 0.0025f;

    QColor colLine = GetColor();
    QColor colPoint = GetColor();

    const Matrix34& wtm = GetWorldTM();

    if (IsFrozen())
    {
        colLine = dc.GetFreezeColor();
        colPoint = dc.GetFreezeColor();
    }
    else if (isSelected || IsHighlighted())
    {
        colLine = dc.GetSelectedColor();
        colPoint = m_isEditMode ? QColor(0, 255, 0) : dc.GetSelectedColor();
    }

    dc.SetLineWidth(1);

    for (int i = 0, n = GetPointCount(); i < n; ++i)
    {
        // Draw spline segments
        dc.SetColor(colLine);
        if (i < (n - 1))
        {
            int kn = 8;
            for (int k = 0; k < kn; ++k)
            {
                Vec3 p = wtm.TransformPoint(GetBezierPos(i, float(k) / kn));
                Vec3 p1 = wtm.TransformPoint(GetBezierPos(i, float(k) / kn + 1.0f / kn));
                dc.DrawLine(p, p1);
            }
        }

        // Draw spline control points
        if (m_isEditMode && m_selectedPoint == i)
        {
            dc.SetColor(dc.GetSelectedColor());
        }
        else
        {
            dc.SetColor(colPoint);
        }

        Vec3 p0 = wtm.TransformPoint(m_points[i].pos);
        float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0);
        Vec3 sz(fScale, fScale, fScale);
        if (isSelected)
        {
            if (m_isEditMode && m_selectedPoint != i)
            {
                dc.DepthTestOff();
            }
            dc.DrawSolidBox(p0 - sz, p0 + sz);
            if (m_isEditMode && m_selectedPoint != i)
            {
                dc.DepthTestOn();
            }
        }
        else
        {
            dc.DrawWireBox(p0 - sz, p0 + sz);
        }
    }

    // Keep for debug
    //DrawJoints(dc);

    if (m_mergeIndex != -1 && m_mergeIndex < GetPointCount())
    {
        dc.SetColor(QColor(127, 255, 127));
        Vec3 p0 = wtm.TransformPoint(m_points[m_mergeIndex].pos);
        float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0);
        Vec3 sz(fScale, fScale, fScale);
        if (isSelected)
        {
            dc.DepthTestOff();
        }
        dc.DrawWireBox(p0 - sz, p0 + sz);
        if (isSelected)
        {
            dc.DepthTestOn();
        }
    }

    DrawDefault(dc, GetColor());
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::DrawJoints(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();
    float fPointSize = 0.5f;

    for (int i = 0, n = GetPointCount(); i < n; ++i)
    {
        Vec3 p0 = wtm.TransformPoint(m_points[i].pos);

        Vec3 pf0 = wtm.TransformPoint(m_points[i].forw);
        float fScalef = fPointSize * dc.view->GetScreenScaleFactor(pf0) * 0.01f;
        Vec3 szf(fScalef, fScalef, fScalef);
        dc.SetColor(QColor(0, 200, 0));
        dc.DrawLine(p0, pf0);
        dc.SetColor(QColor(0, 255, 0));
        if (IsSelected())
        {
            dc.DepthTestOff();
        }
        dc.DrawWireBox(pf0 - szf, pf0 + szf);
        if (IsSelected())
        {
            dc.DepthTestOn();
        }

        Vec3 pb0 = wtm.TransformPoint(m_points[i].back);
        float fScaleb = fPointSize * dc.view->GetScreenScaleFactor(pb0) * 0.01f;
        Vec3 szb(fScaleb, fScaleb, fScaleb);
        dc.SetColor(QColor(0, 200, 0));
        dc.DrawLine(p0, pb0);
        dc.SetColor(QColor(0, 255, 0));
        if (IsSelected())
        {
            dc.DepthTestOff();
        }
        dc.DrawWireBox(pb0 - szb, pb0 + szb);
        if (IsSelected())
        {
            dc.DepthTestOn();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
int CSplineObject::GetNearestPoint(const Vec3& raySrc, const Vec3& rayDir, float& distance)
{
    int index = -1;
    const float kBigRayLen = 100000.0f;
    float minDist = FLT_MAX;
    Vec3 rayLineP1 = raySrc;
    Vec3 rayLineP2 = raySrc + rayDir * kBigRayLen;
    Vec3 intPnt;
    const Matrix34& wtm = GetWorldTM();

    for (int i = 0, n = GetPointCount(); i < n; ++i)
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
bool CSplineObject::HitTest(HitContext& hc)
{
    // First check if ray intersect our bounding box.
    float tr = hc.distanceTolerance / 2 + kSplinePointSelectionRadius;

    // Find intersection of line with zero Z plane.
    float minDist = FLT_MAX;
    Vec3 intPnt;
    //GetNearestEdge( hc.raySrc,hc.rayDir,p1,p2,minDist,intPnt );

    const float kBigRayLen = 100000.0f;
    bool bWasIntersection = false;
    Vec3 ip;
    Vec3 rayLineP1 = hc.raySrc;
    Vec3 rayLineP2 = hc.raySrc + hc.rayDir * kBigRayLen;
    const Matrix34& wtm = GetWorldTM();

    int maxpoint = GetPointCount();
    if (maxpoint-- >= GetMinPoints())
    {
        for (int i = 0; i < maxpoint; ++i)
        {
            int kn = 6;
            for (int k = 0; k < kn; ++k)
            {
                Vec3 pi = wtm.TransformPoint(GetBezierPos(i, float(k) / kn));
                Vec3 pj = wtm.TransformPoint(GetBezierPos(i, float(k) / kn + 1.0f / kn));

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
    }

    if (bWasIntersection)
    {
        float fSplineCloseDistance = kSplinePointSelectionRadius * hc.view->GetScreenScaleFactor(intPnt) * 0.01f;

        if (minDist < fSplineCloseDistance + hc.distanceTolerance)
        {
            hc.dist = hc.raySrc.GetDistance(intPnt);
            hc.object = this;
            return true;
        }
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
bool CSplineObject::HitTestRect(HitContext& hc)
{
    AABB box;
    // Retrieve world space bound box.
    GetBoundBox(box);

    // Project all edges to viewport.
    const Matrix34& wtm = GetWorldTM();

    int maxpoint = GetPointCount();
    if (maxpoint-- >= GetMinPoints())
    {
        for (int i = 0, j = 1; i < maxpoint; ++i, ++j)
        {
            Vec3 p0 = wtm.TransformPoint(m_points[i].pos);
            Vec3 p1 = wtm.TransformPoint(m_points[j].pos);

            QPoint pnt0 = hc.view->WorldToView(p0);
            QPoint pnt1 = hc.view->WorldToView(p1);

            // See if pnt0 to pnt1 line intersects with rectangle.
            // check see if one point is inside rect and other outside, or both inside.
            bool in0 = hc.rect.contains(pnt0);
            bool in1 = hc.rect.contains(pnt1);
            if ((in0 && in1) || (in0 && !in1) || (!in0 && in1))
            {
                hc.object = this;
                return true;
            }
        }
    }

    return false;
}


//////////////////////////////////////////////////////////////////////////
bool CSplineObject::RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt)
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
void CSplineObject::GetNearestEdge(const Vec3& raySrc, const Vec3& rayDir, int& p1, int& p2, float& distance, Vec3& intersectPoint)
{
    p1 = -1;
    p2 = -1;
    const float kBigRayLen = 100000.0f;
    float minDist = FLT_MAX;
    Vec3 intPnt;
    Vec3 rayLineP1 = raySrc;
    Vec3 rayLineP2 = raySrc + rayDir * kBigRayLen;

    const Matrix34& wtm = GetWorldTM();

    int maxpoint = GetPointCount();
    if (maxpoint-- >= GetMinPoints())
    {
        for (int i = 0, j = 1; i < maxpoint; ++i, ++j)
        {
            int kn = 6;
            for (int k = 0; k < kn; ++k)
            {
                Vec3 pi = wtm.TransformPoint(GetBezierPos(i, float(k) / kn));
                Vec3 pj = wtm.TransformPoint(GetBezierPos(i, float(k) / kn + 1.0f / kn));

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
    }

    distance = minDist;
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::OnUpdateUI()
{
    if (m_pSplinePanel)
    {
        m_pSplinePanel->Update();
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);

    if (ar.bLoading)
    {
        m_points.clear();
        XmlNodeRef points = ar.node->findChild("Points");
        if (points)
        {
            for (int i = 0, n = points->getChildCount(); i < n; ++i)
            {
                XmlNodeRef pnt = points->getChild(i);
                CSplinePoint pt;
                pnt->getAttr("Pos", pt.pos);
                pnt->getAttr("Back", pt.back);
                pnt->getAttr("Forw", pt.forw);
                pnt->getAttr("Angle", pt.angle);
                pnt->getAttr("Width", pt.width);
                pnt->getAttr("IsDefaultWidth", pt.isDefaultWidth);
                m_points.push_back(pt);
            }
        }

        CalcBBox();

        // Update UI.
        if (m_pSplinePanel && m_pSplinePanel->GetSpline() == this)
        {
            m_pSplinePanel->SetSpline(this);
        }

        OnUpdate();
        OnUpdateUI();
        UpdateTransformManipulator();
    }
    else // Saving.
    {
        if (!m_points.empty())
        {
            XmlNodeRef points = ar.node->newChild("Points");
            for (int i = 0, n = GetPointCount(); i < n; ++i)
            {
                XmlNodeRef pnt = points->newChild("Point");
                pnt->setAttr("Pos", m_points[i].pos);
                pnt->setAttr("Back", m_points[i].back);
                pnt->setAttr("Forw", m_points[i].forw);
                pnt->setAttr("Angle", m_points[i].angle);
                pnt->setAttr("Width", m_points[i].width);
                pnt->setAttr("IsDefaultWidth", m_points[i].isDefaultWidth);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
int CSplineObject::MouseCreateCallback(CViewport* pView, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown || event == eMouseLDblClick)
    {
        Vec3 pos = pView->MapViewToCP(point);

        if (GetPointCount() < GetMinPoints())
        {
            SetPos(pos);
        }

        pos.z += CreationZOffset();

        if (GetPointCount() == 0)
        {
            InsertPoint(-1, Vec3(0, 0, 0));
        }
        else
        {
            SetPoint(GetPointCount() - 1, pos - GetWorldPos());
        }

        if (event == eMouseLDblClick)
        {
            if (GetPointCount() > GetMinPoints())
            {
                // Remove last unneeded point.
                m_points.pop_back();
                OnUpdate();
                OnUpdateUI();
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
            if (GetPointCount() > GetMinPoints() && vlen.GetLength() < kSplinePointSelectionRadius)
            {
                OnUpdateUI();
                return MOUSECREATE_OK;
            }
            if (GetPointCount() >= GetMaxPoints())
            {
                OnUpdateUI();
                return MOUSECREATE_OK;
            }

            InsertPoint(-1, pos - GetWorldPos());
        }
        return MOUSECREATE_CONTINUE;
    }

    return CBaseObject::MouseCreateCallback(pView, event, point, flags);
}


//////////////////////////////////////////////////////////////////////////
void CSplineObject::UpdateTransformManipulator()
{
    if (m_selectedPoint == -1)
    {
        GetIEditor()->ShowTransformManipulator(false);
    }
    else
    {
        const Matrix34& wtm = GetWorldTM();
        Vec3 pos = wtm.TransformPoint(GetPoint(m_selectedPoint));

        Matrix34A tm;
        tm.SetIdentity();
        tm.SetTranslation(pos);
        GetIEditor()->ShowTransformManipulator(true);
        ITransformManipulator* pManip = GetIEditor()->GetTransformManipulator();
        if (pManip)
        {
            pManip->SetTransformation(COORDS_LOCAL, tm);
            pManip->SetTransformation(COORDS_PARENT, tm);
            pManip->SetTransformation(COORDS_USERDEFINED, tm);
        }
    }
}

#include <Objects/SplineObject.moc>