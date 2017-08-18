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
#include "HideFaceTool.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/DesignerTool.h"
#include "Serialization/Decorators/EditorActionButton.h"

using Serialization::ActionButton;

void HideFaceTool::Enter()
{
    SelectTool::Enter();

    if (HideSelectedFaces())
    {
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Face);
    }
}

bool HideFaceTool::HideSelectedFaces()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    std::vector<CD::PolygonPtr> selectedPolygons;

    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || (*pSelected)[i].m_pPolygon == NULL)
        {
            continue;
        }
        selectedPolygons.push_back((*pSelected)[i].m_pPolygon);
    }

    if (selectedPolygons.empty())
    {
        return false;
    }

    CUndo undo("Designer : Hide Face(s)");
    GetModel()->RecordUndo("Designer : Hide Face(s)", GetBaseObject());
    for (int i = 0, iPolygonCount(selectedPolygons.size()); i < iPolygonCount; ++i)
    {
        selectedPolygons[i]->AddFlags(CD::ePolyFlag_Hidden);
    }
    pSelected->Clear();
    CD::GetDesigner()->ReleaseSelectionMesh();
    UpdateAll();

    return true;
}

void HideFaceTool::UnhideAll()
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(0);

    ElementManager hiddenElements;
    for (int i = 0, iCount(GetModel()->GetPolygonCount()); i < iCount; ++i)
    {
        CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
        if (pPolygon->CheckFlags(CD::ePolyFlag_Hidden) && !pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
        {
            hiddenElements.Add(SElement(GetBaseObject(), pPolygon));
        }
    }

    if (!hiddenElements.IsEmpty())
    {
        CUndo undo("Designer : Unhide Face(s)");
        GetModel()->RecordUndo("Designer : Unhide Face(s)", GetBaseObject());

        for (int i = 0, iElementCount(hiddenElements.GetCount()); i < iElementCount; ++i)
        {
            hiddenElements[i].m_pPolygon->RemoveFlags(CD::ePolyFlag_Hidden);
        }

        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        pSelected->Set(hiddenElements);
        UpdateAll();
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Face);
    }
    else
    {
        CD::GetDesigner()->SwitchToPrevTool();
    }
}

void HideFaceTool::Serialize(Serialization::IArchive& ar)
{
    ar(ActionButton([this] {HideSelectedFaces();
            }), "Apply", "Apply");
    ar(ActionButton([this] {UnhideAll();
            }), "UnhideAllFaces", "Unhide All");
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_HideFace, CD::eToolGroup_Misc, "Hide Face", HideFaceTool, PropertyTreePanel<HideFaceTool>)