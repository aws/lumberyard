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
#include "Tools/Select/SelectTool.h"
#include "Viewport.h"
#include "Tools/DesignerTool.h"
#include "Util/Undo.h"
#include "Core/Model.h"
#include "Tools/Edit/RemoveTool.h"
#include "Core/PolygonDecomposer.h"
#include "Core/BrushHelper.h"
#include "ToolFactory.h"

using namespace CD;

void SelectTool::Enter()
{
    BaseTool::Enter();
    if (GetModel())
    {
        GetModel()->ResetDB(CD::eDBRF_ALL);
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->RemoveInvalidElements();

    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());

    if (GetIEditor()->GetEditMode() == eEditModeRotateCircle)
    {
        GetIEditor()->SetEditMode(eEditModeRotate);
        CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(pSelected);
    }
}

void SelectTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);
    int nPolygonIndex(0);

    GetIEditor()->BeginUndo();
    CD::GetDesigner()->StoreSelectionUndo();

    bool bOnlyUseSelectionCube = GetKeyState(VK_SPACE) & (1 << 15);
    bool bRemoveSelectedElements = GetKeyState(VK_MENU) & (1 << 15);

    ElementManager pickedElements;
    if (!bRemoveSelectedElements && !pickedElements.Pick(GetBaseObject(), GetModel(), view, point, m_nPickFlag, bOnlyUseSelectionCube, &m_PickedPosAsLMBDown))
    {
        GetIEditor()->ShowTransformManipulator(false);
    }

    bool bPicked = pickedElements.IsEmpty() ? false : true;
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    if (nFlags & MK_CONTROL)
    {
        m_InitialSelectionElementsInRectangleSel = *pSelected;

        if (!bPicked)
        {
            m_SelectionType = eST_RectangleSelection;
            view->SetSelectionRectangle(QRect(point, QSize(0, 0)));
            m_MouseDownPos = point;
        }
        else
        {
            m_SelectionType = eST_NormalSelection;
            if (!pSelected->Erase(pickedElements))
            {
                pSelected->Add(pickedElements);
            }
        }
    }
    else if (bRemoveSelectedElements)
    {
        if (!pSelected->IsEmpty())
        {
            m_InitialSelectionElementsInRectangleSel = *pSelected;
            m_SelectionType = eST_EraseSelectionInRectangle;
            view->SetSelectionRectangle(QRect(point, QSize(0, 0)));
            m_MouseDownPos = point;
        }
    }
    else
    {
        pSelected->Clear();
        if (!bPicked)
        {
            m_InitialSelectionElementsInRectangleSel.Clear();
            m_SelectionType = eST_RectangleSelection;
            view->SetSelectionRectangle(QRect(point, QSize(0, 0)));
        }
        else
        {
            m_SelectionType = eST_JustAboutToTransformSelectedElements;
        }
        m_MouseDownPos = point;
        pSelected->Add(pickedElements);
    }

    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(CD::GetDesigner()->GetSelectedElements());
    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
    CD::UpdateDrawnEdges(GetMainContext());
}

void SelectTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    GetIEditor()->AcceptUndo("Designer : Selection");

    if (m_SelectionType == eST_RectangleSelection || m_SelectionType == eST_EraseSelectionInRectangle)
    {
        // Reset selected rectangle.
        view->SetSelectionRectangle(QRect());
        CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(CD::GetDesigner()->GetSelectedElements());
    }

    m_SelectionType = eST_Nothing;
}

void SelectTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    bool bOnlyUseSelectionCube = GetKeyState(VK_SPACE) & (1 << 15);
    ElementManager pickedElements;
    bool bPicked = pickedElements.Pick(GetBaseObject(), GetModel(), view, point, m_nPickFlag, bOnlyUseSelectionCube, NULL);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    if (!m_bHitGizmo)
    {
        UpdateCursor(view, bPicked);
    }

    if (m_SelectionType == eST_RectangleSelection)
    {
        SelectElementsInRectangle(view, point, bOnlyUseSelectionCube);
    }
    else if (m_SelectionType == eST_EraseSelectionInRectangle)
    {
        EraseElementsInRectangle(view, point, bOnlyUseSelectionCube);
    }
    else if ((nFlags & MK_CONTROL) && (nFlags & MK_LBUTTON))
    {
        if (bPicked)
        {
            pSelected->Add(pickedElements);
            CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
            CD::UpdateDrawnEdges(GetMainContext());
        }
    }
}

void SelectTool::SelectElementsInRectangle(CViewport* view, const QPoint& point, bool bOnlyUseSelectionCube)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    view->SetSelectionRectangle(m_MouseDownPos, point);
    QRect rc = view->GetSelectionRectangle();
    if (std::abs(rc.width()) < 2 || std::abs(rc.height()) < 2)
    {
        return;
    }

    GetModel()->ClearExcludedEdgesInDrawing();
    *pSelected = m_InitialSelectionElementsInRectangleSel;

    QRect selectionRect = view->GetSelectionRectangle();
    CD::PolygonPtr pRectPolygon = CD::MakePolygonFromRectangle(selectionRect);

    if (selectionRect.isValid())
    {
        ElementManager selectionList;

        if (m_nPickFlag & CD::ePF_Vertex)
        {
            ModelDB::QueryResult qResult;
            GetModel()->GetDB()->QueryAsRectangle(view, GetWorldTM(), selectionRect, qResult);
            for (int i = 0, iCount(qResult.size()); i < iCount; ++i)
            {
                selectionList.Add(SElement(GetBaseObject(), qResult[i].m_Pos));
            }
        }

        if (m_nPickFlag & CD::ePF_Edge)
        {
            std::vector< std::pair<BrushEdge3D, BrushVec3> > edges;
            GetModel()->QueryIntersectionEdgesWith2DRect(view, GetWorldTM(), pRectPolygon, false, edges);
            for (int i = 0, iCount(edges.size()); i < iCount; ++i)
            {
                selectionList.Add(SElement(GetBaseObject(), edges[i].first));
                GetModel()->AddExcludedEdgeInDrawing(edges[i].first);
            }
        }

        if (m_nPickFlag & CD::ePF_Face)
        {
            std::vector<CD::PolygonPtr> polygonList;
            if (!bOnlyUseSelectionCube)
            {
                GetModel()->QueryIntersectionPolygonsWith2DRect(view, GetWorldTM(), pRectPolygon, false, polygonList);
            }

            if (gDesignerSettings.bHighlightElements)
            {
                for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
                {
                    CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
                    if (CD::DoesSelectionBoxIntersectRect(view, GetWorldTM(), selectionRect, pPolygon))
                    {
                        polygonList.push_back(pPolygon);
                    }
                }
            }

            for (int i = 0, iPolygonCount(polygonList.size()); i < iPolygonCount; ++i)
            {
                selectionList.Add(SElement(GetBaseObject(), polygonList[i]));
            }
        }

        pSelected->Add(selectionList);
    }

    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
}

void SelectTool::EraseElementsInRectangle(CViewport* view, const QPoint& point, bool bOnlyUseSelectionCube)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    view->SetSelectionRectangle(m_MouseDownPos, point);
    QRect rc = view->GetSelectionRectangle();
    if (std::abs(rc.width()) < 2 || std::abs(rc.height()) < 2)
    {
        return;
    }

    GetModel()->ClearExcludedEdgesInDrawing();
    *pSelected = m_InitialSelectionElementsInRectangleSel;

    QRect selectionRect = view->GetSelectionRectangle();

    if (selectionRect.isValid())
    {
        std::vector<SElement> elementsInRect;
        pSelected->FindElementsInRect(selectionRect, view, GetWorldTM(), bOnlyUseSelectionCube, elementsInRect);

        for (int i = 0, iCount(elementsInRect.size()); i < iCount; ++i)
        {
            pSelected->Erase(elementsInRect[i]);
        }

        for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
        {
            if ((*pSelected)[i].IsEdge())
            {
                GetModel()->AddExcludedEdgeInDrawing((*pSelected)[i].GetEdge());
            }
        }
    }

    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
}

bool SelectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (nChar == VK_ESCAPE)
    {
        GetModel()->ClearExcludedEdgesInDrawing();
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
    }
    return true;
}

void SelectTool::UpdateCursor(CViewport* view, bool bPickingElements)
{
    if (!bPickingElements)
    {
        view->SetCurrentCursor(STD_CURSOR_DEFAULT, "");
        return;
    }
    else if (GetIEditor()->GetEditMode() == eEditModeMove)
    {
        view->SetCurrentCursor(STD_CURSOR_MOVE, "");
    }
    else if (GetIEditor()->GetEditMode() == eEditModeRotateCircle || GetIEditor()->GetEditMode() == eEditModeRotate)
    {
        view->SetCurrentCursor(STD_CURSOR_ROTATE, "");
    }
    else if (GetIEditor()->GetEditMode() == eEditModeScale)
    {
        view->SetCurrentCursor(STD_CURSOR_SCALE, "");
    }
}

void SelectTool::Display(DisplayContext& dc)
{
    BaseTool::Display(dc);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    if (gDesignerSettings.bHighlightElements)
    {
        pSelected->DisplayHighlightElements(GetBaseObject(), GetModel(), dc, m_nPickFlag);
    }

    pSelected->Display(GetBaseObject(), dc);
}

void SelectTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    BaseTool::OnEditorNotifyEvent(event);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    switch (event)
    {
    case eNotify_OnEndUndoRedo:
        for (int i = 0, iElementSize(pSelected->GetCount()); i < iElementSize; ++i)
        {
            SElement element = pSelected->Get(i);

            if (!element.IsFace())
            {
                continue;
            }

            CD::PolygonPtr pEquivalentPolygon = GetModel()->QueryEquivalentPolygon(element.m_pPolygon);
            if (pEquivalentPolygon)
            {
                element.m_pPolygon = pEquivalentPolygon;
                pSelected->Set(i, element);
            }
        }
        CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
        break;
    }
}

SelectTool::OrganizedQueryResults SelectTool::CreateOrganizedResultsAroundPolygonFromQueryResults(const ModelDB::QueryResult& queryResult)
{
    SelectTool::OrganizedQueryResults organizedQueryResults;
    int iQueryResultSize(queryResult.size());
    for (int i = 0; i < iQueryResultSize; ++i)
    {
        const ModelDB::Vertex& v = queryResult[i];
        for (int k = 0, iMarkListSize(v.m_MarkList.size()); k < iMarkListSize; ++k)
        {
            CD::PolygonPtr pPolygon = v.m_MarkList[k].m_pPolygon;
            DESIGNER_ASSERT(pPolygon);
            if (!pPolygon)
            {
                continue;
            }

            if (pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
            {
                continue;
            }

            organizedQueryResults[v.m_MarkList[k].m_pPolygon].push_back(QueryInput(i, k));
        }
    }
    return organizedQueryResults;
}

void SelectTool::SelectAllElements()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    bool bSelectedElementsExist = !pSelected->IsEmpty();
    pSelected->Clear();

    if (!bSelectedElementsExist)
    {
        ElementManager elements;
        for (int k = 0, iPolygonCount(GetModel()->GetPolygonCount()); k < iPolygonCount; ++k)
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(k);
            SElement element;
            for (int i = 0, iVertexCount(pPolygon->GetVertexCount()); i < iVertexCount; ++i)
            {
                element.m_Vertices.push_back(pPolygon->GetPos(i));
            }
            element.m_pPolygon = pPolygon;
            element.m_pObject = GetBaseObject();
            elements.Add(element);
        }
        pSelected->Add(elements);
    }

    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
    CD::UpdateDrawnEdges(GetMainContext());
    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(pSelected);
}

QString SelectTool::GetStatusText() const
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return BaseTool::GetStatusText();
    }

    QString str = pSelected->GetElementsInfoText();
    str += QObject::tr(" Selected.");

    return str;
}

