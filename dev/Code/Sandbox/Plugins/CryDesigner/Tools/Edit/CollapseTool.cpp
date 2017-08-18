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
#include "CollapseTool.h"
#include "WeldTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include "Util/ElementManager.h"

void CollapseTool::Enter()
{
    ElementManager* pElements = CD::GetDesigner()->GetSelectedElements();

    std::vector<BrushEdge3D> edgeList;
    for (int i = 0, iElementCount(pElements->GetCount()); i < iElementCount; ++i)
    {
        const SElement& element = pElements->Get(i);

        if (element.IsEdge())
        {
            if (!CD::HasEdgeInEdgeList(edgeList, element.GetEdge()))
            {
                edgeList.push_back(element.GetEdge());
            }
        }
        else if (element.IsFace() && element.m_pPolygon)
        {
            for (int k = 0, iEdgeCount(element.m_pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
            {
                BrushEdge3D e = element.m_pPolygon->GetEdge(k);
                if (!CD::HasEdgeInEdgeList(edgeList, e))
                {
                    edgeList.push_back(e);
                }
            }
        }
    }

    if (edgeList.empty())
    {
        return;
    }

    CUndo undo("Designer : Collapse");
    GetModel()->RecordUndo("Designer : Collapse", GetBaseObject());

    std::set<int> usedEdgeIndices;
    for (int i = 0, iEdgeCount(edgeList.size()); i < iEdgeCount; ++i)
    {
        if (usedEdgeIndices.find(i) != usedEdgeIndices.end())
        {
            continue;
        }

        const BrushEdge3D& e0 = edgeList[i];
        usedEdgeIndices.insert(i);
        std::vector<BrushVec3> connectedSet;
        connectedSet.push_back(e0.m_v[0]);
        connectedSet.push_back(e0.m_v[1]);

        bool bAdded = true;
        while (bAdded)
        {
            bAdded = false;
            for (int k = 0; k < iEdgeCount; ++k)
            {
                if (usedEdgeIndices.find(k) != usedEdgeIndices.end())
                {
                    continue;
                }

                const BrushEdge3D& e1 = edgeList[k];

                bool bHasFirstVertex = CD::HasVertexInVertexList(connectedSet, e1.m_v[0]);
                bool bHasSecondVertex = CD::HasVertexInVertexList(connectedSet, e1.m_v[1]);

                if (bHasFirstVertex || bHasSecondVertex)
                {
                    if (!bHasFirstVertex)
                    {
                        connectedSet.push_back(e1.m_v[0]);
                    }
                    if (!bHasSecondVertex)
                    {
                        connectedSet.push_back(e1.m_v[1]);
                    }
                    usedEdgeIndices.insert(k);
                    bAdded = true;
                }
            }
        }

        AABB aabbConnected;
        aabbConnected.Reset();
        int iConnectedCount(connectedSet.size());
        for (int k = 0; k < iConnectedCount; ++k)
        {
            aabbConnected.Add(connectedSet[k]);
        }
        BrushVec3 vCenterPos = aabbConnected.GetCenter();

        for (int k = 0; k < iConnectedCount; ++k)
        {
            WeldTool::Weld(GetMainContext(), connectedSet[k], vCenterPos);
        }
    }

    UpdateAll();
    pElements->Clear();
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Collapse, CD::eToolGroup_Edit, "Collapse", CollapseTool)