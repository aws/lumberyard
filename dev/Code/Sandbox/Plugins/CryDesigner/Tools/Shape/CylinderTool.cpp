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
#include "CylinderTool.h"
#include "Core/Model.h"
#include "Util/PrimitiveShape.h"
#include "Tools/DesignerTool.h"
#include "Util/HeightManipulator.h"
#include "Util/ExtrusionSnappingHelper.h"
#include "ViewManager.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

void CylinderTool::Enter()
{
    ShapeTool::Enter();
    m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
}

void CylinderTool::Leave()
{
    if (m_CylinderPhase == eCylinderPhase_Done)
    {
        FreezeModel();
        GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
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

void CylinderTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    bool bKeepInitPlane = m_CylinderPhase == eCylinderPhase_Radius ? true : false;
    bool kSearchAllShelves = m_CylinderPhase == eCylinderPhase_Done ? true : false;

    if (m_CylinderPhase == eCylinderPhase_PlaceFirstPoint || m_CylinderPhase == eCylinderPhase_Radius)
    {
        if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, bKeepInitPlane, kSearchAllShelves))
        {
            return;
        }
    }

    if (m_CylinderPhase == eCylinderPhase_Radius)
    {
        BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());
        m_CylinderParameter.m_Radius = aznumeric_cast<float>((vSpotPos2D - m_vCenterOnPlane).GetLength());
        const BrushFloat kSmallestRadius = 0.05f;
        if (m_CylinderParameter.m_Radius < kSmallestRadius)
        {
            m_CylinderParameter.m_Radius = kSmallestRadius;
        }
        m_fAngle = CD::ComputeAnglePointedByPos(m_vCenterOnPlane, vSpotPos2D);
        UpdateBasePolygon(m_CylinderParameter.m_Radius, m_CylinderParameter.m_NumOfSubdivision);
        GetPanel()->Update();
    }
    else if (m_CylinderPhase == eCylinderPhase_RaiseHeight)
    {
        m_CylinderParameter.m_Height = aznumeric_cast<float>(s_HeightManipulator.UpdateHeight(GetWorldTM(), view, point));
        if (m_CylinderParameter.m_Height < kInitialPrimitiveHeight)
        {
            m_CylinderParameter.m_Height = 0;
        }
        if (nFlags & MK_SHIFT)
        {
            CD::PolygonPtr pAlignedPolygon = s_SnappingHelper.FindAlignedPolygon(m_pCapPolygon, GetWorldTM(), view, point);
            if (pAlignedPolygon)
            {
                m_CylinderParameter.m_Height = aznumeric_cast<float>(GetPlane().Distance() - pAlignedPolygon->GetPlane().Distance());
            }
        }
        UpdateHeightWithBoundaryCheck(m_CylinderParameter.m_Height);
        GetPanel()->Update();
    }
}

void CylinderTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_CylinderPhase == eCylinderPhase_PlaceFirstPoint)
    {
        if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
        {
            SetStartSpot(GetCurrentSpot());
            m_CylinderPhase = eCylinderPhase_Radius;
            SetPlane(GetCurrentSpot().m_Plane);
            m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
            GetIEditor()->BeginUndo();
            GetModel()->RecordUndo("Designer : Create a Cylinder", GetBaseObject());
            SetTempPolygon(GetCurrentSpot().m_pPolygon);
            StoreSeparateStatus();
        }
    }
    else if (m_CylinderPhase == eCylinderPhase_Radius)
    {
        if (GetTempPolygon())
        {
            BrushVec3 localRaySrc, localRayDir;
            CD::GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);
            BrushVec3 vHit;
            if (GetTempPolygon()->GetPlane().HitTest(localRaySrc, localRaySrc + localRayDir, NULL, &vHit))
            {
                s_HeightManipulator.Init(GetTempPolygon()->GetPlane(), vHit);
            }
        }
        else if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
        {
            s_HeightManipulator.Init(GetPlane(), GetCurrentSpotPos());
        }
        s_SnappingHelper.Init(GetModel());
        UpdateHeight(0);
        if (m_pCapPolygon)
        {
            s_SnappingHelper.SearchForOppositePolygons(m_pCapPolygon);
        }
        m_bIsOverOpposite = false;
        m_CylinderPhase = eCylinderPhase_RaiseHeight;
    }
    else if (m_CylinderPhase == eCylinderPhase_RaiseHeight)
    {
        m_CylinderPhase = eCylinderPhase_Done;
    }
}

void CylinderTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_CylinderPhase == eCylinderPhase_Done)
    {
        m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
        FreezeModel();
        GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
    }
}

bool CylinderTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_CylinderPhase == eCylinderPhase_Radius || m_CylinderPhase == eCylinderPhase_RaiseHeight)
        {
            CancelCreation();
        }
        else if (m_CylinderPhase == eCylinderPhase_Done)
        {
            FreezeModel();
            m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
            GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void CylinderTool::Display(DisplayContext& dc)
{
    if (!GetModel() || !GetBaseObject())
    {
        return;
    }

    DisplayCurrentSpot(dc);

    if (m_CylinderPhase == eCylinderPhase_Radius || m_CylinderPhase == eCylinderPhase_RaiseHeight || m_CylinderPhase == eCylinderPhase_Done)
    {
        DrawIntermediatePolygon(dc);
    }

    if (m_CylinderPhase == eCylinderPhase_Radius && GetIntermediatePolygon())
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

    s_HeightManipulator.Display(dc);
}

void CylinderTool::UpdateBasePolygon(float fRadius, int nNumOfSubdivision)
{
    std::vector<BrushVec2> vertices2D;
    CD::MakeSectorOfCircle(fRadius, m_vCenterOnPlane, m_fAngle, CD::PI2, nNumOfSubdivision + 1, vertices2D);
    vertices2D.erase(vertices2D.begin());
    CD::STexInfo texInfo = GetTexInfo();
    if (GetIntermediatePolygon())
    {
        *GetIntermediatePolygon() = CD::Polygon(vertices2D, GetPlane(), GetMatID(), &texInfo, true);
    }
    else
    {
        SetIntermediatePolygon(new CD::Polygon(vertices2D, GetPlane(), GetMatID(), &texInfo, true));
    }
    m_pBasePolygon = GetIntermediatePolygon()->Flip();
}

void CylinderTool::UpdateHeightWithBoundaryCheck(BrushFloat fHeight)
{
    UpdateHeight(aznumeric_cast<float>(fHeight));
    m_bIsOverOpposite = m_pCapPolygon && s_SnappingHelper.IsOverOppositePolygon(m_pCapPolygon, CD::ePP_Pull);
    if (m_bIsOverOpposite)
    {
        fHeight = s_SnappingHelper.GetNearestDistanceToOpposite(CD::ePP_Pull);
        UpdateHeight(aznumeric_cast<float>(fHeight));
    }
}

void CylinderTool::Update()
{
    UpdateBasePolygon(m_CylinderParameter.m_Radius, m_CylinderParameter.m_NumOfSubdivision);
    if (m_CylinderPhase == eCylinderPhase_Done)
    {
        UpdateHeightWithBoundaryCheck(m_CylinderParameter.m_Height);
    }
}

void CylinderTool::UpdateHeight(float fHeight)
{
    if (m_CylinderPhase == eCylinderPhase_PlaceFirstPoint)
    {
        return;
    }

    DESIGNER_ASSERT(m_pBasePolygon);
    if (!m_pBasePolygon)
    {
        return;
    }

    std::vector<CD::PolygonPtr> polygonList;
    PrimitiveShape bp;
    bp.CreateCylinder(m_pBasePolygon, fHeight, &polygonList);

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);
    GetModel()->Clear();
    for (int i = 0, iPolygonCount(polygonList.size()); i < iPolygonCount; ++i)
    {
        if (GetTempPolygon())
        {
            polygonList[i]->SetTexInfo(GetTempPolygon()->GetTexInfo());
        }
        CD::AddPolygonWithSubMatID(polygonList[i]);
        if (CD::IsEquivalent(polygonList[i]->GetPlane().Normal(), GetPlane().Normal()))
        {
            m_pCapPolygon = polygonList[i];
        }
    }

    UpdateAll(CD::eUT_Mirror);
    UpdateShelf(1);
}

void CylinderTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (m_CylinderPhase == eCylinderPhase_Done)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Cylinder");
            FreezeModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
    }
}

void CylinderTool::FreezeModel()
{
    if (m_bIsOverOpposite)
    {
        s_SnappingHelper.ApplyOppositePolygons(m_pCapPolygon, CD::ePP_Pull);
    }
    ShapeTool::FreezeModel();
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Cylinder, CD::eToolGroup_Shape, "Cylinder", CylinderTool, PropertyTreePanel<CylinderTool>)