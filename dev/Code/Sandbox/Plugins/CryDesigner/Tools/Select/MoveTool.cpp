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
#include "MoveTool.h"
#include "ViewManager.h"
#include "Core/Model.h"
#include "Core/Polygon.h"
#include "ITransformManipulator.h"
#include "Tools/DesignerTool.h"
#include "MovePipeline.h"
#include "Core/BrushHelper.h"

static const BrushFloat kQueryEpsilon = 0.005f;

MoveTool::MoveTool(CD::EDesignerTool tool)
    : SelectTool(tool)
    , m_Pipeline(new MovePipeline)
    , m_bManipulatingGizmo(false)
{
}

MoveTool::~MoveTool()
{
}

void MoveTool::Enter()
{
    SelectTool::Enter();
    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(CD::GetDesigner()->GetSelectedElements());
    CD::UpdateDrawnEdges(GetMainContext());
    m_bManipulatingGizmo = false;
}

BrushMatrix34 MoveTool::GetOffsetTMOnAlignedPlane(CViewport* pView, const BrushPlane& planeAlighedWithView, const QPoint& prevPos, const QPoint& currentPos)
{
    BrushMatrix34 offsetTM;
    offsetTM.SetIdentity();

    BrushVec3 localPrevRaySrc, localPrevRayDir;
    CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), pView, prevPos, localPrevRaySrc, localPrevRayDir);

    BrushVec3 localCurrentRaySrc, localCurrentRayDir;
    CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), pView, currentPos, localCurrentRaySrc, localCurrentRayDir);

    BrushVec3 prevHitPos;
    m_PlaneAlignedWithView.HitTest(localPrevRaySrc, localPrevRaySrc + localPrevRayDir, NULL, &prevHitPos);

    BrushVec3 currentHitPos;
    m_PlaneAlignedWithView.HitTest(localCurrentRaySrc, localCurrentRaySrc + localCurrentRayDir, NULL, &currentHitPos);

    offsetTM.SetTranslation(currentHitPos - prevHitPos);

    return offsetTM;
}

void MoveTool::InitializeMovementOnViewport(CViewport* pView, UINT nMouseFlags)
{
    m_PlaneAlignedWithView = BrushPlane(m_PickedPosAsLMBDown, m_PickedPosAsLMBDown + pView->GetViewTM().GetColumn0(), m_PickedPosAsLMBDown + pView->GetViewTM().GetColumn2());
    StartTransformation(nMouseFlags & MK_SHIFT);
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);
    UpdateAll(CD::eUT_Mirror);
}

void MoveTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_bManipulatingGizmo)
    {
        return;
    }

    SelectTool::OnLButtonDown(view, nFlags, point);
    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(CD::GetDesigner()->GetSelectedElements());
}

void MoveTool::OnLButtonUp(CViewport* pView, UINT nFlags, const QPoint& point)
{
    if (m_SelectionType == eST_TransformSelectedElements)
    {
        EndTransformation();
    }

    SelectTool::OnLButtonUp(pView, nFlags, point);
}

void MoveTool::OnMouseMove(CViewport* pView, UINT nFlags, const QPoint& point)
{
    if (m_bManipulatingGizmo)
    {
        return;
    }

    if (m_SelectionType == eST_NormalSelection && !(nFlags & MK_CONTROL))
    {
        m_SelectionType = eST_JustAboutToTransformSelectedElements;
        m_MouseDownPos = point;
    }

    if (m_SelectionType == eST_JustAboutToTransformSelectedElements && (nFlags & MK_LBUTTON))
    {
        if (std::abs(m_MouseDownPos.x() - point.x()) > 5 || std::abs(m_MouseDownPos.y() - point.y()) > 5)
        {
            InitializeMovementOnViewport(pView, nFlags);
            m_SelectionType = eST_TransformSelectedElements;
        }
    }

    if (m_SelectionType == eST_TransformSelectedElements)
    {
        TransformSelections(GetOffsetTMOnAlignedPlane(pView, m_PlaneAlignedWithView, m_MouseDownPos, point));
    }
    else
    {
        SelectTool::OnMouseMove(pView, nFlags, point);
    }

    if ((nFlags & MK_CONTROL) || m_SelectionType == eST_TransformSelectedElements)
    {
        CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(CD::GetDesigner()->GetSelectedElements());
    }
}

void MoveTool::TransformSelections(const BrushMatrix34& offsetTM)
{
    CD::SMainContext mc(GetMainContext());

    m_Pipeline->TransformSelections(mc, offsetTM);
    mc.pModel->ClearExcludedEdgesInDrawing();

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);

    UpdateAll(CD::eUT_Mirror);
    GetModel()->ResetDB(CD::eDBRF_ALL, 1);

    m_Pipeline->SetQueryResultsFromSelectedElements(*mc.pSelected);

    if (GetIEditor()->GetEditMode() == eEditModeMove)
    {
        CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(mc.pSelected);
    }

    CD::UpdateDrawnEdges(GetMainContext());

    UpdateAll(CD::eUT_Mesh);
    if (CD::GetDesigner())
    {
        CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
    }
}

void MoveTool::OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value)
{
    if (!m_bManipulatingGizmo)
    {
        return;
    }

    TransformSelections(CD::GetOffsetTM(pManipulator, value, GetWorldTM()));
}

void MoveTool::StartTransformation(bool bSeparate)
{
    GetIEditor()->BeginUndo();
    CD::GetDesigner()->StoreSelectionUndo();
    GetModel()->RecordUndo("Move", GetBaseObject());

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    m_SelectedElementNormal = pSelected->GetNormal(GetModel());
    m_Pipeline->SetModel(GetModel());
    if (bSeparate && pSelected->GetFaceCount() > 0)
    {
        m_Pipeline->InitializeIndependently(*pSelected);
    }
    else
    {
        m_Pipeline->Initialize(*pSelected);
    }
}

void MoveTool::EndTransformation()
{
    m_Pipeline->End();

    BrushVec3 vOffset(CD::GetOffsetToAnother(GetBaseObject(), CD::GetWorldBottomCenter(GetBaseObject())));

    UpdateAll(CD::eUT_CenterPivot | CD::eUT_SyncPrefab | CD::eUT_Mesh);
    GetModel()->ResetDB(CD::eDBRF_Vertex);

    if (gDesignerSettings.bKeepPivotCenter)
    {
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        pSelected->ApplyOffset(vOffset);
    }

    GetIEditor()->AcceptUndo("Designer Move");
}

void MoveTool::OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return;
    }

    m_bHitGizmo = bHitGizmo;
    UpdateCursor(pView, bHitGizmo);

    if (event == eMouseLDown)
    {
        if (!m_bManipulatingGizmo)
        {
            StartTransformation(flags & MK_SHIFT);
            m_bManipulatingGizmo = true;
        }
    }
    else if (event == eMouseLUp)
    {
        if (m_bManipulatingGizmo)
        {
            EndTransformation();
            m_bManipulatingGizmo = false;
        }
    }
}

void MoveTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (GetBaseObject() == NULL)
    {
        return;
    }
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->QueryFromElements(GetModel()).empty())
    {
        return;
    }
    SelectTool::OnEditorNotifyEvent(event);
    switch (event)
    {
    case eNotify_OnEndUndoRedo:
    {
        CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(pSelected);
        m_Pipeline->SetModel(GetModel());
        m_Pipeline->SetQueryResultsFromSelectedElements(*pSelected);
    }
    break;
    }
}

void MoveTool::Transform(CD::SMainContext& mc, const BrushMatrix34& tm, bool bMoveTogether)
{
    MovePipeline pipeline;

    pipeline.SetModel(mc.pModel);

    if (bMoveTogether)
    {
        pipeline.Initialize(*mc.pSelected);
    }
    else
    {
        pipeline.InitializeIndependently(*mc.pSelected);
    }

    pipeline.TransformSelections(mc, tm);

    MODEL_SHELF_RECONSTRUCTOR(mc.pModel);
    mc.pModel->SetShelf(1);
    mc.pModel->ResetDB(CD::eDBRF_ALL, 1);

    pipeline.End();
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Vertex, CD::eToolGroup_BasicSelection, "Vertex", MoveVertexTool)
REGISTER_DESIGNER_TOOL(CD::eDesigner_Edge, CD::eToolGroup_BasicSelection, "Edge", MoveEdgeTool)
REGISTER_DESIGNER_TOOL(CD::eDesigner_Face, CD::eToolGroup_BasicSelection, "Face", MoveFaceTool)
REGISTER_DESIGNER_TOOL(CD::eDesigner_VertexEdge, CD::eToolGroup_BasicSelectionCombination, "VertexEdge", MoveVertexEdgeTool)
REGISTER_DESIGNER_TOOL(CD::eDesigner_VertexFace, CD::eToolGroup_BasicSelectionCombination, "VertexFace", MoveVertexFaceTool)
REGISTER_DESIGNER_TOOL(CD::eDesigner_EdgeFace, CD::eToolGroup_BasicSelectionCombination, "EdgeFace", MoveEdgeFaceTool)
REGISTER_DESIGNER_TOOL(CD::eDesigner_VertexEdgeFace, CD::eToolGroup_BasicSelectionCombination, "VertexEdgeFace", MoveVertexEdgeFaceTool)