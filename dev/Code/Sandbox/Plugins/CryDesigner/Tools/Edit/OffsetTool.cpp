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
#include "OffsetTool.h"
#include "Viewport.h"
#include "ViewManager.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "Tools/DesignerTool.h"

void OffsetTool::Leave()
{
    m_pOffsetedPolygon = NULL;
    BaseTool::Leave();
}

void OffsetTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_fScale = 0;
    m_LButtonInfo.m_DownTimeStamp = GetTickCount();
    m_LButtonInfo.m_DownPos = point;
    m_LButtonInfo.m_LastAction = CD::eMouseAction_LButtonDown;

    m_Context.polygon = QueryOffsetPolygon(view, point);
    if (!m_Context.polygon)
    {
        return;
    }

    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);

    BrushVec3 pickedPos;
    GetModel()->QueryPosition(m_Context.polygon->GetPlane(), localRaySrc, localRayDir, pickedPos);

    BrushEdge3D nearestEdge;
    BrushVec3 nearestPos;

    std::vector<CD::PolygonPtr> outerPolygons;
    m_Context.polygon->GetSeparatedPolygons(outerPolygons, CD::eSP_OuterHull);
    if (!outerPolygons.empty() && outerPolygons[0]->QueryNearestEdge(pickedPos, nearestEdge, nearestPos))
    {
        m_Context.polygon->GetSeparatedPolygons(m_Context.holes, CD::eSP_InnerHull);

        BrushVec3 edgeDir = (nearestEdge.m_v[1] - nearestEdge.m_v[0]).GetNormalized();
        m_Context.origin = nearestPos;
        m_Context.plane = BrushPlane(nearestEdge.m_v[0], nearestEdge.m_v[1], nearestEdge.m_v[0] - GetPlane().Normal());
        m_Context.polygon = outerPolygons[0];
    }

    m_pOffsetedPolygon = m_Context.polygon->Clone();
    CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());

    CalculateOffset(view, point);
}

CD::PolygonPtr OffsetTool::QueryOffsetPolygon(CViewport* view, const QPoint& point) const
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);
    int nPolygonIndex = 0;
    if (GetModel()->QueryPolygon(localRaySrc, localRayDir, nPolygonIndex))
    {
        CD::PolygonPtr pPolygon = GetModel()->GetPolygon(nPolygonIndex);
        if (!pPolygon->IsOpen())
        {
            return pPolygon;
        }
    }
    return NULL;
}

void OffsetTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_LButtonInfo.m_LastAction = CD::eMouseAction_LButtonUp;

    if (!IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) && IsTwoPointEquivalent(point, m_LButtonInfo.m_DownPos))
    {
        m_pOffsetedPolygon = NULL;
    }
    else if (m_pOffsetedPolygon)
    {
        AddScaledPolygon(nFlags & MK_CONTROL);
        m_fPrevScale = m_fScale;
    }
}

void OffsetTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_pOffsetedPolygon == NULL || !(nFlags & MK_LBUTTON))
    {
        BrushVec3 localRaySrc, localRayDir;
        CD::GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);
        int nPolygonIndex(-1);
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
        if (GetModel()->QueryPolygon(localRaySrc, localRayDir, nPolygonIndex))
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(nPolygonIndex);
            if (!pPolygon->CheckFlags(CD::ePolyFlag_Mirrored) && !pPolygon->IsOpen())
            {
                CD::GetDesigner()->UpdateSelectionMesh(pPolygon, GetCompiler(), GetBaseObject());
                SetPlane(pPolygon->GetPlane());
            }
        }
    }
    else if (IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) || !IsTwoPointEquivalent(point, m_LButtonInfo.m_DownPos))
    {
        CalculateOffset(view, point);
    }
}

void OffsetTool::CalculateOffset(CViewport* view, const QPoint& point)
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);
    BrushVec3 hitPos;
    if (m_Context.polygon->GetPlane().HitTest(localRaySrc, localRaySrc + localRayDir, NULL, &hitPos))
    {
        m_pOffsetedPolygon = m_Context.polygon->Clone();
        m_fScale = m_Context.plane.Distance(hitPos);

        if (GetIEditor()->GetViewManager()->GetGrid()->IsEnabled())
        {
            m_fScale = CD::Snap(m_fScale);
        }

        m_fScale = ApplyScaleToSelectedPolygon(m_pOffsetedPolygon, m_fScale);
        m_HitPos = m_Context.origin + m_Context.plane.Normal() * m_fScale;
    }
}

void OffsetTool::ApplyOffset(CD::Model* pModel, CD::PolygonPtr pScaledPolygon, CD::PolygonPtr pOriginalPolygon, std::vector<CD::PolygonPtr>& holes, bool bCreateBridgeEdges)
{
    for (int i = 0, iHoleCount(holes.size()); i < iHoleCount; ++i)
    {
        pScaledPolygon->Intersect(holes[i]);
    }

    pModel->AddPolygon(pScaledPolygon, CD::eOpType_Split);

    if (bCreateBridgeEdges)
    {
        std::map<int, BrushLine> correspondingLineMap;
        CD::PolygonPtr pResizedPolygon = pOriginalPolygon->Clone();
        pResizedPolygon->Scale((BrushFloat)0.01);

        for (int i = 0, iCount(pResizedPolygon->GetVertexCount()); i < iCount; ++i)
        {
            int nCorrespondingIndex = 0;
            if (!pOriginalPolygon->GetNearestVertexIndex(pResizedPolygon->GetPos(i), nCorrespondingIndex))
            {
                DESIGNER_ASSERT(0);
                correspondingLineMap.clear();
                break;
            }
            BrushVec2 vFromSource = pOriginalPolygon->GetPlane().W2P(pOriginalPolygon->GetPos(nCorrespondingIndex));
            BrushVec2 vToResized = pOriginalPolygon->GetPlane().W2P(pResizedPolygon->GetPos(i));
            correspondingLineMap[nCorrespondingIndex] = BrushLine(vFromSource, vToResized);
        }

        for (int i = 0, iVertexCount(pScaledPolygon->GetVertexCount()); i < iVertexCount; ++i)
        {
            BrushVec3 vInOffsetPolygon = pScaledPolygon->GetPos(i);

            std::map<int, BrushLine>::iterator ii = correspondingLineMap.begin();
            std::map<BrushFloat, BrushVec3> candidatedVertices;

            for (; ii != correspondingLineMap.end(); ++ii)
            {
                int nVertexIndexInSelectedPolygon = ii->first;
                const BrushLine& correspondingLine = ii->second;

                if (std::abs(correspondingLine.Distance(pOriginalPolygon->GetPlane().W2P(vInOffsetPolygon))) < kDesignerLooseEpsilon)
                {
                    BrushVec3 vInSelectedPolygon = pOriginalPolygon->GetPos(nVertexIndexInSelectedPolygon);
                    candidatedVertices[vInSelectedPolygon.GetDistance(vInOffsetPolygon)] = vInSelectedPolygon;
                }
            }

            if (!candidatedVertices.empty())
            {
                std::vector<BrushVec3> vList;
                vList.push_back(candidatedVertices.begin()->second);
                vList.push_back(vInOffsetPolygon);
                CD::PolygonPtr pPolygon = new CD::Polygon(vList, pOriginalPolygon->GetPlane(), pOriginalPolygon->GetMaterialID(), &(pOriginalPolygon->GetTexInfo()), false);
                if (pPolygon->IsValid())
                {
                    pModel->AddOpenPolygon(pPolygon, false);
                }
            }
        }
    }
}

void OffsetTool::AddScaledPolygon(bool bCreateBridgeEdges)
{
    CUndo undo("Designer : Offset");
    GetModel()->RecordUndo("Offset", GetBaseObject());
    ApplyOffset(GetModel(), m_pOffsetedPolygon, m_Context.polygon, m_Context.holes, bCreateBridgeEdges);
    CD::UpdateMirroredPartWithPlane(GetModel(), m_pOffsetedPolygon->GetPlane());
    m_pOffsetedPolygon = NULL;
    UpdateAll(CD::eUT_ExceptMirror);
}

void OffsetTool::OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_LButtonInfo.m_LastAction = CD::eMouseAction_LButtonDoubleClick;

    if (!IsTwoPointEquivalent(point, m_LButtonInfo.m_DownPos))
    {
        return;
    }
    m_pOffsetedPolygon = QueryOffsetPolygon(view, point);
    if (!m_pOffsetedPolygon || fabs(m_fPrevScale) < kDesignerEpsilon)
    {
        return;
    }
    m_pOffsetedPolygon = m_pOffsetedPolygon->Clone();
    ApplyScaleToSelectedPolygon(m_pOffsetedPolygon, m_fPrevScale);
    AddScaledPolygon(nFlags & MK_CONTROL);
}

BrushFloat OffsetTool::ApplyScaleToSelectedPolygon(CD::PolygonPtr pPolygon, BrushFloat fScale)
{
    if (!pPolygon)
    {
        return fScale;
    }

    if (!pPolygon->Scale(fScale, true))
    {
        BrushFloat fChangedScale = fScale;
        if (CD::BinarySearchForScale(0, fScale, 0, *pPolygon, fChangedScale))
        {
            if (pPolygon->Scale(fChangedScale, true))
            {
                return fChangedScale;
            }
        }
    }

    return fScale;
}

void OffsetTool::Display(DisplayContext& dc)
{
    BaseTool::Display(dc);

    dc.SetFillMode(e_FillModeSolid);

    if (m_pOffsetedPolygon)
    {
        float oldThickness = dc.GetLineWidth();
        dc.SetLineWidth(CD::kLineThickness);

        dc.SetColor(CD::kResizedPolygonColor);
        m_pOffsetedPolygon->Display(dc);

        dc.SetColor(ColorB(160, 160, 160, 255));
        dc.DrawLine(m_Context.origin, m_HitPos);

        dc.SetLineWidth(oldThickness);
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Offset, CD::eToolGroup_Edit, "Offset", OffsetTool)