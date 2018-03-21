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

// Description : CShapeObject implementation.

#include "StdAfx.h"
#include "ShapeObject.h"
#include "../ShapePanel.h"
#include "../Viewport.h"
#include "Objects/AIWave.h"
#include "Util/Triangulate.h"
#include "AI/AIManager.h"

#include <I3DEngine.h>
#include <IAISystem.h>

#include <vector>
#include "IEntitySystem.h"
#include "EntityPanel.h"

#include "GameEngine.h"
#include "AI/NavDataGeneration/Navigation.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>

#include <IGameFramework.h>
#include <IGameVolumes.h>

#include "Util/BoostPythonHelpers.h"
#include <IEntityHelper.h>
#include "Components/IComponentArea.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

CNavigation* GetNavigation ();

CGraph* GetGraph ();

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

#define RAY_DISTANCE 100000.0f

//////////////////////////////////////////////////////////////////////////
int CShapeObject::m_rollupId                                                        = 0;
ShapeEditSplitPanel* CShapeObject::m_panel = 0;
int CShapeObject::m_rollupMultyId                                               = 0;
CShapeMultySelPanel* CShapeObject::m_panelMulty                 = 0;
CAxisHelper CShapeObject::m_selectedPointAxis;
ReflectedPropertiesPanel* CShapeObject::m_pSoundPropertiesPanel = nullptr;
int CShapeObject::m_nSoundPanelID                                               = 0;
CVarBlockPtr CShapeObject::m_pSoundPanelVarBlock                = NULL;

//////////////////////////////////////////////////////////////////////////
CShapeObject::CShapeObject()
{
    m_useAxisHelper = false;
    m_bForce2D = false;
    m_bNoCulling = false;
    mv_closed = true;

    mv_areaId = 0;

    mv_groupId = 0;
    mv_priority = 0;
    mv_width = 0.0f;
    mv_height = 0.0f;
    mv_displayFilled = false;
    mv_displaySoundInfo = false;

    mv_agentHeight = 0.0f;
    mv_agentHeight = 0.0f;
    mv_VoxelOffsetX = 0.0f;
    mv_VoxelOffsetY = 0.0f;
    mv_renderVoxelGrid = false;

    m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    m_selectedPoint = -1;
    m_lowestHeight = 0;
    m_bIgnoreGameUpdate = true;
    m_bAreaModified = true;
    m_bDisplayFilledWhenSelected = true;
    m_entityClass = "AreaShape";
    m_bPerVertexHeight = false;

    m_numSplitPoints = 0;
    m_mergeIndex = -1;

    m_updateSucceed = true;

    m_bBeingManuallyCreated = false;

    SetColor(Vec2Rgb(Vec3(0, 0.8f, 1)));

    if (!m_pSoundPanelVarBlock) // Static
    {
        m_pSoundPanelVarBlock = new CVarBlock;
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::Done()
{
    m_entities.Clear();
    m_points.clear();
    UpdateGameArea(true);
    CEntityObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CShapeObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    m_bIgnoreGameUpdate = true;

    bool res = CEntityObject::Init(ie, prev, file);

    if (prev)
    {
        m_points = ((CShapeObject*)prev)->m_points;
        m_bIgnoreGameUpdate = false;
        mv_closed = ((CShapeObject*)prev)->mv_closed;
        m_abObstructSound   = ((CShapeObject*)prev)->m_abObstructSound;
        CalcBBox();
    }

    m_bIgnoreGameUpdate = false;

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::InitVariables()
{
    AddVariable(mv_width, "Width", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_height, "Height", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_areaId, "AreaId", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_groupId, "GroupId", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_priority, "Priority", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_closed, "Closed", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_displayFilled, "DisplayFilled", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_displaySoundInfo, "DisplaySoundInfo", functor(*this, &CShapeObject::OnSoundParamsChange));
    AddVariable(mv_agentHeight, "Agent_height", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_agentWidth, "Agent_width", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_renderVoxelGrid, "Render_voxel_grid", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_VoxelOffsetX, "voxel_offset_x", functor(*this, &CShapeObject::OnShapeChange));
    AddVariable(mv_VoxelOffsetY, "voxel_offset_y", functor(*this, &CShapeObject::OnShapeChange));
}
//////////////////////////////////////////////////////////////////////////
void CShapeObject::SetName(const QString& name)
{
    CEntityObject::SetName(name);
    m_bAreaModified = true;

    if (!IsOnlyUpdateOnUnselect() && !m_bIgnoreGameUpdate)
    {
        UpdateGameArea();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::InvalidateTM(int nWhyFlags)
{
    CEntityObject::InvalidateTM(nWhyFlags);
    m_bAreaModified = true;
    //  CalcBBox();

    if (nWhyFlags & eObjectUpdateFlags_RestoreUndo) // Can skip updating game object when restoring undo.
    {
        return;
    }

    if (!IsOnlyUpdateOnUnselect() && !m_bIgnoreGameUpdate)
    {
        UpdateGameArea();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CShapeObject::ConvertFromObject(CBaseObject* pObject)
{
    CEntityObject::ConvertFromObject(pObject);

    if (qobject_cast<CShapeObject*>(pObject))
    {
        CShapeObject* pShapeObject = (CShapeObject*)pObject;
        ClearPoints();
        for (int i = 0; i < pShapeObject->GetPointCount(); ++i)
        {
            InsertPoint(-1, pShapeObject->GetPoint(i), false);
        }
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::GetBoundBox(AABB& box)
{
    box.SetTransformedAABB(GetWorldTM(), m_bbox);
    float s = 1.0f;
    box.min -= Vec3(s, s, s);
    box.max += Vec3(s, s, s);
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::GetLocalBounds(AABB& box)
{
    box = m_bbox;
}

//////////////////////////////////////////////////////////////////////////
bool CShapeObject::HitTest(HitContext& hc)
{
    // First check if ray intersect our bounding box.
    float tr = hc.distanceTolerance / 2 + SHAPE_CLOSE_DISTANCE;

    // Find intersection of line with zero Z plane.
    float minDist = FLT_MAX;
    Vec3 intPnt(0, 0, 0);
    //GetNearestEdge( hc.raySrc,hc.rayDir,p1,p2,minDist,intPnt );

    bool bWasIntersection = false;
    Vec3 ip(0, 0, 0);
    Vec3 rayLineP1 = hc.raySrc;
    Vec3 rayLineP2 = hc.raySrc + hc.rayDir * RAY_DISTANCE;
    const Matrix34& wtm = GetWorldTM();

    for (int i = 0; i < m_points.size(); i++)
    {
        int j = (i < m_points.size() - 1) ? i + 1 : 0;

        if (!mv_closed && j == 0 && i != 0)
        {
            continue;
        }

        Vec3 pi = wtm.TransformPoint(m_points[i]);
        Vec3 pj = wtm.TransformPoint(m_points[j]);

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

        if (mv_height > 0)
        {
            if (RayToLineDistance(rayLineP1, rayLineP2, pi + Vec3(0, 0, mv_height), pj + Vec3(0, 0, mv_height), d, ip))
            {
                if (d < minDist)
                {
                    bWasIntersection = true;
                    minDist = d;
                    intPnt = ip;
                }
            }
            if (RayToLineDistance(rayLineP1, rayLineP2, pi, pi + Vec3(0, 0, mv_height), d, ip))
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

    float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * hc.view->GetScreenScaleFactor(intPnt) * 0.01f;

    if (bWasIntersection && minDist < fShapeCloseDistance + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(intPnt);
        hc.object = this;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);

    if (!m_panel)
    {
        auto panel = new CShapePanel;
        m_panel = panel;
        m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "Shape Parameters", panel);
    }
    if (m_panel)
    {
        m_panel->SetShape(this);
    }

    // Make sure to first remove any old sound-obstruction-roll-up-page in case EndEditParams() didn't get called on a previous instance
    if (m_nSoundPanelID != 0)
    {
        // Internally a var block holds "IVariablePtr", on destroy delete is already called
        m_pSoundPanelVarBlock->DeleteAllVariables();

        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
        m_nSoundPanelID = 0;
        m_pSoundPropertiesPanel = new ReflectedPropertiesPanel;
        m_pSoundPropertiesPanel->Setup();
    }
    else if (!m_pSoundPropertiesPanel) // Static
    {
        m_pSoundPropertiesPanel = new ReflectedPropertiesPanel;
        m_pSoundPropertiesPanel->Setup();
    }

    if (!m_bIgnoreGameUpdate)
    {
        UpdateSoundPanelParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::EndEditParams(IEditor* ie)
{
    if (m_rollupId != 0)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupId);
    }

    m_rollupId          = 0;
    m_panel                 = 0;

    if (m_nSoundPanelID != 0)
    {
        // Internally a var block holds "IVariablePtr", on destroy delete is already called
        m_pSoundPanelVarBlock->DeleteAllVariables();

        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
        m_nSoundPanelID                 = 0;
        m_pSoundPropertiesPanel = NULL;
    }

    CalcBBox();

    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CEntityObject::BeginEditMultiSelParams(bAllOfSameType);

    if (!bAllOfSameType)
    {
        return;
    }

    if (!m_panelMulty)
    {
        m_panelMulty = new CShapeMultySelPanel;
        m_rollupMultyId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Multi Shape Operation", m_panelMulty);
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::EndEditMultiSelParams()
{
    if (m_rollupMultyId != 0)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupMultyId);
        CalcBBox();
    }
    m_rollupMultyId = 0;
    m_panelMulty = 0;

    CEntityObject::EndEditMultiSelParams();
}


//////////////////////////////////////////////////////////////////////////
int CShapeObject::MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseMove || event == eMouseLDown || event == eMouseLDblClick)
    {
        Vec3 pos = view->MapViewToCP(point);

        bool firstTime = false;
        if (m_points.size() < 2)
        {
            SetPos(pos);
        }

        pos.z += GetShapeZOffset();

        if (m_points.size() == 0)
        {
            InsertPoint(-1, Vec3(0, 0, 0), false);
            firstTime = true;
        }
        else
        {
            SetPoint(m_points.size() - 1, pos - GetWorldPos());
        }

        if (event == eMouseLDblClick)
        {
            if (m_points.size() > GetMinPoints())
            {
                m_points.pop_back(); // Remove last unneeded point.
                m_abObstructSound.pop_back();   // Same with the "side sound obstruction list"
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
            m_bBeingManuallyCreated = true;

            Vec3 vlen = Vec3(pos.x, pos.y, 0) - Vec3(GetWorldPos().x, GetWorldPos().y, 0);
            /* Disable that for now.
            if (m_points.size() > 2 && vlen.GetLength() < SHAPE_CLOSE_DISTANCE)
            {
                EndCreation();
                return MOUSECREATE_OK;
            }
            */
            if (GetPointCount() >= GetMaxPoints())
            {
                EndCreation();
                return MOUSECREATE_OK;
            }

            InsertPoint(-1, pos - GetWorldPos(), false);

            if (!IsOnlyUpdateOnUnselect())
            {
                UpdateGameArea();
            }
        }
        return MOUSECREATE_CONTINUE;
    }
    return CEntityObject::MouseCreateCallback(view, event, point, flags);
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::EndCreation()
{
    SetClosed(mv_closed);
    if (m_panel)
    {
        m_panel->SetShape(this);
    }
    m_bBeingManuallyCreated = false;
}

void CShapeObject::Display(DisplayContext& dc)
{
    if (mv_displaySoundInfo)
    {
        DisplaySoundInfo(dc);
    }
    else
    {
        DisplayNormal(dc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::DisplayNormal(DisplayContext& dc)
{
    //dc.renderer->EnableDepthTest(false);

    const Matrix34& wtm = GetWorldTM();
    QColor col(Qt::black);

    float fPointSize = 0.5f;
    if (!IsSelected())
    {
        fPointSize *= 0.5f;
    }

    bool bPointSelected = false;
    if (m_selectedPoint >= 0 && m_selectedPoint < m_points.size())
    {
        bPointSelected = true;
    }

    bool bLineWidth = 0;

    if (!m_updateSucceed)
    {
        // Draw Error in update
        dc.SetColor(GetColor());
        dc.DrawTextLabel(wtm.GetTranslation(), 2.0f, "Error!", true);
        QString msg("\n\n");
        msg += GetName();
        msg += " (see log)";
        dc.DrawTextLabel(wtm.GetTranslation(), 1.2f, msg.toUtf8().data(), true);
    }

    if (m_points.size() > 1)
    {
        if (IsSelected())
        {
            col = dc.GetSelectedColor();
            dc.SetColor(col);
        }
        else if (IsHighlighted() && !bPointSelected)
        {
            dc.SetColor(QColor(255, 120, 0));
            dc.SetLineWidth(3);
            bLineWidth = true;
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
        dc.SetAlpha(0.8f);

        int num = m_points.size();
        if (num < GetMinPoints())
        {
            num = 1;
        }
        for (int i = 0; i < num; i++)
        {
            int j = (i < m_points.size() - 1) ? i + 1 : 0;
            if (!mv_closed && j == 0 && i != 0)
            {
                continue;
            }

            Vec3 p0 = wtm.TransformPoint(m_points[i]);
            Vec3 p1 = wtm.TransformPoint(m_points[j]);

            ColorB colMergedTmp = dc.GetColor();
            if (m_mergeIndex == i)
            {
                dc.SetColor(QColor(255, 255, 127));
            }
            else
            {
                if (m_bBeingManuallyCreated)
                {
                    if ((num == 1) || ((num >= 2) && (i == num - 2)))
                    {
                        dc.SetColor(QColor(255, 255, 0));
                    }
                }
            }
            dc.DrawLine(p0, p1);
            dc.SetColor(colMergedTmp);
            //DrawTerrainLine( dc,pos+m_points[i],pos+m_points[j] );

            if (mv_height != 0)
            {
                AABB box;
                box.SetTransformedAABB(GetWorldTM(), m_bbox);
                m_lowestHeight = box.min.z;
                // Draw second capping shape from above.
                float z0 = 0;
                float z1 = 0;
                if (m_bPerVertexHeight)
                {
                    z0 = p0.z + mv_height;
                    z1 = p1.z + mv_height;
                }
                else
                {
                    z0 = m_lowestHeight + mv_height;
                    z1 = z0;
                }
                dc.DrawLine(p0, Vec3(p0.x, p0.y, z0));
                dc.DrawLine(Vec3(p0.x, p0.y, z0), Vec3(p1.x, p1.y, z1));

                if (mv_displayFilled || (gSettings.viewports.bFillSelectedShapes && IsSelected()))
                {
                    dc.CullOff();
                    ColorB c = dc.GetColor();
                    dc.SetAlpha(0.3f);
                    dc.DrawQuad(p0, Vec3(p0.x, p0.y, z0), Vec3(p1.x, p1.y, z1), p1);
                    dc.CullOn();
                    dc.SetColor(c);
                    if (!IsHighlighted())
                    {
                        dc.SetAlpha(0.8f);
                    }
                }
            }
        }

        // Draw selected point.
        if (bPointSelected && m_useAxisHelper)
        {
            // The axis is drawn before the points because the points have higher pick priority and should appear on top of the axis.
            Matrix34    axis;
            axis.SetTranslationMat(wtm.TransformPoint(m_points[m_selectedPoint]));
            m_selectedPointAxis.SetMode(CAxisHelper::MOVE_MODE);
            m_selectedPointAxis.DrawAxis(axis, GetIEditor()->GetGlobalGizmoParameters(), dc);
        }

        // Draw points without depth test to make them editable even behind other objects.
        if (IsSelected())
        {
            dc.DepthTestOff();
        }

        if (IsFrozen())
        {
            col = dc.GetFreezeColor();
        }
        else
        {
            col = GetColor();
        }

        for (int i = 0; i < num; i++)
        {
            Vec3 p0 = wtm.TransformPoint(m_points[i]);
            float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
            Vec3 sz(fScale, fScale, fScale);

            if (bPointSelected && i == m_selectedPoint)
            {
                dc.SetSelectedColor();
            }
            else
            {
                dc.SetColor(col);
            }

            dc.DrawWireBox(p0 - sz, p0 + sz);
        }

        if (IsSelected())
        {
            dc.DepthTestOn();
        }
    }

    if (!m_entities.IsEmpty())
    {
        Vec3 vcol(col.redF(), col.greenF(), col.blueF());
        int num = m_entities.GetCount();
        for (int i = 0; i < num; i++)
        {
            CBaseObject* obj = m_entities.Get(i);
            if (!obj)
            {
                continue;
            }
            int p1, p2;
            float dist;
            Vec3 intPnt;
            GetNearestEdge(obj->GetWorldPos(), p1, p2, dist, intPnt);
            dc.DrawLine(intPnt, obj->GetWorldPos(), ColorF(vcol.x, vcol.y, vcol.z, 0.7f), ColorF(1, 1, 1, 0.7f));
        }
    }

    if (mv_closed && !IsFrozen())
    {
        if (mv_displayFilled || (gSettings.viewports.bFillSelectedShapes && IsSelected()))
        {
            if (IsHighlighted())
            {
                dc.SetColor(GetColor(), 0.1f);
            }
            else
            {
                dc.SetColor(GetColor(), 0.3f);
            }
            static std::vector<Vec3> tris;
            tris.resize(0);
            tris.reserve(m_points.size() * 3);
            if (Triangulator::Triangulate(m_points, tris))
            {
                if (m_bNoCulling)
                {
                    dc.CullOff();
                }
                for (int i = 0; i < tris.size(); i += 3)
                {
                    dc.DrawTri(wtm.TransformPoint(tris[i]), wtm.TransformPoint(tris[i + 1]), wtm.TransformPoint(tris[i + 2]));
                }
                if (m_bNoCulling)
                {
                    dc.CullOn();
                }
            }
        }
    }

    if (m_numSplitPoints > 0)
    {
        QColor col = GetColor();
        dc.SetColor(QColor(127, 255, 127));
        for (int i = 0; i < m_numSplitPoints; i++)
        {
            Vec3 p0 = wtm.TransformPoint(m_splitPoints[i]);
            float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
            Vec3 sz(fScale, fScale, fScale);
            dc.DrawWireBox(p0 - sz, p0 + sz);
        }
        if (m_numSplitPoints == 2)
        {
            Vec3 p0 = wtm.TransformPoint(m_splitPoints[0]);
            Vec3 p1 = wtm.TransformPoint(m_splitPoints[1]);
            dc.DrawLine(p0, p1);
        }
        dc.SetColor(col);
    }

    if (bLineWidth)
    {
        dc.SetLineWidth(0);
    }

    if (mv_renderVoxelGrid)
    {
        AABB tempbox;
        tempbox.SetTransformedAABB(GetWorldTM(), m_bbox);

        // if either m_agentHeight or mv_agentWidth is zero, this causes infinite loop. In order to avoid it, we should make sure that both parameters are more than a critical value.
        const float criticalSize(0.1f);
        if (mv_agentHeight > criticalSize && mv_agentWidth > criticalSize)
        {
            tempbox.min -= 5.0f * Vec3(mv_agentWidth, mv_agentHeight, 0.0f);
            tempbox.max += 5.0f * Vec3(mv_agentWidth, mv_agentHeight, 0.0f);

            float starty = tempbox.min.y + mv_VoxelOffsetY;
            ColorB colVoxel(255, 255, 255);

            while (starty < tempbox.max.y)
            {
                float startx = tempbox.min.x + mv_VoxelOffsetX;
                while (startx < tempbox.max.x)
                {
                    AABB box(
                        Vec3(startx, starty, tempbox.min.z),
                        Vec3(startx + mv_agentWidth, starty + mv_agentHeight, tempbox.min.z + mv_height));
                    gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(box, false, colVoxel, eBBD_Faceted);
                    startx += mv_agentWidth;
                }
                starty += mv_agentHeight;
            }
        }
    }

    DrawDefault(dc, GetColor());
}

void CShapeObject::DisplaySoundInfo(DisplayContext& dc)
{
    //dc.renderer->EnableDepthTest(false);

    const Matrix34& wtm = GetWorldTM();
    QColor col(Qt::black);

    ColorB const oObstructionFilled(255, 0, 0, 120);
    ColorB const oObstructionNotFilled(255, 0, 0, 20);
    ColorB const oNoObstructionFilled(0, 255, 0, 120);
    ColorB const oNoObstructionNotFilled(0, 255, 0, 20);

    float fPointSize = 0.5f;
    if (!IsSelected())
    {
        fPointSize *= 0.5f;
    }

    bool bPointSelected = false;
    if (m_selectedPoint >= 0 && m_selectedPoint < m_points.size())
    {
        bPointSelected = true;
    }

    bool bLineWidth = 0;

    if (!m_updateSucceed)
    {
        // Draw Error in update
        dc.SetColor(GetColor());
        dc.DrawTextLabel(wtm.GetTranslation(), 2.0f, "Error!", true);
        QString msg("\n\n");
        msg += GetName();
        msg += " (see log)";
        dc.DrawTextLabel(wtm.GetTranslation(), 1.2f, msg.toUtf8().data(), true);
    }

    if (m_points.size() > 1)
    {
        if (IsSelected() && mv_height != 0.0f)
        {
            col = dc.GetSelectedColor();
            dc.SetColor(col);
            dc.SetLineWidth(2);
            bLineWidth = true;
        }
        else if (IsHighlighted() && !bPointSelected)
        {
            dc.SetColor(QColor(255, 120, 0));
            dc.SetLineWidth(3);
            bLineWidth = true;
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
        dc.SetAlpha(0.8f);

        int num = m_points.size();
        if (num < GetMinPoints())
        {
            num = 1;
        }
        for (int i = 0; i < num; i++)
        {
            int j = (i < m_points.size() - 1) ? i + 1 : 0;
            if (!mv_closed && j == 0 && i != 0)
            {
                continue;
            }

            Vec3 p0 = wtm.TransformPoint(m_points[i]);
            Vec3 p1 = wtm.TransformPoint(m_points[j]);

            ColorB colMergedTmp = dc.GetColor();
            if (m_mergeIndex == i)
            {
                dc.SetColor(QColor(255, 255, 127));
            }
            else if (mv_height == 0.0f && IsSelected())
            {
                if (m_abObstructSound[i])
                {
                    dc.SetColor(oObstructionFilled);
                }
                else
                {
                    dc.SetColor(oNoObstructionFilled);
                }
            }

            dc.DrawLine(p0, p1);
            dc.SetColor(colMergedTmp);
            //DrawTerrainLine( dc,pos+m_points[i],pos+m_points[j] );

            if (mv_height != 0)
            {
                AABB box;
                box.SetTransformedAABB(GetWorldTM(), m_bbox);
                m_lowestHeight = box.min.z;
                // Draw second capping shape from above.
                float z0 = 0;
                float z1 = 0;
                if (m_bPerVertexHeight)
                {
                    z0 = p0.z + mv_height;
                    z1 = p1.z + mv_height;
                }
                else
                {
                    z0 = m_lowestHeight + mv_height;
                    z1 = z0;
                }
                dc.DrawLine(p0, Vec3(p0.x, p0.y, z0));
                dc.DrawLine(Vec3(p0.x, p0.y, z0), Vec3(p1.x, p1.y, z1));

                // Draw the sides
                if (mv_displayFilled || (gSettings.viewports.bFillSelectedShapes && IsSelected()))
                {
                    dc.CullOff();
                    dc.DepthWriteOff();
                    //dc.pRenderAuxGeom->GetRenderFlags().SetAlphaBlendMode(e_AlphaAdditive);
                    ColorB c = dc.GetColor();

                    // Manipulate color on sound obstruction, draw it a little thicker and redder to give this impression
                    if (m_abObstructSound[i])
                    {
                        dc.SetColor(oObstructionFilled);
                    }
                    else
                    {
                        dc.SetColor(oNoObstructionFilled);
                    }

                    dc.DrawQuad(p0, Vec3(p0.x, p0.y, z0), Vec3(p1.x, p1.y, z1), p1);
                    //dc.pRenderAuxGeom->GetRenderFlags().SetAlphaBlendMode(e_AlphaBlended);
                    dc.DepthWriteOn();
                    dc.CullOn();
                    dc.SetColor(c);
                }
                else if (IsSelected())
                {
                    // If it's selected but not supposed to be rendered filled, give it slightly an obstructing impression
                    dc.CullOff();
                    dc.DepthWriteOff();
                    ColorB c = dc.GetColor();

                    // Manipulate color so it looks a little thicker and redder
                    if (m_abObstructSound[i])
                    {
                        dc.SetColor(oObstructionNotFilled);
                    }
                    else
                    {
                        dc.SetColor(oNoObstructionNotFilled);
                    }

                    dc.DrawQuad(p0, Vec3(p0.x, p0.y, z0), Vec3(p1.x, p1.y, z1), p1);
                    dc.DepthWriteOn();
                    dc.CullOn();
                    dc.SetColor(c);
                }
            }
        }

        // Draw selected point.
        if (bPointSelected && m_useAxisHelper)
        {
            // The axis is drawn before the points because the points have higher pick priority and should appear on top of the axis.
            Matrix34    axis;
            axis.SetTranslationMat(wtm.TransformPoint(m_points[m_selectedPoint]));
            m_selectedPointAxis.SetMode(CAxisHelper::MOVE_MODE);
            m_selectedPointAxis.DrawAxis(axis, GetIEditor()->GetGlobalGizmoParameters(), dc);
        }

        // Draw points without depth test to make them editable even behind other objects.
        if (IsSelected())
        {
            dc.DepthTestOff();
        }

        if (IsFrozen())
        {
            col = dc.GetFreezeColor();
        }
        else
        {
            col = GetColor();
        }

        for (int i = 0; i < num; i++)
        {
            Vec3 p0 = wtm.TransformPoint(m_points[i]);
            float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
            Vec3 sz(fScale, fScale, fScale);

            if (bPointSelected && i == m_selectedPoint)
            {
                dc.SetSelectedColor();
            }
            else
            {
                dc.SetColor(col);
            }

            dc.DrawWireBox(p0 - sz, p0 + sz);
        }

        if (IsSelected())
        {
            dc.DepthTestOn();
        }
    }

    if (!m_entities.IsEmpty())
    {
        Vec3 vcol(col.redF(), col.greenF(), col.blueF());
        int num = m_entities.GetCount();
        for (int i = 0; i < num; i++)
        {
            CBaseObject* obj = m_entities.Get(i);
            if (!obj)
            {
                continue;
            }
            int p1, p2;
            float dist;
            Vec3 intPnt;
            GetNearestEdge(obj->GetWorldPos(), p1, p2, dist, intPnt);
            dc.DrawLine(intPnt, obj->GetWorldPos(), ColorF(vcol.x, vcol.y, vcol.z, 0.7f), ColorF(1, 1, 1, 0.7f));
        }
    }

    if (mv_closed && !IsFrozen() && mv_height > 0.0f)
    {
        //if (mv_displayFilled) // || (IsSelected() && m_bDisplayFilledWhenSelected) || (IsHighlighted() && m_bDisplayFilledWhenSelected))
        if (mv_displayFilled || (gSettings.viewports.bFillSelectedShapes && IsSelected()))
        {
            if (IsHighlighted())
            {
                dc.SetColor(GetColor(), 0.1f);
            }
            else
            {
                dc.SetColor(GetColor(), 0.3f);
            }

            static std::vector<Vec3> tris;
            tris.resize(0);
            tris.reserve(m_points.size() * 3);
            if (Triangulator::Triangulate(m_points, tris))
            {
                // Manipulate color on sound obstruction, draw it a little thicker and redder to give this impression
                //bool bRoofObstructs = false;
                bool bFloorObstructs = false;

                /*if(m_abObstructSound[m_abObstructSound.size()-2])
                bRoofObstructs = true;*/
                if (m_abObstructSound[m_abObstructSound.size() - 1])
                {
                    bFloorObstructs = true;
                }

                ColorB c = dc.GetColor();
                dc.CullOff();

                //float const fAreaTopPos = m_lowestHeight + mv_height;
                for (int i = 0; i < tris.size(); i += 3)
                {
                    if (bFloorObstructs)
                    {
                        dc.SetColor(oObstructionFilled);
                    }
                    else
                    {
                        dc.SetColor(oNoObstructionFilled);
                    }

                    // Draw the floor
                    dc.DrawTri(wtm.TransformPoint(tris[i]), wtm.TransformPoint(tris[i + 1]), wtm.TransformPoint(tris[i + 2]));

                    // Draw a roof only if it obstructs sounds (temporarily disabled because of too many ugly render artifacts)
                    /*if(mv_height != 0 && bRoofObstructs)
                    {
                    dc.SetColor(oObstructionFilled);

                    tris[i]         = wtm.TransformPoint(tris[i]);
                    tris[i+1]       = wtm.TransformPoint(tris[i+1]);
                    tris[i+2]       = wtm.TransformPoint(tris[i+2]);

                    tris[i].z       = fAreaTopPos;
                    tris[i+1].z = fAreaTopPos;
                    tris[i+2].z = fAreaTopPos;

                    dc.DrawTri(tris[i], tris[i+1], tris[i+2]);
                    }*/
                }

                dc.SetColor(c);
                dc.CullOn();
            }
        }
        else if (IsSelected())
        {
            static std::vector<Vec3> tris;
            tris.resize(0);
            tris.reserve(m_points.size() * 3);
            if (Triangulator::Triangulate(m_points, tris))
            {
                // Manipulate color on sound obstruction, draw it a little thicker and redder to give this impression
                //bool bRoofObstructs = false;
                bool bFloorObstructs = false;

                /*if(m_abObstructSound[m_abObstructSound.size()-2])
                bRoofObstructs = true;*/
                if (m_abObstructSound[m_abObstructSound.size() - 1])
                {
                    bFloorObstructs = true;
                }

                dc.CullOff();
                ColorB c = dc.GetColor();
                dc.SetAlpha(0.0f);

                //float const fAreaTopPos = m_lowestHeight + mv_height;
                for (int i = 0; i < tris.size(); i += 3)
                {
                    if (bFloorObstructs)
                    {
                        dc.SetColor(oObstructionNotFilled);
                    }
                    else
                    {
                        dc.SetColor(oNoObstructionNotFilled);
                    }

                    // Draw the floor
                    dc.DrawTri(wtm.TransformPoint(tris[i]), wtm.TransformPoint(tris[i + 1]), wtm.TransformPoint(tris[i + 2]));

                    // Draw a roof only if it obstructs sounds (temporarily disabled because of too many ugly render artifacts)
                    //if(mv_height != 0 && bRoofObstructs)
                    //{
                    //  dc.SetColor(oObstructionNotFilled);

                    //  tris[i]         = wtm.TransformPoint(tris[i]);
                    //  tris[i+1]       = wtm.TransformPoint(tris[i+1]);
                    //  tris[i+2]       = wtm.TransformPoint(tris[i+2]);

                    //  tris[i].z       = fAreaTopPos;
                    //  tris[i+1].z = fAreaTopPos;
                    //  tris[i+2].z = fAreaTopPos;

                    //  // Draw the roof
                    //  dc.DrawTri(tris[i], tris[i+1], tris[i+2]);
                    //}
                }

                dc.SetColor(c);
                dc.CullOn();
            }
        }
    }

    if (m_numSplitPoints > 0)
    {
        QColor col = GetColor();
        dc.SetColor(QColor(127, 255, 127));
        for (int i = 0; i < m_numSplitPoints; i++)
        {
            Vec3 p0 = wtm.TransformPoint(m_splitPoints[i]);
            float fScale = fPointSize * dc.view->GetScreenScaleFactor(p0) * 0.01f;
            Vec3 sz(fScale, fScale, fScale);
            dc.DrawWireBox(p0 - sz, p0 + sz);
        }
        if (m_numSplitPoints == 2)
        {
            Vec3 p0 = wtm.TransformPoint(m_splitPoints[0]);
            Vec3 p1 = wtm.TransformPoint(m_splitPoints[1]);
            dc.DrawLine(p0, p1);
        }
        dc.SetColor(col);
    }

    if (bLineWidth)
    {
        dc.SetLineWidth(0);
    }

    //dc.renderer->EnableDepthTest(true);

    DrawDefault(dc, GetColor());
};
//////////////////////////////////////////////////////////////////////////
void CShapeObject::DrawTerrainLine(DisplayContext& dc, const Vec3& p1, const Vec3& p2)
{
    float len = (p2 - p1).GetLength();
    int steps = len / 2;
    if (steps <= 1)
    {
        dc.DrawLine(p1, p2);
        return;
    }
    Vec3 pos1 = p1;
    Vec3 pos2 = p1;
    for (int i = 0; i < steps - 1; i++)
    {
        pos2 = p1 + (1.0f / i) * (p2 - p1);
        pos2.z = dc.engine->GetTerrainElevation(pos2.x, pos2.y);
        dc.SetColor(i * 2, 0, 0, 1);
        dc.DrawLine(pos1, pos2);
        pos1 = pos2;
    }
    //dc.DrawLine( pos2,p2 );
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::Serialize(CObjectArchive& ar)
{
    m_bIgnoreGameUpdate = true;
    CEntityObject::Serialize(ar);
    m_bIgnoreGameUpdate = false;

    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        m_bAreaModified = true;
        m_bIgnoreGameUpdate = true;
        m_entities.Clear();

        GUID entityId;
        if (xmlNode->getAttr("EntityId", entityId))
        {
            // For Backward compatability.
            //m_entities.push_back(entityId);
            ar.SetResolveCallback(this, entityId, functor(m_entities, &CSafeObjectsArray::Add));
        }

        // Load Entities.
        m_points.clear();
        XmlNodeRef points = xmlNode->findChild("Points");
        if (points)
        {
            for (int i = 0; i < points->getChildCount(); i++)
            {
                XmlNodeRef pnt = points->getChild(i);

                // Get position
                Vec3 pos;
                pnt->getAttr("Pos", pos);
                m_points.push_back(pos);
            }
        }
        XmlNodeRef entities = xmlNode->findChild("Entities");
        if (entities)
        {
            for (int i = 0; i < entities->getChildCount(); i++)
            {
                XmlNodeRef ent = entities->getChild(i);
                GUID entityId;
                if (ent->getAttr("Id", entityId))
                {
                    //m_entities.push_back(id);
                    ar.SetResolveCallback(this, entityId, functor(m_entities, &CSafeObjectsArray::Add));
                }
            }
        }

        // Now serialize the sound variables.
        if (m_pSoundPanelVarBlock)
        {
            // Create the variables
            // First clear the remains out
            m_pSoundPanelVarBlock->DeleteAllVariables();

            size_t const nPointsCount = m_points.size();
            for (size_t i = 0; i < nPointsCount; ++i)
            {
                CVariable<bool>* const pvTemp = new CVariable<bool>;

                stack_string cTemp;
                cTemp.Format("Side%d", i + 1);

                // And add each to the block
                m_pSoundPanelVarBlock->AddVariable(pvTemp, cTemp);

                // If we're at the last point add the last 2 "Roof" and "Floor"
                if (i == nPointsCount - 1)
                {
                    CVariable<bool>* const __restrict pvTempRoof      = new CVariable<bool>;
                    CVariable<bool>* const __restrict pvTempFloor     = new CVariable<bool>;
                    m_pSoundPanelVarBlock->AddVariable(pvTempRoof, "Roof");
                    m_pSoundPanelVarBlock->AddVariable(pvTempFloor, "Floor");
                }
            }

            // Now read in the data
            XmlNodeRef pSoundDataNode = xmlNode->findChild("SoundData");
            if (pSoundDataNode)
            {
                // Make sure we stay backwards compatible and also find the old "Side:" formatting
                // This can go out once every level got saved with the new formatting
                char const* const pcTemp = pSoundDataNode->getAttr("Side:1");
                if (pcTemp && pcTemp[0])
                {
                    // We have the old formatting
                    m_pSoundPanelVarBlock->DeleteAllVariables();

                    for (size_t i = 0; i < nPointsCount; ++i)
                    {
                        CVariable<bool>* const pvTempOld = new CVariable<bool>;

                        stack_string cTempOld;
                        cTempOld.Format("Side:%d", i + 1);

                        // And add each to the block
                        m_pSoundPanelVarBlock->AddVariable(pvTempOld, cTempOld);

                        // If we're at the last point add the last 2 "Roof" and "Floor"
                        if (i == nPointsCount - 1)
                        {
                            CVariable<bool>* const __restrict pvTempRoofOld       = new CVariable<bool>;
                            CVariable<bool>* const __restrict pvTempFloorOld  = new CVariable<bool>;
                            m_pSoundPanelVarBlock->AddVariable(pvTempRoofOld, "Roof");
                            m_pSoundPanelVarBlock->AddVariable(pvTempFloorOld, "Floor");
                        }
                    }
                }

                m_pSoundPanelVarBlock->Serialize(pSoundDataNode, true);
            }

            // Copy the data to the "bools" list
            // First clear the remains out
            m_abObstructSound.clear();

            size_t const nVarCount = m_pSoundPanelVarBlock->GetNumVariables();
            for (size_t i = 0; i < nVarCount; ++i)
            {
                IVariable const* const pTemp = m_pSoundPanelVarBlock->GetVariable(i);
                if (pTemp)
                {
                    bool bTemp = false;
                    pTemp->Get(bTemp);
                    m_abObstructSound.push_back(bTemp);

                    if (i >= nVarCount - 2)
                    {
                        // Here check for backwards compatibility reasons if "ObstructRoof" and "ObstructFloor" still exists
                        bool bTemp = false;
                        if (i == nVarCount - 2)
                        {
                            if (xmlNode->getAttr("ObstructRoof", bTemp))
                            {
                                m_abObstructSound[i] = bTemp;
                            }
                        }
                        else
                        {
                            if (xmlNode->getAttr("ObstructFloor", bTemp))
                            {
                                m_abObstructSound[i] = bTemp;
                            }
                        }
                    }
                }
            }

            // Since the var block is a static object clear it for the next object to be empty
            m_pSoundPanelVarBlock->DeleteAllVariables();
        }

        if (ar.bUndo)
        {
            // Update game area only in undo mode.
            m_bIgnoreGameUpdate = false;
        }

        CalcBBox();
        UpdateGameArea();

        // Update UI.
        if (m_panel && m_panel->GetShape() == this)
        {
            m_panel->SetShape(this);
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
                pnt->setAttr("Pos", m_points[i]);
            }
        }
        // Save Entities.
        if (!m_entities.IsEmpty())
        {
            XmlNodeRef nodes = xmlNode->newChild("Entities");
            for (int i = 0; i < m_entities.GetCount(); i++)
            {
                XmlNodeRef entNode = nodes->newChild("Entity");
                if (m_entities.Get(i))
                {
                    entNode->setAttr("Id", m_entities.Get(i)->GetId());
                }
            }
        }

        if (m_pSoundPanelVarBlock)
        {
            XmlNodeRef pSoundDataNode = xmlNode->newChild("SoundData");

            if (pSoundDataNode)
            {
                // First clear the remains out
                m_pSoundPanelVarBlock->DeleteAllVariables();

                // Create the variables
                size_t const nCount = m_abObstructSound.size();
                for (size_t i = 0; i < nCount; ++i)
                {
                    CVariable<bool>* const pvTemp = new CVariable<bool>;
                    pvTemp->Set(m_abObstructSound[i]);
                    stack_string cTemp;

                    if (i == nCount - 2) // Next to last one == "Roof"
                    {
                        cTemp.Format("Roof");
                    }
                    else if (i == nCount - 1) // Last one == "Floor"
                    {
                        cTemp.Format("Floor");
                    }
                    else
                    {
                        cTemp.Format("Side%d", i + 1);
                    }

                    // And add each to the block
                    m_pSoundPanelVarBlock->AddVariable(pvTemp, cTemp);
                }

                m_pSoundPanelVarBlock->Serialize(pSoundDataNode, false);
                m_pSoundPanelVarBlock->DeleteAllVariables();
            }
        }
    }

    m_bIgnoreGameUpdate = false;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CShapeObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CEntityObject::Export(levelPath, xmlNode);
    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::CalcBBox()
{
    if (m_points.empty())
    {
        return;
    }

    // Reposition shape, so that shape object position is in the middle of the shape.
    Vec3 center = m_points[0];
    if (center.x != 0 || center.y != 0 || center.z != 0)
    {
        Matrix34 ltm = GetLocalTM();

        Vec3 newPos = GetPos() + ltm.TransformVector(center);
        // May not work correctly if shape is transformed.
        for (int i = 0; i < m_points.size(); i++)
        {
            m_points[i] -= center;
        }
        SetPos(GetPos() + ltm.TransformVector(center));
    }

    // First point must always be 0,0,0.
    m_bbox.Reset();
    for (int i = 0; i < m_points.size(); i++)
    {
        m_bbox.Add(m_points[i]);
    }
    if (m_bbox.min.x > m_bbox.max.x)
    {
        m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    }
    if (mv_height)
    {
        AABB box;
        box.SetTransformedAABB(GetWorldTM(), m_bbox);
        m_lowestHeight = box.min.z;

        if (m_bPerVertexHeight)
        {
            m_bbox.max.z += mv_height;
        }
        else
        {
            //m_bbox.max.z = max( m_bbox.max.z,(float)(m_lowestHeight+mv_height) );
            box.max.z = max(box.max.z, (float)(m_lowestHeight + mv_height));
            Matrix34 mat = GetWorldTM().GetInverted();
            Vec3 p = mat.TransformPoint(box.max);
            m_bbox.max.z = max(m_bbox.max.z, p.z);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::SetSelected(bool bSelect)
{
    bool bWasSelected = IsSelected();
    CEntityObject::SetSelected(bSelect);
    if (!bSelect && bWasSelected)
    {
        // When unselecting shape, update area in game.
        if (m_bAreaModified)
        {
            UpdateGameArea();
        }
        m_bAreaModified = false;
    }
    m_mergeIndex = -1;
}

//////////////////////////////////////////////////////////////////////////
int CShapeObject::InsertPoint(int index, const Vec3& point, bool const bModifying)
{
    if (GetPointCount() >= GetMaxPoints())
    {
        return GetPointCount() - 1;
    }

    // Make sure we don't have points too close to each other
    size_t nCount   = m_points.size();

    if (nCount > 1) // At least 1 point should be set already
    {
        // Don't compare to the last entry if index is smaller 0,
        // this indicates that we're creating a new shape and not editing it and just adding points.
        // When creating a new shape the new point is already pushed to the end of the vector.
        nCount -= (bModifying ? 0 : 1);
        for (size_t nIdx = 0; nIdx < nCount; ++nIdx)
        {
            float const fDiffX = fabs_tpl(m_points[nIdx].x - point.x);
            float const fDiffY = fabs_tpl(m_points[nIdx].y - point.y);
            float const fDiffZ = fabs_tpl(m_points[nIdx].z - point.z);

            if (fDiffX < SHAPE_POINT_MIN_DISTANCE && fDiffY < SHAPE_POINT_MIN_DISTANCE && fDiffZ < SHAPE_POINT_MIN_DISTANCE)
            {
                AZ_Warning("CShapeObject::InsertPoint", fDiffX > SHAPE_POINT_MIN_DISTANCE && fDiffY > SHAPE_POINT_MIN_DISTANCE && fDiffZ > SHAPE_POINT_MIN_DISTANCE, "The point is too close to another point!");
                return nIdx;
            }
        }
    }

    StoreUndo("Insert Point");

    int nNewIndex       = -1;
    m_bAreaModified = true;

    if (index < 0 || index >= m_points.size())
    {
        m_points.push_back(point);
        nNewIndex = m_points.size() - 1;

        if (m_abObstructSound.empty())
        {
            // This shape is being created therefore add the point plus roof and floor positions
            m_abObstructSound.resize(3);
        }
        else
        {
            // Update the "bools list", insert just before the "Roof" position
            m_abObstructSound.insert(m_abObstructSound.end() - 2, false);
        }
    }
    else
    {
        m_points.insert(m_points.begin() + index, point);
        nNewIndex = index;

        // Also update the "bools list"
        m_abObstructSound.insert(m_abObstructSound.begin() + index, false);
    }

    // Refresh the properties panel as well to display the new point
    UpdateSoundPanelParams();

    SetPoint(nNewIndex, point);
    CalcBBox();
    UpdatePrefab();

    return nNewIndex;
}

//////////////////////////////////////////////////////////////////////////
int CShapeObject::SetSplitPoint(int index, const Vec3& point, int numPoint)
{
    if (numPoint > 1)
    {
        return -1;
    }
    if (numPoint < 0)
    {
        m_numSplitPoints = 0;
        return -1;
    }

    if (numPoint == 1 && index == m_splitIndicies[0])
    {
        return 1;
    }

    if (index != -2)// if index==-2 need change only m_numSplitPoints
    {
        m_splitIndicies[numPoint] = index;
        m_splitPoints[numPoint] = point;
    }
    m_numSplitPoints = numPoint + 1;
    return m_numSplitPoints;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::Split()
{
    if (m_numSplitPoints != 2)
    {
        return;
    }

    int in0 = InsertPoint(m_splitIndicies[0], m_splitPoints[0], true);
    if (m_splitIndicies[0] != -1 && m_splitIndicies[1] != -1 && m_splitIndicies[1] > m_splitIndicies[0])
    {
        m_splitIndicies[1]++;
    }
    int in1 = InsertPoint(m_splitIndicies[1], m_splitPoints[1], true);

    if (in1 <= in0)
    {
        in0++;
        int vs = in1;
        in1 = in0;
        in0 = vs;
    }

    CShapeObject* pNewShape = (CShapeObject*)GetObjectManager()->CloneObject(this);
    int i;
    for (i = in0 + 1; i < in1; i++)
    {
        RemovePoint(in0 + 1);
    }
    if (pNewShape)
    {
        int cnt = pNewShape->GetPointCount();
        for (i = in1 + 1; i < cnt; i++)
        {
            pNewShape->RemovePoint(in1 + 1);
        }
        for (i = 0; i < in0; i++)
        {
            pNewShape->RemovePoint(0);
        }
    }

    m_bAreaModified = true;
    if (!IsOnlyUpdateOnUnselect())
    {
        UpdateGameArea();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::SetMergeIndex(int index)
{
    m_mergeIndex = index;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::Merge(CShapeObject* shape)
{
    if (!shape || m_mergeIndex < 0 ||  shape->m_mergeIndex < 0)
    {
        return;
    }

    const Matrix34& tm = GetWorldTM();
    const Matrix34& shapeTM = shape->GetWorldTM();

    int index = m_mergeIndex + 1;

    Vec3 p0 = tm.TransformPoint(GetPoint(m_mergeIndex));
    Vec3 p1 = tm.TransformPoint(GetPoint(m_mergeIndex + 1 < GetPointCount() ? m_mergeIndex + 1 : 0));

    Vec3 p2 = shapeTM.TransformPoint(shape->GetPoint(shape->m_mergeIndex));
    Vec3 p3 = shapeTM.TransformPoint(shape->GetPoint(shape->m_mergeIndex + 1 < shape->GetPointCount() ? shape->m_mergeIndex + 1 : 0));

    float sum1 = p0.GetDistance(p2) + p1.GetDistance(p3);
    float sum2 = p0.GetDistance(p3) + p1.GetDistance(p2);

    if (sum2 < sum1)
    {
        shape->ReverseShape();
        shape->m_mergeIndex = shape->GetPointCount() - 1 - shape->m_mergeIndex;
    }

    int i;
    for (i = shape->m_mergeIndex + 1; i < shape->GetPointCount(); i++)
    {
        Vec3 pnt = shapeTM.TransformPoint(shape->GetPoint(i));
        pnt = tm.GetInverted().TransformPoint(pnt);
        InsertPoint(index, pnt, true);
    }
    for (i = 0; i <= shape->m_mergeIndex; i++)
    {
        Vec3 pnt = shapeTM.TransformPoint(shape->GetPoint(i));
        pnt = tm.GetInverted().TransformPoint(pnt);
        InsertPoint(index, pnt, true);
    }

    shape->SetMergeIndex(-1);
    GetObjectManager()->DeleteObject(shape);

    m_bAreaModified = true;
    if (!IsOnlyUpdateOnUnselect())
    {
        UpdateGameArea();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::RemovePoint(int index)
{
    if ((index >= 0 || index < m_points.size()) && m_points.size() > GetMinPoints())
    {
        m_bAreaModified = true;
        StoreUndo("Remove Point");
        m_points.erase(m_points.begin() + index);
        m_abObstructSound.erase(m_abObstructSound.begin() + index);
        CalcBBox();

        m_bAreaModified = true;
        if (!IsOnlyUpdateOnUnselect())
        {
            UpdateGameArea();
        }
        UpdatePrefab();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::ClearPoints()
{
    m_bAreaModified = true;
    StoreUndo("Clear Points");
    m_points.clear();
    m_abObstructSound.clear();
    CalcBBox();
    m_bAreaModified = true;
    if (!IsOnlyUpdateOnUnselect())
    {
        UpdateGameArea();
    }
    UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    CEntityObject::PostClone(pFromObject, ctx);

    CShapeObject* pFromShape = (CShapeObject*)pFromObject;
    // Clone event targets.
    if (!pFromShape->m_entities.IsEmpty())
    {
        int numEntities = pFromShape->m_entities.GetCount();
        for (int i = 0; i < numEntities; i++)
        {
            CBaseObject* pTarget = pFromShape->m_entities.Get(i);
            CBaseObject* pClonedTarget = ctx.FindClone(pTarget);
            if (!pClonedTarget)
            {
                pClonedTarget = pTarget; // If target not cloned, link to original target.
            }
            // Add cloned event.
            if (pClonedTarget)
            {
                AddEntity(pClonedTarget);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::AddEntity(CBaseObject* entity)
{
    assert(entity);

    m_bAreaModified = true;

    StoreUndo("Add Entity");
    m_entities.Add(entity);
    UpdateGameArea();
    UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::RemoveEntity(int index)
{
    assert(index >= 0 && index < m_entities.GetCount());
    StoreUndo("Remove Entity");

    m_bAreaModified = true;

    if (index < m_entities.GetCount())
    {
        CBaseObject* pObject = m_entities.Get(index);

        if (qobject_cast<CAITerritoryObject*>(this) && qobject_cast<CAIWaveObject*>(pObject))
        {
            GetObjectManager()->FindAndRenameProperty2If("aiwave_Wave", pObject->GetName(), "<None>", "aiterritory_Territory", this->GetName());
        }

        m_entities.Remove(pObject);
    }
    UpdateGameArea();
    UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CShapeObject::GetEntity(int index)
{
    assert(index >= 0 && index < m_entities.GetCount());
    return m_entities.Get(index);
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::SetClosed(bool bClosed)
{
    StoreUndo("Set Closed");
    mv_closed = bClosed;

    m_bAreaModified = true;

    if (mv_closed)
    {
        UpdateGameArea();
    }

    UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
int CShapeObject::GetAreaId()
{
    return mv_areaId;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::SelectPoint(int index)
{
    m_selectedPoint = index;
}

void CShapeObject::SetPoint(int index, const Vec3& pos)
{
    Vec3 p = pos;
    if (m_bForce2D)
    {
        if (!m_points.empty())
        {
            p.z = m_points[0].z; // Keep original Z.
        }
    }

    size_t const nCount = m_points.size();

    if (index >= 0 && index < nCount)
    {
        for (size_t nIdx = 0; nIdx < nCount; ++nIdx)
        {
            if (nIdx != index) // Don't check against the same point
            {
                float const fDiffX = fabs_tpl(m_points[nIdx].x - p.x);
                float const fDiffY = fabs_tpl(m_points[nIdx].y - p.y);
                float const fDiffZ = fabs_tpl(m_points[nIdx].z - p.z);

                if (fDiffX < SHAPE_POINT_MIN_DISTANCE && fDiffY < SHAPE_POINT_MIN_DISTANCE && fDiffZ < SHAPE_POINT_MIN_DISTANCE)
                {
                    //CRY_ASSERT_MESSAGE(fDiffX > SHAPE_POINT_MIN_DISTANCE && fDiffY > SHAPE_POINT_MIN_DISTANCE && fDiffZ > SHAPE_POINT_MIN_DISTANCE, "The point is too close to another point!");
                    return;
                }
            }
        }

        m_points[index] = p;
    }

    m_bAreaModified = true;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::ReverseShape()
{
    StoreUndo("Reverse Shape");
    std::reverse(m_points.begin(), m_points.end());
    if (mv_closed)
    {
        // Keep the same starting point for closed paths.
        Vec3    tmp(m_points.back());
        for (size_t i = m_points.size() - 1; i > 0; --i)
        {
            m_points[i] = m_points[i - 1];
        }
        m_points[0] = tmp;
    }
    m_bAreaModified = true;
    if (!IsOnlyUpdateOnUnselect())
    {
        UpdateGameArea();
    }
    UpdatePrefab();
}


//////////////////////////////////////////////////////////////////////////
void CShapeObject::ResetShape()
{
    StoreUndo("Reset Height Shape");

    for (size_t i = 0, count = m_points.size(); i < count; i++)
    {
        m_points[i].z = 0;
    }

    m_bAreaModified = true;
    if (!IsOnlyUpdateOnUnselect())
    {
        UpdateGameArea();
    }
    UpdatePrefab();
}


//////////////////////////////////////////////////////////////////////////
int CShapeObject::GetNearestPoint(const Vec3& raySrc, const Vec3& rayDir, float& distance)
{
    int index = -1;
    float minDist = FLT_MAX;
    Vec3 rayLineP1 = raySrc;
    Vec3 rayLineP2 = raySrc + rayDir * RAY_DISTANCE;
    Vec3 intPnt;
    const Matrix34& wtm = GetWorldTM();
    for (int i = 0; i < m_points.size(); i++)
    {
        float d = PointToLineDistance(rayLineP1, rayLineP2, wtm.TransformPoint(m_points[i]), intPnt);
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
void CShapeObject::GetNearestEdge(const Vec3& pos, int& p1, int& p2, float& distance, Vec3& intersectPoint)
{
    p1 = -1;
    p2 = -1;
    float minDist = FLT_MAX;
    Vec3 intPnt;

    const Matrix34& wtm = GetWorldTM();

    for (int i = 0; i < m_points.size(); i++)
    {
        int j = (i < m_points.size() - 1) ? i + 1 : 0;

        if (!mv_closed && j == 0 && i != 0)
        {
            continue;
        }

        float d = PointToLineDistance(wtm.TransformPoint(m_points[i]), wtm.TransformPoint(m_points[j]), pos, intPnt);
        if (d < minDist)
        {
            minDist = d;
            p1 = i;
            p2 = j;
            intersectPoint = intPnt;
        }
    }
    distance = minDist;
}

//////////////////////////////////////////////////////////////////////////
bool CShapeObject::RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt)
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
void CShapeObject::GetNearestEdge(const Vec3& raySrc, const Vec3& rayDir, int& p1, int& p2, float& distance, Vec3& intersectPoint)
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

        if (!mv_closed && j == 0 && i != 0)
        {
            continue;
        }

        Vec3 pi = wtm.TransformPoint(m_points[i]);
        Vec3 pj = wtm.TransformPoint(m_points[j]);

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
    distance = minDist;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::UpdateGameArea(bool bRemove)
{
    if (bRemove && m_pSoundPropertiesPanel)
    {
        if (m_nSoundPanelID != 0)
        {
            // Internally a var block holds "IVariablePtr", on destroy delete is already called
            m_pSoundPanelVarBlock->DeleteAllVariables();

            GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
            m_nSoundPanelID = 0;
        }

        m_pSoundPropertiesPanel = NULL;
        return;
    }

    if (m_bIgnoreGameUpdate)
    {
        return;
    }

    if (!m_pEntity)
    {
        return;
    }

    if (GetPointCount() < 2)
    {
        return;
    }

    if (!mv_closed)
    {
        return;
    }

    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
    if (!pArea)
    {
        return;
    }

    unsigned int const nPointsCount = GetPointCount();
    if (nPointsCount > 0)
    {
        std::vector<Vec3> points(nPointsCount);
        bool* const pbObstructSound = new bool[nPointsCount];

        for (unsigned int i = 0; i < nPointsCount; ++i)
        {
            points[i] = GetPoint(i);

            // Here we "unpack" the data! (1 bit*nPointsCount to 1 byte*nPointsCount)
            pbObstructSound[i] = m_abObstructSound[i];
        }

        UpdateSoundPanelParams();
        pArea->SetPoints(&points[0], &pbObstructSound[0], points.size(), GetHeight());
        delete[] pbObstructSound;
    }

    pArea->SetProximity(GetWidth());
    pArea->SetID(mv_areaId);
    pArea->SetGroup(mv_groupId);
    pArea->SetPriority(mv_priority);
    pArea->SetSoundObstructionOnAreaFace(nPointsCount, m_abObstructSound[nPointsCount]);        // Roof
    pArea->SetSoundObstructionOnAreaFace(nPointsCount + 1, m_abObstructSound[nPointsCount + 1]); // Floor

    pArea->ClearEntities();
    for (int i = 0; i < GetEntityCount(); i++)
    {
        CEntityObject* pEntity = (CEntityObject*)GetEntity(i);
        if (pEntity && pEntity->GetIEntity())
        {
            pArea->AddEntity(pEntity->GetIEntity()->GetId());
        }
    }

    m_bAreaModified = false;
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::OnShapeChange(IVariable* var)
{
    m_bAreaModified = true;
    if (!m_bIgnoreGameUpdate)
    {
        UpdateGameArea();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::OnSoundParamsChange(IVariable* var)
{
    if (!m_bIgnoreGameUpdate)
    {
        if (!mv_displaySoundInfo)
        {
            if (m_nSoundPanelID != 0)
            {
                // Internally a var block holds "IVariablePtr", on destroy delete is already called
                m_pSoundPanelVarBlock->DeleteAllVariables();

                GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
                m_nSoundPanelID                 = 0;
                m_pSoundPropertiesPanel = 0;
            }
        }
        else
        {
            if (!m_pSoundPropertiesPanel)
            {
                m_pSoundPropertiesPanel = new ReflectedPropertiesPanel;
                m_pSoundPropertiesPanel->Setup();
            }
        }

        UpdateSoundPanelParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::OnPointChange(IVariable* var)
{
    if (m_pSoundPanelVarBlock)
    {
        size_t const nVarCount = m_pSoundPanelVarBlock->GetNumVariables();
        for (size_t i = 0; i < nVarCount; ++i)
        {
            if (m_pSoundPanelVarBlock->GetVariable(i) == var)
            {
                if (m_pEntity)
                {
                    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
                    if (pArea)
                    {
                        bool bObstructs = false;
                        if (!QString::compare(var->GetName(), "Roof", Qt::CaseInsensitive))
                        {
                            IVariable const* const __restrict pTempRoof = m_pSoundPanelVarBlock->FindVariable("Roof");
                            if (pTempRoof)
                            {
                                pTempRoof->Get(bObstructs);
                                pArea->SetSoundObstructionOnAreaFace(nVarCount - 2, bObstructs);
                                m_abObstructSound[nVarCount - 2] = bObstructs;
                            }
                        }
                        else if (!QString::compare(var->GetName(), "Floor", Qt::CaseInsensitive))
                        {
                            IVariable const* const __restrict   pTempFloor = m_pSoundPanelVarBlock->FindVariable("Floor");
                            if (pTempFloor)
                            {
                                pTempFloor->Get(bObstructs);
                                pArea->SetSoundObstructionOnAreaFace(nVarCount - 1, bObstructs);
                                m_abObstructSound[nVarCount - 1] = bObstructs;
                            }
                        }
                        else
                        {
                            var->Get(bObstructs);
                            pArea->SetSoundObstructionOnAreaFace(i, bObstructs);
                            m_abObstructSound[i] = bObstructs;
                        }
                    }
                }
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::UpdateSoundPanelParams()
{
    if (!m_rollupMultyId && mv_displaySoundInfo && m_pSoundPropertiesPanel)
    {
        if (!m_nSoundPanelID)
        {
            m_nSoundPanelID = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Sound Obstruction", m_pSoundPropertiesPanel);
        }

        m_pSoundPropertiesPanel->DeleteVars();

        if (m_nSoundPanelID)
        {
            if (m_pSoundPanelVarBlock)
            {
                if (!m_pSoundPanelVarBlock->IsEmpty())
                {
                    m_pSoundPanelVarBlock->DeleteAllVariables();
                }

                if (!m_abObstructSound.empty())
                {
                    // If the shape hasn't got a height subtract 2 for roof and floor
                    size_t const nVarCount = m_abObstructSound.size() - ((mv_height > 0.0f) ? 0 : 2);
                    for (size_t i = 0; i < nVarCount; ++i)
                    {
                        CVariable<bool>* const pvTemp = new CVariable<bool>;
                        pvTemp->Set(m_abObstructSound[i]);
                        pvTemp->AddOnSetCallback(functor(*this, &CShapeObject::OnPointChange));

                        stack_string cTemp;
                        if (i == nVarCount - 2 && mv_height > 0.0f)
                        {
                            cTemp.Format("Roof");
                        }
                        else if (i == nVarCount - 1 && mv_height > 0.0f)
                        {
                            cTemp.Format("Floor");
                        }
                        else
                        {
                            cTemp.Format("Side:%d", i + 1);
                        }

                        m_pSoundPanelVarBlock->AddVariable(pvTemp, cTemp);
                    }
                }

                m_pSoundPropertiesPanel->AddVars(m_pSoundPanelVarBlock);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::SpawnEntity()
{
    if (!m_entityClass.isEmpty())
    {
        CEntityObject::SpawnEntity();
        UpdateGameArea();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::OnEvent(ObjectEvent event)
{
    switch (event)
    {
    case EVENT_ALIGN_TOGRID:
    {
        AlignToGrid();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::PostLoad(CObjectArchive& ar)
{
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
bool CShapeObject::HitTestRect(HitContext& hc)
{
    AABB box;
    // Retrieve world space bound box.
    GetBoundBox(box);

    // Project all edges to viewport.
    const Matrix34& wtm = GetWorldTM();
    for (int i = 0; i < m_points.size(); i++)
    {
        int j = (i < m_points.size() - 1) ? i + 1 : 0;
        if (!mv_closed && j == 0 && i != 0)
        {
            continue;
        }

        Vec3 p0 = wtm.TransformPoint(m_points[i]);
        Vec3 p1 = wtm.TransformPoint(m_points[j]);

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

    /*

        // transform all 8 vertices into world space
        CPoint p[8] =
        {
            hc.view->WorldToView(Vec3d(box.min.x,box.min.y,box.min.z)),
                hc.view->WorldToView(Vec3d(box.min.x,box.max.y,box.min.z)),
                hc.view->WorldToView(Vec3d(box.max.x,box.min.y,box.min.z)),
                hc.view->WorldToView(Vec3d(box.max.x,box.max.y,box.min.z)),
                hc.view->WorldToView(Vec3d(box.min.x,box.min.y,box.max.z)),
                hc.view->WorldToView(Vec3d(box.min.x,box.max.y,box.max.z)),
                hc.view->WorldToView(Vec3d(box.max.x,box.min.y,box.max.z)),
                hc.view->WorldToView(Vec3d(box.max.x,box.max.y,box.max.z))
        };

        CRect objrc,temprc;

        objrc.left = 10000;
        objrc.top = 10000;
        objrc.right = -10000;
        objrc.bottom = -10000;
        // find new min/max values
        for(int i=0; i<8; i++)
        {
            objrc.left = min(objrc.left,p[i].x);
            objrc.right = max(objrc.right,p[i].x);
            objrc.top = min(objrc.top,p[i].y);
            objrc.bottom = max(objrc.bottom,p[i].y);
        }
        if (objrc.IsRectEmpty())
        {
            // Make objrc at least of size 1.
            objrc.bottom += 1;
            objrc.right += 1;
        }
        if (temprc.IntersectRect( objrc,hc.rect ))
        {
            hc.object = this;
            return true;
        }
        return false;
        */
}

//////////////////////////////////////////////////////////////////////////
void CShapeObject::AlignToGrid()
{
    CViewport* view = GetIEditor()->GetActiveView();
    if (!view)
    {
        return;
    }

    CUndo undo("Align Shape To Grid");
    StoreUndo("Align Shape To Grid");

    Matrix34 wtm = GetWorldTM();
    Matrix34 invTM = wtm.GetInverted();
    for (int i = 0; i < GetPointCount(); i++)
    {
        Vec3 pnt = wtm.TransformPoint(GetPoint(i));
        pnt = view->SnapToGrid(pnt);
        SetPoint(i, invTM.TransformPoint(pnt));
    }
    UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
// CAIPathObject.
//////////////////////////////////////////////////////////////////////////
CAIPathObject::CAIPathObject()
{
    SetColor(QColor(180, 180, 180));
    SetClosed(false);
    m_bRoad = false;
    m_bValidatePath = false;

    m_navType.AddEnumItem("Unset", (int)IAISystem::NAV_UNSET);
    m_navType.AddEnumItem("Triangular", (int)IAISystem::NAV_TRIANGULAR);
    m_navType.AddEnumItem("Waypoint Human", (int)IAISystem::NAV_WAYPOINT_HUMAN);
    m_navType.AddEnumItem("Waypoint 3D Surface", (int)IAISystem::NAV_WAYPOINT_3DSURFACE);
    m_navType.AddEnumItem("Flight", (int)IAISystem::NAV_FLIGHT);
    m_navType.AddEnumItem("Volume", (int)IAISystem::NAV_VOLUME);
    m_navType.AddEnumItem("Road", (int)IAISystem::NAV_ROAD);
    m_navType.AddEnumItem("SmartObject", (int)IAISystem::NAV_SMARTOBJECT);
    m_navType.AddEnumItem("Free 2D", (int)IAISystem::NAV_FREE_2D);

    m_navType = (int)IAISystem::NAV_UNSET;
    m_anchorType = "";

    AddVariable(m_bRoad, "Road", functor(*this, &CAIPathObject::OnShapeChange));
    AddVariable(m_navType, "PathNavType", functor(*this, &CAIPathObject::OnShapeChange));
    AddVariable(m_anchorType, "AnchorType", functor(*this, &CAIPathObject::OnShapeChange), IVariable::DT_AI_ANCHOR);
    AddVariable(m_bValidatePath, "ValidatePath");  //,functor(*this,&CAIPathObject::OnShapeChange) );

    m_bDisplayFilledWhenSelected = false;
}

void CAIPathObject::UpdateGameArea(bool bRemove)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }
    if (!IsCreateGameObjects())
    {
        return;
    }

    IAISystem* ai = GetIEditor()->GetSystem()->GetAISystem();
    if (ai)
    {
        if (m_updateSucceed && !m_lastGameArea.isEmpty())
        {
            GetNavigation()->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
            ai->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
        }
        if (bRemove)
        {
            return;
        }

        if (GetNavigation()->DoesNavigationShapeExists(GetName().toUtf8().data(), AREATYPE_PATH, m_bRoad))
        {
            gEnv->pSystem->GetILog()->LogError("AI Path: Path '%s' already exists in AIsystem, please rename the path.", GetName());
            m_updateSucceed = false;
            return;
        }

        std::vector<Vec3> worldPts;
        const Matrix34& wtm = GetWorldTM();
        for (int i = 0; i < GetPointCount(); i++)
        {
            worldPts.push_back(wtm.TransformPoint(GetPoint(i)));
        }

        m_lastGameArea = GetName();
        if (!worldPts.empty())
        {
            CAIManager* pAIMgr = GetIEditor()->GetAI();
            QString anchorType(m_anchorType);
            int type = pAIMgr->AnchorActionToId(anchorType.toUtf8().data());
            QByteArray lastGameArea = m_lastGameArea.toUtf8();
            SNavigationShapeParams params(lastGameArea, AREATYPE_PATH, m_bRoad, mv_closed, &worldPts[0], worldPts.size(), 0, m_navType, type);
            m_updateSucceed = GetNavigation()->CreateNavigationShape(params);
            ai->CreateNavigationShape(params);
        }
    }
    m_bAreaModified = false;
}

void CAIPathObject::DrawSphere(DisplayContext& dc, const Vec3& p0, float r, int n)
{
    // Note: The Aux Render already supports drawing spheres, but they appear
    // shaded when rendered and there is no cylinder rendering available.
    // This function is here just for the purpose to make the rendering of
    // the validatedsegments more consistent.
    Vec3    a0, a1, b0, b1;
    for (int j = 0; j < n / 2; j++)
    {
        float theta1 = j * gf_PI2 / n - gf_PI / 2;
        float theta2 = (j + 1) * gf_PI2 / n - gf_PI / 2;

        for (int i = 0; i <= n; i++)
        {
            float theta3 = i * gf_PI2 / n;

            a0.x = p0.x + r * cosf(theta2) * cosf(theta3);
            a0.y = p0.y + r * sinf(theta2);
            a0.z = p0.z + r * cosf(theta2) * sinf(theta3);

            b0.x = p0.x + r * cosf(theta1) * cosf(theta3);
            b0.y = p0.y + r * sinf(theta1);
            b0.z = p0.z + r * cosf(theta1) * sinf(theta3);

            if (i > 0)
            {
                dc.DrawQuad(a0, b0, b1, a1);
            }

            a1 = a0;
            b1 = b0;
        }
    }
}

void CAIPathObject::DrawCylinder(DisplayContext& dc, const Vec3& p0, const Vec3& p1, float rad, int n)
{
    Vec3    dir(p1 - p0);
    Vec3    a = dir.GetOrthogonal();
    Vec3    b = dir.Cross(a);
    a.NormalizeSafe();
    b.NormalizeSafe();

    for (int i = 0; i < n; i++)
    {
        float   a0 = ((float)i / (float)n) * gf_PI2;
        float   a1 = ((float)(i + 1) / (float)n) * gf_PI2;
        Vec3    n0 = sinf(a0) * rad * a + cosf(a0) * rad * b;
        Vec3    n1 = sinf(a1) * rad * a + cosf(a1) * rad * b;

        dc.DrawQuad(p0 + n0, p1 + n0, p1 + n1, p0 + n1);
    }
}

bool CAIPathObject::IsSegmentValid(const Vec3& p0, const Vec3& p1, float rad)
{
    AABB    box;
    box.Reset();
    box.Add(p0);
    box.Add(p1);
    box.min -= Vec3(rad, rad, rad);
    box.max += Vec3(rad, rad, rad);

    IPhysicalEntity** entities;
    unsigned nEntities = gEnv->pPhysicalWorld->GetEntitiesInBox(box.min, box.max, entities, ent_static | ent_ignore_noncolliding);

    primitives::sphere spherePrim;
    spherePrim.center = p0;
    spherePrim.r = rad;
    Vec3    dir(p1 - p0);

    unsigned hitCount = 0;
    ray_hit hit;
    IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;

    for (unsigned iEntity = 0; iEntity < nEntities; ++iEntity)
    {
        IPhysicalEntity* pEntity = entities[iEntity];
        if (pPhysics->CollideEntityWithPrimitive(pEntity, spherePrim.type, &spherePrim, dir, &hit))
        {
            return false;
            break;
        }
    }
    return true;
}

void CAIPathObject::Display(DisplayContext& dc)
{
    // Display path validation
    if (m_bValidatePath && IsSelected())
    {
        dc.DepthWriteOff();
        int num = m_points.size();
        if (!m_bRoad && m_navType == (int)IAISystem::NAV_VOLUME && num >= 3)
        {
            const float rad = mv_width;
            const Matrix34& wtm = GetWorldTM();

            if (!mv_closed)
            {
                Vec3 p0 = wtm.TransformPoint(m_points.front());
                float   viewScale = dc.view->GetScreenScaleFactor(p0);
                if (viewScale < 0.01f)
                {
                    viewScale = 0.01f;
                }
                unsigned n = CLAMP((unsigned)(300.0f / viewScale), 4, 20);
                DrawSphere(dc, p0, rad, n);
            }

            for (int i = 0; i < num; i++)
            {
                int j = (i < m_points.size() - 1) ? i + 1 : 0;
                if (!mv_closed && j == 0 && i != 0)
                {
                    continue;
                }

                Vec3 p0 = wtm.TransformPoint(m_points[i]);
                Vec3 p1 = wtm.TransformPoint(m_points[j]);

                float   viewScale = max(dc.view->GetScreenScaleFactor(p0), dc.view->GetScreenScaleFactor(p1));
                if (viewScale < 0.01f)
                {
                    viewScale = 0.01f;
                }
                unsigned n = CLAMP((unsigned)(300.0f / viewScale), 4, 20);

                if (IsSegmentValid(p0, p1, rad))
                {
                    dc.SetColor(1, 1, 1, 0.25f);
                }
                else
                {
                    dc.SetColor(1, 0.5f, 0, 0.25f);
                }

                DrawCylinder(dc, p0, p1, rad, n);
                DrawSphere(dc, p1, rad, n);
            }
        }
        dc.DepthWriteOn();
    }

    // Display the direction of the path
    if (m_points.size() > 1)
    {
        const Matrix34& wtm = GetWorldTM();
        Vec3    p0 = wtm.TransformPoint(m_points[0]);
        Vec3    p1 = wtm.TransformPoint(m_points[1]);
        if (IsSelected())
        {
            dc.SetColor(dc.GetSelectedColor());
        }
        else if (IsHighlighted())
        {
            dc.SetColor(QColor(255, 120, 0));
        }
        else
        {
            dc.SetColor(GetColor());
        }
        Vec3    pt((p1 - p0) / 2);
        if (pt.GetLength() > 1.0f)
        {
            pt.SetLength(1.0f);
        }
        dc.DrawArrow(p0, p0 + pt);

        float l = 0;
        for (int i = 1; i < m_points.size(); ++i)
        {
            l += (m_points[i - 1] - m_points[i]).len();
        }

        QString msg("");
        msg = tr(" Length: %1m").arg(l, 0, 'f', 2);
        dc.DrawTextLabel(wtm.GetTranslation(), 1.2f, msg.toUtf8().data(), true);
    }

    CShapeObject::Display(dc);
}

int CAIPathObject::InsertPoint(int index, const Vec3& point, bool const bModifying)
{
    int result = CAIShapeObjectBase::InsertPoint(index, point, bModifying);

    UpdateGameArea(false);

    return result;
}

void CAIPathObject::SetPoint(int index, const Vec3& pos)
{
    CAIShapeObjectBase::SetPoint(index, pos);

    UpdateGameArea(false);
}


void CAIPathObject::RemovePoint(int index)
{
    CAIShapeObjectBase::RemovePoint(index);

    UpdateGameArea(false);
}

//////////////////////////////////////////////////////////////////////////
// CAIShapeObject.
//////////////////////////////////////////////////////////////////////////
CAIShapeObject::CAIShapeObject()
{
    SetColor(QColor(30, 65, 120));
    SetClosed(true);

    m_anchorType = "";
    AddVariable(m_anchorType, "AnchorType", functor(*this, &CAIShapeObject::OnShapeChange), IVariable::DT_AI_ANCHOR);

    m_lightLevel.AddEnumItem("Default", AILL_NONE);
    m_lightLevel.AddEnumItem("Light", AILL_LIGHT);
    m_lightLevel.AddEnumItem("Medium", AILL_MEDIUM);
    m_lightLevel.AddEnumItem("Dark", AILL_DARK);
    m_lightLevel.AddEnumItem("SuperDark", AILL_SUPERDARK);
    m_lightLevel = AILL_NONE;
    AddVariable(m_lightLevel, "LightLevel", functor(*this, &CAIShapeObject::OnShapeChange));

    m_bDisplayFilledWhenSelected = false;
}

void CAIShapeObject::UpdateGameArea(bool bRemove)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }
    if (!IsCreateGameObjects())
    {
        return;
    }

    IAISystem* ai = GetIEditor()->GetSystem()->GetAISystem();
    if (ai)
    {
        if (m_updateSucceed && !m_lastGameArea.isEmpty())
        {
            GetNavigation()->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
        }
        if (bRemove)
        {
            return;
        }

        if (GetNavigation()->DoesNavigationShapeExists(GetName().toUtf8().data(), AREATYPE_GENERIC))
        {
            gEnv->pSystem->GetILog()->LogError("AI Shape: Shape '%s' already exists in AIsystem, please rename the shape.", GetName());
            m_updateSucceed = false;
            return;
        }

        std::vector<Vec3> worldPts;
        const Matrix34& wtm = GetWorldTM();
        for (int i = 0; i < GetPointCount(); i++)
        {
            worldPts.push_back(wtm.TransformPoint(GetPoint(i)));
        }

        m_lastGameArea = GetName();
        if (!worldPts.empty())
        {
            CAIManager* pAIMgr = GetIEditor()->GetAI();
            QString anchorType(m_anchorType);
            int type = pAIMgr->AnchorActionToId(anchorType.toUtf8().data());
            SNavigationShapeParams params(m_lastGameArea.toUtf8().data(), AREATYPE_GENERIC, false, mv_closed, &worldPts[0], worldPts.size(),
                GetHeight(), 0, type, (EAILightLevel)(int)m_lightLevel);
            m_updateSucceed = GetNavigation()->CreateNavigationShape(params);
        }
    }
    m_bAreaModified = false;
}


//////////////////////////////////////////////////////////////////////////
// CAIOclussionPlaneObject
//////////////////////////////////////////////////////////////////////////
CAIOcclusionPlaneObject::CAIOcclusionPlaneObject()
{
    m_bDisplayFilledWhenSelected = true;
    SetColor(QColor(24, 90, 231));
    m_bForce2D = true;
    mv_closed = true;
    mv_displayFilled = true;
}

void CAIOcclusionPlaneObject::UpdateGameArea(bool bRemove)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }
    if (!IsCreateGameObjects())
    {
        return;
    }

    IAISystem* ai = GetIEditor()->GetSystem()->GetAISystem();
    if (ai)
    {
        if (m_updateSucceed && !m_lastGameArea.isEmpty())
        {
            GetNavigation()->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
            ai->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
        }
        if (bRemove)
        {
            return;
        }

        if (GetNavigation()->DoesNavigationShapeExists(GetName().toUtf8().data(), AREATYPE_OCCLUSION_PLANE))
        {
            gEnv->pSystem->GetILog()->LogError("OcclusionPlane: Shape '%s' already exists in AIsystem, please rename the shape.", GetName());
            m_updateSucceed = false;
            return;
        }

        std::vector<Vec3> worldPts;
        const Matrix34& wtm = GetWorldTM();
        for (int i = 0; i < GetPointCount(); i++)
        {
            worldPts.push_back(wtm.TransformPoint(GetPoint(i)));
        }

        m_lastGameArea = GetName();
        if (!worldPts.empty())
        {
            SNavigationShapeParams params(m_lastGameArea.toUtf8().data(), AREATYPE_OCCLUSION_PLANE, false, mv_closed, &worldPts[0], worldPts.size(), GetHeight());
            m_updateSucceed = GetNavigation()->CreateNavigationShape(params);
            ai->CreateNavigationShape(params);
        }
    }
    m_bAreaModified = false;
}


//////////////////////////////////////////////////////////////////////////
// CAIPerceptionModifier
//////////////////////////////////////////////////////////////////////////
CAIPerceptionModifierObject::CAIPerceptionModifierObject()
{
    m_bDisplayFilledWhenSelected = true;
    SetColor(QColor(24, 90, 231));
    m_bForce2D = false;
    mv_closed = false;
    mv_displayFilled = true;
    m_bPerVertexHeight = false;
    m_fReductionPerMetre = 0.0f;
    m_fReductionMax = 1.0f;
    //AddVariable( m_fReductionPerMetre, "ReductionPerMetre", NULL, IVariable::DT_PERCENT );
    //AddVariable( m_fReductionMax, "ReductionMax", NULL, IVariable::DT_PERCENT );
}

void CAIPerceptionModifierObject::InitVariables()
{
    AddVariable(m_fReductionPerMetre, "ReductionPerMetre", functor(*this, &CAIPerceptionModifierObject::OnShapeChange), IVariable::DT_PERCENT);
    AddVariable(m_fReductionMax, "ReductionMax", functor(*this, &CAIPerceptionModifierObject::OnShapeChange), IVariable::DT_PERCENT);
    AddVariable(mv_height, "Height", functor(*this, &CAIPerceptionModifierObject::OnShapeChange));
    AddVariable(mv_closed, "Closed", functor(*this, &CAIPerceptionModifierObject::OnShapeChange));
    AddVariable(mv_displayFilled, "DisplayFilled");
    //CAIShapeObjectBase::InitVariables();
}

void CAIPerceptionModifierObject::UpdateGameArea(bool bRemove)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }
    if (!IsCreateGameObjects())
    {
        return;
    }

    IAISystem* pAI = GetIEditor()->GetSystem()->GetAISystem();
    if (pAI)
    {
        if (!m_lastGameArea.isEmpty())
        {
            GetNavigation()->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
        }
        if (bRemove)
        {
            return;
        }

        std::vector<Vec3> worldPts;
        const Matrix34& wtm = GetWorldTM();
        for (int i = 0; i < GetPointCount(); i++)
        {
            worldPts.push_back(wtm.TransformPoint(GetPoint(i)));
        }

        m_lastGameArea = GetName();
        if (!worldPts.empty())
        {
            SNavigationShapeParams params(m_lastGameArea.toUtf8().data(), AREATYPE_PERCEPTION_MODIFIER, false, mv_closed, &worldPts[0], worldPts.size(), GetHeight());
            params.fReductionPerMetre = m_fReductionPerMetre;
            params.fReductionMax = m_fReductionMax;
            GetNavigation()->CreateNavigationShape(params);
        }
    }
    m_bAreaModified = false;
}



//////////////////////////////////////////////////////////////////////////
// CTerritory
//////////////////////////////////////////////////////////////////////////

CAITerritoryObject::CAITerritoryObject()
{
    m_entityClass = "AITerritory";

    m_bDisplayFilledWhenSelected = true;
    SetColor(QColor(24, 90, 231));
    m_bForce2D = false;
    mv_closed = true;
    mv_displayFilled = false;
    m_bPerVertexHeight = false;
}

void CAITerritoryObject::BeginEditParams(IEditor* ie, int flags)
{
    CAIShapeObjectBase::BeginEditParams(ie, flags);

    if (!CEntityObject::m_panel)
    {
        CEntityObject::m_panel = new CAITerritoryPanel;
        CEntityObject::m_rollupId = AddUIPage((QString("Entity: ") + m_entityClass).toUtf8().data(), CEntityObject::m_panel);
    }
    if (CEntityObject::m_panel && CEntityObject::m_panel->isVisible())
    {
        CEntityObject::m_panel->SetEntity(this);
        static_cast<CAITerritoryPanel*>(CEntityObject::m_panel)->UpdateAssignedAIsPanel();
    }

    CEntityObject::BeginEditParams(ie, flags);  // At the end add entity dialogs.
}

void CAITerritoryObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CBaseObject::BeginEditMultiSelParams(bAllOfSameType);

    if (!CEntityObject::m_panel)
    {
        CEntityObject::m_panel = new CAITerritoryPanel;
        CEntityObject::m_rollupId = AddUIPage((QString(m_entityClass) + " and other Entities").toUtf8().data(), CEntityObject::m_panel);
    }
    if (CEntityObject::m_panel && CEntityObject::m_panel->isVisible())
    {
        static_cast<CAITerritoryPanel*>(CEntityObject::m_panel)->UpdateAssignedAIsPanel();
    }

    if (!bAllOfSameType)
    {
        return;
    }

    if (!m_panelMulty)
    {
        m_panelMulty = new CShapeMultySelPanel;
        m_rollupMultyId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Multi Shape Operation", m_panelMulty);
    }
}

void CAITerritoryObject::EndEditMultiSelParams()
{
    if (m_rollupId != 0)
    {
        RemoveUIPage(m_rollupId);
    }
    m_rollupId = 0;
    m_panel = 0;

    if (m_rollupMultyId != 0)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_rollupMultyId);
        CalcBBox();
    }
    m_rollupMultyId = 0;
    m_panelMulty = 0;

    CBaseObject::EndEditMultiSelParams();
}

void CAITerritoryObject::SetName(const QString& newName)
{
    QString oldName = GetName();

    CAIShapeObjectBase::SetName(newName);

    GetIEditor()->GetAI()->GetAISystem()->Reset(IAISystem::RESET_INTERNAL);

    GetObjectManager()->FindAndRenameProperty2("aiterritory_Territory", oldName, newName);
}

void CAITerritoryObject::InitVariables()
{
    AddVariable(mv_height, "Height", functor(*this, &CAITerritoryObject::OnShapeChange));
    AddVariable(mv_displayFilled, "DisplayFilled");
}

void CAITerritoryObject::UpdateGameArea(bool bRemove)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }
    if (!IsCreateGameObjects())
    {
        return;
    }

    IAISystem* pAI = GetIEditor()->GetSystem()->GetAISystem();
    if (pAI)
    {
        if (!m_lastGameArea.isEmpty())
        {
            GetNavigation()->DeleteNavigationShape(m_lastGameArea.toUtf8().data());
        }
        if (bRemove)
        {
            return;
        }

        std::vector<Vec3> worldPts;
        const Matrix34& wtm = GetWorldTM();
        for (int i = 0; i < GetPointCount(); i++)
        {
            worldPts.push_back(wtm.TransformPoint(GetPoint(i)));
        }

        m_lastGameArea = GetName();
        if (!worldPts.empty())
        {
            SNavigationShapeParams params(m_lastGameArea.toUtf8().data(), AREATYPE_GENERIC, false, mv_closed, &worldPts[0], worldPts.size(), GetHeight(), 0, AIANCHOR_COMBAT_TERRITORY);
            GetNavigation()->CreateNavigationShape(params);
        }
    }
    m_bAreaModified = false;
}

void CAITerritoryObject::GetLinkedWaves(std::vector<CAIWaveObject*>& result)
{
    result.clear();

    for (int i = 0, n = GetEntityCount(); i < n; ++i)
    {
        CBaseObject* pBaseObject = GetEntity(i);
        if (qobject_cast<CAIWaveObject*>(pBaseObject))
        {
            result.push_back(static_cast<CAIWaveObject*>(pBaseObject));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// GameShapeObject
//////////////////////////////////////////////////////////////////////////
CGameShapeObject::CGameShapeObject()
    : m_minPoints(3)
    , m_maxPoints(1000)
{
    m_entityClass = "";
}

void CGameShapeObject::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);

    if (!m_panel)
    {
        auto panel = new CShapePanel;
        m_panel = panel;
        m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "Shape Parameters", panel);
    }
    if (m_panel)
    {
        m_panel->SetShape(this);
    }
}

void CGameShapeObject::InitVariables()
{
    bool noClassesAvailable = true;
    IGameVolumesEdit* pGameVolumesEdit = GetGameVolumesEdit();
    if (pGameVolumesEdit)
    {
        size_t volumeClassesCount = pGameVolumesEdit->GetVolumeClassesCount();
        for (size_t i = 0; i < volumeClassesCount; ++i)
        {
            const char* className = pGameVolumesEdit->GetVolumeClass(i);
            m_shapeClasses.AddEnumItem(className, (int)i);

            noClassesAvailable = false;
        }
    }

    AddVariable(mv_displayFilled, "DisplayFilled");
    AddVariable(mv_height, "Height");
    AddVariable(mv_closed, "Closed");
    AddVariable(m_shapeClasses, "VolumeClass", functor(*this, &CGameShapeObject::OnClassChanged));

    if (noClassesAvailable)
    {
        m_shapeClasses.SetFlags(m_shapeClasses.GetFlags() | IVariable::UI_INVISIBLE);
    }

    m_entityClass = m_shapeClasses.GetDisplayValue();
}

void CGameShapeObject::OnClassChanged(IVariable* var)
{
    QString classSelection = m_shapeClasses.GetDisplayValue();

    if (classSelection != m_entityClass)
    {
        ReloadGameObject(classSelection);
    }
}

void CGameShapeObject::ReloadGameObject(const QString& newClass)
{
    UnloadScript();

    m_entityClass = newClass;

    CreateGameObject();
}

IGameVolumesEdit* CGameShapeObject::GetGameVolumesEdit() const
{
    if (!gEnv->pGame || !gEnv->pGame->GetIGameFramework())
    {
        return nullptr;
    }

    IGameVolumes* pGameVolumesMgr = gEnv->pGame->GetIGameFramework()->GetIGameVolumesManager();
    return pGameVolumesMgr ? pGameVolumesMgr->GetEditorInterface() : nullptr;
}

bool CGameShapeObject::CreateGameObject()
{
    const bool output = CEntityObject::CreateGameObject();

    UpdateGameShape();

    return output;
}

void CGameShapeObject::UpdateGameArea(bool bRemove /* =false */)
{
    if (!m_bIgnoreGameUpdate)
    {
        UpdateGameShape();
    }
}

void CGameShapeObject::UpdateGameShape()
{
    IEntity* pEntity = GetIEntity();
    if (pEntity)
    {
        if (IGameVolumesEdit* pGameVolumesEdit = GetGameVolumesEdit())
        {
            IGameVolumes::VolumeInfo volumeInfo;
            volumeInfo.pVertices = m_points.size() > 0 ? &m_points[0] : nullptr;
            volumeInfo.verticesCount = m_points.size();
            volumeInfo.volumeHeight = mv_height;
            pGameVolumesEdit->SetVolume(pEntity->GetId(), volumeInfo);
        }

        NotifyPropertyChange();
    }
}

bool CGameShapeObject::IsShapeOnly() const
{
    bool isShapeOnly = false;

    IScriptTable* pScript = GetIEntity() ? GetIEntity()->GetScriptTable() : NULL;
    if (pScript)
    {
        const char* functionName = "IsShapeOnly";
        HSCRIPTFUNCTION scriptFunction(NULL);

        if (pScript->GetValue(functionName, scriptFunction) && (scriptFunction != NULL))
        {
            Script::CallReturn(gEnv->pScriptSystem, scriptFunction, isShapeOnly);

            gEnv->pScriptSystem->ReleaseFunc(scriptFunction);
        }
    }

    return isShapeOnly;
}

bool CGameShapeObject::GetIsClosedShape(bool& isClosed) const
{
    IScriptTable* pScript = GetIEntity() ? GetIEntity()->GetScriptTable() : NULL;
    if (pScript)
    {
        const char* functionName = "IsClosed";
        HSCRIPTFUNCTION scriptFunction(NULL);

        if (pScript->GetValue(functionName, scriptFunction) && (scriptFunction != NULL))
        {
            Script::CallReturn(gEnv->pScriptSystem, scriptFunction, isClosed);

            gEnv->pScriptSystem->ReleaseFunc(scriptFunction);
            return true;
        }
    }
    return false;
}

bool CGameShapeObject::NeedExportToGame() const
{
    bool exportToGame = true;

    IScriptTable* pScript = GetIEntity() ? GetIEntity()->GetScriptTable() : NULL;
    if (pScript)
    {
        const char* functionName = "ExportToGame";
        HSCRIPTFUNCTION scriptFunction(NULL);

        if (pScript->GetValue(functionName, scriptFunction) && (scriptFunction != NULL))
        {
            Script::CallReturn(gEnv->pScriptSystem, scriptFunction, exportToGame);

            gEnv->pScriptSystem->ReleaseFunc(scriptFunction);
        }
    }

    return exportToGame;
}

void CGameShapeObject::RefreshVariables()
{
    if (IsShapeOnly())
    {
        mv_height.SetFlags (mv_height.GetFlags() | IVariable::UI_INVISIBLE);
        mv_height.Set(0.0f);
    }
    else
    {
        mv_height.SetFlags (mv_height.GetFlags() & ~IVariable::UI_INVISIBLE);
    }

    bool isClosed;
    if (GetIsClosedShape(isClosed))
    {
        if (isClosed)
        {
            mv_closed.SetFlags(mv_closed.GetFlags() & ~IVariable::UI_INVISIBLE);
            mv_closed.Set(true);
        }
        else
        {
            mv_closed.SetFlags(mv_closed.GetFlags() | IVariable::UI_INVISIBLE);
            mv_closed.Set(false);
        }
    }

    if (m_pProperties)
    {
        if (IVariable* pVariable_MinSpec = m_pProperties->FindVariable("MinSpec"))
        {
            pVariable_MinSpec->SetFlags(pVariable_MinSpec->GetFlags()  | IVariable::UI_INVISIBLE);
        }
        if (IVariable* pVariable_MaterialLayerMask = m_pProperties->FindVariable("MaterialLayerMask"))
        {
            pVariable_MaterialLayerMask->SetFlags(pVariable_MaterialLayerMask->GetFlags()  | IVariable::UI_INVISIBLE);
        }
    }

    SetCustomMinAndMaxPoints();

    // Delete not needed points in the shape if there are too many for the new object class
    while (GetPointCount() > GetMaxPoints())
    {
        RemovePoint(GetPointCount() - 1);
    }
}

void CGameShapeObject::SetMaterial(CMaterial* mtl)
{
    CShapeObject::SetMaterial(mtl);

    NotifyPropertyChange();
}

void CGameShapeObject::NotifyPropertyChange()
{
    IEntity* pEntity = GetIEntity();
    if (pEntity != NULL)
    {
        SEntityEvent entityEvent;
        entityEvent.event = ENTITY_EVENT_EDITOR_PROPERTY_CHANGED;

        pEntity->SendEvent(entityEvent);
    }
}

void CGameShapeObject::InvalidateTM(int nWhyFlags)
{
    CEntityObject::InvalidateTM(nWhyFlags);

    m_bAreaModified = true;
}

void CGameShapeObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    CShapeObject::SetMinSpec(nSpec, bSetChildren);

    if (m_pProperties != NULL)
    {
        if (IVariable* pVariable = m_pProperties->FindVariable("MinSpec"))
        {
            pVariable->Set((int)nSpec);

            NotifyPropertyChange();
        }
    }
}

void CGameShapeObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    CShapeObject::SetMaterialLayersMask(nLayersMask);

    if (m_pProperties != NULL)
    {
        if (IVariable* pVariable = m_pProperties->FindVariable("MaterialLayerMask"))
        {
            pVariable->Set((int)nLayersMask);

            NotifyPropertyChange();
        }
    }
}

XmlNodeRef CGameShapeObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    // Allow the game side of the entity to decide if we want to export the data
    // In same cases the game is storing the required data in its own format,
    // and we don't need the entities to be created in pure game mode, they only make sense in the editor
    if (NeedExportToGame())
    {
        return CEntityObject::Export(levelPath, xmlNode);
    }
    else
    {
        return XmlNodeRef(NULL);
    }
}

void CGameShapeObject::SpawnEntity()
{
    RefreshVariables();

    CShapeObject::SpawnEntity();
}

void CGameShapeObject::DeleteEntity()
{
    if (IGameVolumesEdit* pVolumesEdit = GetGameVolumesEdit())
    {
        if (IEntity* pEntity = GetIEntity())
        {
            pVolumesEdit->DestroyVolume(pEntity->GetId());
        }
    }

    CShapeObject::DeleteEntity();
}

void CGameShapeObject::CalcBBox()
{
    if (m_points.empty())
    {
        return;
    }

    m_bbox.Reset();

    for (int i = 0; i < m_points.size(); i++)
    {
        m_bbox.Add(m_points[i]);
    }

    if (mv_height)
    {
        AABB box;
        box.SetTransformedAABB(GetWorldTM(), m_bbox);
        m_lowestHeight = box.min.z;

        if (m_bPerVertexHeight)
        {
            m_bbox.max.z += mv_height;
        }
        else
        {
            box.max.z = max(box.max.z, (float)(m_lowestHeight + mv_height));
            Matrix34 mat = GetWorldTM().GetInverted();
            Vec3 p = mat.TransformPoint(box.max);
            m_bbox.max.z = max(m_bbox.max.z, p.z);
        }
    }
}
void CGameShapeObject::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    CShapeObject::PostClone(pFromObject, ctx);

    //////////////////////////////////////////////////////////////////////////
    // Make sure properties are refreshed after cloning
    if ((m_pEntityScript != NULL) && (m_pEntity != NULL))
    {
        if ((m_pProperties != NULL) && (m_prototype == NULL))
        {
            m_pEntityScript->CopyPropertiesToScriptTable(m_pEntity, m_pProperties, false);
        }

        m_pEntityScript->CallOnPropertyChange(m_pEntity);
    }

    NotifyPropertyChange();
}

int CGameShapeObject::GetMaxPoints() const
{
    return m_maxPoints;
}

int CGameShapeObject::GetMinPoints() const
{
    return m_minPoints;
}

void CGameShapeObject::OnPropertyChangeDone(IVariable* var)
{
    NotifyPropertyChange();
    if (var)
    {
        UpdateGroup();
    }
}

void CGameShapeObject::SetCustomMinAndMaxPoints()
{
    IScriptTable* pScript = GetIEntity() ? GetIEntity()->GetScriptTable() : NULL;
    if (pScript)
    {
        const char* functionNameMinPoints = "ShapeMinPoints";
        HSCRIPTFUNCTION scriptFunctionMinPoints(NULL);

        int minPoints = CShapeObject::GetMinPoints();
        if (pScript->GetValue(functionNameMinPoints, scriptFunctionMinPoints) && (scriptFunctionMinPoints != NULL))
        {
            Script::CallReturn(gEnv->pScriptSystem, scriptFunctionMinPoints, minPoints);

            gEnv->pScriptSystem->ReleaseFunc(scriptFunctionMinPoints);
        }
        m_minPoints = minPoints;

        const char* functionNameMaxPoints = "ShapeMaxPoints";
        HSCRIPTFUNCTION scriptFunctionMaxPoints(NULL);

        int maxPoints = CShapeObject::GetMaxPoints();
        if (pScript->GetValue(functionNameMaxPoints, scriptFunctionMaxPoints) && (scriptFunctionMaxPoints != NULL))
        {
            Script::CallReturn(gEnv->pScriptSystem, scriptFunctionMaxPoints, maxPoints);

            gEnv->pScriptSystem->ReleaseFunc(scriptFunctionMaxPoints);
        }
        m_maxPoints = maxPoints;
    }
}

//////////////////////////////////////////////////////////////////////////
/// GameShape Ledge Object
//////////////////////////////////////////////////////////////////////////

CGameShapeLedgeObject::CGameShapeLedgeObject()
{
    m_entityClass = "LedgeObject";
}

void CGameShapeLedgeObject::BeginEditParams(IEditor* ie, int flags)
{
    CGameShapeObject::BeginEditParams(ie, flags);

    if (!m_panel)
    {
        auto panel = new CShapePanel;
        m_panel = panel;
        m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "Shape Parameters", panel);
    }
    if (m_panel)
    {
        m_panel->SetShape(this);
    }
}

void CGameShapeLedgeObject::InitVariables()
{
    //Leave empty, we don't need any of the properties of the base class
}

void CGameShapeLedgeObject::UpdateGameArea(bool bRemove)
{
    CGameShapeObject::UpdateGameArea(bRemove);

    // update the vault shape in realtime
    NotifyPropertyChange();
}

void CGameShapeLedgeObject::SetPoint(int index, const Vec3& pos)
{
    CGameShapeObject::SetPoint(index, pos);

    UpdateGameArea(false);
}

//////////////////////////////////////////////////////////////////////////
/// GameShape LedgeStatic Object
//////////////////////////////////////////////////////////////////////////
CGameShapeLedgeStaticObject::CGameShapeLedgeStaticObject()
{
    m_entityClass = "LedgeObjectStatic";
}

//////////////////////////////////////////////////////////////////////////
// Navigation Area Object
//////////////////////////////////////////////////////////////////////////
CNavigationAreaObject::CNavigationAreaObject()
{
    m_bDisplayFilledWhenSelected = true;

    m_bForce2D = false;
    mv_closed = true;
    mv_displayFilled = true;
    m_bPerVertexHeight = true;

    m_volume = NavigationVolumeID();
}


CNavigationAreaObject::~CNavigationAreaObject()
{
    mv_agentTypes.DeleteAllVariables();
}

void CNavigationAreaObject::Serialize(CObjectArchive& ar)
{
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        // Patch to avoid that old NavigationAreaObject have a serialized entity class name.
        // The navigationAreaObject currently needs to have an empty entity class name after this point
        xmlNode->setAttr("EntityClass", "");
    }
    CAIShapeObjectBase::Serialize(ar);
}

void CNavigationAreaObject::BeginEditParams(IEditor* pEditor, int flags)
{
    CBaseObject::BeginEditParams(pEditor, flags);

    if (!m_panel)
    {
        auto panel = new CNavigationAreaPanel;
        m_panel = panel;
        m_rollupId = pEditor->AddRollUpPage(ROLLUP_OBJECTS, "Edit Options", panel);
    }

    if (m_panel)
    {
        m_panel->SetShape(this);
    }
}

void CNavigationAreaObject::Display(DisplayContext& dc)
{
    if (IsSelected() || GetIEditor()->GetAI()->GetShowNavigationAreasState())
    {
        SetColor(mv_exclusion ? QColor(200, 0, 0) : QColor(0, 126, 255));

        float lineWidth = dc.GetLineWidth();
        dc.SetLineWidth(8.0f);
        CAIShapeObjectBase::Display(dc);
        dc.SetLineWidth(lineWidth);
    }
}

void CNavigationAreaObject::PostLoad(CObjectArchive& ar)
{
    if (IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem())
    {
        if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
        {
            aiNavigation->RegisterArea(GetName().toUtf8().data());
        }
    }

    CAIShapeObjectBase::PostLoad(ar);
}

void CNavigationAreaObject::Done()
{
    CAIShapeObjectBase::Done();

    if (IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem())
    {
        if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
        {
            aiNavigation->UnRegisterArea(GetName().toUtf8().data());
        }
    }
}

void CNavigationAreaObject::InitVariables()
{
    mv_height = 16;
    mv_exclusion = false;
    mv_displayFilled = false;

    AddVariable(mv_height, "Height", functor(*this, &CNavigationAreaObject::OnShapeChange));
    AddVariable(mv_exclusion, "Exclusion", functor(*this, &CNavigationAreaObject::OnShapeTypeChange));
    AddVariable(mv_displayFilled, "DisplayFilled");

    CAIManager* manager = GetIEditor()->GetAI();
    size_t agentTypeCount = manager->GetNavigationAgentTypeCount();

    if (agentTypeCount)
    {
        mv_agentTypeVars.resize(agentTypeCount);
        AddVariable(mv_agentTypes, "AffectedAgentTypes", "Affected Agent Types");

        for (size_t i = 0; i < agentTypeCount; ++i)
        {
            const char* name = manager->GetNavigationAgentTypeName(i);
            AddVariable(mv_agentTypes, mv_agentTypeVars[i], name, name,
                functor(*this, &CNavigationAreaObject::OnShapeAgentTypeChange));
            mv_agentTypeVars[i].Set(true);
        }
    }
}

void CNavigationAreaObject::OnShapeTypeChange(IVariable* var)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }

    UpdateMeshes();
    ApplyExclusion(mv_exclusion);
}

void CNavigationAreaObject::OnShapeAgentTypeChange(IVariable* var)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }

    UpdateMeshes();
    ApplyExclusion(mv_exclusion);
}

void CNavigationAreaObject::OnNavigationEvent(const INavigationSystem::ENavigationEvent event)
{
    switch (event)
    {
    case INavigationSystem::MeshReloaded:
        RelinkWithMesh();
        break;
    case INavigationSystem::MeshReloadedAfterExporting:
        RelinkWithMesh(false);
        break;
    case INavigationSystem::NavigationCleared:
        RelinkWithMesh();
        break;
    default:
        assert(0);
        break;
    }
}

void CNavigationAreaObject::UpdateMeshes()
{
    CAIManager* manager = GetIEditor()->GetAI();
    const size_t agentTypeCount = manager->GetNavigationAgentTypeCount();

    IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem();

    if (mv_exclusion)
    {
        for (size_t i = 0; i < m_meshes.size(); ++i)
        {
            if (const NavigationMeshID meshID = m_meshes[i])
            {
                aiSystem->GetNavigationSystem()->DestroyMesh(meshID);
            }
        }

        m_meshes.clear();
    }
    else
    {
        m_meshes.resize(agentTypeCount);

        for (size_t i = 0; i < agentTypeCount; ++i)
        {
            NavigationMeshID meshID = m_meshes[i];

            if (mv_agentTypeVars[i] && !meshID)
            {
                NavigationAgentTypeID agentTypeID = aiSystem->GetNavigationSystem()->GetAgentTypeID(i);

                INavigationSystem::CreateMeshParams params; // TODO: expose at least the tile size
                meshID = aiSystem->GetNavigationSystem()->CreateMesh(GetName().toUtf8().data(), agentTypeID, params);
                aiSystem->GetNavigationSystem()->SetMeshBoundaryVolume(meshID, m_volume);

                AABB aabb;
                GetBoundBox(aabb);
                aiSystem->GetNavigationSystem()->QueueMeshUpdate(meshID, aabb);

                m_meshes[i] = meshID;
            }
            else if (!mv_agentTypeVars[i] && meshID)
            {
                aiSystem->GetNavigationSystem()->DestroyMesh(meshID);
                m_meshes[i] = NavigationMeshID();
            }
        }
    }
}

void CNavigationAreaObject::ApplyExclusion(bool apply)
{
    CAIManager* manager = GetIEditor()->GetAI();
    size_t agentTypeCount = manager->GetNavigationAgentTypeCount();

    IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem();

    std::vector<NavigationAgentTypeID> affectedAgentTypes;

    if (apply)
    {
        affectedAgentTypes.reserve(agentTypeCount);

        for (size_t i = 0; i < agentTypeCount; ++i)
        {
            if (mv_agentTypeVars[i])
            {
                NavigationAgentTypeID agentTypeID = aiSystem->GetNavigationSystem()->GetAgentTypeID(i);
                affectedAgentTypes.push_back(agentTypeID);
            }
        }
    }

    if (affectedAgentTypes.empty())
    {
        aiSystem->GetNavigationSystem()->SetExclusionVolume(0, 0, m_volume);
    }
    else
    {
        aiSystem->GetNavigationSystem()->SetExclusionVolume(&affectedAgentTypes[0], affectedAgentTypes.size(), m_volume);
    }
}

void CNavigationAreaObject::RelinkWithMesh(bool bUpdateGameArea)
{
    IAISystem* pAISystem = GetIEditor()->GetSystem()->GetAISystem();

    if (!pAISystem)
    {
        return;
    }

    INavigationSystem* pAINavigation = pAISystem->GetNavigationSystem();
    m_volume = pAINavigation->GetAreaId(GetName().toUtf8().data());

    if (!mv_exclusion)
    {
        CAIManager* manager = GetIEditor()->GetAI();
        const size_t agentTypeCount = manager->GetNavigationAgentTypeCount();
        m_meshes.resize(agentTypeCount);
        for (size_t i = 0; i < agentTypeCount; ++i)
        {
            NavigationAgentTypeID agentTypeID = pAINavigation->GetAgentTypeID(i);
            m_meshes[i] = pAINavigation->GetMeshID(GetName().toUtf8().data(), agentTypeID);
        }
    }

    /*
      FR: We need to update the game area even in the case that after having read
      the data from the binary file the volumewas not recreated.
      This happens when there was no mesh associated to the actual volume.
    */
    if (bUpdateGameArea || !pAINavigation->ValidateVolume(m_volume))
    {
        UpdateGameArea(false);
    }
}

void CNavigationAreaObject::CreateVolume(Vec3* points, size_t pointsSize, NavigationVolumeID requestedID /* = NavigationVolumeID */)
{
    if (IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem())
    {
        if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
        {
            m_volume = aiNavigation->CreateVolume(points, pointsSize, mv_height, requestedID);

            aiNavigation->RegisterListener(this, GetName().toUtf8().data());
            if (requestedID == NavigationVolumeID())
            {
                aiNavigation->SetAreaId(GetName().toUtf8().data(), m_volume);
            }
        }
    }
}

void CNavigationAreaObject::DestroyVolume()
{
    if (!m_volume)
    {
        return;
    }
    if (IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem())
    {
        if (INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem())
        {
            aiNavigation->DestroyVolume(m_volume);
            aiNavigation->UnRegisterListener(this);
            m_volume = NavigationVolumeID();
        }
    }
}

void CNavigationAreaObject::UpdateGameArea(bool remove)
{
    if (m_bIgnoreGameUpdate)
    {
        return;
    }

    if (!IsCreateGameObjects())
    {
        return;
    }

    if (IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem())
    {
        INavigationSystem* aiNavigation = aiSystem->GetNavigationSystem();

        if (remove)
        {
            for (size_t i = 0; i < m_meshes.size(); ++i)
            {
                if (m_meshes[i])
                {
                    aiNavigation->DestroyMesh(m_meshes[i]);
                }
            }

            m_meshes.clear();
            DestroyVolume();
            return;
        }

        const Matrix34& worldTM = GetWorldTM();
        const size_t pointCount = GetPointCount();

        if (pointCount > 2)
        {
            std::vector<Vec3> points;
            points.reserve(pointCount);

            for (size_t i = 0; i < pointCount; ++i)
            {
                points.push_back(worldTM.TransformPoint(GetPoint(i)));
            }

            // The volume could be set but if the binary data didn't exist the volume
            // was not correctly recreated
            if (!m_volume || !aiNavigation->ValidateVolume(m_volume))
            {
                CreateVolume(&points[0], points.size(), m_volume);
            }
            else
            {
                aiNavigation->SetVolume(m_volume, &points[0], points.size(), mv_height);
            }

            UpdateMeshes();
        }
        else if (m_volume)
        {
            DestroyVolume();
            UpdateMeshes();
        }

        ApplyExclusion(mv_exclusion);
    }

    m_lastGameArea = GetName();
    m_bAreaModified = false;
}

void CNavigationAreaObject::SetName(const QString& name)
{
    CAIShapeObjectBase::SetName(name);

    if (IAISystem* aiSystem = GetIEditor()->GetSystem()->GetAISystem())
    {
        INavigationSystem* pNavigationSystem = aiSystem->GetNavigationSystem();
        pNavigationSystem->UpdateAreaNameForId(m_volume, name.toUtf8().data());
        for (size_t i = 0; i < m_meshes.size(); ++i)
        {
            if (m_meshes[i])
            {
                pNavigationSystem->SetMeshName(m_meshes[i], name.toUtf8().data());
            }
        }
    }
}

void CNavigationAreaObject::RemovePoint(int index)
{
    CAIShapeObjectBase::RemovePoint(index);

    UpdateGameArea(false);
}

int CNavigationAreaObject::InsertPoint(int index, const Vec3& point, bool const bModifying)
{
    int result = CAIShapeObjectBase::InsertPoint(index, Vec3(point.x, point.y, 0.0f), bModifying);

    UpdateGameArea(false);

    return result;
}

void CNavigationAreaObject::SetPoint(int index, const Vec3& pos)
{
    CAIShapeObjectBase::SetPoint(index, Vec3(pos.x, pos.y, 0.0f));

    UpdateGameArea(false);
}

void CNavigationAreaObject::ChangeColor(const QColor& color)
{
    SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    static void PyInsertPoint(const char* objName, int idx, float xPos, float yPos, float zPos)
    {
        CBaseObject* pObject;

        if (GetIEditor()->GetObjectManager()->FindObject(objName))
        {
            pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        }
        else if (GetIEditor()->GetObjectManager()->FindObject(GuidUtil::FromString(objName)))
        {
            pObject = GetIEditor()->GetObjectManager()->FindObject(GuidUtil::FromString(objName));
        }
        else
        {
            throw std::logic_error((QString("\"") + objName + "\" is an invalid object.").toUtf8().data());
        }

        if (qobject_cast<CNavigationAreaObject*>(pObject))
        {
            CNavigationAreaObject* pNavArea = static_cast<CNavigationAreaObject*>(pObject);
            if (pNavArea != NULL)
            {
                Vec3 pos(xPos, yPos, zPos);
                pNavArea->SetPoint(idx, pos);
                pNavArea->InsertPoint(-1,  pos, false);
            }
        }
    }
}

REGISTER_PYTHON_COMMAND(PyInsertPoint, general, nav_insert_point, "Added a point at the given position to the given nav area");

#include <Objects/ShapeObject.moc>
