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
#include "RemoveDoublesTool.h"
#include "Tools/Select/SelectTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "WeldTool.h"
#include "Serialization/Decorators/EditorActionButton.h"

using Serialization::ActionButton;

bool HasVertexInList(const std::vector<BrushVec3>& vList, const BrushVec3& vPos)
{
    for (int i = 0, iVListCount(vList.size()); i < iVListCount; ++i)
    {
        if (CD::IsEquivalent(vList[i], vPos))
        {
            return true;
        }
    }
    return false;
}

void RemoveDoublesTool::Enter()
{
    BaseTool::Enter();
    if (RemoveDoubles())
    {
        CD::GetDesigner()->SwitchToSelectTool();
    }
}

bool RemoveDoublesTool::RemoveDoubles()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (!pSelected || pSelected->IsEmpty())
    {
        return false;
    }
    CUndo undo("Designer : Remove Doubles");
    GetModel()->RecordUndo("Designer : Remove Doubles", GetBaseObject());
    RemoveDoubles(GetMainContext(), m_RemoveDoubleParameter.m_Distance);
    pSelected->Clear();
    UpdateAll();
    return true;
}

void RemoveDoublesTool::RemoveDoubles(CD::SMainContext& mc, float fDistance)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    std::vector<BrushVec3> uniqueVertices;
    for (int i = 0, iSelectedElementCount(pSelected->GetCount()); i < iSelectedElementCount; ++i)
    {
        for (int k = 0, iVertexCount((*pSelected)[i].m_Vertices.size()); k < iVertexCount; ++k)
        {
            if (!HasVertexInList(uniqueVertices, (*pSelected)[i].m_Vertices[k]))
            {
                uniqueVertices.push_back((*pSelected)[i].m_Vertices[k]);
            }
        }
    }

    int iVertexCount(uniqueVertices.size());
    BrushVec3 vTargetPos;
    std::set<int> alreadyUsedVertices;

    while (alreadyUsedVertices.size() < iVertexCount)
    {
        for (int i = iVertexCount - 1; i >= 0; --i)
        {
            if (alreadyUsedVertices.find(i) == alreadyUsedVertices.end())
            {
                vTargetPos = uniqueVertices[i];
                alreadyUsedVertices.insert(i);
                break;
            }
        }

        for (int i = 0; i < iVertexCount; ++i)
        {
            if (alreadyUsedVertices.find(i) != alreadyUsedVertices.end())
            {
                continue;
            }
            if (vTargetPos.GetDistance(uniqueVertices[i]) < fDistance)
            {
                WeldTool::Weld(mc, uniqueVertices[i], vTargetPos);
                alreadyUsedVertices.insert(i);
            }
        }
    }
}

void RemoveDoublesTool::Serialize(Serialization::IArchive& ar)
{
    m_RemoveDoubleParameter.Serialize(ar);
    if (ar.IsEdit())
    {
        ar(ActionButton( [this] { RemoveDoubles();
                }), "Apply", "Apply");
    }
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_RemoveDoubles, CD::eToolGroup_Edit, "Remove Doubles", RemoveDoublesTool, PropertyTreePanel<RemoveDoublesTool>)