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
#include "SelectConnectedTool.h"
#include "Tools/DesignerTool.h"
#include "Util/ElementManager.h"

void SelectConnectedTool::Enter()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        CD::MessageBox("Warning", "At least one element should be selected to use this selection.");
        CD::GetDesigner()->SwitchToPrevTool();
        return;
    }
    CUndo undo("Designer : Selection of connected elements");
    CD::GetDesigner()->StoreSelectionUndo();
    SelectConnectedPolygons(GetMainContext());
    CD::GetDesigner()->SwitchToPrevTool();
}

void SelectConnectedTool::SelectConnectedPolygons(CD::SMainContext& mc)
{
    std::set<CD::PolygonPtr> selectedSet = MakeInitialSelectedSet(mc);
    SelectAdjacentPolygonsFromEdgeVertex(mc, selectedSet, true);
    if (!selectedSet.empty())
    {
        int nPolygonCount = mc.pModel->GetPolygonCount();
        while (SelectAdjacentPolygons(mc, selectedSet, true) && selectedSet.size() < nPolygonCount)
        {
            ;
        }
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Connected, CD::eToolGroup_Selection, "Connected", SelectConnectedTool)