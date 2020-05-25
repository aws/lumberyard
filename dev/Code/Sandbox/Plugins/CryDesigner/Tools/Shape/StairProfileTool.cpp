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
#include "StairProfileTool.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "Viewport.h"
#include "Tools/DesignerTool.h"
#include "Core/BrushHelper.h"

void StairProfileTool::Enter()
{
    ShapeTool::Enter();
    m_SideStairMode = eSideStairMode_PlaceFirstPoint;
}

void StairProfileTool::Display(DisplayContext& dc)
{
    if (m_SideStairMode == eSideStairMode_PlaceFirstPoint || m_SideStairMode == eSideStairMode_DrawDiagonal)
    {
        DrawCurrentSpot(dc, GetWorldTM());
        if (m_SideStairMode == eSideStairMode_DrawDiagonal)
        {
            dc.SetColor(CD::PolygonLineColor);
            dc.DrawLine(GetCurrentSpotPos(), GetStartSpotPos());
        }
    }
    else if (m_SideStairMode == eSideStairMode_SelectDirection)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (m_nSelectedCandidate == i)
            {
                dc.SetLineWidth(CD::kChosenLineThickness);
                DrawCandidateStair(dc, i, CD::PolygonLineColor);
            }
            else
            {
                dc.SetLineWidth(CD::kLineThickness);
                DrawCandidateStair(dc, i, ColorB(100, 100, 100, 255));
            }
        }
    }
}

void StairProfileTool::DrawCandidateStair(DisplayContext& dc, int nIndex, const ColorB& color)
{
    if (m_CandidateStairs[nIndex].empty())
    {
        return;
    }

    dc.SetColor(color);

    int nSize = m_CandidateStairs[nIndex].size();
    if (nSize == 0)
    {
        return;
    }
    std::vector<Vec3> vList;
    for (int i = 0; i < nSize; ++i)
    {
        vList.push_back(m_CandidateStairs[nIndex][i].m_Pos);
    }
    if (vList.size() >= 2)
    {
        dc.DrawPolyLine(&vList[0], vList.size(), false);
    }
}

bool StairProfileTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_SideStairMode == eSideStairMode_SelectDirection)
        {
            SetCurrentSpot(GetStartSpot());
            m_SideStairMode = eSideStairMode_DrawDiagonal;
        }
        else if (m_SideStairMode == eSideStairMode_DrawDiagonal)
        {
            m_SideStairMode = eSideStairMode_PlaceFirstPoint;
            ResetAllSpots();
        }
        else if (m_SideStairMode == eSideStairMode_PlaceFirstPoint)
        {
            return CD::GetDesigner()->EndCurrentEditSession();
        }
    }

    return true;
}

void StairProfileTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_SideStairMode == eSideStairMode_PlaceFirstPoint)
    {
        SetStartSpot(GetCurrentSpot());
        m_SideStairMode = eSideStairMode_DrawDiagonal;
    }
    else if (m_SideStairMode == eSideStairMode_DrawDiagonal)
    {
        CreateCandidates();
        m_SideStairMode = eSideStairMode_SelectDirection;
    }
    else if (m_SideStairMode == eSideStairMode_SelectDirection)
    {
        CUndo undo("Designer : Add Stair");
        GetModel()->RecordUndo("Add Stair", GetBaseObject());

        std::vector<SSpot>& selectedStairs = m_CandidateStairs[m_nSelectedCandidate];
        int nStairSize = selectedStairs.size();

        DESIGNER_ASSERT(nStairSize >= 3);

        if (nStairSize >= 3)
        {
            if (!(nFlags & MK_SHIFT))
            {
                BrushVec2 v0 = GetPlane().W2P(selectedStairs[0].m_Pos);
                BrushVec2 v1 = GetPlane().W2P(selectedStairs[1].m_Pos);
                BrushVec2 v2 = GetPlane().W2P(selectedStairs[2].m_Pos);

                BrushVec2 v3 = GetPlane().W2P(selectedStairs[nStairSize - 3].m_Pos);
                BrushVec2 v4 = GetPlane().W2P(selectedStairs[nStairSize - 2].m_Pos);
                BrushVec2 v5 = GetPlane().W2P(selectedStairs[nStairSize - 1].m_Pos);

                BrushLine l0(v0, v0 + (v1 - v2));
                BrushLine l1(v5, v5 + (v4 - v3));

                BrushVec2 intersection;
                if (l0.Intersect(l1, intersection))
                {
                    selectedStairs.push_back(SSpot(GetPlane().P2W(intersection)));
                    std::vector<BrushVec3> vList;
                    GenerateVertexListFromSpotList(selectedStairs, vList);
                    auto texInfo = GetTexInfo();
                    CD::PolygonPtr pPolygon = new CD::Polygon(vList, GetPlane(), GetMatID(), &texInfo, true);
                    pPolygon->ModifyOrientation();
                    pPolygon->SetMaterialID(CD::GetDesigner()->GetCurrentSubMatID());
                    GetModel()->AddPolygon(pPolygon, CD::eOpType_Split);
                }
            }
            else
            {
                RegisterSpotList(GetModel(), selectedStairs);
            }
        }

        CD::UpdateMirroredPartWithPlane(GetModel(), GetPlane());
        ResetAllSpots();
        UpdateAll(CD::eUT_ExceptMirror);
        m_SideStairMode = eSideStairMode_PlaceFirstPoint;
    }
}

void StairProfileTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_SideStairMode == eSideStairMode_SelectDirection)
    {
        BrushVec3 localRaySrc, localRayDir;
        CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);

        BrushVec3 vPosition;
        if (!GetModel()->QueryPosition(GetPlane(), localRaySrc, localRayDir, vPosition))
        {
            return;
        }

        DESIGNER_ASSERT(!m_CandidateStairs[0].empty());
        if (!m_CandidateStairs[0].empty())
        {
            BrushFloat dist2Pos = m_BorderLine.Distance(GetPlane().W2P(vPosition));
            BrushFloat dist2Candidate0 = m_BorderLine.Distance(GetPlane().W2P(m_CandidateStairs[0][1].m_Pos));

            if (dist2Pos * dist2Candidate0 > 0)
            {
                m_nSelectedCandidate = 0;
            }
            else
            {
                m_nSelectedCandidate = 1;
            }
        }
    }
    else
    {
        if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, m_SideStairMode == eSideStairMode_DrawDiagonal))
        {
            return;
        }
        SetPlane(GetCurrentSpot().m_Plane);
    }
}

void StairProfileTool::CreateCandidates()
{
    int nStepNumber = 0;
    float fStepRise = m_StairProfileParameter.m_StepRise;
    CD::EStairHeightCalculationWayMode heightMode = CD::eStairHeightCalculationWay_StepRise;

    BrushVec2 vStartPoint = GetPlane().W2P(GetStartSpotPos());
    BrushVec2 vEndPoint = GetPlane().W2P(GetCurrentSpotPos());
    if (vStartPoint.y > vEndPoint.y)
    {
        std::swap(vStartPoint, vEndPoint);
        SwapCurrentAndStartSpots();
    }
    BrushVec2 vShrinkedEndPoint = vEndPoint;

    if (heightMode == CD::eStairHeightCalculationWay_StepRise)
    {
        BrushVec3 vX3D = GetWorldTM().TransformVector(GetPlane().P2W(BrushVec2(1, 0)));
        BrushVec3 vY3D = GetWorldTM().TransformVector(GetPlane().P2W(BrushVec2(0, 1)));
        const BrushVec3 vZ(0, 0, 1);
        int nElement = std::abs(vZ.Dot(vX3D)) > std::abs(vZ.Dot(vY3D)) ? 0 : 1;

        BrushFloat fFullRise = std::abs(vEndPoint[nElement] - vStartPoint[nElement]);
        BrushFloat fFullLength = (vEndPoint - vStartPoint).GetLength();

        nStepNumber = aznumeric_cast<int>(fFullRise / fStepRise);
        vShrinkedEndPoint = vStartPoint + ((vEndPoint - vStartPoint).GetNormalized() * ((fStepRise * fFullLength) / fFullRise)) * nStepNumber;
    }

    BrushVec2 vStart2End = vShrinkedEndPoint - vStartPoint;

    if (fabs(vStart2End.x) < kDesignerEpsilon || fabs(vStart2End.y) < kDesignerEpsilon)
    {
        return;
    }

    m_BorderLine = BrushLine(vStartPoint, vEndPoint);

    BrushVec2 vOneStep = vStart2End / nStepNumber;
    BrushVec2 vCurrentShotDir(0, 1);
    BrushVec2 vNextShotDir(1, 0);

    for (int k = 0; k < 2; ++k)
    {
        BrushVec2 vCurrentPoint = vStartPoint;
        BrushVec2 vNextPoint = vCurrentPoint;

        m_CandidateStairs[k].clear();
        m_CandidateStairs[k].push_back(Convert2Spot(GetModel(), GetPlane().P2W(vCurrentPoint)));

        for (int i = 0; i < nStepNumber + 1; ++i)
        {
            vNextPoint += vOneStep;
            if (i == nStepNumber)
            {
                if (heightMode == CD::eStairHeightCalculationWay_StepNumber)
                {
                    break;
                }
                vNextPoint = vEndPoint;
            }

            BrushLine currentLine(vCurrentPoint, vCurrentPoint + vCurrentShotDir);
            BrushLine nextLine(vNextPoint, vNextPoint + vNextShotDir);

            BrushVec2 vIntersection;
            if (!currentLine.Intersect(nextLine, vIntersection))
            {
                GetIEditor()->CancelUndo();
                DESIGNER_ASSERT(0);
                return;
            }

            m_CandidateStairs[k].push_back(Convert2Spot(GetModel(), GetPlane().P2W(vIntersection)));
            m_CandidateStairs[k].push_back(Convert2Spot(GetModel(), GetPlane().P2W(vNextPoint)));

            vCurrentPoint = vNextPoint;
        }

        std::swap(vCurrentShotDir, vNextShotDir);

        m_CandidateStairs[k][0] = GetStartSpot();
        m_CandidateStairs[k][m_CandidateStairs[k].size() - 1] = GetCurrentSpot();
    }
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_StairProfile, CD::eToolGroup_Shape, "Stair Profile", StairProfileTool, PropertyTreePanel<StairProfileTool>)
