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
#include "WeldTool.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/DesignerTool.h"
#include "Core/PolygonDecomposer.h"

void WeldTool::Weld(CD::SMainContext& mc, const BrushVec3& vSrc, const BrushVec3& vTarget)
{
    BrushEdge3D e(vSrc, vTarget);
    std::vector<CD::PolygonPtr> unnecessaryPolygonList;
    std::vector<CD::PolygonPtr> newPolygonList;
    for (int i = 0, iPolygonCount(mc.pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = mc.pModel->GetPolygon(i);
        DESIGNER_ASSERT(pPolygon);
        if (!pPolygon)
        {
            continue;
        }

        int nVertexIndex = -1;
        int nEdgeIndex = -1;
        if (pPolygon->HasEdge(e, false, &nEdgeIndex))
        {
            CD::SEdge edgeIndexPair = pPolygon->GetEdgeIndexPair(nEdgeIndex);
            BrushVec3 v0 = pPolygon->GetPos(edgeIndexPair.m_i[0]);
            BrushVec3 v1 = pPolygon->GetPos(edgeIndexPair.m_i[1]);
            if (CD::IsEquivalent(e.m_v[0], v1))
            {
                std::swap(edgeIndexPair.m_i[0], edgeIndexPair.m_i[1]);
                std::swap(v0, v1);
            }
            pPolygon->SetPos(edgeIndexPair.m_i[0], v1);
            pPolygon->Optimize();
            if (!pPolygon->IsValid() || pPolygon->IsOpen())
            {
                unnecessaryPolygonList.push_back(pPolygon);
            }
        }
        else if (pPolygon->HasVertex(e.m_v[0], &nVertexIndex))
        {
            if (std::abs(pPolygon->GetPlane().Distance(e.m_v[1])) > kDesignerEpsilon)
            {
                if (pPolygon->IsQuad())
                {
                    pPolygon->SetPos(nVertexIndex, e.m_v[1]);
                    pPolygon->AddFlags(CD::ePolyFlag_NonplanarQuad);
                    BrushPlane newPlane;
                    pPolygon->GetComputedPlane(newPlane);
                    pPolygon->SetPlane(newPlane);
                }
                else
                {
                    PolygonDecomposer decomposer;
                    std::vector<CD::PolygonPtr> triangulatedPolygons;
                    if (decomposer.TriangulatePolygon(pPolygon, triangulatedPolygons))
                    {
                        unnecessaryPolygonList.push_back(pPolygon);
                        for (int k = 0, iTriangulatedPolygonCount(triangulatedPolygons.size()); k < iTriangulatedPolygonCount; ++k)
                        {
                            newPolygonList.push_back(triangulatedPolygons[k]);
                            int nVertexIndexInSubTri = -1;
                            if (!triangulatedPolygons[k]->HasVertex(e.m_v[0], &nVertexIndexInSubTri))
                            {
                                continue;
                            }
                            triangulatedPolygons[k]->SetPos(nVertexIndexInSubTri, e.m_v[1]);
                            BrushPlane newPlane;
                            triangulatedPolygons[k]->GetComputedPlane(newPlane);
                            triangulatedPolygons[k]->SetPlane(newPlane);
                        }
                    }
                }
            }
            else
            {
                pPolygon->SetPos(nVertexIndex, e.m_v[1]);
                pPolygon->Optimize();
                if (!pPolygon->IsValid() || pPolygon->IsOpen())
                {
                    unnecessaryPolygonList.push_back(pPolygon);
                }
            }
        }
    }

    for (int i = 0, iUnnecessaryCount(unnecessaryPolygonList.size()); i < iUnnecessaryCount; ++i)
    {
        mc.pModel->RemovePolygon(unnecessaryPolygonList[i]);
    }

    for (int i = 0, iNewPolygonCount(newPolygonList.size()); i < iNewPolygonCount; ++i)
    {
        mc.pModel->AddPolygon(newPolygonList[i], CD::eOpType_Union);
    }
}

void WeldTool::Enter()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    int nSelectionCount = pSelected->GetCount();
    if (nSelectionCount < 2)
    {
        CD::MessageBox("Warning", "More than two vertices should be selected for using this tool.");
        CD::GetDesigner()->SwitchToSelectTool();
        return;
    }

    for (int i = 0; i < nSelectionCount; ++i)
    {
        if (!(*pSelected)[i].IsVertex())
        {
            CD::MessageBox("Warning", "Only vertices must be selected for using this tool.");
            CD::GetDesigner()->SwitchToSelectTool();
            return;
        }
    }

    CUndo undo("Designer : Weld");
    GetModel()->RecordUndo("Designer : Weld", GetBaseObject());

    SElement lastSelection = (*pSelected)[nSelectionCount - 1];

    for (int i = 0; i < nSelectionCount - 1; ++i)
    {
        Weld(GetMainContext(), (*pSelected)[i].GetPos(), (*pSelected)[i + 1].GetPos());
    }

    UpdateAll();

    pSelected->Clear();
    pSelected->Add(lastSelection);
    CD::GetDesigner()->SwitchToSelectTool();
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Weld, CD::eToolGroup_Edit, "Weld", WeldTool)