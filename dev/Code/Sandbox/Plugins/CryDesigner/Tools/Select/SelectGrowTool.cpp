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
#include "SelectGrowTool.h"
#include "Tools/DesignerTool.h"
#include "SelectGrowTool.h"
#include "Util/ElementManager.h"

using namespace CD;

void SelectGrowTool::Enter()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        CD::MessageBox("Warning", "At least one element should be selected to use this selection.");
        CD::GetDesigner()->SwitchToPrevTool();
        return;
    }
    CUndo undo("Designer : Growing Selection");
    CD::GetDesigner()->StoreSelectionUndo();
    GrowSelection(GetMainContext());
    CD::GetDesigner()->SwitchToPrevTool();
}

void SelectGrowTool::GrowSelection(CD::SMainContext& mc)
{
    std::set<CD::PolygonPtr> selectedSet = MakeInitialSelectedSet(mc);
    SelectAdjacentPolygonsFromEdgeVertex(mc, selectedSet, false);
    if (!selectedSet.empty())
    {
        SelectAdjacentPolygons(mc, selectedSet, false);
    }
}

std::set<CD::PolygonPtr> SelectGrowTool::MakeInitialSelectedSet(CD::SMainContext& mc)
{
    std::set<CD::PolygonPtr> selectedSet;
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return selectedSet;
    }

    for (int i = 0, iSelectionCount(pSelected->GetCount()); i < iSelectionCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || (*pSelected)[i].m_pPolygon == NULL)
        {
            continue;
        }
        selectedSet.insert((*pSelected)[i].m_pPolygon);
    }

    return selectedSet;
}

void SelectGrowTool::SelectAdjacentPolygonsFromEdgeVertex(CD::SMainContext& mc, std::set<CD::PolygonPtr>& selectedSet, bool bAddNewSelections)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return;
    }

    for (int i = 0, iSelectionCount(pSelected->GetCount()); i < iSelectionCount; ++i)
    {
        if (!(*pSelected)[i].IsVertex() && !(*pSelected)[i].IsEdge())
        {
            continue;
        }

        ModelDB::QueryResult qResult;
        for (int k = 0, iElementCount((*pSelected)[i].m_Vertices.size()); k < iElementCount; ++k)
        {
            mc.pModel->GetDB()->QueryAsVertex((*pSelected)[i].m_Vertices[k], qResult);
        }

        for (int k = 0, iQueryCount(qResult.size()); k < iQueryCount; ++k)
        {
            for (int a = 0, iMarkCount(qResult[k].m_MarkList.size()); a < iMarkCount; ++a)
            {
                CD::PolygonPtr pPolygon = qResult[k].m_MarkList[a].m_pPolygon;
                SElement de;
                de.SetFace(mc.pObject, pPolygon);
                pSelected->Add(de);
                if (bAddNewSelections && (*pSelected)[i].m_pPolygon)
                {
                    selectedSet.insert((*pSelected)[i].m_pPolygon);
                }
            }
        }
    }
}

bool SelectGrowTool::SelectAdjacentPolygons(CD::SMainContext& mc, std::set<CD::PolygonPtr>& selectedSet, bool bAddNewSelections)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    bool bAddedNewAdjacent = false;
    for (int i = 0, nPolygonCount(mc.pModel->GetPolygonCount()); i < nPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = mc.pModel->GetPolygon(i);
        DESIGNER_ASSERT(pPolygon);
        if (!pPolygon || selectedSet.find(pPolygon) != selectedSet.end())
        {
            continue;
        }

        bool bAdjacent = false;
        std::set<CD::PolygonPtr>::iterator ii = selectedSet.begin();
        for (; ii != selectedSet.end(); ++ii)
        {
            if (pPolygon->HasOverlappedEdges(*ii))
            {
                bAdjacent = true;
                break;
            }
        }
        if (!bAdjacent)
        {
            continue;
        }

        pSelected->Add(SElement(mc.pObject, pPolygon));
        if (bAddNewSelections)
        {
            selectedSet.insert(pPolygon);
        }

        bAddedNewAdjacent = true;
    }

    return bAddedNewAdjacent;
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Grow, CD::eToolGroup_Selection, "Grow", SelectGrowTool)