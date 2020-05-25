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
#include "DiscTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

void DiscTool::Enter()
{
    ShapeTool::Enter();
    SetEditMode(eEditMode_Beginning);
}

void DiscTool::Leave()
{
    if (GetEditMode() == eEditMode_Done)
    {
        Freeze2DModel();
        GetIEditor()->AcceptUndo("Designer : Create a Disc");
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

void DiscTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    bool bKeepInitPlane = GetEditMode() == eEditMode_Editing ? true : false;
    bool bSearchAllShelves = GetEditMode() == eEditMode_Done ? true : false;
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, bKeepInitPlane, bSearchAllShelves))
    {
        return;
    }

    if (GetEditMode() == eEditMode_Editing)
    {
        BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());

        m_DiscParameter.m_Radius = aznumeric_cast<float>((vSpotPos2D - m_vCenterOnPlane).GetLength());
        const BrushFloat kSmallestRadius = 0.05f;
        if (m_DiscParameter.m_Radius < kSmallestRadius)
        {
            m_DiscParameter.m_Radius = kSmallestRadius;
        }

        m_fAngle = CD::ComputeAnglePointedByPos(m_vCenterOnPlane, vSpotPos2D);
        Update();
        GetPanel()->Update();
    }
}

void DiscTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (GetEditMode() == eEditMode_Beginning)
    {
        if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
        {
            SetStartSpot(GetCurrentSpot());
            SetEditMode(eEditMode_Editing);
            SetPlane(GetCurrentSpot().m_Plane);
            SetTempPolygon(GetCurrentSpot().m_pPolygon);
            m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
            GetIEditor()->BeginUndo();
            GetModel()->RecordUndo("Designer : Create a Disc", GetBaseObject());
            StoreSeparateStatus();
        }
    }
    else if (GetEditMode() == eEditMode_Editing)
    {
        SetEditMode(eEditMode_Done);
        RegisterDrawnPolygonToDesigner();
    }
}

void DiscTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (GetEditMode() == eEditMode_Done)
    {
        SetEditMode(eEditMode_Beginning);
        if (GetIntermediatePolygon()->IsValid())
        {
            Freeze2DModel();
            GetIEditor()->AcceptUndo("Designer : Create a Disc");
        }
    }
}

bool DiscTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (GetEditMode() == eEditMode_Editing)
        {
            CancelCreation();
            SetEditMode(eEditMode_Beginning);
        }
        else if (GetEditMode() == eEditMode_Done)
        {
            Freeze2DModel();
            SetEditMode(eEditMode_Beginning);
            GetIEditor()->AcceptUndo("Designer : Create a Disc");
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void DiscTool::Display(DisplayContext& dc)
{
    DisplayCurrentSpot(dc);

    if (GetEditMode() == eEditMode_Editing && GetIntermediatePolygon())
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
    if (GetEditMode() == eEditMode_Editing || GetEditMode() == eEditMode_Done)
    {
        DrawIntermediatePolygon(dc);
    }
}

void DiscTool::Update()
{
    if (GetEditMode() == eEditMode_Beginning || m_DiscParameter.m_NumOfSubdivision < 3)
    {
        return;
    }
    std::vector<BrushVec2> vertices2D;
    CD::MakeSectorOfCircle(m_DiscParameter.m_Radius, m_vCenterOnPlane, m_fAngle, CD::PI2, m_DiscParameter.m_NumOfSubdivision + 1, vertices2D);
    vertices2D.erase(vertices2D.begin());
    CD::STexInfo texInfo = GetTempPolygon() ? GetTempPolygon()->GetTexInfo() : GetTexInfo();
    int nMaterialID = GetTempPolygon() ? GetTempPolygon()->GetMaterialID() : 0;
    if (GetIntermediatePolygon())
    {
        *GetIntermediatePolygon() = CD::Polygon(vertices2D, GetPlane(), GetMatID(), &texInfo, true);
    }
    else
    {
        SetIntermediatePolygon(new CD::Polygon(vertices2D, GetPlane(), GetMatID(), &texInfo, true));
    }
    GetIntermediatePolygon()->SetMaterialID(nMaterialID);
    if (GetEditMode() == eEditMode_Done)
    {
        RegisterDrawnPolygonToDesigner();
    }
}

void DiscTool::RegisterDrawnPolygonToDesigner()
{
    if (!GetIntermediatePolygon())
    {
        return;
    }
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);
    GetModel()->Clear();
    CD::AddPolygonWithSubMatID(GetIntermediatePolygon());
    UpdateAll(CD::eUT_Mirror);
    UpdateShelf(1);
}

void DiscTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (GetEditMode() == eEditMode_Done)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Disc");
            Freeze2DModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        SetEditMode(eEditMode_Beginning);
    }
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Disc, CD::eToolGroup_Shape, "Disc", DiscTool, PropertyTreePanel<DiscTool>)
