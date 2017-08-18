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
#include "ObjectModeTool.h"
#include "EditMode/ObjectMode.h"
#include "Core/Model.h"
#include "ViewManager.h"
#include "Tools/DesignerTool.h"
#include "RotateTool.h"

void ObjectModeTool::Enter()
{
    BaseTool::Enter();

    SetEditMode();

    if (GetModel())
    {
        GetModel()->SetShelf(0);
        if (GetModel()->GetPolygonCount() > 0)
        {
            GetIEditor()->ShowTransformManipulator(false);
            CD::GetDesigner()->CreateObjectGizmo();
        }
    }

    if (GetIEditor()->GetEditMode() == eEditModeRotateCircle)
    {
        GetIEditor()->SetEditMode(eEditModeRotate);
    }

    m_bSelectedAnother = false;
}

void ObjectModeTool::Leave()
{
    if (m_pObjectMode)
    {
        delete m_pObjectMode;
        m_pObjectMode = NULL;
    }

    DynArray<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);
    for (int i = 0, iObjectCount(objects.size()); i < iObjectCount; ++i)
    {
        objects[i]->ClearFlags(OBJFLAG_HIGHLIGHT);
    }

    BaseTool::Leave();
}

void ObjectModeTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnSelectionChange:
    {
        CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection();
        if (pGroup->GetCount() == 1 && pGroup->GetObject(0) == GetBaseObject())
        {
            m_bSelectedAnother = false;
        }
        else
        {
            m_bSelectedAnother = true;
        }
    }
    break;
    case eNotify_OnEditModeChange:
        SetEditMode();
        break;
    }
}

void ObjectModeTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_bSelectedAnother = false;
    m_pObjectMode->MouseCallback(view, eMouseLDown, point, nFlags);
}

void ObjectModeTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_pObjectMode->MouseCallback(view, eMouseLUp, point, nFlags);
    if (m_bSelectedAnother)
    {
        DesignerTool* pDesignerTool = CD::GetDesigner();
        if (pDesignerTool)
        {
            pDesignerTool->EndEditTool();
        }
    }
}

void ObjectModeTool::OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_pObjectMode->MouseCallback(view, eMouseLDblClick, point, nFlags);
}

void ObjectModeTool::OnRButtonDown(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_pObjectMode->MouseCallback(view, eMouseRDown, point, nFlags);
}

void ObjectModeTool::OnRButtonUp(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_pObjectMode->MouseCallback(view, eMouseRUp, point, nFlags);
}

void ObjectModeTool::OnMButtonDown(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_pObjectMode->MouseCallback(view, eMouseMDown, point, nFlags);
}

void ObjectModeTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& p)
{
    QPoint point = p;
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    m_pObjectMode->MouseCallback(view, eMouseMove, point, nFlags);
}

bool ObjectModeTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->GetObjectManager()->ClearSelection();
        return false;
    }

    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return true;
    }

    m_pObjectMode->OnKeyDown(view, nChar, nRepCnt, nFlags);
    return true;
}

void ObjectModeTool::OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value)
{
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    return m_pObjectMode->OnManipulatorDrag(view, pManipulator, p0, p1, value);
}

void ObjectModeTool::Display(DisplayContext& dc)
{
    BaseTool::Display(dc);
    DESIGNER_ASSERT(m_pObjectMode);
    if (!m_pObjectMode)
    {
        return;
    }
    Matrix34 tm = dc.GetMatrix();
    dc.PopMatrix();
    m_pObjectMode->Display(dc);
    dc.PushMatrix(tm);
}

void ObjectModeTool::SetEditMode()
{
    SAFE_DELETE(m_pObjectMode);

    auto selection = GetIEditor()->GetSelection();
    if (GetIEditor()->GetEditMode() == eEditModeRotate && selection && selection->GetCount() > 0)
    {
        m_pObjectMode = new CRotateTool(selection->GetObject(0));
    }
    else
    {
        m_pObjectMode = new CObjectMode;
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Object, CD::eToolGroup_BasicSelection, "Object", ObjectModeTool)