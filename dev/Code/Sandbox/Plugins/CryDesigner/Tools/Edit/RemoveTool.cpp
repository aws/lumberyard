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
#include "RemoveTool.h"
#include "Viewport.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/DesignerTool.h"
#include "Core/BrushHelper.h"

void RemoveTool::Enter()
{
    BaseTool::Enter();
    RemoveSelectedElements();
    CD::GetDesigner()->SwitchToSelectTool();
}

bool RemoveTool::RemoveSelectedElements(CD::SMainContext& mc, bool bEraseMirrored)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    if (pSelected->IsEmpty())
    {
        return false;
    }

    bool bRemoved = false;
    int updateType = CD::eUT_ExceptMirror;

    int iSelectedElementCount(pSelected->GetCount());

    for (int i = 0; i < iSelectedElementCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || (*pSelected)[i].m_pPolygon == NULL)
        {
            continue;
        }

        int nPolygonIndex = -1;
        CD::PolygonPtr pPolygon = mc.pModel->QueryEquivalentPolygon((*pSelected)[i].m_pPolygon, &nPolygonIndex);
        if (!pPolygon)
        {
            continue;
        }
        mc.pModel->DrillPolygon(nPolygonIndex, CD::IsFrameRemainInRemovingFace(mc.pObject));
        if (bEraseMirrored)
        {
            CD::DrillMirroredPolygon(mc.pModel, pPolygon);
        }
        bRemoved = true;
    }

    for (int i = 0; i < iSelectedElementCount; ++i)
    {
        if (!(*pSelected)[i].IsEdge())
        {
            continue;
        }
        BrushEdge3D edge = (*pSelected)[i].GetEdge();
        if (mc.pModel->EraseEdge(edge))
        {
            mc.pModel->EraseEdge(edge.GetInverted());
            if (bEraseMirrored)
            {
                CD::EraseMirroredEdge(mc.pModel, edge);
            }
            bRemoved = true;
        }
    }

    for (int i = 0; i < iSelectedElementCount; ++i)
    {
        if (!(*pSelected)[i].IsVertex())
        {
            continue;
        }
        if (mc.pModel->EraseVertex((*pSelected)[i].GetPos()))
        {
            bRemoved = true;
            updateType |= CD::eUT_Mirror;
        }
    }

    if (bRemoved)
    {
        CD::UpdateAll(mc, updateType);
    }

    pSelected->Clear();
    return true;
}

bool RemoveTool::RemoveSelectedElements()
{
    CUndo undo("CryDesigner : Remove elements");
    GetModel()->RecordUndo("Remove elements", GetBaseObject());

    if (RemoveSelectedElements(GetMainContext(), true))
    {
        UpdateAll();
        return true;
    }

    UpdateAll(CD::eUT_GameResource);
    return false;
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Remove, CD::eToolGroup_Edit, "Remove", RemoveTool)