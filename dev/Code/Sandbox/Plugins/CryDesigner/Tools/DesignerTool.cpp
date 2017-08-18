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
#include <InitGuid.h>
#include "Tools/DesignerTool.h"
#include "ViewManager.h"
#include "Grid.h"
#include "Objects/DesignerObject.h"
#include "Core/ModelCompiler.h"
#include "ToolFactory.h"
#include "Core/Model.h"
#include "Tools/BaseTool.h"
#include "Util/ShortcutManager.h"
#include "Util/Undo.h"
#include "Core/BrushHelper.h"
#include "DisplaySettings.h"
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"
#include "Util/ElementManager.h"
#include "Material/MaterialManager.h"
#include "SurfaceInfoPicker.h"
#include "ToolFactory.h"
#include "ITransformManipulator.h"
#include "IGizmoManager.h"

using namespace CD;

namespace
{
    const GUID BRUSHDESIGNER_TOOL_GUID = {
        0x982FC59C, 0xC4CF, 0x11E0, { 0x9C, 0xDE, 0x60, 0x73, 0x48, 0x24, 0x01, 0x9B }
    };

    IViewPaneClass* g_BrushDesignerClassDesc = nullptr;
}

const GUID& DesignerTool::GetClassID()
{
    return BRUSHDESIGNER_TOOL_GUID;
}

void DesignerTool::RegisterTool(CRegistrationContext& rc)
{
    if (!g_BrushDesignerClassDesc)
    {
        g_BrushDesignerClassDesc = new CQtViewClass<DesignerTool>("EditTool.DesignerTool", "Brush", ESYSTEM_CLASS_EDITTOOL);
        rc.pClassFactory->RegisterClass(g_BrushDesignerClassDesc);
    }
}

void DesignerTool::UnregisterTool(CRegistrationContext& rc)
{
    if (g_BrushDesignerClassDesc)
    {
        rc.pClassFactory->UnregisterClass(g_BrushDesignerClassDesc->ClassID());
        delete g_BrushDesignerClassDesc;
        g_BrushDesignerClassDesc = nullptr;
    }
}

DesignerTool::DesignerTool()
    : m_pSelectedElements(new ElementManager)
    , m_Tool(CD::eDesigner_Invalid)
    , m_PreviousTool(CD::eDesigner_Invalid)
    , m_PreviousSelectTool(CD::eDesigner_Invalid)
    , m_pMenuPanel(NULL)
    , m_pSettingPanel(NULL)
{
    GetIEditor()->GetMaterialManager()->AddListener(this);
}

DesignerTool::~DesignerTool()
{
    DestroyDesignerPanels();
    GetIEditor()->GetMaterialManager()->RemoveListener(this);
}

void DesignerTool::CreateDesignerPanels()
{
    if (!m_pSettingPanel)
    {
        m_pSettingPanel = (IDesignerSubPanel*)uiFactory::the().Create("SettingPanel");
    }

    if (!m_pMenuPanel)
    {
        m_pMenuPanel = (IDesignerPanel*)uiFactory::the().Create("MainPanel");
    }

    DESIGNER_ASSERT(m_pMenuPanel);
    if (m_pMenuPanel)
    {
        m_pMenuPanel->SetDesignerTool(this);
    }
    if (m_pSettingPanel)
    {
        m_pSettingPanel->SetDesignerTool(this);
    }
}

void DesignerTool::DestroyDesignerPanels()
{
    CD::DestroyPanel(&m_pMenuPanel);
    CD::DestroyPanel(&m_pSettingPanel);
}

void DesignerTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    bool bExclusiveModeBeforeSave = false;
    ElementManager* pSelected = GetSelectedElements();
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        gDesignerSettings.bExclusiveMode = false;
        gExclusiveModeSettings.EnableExclusiveMode(gDesignerSettings.bExclusiveMode);
        pSelected->Clear();
        break;
    case eNotify_OnEndUndoRedo:
        if (!CD::IsSelectElementMode(m_Tool))
        {
            pSelected->Clear();
            CD::GetDesigner()->ClearPolygonSelections();
        }
        break;
    case eNotify_OnBeginSceneSave:
    case eNotify_OnBeginSceneOpen:
    case eNotify_OnBeginLoad:
    case eNotify_OnBeginNewScene:
        bExclusiveModeBeforeSave = gDesignerSettings.bExclusiveMode;
        if (gDesignerSettings.bExclusiveMode)
        {
            gDesignerSettings.bExclusiveMode = false;
            gExclusiveModeSettings.EnableExclusiveMode(gDesignerSettings.bExclusiveMode);
        }
        break;

    case eNotify_OnEndSceneSave:
        if (bExclusiveModeBeforeSave)
        {
            gDesignerSettings.bExclusiveMode = true;
            gExclusiveModeSettings.EnableExclusiveMode(gDesignerSettings.bExclusiveMode);
            bExclusiveModeBeforeSave = false;
        }
        break;
    }

    if (GetCurrentTool())
    {
        GetCurrentTool()->OnEditorNotifyEvent(event);
    }
}

bool DesignerTool::Activate(CEditTool* pPreviousTool)
{
    // Remember selection here.
    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (pSel->IsEmpty())
    {
        return false;
    }

    bool bConsistOfOnlyDesigner = true;
    for (int i = 0, iCount(pSel->GetCount()); i < iCount; ++i)
    {
        if (!qobject_cast<DesignerObject*>(pSel->GetObject(i)))
        {
            bConsistOfOnlyDesigner = false;
            break;
        }
    }
    if (!bConsistOfOnlyDesigner)
    {
        GetIEditor()->GetObjectManager()->EndEditParams();
    }

    return true;
}

void DesignerTool::BeginEditParams(IEditor* ie, int flags)
{
    m_pBaseObject = NULL;
    m_bEndEditTool = false;
    m_Tool = CD::eDesigner_Invalid;
    m_PreviousSelectTool = CD::eDesigner_Face;

    ElementManager* pSelectedElements = GetSelectedElements();
    pSelectedElements->Clear();

    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();

    if (!pSelection->IsEmpty() && CD::IsDesignerRelated(pSelection->GetObject(0)))
    {
        SetBaseObject(pSelection->GetObject(0));
        CreateDesignerPanels();
        GetModel()->SetShelf(0);
        if (GetModel()->IsEmpty())
        {
            SwitchTool(CD::eDesigner_Box);
        }
        else
        {
            SwitchTool(CD::eDesigner_Object);
        }
    }

    if (gDesignerSettings.bExclusiveMode)
    {
        if (GetModel()->IsEmpty())
        {
            gDesignerSettings.bExclusiveMode = false;
            gDesignerSettings.Update(true);
        }
        else
        {
            gExclusiveModeSettings.EnableExclusiveMode(true);
        }
    }
}

void DesignerTool::EndEditParams()
{
    LeaveCurrentTool();
    gExclusiveModeSettings.EnableExclusiveMode(false);
    DeleteObjectIfEmpty();
    GetIEditor()->ShowTransformManipulator(false);
    DestroyDesignerPanels();
}

void DesignerTool::SetBaseObject(CBaseObject* pBaseObject)
{
    m_pBaseObject = pBaseObject;
    if (m_pMenuPanel)
    {
        m_pMenuPanel->MaterialChanged();
    }
}

void DesignerTool::Display(DisplayContext& dc)
{
    if (!GetModel() || !GetBaseObject())
    {
        return;
    }

    dc.SetDrawInFrontMode(true);

    dc.PushMatrix(GetBaseObject()->GetWorldTM());

    if (!gDesignerSettings.bHighlightElements || !CD::IsEdgeSelectMode(m_Tool))
    {
        GetModel()->Display(dc);
    }

    if (m_Tool != CD::eDesigner_Invalid && GetTool(m_Tool))
    {
        GetTool(m_Tool)->Display(dc);
    }

    if (gDesignerSettings.bDisplayTriangulation)
    {
        GetCompiler()->DisplayTriangulation(GetBaseObject(), GetModel(), dc);
    }

    dc.PopMatrix();

    dc.SetDrawInFrontMode(false);
}

bool DesignerTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    m_bEndEditTool = false;

    if (m_Tool == CD::eDesigner_Invalid)
    {
        return false;
    }

    _smart_ptr<BaseTool> pTool = GetTool(m_Tool);
    if (pTool == NULL)
    {
        return false;
    }

    if (m_Tool != CD::eDesigner_Object && !GetModel())
    {
        return true;
    }

    view->SetFocus();

    if (event == eMouseLDown)
    {
        if (GetModel() && GetCurrentTool() && CD::IsCreationTool(m_Tool) && GetModel()->IsEmpty() && GetCurrentTool()->IsPhaseFirstStepOnPrimitiveCreation())
        {
            CSurfaceInfoPicker surfacePicker;
            surfacePicker.SetPickOptionFlag(CSurfaceInfoPicker::ePickOption_IncludeFrozenObject);
            SRayHitInfo hitInfo;
            if (surfacePicker.Pick(point, hitInfo, NULL, CSurfaceInfoPicker::ePOG_GeneralObjects | CSurfaceInfoPicker::ePOG_Terrain))
            {
                GetIEditor()->SuspendUndo();
                Matrix34 worldTM = Matrix34::CreateIdentity();
                worldTM.SetTranslation(GetIEditor()->GetViewManager()->GetGrid()->Snap(hitInfo.vHitPos));
                GetBaseObject()->SetWorldTM(worldTM);
                GetIEditor()->ResumeUndo();
            }
        }
        view->SetCapture();
        pTool->OnLButtonDown(view, flags, point);
    }
    else if (event == eMouseLUp)
    {
        pTool->OnLButtonUp(view, flags, point);
        view->ReleaseMouse();
        ReleaseCapture();
    }
    else if (event == eMouseMove)
    {
        ElementManager* pSelected = GetSelectedElements();
        std::vector<DesignerObject*> selectedDesignerObjects = GetSelectedDesignerObjects();
        if (flags == 0 && pTool->EnabledSeamlessSelection() && (gDesignerSettings.bSeamlessEdit || selectedDesignerObjects.size() > 1))
        {
            if (pSelected->IsEmpty())
            {
                SelectDesignerObject(point);
            }
        }
        pTool->OnMouseMove(view, flags, point);
    }
    else if (event == eMouseLDblClick)
    {
        pTool->OnLButtonDblClk(view, flags, point);
    }
    else if (event == eMouseRDown)
    {
        pTool->OnRButtonDown(view, flags, point);
    }
    else if (event == eMouseRUp)
    {
        pTool->OnRButtonUp(view, flags, point);
    }
    else if (event == eMouseMDown)
    {
        pTool->OnMButtonDown(view, flags, point);
    }
    else if (event == eMouseWheel)
    {
        pTool->OnMouseWheel(view, flags, point);
    }

    UpdateStatusText();

    if (m_bEndEditTool)
    {
        GetIEditor()->SetEditTool(NULL);
    }

    return true;
}

bool DesignerTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (m_Tool == CD::eDesigner_Invalid)
    {
        return false;
    }

    if (m_Tool == CD::eDesigner_Object && nChar == VK_DELETE)
    {
        return false;
    }

    if (ShortcutManager::the().Process(nChar))
    {
        return true;
    }

    BaseTool* pTool = GetTool(m_Tool);
    if (pTool == NULL)
    {
        return false;
    }

    bool bContinueEdit = pTool->OnKeyDown(view, nChar, nRepCnt, nFlags);
    if (!bContinueEdit)
    {
        CD::GetDesigner()->ReleaseObjectGizmo();
    }

    return bContinueEdit;
}

void DesignerTool::SetCheckButton(CD::EDesignerTool tool, int nCheck)
{
    IDesignerPanel* pMenuPanel = GetDesignerPanel();
    if (!pMenuPanel)
    {
        return;
    }

    if (CD::IsSelectElementMode(tool))
    {
        int nDesignerMode = (int)tool;
        if (nDesignerMode & CD::eDesigner_Vertex)
        {
            pMenuPanel->SetButtonCheck(CD::eDesigner_Vertex, nCheck);
        }
        if (nDesignerMode & CD::eDesigner_Edge)
        {
            pMenuPanel->SetButtonCheck(CD::eDesigner_Edge, nCheck);
        }
        if (nDesignerMode & CD::eDesigner_Face)
        {
            pMenuPanel->SetButtonCheck(CD::eDesigner_Face, nCheck);
        }
    }
    else
    {
        pMenuPanel->SetButtonCheck(tool, nCheck);
    }
}

bool DesignerTool::SetSelectionDesignerMode(CD::EDesignerTool tool, bool bAllowMultipleMode)
{
    if (!CD::IsSelectElementMode(tool) || m_DisabledTools.find(tool) != m_DisabledTools.end())
    {
        return false;
    }

    int nCombinationDesignerMode = m_Tool;

    bool bPressedCtrl = CheckVirtualKey(Qt::Key_Control) && bAllowMultipleMode;
    if (bPressedCtrl)
    {
        SelectTool* pTool = (SelectTool*)GetCurrentTool();
        if (pTool && pTool->CheckPickFlag(CD::ePF_Vertex) && tool == CD::eDesigner_Vertex)
        {
            SetCheckButton(CD::eDesigner_Vertex, CD::eButton_Unpressed);
            nCombinationDesignerMode &= ~(CD::ePF_Vertex);
        }
        else if (pTool && pTool->CheckPickFlag(CD::ePF_Edge) && tool == CD::eDesigner_Edge)
        {
            SetCheckButton(CD::eDesigner_Edge, CD::eButton_Unpressed);
            nCombinationDesignerMode &= ~(CD::ePF_Edge);
        }
        else if (pTool && pTool->CheckPickFlag(CD::ePF_Face) && tool == CD::eDesigner_Face)
        {
            SetCheckButton(CD::eDesigner_Face, CD::eButton_Unpressed);
            nCombinationDesignerMode &= ~(CD::ePF_Face);
        }
        else if (CD::IsSelectElementMode(m_Tool))
        {
            nCombinationDesignerMode |= tool;
        }
        else
        {
            nCombinationDesignerMode = tool;
        }
    }
    else
    {
        nCombinationDesignerMode = tool;
    }

    if (!CD::IsSelectElementMode((CD::EDesignerTool)nCombinationDesignerMode))
    {
        return false;
    }

    BaseTool* pTool  = GetTool((CD::EDesignerTool)nCombinationDesignerMode);
    if (pTool == NULL)
    {
        return false;
    }

    SelectTool* pNextTool = (SelectTool*)pTool;

    if (!bPressedCtrl)
    {
        SetCheckButton(CD::eDesigner_Vertex, CD::eButton_Unpressed);
        SetCheckButton(CD::eDesigner_Edge, CD::eButton_Unpressed);
        SetCheckButton(CD::eDesigner_Face, CD::eButton_Unpressed);
    }

    SetCheckButton(m_Tool, CD::eButton_Unpressed);
    LeaveCurrentTool();
    m_PreviousTool = CD::IsSelectElementMode(m_Tool) ? m_PreviousTool : m_Tool;
    m_Tool = (CD::EDesignerTool)nCombinationDesignerMode;
    pNextTool->Enter();

    SetCheckButton((CD::EDesignerTool)nCombinationDesignerMode, CD::eButton_Pressed);

    GetIEditor()->GetActiveView()->SetFocus();
    GetIEditor()->GetActiveView()->Invalidate();

    m_PreviousSelectTool = m_Tool;

    return true;
}

bool DesignerTool::SwitchTool(CD::EDesignerTool tool, bool bForceChange, bool bAllowMultipleMode)
{
    if (m_pBaseObject == NULL || m_DisabledTools.find(tool) != m_DisabledTools.end())
    {
        return false;
    }

    if (SetSelectionDesignerMode(tool, bAllowMultipleMode))
    {
        return true;
    }

    if (m_Tool == tool && !bForceChange)
    {
        SetCheckButton(tool, CD::eButton_Pressed);
        return false;
    }

    LeaveCurrentTool();

    if (CD::IsSelectElementMode(m_Tool))
    {
        SetCheckButton(CD::eDesigner_Vertex, CD::eButton_Unpressed);
        SetCheckButton(CD::eDesigner_Edge, CD::eButton_Unpressed);
        SetCheckButton(CD::eDesigner_Face, CD::eButton_Unpressed);
    }

    SetCheckButton(m_Tool, CD::eButton_Unpressed);
    SetCheckButton(tool, CD::eButton_Pressed);

    m_PreviousTool = m_Tool;
    m_Tool = tool;

    _smart_ptr<BaseTool> pTool = GetCurrentTool();
    if (pTool == NULL)
    {
        return false;
    }

    pTool->Enter();

    GetIEditor()->GetActiveView()->SetFocus();
    GetIEditor()->GetActiveView()->Invalidate();

    return true;
}

bool DesignerTool::EndCurrentEditSession()
{
    return SwitchTool(CD::eDesigner_Object);
}

void DesignerTool::DisableTool(IDesignerPanel* pEditPanel, CD::EDesignerTool tool)
{
    if (pEditPanel)
    {
        pEditPanel->DisableButton(tool);
    }
    m_DisabledTools.insert(tool);
}

void DesignerTool::OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const Vec3& value)
{
    BaseTool* pTool = GetCurrentTool();
    if (pTool == NULL)
    {
        return;
    }

    pTool->OnManipulatorDrag(view, pManipulator, p0, p0, value);
}

void DesignerTool::OnManipulatorMouseEvent(CViewport* view, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    BaseTool* pTool = GetCurrentTool();
    if (pTool == NULL)
    {
        return;
    }

    pTool->OnManipulatorMouseEvent(view, pManipulator, event, point, flags, bHitGizmo);
}

BaseTool* DesignerTool::GetCurrentTool() const
{
    if (m_Tool == CD::eDesigner_Invalid)
    {
        return NULL;
    }

    BaseTool* pTool = GetTool(m_Tool);
    if (pTool == NULL)
    {
        return NULL;
    }

    return pTool;
}

void DesignerTool::LeaveCurrentTool()
{
    if (GetCurrentTool())
    {
        GetCurrentTool()->Leave();
    }
}

void DesignerTool::EnterCurrentTool()
{
    if (GetCurrentTool())
    {
        GetCurrentTool()->Enter();
    }
}

void DesignerTool::SelectAllElements()
{
    if (CD::IsSelectElementMode(m_Tool))
    {
        ((SelectTool*)GetTool(m_Tool))->SelectAllElements();
    }
}

CD::Model* DesignerTool::GetModel() const
{
    if (m_pBaseObject == NULL)
    {
        return NULL;
    }

    CD::Model* pModel;
    if (CD::GetModel(m_pBaseObject, pModel))
    {
        return pModel;
    }

    DESIGNER_ASSERT(0);
    return NULL;
}

CD::ModelCompiler* DesignerTool::GetCompiler() const
{
    DESIGNER_ASSERT(m_pBaseObject);
    if (m_pBaseObject == NULL)
    {
        return NULL;
    }

    CD::ModelCompiler* pCompiler;
    if (CD::GetCompiler(m_pBaseObject, pCompiler))
    {
        return pCompiler;
    }

    DESIGNER_ASSERT(0);
    return NULL;
}

void DesignerTool::SetSubMatID(int nSubMatID, CD::Model* pModel)
{
    if (CUndo::IsRecording() && GetBaseObject())
    {
        CUndo::Record(new CD::UndoTextureMapping(GetBaseObject(), "Designer : SetSubMatID"));
    }

    ElementManager* pSelected = GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
            if (pPolygon == NULL)
            {
                continue;
            }
            pPolygon->SetMaterialID(nSubMatID);
        }
    }
    else
    {
        CD::AssignMatIDToSelectedPolygons(nSubMatID);
    }

    if (GetCurrentTool())
    {
        GetCurrentTool()->SetSubMatID(nSubMatID);
    }
}

void DesignerTool::MaterialChanged()
{
    if (m_Tool == CD::eDesigner_CubeEditor)
    {
        GetTool(CD::eDesigner_CubeEditor)->MaterialChanged();
    }
    if (GetDesignerPanel())
    {
        GetDesignerPanel()->MaterialChanged();
    }
}

bool DesignerTool::IsDisplayGrid()
{
    if (m_Tool == CD::eDesigner_Object)
    {
        return true;
    }
    return false;
}

bool DesignerTool::IsUpdateUIPanel()
{
    if (m_PreviousTool == CD::eDesigner_CircleClone || m_PreviousTool == CD::eDesigner_ArrayClone ||
        m_PreviousTool == CD::eDesigner_Merge || m_PreviousTool == CD::eDesigner_Boolean)
    {
        return true;
    }
    return false;
}

bool DesignerTool::IsCircleTypeRotateGizmo()
{
    if (GetCurrentTool())
    {
        return GetCurrentTool()->IsCircleTypeRotateGizmo();
    }
    return false;
}

void DesignerTool::SetUserData(const char* key, void* userData)
{
    Matrix34 objectTM;
    objectTM.SetIdentity();

    CBaseObject* pObject = GetIEditor()->NewObject("Designer");
    if (pObject)
    {
        pObject->SetLocalTM(objectTM);

        int hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
        hideMask = hideMask & ~(pObject->GetType());
        GetIEditor()->GetDisplaySettings()->SetObjectHideMask(hideMask);

        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
        pLayer->SetFrozen(false);
        pLayer->SetVisible(true);
        pLayer->SetModified();

        GetIEditor()->GetObjectManager()->BeginEditParams(pObject, OBJECT_CREATE);

        GetIEditor()->SetModifiedFlag();
        GetIEditor()->SetModifiedModule(eModifiedBrushes);
    }
}

void DesignerTool::DeleteObjectIfEmpty()
{
    if (!GetBaseObject())
    {
        return;
    }

    if (!GetModel() || GetModel()->IsEmpty(0))
    {
        GetIEditor()->SuspendUndo();
        GetIEditor()->GetObjectManager()->DeleteObject(GetBaseObject());
        GetIEditor()->ResumeUndo();
    }
}

void DesignerTool::UpdateStatusText()
{
    if (GetCurrentTool() == NULL)
    {
        return;
    }
    SetStatusText(GetCurrentTool()->GetStatusText());
}

void DesignerTool::StoreSelectionUndo()
{
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CD::UndoSelection(*GetSelectedElements(), GetBaseObject()));
    }
}

void DesignerTool::GetAffectedObjects(DynArray<CBaseObject*>& outAffectedObjects)
{
    if (GetBaseObject())
    {
        outAffectedObjects.push_back(GetBaseObject());
    }
}

void DesignerTool::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    if (GetCurrentTool())
    {
        GetCurrentTool()->OnDataBaseItemEvent(pItem, event);
    }

    if (GetDesignerPanel())
    {
        GetDesignerPanel()->OnDataBaseItemEvent(pItem, event);
    }
}

int DesignerTool::GetCurrentSubMatID() const
{
    if (GetDesignerPanel())
    {
        return GetDesignerPanel()->GetSubMatID();
    }
    return 0;
}

BaseTool* DesignerTool::GetTool(CD::EDesignerTool tool) const
{
    if (m_ToolMap.find(tool) == m_ToolMap.end())
    {
        m_ToolMap[tool] = toolFactory::the().Create(tool);
    }
    return m_ToolMap[tool];
}

void DesignerTool::UpdateSelectionMeshFromSelectedElements(CD::SMainContext& mc)
{
    if (!mc.pModel || !mc.pCompiler)
    {
        return;
    }

    std::vector<CD::PolygonPtr> polygonlist;

    ElementManager* pSelectedElement = CD::GetDesigner()->GetSelectedElements();
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

    int renderFlag = mc.pCompiler->GetRenderFlags();
    float viewDistMult = mc.pCompiler->GetViewDistanceMultiplier();
    uint32 minSpec = mc.pObject->GetMinSpec();
    uint32 materialLayerMask = mc.pObject->GetMaterialLayersMask();
    BrushMatrix34 worldTM = mc.pObject->GetWorldTM();

    m_pSelectionMesh->SetPolygons(polygonlist, false, worldTM, renderFlag, viewDistMult, minSpec, materialLayerMask);
}

void DesignerTool::UpdateSelectionMesh(PolygonPtr pPolygon, CD::ModelCompiler* pCompiler, CBaseObject* pObj, bool bForce)
{
    if (m_pSelectionMesh == NULL)
    {
        m_pSelectionMesh = new PolygonMesh;
    }

    int renderFlag = pCompiler->GetRenderFlags();
    float viewDistMult = pCompiler->GetViewDistanceMultiplier();
    uint32 minSpec = pObj->GetMinSpec();
    uint32 materialLayerMask = pObj->GetMaterialLayersMask();
    BrushMatrix34 worldTM = pObj->GetWorldTM();

    if (pPolygon)
    {
        BrushVec3 vDir = worldTM.TransformVector(pPolygon->GetPlane().Normal()).GetNormalized();
        worldTM.SetTranslation(worldTM.GetTranslation() + vDir * 0.01f);
    }

    m_pSelectionMesh->SetPolygon(pPolygon, bForce, worldTM, renderFlag, viewDistMult, minSpec, materialLayerMask);
}

void DesignerTool::ReleaseSelectionMesh()
{
    if (m_pSelectionMesh)
    {
        m_pSelectionMesh->ReleaseResources();
        m_pSelectionMesh = NULL;
    }
}

void DesignerTool::CreateObjectGizmo()
{
    if (GetIEditor()->GetTransformManipulator())
    {
        return;
    }
    if (CAxisGizmo::GetGlobalAxisGizmoCount() < gSettings.gizmo.axisGizmoMaxCount)
    {
        m_pObjectGizmo = new CAxisGizmo(GetBaseObject());
        GetIEditor()->GetObjectManager()->GetGizmoManager()->AddGizmo(m_pObjectGizmo);
    }
}

void DesignerTool::ReleaseObjectGizmo()
{
    GetIEditor()->ShowTransformManipulator(false);

    if (m_pObjectGizmo)
    {
        GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_pObjectGizmo);
        m_pObjectGizmo = NULL;
    }

    std::vector<CGizmo*> gizmoList;
    IGizmoManager* pGizmoMgr = GetIEditor()->GetObjectManager()->GetGizmoManager();
    for (int i = 0, iCount(pGizmoMgr->GetGizmoCount()); i < iCount; ++i)
    {
        gizmoList.push_back(pGizmoMgr->GetGizmoByIndex(i));
    }

    for (int i = 0, iCount(gizmoList.size()); i < iCount; ++i)
    {
        if (!gizmoList[i])
        {
            continue;
        }
        if (gizmoList[i]->GetBaseObject() == GetBaseObject())
        {
            pGizmoMgr->RemoveGizmo(gizmoList[i]);
        }
    }
}

bool DesignerTool::SelectDesignerObject(const QPoint& point)
{
    if (!GetModel())
    {
        return false;
    }

    CBaseObject* pBaseObject = GetBaseObject();
    if (pBaseObject && pBaseObject->GetType() != OBJTYPE_SOLID)
    {
        return false;
    }

    std::vector<DesignerObject*> selectedObjects;
    std::set<DesignerObject*> selectedObjectSet;
    selectedObjects = CD::GetSelectedDesignerObjects();
    selectedObjectSet.insert(selectedObjects.begin(), selectedObjects.end());

    CSurfaceInfoPicker picker;
    SRayHitInfo hitInfo;
    if (picker.Pick(point, hitInfo, NULL, CSurfaceInfoPicker::ePOG_DesignerObject))
    {
        CBaseObject* pPickedObj = picker.GetPickedObject();

        if (!pPickedObj || GetBaseObject() == pPickedObj)
        {
            return false;
        }

        if (!qobject_cast<DesignerObject*>(pPickedObj))
        {
            return false;
        }

        if (!gDesignerSettings.bSeamlessEdit && selectedObjectSet.size() > 2 && selectedObjectSet.find((DesignerObject*)pPickedObj) == selectedObjectSet.end())
        {
            return false;
        }

        DesignerObject* pNewDesignerObj = (DesignerObject*)pPickedObj;
        if (!pNewDesignerObj->GetCompiler() || !pNewDesignerObj->GetModel())
        {
            return false;
        }

        if (CD::GetDesigner())
        {
            CD::GetDesigner()->SetBaseObject(pNewDesignerObj);
            ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
            pSelected->Clear();
            return true;
        }
    }

    return false;
}

void DesignerTool::UpdateTMManipulatorBasedOnElements(ElementManager* elements)
{
    if (elements->IsEmpty())
    {
        return;
    }
    BrushVec3 averagePos(0, 0, 0);
    int iResultSize(0);
    for (int i = 0, iListSize(elements->GetCount()); i < iListSize; ++i)
    {
        if ((*elements)[i].IsFace() && (*elements)[i].m_pPolygon)
        {
            averagePos += (*elements)[i].m_pPolygon->GetRepresentativePosition();
            ++iResultSize;
        }
        else
        {
            for (int k = 0, iElementSize((*elements)[i].m_Vertices.size()); k < iElementSize; ++k)
            {
                averagePos += (*elements)[i].m_Vertices[k];
                ++iResultSize;
            }
        }
    }
    averagePos /= iResultSize;
    UpdateTMManipulator(averagePos, elements->GetNormal(GetModel()));
}

void DesignerTool::UpdateTMManipulator(const BrushVec3& localPos, const BrushVec3& localNormal)
{
    ITransformManipulator* pManipulator = GetIEditor()->ShowTransformManipulator(true);
    if (pManipulator == NULL)
    {
        return;
    }

    BrushMatrix34 worldTM = ToBrushMatrix34(GetBaseObject()->GetWorldTM());

    BrushMatrix34 localOrthogonalTM = Matrix33::CreateOrthogonalBase(localNormal);
    BrushVec3 worldPos = worldTM.TransformPoint(localPos);

    BrushMatrix34 userTM = GetIEditor()->GetViewManager()->GetGrid()->GetMatrix();
    userTM.SetTranslation(worldPos);

    BrushMatrix34 refFrame(worldTM * localOrthogonalTM);
    refFrame.SetTranslation(worldPos);
    pManipulator->SetTransformation(COORDS_LOCAL, refFrame);
    pManipulator->SetTransformation(COORDS_USERDEFINED, userTM);

    CBaseObject* pObj = GetBaseObject();
    if (pObj && pObj->GetParent())
    {
        BrushMatrix34 parentTM = pObj->GetParent()->GetWorldTM();
        parentTM.SetTranslation(worldPos);
        pManipulator->SetTransformation(COORDS_PARENT, parentTM);
    }
}

#include <Tools/DesignerTool.moc>