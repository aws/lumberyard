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
#include "LoopCutTool.h"
#include "Util/ElementManager.h"
#include "Core/Model.h"
#include "Tools/Select/LoopSelectionTool.h"
#include "ViewManager.h"
#include "Util/HeightManipulator.h"
#include "Tools/DesignerTool.h"

void LoopCutTool::Enter()
{
    BaseTool::Enter();
    m_pSelectedPolygon = NULL;
    m_SegmentNum = 1;
    m_SlideOffset = 0;
    m_NormalizedSlideOffset = 0;
    m_LoopCutMode = eLoopCutMode_Divide;
    m_LoopCutDir = BrushVec3(0, 0, 0);
}

void LoopCutTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_LoopCutMode == eLoopCutMode_Divide)
    {
        if (!m_LoopPolygons.empty())
        {
            m_LoopCutMode = eLoopCutMode_Slide;
            m_SlideOffset = m_NormalizedSlideOffset = 0;
            s_HeightManipulator.Init(BrushPlane(m_LoopCutDir, -m_LoopCutDir.Dot(m_vPosOnSurface)), m_vPosOnSurface);
        }
    }
    else if (m_LoopCutMode == eLoopCutMode_Slide)
    {
        m_LoopCutMode = eLoopCutMode_Divide;
        FreezeLoops();
    }
}

void LoopCutTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_LoopCutMode == eLoopCutMode_Divide)
    {
        BrushVec3 localRaySrc, localRayDir;
        CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);

        int nPolygonIndex(0);
        if (GetModel()->QueryPolygon(localRaySrc, localRayDir, nPolygonIndex))
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(nPolygonIndex);
            BrushVec3 vPosOnSurface;
            GetModel()->QueryPosition(pPolygon->GetPlane(), localRaySrc, localRayDir, vPosOnSurface);
            UpdateLoops(pPolygon, vPosOnSurface);
        }
        else
        {
            m_LoopPolygons.clear();
        }
    }
    else if (m_LoopCutMode == eLoopCutMode_Slide)
    {
        m_SlideOffset = s_HeightManipulator.UpdateHeight(GetWorldTM(), view, point);
        UpdateLoops();
    }
}

void LoopCutTool::OnMouseWheel(CViewport* view, UINT nFlags, const QPoint& point)
{
    short zDelta = (short)nFlags;

    if (zDelta > 0)
    {
        ++m_SegmentNum;
    }
    if (zDelta < 0)
    {
        --m_SegmentNum;
    }

    if (m_SegmentNum < 1)
    {
        m_SegmentNum = 1;
    }
    if (m_SegmentNum > 64)
    {
        m_SegmentNum = 64;
    }

    UpdateLoops();
}

bool LoopCutTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_LoopCutMode == eLoopCutMode_Divide)
        {
            return CD::GetDesigner()->EndCurrentEditSession();
        }
        else if (m_LoopCutMode == eLoopCutMode_Slide)
        {
            m_LoopCutMode = eLoopCutMode_Divide;
            m_SlideOffset = 0;
            UpdateLoops();
        }
    }

    return true;
}

void LoopCutTool::UpdateLoops(CD::PolygonPtr pPolygon, const BrushVec3& posOnSurface)
{
    m_pSelectedPolygon = pPolygon;
    m_vPosOnSurface = posOnSurface;

    UpdateLoops();
}

void RemoveComplexPolygons(std::vector<CD::PolygonPtr>& polygons)
{
    std::vector<CD::PolygonPtr>::iterator iter = polygons.begin();
    for (; iter != polygons.end(); )
    {
        if (!(*iter)->IsQuad() && !(*iter)->IsTriangle())
        {
            iter = polygons.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void LoopCutTool::UpdateLoops()
{
    if (m_pSelectedPolygon == NULL)
    {
        return;
    }

    if (!m_pSelectedPolygon->IsQuad())
    {
        m_LoopPolygons.clear();
        return;
    }

    BrushEdge3D nearestEdge;
    BrushVec3 nearestPos;
    if (m_pSelectedPolygon->QueryNearestEdge(m_vPosOnSurface, nearestEdge, nearestPos))
    {
        std::vector<CD::PolygonPtr> neighborPolygons;
        GetModel()->QueryPolygonsSharingEdge(nearestEdge.GetInverted(), neighborPolygons);
        RemoveComplexPolygons(neighborPolygons);

        std::vector<BrushEdge3D> eList;

        if (neighborPolygons.size() == 1)
        {
            neighborPolygons.clear();
            BrushEdge3D oppositeEdge;
            if (m_pSelectedPolygon->FindOppositeEdge(nearestEdge, oppositeEdge))
            {
                GetModel()->QueryPolygonsSharingEdge(oppositeEdge, neighborPolygons);
                RemoveComplexPolygons(neighborPolygons);
            }
        }

        if (neighborPolygons.size() == 1)
        {
            BrushEdge3D separatedEdge;
            if (neighborPolygons[0]->FindOppositeEdge(nearestEdge, separatedEdge))
            {
                m_LoopPolygons.clear();
                m_LoopPolygons.push_back(neighborPolygons[0]);
                eList.push_back(nearestEdge);
                eList.push_back(separatedEdge);
            }
        }
        else
        {
            for (int i = 0, iPolygonCount(neighborPolygons.size()); i < iPolygonCount; ++i)
            {
                if (neighborPolygons[i] == m_pSelectedPolygon)
                {
                    continue;
                }

                m_LoopPolygons.clear();

                if (!neighborPolygons[i]->IsQuad() && !neighborPolygons[i]->IsTriangle())
                {
                    continue;
                }

                std::vector<CD::PolygonPtr> loopPolygons;
                loopPolygons.push_back(m_pSelectedPolygon);
                loopPolygons.push_back(neighborPolygons[i]);
                LoopSelectionTool::GetLoopPolygonsInBothWays(GetMainContext(), m_pSelectedPolygon, neighborPolygons[i], loopPolygons);
                m_LoopPolygons.insert(m_LoopPolygons.begin(), loopPolygons.begin(), loopPolygons.end());

                for (int k = 0, iPolygonCount(m_LoopPolygons.size()); k < iPolygonCount; ++k)
                {
                    CD::PolygonPtr pPolygon = m_LoopPolygons[k].pPolygon;
                    CD::PolygonPtr pNextPolygon = m_LoopPolygons[(k + 1) % iPolygonCount].pPolygon;
                    BrushEdge3D e;
                    if (pPolygon->FindSharingEdge(pNextPolygon, e) && !CD::HasEdgeInEdgeList(eList, e))
                    {
                        eList.push_back(e);
                    }
                    else if (!eList.empty())
                    {
                        BrushEdge3D separatedEdge;

                        const BrushEdge3D& lastEdge = eList[eList.size() - 1];
                        if (pPolygon->FindOppositeEdge(lastEdge, separatedEdge))
                        {
                            eList.push_back(separatedEdge);
                        }

                        const BrushEdge3D& firstEdge = eList[0];
                        if (pNextPolygon->FindOppositeEdge(firstEdge, separatedEdge))
                        {
                            eList.push_back(separatedEdge);
                        }
                    }
                }
                break;
            }
        }

        if (eList.empty())
        {
            return;
        }

        if (m_LoopCutMode == eLoopCutMode_Divide)
        {
            for (int i = 0, eListCcount(eList.size()); i < eListCcount; ++i)
            {
                if (!eList[i].IsPoint())
                {
                    m_LoopCutDir = (eList[i].m_v[1] - eList[i].m_v[0]).GetNormalized();
                    break;
                }
            }
        }

        m_NormalizedSlideOffset = 0;

        for (int k = 0, iPolygonCount(m_LoopPolygons.size()); k < iPolygonCount; ++k)
        {
            if (m_LoopPolygons[k].pPolygon == m_pSelectedPolygon)
            {
                const BrushEdge3D& e = eList[(k + eList.size() - 1) % eList.size()];
                BrushFloat eLength = e.m_v[0].GetDistance(e.m_v[1]);
                m_NormalizedSlideOffset = (m_SlideOffset * 2.0f) / eLength;

                if (m_NormalizedSlideOffset < -1.0)
                {
                    m_NormalizedSlideOffset = -1.0;
                }

                if (m_NormalizedSlideOffset > 1.0)
                {
                    m_NormalizedSlideOffset = 1.0;
                }

                break;
            }
        }

        BrushFloat delta = 1 / (BrushFloat)(m_SegmentNum + 1);
        BrushFloat offset = m_NormalizedSlideOffset * delta;

        for (int k = 0, iPolygonCount(m_LoopPolygons.size()); k < iPolygonCount; ++k)
        {
            const BrushEdge3D& e0 = eList[(k + eList.size() - 1) % eList.size()];
            const BrushEdge3D& e1 = eList[k];
            BrushFloat t = delta + offset;
            BrushVec3 vDir0 = e0.m_v[1] - e0.m_v[0];
            BrushVec3 vDir1 = e1.m_v[1] - e1.m_v[0];

            m_LoopPolygons[k].edges.push_back(BrushEdge3D(e0.m_v[0], e1.m_v[0]));
            for (int a = 0; a < m_SegmentNum; ++a)
            {
                BrushVec3 v0 = e0.IsPoint() ? e0.m_v[0] : e0.m_v[0] + vDir0 * t;
                BrushVec3 v1 = e1.IsPoint() ? e1.m_v[0] : e1.m_v[0] + vDir1 * t;
                m_LoopPolygons[k].edges.push_back(BrushEdge3D(v0, v1));
                t += delta;
            }
            m_LoopPolygons[k].edges.push_back(BrushEdge3D(e0.m_v[1], e1.m_v[1]));
        }
    }
}

void LoopCutTool::Display(DisplayContext& dc)
{
    if (!m_LoopPolygons.empty())
    {
        dc.DepthTestOff();
        dc.SetLineWidth(1);
        if (m_LoopCutMode == eLoopCutMode_Divide)
        {
            dc.SetColor(ColorB(150, 50, 255, 255));
        }
        else
        {
            dc.SetColor(CD::kSelectedColor);
        }
        for (int i = 0, iCount(m_LoopPolygons.size()); i < iCount; ++i)
        {
            const std::vector<BrushEdge3D>& edges = m_LoopPolygons[i].edges;
            for (int k = 1, iEdgeCount(edges.size()); k < iEdgeCount - 1; ++k)
            {
                dc.DrawLine(edges[k].m_v[0], edges[k].m_v[1]);
            }
        }
        dc.DepthTestOn();
    }
}

void LoopCutTool::FreezeLoops()
{
    CUndo undo("Designer : LoopCut");
    GetModel()->RecordUndo("Designer : LoopCut", GetBaseObject());

    for (int i = 0, iLoopCount(m_LoopPolygons.size()); i < iLoopCount; ++i)
    {
        CD::PolygonPtr pPolygon = m_LoopPolygons[i].pPolygon;

        int nInit = std::abs(m_NormalizedSlideOffset + 1.0) <= kDesignerEpsilon ? 1 : 0;
        int nEnd = std::abs(m_NormalizedSlideOffset - 1.0) <= kDesignerEpsilon ? 1 : 0;
        for (int k = nInit, iEdgeCount(m_LoopPolygons[i].edges.size()); k < iEdgeCount - 1 - nEnd; ++k)
        {
            const BrushEdge3D& e0 = m_LoopPolygons[i].edges[k];
            const BrushEdge3D& e1 = m_LoopPolygons[i].edges[k + 1];

            std::vector<BrushVec3> vList;
            vList.reserve(4);

            vList.push_back(e0.m_v[0]);
            if (!CD::IsEquivalent(e0.m_v[0], e0.m_v[1]))
            {
                vList.push_back(e0.m_v[1]);
            }

            if (!CD::IsEquivalent(e1.m_v[1], e0.m_v[0]) && !CD::IsEquivalent(e1.m_v[1], e0.m_v[1]))
            {
                vList.push_back(e1.m_v[1]);
            }
            if (!CD::IsEquivalent(e1.m_v[0], e0.m_v[0]) && !CD::IsEquivalent(e1.m_v[0], e0.m_v[1]))
            {
                vList.push_back(e1.m_v[0]);
            }

            if (vList.size() < 3)
            {
                continue;
            }

            CD::PolygonPtr pSplittedPolygon = new CD::Polygon(vList);
            pSplittedPolygon->SetFlag(pPolygon->GetFlag());
            pSplittedPolygon->SetTexInfo(pPolygon->GetTexInfo());
            pSplittedPolygon->SetMaterialID(pPolygon->GetMaterialID());
            if (!pSplittedPolygon->GetPlane().IsSameFacing(pPolygon->GetPlane()))
            {
                pSplittedPolygon->Flip();
            }
            if (!pPolygon->CheckFlags(CD::ePolyFlag_NonplanarQuad))
            {
                pSplittedPolygon->SetPlane(pPolygon->GetPlane());
            }
            GetModel()->AddPolygon(pSplittedPolygon, CD::eOpType_Add);
        }

        GetModel()->RemovePolygon(pPolygon);
    }

    UpdateAll();

    m_pSelectedPolygon = NULL;
    m_SlideOffset = 0;
    m_LoopPolygons.clear();
    m_LoopCutDir = BrushVec3(0, 0, 0);
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_LoopCut, CD::eToolGroup_Modify, "LoopCut", LoopCutTool)