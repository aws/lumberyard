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
#include "PolylineTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "ViewManager.h"
#include "Grid.h"

using namespace CD;

void PolylineTool::Enter()
{
    ShapeTool::Enter();
    m_bHasValidRecentEdge = false;
}

void PolylineTool::Leave()
{
    Complete();
    ResetAllSpots();
    ShapeTool::Leave();
}

void PolylineTool::RegisterSpotListAfterBreaking()
{
    int nFirstSpotIndexOnEdge = -1;
    for (int i = 0, iSpotSize(GetSpotListCount()); i < iSpotSize; ++i)
    {
        const SSpot& spot = GetSpot(i);
        if (spot.m_bProcessed)
        {
            continue;
        }
        if (spot.IsOnEdge())
        {
            if (nFirstSpotIndexOnEdge == -1)
            {
                nFirstSpotIndexOnEdge = i;
            }
            else
            {
                SpotList spotList;
                for (int k = nFirstSpotIndexOnEdge; k <= i; ++k)
                {
                    spotList.push_back(GetSpot(k));
                    SetSpotProcessed(k, true);
                }

                SetSpotProcessed(i, false);
                GetModel()->RecordUndo("Add Polygon", GetBaseObject());

                RegisterSpotList(GetModel(), spotList);
                UpdateMirroredPartWithPlane(GetModel(), GetPlane());
                nFirstSpotIndexOnEdge = i;
            }
        }
    }
}

void PolylineTool::RegisterEitherEndSpotList()
{
    if (GetSpotListCount() == 0)
    {
        return;
    }
    bool bAddedLastSpot = false;
    if (GetSpot(0).m_bProcessed == false)
    {
        for (int i = 1; i < GetSpotListCount(); ++i)
        {
            if (GetSpot(i).IsOnEdge() || i == GetSpotListCount() - 1)
            {
                bAddedLastSpot = i == GetSpotListCount() - 1;
                SpotList spotList;
                for (int k = 0; k <= i; ++k)
                {
                    spotList.push_back(GetSpot(k));
                }
                RegisterSpotList(GetModel(), spotList);
                break;
            }
        }
    }

    if (!bAddedLastSpot && !GetSpot(GetSpotListCount() - 1).IsOnEdge())
    {
        for (int i = GetSpotListCount() - 2; i >= 0; --i)
        {
            if (GetSpot(i).IsOnEdge())
            {
                SpotList spotList;
                for (int k = i; k < GetSpotListCount(); ++k)
                {
                    spotList.push_back(GetSpot(k));
                }
                RegisterSpotList(GetModel(), spotList);
                break;
            }
        }
    }
}

void PolylineTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_bAlignedToRecentSpotLine = false;
    m_bAlignedToAnotherEdge = false;
    SetLineState(eLineState_Diagonal);
}

void PolylineTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_bHasValidRecentEdge = false;

    if (GetSpotListCount() == 1 && IsEquivalent(GetSpotPos(0), GetCurrentSpotPos()))
    {
        return;
    }

    if (GetModel()->IsEmpty() && GetSpotListCount() == 0)
    {
        ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false);
        SSpot spot = GetCurrentSpot();
        spot.m_Plane.Set(spot.m_Plane.Normal(), -spot.m_Plane.Normal().Dot(spot.m_Pos));
        SetPlane(spot.m_Plane);
        SetCurrentSpot(spot);
    }

    PutCurrentSpot();
}

void PolylineTool::AddPolygonWithCurrentSameAsFirst()
{
    if (GetModel())
    {
        GetModel()->RecordUndo("Add Polygon", GetBaseObject());
    }

    int nFirstSpotIndex = GetSpotListCount() - 1;
    for (; nFirstSpotIndex >= 0; --nFirstSpotIndex)
    {
        if (GetSpot(nFirstSpotIndex).IsOnEdge() && !GetSpot(nFirstSpotIndex).m_bProcessed)
        {
            break;
        }
    }

    if (nFirstSpotIndex == -1)
    {
        CreatePolygonFromSpots(true, GetSpotList());
    }
    else
    {
        std::vector<SSpot> spotList;
        int nSpotIndex = nFirstSpotIndex;
        do
        {
            const SSpot& spot = GetSpot(nSpotIndex % GetSpotListCount());
            spotList.push_back(spot);
        } while (!GetSpot((++nSpotIndex) % GetSpotListCount()).IsOnEdge());
        spotList.push_back(GetSpot(nSpotIndex % GetSpotListCount()));
        RegisterSpotList(GetModel(), spotList);
    }
    ResetAllSpots();
    UpdateAll(eUT_ExceptMirror);
}

void PolylineTool::PutCurrentSpot()
{
    if (GetLineState() == eLineState_Cross)
    {
        return;
    }

    if (GetSpotListCount() == 0)
    {
        SetStartSpot(GetCurrentSpot());
        DESIGNER_ASSERT(GetStartSpot().m_PosState != eSpotPosState_Invalid);
    }

    CUndo undo("Designer : Add Polygon");

    if (GetSpotListCount() > 0 && GetModel())
    {
        const BrushVec3& lastPos = GetSpotPos(GetSpotListCount() - 1);
        const BrushVec3& currPos = GetCurrentSpot().m_Pos;

        const SSpot& lastSpot = GetSpot(GetSpotListCount() - 1);
        if (lastSpot.IsInPolygon() && GetCurrentSpot().IsInPolygon() && !GetCurrentSpot().m_Plane.IsEquivalent(lastSpot.m_Plane))
        {
            return;
        }

        std::vector<IntersectionPair> intersections;
        GetModel()->QueryIntersectionByEdge(BrushEdge3D(lastPos, currPos), intersections);
        for (int i = 0, iSize(intersections.size()); i < iSize; ++i)
        {
            if (intersections[i].second.IsEquivalent(lastPos) || intersections[i].second.IsEquivalent(currPos))
            {
                continue;
            }
            AddSpotToSpotList(SSpot(intersections[i].second, eSpotPosState_Edge, intersections[i].first));
        }
    }

    AddSpotToSpotList(GetCurrentSpot());
    RegisterSpotListAfterBreaking();

    if (GetSpotListCount() > 1)
    {
        if (GetSpotListCount() > 2 && GetCurrentSpot().IsSamePos(GetSpot(0)) && !GetSpot(0).m_bProcessed)
        {
            AddPolygonWithCurrentSameAsFirst();
        }
    }

    ResetCurrentSpotWeakly();
    m_RecentSpotOnEdge.Reset();
}

void PolylineTool::CreatePolygonFromSpots(bool bClosePolygon, const SpotList& spotList)
{
    std::vector<BrushVec3> vList;
    GenerateVertexListFromSpotList(spotList, vList);

    STexInfo texInfo = GetTexInfo();
    SetIntermediatePolygon(new CD::Polygon(vList, GetPlane(), GetMatID(), &texInfo, bClosePolygon));
    if (GetIntermediatePolygon())
    {
        if (bClosePolygon)
        {
            GetIntermediatePolygon()->ModifyOrientation();
        }
        GetModel()->AddPolygon(GetIntermediatePolygon(), eOpType_Split);
        UpdateMirroredPartWithPlane(GetModel(), GetPlane());
    }
}

void PolylineTool::Complete()
{
    if (GetSpotListCount())
    {
        CUndo undo("Designer : Add Polygon");
        if (GetModel())
        {
            GetModel()->RecordUndo("Add Polygon", GetBaseObject());
        }
        SetEditMode(eEditMode_None);
        RegisterEitherEndSpotList();
        UpdateMirroredPartWithPlane(GetModel(), GetPlane());
        ResetAllSpots();
        UpdateAll(eUT_ExceptMirror);
    }
}

bool PolylineTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (GetSpotListCount())
        {
            Complete();
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }

    return true;
}

PolylineTool::ELineState PolylineTool::GetAlienedPointWithAxis(const BrushVec3& v0, const BrushVec3& v1, const BrushPlane& plane, IDisplayViewport* view, std::vector<BrushEdge3D>* pAxisList, BrushVec3& outPos) const
{
    BrushVec2 prevPosOnPlane = plane.W2P(v0);
    BrushVec2 curPosOnPlane = plane.W2P(v1);

    static const BrushFloat kMaxLength = 15.0f;
    ELineState lineState = eLineState_Diagonal;

    if (!pAxisList)
    {
        BrushLine vXAxisLine(prevPosOnPlane, prevPosOnPlane + BrushVec2(1, 0));
        BrushLine vYAxisLine(prevPosOnPlane, prevPosOnPlane + BrushVec2(0, 1));
        BrushVec2 vProjectedToXAxis;
        BrushVec2 vProjectedToYAxis;
        vXAxisLine.HitTest(curPosOnPlane, curPosOnPlane + vXAxisLine.m_Normal, NULL, &vProjectedToXAxis);
        vYAxisLine.HitTest(curPosOnPlane, curPosOnPlane + vYAxisLine.m_Normal, NULL, &vProjectedToYAxis);

        if (AreTwoPositionsNear(v1, plane.P2W(vProjectedToXAxis), GetWorldTM(), view, kLimitForMagnetic))
        {
            lineState = eLineState_ParallelToAxis;
            curPosOnPlane.y = prevPosOnPlane.y;
        }
        else if (AreTwoPositionsNear(v1, plane.P2W(vProjectedToYAxis), GetWorldTM(), view, kLimitForMagnetic))
        {
            lineState = eLineState_ParallelToAxis;
            curPosOnPlane.x = prevPosOnPlane.x;
        }
    }
    else
    {
        for (int i = 0, iSize(pAxisList->size()); i < iSize; ++i)
        {
            BrushVec2 vAxisV0 = plane.W2P((*pAxisList)[i].m_v[0]);
            BrushVec2 vAxisV1 = plane.W2P((*pAxisList)[i].m_v[1]);
            BrushVec2 customAxis = (vAxisV1 - vAxisV0).GetNormalized();
            BrushLine vAxisLine(prevPosOnPlane, prevPosOnPlane + customAxis);
            BrushVec2 vProjectedOnAxis;

            if (vAxisLine.HitTest(curPosOnPlane, curPosOnPlane + vAxisLine.m_Normal, NULL, &vProjectedOnAxis))
            {
                if (AreTwoPositionsNear(v1, plane.P2W(vProjectedOnAxis), GetWorldTM(), view, kLimitForMagnetic))
                {
                    lineState = eLineState_ParallelToAxis;
                    break;
                }
            }
        }
    }

    outPos = plane.P2W(curPosOnPlane);

    return lineState;
}

bool PolylineTool::IsPhaseFirstStepOnPrimitiveCreation() const
{
    return GetSpotListCount() == 0;
}

void PolylineTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!GetModel())
    {
        return;
    }

    m_bAlignedToRecentSpotLine = false;
    const float fMagneticSize = 22.0f;

    BrushVec3 localRaySrc, localRayDir;
    GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);
    int nPolygonIdx = 0;
    bool bPickedDesignerObject = GetModel()->QueryPolygon(localRaySrc, localRayDir, nPolygonIdx);
    bool bKeepInitialPlane = GetSpotListCount() > 0 && (GetSpot(0).m_pPolygon == NULL || !bPickedDesignerObject) ? true : false;

    if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, bKeepInitialPlane))
    {
        SetPlane(GetCurrentSpot().m_Plane);
        SetLineState(eLineState_Diagonal);

        bool bProceedFurther = true;

        if (nFlags == MK_LBUTTON)
        {
            if (GetCurrentSpot().IsAtEitherPointOnEdge() || GetCurrentSpot().IsCenterOfEdge())
            {
                if (!IsEquivalent(GetStartSpotPos(), GetCurrentSpotPos()))
                {
                    m_RecentSpotOnEdge = GetCurrentSpot();
                }
            }
            else if (GetCurrentSpot().IsOnEdge())
            {
                if (GetCurrentSpot().m_pPolygon && !m_bAlignedToAnotherEdge)
                {
                    BrushVec3 pos;
                    m_bHasValidRecentEdge = GetCurrentSpot().m_pPolygon->QueryNearestEdge(GetCurrentSpotPos(), m_RecentEdge, pos);
                }
            }

            if (m_RecentSpotOnEdge.IsOnEdge())
            {
                BrushVec3 outPos;
                ELineState lineState = GetAlienedPointWithAxis(m_RecentSpotOnEdge.m_Pos, GetCurrentSpotPos(), GetPlane(), view, NULL, outPos);
                if (lineState == eLineState_ParallelToAxis)
                {
                    m_bAlignedToRecentSpotLine = true;
                    SetCurrentSpotPos(outPos);
                    bProceedFurther = false;
                }
            }

            if (m_bHasValidRecentEdge && GetSpotListCount() > 0)
            {
                std::vector<BrushEdge3D> vAxisList;
                vAxisList.push_back(m_RecentEdge);
                BrushVec3 outPos;
                ELineState lineState = GetAlienedPointWithAxis(GetSpotPos(GetSpotListCount() - 1), GetCurrentSpotPos(), GetPlane(), view, &vAxisList, outPos);
                if (lineState == eLineState_ParallelToAxis)
                {
                    if (m_bAlignedToRecentSpotLine || GetCurrentSpot().IsOnEdge() && GetCurrentSpot().m_pPolygon)
                    {
                        BrushVec2 v0 = GetPlane().W2P(GetSpotPos(GetSpotListCount() - 1));
                        BrushVec2 v1 = GetPlane().W2P(GetCurrentSpotPos());
                        BrushVec2 vEdgeDir = (GetPlane().W2P(m_RecentEdge.m_v[1]) - GetPlane().W2P(m_RecentEdge.m_v[0])).GetNormalized();
                        BrushLine line(v0, v0 + vEdgeDir);
                        BrushVec2 hitPos2D;

                        BrushVec2 vTarget;
                        if (GetCurrentSpot().IsOnEdge() && GetCurrentSpot().m_pPolygon)
                        {
                            BrushEdge3D nearestEdge;
                            BrushVec3 posOnEdge;
                            if (GetCurrentSpot().m_pPolygon->QueryNearestEdge(GetCurrentSpotPos(), nearestEdge, posOnEdge))
                            {
                                vTarget = GetPlane().W2P(nearestEdge.m_v[1]);
                            }
                            else
                            {
                                vTarget = v1 + line.m_Normal;
                            }
                        }
                        else
                        {
                            vTarget = GetPlane().W2P(m_RecentSpotOnEdge.m_Pos);
                        }

                        if (line.HitTest(v1, vTarget, NULL, &hitPos2D))
                        {
                            outPos = GetPlane().P2W(hitPos2D);
                        }
                    }

                    SetLineState(lineState);
                    SetCurrentSpotPos(outPos);
                    bProceedFurther = false;
                    m_bAlignedToAnotherEdge = true;
                }
            }
        }

        if (bProceedFurther)
        {
            if (!GetCurrentSpot().IsAtEitherPointOnEdge() && !GetCurrentSpot().IsAtEndPoint())
            {
                bool bAlign = (nFlags & MK_SHIFT) ? false : true;
                AlignEdgeWithPrincipleAxises(view, bAlign);
            }
        }
    }
}

void PolylineTool::AlignEdgeWithPrincipleAxises(IDisplayViewport* view, bool bAlign)
{
    if (GetSpotListCount() == 0)
    {
        return;
    }

    int nSpotIndex = -1;
    bool bIntersectAgainstOtherEdges = IntersectExisintingLines(GetSpot(GetSpotListCount() - 1).m_Pos, GetCurrentSpotPos(), &nSpotIndex);
    bool bStartAndCurrentSame = AreTwoPositionsNear(GetCurrentSpotPos(), GetStartSpotPos(), GetWorldTM(), view, kLimitForMagnetic);

    if (bIntersectAgainstOtherEdges && (nSpotIndex > 0 || !bStartAndCurrentSame))
    {
        SetLineState(eLineState_Cross);
        return;
    }

    if (bAlign)
    {
        BrushVec3 outPos;
        ELineState lineState = GetAlienedPointWithAxis(GetSpot(GetSpotListCount() - 1).m_Pos, GetCurrentSpotPos(), GetPlane(), view, NULL, outPos);

        if (GetCurrentSpot().IsOnEdge())
        {
            std::vector<SQueryEdgeResult> queryResults;
            BrushVec3 nearestPos;
            if (GetModel()->QueryNearestEdges(GetPlane(), outPos, nearestPos, queryResults) && !queryResults.empty())
            {
                BrushVec2 lastSpotPos2D = GetPlane().W2P(GetSpot(GetSpotListCount() - 1).m_Pos);
                BrushVec2 currentSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());
                BrushVec2 pos2D = GetPlane().W2P(outPos);
                BrushLine line(GetPlane().W2P(queryResults[0].m_Edge.m_v[0]), GetPlane().W2P(queryResults[0].m_Edge.m_v[1]));
                if (line.HitTest(pos2D, pos2D + (lastSpotPos2D - currentSpotPos2D), NULL, &pos2D))
                {
                    outPos = GetPlane().P2W(pos2D);
                }
            }
        }

        SetLineState(lineState);
        SetCurrentSpotPos(outPos);
    }
}

bool PolylineTool::IntersectExisintingLines(const BrushVec3& v0, const BrushVec3& v1, int* pOutSpotIndex) const
{
    BrushVec2 vP0 = GetPlane().W2P(v0);
    BrushVec2 vP1 = GetPlane().W2P(v1);
    BrushEdge edgeP(vP0, vP1);
    BrushLine lineP(edgeP.m_v[0], edgeP.m_v[1]);

    for (int i = 0, iSpotSize(GetSpotListCount() - 1); i < iSpotSize; ++i)
    {
        if (GetSpot(i).m_bProcessed)
        {
            continue;
        }

        BrushVec2 vQ0 = GetPlane().W2P(GetSpot(i).m_Pos);
        BrushVec2 vQ1 = GetPlane().W2P(GetSpot(i + 1).m_Pos);
        BrushEdge edgeQ(vQ0, vQ1);
        BrushLine lineQ(edgeQ.m_v[0], edgeQ.m_v[1]);

        BrushFloat d0 = lineQ.Distance(edgeP.m_v[0]);
        BrushFloat d1 = lineQ.Distance(edgeP.m_v[1]);

        if (d0 > kDesignerEpsilon && d1 > kDesignerEpsilon || d0 < -kDesignerEpsilon && d1 < -kDesignerEpsilon)
        {
            continue;
        }

        if (d0 > -kDesignerEpsilon && d0 < kDesignerEpsilon || d1 > -kDesignerEpsilon && d1 < kDesignerEpsilon)
        {
            continue;
        }

        BrushVec2 intersection;
        if (lineQ.Intersect(lineP, intersection))
        {
            if ((intersection.x > edgeQ.m_v[0].x && intersection.x < edgeQ.m_v[1].x || intersection.x > edgeQ.m_v[1].x && intersection.x < edgeQ.m_v[0].x) &&
                (intersection.y > edgeQ.m_v[0].y && intersection.y < edgeQ.m_v[1].y || intersection.y > edgeQ.m_v[1].y && intersection.y < edgeQ.m_v[0].y))
            {
                if (pOutSpotIndex)
                {
                    *pOutSpotIndex = i;
                }
                return true;
            }
        }
    }

    return false;
}

void PolylineTool::Display(DisplayContext& dc)
{
    ShapeTool::Display(dc);

    float oldThickness = dc.GetLineWidth();
    dc.SetFillMode(e_FillModeSolid);
    DrawCurrentSpot(dc, GetWorldTM());

    dc.SetColor(PolygonLineColor);
    DrawPolyline(dc);

    if (GetSpotListCount())
    {
        if (GetLineState() == eLineState_Cross)
        {
            dc.SetColor(PolygonInvalidLineColor);
        }
        else if (GetLineState() == eLineState_ParallelToAxis)
        {
            dc.SetColor(PolygonParallelToAxis);
        }
        dc.DrawLine(GetSpot(GetSpotListCount() - 1).m_Pos, GetCurrentSpotPos());
    }

    if (m_bAlignedToRecentSpotLine)
    {
        dc.SetColor(ColorB(210, 210, 210, 255));
        dc.DrawLine(m_RecentSpotOnEdge.m_Pos, GetCurrentSpotPos());
        DrawSpot(dc, GetWorldTM(), m_RecentSpotOnEdge.m_Pos, ColorB(100, 100, 180, 255));
    }

    dc.SetLineWidth(oldThickness);
}

void PolylineTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    ShapeTool::OnEditorNotifyEvent(event);
    switch (event)
    {
    case eNotify_OnEndUndoRedo:
        ResetAllSpots();
        break;
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Line, CD::eToolGroup_Shape, "Polyline", PolylineTool)
