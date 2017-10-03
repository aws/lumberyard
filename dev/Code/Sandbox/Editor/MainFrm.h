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
#ifndef CRYINCLUDE_EDITOR_MAINFRM_H
#define CRYINCLUDE_EDITOR_MAINFRM_H
#include "Controls/RollupBar.h"
#include "Controls/RollupCtrl.h"

#include "Include/IViewPane.h"
#include <AzCore/std/smart_ptr/shared_ptr.h>

#define MAINFRM_LAYOUT_NORMAL "NormalLayout"
#define MAINFRM_LAYOUT_PREVIEW "PreviewLayout"
#define WM_FRAME_CAN_CLOSE (WM_APP + 1000)
#define IDW_VIEW_ROLLUP_BAR (AFX_IDW_CONTROLBAR_FIRST + 20)
#define IDW_VIEW_CONSOLE_BAR (AFX_IDW_CONTROLBAR_FIRST + 21)

class CMission;
class CViewPaneManager;
class CPanelDisplayLayer;
struct ICVar;
class CMainStatusBar;
class QAction;

class MainWindow;

class SANDBOX_API CMainFrame
    : public CFrameWnd
    , public IEditorNotifyListener
{
    friend class MainWindow;

public:
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

    // Attributes
public:
    // Operations
    //! Show window and restore saved state.
    void ShowWindowEx(int nCmdShow);
public:
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL DestroyWindow();
    virtual void ActivateFrame(int nCmdShow);
    virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext);

protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CMainFrame();

    void SetSelectionName(const CString& name);
    void IdleUpdate();
    //! Edit keyboard shortcuts.
    void EditAccelerator();
    //! Put external tools to menu.
    void UpdateToolsMenu();
    void UpdateExternalsTools();
    //! Reset timers used for auto saving.
    void ResetAutoSaveTimers(bool bForceInit = false);
    void ResetBackgroundUpdateTimer();

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener
    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    CViewPaneManager* GetViewPaneManager();
protected:
    static void OnBackgroundUpdatePeriodChanged(ICVar* pVar);

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnActivate(UINT state, CWnd* owner, BOOL minimized);
    afx_msg void OnCustomize();
    afx_msg int OnCreateControl(LPCREATECONTROLSTRUCT lpCreateControl);
    afx_msg void OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateControlBar(CCmdUI* pCmdUI);

    DECLARE_MESSAGE_MAP()

    static void Command_Open_MaterialEditor();
    static void Command_Open_CharacterEditor();
    static void Command_Open_DataBaseView();
    static void Command_Open_TrackView();
    static void Command_Open_LightmapCompiler();
    static void Command_Open_SelectObjects();
    static void Command_Open_FlowGraph();
    static void Command_Open_TimeOfDay();
    static void Command_Open_FacialEditor();
    static void Command_Open_SmartObjectsEditor();
    static void Command_Open_DialogSystemEditor();
    static void Command_Open_VisualLogViewer();
    static void Command_Open_TerrainEditor();
    static void Command_Open_TerrainTextureLayers();
    static void Command_Open_LiveMocap();
    static void Command_Open_TextureBrowser();
    static void Command_Open_AssetBrowser();
    static void Command_Open_AnimActionEditor();
    static void Command_Open_LayerEditor();

    static void Command_Close_MaterialEditor();
    static void Command_Close_CharacterEditor();
    static void Command_Close_DataBaseView();
    static void Command_Close_TrackView();
    static void Command_Close_LightmapCompiler();
    static void Command_Close_SelectObjects();
    static void Command_Close_FlowGraph();
    static void Command_Close_TimeOfDay();
    static void Command_Close_FacialEditor();
    static void Command_Close_SmartObjectsEditor();
    static void Command_Close_DialogSystemEditor();
    static void Command_Close_VisualLogViewer();
    static void Command_Close_TerrainEditor();
    static void Command_Close_TerrainTextureLayers();
    static void Command_Close_LiveMocap();
    static void Command_Close_TextureBrowser();
    static void Command_Close_AssetBrowser();
    static void Command_Close_AnimActionEditor();
    static void Command_Close_LayerEditor();
#ifdef SEG_WORLD
    static void Command_Close_GridMapEditor();
#endif

protected:
    // Added this variable for quick access to this particular toolbar.
    CXTPToolBar* m_pEditToolBar;
    CReBar m_wndReBar;
    CToolBar m_wndToolBar;
    CToolBar m_objectModifyBar;
    CToolBar m_missionToolBar;
    CToolBar m_wndTerrainToolBar;
    CToolBar m_wndAvoToolBar;
    CXTFlatComboBox m_missions;
    CPanelDisplayLayer* m_pLayerPanel;
    CViewPaneManager* m_viewPaneManager;
    CString m_selectionName;
    class CObjectLayer* m_currentLayer;
    int m_autoSaveTimer;
    int m_autoRemindTimer;
    int m_backgroundUpdateTimer;
    bool m_bIdleRedrawWindow;
    //! names of available pane layout filenames in the Layouts folder.
    std::vector< CString > m_layoutFilenames;

    std::vector<IViewPaneClass*> m_pluginViewPaneClasses;

private:
    void RequireGemForControl(const CString& uuid, const CString& version, UINT controlId);
};

class CCustomControlSplitButtonPopup
    : public CXTPControlPopup
{
public:
    DECLARE_XTP_CONTROL(CCustomControlSplitButtonPopup)

    CCustomControlSplitButtonPopup()
    {
        m_controlType = xtpControlSplitButtonPopup;
    }

    virtual BOOL OnSetPopup(BOOL bPopup)
    {
        return TRUE;
    }
};

class CCustomControlButtonPopup
    : public CXTPControlPopup
{
public:
    DECLARE_XTP_CONTROL(CCustomControlButtonPopup)

    CCustomControlButtonPopup()
    {
        m_controlType = xtpControlButtonPopup;
    }

    virtual BOOL OnSetPopup(BOOL bPopup)
    {
        return TRUE;
    }
};

#endif // CRYINCLUDE_EDITOR_MAINFRM_H
