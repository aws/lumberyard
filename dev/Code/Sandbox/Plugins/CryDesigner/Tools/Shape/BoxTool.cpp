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
#include "BoxTool.h"
#include "Tools/DesignerTool.h"
#include "Tools/Select/SelectTool.h"
#include "Viewport.h"
#include "Util/HeightManipulator.h"
#include "Util/ExtrusionSnappingHelper.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

void BoxTool::Enter()
{
    ShapeTool::Enter();
    m_Phase = eBoxPhase_PlaceFirstPoint;
    m_bStartedUndo = false;
}

void BoxTool::Leave()
{
    if (m_Phase == eBoxPhase_Done)
    {
        if (m_bStartedUndo)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Box");
        }
        FreezeModel();
    }
    else
    {
        if (GetModel())
        {
            MODEL_SHELF_RECONSTRUCTOR(GetModel());
            GetModel()->SetShelf(1);
            GetModel()->Clear();
            UpdateShelf(1);
        }
        if (m_bStartedUndo)
        {
            GetIEditor()->CancelUndo();
        }
    }
    m_bStartedUndo = false;
    ShapeTool::Leave();
}

void BoxTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eBoxPhase_PlaceFirstPoint)
    {
        if (!UpdateCurrentSpotPosition(view, nFlags, point, false, false))
        {
            return;
        }

        SetStartSpot(GetCurrentSpot());
        SetPlane(GetCurrentSpot().m_Plane);
        SetTempPolygon(GetCurrentSpot().m_pPolygon);
        m_Phase = eBoxPhase_DrawRectangle;
        s_SnappingHelper.Init(GetModel());
        GetIEditor()->BeginUndo();
        GetModel()->RecordUndo("Designer : Create a box", GetBaseObject());
        StoreSeparateStatus();
        m_bStartedUndo = true;
    }
    else if (m_Phase == eBoxPhase_RaiseHeight)
    {
        m_Phase = eBoxPhase_Done;
    }
}

void BoxTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eBoxPhase_DrawRectangle)
    {
        if (!GetCurrentSpotPos().IsEquivalent(GetStartSpotPos(), (BrushFloat)0.01))
        {
            m_Phase = eBoxPhase_RaiseHeight;
            s_SnappingHelper.SearchForOppositePolygons(m_pCapPolygon);
            m_pCapPolygon = NULL;
            m_bIsOverOpposite = false;
            s_HeightManipulator.Init(GetPlane(), GetCurrentSpotPos());
        }
    }

    if (m_Phase == eBoxPhase_Done)
    {
        FreezeModel();
        if (m_bStartedUndo)
        {
            GetIEditor()->AcceptUndo("Designer : Create a box");
        }
        m_Phase = eBoxPhase_PlaceFirstPoint;
        m_bStartedUndo = false;
    }
}

void BoxTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eBoxPhase_PlaceFirstPoint || m_Phase == eBoxPhase_Done)
    {
        UpdateCurrentSpotPosition(view, nFlags, point, false, true);
        if (m_Phase == eBoxPhase_PlaceFirstPoint)
        {
            SetPlane(GetCurrentSpot().m_Plane);
        }
    }
    else if (m_Phase == eBoxPhase_DrawRectangle)
    {
        if ((nFlags & MK_LBUTTON) && UpdateCurrentSpotPosition(view, nFlags, point, true))
        {
            UpdateBox(GetStartSpotPos(), GetCurrentSpotPos(), 0, true);
        }
    }
    else if (m_Phase == eBoxPhase_RaiseHeight)
    {
        BrushFloat fHeight = s_HeightManipulator.UpdateHeight(GetWorldTM(), view, point);

        if (fHeight < kInitialPrimitiveHeight)
        {
            fHeight = 0;
        }

        if (nFlags & MK_SHIFT)
        {
            CD::PolygonPtr pAlignedPolygon = s_SnappingHelper.FindAlignedPolygon(m_pCapPolygon, GetWorldTM(), view, point);
            if (pAlignedPolygon)
            {
                fHeight = GetPlane().Distance() - pAlignedPolygon->GetPlane().Distance();
            }
        }

        UpdateBoxWithBoundaryCheck(GetStartSpotPos(), GetCurrentSpotPos(), fHeight, true);
    }
}

bool BoxTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_Phase == eBoxPhase_DrawRectangle || m_Phase == eBoxPhase_RaiseHeight)
        {
            CancelCreation();
        }
        else if (m_Phase == eBoxPhase_Done)
        {
            FreezeModel();
            GetIEditor()->AcceptUndo("Designer : Create a box");
            m_Phase = eBoxPhase_PlaceFirstPoint;
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void BoxTool::UpdateBoxWithBoundaryCheck(const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight, bool bUpdateUI)
{
    UpdateBox(v0, v1, fHeight, bUpdateUI);
    m_bIsOverOpposite = m_pCapPolygon && s_SnappingHelper.IsOverOppositePolygon(m_pCapPolygon, CD::ePP_Pull);
    if (m_bIsOverOpposite)
    {
        fHeight = s_SnappingHelper.GetNearestDistanceToOpposite(CD::ePP_Pull);
        UpdateBox(v0, v1, fHeight, bUpdateUI);
    }
}

void BoxTool::UpdateBox(const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight, bool bUpdateUI)
{
    std::vector<CD::PolygonPtr> polygonList;

    std::vector<BrushVec3> vList(4);

    BrushVec2 p0 = GetPlane().W2P(v0);
    BrushVec2 p1 = GetPlane().W2P(v1);

    m_v[0] = GetPlane().P2W(p0);
    m_v[1] = GetPlane().P2W(p1);

    vList[0] = m_v[0];
    vList[1] = GetPlane().P2W(BrushVec2(p1.x, p0.y));
    vList[2] = m_v[1];
    vList[3] = GetPlane().P2W(BrushVec2(p0.x, p1.y));

    polygonList.push_back(new CD::Polygon(vList));
    if (polygonList[0]->IsOpen())
    {
        return;
    }
    if (polygonList[0]->GetPlane().IsSameFacing(GetPlane()))
    {
        polygonList[0]->Flip();
    }

    for (int i = 0; i < 4; ++i)
    {
        vList[i] += GetPlane().Normal() * fHeight;
    }
    polygonList.push_back(new CD::Polygon(vList));
    if (!polygonList[1]->GetPlane().IsSameFacing(GetPlane()))
    {
        polygonList[1]->Flip();
    }

    for (int i = 0; i < 4; ++i)
    {
        BrushEdge3D e = polygonList[0]->GetEdge(i);

        vList[0] = e.m_v[1];
        vList[1] = e.m_v[0];
        vList[2] = e.m_v[0] + GetPlane().Normal() * fHeight;
        vList[3] = e.m_v[1] + GetPlane().Normal() * fHeight;

        polygonList.push_back(new CD::Polygon(vList));
    }

    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    GetModel()->SetShelf(1);
    GetModel()->Clear();

    for (int i = 0, iCount(polygonList.size()); i < iCount; ++i)
    {
        if (polygonList[i]->IsOpen())
        {
            continue;
        }

        if (GetTempPolygon())
        {
            polygonList[i]->SetTexInfo(GetTempPolygon()->GetTexInfo());
            polygonList[i]->SetMaterialID(GetTempPolygon()->GetMaterialID());
        }

        CD::AddPolygonWithSubMatID(polygonList[i]);

        if (CD::IsEquivalent(polygonList[i]->GetPlane().Normal(), GetPlane().Normal()))
        {
            m_pCapPolygon = polygonList[i];
        }
    }

    if (bUpdateUI)
    {
        m_BoxParameter.m_Width = aznumeric_cast<float>(std::abs(p0.x - p1.x));
        m_BoxParameter.m_Height = (float)fHeight;
        m_BoxParameter.m_Depth = aznumeric_cast<float>(std::abs(p0.y - p1.y));
        GetPanel()->Update();
    }

    UpdateAll(CD::eUT_Mesh | CD::eUT_Mirror);
}

void BoxTool::Display(DisplayContext& dc)
{
    DisplayCurrentSpot(dc);
    DisplayDimensionHelper(dc, 1);
    if (GetStartSpot().m_pPolygon)
    {
        DisplayDimensionHelper(dc);
    }
    s_HeightManipulator.Display(dc);
}

void BoxTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (m_Phase == eBoxPhase_Done)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Box");
            FreezeModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        m_Phase = eBoxPhase_PlaceFirstPoint;
        break;
    }
}

void BoxTool::FreezeModel()
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    if (m_pCapPolygon)
    {
        if (m_bIsOverOpposite)
        {
            s_SnappingHelper.ApplyOppositePolygons(m_pCapPolygon, CD::ePP_Pull, true);
        }

        if (!IsSeparateStatus())
        {
            GetModel()->SetShelf(1);
            std::vector<CD::PolygonPtr> sidePolygons;

            BrushPlane invertedCapPolygonPlane = m_pCapPolygon->GetPlane().GetInverted();
            for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
            {
                CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
                if (pPolygon != m_pCapPolygon && !CD::IsEquivalent(invertedCapPolygonPlane.Normal(), pPolygon->GetPlane().Normal()))
                {
                    sidePolygons.push_back(pPolygon);
                }
            }

            std::vector<CD::PolygonPtr> intersectedSidePolygons;
            for (int i = 0, iSidePolygonCount(sidePolygons.size()); i < iSidePolygonCount; ++i)
            {
                for (int k = 0; k < 2; ++k)
                {
                    CD::PolygonPtr pSidePolygon = k == 0 ? sidePolygons[i]->Clone()->Flip() : sidePolygons[i];

                    GetModel()->SetShelf(0);
                    bool bHasIntersected = GetModel()->HasIntersection(pSidePolygon, true);
                    bool bTouched = GetModel()->HasTouched(pSidePolygon);
                    if ((!bHasIntersected && !bTouched) || (bTouched && k == 0))
                    {
                        continue;
                    }

                    GetModel()->SetShelf(1);
                    GetModel()->RemovePolygon(sidePolygons[i]);

                    GetModel()->SetShelf(0);
                    if (bHasIntersected)
                    {
                        GetModel()->AddPolygon(pSidePolygon, CD::eOpType_ExclusiveOR);
                        intersectedSidePolygons.push_back(pSidePolygon);
                    }
                    else if (bTouched)
                    {
                        GetModel()->AddPolygon(pSidePolygon, CD::eOpType_Union);
                    }
                    break;
                }
            }

            for (int i = 0, iCount(intersectedSidePolygons.size()); i < iCount; ++i)
            {
                GetModel()->SetShelf(0);
                GetModel()->SeparatePolygons(intersectedSidePolygons[i]->GetPlane());
            }
        }
    }

    ShapeTool::FreezeModel();
}

void BoxTool::OnChangeParameter(bool continuous)
{
    BrushVec2 p0 = GetPlane().W2P(m_v[0]);
    BrushVec2 p1 = GetPlane().W2P(m_v[1]);
    BrushVec2 center = (p0 + p1) * 0.5f;
    p0.x = center.x - m_BoxParameter.m_Width * 0.5f;
    p1.x = center.x + m_BoxParameter.m_Width * 0.5f;
    p0.y = center.y - m_BoxParameter.m_Depth * 0.5f;
    p1.y = center.y + m_BoxParameter.m_Depth * 0.5f;
    UpdateBoxWithBoundaryCheck(GetPlane().P2W(p0), GetPlane().P2W(p1), m_BoxParameter.m_Height, false);
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Box, CD::eToolGroup_Shape, "Box", BoxTool, PropertyTreePanel<BoxTool>)