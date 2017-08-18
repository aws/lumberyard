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
#include "SelectAllNoneTool.h"
#include "Tools/DesignerTool.h"
#include "Tools/Select/SelectTool.h"

void SelectAllNoneTool::Enter()
{
    CUndo undo("Designer : All or None Selection");
    CD::GetDesigner()->StoreSelectionUndo();

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    bool bSelectedElementsExist = !pSelected->IsEmpty();
    pSelected->Clear();

    if (!bSelectedElementsExist)
    {
        CD::EDesignerTool prevDesignerMode = CD::GetDesigner()->GetPrevTool();
        if (!CD::IsSelectElementMode(prevDesignerMode))
        {
            prevDesignerMode = CD::GetDesigner()->GetPrevSelectTool();
            CD::GetDesigner()->SetPrevTool(prevDesignerMode);
        }
        if (prevDesignerMode & CD::eDesigner_Edge)
        {
            SelectAllEdges(GetBaseObject(), GetModel());
        }
        else if (prevDesignerMode & CD::eDesigner_Vertex)
        {
            SelectAllVertices(GetBaseObject(), GetModel());
        }
        else
        {
            SelectAllFaces(GetBaseObject(), GetModel());
        }
    }

    CD::UpdateDrawnEdges(GetMainContext());
    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(pSelected);
    CD::GetDesigner()->SwitchToPrevTool();
}

void SelectAllNoneTool::SelectAllVertices(CBaseObject* pObject, CD::Model* pModel)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
        for (int k = 0, iVertexCount(pPolygon->GetVertexCount()); k < iVertexCount; ++k)
        {
            pSelected->Add(SElement(pObject, pPolygon->GetPos(k)));
        }
    }
}

void SelectAllNoneTool::SelectAllEdges(CBaseObject* pObject, CD::Model* pModel)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
        for (int k = 0, iEdgeCount(pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
        {
            BrushEdge3D e = pPolygon->GetEdge(k);
            pSelected->Add(SElement(pObject, e));
        }
    }
}

void SelectAllNoneTool::SelectAllFaces(CBaseObject* pObject, CD::Model* pModel)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
        pSelected->Add(SElement(pObject, pPolygon));
    }
}

void SelectAllNoneTool::DeselectAllVertices()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Erase(CD::ePF_Vertex);
}

void SelectAllNoneTool::DeselectAllEdges()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Erase(CD::ePF_Edge);
}

void SelectAllNoneTool::DeselectAllFaces()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Erase(CD::ePF_Face);
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_AllNone, CD::eToolGroup_Selection, "AllNone", SelectAllNoneTool)