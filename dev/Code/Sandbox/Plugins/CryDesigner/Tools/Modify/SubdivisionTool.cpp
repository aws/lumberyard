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
#include "SubdivisionTool.h"
#include "Core/Model.h"
#include "Util/EdgesSharpnessManager.h"
#include "Util/ElementManager.h"
#include "Core/SubdivisionModifier.h"
#include "Core/SmoothingGroup.h"
#include "Core/SmoothingGroupManager.h"
#include "Tools/DesignerTool.h"
#include "Objects/DesignerObject.h"

void SubdivisionTool::Enter()
{
    BaseTool::Enter();
    GetModel()->ClearExcludedEdgesInDrawing();
    m_SelectedEdgesAsEnter.clear();
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iSize(pSelected->GetCount()); i < iSize; ++i)
    {
        const SElement& element = pSelected->Get(i);
        if (element.IsEdge())
        {
            GetModel()->AddExcludedEdgeInDrawing(element.GetEdge());
            m_SelectedEdgesAsEnter.push_back(element.GetEdge());
        }
        else if (element.IsFace() && element.m_pPolygon)
        {
            for (int k = 0, iEdgeCount(element.m_pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
            {
                const BrushEdge3D& edge = element.m_pPolygon->GetEdge(k);
                GetModel()->AddExcludedEdgeInDrawing(edge);
                m_SelectedEdgesAsEnter.push_back(edge);
            }
        }
    }
    pSelected->Clear();
}

void SubdivisionTool::Leave()
{
    BaseTool::Leave();
    m_HighlightedSharpEdges.clear();
    GetModel()->ClearExcludedEdgesInDrawing();
}

void SubdivisionTool::Subdivide(int nLevel, bool bUpdateBrush)
{
    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
    {
        CBaseObject* pObj = pSelection->GetObject(i);
        if (!qobject_cast<DesignerObject*>(pObj))
        {
            continue;
        }
        DesignerObject* pDesignerObj = (DesignerObject*)pObj;
        pDesignerObj->GetModel()->SetSubdivisionLevel(nLevel);
        if (bUpdateBrush)
        {
            CD::UpdateAll(pDesignerObj->MainContext());
        }
    }
}

void SubdivisionTool::HighlightEdgeGroup(const char* edgeGroupName)
{
    EdgeSharpnessManager* pEdgeSharpnessMgr = GetModel()->GetEdgeSharpnessMgr();
    CD::SEdgeSharpness* pEdgeSharpness = pEdgeSharpnessMgr->FindEdgeSharpness(edgeGroupName);
    if (pEdgeSharpness == NULL)
    {
        m_HighlightedSharpEdges.clear();
        GetModel()->ClearExcludedEdgesInDrawing();
        return;
    }
    m_HighlightedSharpEdges = pEdgeSharpness->edges;
    for (int i = 0, iSize(m_HighlightedSharpEdges.size()); i < iSize; ++i)
    {
        GetModel()->AddExcludedEdgeInDrawing(m_HighlightedSharpEdges[i]);
    }
}

void SubdivisionTool::AddNewEdgeTag()
{
    if (m_SelectedEdgesAsEnter.empty())
    {
        return;
    }
    EdgeSharpnessManager* pEdgeSharpnessMgr = GetModel()->GetEdgeSharpnessMgr();
    string newEdgeGroupName = pEdgeSharpnessMgr->GenerateValidName();
    pEdgeSharpnessMgr->AddEdges(newEdgeGroupName, m_SelectedEdgesAsEnter, 1);
    HighlightEdgeGroup(newEdgeGroupName);
    m_SelectedEdgesAsEnter.clear();
    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

void SubdivisionTool::DeleteEdgeTag(const char* name)
{
    if (name == NULL)
    {
        return;
    }
    EdgeSharpnessManager* pEdgeSharpnessMgr = GetModel()->GetEdgeSharpnessMgr();
    pEdgeSharpnessMgr->RemoveEdgeSharpness(name);
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();
    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

void SubdivisionTool::Display(DisplayContext& dc)
{
    dc.SetLineWidth(7);
    dc.SetColor(ColorB(150, 255, 50, 255));
    for (int i = 0, iCount(m_HighlightedSharpEdges.size()); i < iCount; ++i)
    {
        dc.DrawLine(m_HighlightedSharpEdges[i].m_v[0], m_HighlightedSharpEdges[i].m_v[1]);
    }
    dc.DepthTestOn();

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    dc.SetColor(CD::kSelectedColor);
    dc.SetLineWidth(CD::kChosenLineThickness);
    for (int i = 0, iCount(m_SelectedEdgesAsEnter.size()); i < iCount; ++i)
    {
        dc.DrawLine(m_SelectedEdgesAsEnter[i].m_v[0], m_SelectedEdgesAsEnter[i].m_v[1]);
    }
}

void SubdivisionTool::FreezeSubdividedMesh()
{
    int nTessFactor = GetModel()->GetTessFactor();
    int nSubdivisionLevel = GetModel()->GetSubdivisionLevel();

    SubdivisionModifier subdivisionModifier;
    CD::SSubdivisionContext sc = subdivisionModifier.CreateSubdividedMesh(GetModel(), nSubdivisionLevel, nTessFactor);

    _smart_ptr<CD::Model> pModel = sc.fullPatches->CreateModel();
    if (pModel == NULL)
    {
        return;
    }

    pModel->SetTessFactor(0);
    pModel->SetSubdivisionLevel(0);

    if (GetModel()->IsSmoothingSurfaceForSubdividedMeshEnabled())
    {
        SmoothingGroupManager* pSmoothingMgr = pModel->GetSmoothingGroupMgr();
        std::vector<CD::PolygonPtr> polygons;
        pModel->GetPolygonList(polygons);
        pSmoothingMgr->AddSmoothingGroup("SG From Subdivision", new SmoothingGroup(polygons));
    }

    (*GetModel()) = *pModel;

    CD::GetDesigner()->GetSelectedElements()->Clear();
    UpdateAll();
}

void SubdivisionTool::DeleteAllUnused()
{
    EdgeSharpnessManager* pEdgeSharpnessMgr = GetModel()->GetEdgeSharpnessMgr();
    std::vector<BrushEdge3D> deletedEdges;

    for (int i = 0, iCount(pEdgeSharpnessMgr->GetCount()); i < iCount; ++i)
    {
        const CD::SEdgeSharpness& sharpness = pEdgeSharpnessMgr->Get(i);
        for (int k = 0, iEdgeCount(sharpness.edges.size()); k < iEdgeCount; ++k)
        {
            if (!GetModel()->HasEdge(sharpness.edges[k]))
            {
                deletedEdges.push_back(sharpness.edges[k]);
            }
        }
    }

    for (int i = 0, iCount(deletedEdges.size()); i < iCount; ++i)
    {
        pEdgeSharpnessMgr->RemoveEdgeSharpness(deletedEdges[i]);
    }
}

#include "UIs/SubdivisionPanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Subdivision, CD::eToolGroup_Modify, "Subdivision", SubdivisionTool, SubdivisionPanel)