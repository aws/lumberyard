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

// Description : CRoadObject implementation.


#include "StdAfx.h"
#include "RoadObject.h"
#include "RoadPanel.h"
#include "Viewport.h"
#include "Terrain/Heightmap.h"
#include "Material/Material.h"
#include "../VegetationMap.h"
#include "../VegetationObject.h"

#include <I3DEngine.h>
#include <CryArray.h>
#include "MeasurementSystem/MeasurementSystem.h"

namespace
{
    // This is a quadratic ease in out function.  t is expected to be [0.0 - 1.0]
    // This is used to prevent sharp angles from appearing at a road's narrowest point.
    float EaseInOut(float t)
    {
        t *= 2.0f;
        if (t < 1)
        {
            return 0.5f * t * t;
        }
        t -= 1.0f;
        return -0.5f * (t * (t - 2.0f) - 1.0f);
    }
}

//////////////////////////////////////////////////////////////////////////
// class CRoadSector

void CRoadSector::Release()
{
    if (m_pRoadSector)
    {
        GetIEditor()->Get3DEngine()->DeleteRenderNode(m_pRoadSector);
    }
    m_pRoadSector = 0;
}


//////////////////////////////////////////////////////////////////////////
// CRoadObject implementation.
//////////////////////////////////////////////////////////////////////////

#define RAY_DISTANCE 100000.0f

//////////////////////////////////////////////////////////////////////////
int CRoadObject::m_rollupId = 0;
CRoadPanel* CRoadObject::m_panel = 0;

//////////////////////////////////////////////////////////////////////////
CRoadObject::CRoadObject()
{
    m_bNeedUpdateSectors = true;
    mv_width = 4.0f;
    mv_borderWidth = 6.0f;
    mv_eraseVegWidth = 3.f;
    mv_eraseVegWidthVar = 0.f;
    mv_step = 4.0f;
    mv_tileLength = 4.0f;
    mv_sortPrio = 0;
    m_ignoreTerrainHoles = false;
    m_physicalize = false;

    m_bIgnoreParamUpdate = false;

    SetColor(Vec2Rgb(Vec3(0, 0.8f, 1)));
    mv_viewDistanceMultiplier = 1.0f;
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::Done()
{
    m_sectors.clear();
    CSplineObject::Done();
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::InitBaseVariables()
{
    AddVariable(mv_width, "Width", functor(*this, &CRoadObject::OnParamChange));
    AddVariable(mv_borderWidth, "BorderWidth", functor(*this, &CRoadObject::OnParamChange));
    AddVariable(mv_eraseVegWidth, "EraseVegWidth");
    AddVariable(mv_eraseVegWidthVar, "EraseVegWidthVar");
    AddVariable(mv_step, "StepSize", functor(*this, &CRoadObject::OnParamChange));
    AddVariable(mv_viewDistanceMultiplier, "ViewDistanceMultiplier", functor(*this, &CRoadObject::OnParamChange));
    mv_viewDistanceMultiplier.SetLimits(0.0f, IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX);
    AddVariable(mv_tileLength, "TileLength", functor(*this, &CRoadObject::OnParamChange));
    mv_step.SetLimits(0.25f, 10.f);
    mv_tileLength.SetLimits(0.001f, 1000.f);
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::InitVariables()
{
    InitBaseVariables();

    AddVariable(mv_sortPrio, "SortPriority", functor(*this, &CRoadObject::OnParamChange));
    mv_sortPrio.SetLimits(0, 255);

    AddVariable(m_ignoreTerrainHoles, "IgnoreTerrainHoles", functor(*this, &CRoadObject::OnParamChange));
    AddVariable(m_physicalize, "Physicalize", functor(*this, &CRoadObject::OnParamChange));
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::InvalidateTM(int nWhyFlags)
{
    CSplineObject::InvalidateTM(nWhyFlags);
    SetRoadSectors();
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::BeginEditParams(IEditor* ie, int flags)
{
    CSplineObject::BeginEditParams(ie, flags);

    if (!m_panel)
    {
        m_panel = new CRoadPanel;
        m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, (GetClassDesc()->ClassName() + " Parameters"), m_panel);
    }
    if (m_panel)
    {
        m_panel->SetRoad(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::EndEditParams(IEditor* ie)
{
    if (m_rollupId != 0)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupId);
    }
    m_rollupId = 0;
    m_panel = 0;

    CSplineObject::EndEditParams(ie);
}


//////////////////////////////////////////////////////////////////////////
float CRoadObject::GetLocalWidth(int index, float t)
{
    float kof = t;
    int i = index;

    if (index >= m_points.size() - 1)
    {
        kof = 1.0f + t;
        i = m_points.size() - 2;
        if (i < 0)
        {
            return mv_width;
        }
    }

    float an1 = m_points[i  ].isDefaultWidth ? mv_width : m_points[i  ].width;
    float an2 = m_points[i + 1].isDefaultWidth ? mv_width : m_points[i + 1].width;

    if (an1 == an2)
    {
        return an1;
    }

    float af = EaseInOut(kof);

    return ((1.0f - af) * an1 + af * an2);
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::OnUpdate()
{
    SetRoadSectors();
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetRoadSectors()
{
    const Matrix34& wtm = GetWorldTM();

    int sizeOld = m_sectors.size();
    int sizeNew = 0;

    int points_size = m_points.size();

    float fSegLen = 0.0f;

    for (int i = 0; i < points_size - 1; ++i)
    {
        int kn = GetRoadSectorCount(i);

        for (int k = 0; k <= kn; ++k)
        {
            if (i != points_size - 2 && k == kn)
            {
                break;
            }
            float t = float(k) / kn;
            Vec3 p = GetBezierPos(i, t);
            Vec3 n = GetLocalBezierNormal(i, t);

            float fWidth = GetLocalWidth(i, t);

            Vec3 r = p - 0.5f * fWidth * n;
            Vec3 l = p + 0.5f * fWidth * n;

            if (sizeNew >= sizeOld)
            {
                CRoadSector sector;
                m_sectors.push_back(sector);
            }

            m_sectors[sizeNew].points.clear();
            m_sectors[sizeNew].points.push_back(l);
            m_sectors[sizeNew].points.push_back(r);
            m_sectors[sizeNew].points.push_back(l);
            m_sectors[sizeNew].points.push_back(r);

            m_sectors[sizeNew].t0 = (fSegLen + GetBezierSegmentLength(i, t)) / mv_tileLength;

            sizeNew++;
        }
        fSegLen += GetBezierSegmentLength(i);
    }

    if (sizeNew < sizeOld)
    {
        m_sectors.resize(sizeNew);
    }

    for (size_t i = 0; i < m_sectors.size(); ++i)
    {
        m_sectors[i].points[0] = wtm.TransformPoint(m_sectors[i].points[0]);
        m_sectors[i].points[1] = wtm.TransformPoint(m_sectors[i].points[1]);

        if (i > 0)
        {
            // adjust mesh
            m_sectors[i - 1].points[2] = m_sectors[i].points[0];
            m_sectors[i - 1].points[3] = m_sectors[i].points[1];
            m_sectors[i - 1].t1 = m_sectors[i].t0;
        }
    }

    if (m_sectors.size() > 0)
    {
        m_sectors.pop_back();
    }

    // mark end of the road for road alpha fading
    if (m_sectors.size() > 0)
    {
        m_sectors[m_sectors.size() - 1].t1 *= -1.f;
    }

    // call only in the end of this function
    UpdateSectors();
}


//////////////////////////////////////////////////////////////////////////
int CRoadObject::GetRoadSectorCount(int index)
{
    int kn = int((GetBezierSegmentLength(index) + 0.5f) / GetStepSize());
    if (kn == 0)
    {
        return 1;
    }
    return kn;
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::UpdateSectors()
{
    if (!m_bNeedUpdateSectors)
    {
        return;
    }

    m_mergeIndex = -1;

    if (!m_sectors.size())
    {
        return;
    }

    for (size_t i = 0; i < m_sectors.size(); ++i)
    {
        if (m_sectors[i].m_pRoadSector)
        {
            GetIEditor()->Get3DEngine()->DeleteRenderNode(m_sectors[i].m_pRoadSector);
        }
        m_sectors[i].m_pRoadSector = 0;
    }

    int MAX_TRAPEZOIDS_IN_CHUNK = 16;

    int nChunksNum = m_sectors.size() / MAX_TRAPEZOIDS_IN_CHUNK + 1;

    CRoadSector& sectorFirstGlobal = m_sectors[0];
    CRoadSector& sectorLastGlobal = m_sectors[m_sectors.size() - 1];

    for (int nChunkId = 0; nChunkId < nChunksNum; ++nChunkId)
    {
        int nStartSecId = nChunkId * MAX_TRAPEZOIDS_IN_CHUNK;
        int nSectorsNum = min(MAX_TRAPEZOIDS_IN_CHUNK, (int)m_sectors.size() - nStartSecId);

        if (!nSectorsNum)
        {
            break;
        }

        CRoadSector& sectorFirst = m_sectors[nStartSecId];

        // make render node
        if (!sectorFirst.m_pRoadSector)
        {
            sectorFirst.m_pRoadSector = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Road);
        }
        if (!sectorFirst.m_pRoadSector)
        {
            return;
        }
        int renderFlags = 0;
        if (CheckFlags(OBJFLAG_INVISIBLE) || IsHiddenBySpec())
        {
            renderFlags = ERF_HIDDEN;
        }
        sectorFirst.m_pRoadSector->SetRndFlags(renderFlags);
        sectorFirst.m_pRoadSector->SetViewDistanceMultiplier(mv_viewDistanceMultiplier);
        sectorFirst.m_pRoadSector->SetMinSpec(GetMinSpec());
        sectorFirst.m_pRoadSector->SetMaterialLayers(GetMaterialLayersMask());

        // make list of verts
        PodArray<Vec3> lstPoints;
        for (int i = 0; i < nSectorsNum; ++i)
        {
            lstPoints.Add(m_sectors[nStartSecId + i].points[0]);
            lstPoints.Add(m_sectors[nStartSecId + i].points[1]);
        }

        CRoadSector& sectorLast = m_sectors[nStartSecId + nSectorsNum - 1];

        // Extend final boundary to cover holes in roads caused by f16 meshes.
        // Overlapping the roads slightly seems to be the nicest way to fix the issue
        Vec3 sectorLastOffset2 = sectorLast.points[2] - sectorLast.points[0];
        Vec3 sectorLastOffset3 = sectorLast.points[3] - sectorLast.points[1];

        sectorLastOffset2.Normalize();
        sectorLastOffset3.Normalize();

        const float sectorLastOffset = 0.075f;
        sectorLastOffset2 *= sectorLastOffset;
        sectorLastOffset3 *= sectorLastOffset;

        lstPoints.Add(sectorLast.points[2] + sectorLastOffset2);
        lstPoints.Add(sectorLast.points[3] + sectorLastOffset3);

        //assert(lstPoints.Count()>=4);
        assert(!(lstPoints.Count() & 1));

        IRoadRenderNode* pRoadRN = static_cast<IRoadRenderNode*>(sectorFirst.m_pRoadSector);
        pRoadRN->SetVertices(lstPoints.GetElements(), lstPoints.Count(), fabs(sectorFirst.t0), fabs(sectorLast.t1), fabs(sectorFirstGlobal.t0), fabs(sectorLastGlobal.t1));
        pRoadRN->SetSortPriority(mv_sortPrio);
        pRoadRN->SetIgnoreTerrainHoles(m_ignoreTerrainHoles);
        pRoadRN->SetPhysicalize(m_physicalize);

        if (GetMaterial())
        {
            GetMaterial()->AssignToEntity(sectorFirst.m_pRoadSector);
        }
        else
        {
            sectorFirst.m_pRoadSector->SetMaterial(NULL);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetHidden(bool bHidden)
{
    CSplineObject::SetHidden(bHidden);
    UpdateSectors();
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::UpdateVisibility(bool visible)
{
    if (visible == CheckFlags(OBJFLAG_INVISIBLE))
    {
        CSplineObject::UpdateVisibility(visible);
        UpdateSectors();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetMaterial(CMaterial* pMaterial)
{
    CMaterial* pPrevMaterial = GetMaterial();
    CSplineObject::SetMaterial(pMaterial);

    if (pPrevMaterial != pMaterial)
    {
        UpdateSectors();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::DrawSectorLines(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();
    float fPointSize = 0.5f;

    dc.SetColor(QColor(127, 127, 255));
    for (size_t i = 0; i < m_sectors.size(); ++i)
    {
        dc.DrawLine(m_sectors[i].points[0], m_sectors[i].points[1]);
        for (size_t k = 0; k < m_sectors[i].points.size(); k += 2)
        {
            if (k + 3 < m_sectors[i].points.size())
            {
                dc.DrawLine(m_sectors[i].points[k + 1], m_sectors[i].points[k + 3]);
                dc.DrawLine(m_sectors[i].points[k + 3], m_sectors[i].points[k + 2]);
                dc.DrawLine(m_sectors[i].points[k + 2], m_sectors[i].points[k]);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::DrawRoadObject(DisplayContext& dc, const QColor& col)
{
    if (IsSelected())
    {
        if (CMeasurementSystem::GetMeasurementSystem().IsMeasurementToolActivated() &&
            CMeasurementSystem::GetMeasurementSystem().GetStartPointIndex() != CMeasurementSystem::GetMeasurementSystem().GetEndPointIndex())
        {
            ((CRoadObjectEnhanced*)this)->DrawMeasurementSystemInfo(dc, col);
        }
        else
        {
            DrawSectorLines(dc);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::Display(DisplayContext& dc)
{
    QColor col(Qt::black);

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

        DrawRoadObject(dc, col);
    }
    CSplineObject::Display(dc);
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::Serialize(CObjectArchive& ar)
{
    m_bIgnoreParamUpdate = true;
    m_bNeedUpdateSectors = false;
    CSplineObject::Serialize(ar);
    m_bNeedUpdateSectors = true;
    m_bIgnoreParamUpdate = false;
    if (ar.bLoading)
    {
        UpdateSectors();
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CRoadObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    //XmlNodeRef objNode = CSplineObject::Export( levelPath,xmlNode );
    //return objNode;
    return 0;
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::OnParamChange(IVariable* var)
{
    if (!m_bIgnoreParamUpdate)
    {
        SetRoadSectors();
    }
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetSelected(bool bSelect)
{
    CSplineObject::SetSelected(bSelect);
    SetRoadSectors();
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::AlignHeightMap()
{
    if (!GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->BeginUndo();
    }

    CHeightmap* heightmap = GetIEditor()->GetHeightmap();
    int unitSize = heightmap->GetUnitSize();
    const Matrix34& wtm = GetWorldTM();

    int minx = 0, miny = 0, maxx = 0, maxy = 0;
    bool bIsInitMinMax = false;

    const int nPoints = GetPointCount();

    for (int i = 0; i < nPoints - 1; ++i)
    {
        float fminx = 0, fminy = 0, fmaxx = 0, fmaxy = 0;
        bool bIsInitminmax = false;
        int kn = int (0.5f + (GetBezierSegmentLength(i) + 1) * 4);

        Vec3 tmp;

        for (int k = 0; k <= kn; ++k)
        {
            float t = float(k) / kn;
            Vec3 p = GetBezierPos(i, t);
            Vec3 n = GetLocalBezierNormal(i, t);

            float fWidth = GetLocalWidth(i, t);
            if (fWidth < 2)
            {
                fWidth = 2;
            }

            Vec3 p1 = p - (0.5f * fWidth + mv_borderWidth + 0.5f * unitSize) * n;
            Vec3 p2 = p + (0.5f * fWidth + mv_borderWidth + 0.5f * unitSize) * n;

            p1 = wtm.TransformPoint(p1);
            p2 = wtm.TransformPoint(p2);

            tmp = p1;

            if (!bIsInitminmax)
            {
                fminx = p1.x;
                fminy = p1.y;
                fmaxx = p1.x;
                fmaxy = p1.y;
                bIsInitminmax = true;
            }
            fminx = min(fminx, p1.x);
            fminx = min(fminx, p2.x);
            fminy = min(fminy, p1.y);
            fminy = min(fminy, p2.y);
            fmaxx = max(fmaxx, p1.x);
            fmaxx = max(fmaxx, p2.x);
            fmaxy = max(fmaxy, p1.y);
            fmaxy = max(fmaxy, p2.y);
        }

        heightmap->RecordUndo(int(fminy / unitSize) - 1, int(fminx / unitSize) - 1, int(fmaxy / unitSize) + unitSize - int(fminy / unitSize) + 1, int(fmaxx / unitSize) + unitSize - int(fminx / unitSize) + 1);

        // Note: Heightmap is rotated compared to world coordinates.
        for (int ty = int(fminx / unitSize); ty <= int(fmaxx / unitSize) + unitSize && ty < heightmap->GetHeight(); ++ty)
        {
            for (int tx = int(fminy / unitSize); tx <= int(fmaxy / unitSize) + unitSize && tx < heightmap->GetWidth(); ++tx)
            {
                int x = ty * unitSize;
                int y = tx * unitSize;

                Vec3 p3 = Vec3(x, y, 0.0f);

                int findk = -1;
                float fWidth = GetLocalWidth(i, 0);
                float fWidth1 = GetLocalWidth(i, 1);
                if (fWidth < fWidth1)
                {
                    fWidth = fWidth1;
                }
                float mind = 0.5f * fWidth + mv_borderWidth + 0.5f * unitSize;

                for (int k = 0; k < kn; ++k)
                {
                    float t = float(k) / kn;
                    Vec3 p1 = wtm.TransformPoint(GetBezierPos(i, t));
                    Vec3 p2 = wtm.TransformPoint(GetBezierPos(i, t + 1.0f / kn));
                    p1.z = 0.0f;
                    p2.z = 0.0f;



                    //Vec3 d = p2 - p1;
                    //float u = d.Dot(p3-p1) / (d).GetLengthSquared();
                    //if (-0.1f <= u && u <=1.1f)
                    //{
                    float d = PointToLineDistance (p1, p2, p3);
                    if (d < mind)
                    {
                        findk = k;
                        mind = d;
                    }
                    //}
                }

                if (findk != -1)
                {
                    float t = float(findk) / kn;

                    float st = 1.0f / kn;
                    int sign = 1;
                    for (int tt = 0; tt < 24; ++tt)
                    {
                        Vec3 p0 = wtm.TransformPoint(GetBezierPos(i, t));
                        ;
                        p0.z = 0.0f;

                        Vec3 ploc = GetBezierPos(i, t);
                        Vec3 nloc = GetLocalBezierNormal(i, t);

                        Vec3 p1 = wtm.TransformPoint(ploc - nloc);
                        p1.z = 0.0f;

                        if (((p0 - p1).Cross(p3 - p1).z) < 0.0f)
                        {
                            sign = 1;
                        }
                        else
                        {
                            sign = -1;
                        }

                        st /= 1.618f;
                        t = t + sign * st;
                    }

                    if (t < 0.0f || t > 1.0f)
                    {
                        continue;
                    }


                    Vec3 p1 = wtm.TransformPoint(GetBezierPos(i, t));
                    Vec3 p2 = wtm.TransformPoint(GetBezierPos(i, t + 1.0f / kn));

                    Vec3 p1_0 = p1;
                    p1_0.z = 0.0f;
                    p2.z = 0.0f;
                    Vec3 p = Vec3(x, y, 0.0f);
                    Vec3 e = (p2 - p1_0).Cross(p1_0 - p);

                    Vec3 p1loc = GetBezierPos(i, t);
                    Vec3 nloc = GetLocalBezierNormal(i, t);

                    Vec3 n = wtm.TransformPoint(p1loc - nloc) - p1;
                    n.Normalize();

                    Vec3 nproj = n;
                    nproj.z = 0.0f;

                    float kof = nproj.GetLength();

                    float length = (p1_0 - p).GetLength() / kof;

                    Vec3 pos;

                    if (e.z < 0.0f)
                    {
                        pos = p1 - length * n;
                    }
                    else
                    {
                        pos = p1 + length * n;
                    }

                    float fWidth = GetLocalWidth(i, t);
                    if (length <= fWidth / 2 + mv_borderWidth + 0.5 * unitSize)
                    {
                        //int tx = RoundFloatToInt(y / unitSize);
                        //int ty = RoundFloatToInt(x / unitSize);

                        float z;
                        if (length <= fWidth / 2 + 0.5 * unitSize)
                        {
                            z = pos.z;
                        }
                        else
                        {
                            //continue;
                            float kof = (length - (fWidth / 2 + 0.5 * unitSize)) / mv_borderWidth;
                            kof = 1.0f - (cos(kof * 3.141593f) + 1.0f) / 2;
                            float z1 = heightmap->GetXY(tx, ty);
                            z = kof * z1 + (1.0f - kof) * pos.z;
                        }

                        heightmap->SetXY(tx, ty, clamp_tpl(z, 0.0f, heightmap->GetMaxHeight()));

                        if (!bIsInitMinMax)
                        {
                            minx = tx;
                            miny = ty;
                            maxx = tx;
                            maxy = ty;
                            bIsInitMinMax = true;
                        }
                        if (minx > tx)
                        {
                            minx = tx;
                        }
                        if (miny > ty)
                        {
                            miny = ty;
                        }
                        if (maxx < tx)
                        {
                            maxx = tx;
                        }
                        if (maxy < ty)
                        {
                            maxy = ty;
                        }
                    }
                }
            }
        }
        //break;
    }

    int w = maxx - minx;
    if (w < maxy - miny)
    {
        w = maxy - miny;
    }
    heightmap->UpdateEngineTerrain(minx, miny, w, w, true, false);

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->AcceptUndo("Heightmap Aligning");
    }

    SetRoadSectors();
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::EraseVegetation()
{
    if (CVegetationMap* vegetationMap = GetIEditor()->GetVegetationMap())
    {
        if (!GetIEditor()->IsUndoRecording())
        {
            GetIEditor()->BeginUndo();
        }

        bool bModified = false;

        vegetationMap->StoreBaseUndo(CVegetationMap::eStoreUndo_Begin);

        const Matrix34& wtm = GetWorldTM();
        const int nPoints = GetPointCount();
        const float fEraseWidth = mv_eraseVegWidth + mv_eraseVegWidthVar * 0.5f;
        const float fSpareWidth = 5.0f;

        for (int i = 0; i < nPoints - 1; ++i)
        {
            const int nSegment = GetBezierSegmentLength(i);
            const int nSector = GetRoadSectorCount(i);

            for (int k = 0; k < nSector; ++k)
            {
                const float t1 = float(k) / nSector;
                const float t2 = float(k + 1) / nSector;

                const Vec3 p1 = GetBezierPos(i, t1);
                const Vec3 n1 = GetLocalBezierNormal(i, t1);
                const Vec3 p2 = GetBezierPos(i, t2);
                const Vec3 n2 = GetLocalBezierNormal(i, t2);

                const float fRoadWidth1 = GetLocalWidth(i, t1);
                const float fRoadWidth2 = GetLocalWidth(i, t2);

                const Vec3 r1(wtm.TransformPoint(p1 - (0.5f * fRoadWidth1 + fEraseWidth + fSpareWidth) * n1));
                const Vec3 l1(wtm.TransformPoint(p1 + (0.5f * fRoadWidth1 + fEraseWidth + fSpareWidth) * n1));
                const Vec3 r2(wtm.TransformPoint(p2 - (0.5f * fRoadWidth2 + fEraseWidth + fSpareWidth) * n2));
                const Vec3 l2(wtm.TransformPoint(p2 + (0.5f * fRoadWidth2 + fEraseWidth + fSpareWidth) * n2));

                const float fminx = min(r1.x, min(l1.x, min(r2.x, l2.x)));
                const float fminy = min(r1.y, min(l1.y, min(r2.y, l2.y)));
                const float fmaxx = max(r1.x, max(l1.x, max(r2.x, l2.x)));
                const float fmaxy = max(r1.y, max(l1.y, max(r2.y, l2.y)));

                std::vector<CVegetationInstance*> instances;
                vegetationMap->GetObjectInstances(fminx, fminy, fmaxx, fmaxy, instances);

                const int segmentStart = k * nSegment / nSector;
                const int segmentEnd = (k + 1) * nSegment / nSector;

                for (std::vector<CVegetationInstance*>::iterator it = instances.begin(); it != instances.end(); ++it)
                {
                    float fFinalEraseWidth = fEraseWidth;

                    if (mv_eraseVegWidthVar != 0.f)
                    {
                        fFinalEraseWidth -= float( rand() % int( mv_eraseVegWidthVar * 10.f )) * 0.1f;
                    }
                    if (CVegetationObject* obj = (*it)->object)
                    {
                        fFinalEraseWidth += obj->GetObjectSize() * 0.5f;
                    }
                    for (int l = segmentStart; l < segmentEnd; ++l)
                    {
                        const float ts = float(l) / nSegment;
                        const float fWidth = 0.5f * GetLocalWidth(i, ts) + fFinalEraseWidth;

                        const Vec2 pos1(wtm.TransformPoint(GetBezierPos(i, ts)));
                        const Vec2 pos2((*it)->pos);

                        const float fDistance = pos1.GetDistance(pos2);

                        if (fDistance <= fWidth)
                        {
                            vegetationMap->StoreBaseUndo(CVegetationMap::eStoreUndo_Once);
                            vegetationMap->DeleteObjInstance(*it);

                            if (!bModified)
                            {
                                GetIEditor()->SetModifiedFlag();
                                GetIEditor()->SetModifiedModule(eModifiedTerrain);
                                bModified = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
        vegetationMap->StoreBaseUndo(CVegetationMap::eStoreUndo_End);

        if (GetIEditor()->IsUndoRecording())
        {
            if (bModified)
            {
                GetIEditor()->AcceptUndo("Erase Vegetation On Road");
            }
            else
            {
                GetIEditor()->CancelUndo();
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    CSplineObject::SetMinSpec(nSpec, bSetChildren);
    UpdateSectors();
}

//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    CSplineObject::SetMaterialLayersMask(nLayersMask);
    UpdateSectors();
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetLayerId(uint16 nLayerId)
{
    for (size_t i = 0; i < m_sectors.size(); ++i)
    {
        if (m_sectors[i].m_pRoadSector)
        {
            m_sectors[i].m_pRoadSector->SetLayerId(nLayerId);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CRoadObject::SetPhysics(bool isPhysics)
{
    for (size_t i = 0; i < m_sectors.size(); ++i)
    {
        IRenderNode* pRenderNode = m_sectors[i].m_pRoadSector;
        if (pRenderNode)
        {
            if (isPhysics)
            {
                pRenderNode->SetRndFlags(pRenderNode->GetRndFlags() & ~ERF_NO_PHYSICS);
            }
            else
            {
                pRenderNode->SetRndFlags(pRenderNode->GetRndFlags() | ERF_NO_PHYSICS);
            }
        }
    }
}







//////////////////////////////////////////////////////////////////////////
// Class Description of RoadObject.


//////////////////////////////////////////////////////////////////////////
class CRoadObjectClassDesc
    : public CObjectClassDesc
{
public:
    REFGUID ClassID()
    {
        // {DEDDEDBD-636E-4B27-915F-18E15B9D3831}
        static const GUID guid =
        {
            0xdeddedbd, 0x636e, 0x4b27, { 0x91, 0x5f, 0x18, 0xe1, 0x5b, 0x9d, 0x38, 0x31 }
        };

        return guid;
    }
    ObjectType GetObjectType() { return OBJTYPE_ROAD; };
    QString ClassName() { return "Road"; };
    QString Category() { return "Misc"; };
    QObject* CreateQObject() const override { return new CRoadObject; }
    int GameCreationOrder() { return 50; };
};

REGISTER_CLASS_DESC(CRoadObjectClassDesc);

#include <Objects/RoadObject.moc>