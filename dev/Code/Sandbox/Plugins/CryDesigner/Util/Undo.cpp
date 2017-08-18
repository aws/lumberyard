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
#include "Undo.h"
#include "Core/Polygon.h"
#include "Core/BrushHelper.h"
#include "Objects/AreaSolidObject.h"
#include "Objects/DesignerObject.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"

namespace CD
{
    bool DesignerUndo::IsAKindOfDesignerTool(CEditTool* pEditTool)
    {
        return qobject_cast<DesignerTool*>(pEditTool);
    }

    DesignerUndo::DesignerUndo(CBaseObject* pObj, const CD::Model* pModel, const char* undoDescription)
        : m_ObjGUID(pObj->GetId())
        , m_Tool(CD::eDesigner_Object)
    {
        DESIGNER_ASSERT(pModel);
        if (pModel)
        {
            if (undoDescription)
            {
                m_undoDescription = undoDescription;
            }
            else
            {
                m_undoDescription = QStringLiteral("CryDesignerUndo");
            }

            StoreEditorTool();

            m_UndoWorldTM = pObj->GetWorldTM();
            m_undo = new CD::Model(*pModel);
        }
    }

    void DesignerUndo::StoreEditorTool()
    {
        CEditTool* pEditTool = GetIEditor()->GetEditTool();
        if (!IsAKindOfDesignerTool(pEditTool))
        {
            m_Tool = CD::eDesigner_Invalid;
        }
        else
        {
            BaseTool* pSubTool = ((DesignerTool*)pEditTool)->GetCurrentTool();
            if (pSubTool)
            {
                m_Tool = pSubTool->Tool();
            }
            else
            {
                m_Tool = CD::eDesigner_Invalid;
            }
        }
    }

    void DesignerUndo::Undo(bool bUndo)
    {
        CD::SMainContext mc(MainContext());
        if (!mc.IsValid())
        {
            return;
        }

        if (bUndo)
        {
            m_RedoWorldTM = mc.pObject->GetWorldTM();
            m_redo = new CD::Model(*mc.pModel);
            RestoreEditTool(mc.pModel);
        }

        if (m_undo)
        {
            *mc.pModel = *m_undo;
            mc.pObject->SetWorldTM(m_UndoWorldTM);
            CD::UpdateAll(mc);
            m_undo = NULL;
        }
    }

    void DesignerUndo::Redo()
    {
        CD::SMainContext mc(MainContext());
        if (!mc.IsValid())
        {
            return;
        }

        m_UndoWorldTM = mc.pObject->GetWorldTM();
        m_undo = new CD::Model(*mc.pModel);

        RestoreEditTool(mc.pModel);

        if (m_redo)
        {
            *mc.pModel = *m_redo;
            mc.pObject->SetWorldTM(m_RedoWorldTM);
            CD::UpdateAll(MainContext());
            m_redo = NULL;
        }
    }

    CBaseObject* DesignerUndo::GetBaseObject(const GUID& objGUID)
    {
        CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(objGUID);
        if (pObj == NULL)
        {
            return NULL;
        }
        return pObj;
    }

    CD::SMainContext DesignerUndo::MainContext(const GUID& objGUID)
    {
        CBaseObject* pObj = GetBaseObject(objGUID);
        CD::ModelCompiler* pCompiler = NULL;
        CD::GetCompiler(pObj, pCompiler);
        return CD::SMainContext(pObj, pCompiler, GetModel(objGUID));
    }

    CD::Model* DesignerUndo::GetModel(const GUID& objGUID)
    {
        CBaseObject* pObj = GetBaseObject(objGUID);
        if (pObj == NULL)
        {
            return NULL;
        }
        CD::Model* pModel = NULL;
        CD::GetModel(pObj, pModel);
        return pModel;
    }

    ModelCompiler* DesignerUndo::GetCompiler(const GUID& objGUID)
    {
        CBaseObject* pObj = GetBaseObject(objGUID);
        if (pObj == NULL)
        {
            return NULL;
        }
        CD::ModelCompiler* pCompiler = NULL;
        CD::GetCompiler(pObj, pCompiler);
        return pCompiler;
    }

    void DesignerUndo::RestoreEditTool(CD::Model* pModel, REFGUID objGUID, CD::EDesignerTool tool)
    {
        CBaseObject* pSelectedObj = GetBaseObject(objGUID);
        if (!pSelectedObj)
        {
            return;
        }

        bool bSelected(pSelectedObj->IsSelected());
        if (!bSelected)
        {
            GetIEditor()->GetObjectManager()->ClearSelection();
            GetIEditor()->GetObjectManager()->SelectObject(pSelectedObj);
        }

        CEditTool* pEditTool = GetIEditor()->GetEditTool();
        if (pEditTool && IsAKindOfDesignerTool(pEditTool))
        {
            if (tool == CD::eDesigner_Merge ||
                tool == CD::eDesigner_Weld ||
                tool == CD::eDesigner_Pivot ||
                tool == CD::eDesigner_Magnet)
            {
                tool = CD::eDesigner_Object;
            }

            DesignerTool* pDesignerTool = (DesignerTool*)pEditTool;
            pDesignerTool->SetBaseObject(pSelectedObj);
            if (!pDesignerTool->GetDesignerPanel())
            {
                pDesignerTool->CreateDesignerPanels();
            }
            pDesignerTool->SwitchTool(tool);
        }
    }

    UndoSelection::UndoSelection(ElementManager& selectionContext, CBaseObject* pObj, const char* undoDescription)
    {
        SetObjGUID(pObj->GetId());

        if (undoDescription)
        {
            SetDescription(undoDescription);
        }
        else
        {
            SetDescription(selectionContext.GetElementsInfoText());
        }

        CopyElements(selectionContext, m_SelectionContextForUndo);

        int nDesignerMode = 0;

        for (int i = 0, iSelectionCount(selectionContext.GetCount()); i < iSelectionCount; ++i)
        {
            if (selectionContext[i].IsVertex())
            {
                nDesignerMode |= CD::eDesigner_Vertex;
            }
            else if (selectionContext[i].IsEdge())
            {
                nDesignerMode |= CD::eDesigner_Edge;
            }
            else if (selectionContext[i].IsFace())
            {
                nDesignerMode |= CD::eDesigner_Face;
            }
        }

        if (nDesignerMode == 0)
        {
            nDesignerMode = CD::eDesigner_Object;
        }

        SwitchTool((CD::EDesignerTool)nDesignerMode);
    }

    SelectTool* UndoSelection::GetSelectTool()
    {
        CEditTool* pEditTool = GetIEditor()->GetEditTool();
        if (!IsAKindOfDesignerTool(pEditTool))
        {
            return NULL;
        }

        DesignerTool* pDesignerTool = (DesignerTool*)pEditTool;
        if (!pDesignerTool->GetCurrentTool())
        {
            return NULL;
        }

        CD::EDesignerTool dm = pDesignerTool->GetCurrentTool()->Tool();
        if (!CD::IsSelectElementMode(dm))
        {
            return NULL;
        }

        return (SelectTool*)(pDesignerTool->GetCurrentTool());
    }

    void UndoSelection::Undo(bool bUndo)
    {
        CD::SMainContext mc(MainContext());
        RestoreEditTool(mc.pModel);

        SelectTool* pTool = GetSelectTool();
        if (pTool == NULL)
        {
            return;
        }

        ElementManager* pSelected = CD::GetDesigner() ? CD::GetDesigner()->GetSelectedElements() : NULL;
        if (bUndo)
        {
            if (pSelected)
            {
                CopyElements(*pSelected, m_SelectionContextForRedo);
            }
        }

        if (CD::GetDesigner())
        {
            CD::GetDesigner()->ClearPolygonSelections();
        }

        ReplacePolygonsWithExistingPolygonsInModel(m_SelectionContextForUndo);
        if (pSelected)
        {
            pSelected->Set(m_SelectionContextForUndo);
        }

        if (CD::GetDesigner())
        {
            CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(pTool->GetMainContext());
        }
        CD::UpdateDrawnEdges(pTool->GetMainContext());
    }

    void UndoSelection::Redo()
    {
        CD::SMainContext mc(MainContext());
        RestoreEditTool(mc.pModel);

        SelectTool* pTool = GetSelectTool();
        if (pTool == NULL)
        {
            return;
        }

        ElementManager* pSelected = CD::GetDesigner() ? CD::GetDesigner()->GetSelectedElements() : NULL;

        CD::GetDesigner()->ClearPolygonSelections();
        ReplacePolygonsWithExistingPolygonsInModel(m_SelectionContextForRedo);
        if (pSelected)
        {
            pSelected->Set(m_SelectionContextForRedo);
        }
        if (CD::GetDesigner())
        {
            CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(pTool->GetMainContext());
        }
        CD::UpdateDrawnEdges(pTool->GetMainContext());
    }

    void UndoSelection::CopyElements(ElementManager& sourceElements, ElementManager& destElements)
    {
        destElements = sourceElements;
        for (int i = 0, iElementSize(destElements.GetCount()); i < iElementSize; ++i)
        {
            if (!destElements[i].IsFace())
            {
                continue;
            }
            destElements[i].m_pPolygon = destElements[i].m_pPolygon->Clone();
        }
    }

    void UndoSelection::ReplacePolygonsWithExistingPolygonsInModel(ElementManager& elements)
    {
        CD::SMainContext mc(MainContext());
        std::vector<SElement> removedElements;

        for (int i = 0, iCount(elements.GetCount()); i < iCount; ++i)
        {
            if (!elements[i].IsFace())
            {
                continue;
            }

            if (elements[i].m_pPolygon == NULL)
            {
                continue;
            }

            CD::PolygonPtr pEquivalentPolygon = mc.pModel->QueryEquivalentPolygon(elements[i].m_pPolygon);
            if (pEquivalentPolygon == NULL)
            {
                removedElements.push_back(elements[i]);
                continue;
            }

            elements[i].m_pPolygon = pEquivalentPolygon;
        }

        for (int i = 0, iRemovedCount(removedElements.size()); i < iRemovedCount; ++i)
        {
            elements.Erase(removedElements[i]);
        }
    }

    void UndoTextureMapping::Undo(bool bUndo)
    {
        CD::Model* pModel = DesignerUndo::GetModel(m_ObjGUID);
        if (pModel == NULL)
        {
            return;
        }
        if (bUndo)
        {
            SaveDesignerTexInfoContext(m_RedoContext);
        }
        RestoreTexInfo(m_UndoContext);
        DesignerUndo::RestoreEditTool(pModel, m_ObjGUID, CD::eDesigner_UVMapping);
    }

    void UndoTextureMapping::Redo()
    {
        CD::Model* pModel = DesignerUndo::GetModel(m_ObjGUID);
        if (pModel == NULL)
        {
            return;
        }
        SaveDesignerTexInfoContext(m_UndoContext);
        RestoreTexInfo(m_RedoContext);
        DesignerUndo::RestoreEditTool(pModel, m_ObjGUID, CD::eDesigner_UVMapping);
    }


    void UndoTextureMapping::RestoreTexInfo(const std::vector<SContextInfo>& contextList)
    {
        CD::Model* pModel = DesignerUndo::GetModel(m_ObjGUID);
        if (pModel == NULL)
        {
            return;
        }

        MODEL_SHELF_RECONSTRUCTOR(pModel);

        for (int i = 0, iContextCount(contextList.size()); i < iContextCount; ++i)
        {
            const SContextInfo& context = contextList[i];
            CD::PolygonPtr pPolygon = pModel->QueryPolygon(context.m_PolygonGUID);
            if (pPolygon == NULL)
            {
                continue;
            }
            pPolygon->SetTexInfo(context.m_TexInfo);
            pPolygon->SetMaterialID(context.m_MatID);
        }

        CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_ObjGUID);
        if (qobject_cast<DesignerObject*>(pObj))
        {
            DesignerObject* pDesignerObject = ((DesignerObject*)pObj);
            CD::UpdateAll(pDesignerObject->MainContext(), CD::eUT_Mesh | CD::eUT_SyncPrefab);
        }
    }

    void UndoTextureMapping::SaveDesignerTexInfoContext(std::vector<SContextInfo>& contextList)
    {
        CD::Model* pModel(DesignerUndo::GetModel(m_ObjGUID));

        if (!pModel)
        {
            return;
        }

        MODEL_SHELF_RECONSTRUCTOR(pModel);

        contextList.clear();

        for (int k = 0; k < CD::kMaxShelfCount; ++k)
        {
            pModel->SetShelf(k);
            for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
            {
                CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
                if (pPolygon == NULL)
                {
                    continue;
                }
                SContextInfo context;
                context.m_PolygonGUID = pPolygon->GetGUID();
                context.m_MatID = pPolygon->GetMaterialID();
                context.m_TexInfo = pPolygon->GetTexInfo();
                contextList.push_back(context);
            }
        }
    }

    UndoSubdivision::UndoSubdivision(const CD::SMainContext& mc)
    {
        m_ObjGUID = mc.pObject->GetId();
        m_SubdivisionLevelForUndo = mc.pModel->GetSubdivisionLevel();
        m_SubdivisionLevelForRedo = 0;
    }

    void UndoSubdivision::UpdatePanel()
    {
        DesignerTool* pDesignerTool = CD::GetDesigner();
        if (pDesignerTool && pDesignerTool->GetCurrentTool())
        {
            BaseTool* pSubTool = pDesignerTool->GetCurrentTool();
            IBasePanel* pPanel = pSubTool->GetPanel();
            if (pPanel && pSubTool->Tool() == CD::eDesigner_Subdivision)
            {
                pPanel->Update();
            }
        }
    }

    void UndoSubdivision::Undo(bool bUndo)
    {
        CD::SMainContext mc(DesignerUndo::MainContext(m_ObjGUID));
        if (mc.pModel == NULL)
        {
            return;
        }

        if (bUndo)
        {
            m_SubdivisionLevelForRedo = mc.pModel->GetSubdivisionLevel();
        }

        mc.pModel->SetSubdivisionLevel(m_SubdivisionLevelForUndo);
        UpdatePanel();

        if (!mc.pObject || !mc.pCompiler)
        {
            return;
        }

        CD::UpdateAll(mc, CD::eUT_Mesh);
    }

    void UndoSubdivision::Redo()
    {
        CD::SMainContext mc(DesignerUndo::MainContext(m_ObjGUID));
        if (mc.pModel == NULL)
        {
            return;
        }

        mc.pModel->SetSubdivisionLevel(m_SubdivisionLevelForRedo);
        UpdatePanel();

        if (!mc.pObject || !mc.pCompiler)
        {
            return;
        }

        CD::UpdateAll(mc, CD::eUT_Mesh);
    }
};
