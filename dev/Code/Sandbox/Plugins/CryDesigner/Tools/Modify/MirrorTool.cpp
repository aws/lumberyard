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
#include "MirrorTool.h"
#include "Core/Model.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"

void MirrorTool::ApplyMirror()
{
    CUndo undo("Designer : Apply Mirror");
    GetModel()->RecordUndo("Designer : Apply Mirror", GetBaseObject());

    _smart_ptr<CD::Model> frontPart;
    _smart_ptr<CD::Model> backPart;
    if (GetModel()->Clip(m_SlicePlane, frontPart, backPart, false) == CD::eCPR_CLIPFAILED || backPart == NULL)
    {
        return;
    }

    (*GetModel()) = *backPart;

    for (int i = 0, iPolygonSize(backPart->GetPolygonCount()); i < iPolygonSize; ++i)
    {
        GetModel()->AddPolygon(backPart->GetPolygon(i)->Mirror(m_SlicePlane), CD::eOpType_Add);
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();

    GetModel()->SetMirrorPlane(m_SlicePlane);
    GetModel()->SetModeFlag(GetModel()->GetModeFlag() | CD::eDesignerMode_Mirror);

    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab | CD::eUT_GameResource);

    GetPanel()->Update();
    UpdateGizmo();
}

void MirrorTool::FreezeModel()
{
    CUndo undo("Designer : Release Mirror");
    GetModel()->RecordUndo("Designer : Freeze Designer", GetBaseObject());
    ReleaseMirrorMode(GetModel());
    RemoveEdgesOnMirrorPlane(GetModel());
    GetPanel()->Update();
    UpdateAll();
    UpdateRestTraverseLineSet();
    UpdateGizmo();
}

void MirrorTool::ReleaseMirrorMode(CD::Model* pModel)
{
    pModel->SetModeFlag(pModel->GetModeFlag() & (~CD::eDesignerMode_Mirror));
    for (int i = 0, iPolygonSize(pModel->GetPolygonCount()); i < iPolygonSize; ++i)
    {
        pModel->GetPolygon(i)->RemoveFlags(CD::ePolyFlag_Mirrored);
    }
}

void MirrorTool::RemoveEdgesOnMirrorPlane(CD::Model* pModel)
{
    BrushPlane mirrorPlane = pModel->GetMirrorPlane();

    struct SEdgeAndPlaneStruct
    {
        SEdgeAndPlaneStruct(const BrushEdge3D& edge, const BrushPlane& plane)
            : m_Edge(edge)
            , m_Plane(plane){}
        BrushEdge3D m_Edge;
        BrushPlane m_Plane;
    };

    std::vector<SEdgeAndPlaneStruct> edgePlaneList;

    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
        DESIGNER_ASSERT(pPolygon);
        if (!pPolygon)
        {
            continue;
        }

        for (int k = 0, iEdgeCount(pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
        {
            BrushEdge3D edge = pPolygon->GetEdge(k);
            if (std::abs(mirrorPlane.Distance(edge.m_v[0])) < kDesignerEpsilon && std::abs(mirrorPlane.Distance(edge.m_v[1])) < kDesignerEpsilon)
            {
                edgePlaneList.push_back(SEdgeAndPlaneStruct(edge, pPolygon->GetPlane()));
                edgePlaneList.push_back(SEdgeAndPlaneStruct(edge.GetInverted(), pPolygon->GetPlane().GetInverted()));
            }
        }
    }

    for (int i = 0, iEdgePlaneListCount(edgePlaneList.size()); i < iEdgePlaneListCount; ++i)
    {
        pModel->EraseEdge(edgePlaneList[i].m_Edge);
    }
}

void MirrorTool::Display(DisplayContext& dc)
{
    if (GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror))
    {
        return;
    }
    SliceTool::Display(dc);
}

void MirrorTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    SliceTool::OnEditorNotifyEvent(event);
    switch (event)
    {
    case eNotify_OnEndUndoRedo:
        if (GetPanel())
        {
            GetPanel()->Update();
        }
        break;
    }
}

void MirrorTool::OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    if (event == eMouseLDown)
    {
        if (GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            GetIEditor()->BeginUndo();
            GetModel()->RecordUndo("Designer : Mirror.Move Pivot", GetBaseObject());
        }
        m_PrevGizmoPos = BrushVec3(0, 0, 0);
    }
    else if (event == eMouseLUp)
    {
        if (GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            UpdateAll(CD::eUT_DataBase);
            GetIEditor()->AcceptUndo("Designer : Mirrir.Move Pivot");
        }
    }
}

void MirrorTool::UpdateGizmo()
{
    if (GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror))
    {
        BrushVec3 vAveragePos(0, 0, 0);
        int nCount(0);
        for (int i = 0, iPolygonSize(GetModel()->GetPolygonCount()); i < iPolygonSize; ++i)
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
            if (pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
            {
                continue;
            }
            for (int k = 0, iVertexSize(pPolygon->GetVertexCount()); k < iVertexSize; ++k)
            {
                vAveragePos += pPolygon->GetPos(k);
                ++nCount;
            }
        }
        vAveragePos /= nCount;
        m_GizmoPos = vAveragePos;
    }
    else
    {
        m_GizmoPos = m_CursorPos;
    }
    CD::GetDesigner()->UpdateTMManipulator(m_GizmoPos, BrushVec3(0, 0, 1));
}

bool MirrorTool::UpdateManipulatorInMirrorMode(const BrushMatrix34& offsetTM)
{
    for (int i = 0, iPolygonSize(GetModel()->GetPolygonCount()); i < iPolygonSize; ++i)
    {
        CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
        if (pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
        {
            continue;
        }
        pPolygon->Transform(offsetTM);
    }

    UpdateAll(CD::eUT_Mirror | CD::eUT_Mesh);

    return true;
}

#include "UIs/MirrorPanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Mirror, CD::eToolGroup_Modify, "Mirror", MirrorTool, MirrorPanel)