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
#include "SnapToGridTool.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/DesignerTool.h"
#include "Core/PolygonDecomposer.h"
#include "Grid.h"
#include "ViewManager.h"

void SnapToGridTool::Enter()
{
    CUndo undo("Designer : Snap to Grid");
    GetModel()->RecordUndo("Snap To Grid", GetBaseObject());

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        for (int k = 0, iVertexCount((*pSelected)[i].m_Vertices.size()); k < iVertexCount; ++k)
        {
            BrushVec3 vSnappedPos = SnapVertexToGrid((*pSelected)[i].m_Vertices[k]);
            (*pSelected)[i].m_Vertices[k] = vSnappedPos;
        }
    }

    UpdateAll();
    CD::GetDesigner()->SwitchToPrevTool();
}

BrushVec3 SnapToGridTool::SnapVertexToGrid(const BrushVec3& vPos)
{
    CGrid* pGrid = GetIEditor()->GetViewManager()->GetGrid();

    BrushVec3 vWorldPos = GetWorldTM().TransformPoint(vPos);

    BrushVec3 vSnappedPos;
    vSnappedPos.x = floor((vWorldPos.x / pGrid->size) / pGrid->scale + 0.5) * pGrid->size * pGrid->scale;
    vSnappedPos.y = floor((vWorldPos.y / pGrid->size) / pGrid->scale + 0.5) * pGrid->size * pGrid->scale;
    vSnappedPos.z = floor((vWorldPos.z / pGrid->size) / pGrid->scale + 0.5) * pGrid->size * pGrid->scale;

    vSnappedPos = GetWorldTM().GetInverted().TransformPoint(vSnappedPos);

    CD::ModelDB::QueryResult qResult;
    GetModel()->GetDB()->QueryAsVertex(vPos, qResult);

    std::vector<CD::PolygonPtr> oldPolygons;
    std::vector<CD::PolygonPtr> newPolygons;

    for (int i = 0, iQueryCount(qResult.size()); i < iQueryCount; ++i)
    {
        for (int k = 0, iMarkCount(qResult[i].m_MarkList.size()); k < iMarkCount; ++k)
        {
            CD::PolygonPtr pPolygon = qResult[i].m_MarkList[k].m_pPolygon;
            int nVertexIndex = qResult[i].m_MarkList[k].m_VertexIndex;

            if (std::abs(pPolygon->GetPlane().Distance(vSnappedPos)) > kDesignerEpsilon)
            {
                oldPolygons.push_back(pPolygon);
                PolygonDecomposer triangulator;
                std::vector<CD::PolygonPtr> triangules;
                triangulator.TriangulatePolygon(pPolygon, triangules);

                for (int a = 0, iTriangleCount(triangules.size()); a < iTriangleCount; ++a)
                {
                    int nVertexIndexInTriangle;
                    if (!triangules[a]->GetVertexIndex(vPos, nVertexIndexInTriangle))
                    {
                        continue;
                    }
                    triangules[a]->SetPos(nVertexIndexInTriangle, vSnappedPos);
                    BrushPlane updatedPlane;
                    if (triangules[a]->GetComputedPlane(updatedPlane))
                    {
                        triangules[a]->SetPlane(updatedPlane);
                    }
                }

                newPolygons.insert(newPolygons.end(), triangules.begin(), triangules.end());
            }
            else
            {
                pPolygon->SetPos(nVertexIndex, vSnappedPos);
            }
        }
    }

    for (int i = 0, iPolygonCount(oldPolygons.size()); i < iPolygonCount; ++i)
    {
        GetModel()->RemovePolygon(oldPolygons[i]);
    }

    for (int i = 0, iPolygonCount(newPolygons.size()); i < iPolygonCount; ++i)
    {
        GetModel()->AddPolygon(newPolygons[i], CD::eOpType_Union);
    }

    GetModel()->ResetDB(CD::eDBRF_ALL);

    return vSnappedPos;
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_SnapToGrid, CD::eToolGroup_Misc, "Snap to Grid", SnapToGridTool)