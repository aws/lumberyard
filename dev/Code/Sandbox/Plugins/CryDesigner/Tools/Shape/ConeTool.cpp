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
#include "ConeTool.h"
#include "Core/Model.h"
#include "Util/PrimitiveShape.h"
#include "Tools/DesignerTool.h"
#include "Util/HeightManipulator.h"
#include "ViewManager.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

void ConeTool::Enter()
{
    ShapeTool::Enter();
    m_ConePhase = eConePhase_PlaceFirstPoint;
}

void ConeTool::Leave()
{
    if (m_ConePhase == eConePhase_Done)
    {
        FreezeModel();
        GetIEditor()->AcceptUndo("Designer : Create a Cone");
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

void ConeTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    bool bKeepInitPlane = m_ConePhase == eConePhase_Radius ? true : false;
    bool bSearchAllShelves = m_ConePhase == eConePhase_Done ? true : false;

    if (m_ConePhase == eConePhase_PlaceFirstPoint || m_ConePhase == eConePhase_Radius)
    {
        if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, bKeepInitPlane, bSearchAllShelves))
        {
            return;
        }
    }

    if (m_ConePhase == eConePhase_Radius)
    {
        BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());
        m_ConeParameter.m_Radius = (vSpotPos2D - m_vCenterOnPlane).GetLength();
        const BrushFloat kSmallestRadius = 0.05f;
        if (m_ConeParameter.m_Radius < kSmallestRadius)
        {
            m_ConeParameter.m_Radius = kSmallestRadius;
        }
        m_fAngle = CD::ComputeAnglePointedByPos(m_vCenterOnPlane, vSpotPos2D);
        UpdateBasePolygon(m_ConeParameter.m_Radius, m_ConeParameter.m_NumOfSubdivision);
        GetPanel()->Update();
    }
    else if (m_ConePhase == eConePhase_RaiseHeight)
    {
        m_ConeParameter.m_Height = s_HeightManipulator.UpdateHeight(GetWorldTM(), view, point);
        if (m_ConeParameter.m_Height < (BrushFloat)0.01f)
        {
            m_ConeParameter.m_Height = (BrushFloat)0.01f;
        }
        UpdateCone(m_ConeParameter.m_Height);
        GetPanel()->Update();
    }
}

void ConeTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_ConePhase == eConePhase_PlaceFirstPoint)
    {
        if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
        {
            SetStartSpot(GetCurrentSpot());
            m_ConePhase = eConePhase_Radius;
            SetPlane(GetCurrentSpot().m_Plane);
            m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
            s_HeightManipulator.Init(GetPlane(), GetCurrentSpotPos());
            GetIEditor()->BeginUndo();
            GetModel()->RecordUndo("Designer : Create a Cone", GetBaseObject());
            SetTempPolygon(GetCurrentSpot().m_pPolygon);
            StoreSeparateStatus();
        }
    }
    else if (m_ConePhase == eConePhase_Radius)
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
        m_ConePhase = eConePhase_RaiseHeight;
    }
    else if (m_ConePhase == eConePhase_RaiseHeight)
    {
        m_ConePhase = eConePhase_Done;
    }
}

void ConeTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_ConePhase == eConePhase_Done)
    {
        m_ConePhase = eConePhase_PlaceFirstPoint;
        FreezeModel();
        GetIEditor()->AcceptUndo("Designer : Create a Cone");
    }
}

bool ConeTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_ConePhase == eConePhase_Radius || m_ConePhase == eConePhase_RaiseHeight)
        {
            CancelCreation();
        }
        else if (m_ConePhase == eConePhase_Done)
        {
            FreezeModel();
            m_ConePhase = eConePhase_PlaceFirstPoint;
            GetIEditor()->AcceptUndo("Designer : Create a Cone");
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void ConeTool::Display(DisplayContext& dc)
{
    DisplayCurrentSpot(dc);

    if (m_ConePhase == eConePhase_Radius || m_ConePhase == eConePhase_RaiseHeight || m_ConePhase == eConePhase_Done)
    {
        DrawIntermediatePolygon(dc);
    }

    if (m_ConePhase == eConePhase_Radius && GetIntermediatePolygon())
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

void ConeTool::UpdateBasePolygon(float fRadius, int nNumOfSubdivision)
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

void ConeTool::Update()
{
    UpdateBasePolygon(m_ConeParameter.m_Radius, m_ConeParameter.m_NumOfSubdivision);
    if (m_ConePhase == eConePhase_Done)
    {
        UpdateCone(m_ConeParameter.m_Height);
    }
}

void ConeTool::UpdateCone(float fHeight)
{
    if (m_ConePhase == eConePhase_PlaceFirstPoint)
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
    bp.CreateCone(m_pBasePolygon, fHeight, &polygonList);

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
        GetModel()->AddPolygon(polygonList[i], CD::eOpType_Add);
    }

    UpdateAll(CD::eUT_Mirror);
    UpdateShelf(1);
}

void ConeTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (m_ConePhase == eConePhase_Done)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Cone");
            FreezeModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        m_ConePhase = eConePhase_PlaceFirstPoint;
    }
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Cone, CD::eToolGroup_Shape, "Cone", ConeTool, PropertyTreePanel<ConeTool>)