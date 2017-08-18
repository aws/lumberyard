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
#include "FlipTool.h"
#include "Viewport.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "Core/SmoothingGroupManager.h"

void FlipTool::Enter()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (!pSelected->IsEmpty())
    {
        FlipPolygons();
    }
    pSelected->Erase(CD::ePF_Face);
    CD::GetDesigner()->SwitchToSelectTool();
}

void FlipTool::Leave()
{
    BaseTool::Leave();
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Set(m_FlipedSelectedElements);
}

void FlipTool::FlipPolygons(CD::SMainContext& mc, ElementManager& outFlipedElements)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    outFlipedElements.Clear();

    for (int i = 0, iElementCount(pSelected->GetCount()); i < iElementCount; ++i)
    {
        if (!(*pSelected)[i].IsFace())
        {
            continue;
        }

        CD::PolygonPtr pPolygon = (*pSelected)[i].m_pPolygon;
        if (pPolygon == NULL)
        {
            continue;
        }

        pPolygon->Flip();
        mc.pModel->GetSmoothingGroupMgr()->RemovePolygon(pPolygon);
        SElement flipedElement;
        flipedElement.SetFace(mc.pObject, pPolygon);
        outFlipedElements.Add(flipedElement);
    }
}

void FlipTool::FlipPolygons()
{
    const ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    CUndo undo("Designer : Flip");

    GetModel()->RecordUndo("Flip", GetBaseObject());

    m_FlipedSelectedElements.Clear();
    FlipPolygons(CD::SMainContext(GetBaseObject(), GetCompiler(), GetModel()), m_FlipedSelectedElements);

    if (m_FlipedSelectedElements.IsEmpty())
    {
        undo.Cancel();
    }

    if (!pSelected->IsEmpty())
    {
        UpdateAll();
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Flip, CD::eToolGroup_Edit, "Flip", FlipTool)