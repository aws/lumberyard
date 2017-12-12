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
#include "InvertSelectionTool.h"
#include "Tools/DesignerTool.h"
#include "Util/ElementManager.h"
#include "Tools/Select/SelectTool.h"

void InvertSelectionTool::InvertSelection(CD::SMainContext& mc)
{
    int nSelectedElementCount = mc.pSelected->GetCount();
    ElementManager newSelectionList;
    for (int k = 0, iPolygonCount(mc.pModel->GetPolygonCount()); k < iPolygonCount; ++k)
    {
        bool bSameExist = false;
        CD::PolygonPtr pPolygon = mc.pModel->GetPolygon(k);
        for (int i = 0; i < nSelectedElementCount; ++i)
        {
            if (!(*mc.pSelected)[i].IsFace() || (*mc.pSelected)[i].m_pPolygon == NULL)
            {
                continue;
            }
            if ((*mc.pSelected)[i].m_pPolygon == pPolygon)
            {
                bSameExist = true;
                break;
            }
        }
        if (!bSameExist)
        {
            SElement de;
            de.SetFace(mc.pObject, pPolygon);
            newSelectionList.Add(de);
        }
    }

    mc.pSelected->Clear();
    mc.pSelected->Add(newSelectionList);
}

void InvertSelectionTool::Enter()
{
    CUndo undo("Designer : Invert Selection");
    CD::GetDesigner()->StoreSelectionUndo();
    InvertSelection(GetMainContext());
    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
    CD::GetDesigner()->SwitchToPrevTool();
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Invert, CD::eToolGroup_Selection, "Invert", InvertSelectionTool)