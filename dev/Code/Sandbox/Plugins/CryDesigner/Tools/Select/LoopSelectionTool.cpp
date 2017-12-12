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
#include "LoopSelectionTool.h"
#include "Tools/DesignerTool.h"
#include "Util/ElementManager.h"

namespace
{
    int s_nMaximumRepeatCount = 0;
}

using namespace CD;

void LoopSelectionTool::LoopSelection(SMainContext& mc)
{
    ElementManager copiedSelected = *mc.pSelected;
    int nSelectedElementCount = copiedSelected.GetCount();

    if (nSelectedElementCount >= 2)
    {
        const SElement& firstElement = copiedSelected[nSelectedElementCount - 2];
        const SElement& secondElement = copiedSelected[nSelectedElementCount - 1];
        if (firstElement.IsFace() && secondElement.IsFace())
        {
            SelectFaceLoop(mc, firstElement.m_pPolygon, secondElement.m_pPolygon);
            SelectFaceLoop(mc, secondElement.m_pPolygon, firstElement.m_pPolygon);
            return;
        }
    }

    mc.pSelected->Clear();
    s_nMaximumRepeatCount = CountAllEdges(mc) * 50;

    for (int i = 0; i < nSelectedElementCount; ++i)
    {
        if (!copiedSelected[i].IsEdge())
        {
            continue;
        }
        if (!SelectLoop(mc, copiedSelected[i].GetEdge()))
        {
            mc.pSelected->Clear();

            if (SelectBorderInOnePolygon(mc, copiedSelected[i].GetEdge()))
            {
                continue;
            }

            ElementManager elements;
            if (SelectBorder(mc, copiedSelected[i].GetEdge(), elements))
            {
                mc.pSelected->Add(elements);
                mc.pSelected->Add(SElement(mc.pObject, copiedSelected[i].GetEdge()));
            }
        }
    }
}

void AddEdgeToList(std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge)
{
    BrushEdge3D invEdge = edge.GetInverted();

    for (int i = 0, iEdgeCount(edgeList.size()); i < iEdgeCount; ++i)
    {
        if (edgeList[i].IsEquivalent(edge) || edgeList[i].IsEquivalent(invEdge))
        {
            return;
        }
    }

    edgeList.push_back(edge);
}

void LoopSelectionTool::Enter()
{
    LoopSelection(GetMainContext());
    CD::GetDesigner()->SwitchToPrevTool();
}

bool HasEdge(std::vector<PolygonPtr>& polygonList, const BrushEdge3D& edge)
{
    for (int i = 0, iPolygonCount(polygonList.size()); i < iPolygonCount; ++i)
    {
        if (polygonList[i]->HasEdge(edge))
        {
            return true;
        }
    }
    return false;
}

bool FindNextEdge(SMainContext& mc, const BrushEdge3D& edge, BrushEdge3D& outEdge)
{
    ModelDB* pDB = mc.pModel->GetDB();
    ModelDB::QueryResult queryResult;

    if (!pDB->QueryAsVertex(edge.m_v[1], queryResult))
    {
        return false;
    }

    if (queryResult.size() != 1)
    {
        return false;
    }

    if (queryResult[0].m_MarkList.size() != 4)
    {
        return false;
    }

    std::vector<PolygonPtr> invalidPolygons;
    std::vector<PolygonPtr> validPolygons;
    for (int i = 0; i < 4; ++i)
    {
        PolygonPtr pPolygon = queryResult[0].m_MarkList[i].m_pPolygon;
        if (pPolygon->HasEdge(edge))
        {
            invalidPolygons.push_back(pPolygon);
        }
        else
        {
            validPolygons.push_back(pPolygon);
        }
    }

    for (int i = 0, iPolygonCount(validPolygons.size()); i < iPolygonCount; ++i)
    {
        PolygonPtr pPolygon = validPolygons[i];
        for (int k = 0, iEdgeCount(pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
        {
            BrushEdge3D e = pPolygon->GetEdge(k);
            if (IsEquivalent(e.m_v[0], edge.m_v[1]) && !HasEdge(invalidPolygons, e))
            {
                outEdge = e;
                return true;
            }
        }
    }

    return false;
}

bool LoopSelectionTool::SelectLoop(SMainContext& mc, const BrushEdge3D& initialEdge)
{
    BrushEdge3D edge = initialEdge;
    ElementManager edgeElements;

    int nCount = 0;
    bool bLoop = false;

    for (int i = 0; i < 2; ++i)
    {
        while (!bLoop)
        {
            edgeElements.Add(SElement(mc.pObject, edge));
            BrushEdge3D nextEdge;
            if (!FindNextEdge(mc, edge, nextEdge))
            {
                break;
            }
            bLoop = nextEdge.IsEquivalent(initialEdge);
            edge = nextEdge;
        }
        if (bLoop)
        {
            break;
        }
        edge = initialEdge.GetInverted();
    }

    if (edgeElements.IsEmpty() || (edgeElements.GetCount() == 1 && edgeElements[0].IsEdge() && edgeElements[0].GetEdge().IsEquivalent(initialEdge)))
    {
        return false;
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Add(edgeElements);

    return true;
}

bool LoopSelectionTool::SelectBorderInOnePolygon(SMainContext& mc, const BrushEdge3D& edge)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    std::vector<PolygonPtr> adjacentPolygons;
    mc.pModel->QueryPolygonsSharingEdge(edge, adjacentPolygons);

    if (adjacentPolygons.size() == 1)
    {
        std::vector<PolygonPtr> outerPolygons;
        adjacentPolygons[0]->GetSeparatedPolygons(outerPolygons, eSP_OuterHull);
        for (int i = 0, outerPolygonCount(outerPolygons.size()); i < outerPolygonCount; ++i)
        {
            if (!outerPolygons[i]->HasEdge(edge))
            {
                continue;
            }
            int nAddedCount = 0;
            int iEdgeCount(outerPolygons[i]->GetEdgeCount());
            for (int k = 0; k < iEdgeCount; ++k)
            {
                BrushEdge3D e = outerPolygons[i]->GetEdge(k);
                if (e.IsEquivalent(edge))
                {
                    continue;
                }
                if (GetPolygonCountSharingEdge(mc, e) == 1)
                {
                    ++nAddedCount;
                    pSelected->Add(SElement(mc.pObject, e));
                }
            }
            if (nAddedCount != iEdgeCount)
            {
                pSelected->Clear();
                return false;
            }
            return true;
        }
    }

    for (int a = 0, iAdjacentPolygonCount(adjacentPolygons.size()); a < iAdjacentPolygonCount; ++a)
    {
        std::vector<PolygonPtr> innterPolygons;
        adjacentPolygons[a]->GetSeparatedPolygons(innterPolygons, eSP_InnerHull);
        for (int i = 0, innerPolygonCount(innterPolygons.size()); i < innerPolygonCount; ++i)
        {
            if (!innterPolygons[i]->HasEdge(edge))
            {
                continue;
            }
            for (int k = 0, iEdgeCount(innterPolygons[i]->GetEdgeCount()); k < iEdgeCount; ++k)
            {
                BrushEdge3D e = innterPolygons[i]->GetEdge(k);
                if (GetPolygonCountSharingEdge(mc, e, &(innterPolygons[i]->GetPlane())) == 1)
                {
                    pSelected->Add(SElement(mc.pObject, e));
                }
            }
            return true;
        }
    }

    return false;
}

bool LoopSelectionTool::SelectBorder(SMainContext& mc, const BrushEdge3D& edge, ElementManager& outElementInfos)
{
    BrushEdge3D e(edge);
    int nCount = 0;

    while (nCount++ < s_nMaximumRepeatCount)
    {
        std::vector<BrushEdge3D> edgeList;
        mc.pModel->QueryEdgesHavingVertex(e.m_v[1], edgeList);

        bool bAdded = false;
        for (int i = 0, iEdgeCount(edgeList.size()); i < iEdgeCount; ++i)
        {
            if (e.IsEquivalent(edgeList[i]) || e.GetInverted().IsEquivalent(edgeList[i]))
            {
                continue;
            }
            if (GetPolygonCountSharingEdge(mc, edgeList[i]) != 1)
            {
                continue;
            }
            outElementInfos.Add(SElement(mc.pObject, edgeList[i]));
            e = edgeList[i];
            bAdded = true;
            break;
        }
        if (!bAdded)
        {
            break;
        }

        if (IsEquivalent(e.m_v[1], edge.m_v[0]))
        {
            return true;
        }
    }

    return false;
}

int LoopSelectionTool::GetPolygonCountSharingEdge(SMainContext& mc, const BrushEdge3D& edge, const BrushPlane* pPlane)
{
    std::vector<PolygonPtr> polygons;
    mc.pModel->QueryPolygonsSharingEdge(edge, polygons);
    int nCount = 0;
    for (int i = 0, iCount(polygons.size()); i < iCount; ++i)
    {
        if (!pPlane || polygons[i]->GetPlane().IsEquivalent(*pPlane))
        {
            ++nCount;
        }
    }
    return nCount;
}

int LoopSelectionTool::CountAllEdges(SMainContext& mc)
{
    int nEdgeCount = 0;
    for (int i = 0, iPolygonCount(mc.pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        nEdgeCount += mc.pModel->GetPolygon(i)->GetEdgeCount();
    }
    return nEdgeCount;
}

bool LoopSelectionTool::SelectFaceLoop(SMainContext& mc, PolygonPtr pFirstPolygon, PolygonPtr pSecondPolygon)
{
    std::vector<PolygonPtr> polygons;
    GetLoopPolygons(mc, pFirstPolygon, pSecondPolygon, polygons);

    if (polygons.empty())
    {
        return false;
    }

    for (int i = 0, iPolygonCount(polygons.size()); i < iPolygonCount; ++i)
    {
        mc.pSelected->Add(SElement(mc.pObject, polygons[i]));
    }

    return true;
}

bool LoopSelectionTool::GetLoopPolygons(SMainContext& mc, PolygonPtr pFirstPolygon, PolygonPtr pSecondPolygon, std::vector<PolygonPtr>& outPolygons)
{
    bool bAdded = false;
    int nCount = 0;

    PolygonPtr pInitPolygon = pFirstPolygon;

    while (pSecondPolygon && pInitPolygon != pSecondPolygon && pSecondPolygon->GetEdgeCount() == 4 && (nCount++) < mc.pModel->GetPolygonCount())
    {
        PolygonPtr pAdjacentPolygon = FindAdjacentNextPolygon(mc, pFirstPolygon, pSecondPolygon);
        pFirstPolygon = pSecondPolygon;
        pSecondPolygon = pAdjacentPolygon;
        if (pAdjacentPolygon && pAdjacentPolygon != pInitPolygon)
        {
            bAdded = true;
            outPolygons.push_back(pAdjacentPolygon);
        }
    }

    return bAdded;
}

bool LoopSelectionTool::GetLoopPolygonsInBothWays(SMainContext& mc, PolygonPtr pFirstPolygon, PolygonPtr pSecondPolygon, std::vector<PolygonPtr>& outPolygons)
{
    std::vector<PolygonPtr> polygonOneWay;
    std::vector<PolygonPtr> polygonTheOtherWay;
    bool bAdded0 = GetLoopPolygons(mc, pFirstPolygon, pSecondPolygon, polygonOneWay);
    bool bAdded1 = GetLoopPolygons(mc, pSecondPolygon, pFirstPolygon, polygonTheOtherWay);

    if (!bAdded0 && !bAdded1)
    {
        return false;
    }

    if (!polygonOneWay.empty())
    {
        outPolygons.insert(outPolygons.end(), polygonOneWay.begin(), polygonOneWay.end());
    }

    std::vector<PolygonPtr>::iterator ii = polygonTheOtherWay.begin();
    for (; ii != polygonTheOtherWay.end(); )
    {
        bool bErased = false;
        for (int k = 0, iPolygonCount(polygonOneWay.size()); k < iPolygonCount; ++k)
        {
            if (polygonOneWay[k] == *ii)
            {
                ii = polygonTheOtherWay.erase(ii);
                bErased = true;
                break;
            }
        }
        if (!bErased)
        {
            ++ii;
        }
    }

    if (!polygonTheOtherWay.empty())
    {
        outPolygons.insert(outPolygons.begin(), polygonTheOtherWay.rbegin(), polygonTheOtherWay.rend());
    }

    return true;
}

PolygonPtr LoopSelectionTool::FindAdjacentNextPolygon(SMainContext& mc, PolygonPtr pFristPolygon, PolygonPtr pSecondPolygon)
{
    int nFirstEdgeCount = pFristPolygon->GetEdgeCount();
    int nSecondEdgeCount = pSecondPolygon->GetEdgeCount();

    if ((nFirstEdgeCount != 3 && nFirstEdgeCount != 4) || (nSecondEdgeCount != 3 && nSecondEdgeCount != 4))
    {
        return NULL;
    }

    std::vector<CD::SVertex> firstVs;
    std::vector<CD::SVertex> secondVs;
    pFristPolygon->GetLinkedVertices(firstVs);
    pSecondPolygon->GetLinkedVertices(secondVs);
    for (int i = 0; i < nFirstEdgeCount; ++i)
    {
        BrushEdge3D first_e(firstVs[i].pos, firstVs[(i + 1) % nFirstEdgeCount].pos);
        for (int k = 0; k < nSecondEdgeCount; ++k)
        {
            BrushEdge3D second_e(secondVs[(k + 1) % nSecondEdgeCount].pos, secondVs[k].pos);
            if (first_e.IsEquivalent(second_e))
            {
                if (nSecondEdgeCount != 4)
                {
                    continue;
                }
                int nSecondEdgeCount_Half = nSecondEdgeCount / 2;
                BrushEdge3D oppositeEdge(secondVs[(k + nSecondEdgeCount_Half) % nSecondEdgeCount].pos, secondVs[(k + nSecondEdgeCount_Half + 1) % nSecondEdgeCount].pos);
                std::vector<PolygonPtr> polygonsSharingEdge;
                mc.pModel->QueryPolygonsSharingEdge(oppositeEdge, polygonsSharingEdge);
                int nPolygonCount(polygonsSharingEdge.size());
                for (int a = 0; a < nPolygonCount; ++a)
                {
                    int nEdgeCount = polygonsSharingEdge[a]->GetEdgeCount();
                    if (polygonsSharingEdge[a] != pSecondPolygon && (nEdgeCount == 3 || nEdgeCount == 4))
                    {
                        return polygonsSharingEdge[a];
                    }
                }
            }
        }
    }
    return NULL;
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Loop, CD::eToolGroup_Selection, "Loop", LoopSelectionTool)