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
#include "StdAfx.h"
#include "SelectTool.h"
#include "Viewport.h"
#include "DesignerTool.h"
#include "Core/Undo.h"
#include "Core/Model.h"
#include "RemoveTool.h"
#include "Core/PolygonDecomposer.h"
#include "Core/BrushHelper.h"
#include "UIFactory.h"

using namespace CD;

void SelectTool::Enter()
{
    BaseTool::Enter();
    if (GetModel())
    {
        GetModel()->ResetDB(CD::eDBRF_ALL);
    }

    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();
    pSelected->RemoveInvalidElements();

    UpdateSelectionMeshFromSelectedElementList(GetMainContext());

    if (GetIEditor()->GetEditMode() == eEditModeRotateCircle)
    {
        GetIEditor()->SetEditMode(eEditModeRotate);
        UpdateTMManipulatorBasedOnElements(pSelected);
    }
}

void SelectTool::OnLButtonDown(CViewport* view, UINT nFlags, CPoint point)
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);
    int nPolygonIndex(0);

    GetIEditor()->BeginUndo();
    CD::GetDesignerTool()->StoreSelectionUndo();

    bool bOnlyUseSelectionCube = GetKeyState(VK_SPACE) & (1 << 15);
    bool bRemoveSelectedElements = GetKeyState(VK_MENU) & (1 << 15);

    ElementManager pickedElements;
    if (!bRemoveSelectedElements && !pickedElements.Pick(GetBaseObject(), GetModel(), view, point, m_nPickFlag, bOnlyUseSelectionCube, &m_PickedPosAsLMBDown))
    {
        GetIEditor()->ShowTransformManipulator(false);
    }

    bool bPicked = pickedElements.IsEmpty() ? false : true;
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

    if (nFlags & MK_CONTROL)
    {
        m_InitialSelectionElementsInRectangleSel = *pSelected;

        if (!bPicked)
        {
            m_SelectionType = eST_RectangleSelection;
            view->SetSelectionRectangle(point, point);
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
            view->SetSelectionRectangle(point, point);
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
            view->SetSelectionRectangle(point, point);
        }
        else
        {
            m_SelectionType = eST_JustAboutToTransformSelectedElements;
        }
        m_MouseDownPos = point;
        pSelected->Add(pickedElements);
    }

    UpdateTMManipulatorBasedOnElements(CD::GetDesignerTool()->GetSelectedElements());
    UpdateSelectionMeshFromSelectedElementList(GetMainContext());
    ResetDesignerRejectedEdgeList(GetMainContext());
}

void SelectTool::OnLButtonUp(CViewport* view, UINT nFlags, CPoint point)
{
    GetIEditor()->AcceptUndo("Designer : Selection");

    if (m_SelectionType == eST_RectangleSelection || m_SelectionType == eST_EraseSelectionInRectangle)
    {
        view->SetSelectionRectangle(CPoint(0, 0), CPoint(0, 0));
        UpdateTMManipulatorBasedOnElements(CD::GetDesignerTool()->GetSelectedElements());
    }

    m_SelectionType = eST_Nothing;
}

void SelectTool::UpdateSelectionMeshFromSelectedElementList(CD::SMainContext& mc)
{
    if (!mc.pModel || !mc.pBrush)
    {
        return;
    }

    std::vector<CD::PolygonPtr> polygonlist;

    ElementManager* pSelectedElement = CD::GetDesignerTool()->GetSelectedElements();
    for (int k = 0, iElementSize(pSelectedElement->GetCount()); k < iElementSize; ++k)
    {
        if (pSelectedElement->Get(k).IsFace())
        {
            polygonlist.push_back(pSelectedElement->Get(k).m_pPolygon);
        }
    }

    if (m_pSelectionMesh == NULL)
    {
        m_pSelectionMesh = new PolygonMesh;
    }

    int renderFlag = mc.pBrush->GetRenderFlags();
    float viewDist = mc.pBrush->GetViewDistanceMultiplier();
    uint32 minSpec = mc.pObject->GetMinSpec();
    uint32 materialLayerMask = mc.pObject->GetMaterialLayersMask();
    BrushMatrix34 worldTM = mc.pObject->GetWorldTM();

    m_pSelectionMesh->SetPolygons(polygonlist, false, worldTM, renderFlag, viewDist, minSpec, materialLayerMask);
}

void SelectTool::OnMouseMove(CViewport* view, UINT nFlags, CPoint point)
{
    bool bOnlyUseSelectionCube = GetKeyState(VK_SPACE) & (1 << 15);
    ElementManager pickedElements;
    bool bPicked = pickedElements.Pick(GetBaseObject(), GetModel(), view, point, m_nPickFlag, bOnlyUseSelectionCube, NULL);

    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

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
            UpdateSelectionMeshFromSelectedElementList(GetMainContext());
            ResetDesignerRejectedEdgeList(GetMainContext());
        }
    }
}

void SelectTool::SelectElementsInRectangle(CViewport* view, CPoint point, bool bOnlyUseSelectionCube)
{
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

    CRect rc(m_MouseDownPos, point);
    view->SetSelectionRectangle(rc.TopLeft(), rc.BottomRight());
    if (std::abs(rc.Width()) < 2 || std::abs(rc.Height()) < 2)
    {
        return;
    }

    GetModel()->ClearExcludedEdgesInDrawing();
    *pSelected = m_InitialSelectionElementsInRectangleSel;

    CRect selectionRect = view->GetSelectionRectangle();
    CD::PolygonPtr pRectPolygon = CD::MakePolygonFromRectangle(selectionRect);

    if (selectionRect.top != selectionRect.bottom && selectionRect.left != selectionRect.right)
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

    UpdateSelectionMeshFromSelectedElementList(GetMainContext());
}

void SelectTool::EraseElementsInRectangle(CViewport* view, CPoint point, bool bOnlyUseSelectionCube)
{
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

    CRect rc(m_MouseDownPos, point);
    view->SetSelectionRectangle(rc.TopLeft(), rc.BottomRight());
    if (std::abs(rc.Width()) < 2 || std::abs(rc.Height()) < 2)
    {
        return;
    }

    GetModel()->ClearExcludedEdgesInDrawing();
    *pSelected = m_InitialSelectionElementsInRectangleSel;

    CRect selectionRect = view->GetSelectionRectangle();

    if (selectionRect.top != selectionRect.bottom && selectionRect.left != selectionRect.right)
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

    UpdateSelectionMeshFromSelectedElementList(GetMainContext());
}

bool SelectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();
    if (nChar == VK_ESCAPE)
    {
        if (!pSelected->IsEmpty())
        {
            GetIEditor()->ShowTransformManipulator(false);
            pSelected->Clear();
            m_pSelectionMesh = NULL;
            ResetDesignerRejectedEdgeList(GetMainContext());
        }
        else
        {
            GetModel()->ClearExcludedEdgesInDrawing();
            CD::GetDesignerTool()->SwitchTool(CD::eDesigner_Object);
        }
    }
#ifdef DEBUG
    else if (nChar == VK_RETURN)
    {
        IPolygonDebuggerDlg* dlg = (IPolygonDebuggerDlg*)uiFactory::the().Create("PolygonDebugger");
        ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();
        int nCount = 0;
        for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
        {
            if ((*pSelected)[i].m_pPolygon)
            {
                CString buff;
                buff.Format("R #%d", ++nCount);
                dlg->AddPolygon((*pSelected)[i].m_pPolygon.get(), buff);
            }
        }
        dlg->Open();

        for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
        {
            if (!(*pSelected)[i].m_pPolygon)
            {
                continue;
            }

            std::vector<BrushVec3> vertexList;
            std::vector<BrushVec3> normalList;
            std::vector<SMeshFace> faceList;
            PolygonDecomposer decomposer;
            decomposer.TriangulatePolygon((*pSelected)[i].m_pPolygon, vertexList, normalList, faceList);
        }
    }
#endif
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

    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

    if (gDesignerSettings.bHighlightElements)
    {
        pSelected->DisplayHighlightElements(GetBaseObject(), GetModel(), dc, m_nPickFlag);
    }

    pSelected->Display(GetBaseObject(), dc);
}

void SelectTool::ResetDesignerRejectedEdgeList(CD::SMainContext& mc)
{
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

    mc.pModel->ClearExcludedEdgesInDrawing();

    for (int i = 0, iElementSize(pSelected->GetCount()); i < iElementSize; ++i)
    {
        const SElement& element = pSelected->Get(i);
        if (element.IsEdge())
        {
            DESIGNER_ASSERT(element.m_Vertices.size() == 2);
            mc.pModel->AddExcludedEdgeInDrawing(BrushEdge3D(element.m_Vertices[0], element.m_Vertices[1]));
        }
        else if (element.IsFace() && element.m_pPolygon->IsOpen())
        {
            for (int a = 0, iEdgeCount(element.m_pPolygon->GetEdgeCount()); a < iEdgeCount; ++a)
            {
                BrushEdge3D e = element.m_pPolygon->GetEdge(a);
                mc.pModel->AddExcludedEdgeInDrawing(e);
            }
        }
    }
}

void SelectTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    BaseTool::OnEditorNotifyEvent(event);

    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();

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
        UpdateSelectionMeshFromSelectedElementList(GetMainContext());
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
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();
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
                element.m_Vertices.push_back(pPolygon->GetVertex(i));
            }
            element.m_pPolygon = pPolygon;
            element.m_pObject = GetBaseObject();
            elements.Add(element);
        }
        pSelected->Add(elements);
    }

    UpdateSelectionMeshFromSelectedElementList(GetMainContext());
    ResetDesignerRejectedEdgeList(GetMainContext());
    UpdateTMManipulatorBasedOnElements(pSelected);
}

CString SelectTool::GetStatusText() const
{
    ElementManager* pSelected = CD::GetDesignerTool()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return BaseTool::GetStatusText();
    }

    CString str = pSelected->GetElementsInfoText();
    str += " Selected.";

    return str;
}