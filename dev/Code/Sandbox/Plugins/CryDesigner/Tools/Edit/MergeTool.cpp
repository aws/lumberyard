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
#include "MergeTool.h"
#include "Tools/DesignerTool.h"
#include "Objects/DesignerObject.h"
#include "Tools/Modify/MirrorTool.h"
#include "Tools/Select/SelectTool.h"
#include "Core/BrushHelper.h"

std::vector<CD::PolygonPtr> GetPolygonList(ElementManager* pElements)
{
    std::vector<CD::PolygonPtr> polygonList;
    for (int i = 0, iElementCount(pElements->GetCount()); i < iElementCount; ++i)
    {
        if ((*pElements)[i].IsFace())
        {
            polygonList.push_back((*pElements)[i].m_pPolygon);
        }
    }
    return polygonList;
}

void MergeTool::Enter()
{
    std::vector<CD::SSelectedInfo> selections;
    CD::GetSelectedObjectList(selections);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    std::vector<CD::PolygonPtr> selectedPolygons = GetPolygonList(pSelected);

    if (selections.size() < 2 && selectedPolygons.size() < 2)
    {
        CD::MessageBox("Warning", "More than 2 designer objects or 2 faces should be selected to be merged.");
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
        return;
    }

    CUndo undo("Designer : Object Merge");

    if (selections.size() >= 2)
    {
        MergeObjects();
    }
    else if (selectedPolygons.size() >= 2)
    {
        MergePolygons();
    }
}

void MergeTool::MergeObjects()
{
    std::vector<CD::SSelectedInfo> selections;
    CD::GetSelectedObjectList(selections);

    int nSelectionCount = selections.size();
    DesignerObject* pMergedModelObj = (DesignerObject*)selections[nSelectionCount - 1].m_pObj;
    CD::Model* pMergedModel = (CD::Model*)selections[nSelectionCount - 1].m_pModel;

    pMergedModel->RecordUndo("Designer : Merge", pMergedModelObj);
    CD::ResetXForm(pMergedModelObj, pMergedModel);

    for (int i = 0; i < nSelectionCount - 1; ++i)
    {
        CD::SSelectedInfo& selection = selections[i];
        if (selection.m_pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            MirrorTool::ReleaseMirrorMode(selection.m_pModel);
            MirrorTool::RemoveEdgesOnMirrorPlane(selection.m_pModel);
        }
        pMergedModelObj->Merge(selection.m_pObj, selection.m_pModel);
    }

    CD::UpdateAll(pMergedModelObj->MainContext());

    CD::GetDesigner()->SetBaseObject(NULL);
    for (int i = 0; i < nSelectionCount - 1; ++i)
    {
        GetIEditor()->DeleteObject(selections[i].m_pObj);
    }

    GetIEditor()->SelectObject(pMergedModelObj);

    DesignerTool* pDesignerTool = CD::GetDesigner();
    if (pDesignerTool)
    {
        pDesignerTool->SetBaseObject(pMergedModelObj);
        pDesignerTool->SwitchTool(CD::eDesigner_Object);
    }
}

void MergeTool::MergePolygons(CD::SMainContext& mc)
{
    std::vector<CD::PolygonPtr> selectedPolygons = GetPolygonList(CD::GetDesigner()->GetSelectedElements());
    int nSelectedPolygonCount = selectedPolygons.size();

    std::set<CD::PolygonPtr> usedPolygons;

    while (usedPolygons.size() < nSelectedPolygonCount)
    {
        CD::PolygonPtr pPolygon = NULL;
        for (int i = 0; i < nSelectedPolygonCount; ++i)
        {
            if (usedPolygons.find(selectedPolygons[i]) == usedPolygons.end())
            {
                pPolygon = selectedPolygons[i];
                break;
            }
        }

        if (pPolygon == NULL)
        {
            break;
        }

        bool bMerged = false;
        for (int i = 0; i < nSelectedPolygonCount; ++i)
        {
            if (pPolygon == selectedPolygons[i] || usedPolygons.find(selectedPolygons[i]) != usedPolygons.end())
            {
                continue;
            }

            if (CD::eIT_None == CD::Polygon::HasIntersection(pPolygon, selectedPolygons[i]) || pPolygon->GetMaterialID() != selectedPolygons[i]->GetMaterialID())
            {
                continue;
            }

            pPolygon->Union(selectedPolygons[i]);
            mc.pModel->RemovePolygon(selectedPolygons[i]);
            usedPolygons.insert(selectedPolygons[i]);
            bMerged = true;
        }

        if (!bMerged)
        {
            usedPolygons.insert(pPolygon);
        }
    }
}

void MergeTool::MergePolygons()
{
    GetModel()->RecordUndo("Designer : Merge Polygons", GetBaseObject());
    MergePolygons(CD::SMainContext(GetBaseObject(), GetCompiler(), GetModel()));
    UpdateAll();
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Merge, CD::eToolGroup_Edit, "Merge", MergeTool)