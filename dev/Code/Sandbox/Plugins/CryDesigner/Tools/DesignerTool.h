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

#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   DesignerTool.h
//  Created:     8/12/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/Model.h"
#include "Core/ModelCompiler.h"
#include "Objects/DesignerObject.h"
#include "Tools/ToolCommon.h"
#include "ToolFactory.h"
#include "IDataBaseManager.h"
#include "Core/PolygonMesh.h"
#include "Objects/AxisGizmo.h"

class BaseTool;
class ElementManager;

namespace CD
{
    class Model;
};

class DesignerTool
    : public CEditTool
    , public IDataBaseManagerListener
{
    Q_OBJECT
public:
    Q_INVOKABLE DesignerTool();

    static void RegisterTool(CRegistrationContext& rc);
    static void UnregisterTool(CRegistrationContext& rc);

    virtual bool Activate(CEditTool* pPreviousTool) override;

    virtual void BeginEditParams(IEditor* ie, int flags) override;
    virtual void EndEditParams() override;

    void Display(DisplayContext& dc) override;
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags) override;

    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

    void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const Vec3& value) override;
    virtual void OnManipulatorMouseEvent(CViewport* view, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo = false) override;

    void GetAffectedObjects(DynArray<CBaseObject*>& outAffectedObjects) override;

    CD::Model* GetModel() const;
    CD::ModelCompiler* GetCompiler() const;
    CBaseObject* GetBaseObject() const { return m_pBaseObject; }

    virtual void SetUserData(const char* key, void* userData) override;
    void SetBaseObject(CBaseObject* pBaseObject);

    virtual bool IsNeedMoveTool() { return true; }

    void LeaveCurrentTool();
    void EnterCurrentTool();

    void SelectAllElements();
    void SetSubMatID(int nSubMatID, CD::Model* pModel);
    void MaterialChanged();

    BaseTool* GetCurrentTool() const;
    bool SwitchTool(CD::EDesignerTool tool, bool bForceChange = false, bool bAllowMultipleMode = true);
    void SwitchToSelectTool() { SwitchTool(m_PreviousSelectTool); }
    bool EndCurrentEditSession();
    void SwitchToPrevTool()
    {
        if (m_PreviousTool == CD::eDesigner_UVMapping || m_PreviousTool == CD::eDesigner_SmoothingGroup)
        {
            SwitchTool(m_PreviousTool);
        }
        else
        {
            SwitchToSelectTool();
        }
    }
    CD::EDesignerTool GetPrevTool() { return m_PreviousTool; }
    CD::EDesignerTool GetPrevSelectTool() { return m_PreviousSelectTool; }
    void SetPrevTool(CD::EDesignerTool tool) { m_PreviousTool = tool; }

    void OnEditorNotifyEvent(EEditorNotifyEvent event);
    bool IsDisplayGrid() override;
    bool IsUpdateUIPanel() override;
    bool IsMoveToObjectModeAfterEnd() override { return false; }
    bool IsCircleTypeRotateGizmo() override;

    void DeleteObjectIfEmpty();
    void EndEditTool() { m_bEndEditTool = true; }

    IDesignerPanel* GetDesignerPanel() const { return m_pMenuPanel; }

    ElementManager* GetSelectedElements() { return m_pSelectedElements.get(); }
    void StoreSelectionUndo();

    void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) override;
    int GetCurrentSubMatID() const;

    void DisableTool(IDesignerPanel* pEditPanel, CD::EDesignerTool tool);

    void CreateDesignerPanels();
    void DestroyDesignerPanels();

    void UpdateSelectionMesh(CD::PolygonPtr pPolygon, CD::ModelCompiler* pCompiler, CBaseObject* pObj,  bool bForce = false);
    void ClearPolygonSelections(){m_pSelectionMesh = NULL; }
    void UpdateSelectionMeshFromSelectedElements(CD::SMainContext& mc);
    _smart_ptr<CD::PolygonMesh> GetSelectionMesh(){return m_pSelectionMesh; }
    void ReleaseSelectionMesh();

    void CreateObjectGizmo();
    void ReleaseObjectGizmo();
    bool SelectDesignerObject(const QPoint& point);

    void UpdateTMManipulator(const BrushVec3& localPos, const BrushVec3& localNormal);
    void UpdateTMManipulatorBasedOnElements(ElementManager* elements);

    BaseTool* GetTool(CD::EDesignerTool tool) const;

    static const GUID& GetClassID();

protected:

    virtual ~DesignerTool();
    // Delete itself.
    void DeleteThis() { delete this; };

protected:

    void UpdateStatusText();
    void RecordUndo();
    void RegisterTools();

    bool SetSelectionDesignerMode(CD::EDesignerTool tool, bool bAllowMultipleMode = true);
    void RegisterTool(BaseTool* pTool);
    void SetCheckButton(CD::EDesignerTool tool, int nCheck);

protected:

    CD::EDesignerTool m_Tool;
    CD::EDesignerTool m_PreviousSelectTool;
    CD::EDesignerTool m_PreviousTool;

    IDesignerPanel* m_pMenuPanel;
    IDesignerSubPanel* m_pSettingPanel;

    _smart_ptr<CD::PolygonMesh> m_pSelectionMesh;
    _smart_ptr<CBaseObject> m_pBaseObject;
    _smart_ptr<CAxisGizmo> m_pObjectGizmo;

    std::unique_ptr<ElementManager> m_pSelectedElements;
    std::set<CD::EDesignerTool> m_DisabledTools;

    bool m_bEndEditTool;

    mutable CD::TOOLDESIGNER_MAP m_ToolMap;
};