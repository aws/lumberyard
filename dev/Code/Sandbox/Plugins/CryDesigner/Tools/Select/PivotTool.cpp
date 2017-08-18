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
#include "PivotTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "ViewManager.h"
#include "Core/ModelDB.h"
#include "Core/BrushHelper.h"

#include "ToolFactory.h"
#include "UIs/PivotPanel.h"

void PivotTool::Enter()
{
    m_nSelectedCandidate = -1;
    m_nPivotIndex = -1;
    m_PivotMoved = false;
    GetModel()->ResetDB(CD::eDBRF_Vertex);

    BaseTool::Enter();

    m_StartingDragManipulatorPos = m_PivotPos = BrushVec3(0, 0, 0);
    GetIEditor()->SetEditMode(eEditModeMove);
    CD::GetDesigner()->UpdateTMManipulator(m_PivotPos, BrushVec3(0, 0, 1));
}

void PivotTool::Leave()
{
    BaseTool::Leave();
    GetIEditor()->ShowTransformManipulator(false);
    if (m_PivotMoved)
    {
        CUndo undo("Designer : Pivot Tool");
        GetModel()->RecordUndo("Designer : Pivot", GetBaseObject());
        CD::PivotToPos(GetBaseObject(), GetModel(), m_PivotPos);
        UpdateAll(CD::eUT_ExceptCenterPivot);
    }
}

void PivotTool::SetSelectionType(EPivotSelectionType selectionType, bool bForce)
{
    if (m_nSelectedCandidate == selectionType && !bForce)
    {
        return;
    }

    m_nPivotIndex = -1;
    m_CandidateVertices.clear();

    if (selectionType == ePST_BoundBox)
    {
        AABB aabb;
        GetBaseObject()->GetLocalBounds(aabb);
        BrushVec3 step = CD::ToBrushVec3((aabb.max - aabb.min) * 0.5f);
        for (int i = 0; i <= 2; ++i)
        {
            for (int j = 0; j <= 2; ++j)
            {
                for (int k = 0; k <= 2; ++k)
                {
                    m_CandidateVertices.push_back(aabb.min + BrushVec3(i * step.x, j * step.y, k * step.z));
                }
            }
        }
    }
    else if (selectionType == ePST_Designer)
    {
        CD::ModelDB* pDB = GetModel()->GetDB();
        pDB->GetVertexList(m_CandidateVertices);
    }
}

void PivotTool::Display(DisplayContext& dc)
{
    dc.SetColor(QColor(0xff, 0xaa, 0xaa));
    dc.DepthTestOff();
    dc.DepthWriteOff();

    dc.PopMatrix();

    for (int i = 0, iCount(m_CandidateVertices.size()); i < iCount; ++i)
    {
        if (m_nSelectedCandidate == i)
        {
            continue;
        }
        BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_CandidateVertices[i]);
        BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldVertexPos);
        dc.DrawSolidBox(CD::ToVec3(vWorldVertexPos - vBoxSize), CD::ToVec3(vWorldVertexPos + vBoxSize));
    }
    if (m_nSelectedCandidate != -1)
    {
        dc.SetColor(RGB(100, 100, 255));
        BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_CandidateVertices[m_nSelectedCandidate]);
        BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldVertexPos);
        dc.DrawSolidBox(CD::ToVec3(vWorldVertexPos - vBoxSize), CD::ToVec3(vWorldVertexPos + vBoxSize));
    }

    dc.PushMatrix(GetWorldTM());

    dc.DepthWriteOn();
    dc.DepthTestOn();
}

void PivotTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_nSelectedCandidate != -1)
    {
        CD::GetDesigner()->UpdateTMManipulator(m_CandidateVertices[m_nSelectedCandidate], BrushVec3(0, 0, 1));
        m_nPivotIndex = m_nSelectedCandidate;
        m_PivotPos = m_CandidateVertices[m_nSelectedCandidate];
        m_PivotMoved = true;                // snapping the pivot point to a new position is equivalent to moving the pivot
    }
}

void PivotTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    Vec3 raySrc, rayDir;
    view->ViewToWorldRay(point, raySrc, rayDir);

    BrushFloat leastT = (BrushFloat)3e10;
    m_nSelectedCandidate = -1;

    for (int i = 0, iCount(m_CandidateVertices.size()); i < iCount; ++i)
    {
        BrushFloat t = 0;
        BrushVec3 vWorldPos = GetWorldTM().TransformPoint(m_CandidateVertices[i]);
        BrushVec3 vBoxSize = CD::GetElementBoxSize(view, view->GetType() != ET_ViewportCamera, vWorldPos);
        if (!CD::GetIntersectionOfRayAndAABB(raySrc, rayDir, AABB(vWorldPos - vBoxSize, vWorldPos + vBoxSize), &t))
        {
            continue;
        }
        if (t < leastT && t > 0)
        {
            m_nSelectedCandidate = i;
            leastT = t;
        }
    }
}

bool PivotTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
    }
    return true;
}

void PivotTool::OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value)
{
    BrushMatrix34 moveTM = CD::GetOffsetTM(pManipulator, value, GetWorldTM());
    m_PivotPos = moveTM.TransformPoint(m_StartingDragManipulatorPos);
    m_PivotMoved = true;
    CD::GetDesigner()->UpdateTMManipulator(m_PivotPos, BrushVec3(0, 0, 1));
}

void PivotTool::OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    if (flags == eMouseLDown)
    {
        m_StartingDragManipulatorPos = m_PivotPos;
    }
}

#include "UIs/PivotPanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Pivot, CD::eToolGroup_BasicSelection, "Pivot", PivotTool, PivotPanel)