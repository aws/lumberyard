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
#include "MainFrm.h"
#include "Commands/CommandManager.h"
#include "CryEdit.h"
#include "CryEditDoc.h"
#include "MainTools.h"
#include "PanelDisplayHide.h"
#include "PanelDisplayRender.h"
#include "PanelDisplayLayer.h"
//#include "QtViewPaneManager.h"
#include "EditMode/SubObjSelectionTypePanel.h"
#include "EditMode/SubObjSelectionPanel.h"
#include "EditMode/SubObjDisplayPanel.h"
#include "ViewManager.h"
#include "ToolBox.h"
#include "UndoDropDown.h"
#include "PropertiesPanel.h"
#include "Material/MaterialManager.h"
#include "AI/AIManager.h"

#include <IBudgetingSystem.h>
#include "ProcessInfo.h"
#include "StringDlg.h"
#include "Util/StringNoCasePredicate.h"
#include "ControlMRU.h"
#include "Controls/CrytekTheme.h"
#include "Util/BoostPythonHelpers.h"
#include "Objects/ObjectLayerManager.h"
#include "TerrainTexture.h"
#include "TerrainLighting.h"
#include "TerrainDialog.h"
#include "IRenderer.h" // IAsyncTextureCompileListener
#include "IResourceCompilerHelper.h" // ERcExitCode

#include "LevelIndependentFileMan.h"
#include <ISourceControl.h>
#include "MainStatusBar.h"
#include "GameEngine.h"
#include "PluginManager.h"
#include "CustomizeKeyboardPage.h"

#include "MainWindow.h"

#include <QAction>
#include <QApplication>

#define AUTOSAVE_TIMER_EVENT 200
#define AUTOREMIND_TIMER_EVENT 201
#define NETWORK_AUDITION_UPDATE_TIMER_EVENT 202
#define BACKGROUND_UPDATE_TIMER_EVENT 204

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainFrame, CXTPFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CXTPFrameWnd)
ON_WM_CREATE()

ON_WM_SIZE()
ON_WM_GETMINMAXINFO()
ON_WM_COPYDATA()
ON_WM_CLOSE()
ON_WM_TIMER()
ON_WM_ACTIVATE()

//////////////////////////////////////////////////////////////////////////
// XT Commands.

ON_XTP_EXECUTE(IDC_SELECTION, OnSelectionChanged)

ON_COMMAND(XTP_ID_CUSTOMIZE, OnCustomize)
ON_XTP_CREATECONTROL()
END_MESSAGE_MAP()

IMPLEMENT_XTP_CONTROL(CCustomControlSplitButtonPopup, CXTPControlPopup)
IMPLEMENT_XTP_CONTROL(CCustomControlButtonPopup, CXTPControlPopup)

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
    : m_viewPaneManager(new CViewPaneManager())
{
    m_autoSaveTimer = 0;
    m_autoRemindTimer = 0;
    m_bIdleRedrawWindow = false;

    m_pEditToolBar = NULL;

    GetIEditor()->RegisterNotifyListener(this);
}

CMainFrame::~CMainFrame()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CMainFrame::OnBackgroundUpdatePeriodChanged(ICVar* pVar)
{
    CMainFrame* pMainFrame = (CMainFrame*)(AfxGetMainWnd());
    if (!pMainFrame)
    {
        return;
    }
    pMainFrame->ResetBackgroundUpdateTimer();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    m_backgroundUpdateTimer = 0;
    ResetBackgroundUpdateTimer();

    // Initialize the docking pane manager and set the
    // initial them for the docking panes.  Do this only after all
    // control bars objects have been created and docked.
    m_viewPaneManager->Install(this, !gSettings.gui.bWindowsVista);

    if (MainWindow::instance()->IsPreview())
    {
        // Hide all menus.
        //ShowControlBar( pMenuBar,FALSE,0 );
        //ShowControlBar( &m_wndReBar,FALSE,0 );
        //ShowControlBar( &m_wndTrackViewBar,FALSE,0 );
        //ShowControlBar( &m_wndDataBaseBar,FALSE,0 );
    }
    else
    {
        // Update tools menu,
        UpdateToolsMenu();
    }

#ifdef KDAB_TEMPORARILY_REMOVED
    // AMZN-401
    // TODO: Toolbox needs porting first
    GetIEditor()->GetToolBoxManager()->Load(pCommandBars);
#endif

    ICVar* pBackgroundUpdatePeriod = gEnv->pConsole->GetCVar("ed_backgroundUpdatePeriod");
    if (pBackgroundUpdatePeriod)
    {
        pBackgroundUpdatePeriod->SetOnChangeCallback(&CMainFrame::OnBackgroundUpdatePeriodChanged);
    }

    return 0;
}

void CMainFrame::ResetBackgroundUpdateTimer()
{
    if (m_backgroundUpdateTimer)
    {
        KillTimer(BACKGROUND_UPDATE_TIMER_EVENT);
        m_backgroundUpdateTimer = 0;
    }

    ICVar* pBackgroundUpdatePeriod = gEnv->pConsole->GetCVar("ed_backgroundUpdatePeriod");
    if (pBackgroundUpdatePeriod && pBackgroundUpdatePeriod->GetIVal() > 0)
    {
        m_backgroundUpdateTimer = SetTimer(BACKGROUND_UPDATE_TIMER_EVENT, pBackgroundUpdatePeriod->GetIVal(), 0);
    }
}

int CMainFrame::OnCreateControl(LPCREATECONTROLSTRUCT lpCreateControl)
{
    if (lpCreateControl->bToolBar)
    {
        if (lpCreateControl->nID == ID_TERRAIN
            || lpCreateControl->nID == ID_GENERATORS_TEXTURE
            || lpCreateControl->nID == ID_TERRAIN_TIMEOFDAY)
        {
            CXTPControlButton* pButton = (CXTPControlButton*)CXTPControlButton::CreateObject();

            lpCreateControl->pControl = pButton;
            pButton->SetID(lpCreateControl->nID);

            if (lpCreateControl->nID == ID_TERRAIN)
            {
                pButton->SetCaption(CString(MAKEINTRESOURCE(IDS_TERRAIN_CAPTION)));
            }

            if (lpCreateControl->nID == ID_GENERATORS_TEXTURE)
            {
                pButton->SetCaption(CString(MAKEINTRESOURCE(IDS_TEXTURE_CAPTION)));
            }

            if (lpCreateControl->nID == ID_TERRAIN_TIMEOFDAY)
            {
                pButton->SetCaption(CString(MAKEINTRESOURCE(IDS_TIMEOFDAY_CAPTION)));
            }

            pButton->SetStyle(xtpButtonIconAndCaption);

            return TRUE;
        }
        else if (lpCreateControl->nID == ID_LAYER_SELECT)
        {
            CXTPControlButton* pButton = (CXTPControlButton*)CXTPControlButton::CreateObject();

            lpCreateControl->pControl = pButton;
            pButton->SetID(lpCreateControl->nID);
            pButton->SetStyle(xtpButtonIconAndCaption);

            return TRUE;
        }
        else if (lpCreateControl->nID == ID_SNAP_TO_GRID)
        {
            CCustomControlSplitButtonPopup* pCmdPopup = new CCustomControlSplitButtonPopup();

            lpCreateControl->pControl = pCmdPopup;
            pCmdPopup->SetID(lpCreateControl->nID);
            pCmdPopup->SetStyle(xtpButtonAutomatic);

            return TRUE;
        }
        else if (lpCreateControl->nID == ID_SNAPANGLE)
        {
            CCustomControlSplitButtonPopup* pCmdPopup = new CCustomControlSplitButtonPopup();
            lpCreateControl->pControl = pCmdPopup;
            pCmdPopup->SetID(lpCreateControl->nID);
            pCmdPopup->SetStyle(xtpButtonAutomatic);
            return TRUE;
        }
        else if (lpCreateControl->nID == ID_UNDO)
        {
            CCustomControlSplitButtonPopup* pCmdPopup = new CCustomControlSplitButtonPopup();

            lpCreateControl->pControl = pCmdPopup;
            pCmdPopup->SetID(lpCreateControl->nID);
            pCmdPopup->SetStyle(xtpButtonAutomatic);

            return TRUE;
        }
        else if (lpCreateControl->nID == ID_REDO)
        {
            CCustomControlSplitButtonPopup* pCmdPopup = new CCustomControlSplitButtonPopup();

            lpCreateControl->pControl = pCmdPopup;
            pCmdPopup->SetID(lpCreateControl->nID);
            pCmdPopup->SetStyle(xtpButtonAutomatic);

            return TRUE;
        }
        else if (lpCreateControl->nID == IDC_SELECTION)
        {
            CXTPControlComboBox* pComboBox = (CXTPControlComboBox*)CXTPControlComboBox::CreateObject();

            pComboBox->SetDropDownListStyle();
            pComboBox->SetWidth(100);
            pComboBox->SetCaption(CString(MAKEINTRESOURCE(IDS_SELECTIONSET_CAPTION)));
            pComboBox->SetStyle(xtpComboNormal);
            pComboBox->SetFlags(xtpFlagManualUpdate);
            lpCreateControl->pControl = pComboBox;

            return TRUE;
        }
    }

    if (lpCreateControl->nID == ID_TOOLS_TOOL1)
    {
        return TRUE;
    }

    if (lpCreateControl->nID == ID_VIEW_LAYOUTS)
    {
        CXTPControlPopup* pPopup = CXTPControlPopup::CreateControlPopup(xtpControlPopup);
        pPopup->SetFlags(xtpFlagManualUpdate);
        lpCreateControl->pControl = pPopup;
#ifdef KDAB_TEMPORARILY_REMOVED
        CXTPPopupBar* pToolsMenuBar = CXTPPopupBar::CreatePopupBar(GetCommandBars());
        pPopup->SetCommandBar(pToolsMenuBar);
#endif
        return TRUE;
    }

    if (ID_VIEW_LAYOUT_FIRST <= lpCreateControl->nID && lpCreateControl->nID <= ID_VIEW_LAYOUT_LAST)
    {
        CXTPControlPopup* pPopup = CXTPControlPopup::CreateControlPopup(xtpControlPopup);
        pPopup->SetFlags(xtpFlagManualUpdate);
        lpCreateControl->pControl = pPopup;
#ifdef KDAB_TEMPORARILY_REMOVED
        CXTPPopupBar* pToolsMenuBar = CXTPPopupBar::CreatePopupBar(GetCommandBars());
        pPopup->SetCommandBar(pToolsMenuBar);
#endif
        return TRUE;
    }

    if (lpCreateControl->nID == ID_FILE_MRU_FILE1)
    {
        CXTPControl* pControl = (CXTPControl*)CControlMRU::CreateObject();
        if (pControl)
        {
            pControl->SetFlags(xtpFlagManualUpdate);
            lpCreateControl->pControl = pControl;
            return TRUE;
        }
    }

    return FALSE;
}


void CMainFrame::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnIdleUpdate:
        IdleUpdate();
        break;
    case eNotify_OnBeginSceneOpen:
    case eNotify_OnBeginNewScene:
    case eNotify_OnCloseScene:
        ResetAutoSaveTimers();
        break;
    case eNotify_OnEndSceneOpen:
    case eNotify_OnEndNewScene:
        ResetAutoSaveTimers(true);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
CViewPaneManager* CMainFrame::GetViewPaneManager()
{
    return m_viewPaneManager;
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    return CFrameWnd::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::DestroyWindow()
{
    if (m_autoSaveTimer)
    {
        KillTimer(m_autoSaveTimer);
    }
    if (m_autoRemindTimer)
    {
        KillTimer(m_autoRemindTimer);
    }

    return CFrameWnd::DestroyWindow();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    // Use our own window class (Needed to detect single application instance).
    cs.lpszClass = _T("LumberyardEditorClass");

    WNDCLASS wndcls;
    BOOL bResult = GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);

    // Init the window with the lowest possible resolution
    RECT rc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    cs.x = rc.left;
    cs.y = rc.top;
    cs.cx = rc.right - rc.left;
    cs.cy = rc.bottom - rc.top;


    BOOL fade = FALSE;
    SystemParametersInfo(SPI_SETMENUFADE, 0, &fade, 0);

    if (!CFrameWnd::PreCreateWindow(cs))
    {
        return FALSE;
    }

    // Create a child window
    cs.style = WS_BORDER | WS_MAXIMIZE;
    cs.dwExStyle = 0;

    //cs.lpszClass = AfxRegisterWndClass( 0, NULL, NULL,AfxGetApp()->LoadIcon(IDR_MAINFRAME));
    //cs.style &= ~FWS_ADDTOTITLE;

    return TRUE;
}

void CMainFrame::IdleUpdate()
{
    if (m_bIdleRedrawWindow)
    {
        m_bIdleRedrawWindow = false;
        RedrawWindow(0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CFrameWnd::OnGetMinMaxInfo(lpMMI);

    if (lpMMI)
    {
        const int kMaxWindowSize = 10000;
        lpMMI->ptMaxTrackSize.x = kMaxWindowSize;
        lpMMI->ptMaxTrackSize.y = kMaxWindowSize;
    }
}

void CMainFrame::OnUpdateControlBar(CCmdUI* pCmdUI)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CXTPCommandBars* pCommandBars = GetCommandBars();
    if (pCommandBars != NULL)
    {
        CXTPToolBar* pBar = pCommandBars->GetToolBar(pCmdUI->m_nID);
        if (pBar)
        {
            pCmdUI->SetCheck((pBar->IsVisible()) ? 1 : 0);
            return;
        }
    }

    pCmdUI->SetCheck(m_viewPaneManager->IsPaneOpen(pCmdUI->m_nID));
#endif
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    // TODO: Add your specialized code here and/or call the base class
    //m_layoutWnd.Create( this,2,2,CSize(10,10),pContext );
    //    return TRUE;
    bool a = CFrameWnd::OnCreateClient(lpcs, pContext);
    return a;
}

//////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    // TODO: Add your message handler code here and/or call default
    if (pCopyDataStruct->dwData == 100 && pCopyDataStruct->lpData != NULL)
    {
        char str[1024];
        memcpy(str, pCopyDataStruct->lpData, pCopyDataStruct->cbData);
        str[pCopyDataStruct->cbData] = 0;

        // Load this file.
        CCryEditApp::instance()->LoadFile(str);
    }

    return CFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnClose()
{
    MainWindow::instance()->close();
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::ActivateFrame(int nCmdShow)
{
    CFrameWnd::ActivateFrame(0);
}

void CMainFrame::ShowWindowEx(int nCmdShow)
{
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
    AfxGetApp()->m_pMainWnd = this;
    if (!CFrameWnd::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
    {
        return FALSE;
    }

#ifdef KDAB_TEMPORARILY_REMOVED
    MainWindow::instance()->SetOldMainFrame(this);
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::EditAccelerator()
{
    // Open accelerator key manager dialog.
    //accelManager.EditKeyboardShortcuts(this);
    OnCustomize();
}

void CMainFrame::UpdateToolsMenu()
{
}

////////////////////////////////////////////////////////////////////////////
////implementation of the callback for removing recently used files
//void CMainFrame::OnFileMRURemove( UINT nID )
//{
//  int nIndex = nID - ID_FILE_MRU_REMOVE_FIRST;
//
//  bool bIndexIsValid = ( 0 <= nIndex && nIndex < 16 );
//  if (!bIndexIsValid)
//      return;
//
//  CRecentFileList *pRecentFileList = ((CCryEditApp *) (AfxGetApp()))->GetRecentFileList();
//  if (!pRecentFileList)
//  {
//      pRecentFileList->Remove(nIndex);
//      pRecentFileList->WriteList();
//  }
//}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
    case AUTOSAVE_TIMER_EVENT:
    {
        if (gSettings.autoBackupEnabled)
        {
            // Call autosave function of CryEditApp.
            GetIEditor()->GetDocument()->SaveAutoBackup();
        }
        break;
    }
    case AUTOREMIND_TIMER_EVENT:
    {
        if (gSettings.autoRemindTime > 0)
        {
            // Remind to save.
            CCryEditApp::instance()->SaveAutoRemind();
        }
        break;
    }
    case NETWORK_AUDITION_UPDATE_TIMER_EVENT:
    {
        if (GetISystem())
        {
            // Audio: external update necessary?
        }
        break;
    }
    case BACKGROUND_UPDATE_TIMER_EVENT:
    {
        // Make sure that visible editor window get low-fps updates while in the background

        CCryEditApp* pApp = CCryEditApp::instance();
        if (!IsIconic() && !pApp->IsWindowInForeground())
        {
            pApp->IdleProcessing(true);
        }
        break;
    }
    default:
    {
    }
    }

    CFrameWnd::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnActivate(UINT state, CWnd* owner, BOOL minimized)
{
    if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
    {
        if (GetIEditor()->GetViewManager()->GetSelectedViewport() == 0)
        {
            GetIEditor()->GetViewManager()->SelectViewport(GetIEditor()->GetViewManager()->GetGameViewport());
        }
    }
}

void CMainFrame::OnCustomize()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Get a pointer to the command bars object.
    CXTPCommandBars* pCommandBars = GetCommandBars();
    if (pCommandBars != NULL)
    {
        // Instantiate the customize dialog object.
        CXTPCustomizeSheet dlg(pCommandBars);

        // Add the options page to the customize dialog.
        CXTPCustomizeOptionsPage pageOptions(&dlg);
        dlg.AddPage(&pageOptions);

        // Add the commands page to the customize dialog.
        CXTPCustomizeCommandsPage* pCommands = dlg.GetCommandsPage();
        pCommands->AddCategories(IDR_MAINFRAME);

        // Use the command bar manager to initialize the
        // customize dialog.
        pCommands->InsertAllCommandsCategory();
        pCommands->InsertBuiltInMenus(IDR_MAINFRAME);
        pCommands->InsertNewMenuCategory();

        // Add the options page to the customize dialog.
        CustomizeKeyboardPage pageKeyboard(&dlg);
        CCustomControlSplitButtonPopup
        dlg.AddPage(&pageKeyboard);
        pageKeyboard.AddCategories(IDR_MAINFRAME, TRUE);
        //pageKeyboard.InsertCategory(()


        // Display the dialog.
        dlg.DoModal();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMXTPCONTROL* tagNMCONTROL = (NMXTPCONTROL*)pNMHDR;
    CXTPControlComboBox* pControl = (CXTPControlComboBox*)tagNMCONTROL->pControl;
    if (pControl->GetType() == xtpControlComboBox)
    {
        CString selection = pControl->GetEditText();

        if (!selection.IsEmpty())
        {
            if (GetIEditor()->GetObjectManager()->GetSelection(selection))
            {
                GetIEditor()->GetObjectManager()->SetSelection(selection);
            }
            else
            {
                GetIEditor()->GetObjectManager()->NameSelection(selection);
                if (pControl->FindStringExact(0, selection.GetBuffer()) == CB_ERR)
                {
                    pControl->AddString(selection.GetBuffer());
                }
            }
        }
    }
    *pResult = 1;
}

void CMainFrame::ResetAutoSaveTimers(bool bForceInit)
{
    if (m_autoSaveTimer)
    {
        KillTimer(m_autoSaveTimer);
    }
    if (m_autoRemindTimer)
    {
        KillTimer(m_autoRemindTimer);
    }
    m_autoSaveTimer = 0;
    m_autoRemindTimer = 0;

    if (bForceInit)
    {
        if (gSettings.autoBackupTime > 0 && gSettings.autoBackupEnabled)
        {
            m_autoSaveTimer = SetTimer(AUTOSAVE_TIMER_EVENT, gSettings.autoBackupTime * 1000 * 60, 0);
        }
        if (gSettings.autoRemindTime > 0)
        {
            m_autoRemindTimer = SetTimer(AUTOREMIND_TIMER_EVENT, gSettings.autoRemindTime * 1000 * 60, 0);
        }
    }
}
