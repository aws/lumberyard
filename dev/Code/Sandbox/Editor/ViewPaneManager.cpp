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
#include "ViewPaneManager.h"

#include "RenderViewport.h" // HACK
#include "UserMessageDefines.h"
#include <QtViewPaneManager.h>
#include <QWidget>

struct SViewPaneDesc
{
    int m_paneId;
    CString m_category;
    CString m_menuSuggestion;
    CWnd* m_pViewWnd;
    CRuntimeClass* m_pRuntimeClass;
    IViewPaneClass* m_pViewClass;
    bool m_bWantsIdleUpdate;
    bool m_bShowInMenu;
};

static const UINT kPaneHistoryRegistryVersion = 1;
static LPCTSTR kDockingPanesHistorySection = _T("DockingPanesHistory");
static LPCTSTR kRegistryVersionEntry = _T("Version");

// ---------------------------------------------------------------------------

CViewPaneManager::CViewPaneManager()
    : m_pDefaultLayout(0)
    , m_isLoadingLayout(false)
{
}


static bool CanClose(CWnd* pWnd)
{
    BOOL bCanClose = TRUE;
    ::SendMessage(pWnd->GetSafeHwnd(), WM_FRAME_CAN_CLOSE, (WPARAM)&bCanClose, 0);
    return bCanClose;
}

void CViewPaneManager::Install(CWnd* window, bool themedFloatingFrames)
{
    m_paneManager.InstallDockingPanes(window);
    m_paneManager.SetThemedFloatingFrames(themedFloatingFrames ? TRUE : FALSE);
}

void CViewPaneManager::SetDockingHelpers(bool dockingHelpers)
{
    m_paneManager.SetAlphaDockingContext(dockingHelpers);
    m_paneManager.SetShowDockingContextStickers(dockingHelpers);
}

void CViewPaneManager::SetTheme(int xtpTheme)
{
    m_paneManager.SetTheme((XTPDockingPanePaintTheme)xtpTheme);
}

LRESULT CViewPaneManager::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
    if (wParam == XTP_DPN_ACTION)
    {
        XTP_DOCKINGPANE_ACTION* pAction = (XTP_DOCKINGPANE_ACTION*)lParam;

        if (pAction->action == xtpPaneActionActivated || pAction->action == xtpPaneActionDeactivated)
        {
            CWnd* pWnd = pAction->pPane->GetChild();
            if (pWnd && pWnd->GetSafeHwnd())
            {
                pWnd->SendMessage(WM_ONWINDOWFOCUSCHANGES, 0, (LPARAM)pAction->action == xtpPaneActionActivated);
            }
        }
        //CryLogAlways("DockPaneAction: %i, %s", pAction->action,(const char*)pAction->pPane->GetTitle() );
        return TRUE;
    }

    if (wParam == XTP_DPN_SHOWWINDOW)
    {
        // get a pointer to the docking pane being shown.
        CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
        if (!pwndDockWindow->IsValid())
        {
            DefaultPaneWindows::iterator it = m_defaultPaneWindows.find(pwndDockWindow->GetID());
            if (it != m_defaultPaneWindows.end())
            {
                pwndDockWindow->Attach(it->second);
            }
            else
            {
                // [2007/05/21 MichaelS] It's possible that we can get recursive calls here, which can result in
                // trying to construct multiple instances of panes (such as the Facial Editor). This may happen
                // as a result of an assert dialog being created as a child of the main frame - for some reason
                // XT sends us a docking pane notify message that looks like a request to create the view a second
                // time. Therefore we add this guard code to detect and reject that request.
                // Note that this check as it stands could allow a situation where pane A opens pane B in its
                // constructor and then XT tries to create pane A again, but this behaviour doesn't seem to occur.
                bool inserted = m_enteredPanes.insert(pwndDockWindow).second;
                if (inserted)
                {
                    // Make a dynamic pane.
                    PanesMap::iterator it = m_panesMap.find(pwndDockWindow->GetID());
                    if (it != m_panesMap.end())
                    {
                        SViewPaneDesc& pn = it->second;
                        if (!pn.m_pViewWnd)
                        {
                            CWnd* pWnd = (CWnd*)pn.m_pRuntimeClass->CreateObject();
                            assert(pWnd);
                            pn.m_pViewWnd = pWnd;
                        }
                        pn.m_pViewWnd->ShowWindow(SW_SHOW);
                        pwndDockWindow->Attach(pn.m_pViewWnd);
                        // FIXME: HACK!
                        /* KDAB_TODO: This whole method is dead code and can be removed
                        if (CRenderViewport* pView = viewport_cast<CRenderViewport*>(pn.m_pViewWnd))
                        {
                            pView->SetDockingPaneWndID(pwndDockWindow->GetID());
                        } */
                    }
                    else
                    {
                        // Try to attach to pane.
                        if (m_isLoadingLayout)
                        {
                            // If currently loading the layout, defer the pane attachment / runtime class creation
                            // This way, we can nicely deal with panes created during the constructor of a runtime
                            // class (e.g. Error Report created by the Mannequin and Character editors)
                            m_pendingPanes.insert(pwndDockWindow);
                        }
                        else
                        {
                            AttachToPane(pwndDockWindow);
                        }
                    }

                    m_enteredPanes.erase(m_enteredPanes.find(pwndDockWindow));
                }
            }
        }
        return TRUE;
    }
    else if (wParam == XTP_DPN_CLOSEPANE)
    {
        // get a pointer to the docking pane being shown.
        CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
        if (pwndDockWindow->IsValid())
        {
            DefaultPaneWindows::iterator it = m_defaultPaneWindows.find(pwndDockWindow->GetID());
            if (it != m_defaultPaneWindows.end())
            {
                pwndDockWindow->Attach(it->second);
            }
            else
            {
                // Delete a dynamic pane.
                PanesMap::iterator it = m_panesMap.find(pwndDockWindow->GetID());
                if (it != m_panesMap.end())
                {
                    SViewPaneDesc& pn = it->second;

                    if (CanClose(pn.m_pViewWnd))
                    {
                        SPaneHistory paneHistory;
                        paneHistory.rect = pwndDockWindow->GetPaneWindowRect();
                        paneHistory.dockDir = m_paneManager.GetPaneDirection(pwndDockWindow);
                        m_panesHistoryMap[pwndDockWindow->GetTitle()] = paneHistory;

                        try
                        {
                            pn.m_pViewWnd->DestroyWindow();
                        } catch (...)
                        {               /* DestroyWindow won't work with QWidget based windows. Can be silently ignored.*/
                        }

                        pn.m_pViewWnd = 0;
                        pwndDockWindow->Detach();
                        m_panesMap.erase(it);
                    }
                    else
                    {
                        return XTP_ACTION_NOCLOSE;
                    }
                }
            }
        }

        return TRUE; // handled
    }

    return FALSE;
}

void CViewPaneManager::CreatePane(CWnd* paneWindow, int id, const char* title, const CRect& rect, int dockTo)
{
    MAKE_SURE(m_defaultPaneWindows.find(id) == m_defaultPaneWindows.end(), return );

    CXTPDockingPane* pDockConsole = m_paneManager.CreatePane(id, rect, (XTPDockingPaneDirection)dockTo);
    pDockConsole->SetTitle(title);

    m_defaultPaneWindows[id] = paneWindow;
    if (!stl::find(m_defaultPaneNames, title))
    {
        m_defaultPaneNames.push_back(title);
    }
}

CWnd* CViewPaneManager::OpenPane(const char* sPaneClassName, bool bReuseOpened)
{
    if (QtViewPaneManager::instance()->OpenPane(sPaneClassName))
    {
        return nullptr;
    }

    IClassDesc* pClassDesc = GetIEditor()->GetClassFactory()->FindClass(sPaneClassName);
    if (!pClassDesc)
    {
        return 0;
    }

    IViewPaneClass* pViewPaneClass = NULL;
    if (FAILED(pClassDesc->QueryInterface(__uuidof(IViewPaneClass), (void**)&pViewPaneClass)))
    {
        return 0;
    }

    // Check if view view pane class support only 1 pane at a time.
    if (pViewPaneClass->SinglePane() || bReuseOpened)
    {
        SViewPaneDesc* pPaneDesc = FindPaneByClass(pViewPaneClass);
        if (pPaneDesc)
        {
            // Pane with this class already created.

            // Only set focus to it.
            CXTPDockingPane* pDockPane = m_paneManager.FindPane(pPaneDesc->m_paneId);
            if (pDockPane)
            {
                /*
                CRect rc = pDockPane->GetWindowRect();
                if (rc.IsRectEmpty())
                {
                    CRect paneRect = pViewPaneClass->GetPaneRect();
                    //m_paneManager.FloatPane( pDockPane,paneRect );
                    m_paneManager.ShowPane( pDockPane );
                }
                */
                m_paneManager.ShowPane(pDockPane);
                pDockPane->SetFocus();
                return pPaneDesc->m_pViewWnd;
            }
            else
            {
                for (PanesMap::iterator it = m_panesMap.begin(); it != m_panesMap.end(); ++it)
                {
                    if (pViewPaneClass == it->second.m_pViewClass)
                    {
                        m_panesMap.erase(it);
                        break;
                    }
                }
            }
        }
    }

    CXTPDockingPane* pDockPane = 0;

    if (pViewPaneClass->SinglePane() || bReuseOpened)
    {
        CXTPDockingPaneInfoList& panes = m_paneManager.GetPaneList();
        POSITION pos = panes.GetHeadPosition();
        while (pos)
        {
            XTP_DOCKINGPANE_INFO& paneInfo = panes.GetNext(pos);
            if (paneInfo.pPane)
            {
                if (strcmp(paneInfo.pPane->GetTitle(), sPaneClassName) == 0)
                {
                    m_paneManager.ShowPane(paneInfo.pPane);   // This will show window and attach pane if needed.
                    paneInfo.pPane->SetFocus();
                    SViewPaneDesc* pPaneDesc = FindPaneByClass(pViewPaneClass);
                    if (pPaneDesc)
                    {
                        return pPaneDesc->m_pViewWnd;
                    }
                    return 0;
                }
            }
        }
    }


    SViewPaneDesc pd;
    pd.m_pViewWnd = NULL;
    pd.m_pViewClass = pViewPaneClass;
    pd.m_pRuntimeClass = pViewPaneClass->GetRuntimeClass();
    pd.m_category = pViewPaneClass->Category();
    pd.m_menuSuggestion = pViewPaneClass->MenuSuggestion();
    pd.m_bWantsIdleUpdate = pViewPaneClass->WantIdleUpdate();
    pd.m_bShowInMenu = pViewPaneClass->ShowInMenu();

    // Find free pane id.
    pd.m_paneId = 1;
    while (m_paneManager.FindPane(pd.m_paneId) != NULL)
    {
        pd.m_paneId++;
    }

    assert(pd.m_pRuntimeClass);
    assert(pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd));
    if (!pd.m_pRuntimeClass)
    {
        return 0;
    }
    if (!(pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd)))
    {
        return 0;
    }

    bool bFloat = false;

    XTPDockingPaneDirection dockDir = xtpPaneDockTop;
    switch (pViewPaneClass->GetDockingDirection())
    {
    case IViewPaneClass::DOCK_TOP:
        dockDir = xtpPaneDockTop;
        break;
    case IViewPaneClass::DOCK_BOTTOM:
        dockDir = xtpPaneDockBottom;
        break;
    case IViewPaneClass::DOCK_LEFT:
        dockDir = xtpPaneDockLeft;
        break;
    case IViewPaneClass::DOCK_RIGHT:
        dockDir = xtpPaneDockRight;
        break;
    case IViewPaneClass::DOCK_FLOAT:
        bFloat = true;
        break;
    }

    CXTPDockingPane* pNextToPane = 0;
    SViewPaneDesc* pSimilarPaneDesc = FindPaneByCategory(pd.m_category);
    if (pSimilarPaneDesc)
    {
        pNextToPane = m_paneManager.FindPane(pSimilarPaneDesc->m_paneId);
    }

    CString paneTitle = pViewPaneClass->GetPaneTitle();
    CRect paneRect = pViewPaneClass->GetPaneRect();
    if (m_panesHistoryMap.find(paneTitle) != m_panesHistoryMap.end())
    {
        const SPaneHistory& paneHistory = m_panesHistoryMap.find(paneTitle)->second;
        paneRect = paneHistory.rect;
        dockDir = paneHistory.dockDir;
    }

    CRect maxRc;
    maxRc.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    maxRc.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    maxRc.right = maxRc.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    maxRc.bottom = maxRc.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Clip to virtual desktop.
    paneRect.IntersectRect(paneRect, maxRc);

    // Ensure it is at least 10x10
    if (paneRect.Width() < 10)
    {
        paneRect.right = paneRect.left + 10;
    }
    if (paneRect.Height() < 10)
    {
        paneRect.bottom = paneRect.top + 10;
    }

    // Add pane description to map (we do it before creating the window object to avoid recursive window creation error)
    SetPane(pd.m_paneId, pd);

    // We cannot create a pane if the pane manager does not have a top pane, this may happen when creating the
    // error report dialog during the initialization of other dialogs.
    if (!m_paneManager.GetTopPane())
    {
        return 0;
    }

    pDockPane = m_paneManager.CreatePane(pd.m_paneId, paneRect, dockDir, pNextToPane);

    // [2012/06/27 PauloZ] It's possible that we can get double calls here (from OpenPane and then OnDockingPaneNotify),
    // which can result in trying to construct multiple instances of panes (such as the Mannequin). This may happen
    // as a result of an assert dialog or message box being created as a child of the main frame - for some reason
    // XT sends us a docking pane notify message that looks like a request to create the view a second
    // time. Therefore we register the variable here, so the guard code in OnDockingPaneNotify can detect and reject that request.
    // Note that this check as it stands could allow a situation where pane A opens pane B in its
    // constructor and then XT tries to create pane A again, but this behavior doesn't seem to occur, yet.

    m_enteredPanes.insert(pDockPane);

    pd.m_pViewWnd = (CWnd*)pd.m_pRuntimeClass->CreateObject();
    assert(pd.m_pViewWnd);

    m_enteredPanes.erase(pDockPane);

    // If the window is not created successfully, should delete the panel as well
    if (!pd.m_pViewWnd)
    {
        m_paneManager.DestroyPane(pd.m_paneId);
        return NULL;
    }

    // Now update pane description with correct view window pointer
    SetPane(pd.m_paneId, pd);

    if (!pDockPane)
    {
        return 0;
    }
    XTPDockingPaneType type = pDockPane->GetType();
    if (bFloat)
    {
        m_paneManager.FloatPane(pDockPane, paneRect);
    }

    pDockPane->SetTitle(pViewPaneClass->GetPaneTitle());
    pDockPane->SetMinTrackSize(pViewPaneClass->GetMinSize());
    //pDockPane->SetPaneData( 2 );

    return pd.m_pViewWnd;
}

CWnd* CViewPaneManager::OpenPaneByTitle(const char* title)
{
    CXTPDockingPane* pBar = FindDockedPaneByTitle(title);
    if (pBar)
    {
        if (pBar->IsClosed())
        {
            m_paneManager.ShowPane(pBar);
        }
        return pBar->GetChild();
    }
    return 0;
}

CWnd* CViewPaneManager::FindPane(const char* sPaneClassName)
{
    IClassDesc* pClassDesc = GetIEditor()->GetClassFactory()->FindClass(sPaneClassName);
    if (!pClassDesc)
    {
        return 0;
    }

    IViewPaneClass* pViewPaneClass = NULL;
    if (FAILED(pClassDesc->QueryInterface(__uuidof(IViewPaneClass), (void**)&pViewPaneClass)))
    {
        return 0;
    }

    SViewPaneDesc* pPaneDesc = FindPaneByClass(pViewPaneClass);
    if (!pPaneDesc)
    {
        return 0;
    }

    // Pane with this class is created.

    CXTPDockingPane* pDockPane = m_paneManager.FindPane(pPaneDesc->m_paneId);
    if (!pDockPane)
    {
        return 0;
    }

    return pPaneDesc->m_pViewWnd;
}

CWnd* CViewPaneManager::FindPaneByTitle(const char* title)
{
    CXTPDockingPane* dockingPane = FindDockedPaneByTitle(title);
    if (!dockingPane)
    {
        return 0;
    }

    return dockingPane->GetChild();
}

QWidget* CViewPaneManager::FindWidgetPaneByTitle(const char* title)
{
    auto pane = FindPaneByTitle(title);
    if (!pane)
    {
        return nullptr;
    }

    return QWidget::find((WId)pane->GetSafeHwnd());
}

CXTPDockingPane* CViewPaneManager::FindDockedPaneByTitle(const char* paneName)
{
    bool boResult(false);

    CXTPDockingPaneInfoList& panes = m_paneManager.GetPaneList();
    POSITION pos = panes.GetHeadPosition();

    while (pos)
    {
        XTP_DOCKINGPANE_INFO& paneInfo = panes.GetNext(pos);
        if (paneInfo.pPane)
        {
            if (strcmp(paneInfo.pPane->GetTitle(), paneName) == 0)
            {
                return paneInfo.pPane;
            }
        }
    }

    return NULL;
}
CFrameWnd* CViewPaneManager::FindFrameWndByPaneID(int id)
{
    CXTPDockingPane* dockingPane = m_paneManager.FindPane(id);
    return dockingPane->GetParentFrame();
}

bool CViewPaneManager::TogglePane(int nID)
{
    CXTPDockingPane* pBar = m_paneManager.FindPane(nID);
    if (pBar)
    {
        if (pBar->IsClosed())
        {
            m_paneManager.ShowPane(pBar);
        }
        else
        {
            m_paneManager.ClosePane(pBar);
        }
        return true;
    }
    return false;
}

bool CViewPaneManager::IsPaneOpen(int id)
{
    CXTPDockingPane* pane = m_paneManager.FindPane(id);
    if (pane)
    {
        return !pane->IsClosed();
    }
    return false;
}

void CViewPaneManager::CloseAllOpenedTools()
{
    bool boResult(false);

    CXTPDockingPaneInfoList& panes = m_paneManager.GetPaneList();
    POSITION pos = panes.GetHeadPosition();

    while (pos)
    {
        XTP_DOCKINGPANE_INFO& paneInfo = panes.GetNext(pos);
        if (paneInfo.pPane)
        {
            bool isStandardTool = false;

            for (int i = 0; i < m_defaultPaneNames.size(); ++i)
            {
                if (strstr(paneInfo.pPane->GetTitle(), m_defaultPaneNames[i]))
                {
                    isStandardTool = true;
                    break;
                }
            }

            if (!isStandardTool)
            {
                if (QtViewPaneManager::instance()->IsVisible(QtUtil::ToQString(paneInfo.pPane->GetTitle())))
                {
                    GetIEditor()->ExecuteCommand("general.close_pane '%s'", paneInfo.pPane->GetTitle());
                }
            }
        }
    }
}

bool CViewPaneManager::IsPaneDocked(const char* sPaneClassName)
{
    PanesMap::iterator      itCurrentPane;

    for (itCurrentPane = m_panesMap.begin(); itCurrentPane != m_panesMap.end(); itCurrentPane++)
    {
        SViewPaneDesc& pn = itCurrentPane->second;
        if (pn.m_category.Compare(sPaneClassName) == 0)
        {
            CXTPDockingPane* pDockPane = m_paneManager.FindPane(pn.m_paneId);

            return (!pDockPane->IsFloating());
        }
    }

    return false;
}

bool CViewPaneManager::AttachToPane(CXTPDockingPane* pDockPane)
{
    CRY_ASSERT_MESSAGE(!m_isLoadingLayout, "AttachToPane called while loading the layout: this can result in pane ID collisions.");
    CString title = pDockPane->GetTitle();
    if (title.IsEmpty())
    {
        return false;
    }

    IViewPaneClass* pViewPaneClass = GetIEditor()->GetClassFactory()->FindViewPaneClassByTitle(title);
    if (!pViewPaneClass)
    {
        return false;
    }

    // Check if view view pane class support only 1 pane at a time.
    if (pViewPaneClass->SinglePane())
    {
        SViewPaneDesc* pPaneDesc = FindPaneByClass(pViewPaneClass);
        if (pPaneDesc)
        {
            // Pane with this class already created.
            // Only set focus to it.
            CXTPDockingPane* pDockPane = m_paneManager.FindPane(pPaneDesc->m_paneId);
            if (pDockPane)
            {
                pDockPane->SetFocus();
            }
            return true;
        }
    }

    SViewPaneDesc pd;
    pd.m_pViewWnd = NULL;
    pd.m_pViewClass = pViewPaneClass;
    pd.m_pRuntimeClass = pViewPaneClass->GetRuntimeClass();
    pd.m_category = pViewPaneClass->Category();
    pd.m_menuSuggestion = pViewPaneClass->MenuSuggestion();
    pd.m_bWantsIdleUpdate = pViewPaneClass->WantIdleUpdate();
    pd.m_bShowInMenu = pViewPaneClass->ShowInMenu();
    pd.m_paneId = pDockPane->GetID();

    assert(pd.m_pRuntimeClass);
    if (!pd.m_pRuntimeClass)
    {
        return false;
    }
    assert(pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd));
    if (!(pd.m_pRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)) || pd.m_pRuntimeClass == RUNTIME_CLASS(CWnd)))
    {
        return false;
    }

    pDockPane->SetTitle(pViewPaneClass->GetPaneTitle());
    pDockPane->SetMinTrackSize(pViewPaneClass->GetMinSize());

    // Add pane description to map (we do it before creating the window object to avoid recursive window creation error)
    SetPane(pd.m_paneId, pd);

    CWnd* pWnd = (CWnd*)pd.m_pRuntimeClass->CreateObject();
    assert(pWnd);
    //pWnd->ShowWindow(SW_SHOW);
    pd.m_pViewWnd = pWnd;
    pDockPane->Attach(pd.m_pViewWnd);

    // FIXME: HACK!
    /*
    if (CRenderViewport* vp = viewport_cast<CRenderViewport*>(pd.m_pViewWnd))
    {
        vp->SetDockingPaneWndID(pDockPane->GetID());
    }*/

    // Add pane description to map.
    SetPane(pd.m_paneId, pd);

    return true;
}

void CViewPaneManager::SetPane(int id, const SViewPaneDesc& desc)
{
    PanesMap::iterator it = m_panesMap.find(id);
    if (it != m_panesMap.end())
    {
        const char* className1 = it->second.m_pRuntimeClass->m_lpszClassName;
        const char* className2 = desc.m_pRuntimeClass->m_lpszClassName;
        if (strcmp(className1, className2) != 0)
        {
            char buf[128];
            _snprintf_s(buf, sizeof(buf), _TRUNCATE, "Overwriting pane %s with %s\n", className1, className2);
            OutputDebugString(buf);
        }
    }

    m_panesMap[id] = desc;
}

SViewPaneDesc* CViewPaneManager::FindPaneByCategory(const char* sPaneCategory)
{
    for (PanesMap::iterator it = m_panesMap.begin(); it != m_panesMap.end(); ++it)
    {
        if (_stricmp(sPaneCategory, it->second.m_category) == 0)
        {
            return &(it->second);
        }
    }
    return 0;
}

SViewPaneDesc* CViewPaneManager::FindPaneByClass(IViewPaneClass* pClass)
{
    for (PanesMap::iterator it = m_panesMap.begin(); it != m_panesMap.end(); ++it)
    {
        if (pClass == it->second.m_pViewClass)
        {
            return &(it->second);
        }
    }
    return 0;
}

void CViewPaneManager::RedrawPanes()
{
    m_paneManager.RedrawPanes();
}

void CViewPaneManager::SetLayout(const CXTPDockingPaneLayout& layout)
{
    CRY_ASSERT(m_pendingPanes.empty());
    m_pendingPanes.clear();

    // Clear cached dynamic panes
    m_panesMap.clear();

    // Load new layout
    m_isLoadingLayout = true;
    m_paneManager.SetLayout(&layout);
    m_isLoadingLayout = false;

    // Attach newly created panes
    std::set<int> paneIDs;
    for (auto itDockingPanes = m_pendingPanes.begin(); itDockingPanes != m_pendingPanes.end(); ++itDockingPanes)
    {
        CXTPDockingPane* pDockingPane = *itDockingPanes;
        const bool hasUniqueId = paneIDs.insert(pDockingPane->GetID()).second;
        CRY_ASSERT_TRACE(hasUniqueId, ("CViewPaneManager: Multiple docking panes sharing same ID %d", pDockingPane->GetID()));
        if (!hasUniqueId)
        {
            // This should hopefully never happen: it would mean that we have different panes sharing the same ID
            m_paneManager.DestroyPane(pDockingPane);
        }
        else
        {
            AttachToPane(pDockingPane);
        }
    }
    m_pendingPanes.clear();
}

void CViewPaneManager::StoreDefaultLayout()
{
    static CXTPDockingPaneLayout s_defaultLayout(&m_paneManager);
    if (m_pDefaultLayout == NULL)
    {
        m_pDefaultLayout = &s_defaultLayout;
        m_paneManager.GetLayout(m_pDefaultLayout);
    }
}

void CViewPaneManager::RestoreDefaultLayout()
{
    MAKE_SURE(m_pDefaultLayout != 0, return );
    SetLayout(*m_pDefaultLayout);
}

void CViewPaneManager::SaveLayoutToRegistry(const char* sectionName)
{
    CXTRegistryManager regMgr;
    regMgr.WriteProfileInt(sectionName, kRegistryVersionEntry, kPaneHistoryRegistryVersion);

    CXTPDockingPaneLayout layoutNormal(&m_paneManager);
    m_paneManager.GetLayout(&layoutNormal);
    layoutNormal.Save(sectionName);
}

bool CViewPaneManager::LoadLayoutFromRegistry(const char* sectionName)
{
    CXTRegistryManager regMgr;
    UINT version = regMgr.GetProfileInt(sectionName, kRegistryVersionEntry, 0);

    if (version != kPaneHistoryRegistryVersion)
    {
        return false;
    }

    CXTPDockingPaneLayout layoutNormal(&m_paneManager);
    if (layoutNormal.Load(_T(MAINFRM_LAYOUT_NORMAL)))
    {
        if (layoutNormal.GetPaneList().GetCount() > 0)
        {
            SetLayout(layoutNormal);
            return true;
        }
    }
    return false;
}

void CViewPaneManager::SaveLayoutToFile(const char* fullFilename, const char* layoutName)
{
    CXTPDockingPaneLayout currentLayout(&m_paneManager);
    m_paneManager.GetLayout(&currentLayout);

    currentLayout.SaveToFile(fullFilename, layoutName);
}

void CViewPaneManager::LoadLayoutFromFile(const char* fullFilename, const char* layoutName)
{
    CXTPDockingPaneLayout layout(&m_paneManager);
    layout.LoadFromFile(fullFilename, layoutName);

    bool loadedLayoutIsValid = (0 < layout.GetPaneList().GetCount());
    if (loadedLayoutIsValid)
    {
        SetLayout(layout);
    }
}

void CViewPaneManager::SavePaneHistoryToRegistry()
{
    CXTRegistryManager regMgr;

    regMgr.WriteProfileInt(kDockingPanesHistorySection, kRegistryVersionEntry, kPaneHistoryRegistryVersion);

    int nIndex = 0;
    for (PanesHistoryMap::iterator it = m_panesHistoryMap.begin(); it != m_panesHistoryMap.end(); ++it)
    {
        const SPaneHistory& paneHistory = it->second;
        CString datastr;
        datastr.Format("%d,%d,%d,%d,%d", paneHistory.rect.left, paneHistory.rect.top, paneHistory.rect.right, paneHistory.rect.bottom, paneHistory.dockDir);
        CString idstr;
        idstr.Format("%d", nIndex++);
        regMgr.WriteProfileString(kDockingPanesHistorySection, idstr + "_Title", (const char*)it->first);
        regMgr.WriteProfileString(kDockingPanesHistorySection, idstr + "_Position", datastr);
    }
}

void CViewPaneManager::LoadPaneHistoryFromRegistry()
{
    CXTRegistryManager regMgr;
    m_panesHistoryMap.clear();
    int nIndex = 0;

    UINT version = regMgr.GetProfileInt(kDockingPanesHistorySection, kRegistryVersionEntry, 0);

    if (version != kPaneHistoryRegistryVersion)
    {
        return;
    }

    while (true)
    {
        CString idstr;
        idstr.Format("%d", nIndex++);
        CString title = regMgr.GetProfileString(kDockingPanesHistorySection, idstr + "_Title", "");
        if (title.IsEmpty())
        {
            break;
        }
        CString datastr = regMgr.GetProfileString(kDockingPanesHistorySection, idstr + "_Position", "");

        int x1, y1, x2, y2, dockDir;
        sscanf(datastr, "%d,%d,%d,%d,%d", &x1, &y1, &x2, &y2, &dockDir);
        if (x1 < -1000 || x1 > 10000)
        {
            continue;
        }
        SPaneHistory paneHistory;
        paneHistory.rect = CRect(x1, y1, x2, y2);
        paneHistory.dockDir = (XTPDockingPaneDirection)dockDir;
        m_panesHistoryMap[title] = paneHistory;
    }
}
void CViewPaneManager::ReattachPane(int id)
{
    CXTPDockingPane* dockingPane = m_paneManager.FindPane(id);
    if (!dockingPane)
    {
        return;
    }

    CWnd* child = dockingPane->GetChild();
    dockingPane->Detach();
    dockingPane->Attach(child);
}
