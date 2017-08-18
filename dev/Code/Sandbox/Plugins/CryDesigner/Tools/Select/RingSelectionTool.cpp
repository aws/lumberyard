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
#include "RingSelectionTool.h"
#include "Tools/DesignerTool.h"
#include "Util/ElementManager.h"
#include "LoopSelectionTool.h"

void RingSelectionTool::Enter()
{
    CUndo undo("Designer : Ring Selection");
    CD::GetDesigner()->StoreSelectionUndo();
    RingSelection(GetMainContext());
    CD::GetDesigner()->SwitchToPrevTool();
}

void RingSelectionTool::RingSelection(CD::SMainContext& mc)
{
    int nSelectedElementCount = mc.pSelected->GetCount();

    if (mc.pSelected->GetCount() == mc.pSelected->GetFaceCount())
    {
        SelectFaceRing(mc);
        return;
    }

    for (int i = 0; i < nSelectedElementCount; ++i)
    {
        if (!(*mc.pSelected)[i].IsEdge())
        {
            continue;
        }
        SelectRing(mc, (*mc.pSelected)[i].GetEdge());
    }
}

void RingSelectionTool::SelectRing(CD::SMainContext& mc, const BrushEdge3D& inputEdge)
{
    BrushEdge3D edge(inputEdge);

    std::vector<CD::PolygonPtr> polygons;
    mc.pModel->QueryPolygonsSharingEdge(edge, polygons);
    if (polygons.size() == 1)
    {
        BrushEdge3D oppositeEdge;
        if (polygons[0]->FindOppositeEdge(edge, oppositeEdge))
        {
            polygons.clear();
            mc.pModel->QueryPolygonsSharingEdge(oppositeEdge, polygons);
        }
        else
        {
            return;
        }
    }

    if (polygons.size() != 2)
    {
        return;
    }

    std::vector<CD::PolygonPtr> loopPolygons;
    loopPolygons.push_back(polygons[0]);
    loopPolygons.push_back(polygons[1]);
    LoopSelectionTool::GetLoopPolygonsInBothWays(mc, polygons[0], polygons[1], loopPolygons);

    for (int i = 0, iLoopPolygonCount(loopPolygons.size()); i < iLoopPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = loopPolygons[i];
        CD::PolygonPtr pNextPolygon = loopPolygons[(i + 1) % iLoopPolygonCount];

        BrushEdge3D sharing_edge;
        if (pPolygon->FindSharingEdge(pNextPolygon, sharing_edge))
        {
            mc.pSelected->Add(SElement(mc.pObject, sharing_edge));
        }
        else
        {
            if (loopPolygons[0]->FindSharingEdge(loopPolygons[1], sharing_edge))
            {
                BrushEdge3D oppositeEdge;
                if (loopPolygons[0]->FindOppositeEdge(sharing_edge, oppositeEdge))
                {
                    mc.pSelected->Add(SElement(mc.pObject, oppositeEdge));
                }
            }

            if (loopPolygons[iLoopPolygonCount - 1]->FindSharingEdge(loopPolygons[iLoopPolygonCount - 2], sharing_edge))
            {
                BrushEdge3D oppositeEdge;
                if (loopPolygons[iLoopPolygonCount - 1]->FindOppositeEdge(sharing_edge, oppositeEdge))
                {
                    mc.pSelected->Add(SElement(mc.pObject, oppositeEdge));
                }
            }
        }
    }
}

void SelectLoopFromOnePolygon(CD::SMainContext& mc, const std::vector<CD::SVertex>& quad, CD::PolygonPtr pQuadPolygon, int nEdgeIndex)
{
    std::vector<CD::PolygonPtr> adjacentPolygons;
    mc.pModel->QueryPolygonsSharingEdge(BrushEdge3D(quad[(nEdgeIndex + 1) % 4].pos, quad[(nEdgeIndex + 2) % 4].pos), adjacentPolygons);
    if (adjacentPolygons.size() != 2)
    {
        adjacentPolygons.clear();
        mc.pModel->QueryPolygonsSharingEdge(BrushEdge3D(quad[(nEdgeIndex + 3) % 4].pos, quad[nEdgeIndex].pos), adjacentPolygons);
    }
    for (int c = 0; c < adjacentPolygons.size(); ++c)
    {
        if (pQuadPolygon == adjacentPolygons[c])
        {
            continue;
        }
        mc.pSelected->Add(SElement(mc.pObject, adjacentPolygons[c]));
        LoopSelectionTool::SelectFaceLoop(mc, pQuadPolygon, adjacentPolygons[c]);
        LoopSelectionTool::SelectFaceLoop(mc, adjacentPolygons[c], pQuadPolygon);
    }
}

void RingSelectionTool::SelectFaceRing(CD::SMainContext& mc)
{
    int nSelectedElementCount = mc.pSelected->GetCount();

    std::vector<CD::PolygonPtr> quads;

    for (int i = 0; i < nSelectedElementCount; ++i)
    {
        const SElement& element = mc.pSelected->Get(i);
        if (!element.IsFace() || !element.m_pPolygon || element.m_pPolygon->GetEdgeCount() != 4)
        {
            continue;
        }
        quads.push_back(element.m_pPolygon);
    }

    if (quads.empty())
    {
        return;
    }

    std::set<int> usedQuads;
    int quadCount = quads.size();
    for (int i = 0; i < quadCount; ++i)
    {
        if (usedQuads.find(i) != usedQuads.end())
        {
            continue;
        }

        usedQuads.insert(i);

        std::vector<CD::SVertex> quad0;
        quads[i]->GetLinkedVertices(quad0);

        bool bDone = false;
        for (int k = 0; k < quadCount; ++k)
        {
            if (i == k)
            {
                continue;
            }

            std::vector<CD::SVertex> quad1;
            quads[k]->GetLinkedVertices(quad1);

            for (int a = 0; a < 4; ++a)
            {
                BrushEdge3D e0(quad0[a].pos, quad0[(a + 1) % 4].pos);
                for (int b = 0; b < 4; ++b)
                {
                    BrushEdge3D e1(quad1[(b + 1) % 4].pos, quad1[b].pos);
                    if (e0.IsEquivalent(e1))
                    {
                        if (usedQuads.find(k) == usedQuads.end())
                        {
                            SelectLoopFromOnePolygon(mc, quad1, quads[k], b);
                        }
                        SelectLoopFromOnePolygon(mc, quad0, quads[i], a);
                        usedQuads.insert(k);
                        bDone = true;
                    }
                }
                if (bDone)
                {
                    break;
                }
            }
            if (bDone)
            {
                break;
            }
        }
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Ring, CD::eToolGroup_Selection, "Ring", RingSelectionTool)