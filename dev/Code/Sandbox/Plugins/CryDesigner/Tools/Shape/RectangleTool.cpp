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
#include "RectangleTool.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

void RectangleTool::Enter()
{
    ShapeTool::Enter();
    m_Phase = eRectanglePhase_PlaceFirstPoint;
}

void RectangleTool::Leave()
{
    if (m_Phase == eRectanglePhase_Done)
    {
        Freeze2DModel();
        GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
    }
    else
    {
        MODEL_SHELF_RECONSTRUCTOR(GetModel());
        GetModel()->SetShelf(1);
        GetModel()->Clear();
        UpdateShelf(1);
        GetIEditor()->CancelUndo();
    }
    ShapeTool::Leave();
}

void RectangleTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eRectanglePhase_PlaceFirstPoint)
    {
        if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
        {
            SetPlane(GetCurrentSpot().m_Plane);
            SetStartSpot(GetCurrentSpot());
            SetTempPolygon(GetCurrentSpot().m_pPolygon);
            m_Phase = eRectanglePhase_DrawRectangle;
            GetIEditor()->BeginUndo();
            GetModel()->RecordUndo("Designer : Create a Rectangle", GetBaseObject());
            StoreSeparateStatus();
        }
    }
}

bool RectangleTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_Phase == eRectanglePhase_DrawRectangle)
        {
            CancelCreation();
            SetEditMode(eEditMode_None);
            return true;
        }
        else if (m_Phase == eRectanglePhase_Done)
        {
            Freeze2DModel();
            m_Phase = eRectanglePhase_PlaceFirstPoint;
            GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void RectangleTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eRectanglePhase_DrawRectangle)
    {
        if (std::abs(GetCurrentSpotPos().x - GetStartSpotPos().x) > (BrushFloat)0.01 ||
            std::abs(GetCurrentSpotPos().y - GetStartSpotPos().y) > (BrushFloat)0.01 ||
            std::abs(GetCurrentSpotPos().z - GetStartSpotPos().z) > (BrushFloat)0.01)
        {
            m_Phase = eRectanglePhase_Done;
            UpdateRectangle(GetStartSpotPos(), GetCurrentSpotPos(), true, true);
        }
    }

    if (m_Phase == eRectanglePhase_Done)
    {
        Freeze2DModel();
        m_Phase = eRectanglePhase_PlaceFirstPoint;
        SetIntermediatePolygon(NULL);
        GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
    }
}

void RectangleTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eRectanglePhase_PlaceFirstPoint || m_Phase == eRectanglePhase_Done)
    {
        UpdateCurrentSpotPosition(view, nFlags, point, false, true);
        if (m_Phase == eRectanglePhase_PlaceFirstPoint)
        {
            SetPlane(GetCurrentSpot().m_Plane);
        }
    }
    else if (m_Phase == eRectanglePhase_DrawRectangle)
    {
        if ((nFlags & MK_LBUTTON) && UpdateCurrentSpotPosition(view, nFlags, point, true, true))
        {
            UpdateRectangle(GetStartSpotPos(), GetCurrentSpotPos(), false, true);
        }
    }
}

void RectangleTool::UpdateRectangle(const BrushVec3& v0, const BrushVec3& v1, bool bRenderFace, bool bUpdateUIs)
{
    CD::PolygonPtr pPolygon = GetIntermediatePolygon();
    if (!pPolygon)
    {
        SetIntermediatePolygon(new CD::Polygon);
        pPolygon = GetIntermediatePolygon();
    }

    std::vector<BrushVec3> vList(4);

    BrushVec2 p0 = GetPlane().W2P(v0);
    BrushVec2 p1 = GetPlane().W2P(v1);

    m_v[0] = v0;
    m_v[1] = v1;

    vList[0] = v0;
    vList[1] = GetPlane().P2W(BrushVec2(p1.x, p0.y));
    vList[2] = v1;
    vList[3] = GetPlane().P2W(BrushVec2(p0.x, p1.y));

    if (bUpdateUIs)
    {
        m_RectangleParameter.m_Width = aznumeric_cast<float>(std::abs(p0.x - p1.x));
        m_RectangleParameter.m_Depth = aznumeric_cast<float>(std::abs(p0.y - p1.y));
        GetPanel()->Update();
    }

    *pPolygon = CD::Polygon(vList);
    if (!pPolygon->GetPlane().IsSameFacing(GetPlane()))
    {
        pPolygon->Flip();
    }

    if (bRenderFace)
    {
        MODEL_SHELF_RECONSTRUCTOR(GetModel());

        GetModel()->SetShelf(1);
        GetModel()->Clear();

        if (GetTempPolygon())
        {
            pPolygon->SetTexInfo(GetTempPolygon()->GetTexInfo());
        }

        if (!pPolygon->IsOpen())
        {
            CD::AddPolygonWithSubMatID(pPolygon);
        }

        UpdateAll(CD::eUT_Mirror);
        UpdateShelf(1);
    }
}

void RectangleTool::Display(DisplayContext& dc)
{
    DisplayCurrentSpot(dc);

    if (m_Phase == eRectanglePhase_DrawRectangle && GetIntermediatePolygon())
    {
        DisplayDimensionHelper(dc, GetIntermediatePolygon()->GetBoundBox());
    }
    else
    {
        DisplayDimensionHelper(dc, 1);
    }

    if (GetStartSpot().m_pPolygon)
    {
        DisplayDimensionHelper(dc);
    }

    if (m_Phase == eRectanglePhase_DrawRectangle || m_Phase == eRectanglePhase_Done)
    {
        DrawIntermediatePolygon(dc);
    }
}

void RectangleTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (m_Phase == eRectanglePhase_Done)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Rectangle");
            Freeze2DModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        m_Phase = eRectanglePhase_PlaceFirstPoint;
    }
}

void RectangleTool::OnChangeParameter(bool continuous)
{
    BrushVec2 p0 = GetPlane().W2P(m_v[0]);
    BrushVec2 p1 = GetPlane().W2P(m_v[1]);
    BrushVec2 center = (p0 + p1) * 0.5f;
    p0.x = center.x - m_RectangleParameter.m_Width * 0.5f;
    p1.x = center.x + m_RectangleParameter.m_Width * 0.5f;
    p0.y = center.y - m_RectangleParameter.m_Depth * 0.5f;
    p1.y = center.y + m_RectangleParameter.m_Depth * 0.5f;
    UpdateRectangle(GetPlane().P2W(p0), GetPlane().P2W(p1), true, false);
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Rectangle, CD::eToolGroup_Shape, "Rectangle", RectangleTool, PropertyTreePanel<RectangleTool>)
