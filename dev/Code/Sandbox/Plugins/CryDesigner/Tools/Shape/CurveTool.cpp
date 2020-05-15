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
#include "CurveTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "ViewManager.h"

void CurveTool::Leave()
{
    ResetAllSpots();
    m_ArcState = eArcState_ChooseFirstPoint;

    PolylineTool::Leave();
}

bool CurveTool::IsPhaseFirstStepOnPrimitiveCreation() const
{
    return GetSpotListCount() == 0 && m_ArcState == eArcState_ChooseFirstPoint;
}

void CurveTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    if (m_ArcState == eArcState_ChooseFirstPoint)
    {
        if (GetModel()->IsEmpty() && GetSpotListCount() == 0)
        {
            ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false);
            SSpot spot = GetCurrentSpot();
            spot.m_Plane.Set(spot.m_Plane.Normal(), -spot.m_Plane.Normal().Dot(spot.m_Pos));
            SetPlane(spot.m_Plane);
            SetCurrentSpot(spot);
        }

        SetStartSpot(GetCurrentSpot());
        DESIGNER_ASSERT(GetStartSpot().m_PosState != eSpotPosState_Invalid);
        m_ArcState = eArcState_ChooseLastPoint;
        ResetCurrentSpot();
    }
    else if (m_ArcState == eArcState_ChooseLastPoint)
    {
        m_LastSpot = GetCurrentSpot();
        ResetCurrentSpot();
        PrepareBeizerSpots(view, nFlags, point);
        m_ArcState = eArcState_ControlMiddlePoint;
    }
    else if (m_ArcState == eArcState_ControlMiddlePoint)
    {
        CUndo undo("Designer : Register Arc");
        GetModel()->RecordUndo("Designer:Arc", GetBaseObject());

        SpotList spotList;
        int iSpotListSize(GetSpotListCount());
        for (int i = 0; i < iSpotListSize - 1; ++i)
        {
            const SSpot& spot0 = GetSpot(i);
            const SSpot& spot1 = GetSpot(i + 1);
            std::vector<CD::IntersectionPair> intersections;
            GetModel()->QueryIntersectionByEdge(BrushEdge3D(spot0.m_Pos, spot1.m_Pos), intersections);
            spotList.push_back(spot0);
            for (int k = 0, iSize(intersections.size()); k < iSize; ++k)
            {
                if (CD::IsEquivalent(intersections[k].second, spot0.m_Pos) || CD::IsEquivalent(intersections[k].second, spot1.m_Pos))
                {
                    continue;
                }
                spotList.push_back(SSpot(intersections[k].second, eSpotPosState_Edge, intersections[k].first));
            }
        }

        if (iSpotListSize - 1 >= 0 && iSpotListSize - 1 < GetSpotList().size())
        {
            spotList.push_back(GetSpot(iSpotListSize - 1));
        }

        ReplaceSpotList(spotList);
        RegisterSpotListAfterBreaking();
        RegisterEitherEndSpotList();
        CD::UpdateMirroredPartWithPlane(GetModel(), GetPlane());
        ResetAllSpots();
        UpdateAll(CD::eUT_ExceptMirror);
        m_ArcState = eArcState_ChooseFirstPoint;
    }
}

void CurveTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_ArcState == eArcState_ControlMiddlePoint)
    {
        if (nFlags & MK_SHIFT)
        {
            PrepareBeizerSpots(view, nFlags, point);
        }
        else
        {
            PrepareArcSpots(view, nFlags, point);
        }
    }
    else
    {
        bool bKeepInitialPlane = (m_ArcState != eArcState_ChooseFirstPoint);
        if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, bKeepInitialPlane))
        {
            return;
        }

        SetPlane(GetCurrentSpot().m_Plane);

        if (m_ArcState != eArcState_ChooseLastPoint)
        {
            return;
        }

        if (nFlags & MK_SHIFT)
        {
            m_LineState = eLineState_Diagonal;
        }
        else
        {
            BrushVec3 outPos;
            m_LineState = GetAlienedPointWithAxis(GetStartSpotPos(), GetCurrentSpotPos(), GetPlane(), view, NULL, outPos);
            SetCurrentSpotPos(outPos);
        }
    }
}

void CurveTool::PrepareArcSpots(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, true))
    {
        return;
    }

    BrushVec3 crossPoint = GetCurrentSpotPos();
    BrushVec2 vCrossPointOnPlane(GetPlane().W2P(crossPoint));
    BrushVec2 vFirstPointOnPlane(GetPlane().W2P(GetStartSpotPos()));
    BrushVec2 vLastPointOnPlane(GetPlane().W2P(m_LastSpot.m_Pos));

    int nEdgeCount = m_CurveParameter.m_NumOfSubdivision;
    std::vector<BrushVec2> arcLastVertexList;
    if (CD::MakeListConsistingOfArc(vCrossPointOnPlane, vFirstPointOnPlane, vLastPointOnPlane, nEdgeCount, arcLastVertexList) && !arcLastVertexList.empty())
    {
        ClearSpotList();
        AddSpotToSpotList(m_LastSpot);
        for (int i = 0; i < nEdgeCount - 1; ++i)
        {
            AddSpotToSpotList(SSpot(GetPlane().P2W(arcLastVertexList[i]), eSpotPosState_InPolygon, GetPlane()));
        }
        AddSpotToSpotList(GetStartSpot());
    }
}

void CurveTool::PrepareBeizerSpots(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, true))
    {
        return;
    }

    BrushVec3 crossPoint = GetCurrentSpotPos();
    BrushVec2 firstPointOnPlane(GetPlane().W2P(GetStartSpotPos()));
    BrushVec2 lastPointOnPlane(GetPlane().W2P(m_LastSpot.m_Pos));

    BrushEdge edge(firstPointOnPlane, lastPointOnPlane);
    BrushLine line(edge.m_v[0], edge.m_v[1]);
    BrushFloat distance = line.Distance(BrushVec2(crossPoint.x, crossPoint.z));
    edge.m_v[0] += line.m_Normal * distance;
    edge.m_v[1] += line.m_Normal * distance;

    BrushFloat t(0.3f);
    BrushVec2 middlePointOnPlane0 = edge.m_v[0] * (1 - t) + edge.m_v[1] * t;
    BrushVec2 middlePointOnPlane1 = edge.m_v[0] * t + edge.m_v[1] * (1 - t);

    int nEdgeCount = m_CurveParameter.m_NumOfSubdivision;

    ClearSpotList();
    AddSpotToSpotList(GetStartSpot());
    for (int i = 1; i <= nEdgeCount - 1; ++i)
    {
        BrushFloat t = (BrushFloat)i / (BrushFloat)nEdgeCount;
        // Calculation of cubic bezier curve as t.
        SSpot spot;
        spot.m_Pos = GetPlane().P2W(
                (1 - t) * (1 - t) * (1 - t) * firstPointOnPlane +
                3 * (1 - t) * (1 - t) * t * middlePointOnPlane0 +
                3 * (1 - t) * t * t * middlePointOnPlane1 +
                t * t * t * lastPointOnPlane);
        spot.m_Plane = GetPlane();
        AddSpotToSpotList(spot);
    }
    AddSpotToSpotList(m_LastSpot);
}

void CurveTool::Display(DisplayContext& dc)
{
    float oldThickness = dc.GetLineWidth();

    if (m_ArcState == eArcState_ChooseFirstPoint || m_ArcState == eArcState_ChooseLastPoint)
    {
        dc.SetFillMode(e_FillModeSolid);
        DrawCurrentSpot(dc, GetWorldTM());
    }

    if (m_ArcState == eArcState_ControlMiddlePoint)
    {
        if (GetSpotListCount())
        {
            dc.SetColor(CD::PolygonLineColor);
            DrawPolyline(dc);
        }
    }
    else if (m_ArcState == eArcState_ChooseLastPoint)
    {
        if (m_LineState == eLineState_ParallelToAxis)
        {
            dc.SetColor(CD::PolygonParallelToAxis);
        }
        else
        {
            dc.SetColor(CD::PolygonLineColor);
        }
        dc.DrawLine(GetStartSpotPos(), GetCurrentSpotPos());
    }

    dc.SetLineWidth(oldThickness);
}

bool CurveTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        return CD::GetDesigner()->EndCurrentEditSession();
    }

    return true;
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Curve, CD::eToolGroup_Shape, "Curve", CurveTool, PropertyTreePanel<CurveTool>)
