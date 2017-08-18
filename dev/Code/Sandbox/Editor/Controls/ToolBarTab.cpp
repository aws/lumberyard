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

// Description : implementation file


#include "stdafx.h"
#include "ToolBarTab.h"

/////////////////////////////////////////////////////////////////////////////
// CToolBarTab

CToolBarTab::CToolBarTab()
{
}

CToolBarTab::~CToolBarTab()
{
}

BEGIN_MESSAGE_MAP(CToolBarTab, CTabCtrl)
//{{AFX_MSG_MAP(CToolBarTab)
ON_WM_CREATE()
ON_WM_SIZE()
ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
ON_NOTIFY_REFLECT(TCN_SELCHANGING, OnSelchanging)
ON_WM_VSCROLL()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarTab message handlers

int CToolBarTab::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    // TODO: Add your specialized creation code here

    // Set another font for the tab control
    SendMessage(WM_SETFONT, (WPARAM)
        gSettings.gui.hSystemFont, MAKELPARAM(FALSE, 0));

    // Create the scrolling controls
    m_cScrollBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT,
        CRect(90, 20, 110, 400), this, NULL);

    m_cScrollBar.EnableScrollBar();
    m_cScrollBar.ShowScrollBar();

    return 0;
}

void CToolBarTab::OnSize(UINT nType, int cx, int cy)
{
    ////////////////////////////////////////////////////////////////////////
    // Resize all toolbars that are linked with tabs
    ////////////////////////////////////////////////////////////////////////

    CTabCtrl::OnSize(nType, cx, cy);

    // Resize the plugin windows in the tab control
    ResizeAllContainers();

    // Resize the scrollbar
    m_cScrollBar.SetWindowPos(NULL, 159, 28, 16, cy - 30, SWP_NOZORDER);
}

void CToolBarTab::ResizeAllContainers()
{
    ////////////////////////////////////////////////////////////////////////
    // Center all containers in the tab control
    ////////////////////////////////////////////////////////////////////////

    unsigned int i;
    TCITEM tci;
    RECT rcClient;
    RECT rcWndOldClient;

    GetClientRect(&rcClient);

    for (i = 0; i < GetItemCount(); i++)
    {
        // Get the item, retrieve the application specific parameter
        tci.mask = TCIF_PARAM;
        GetItem(i, &tci);

        ASSERT(::IsWindow((HWND) tci.lParam));

        // Save the window dimensions so we can restore the height later
        ::GetClientRect((HWND) tci.lParam, &rcWndOldClient);

        // Resize it
        ::SetWindowPos((HWND) tci.lParam, NULL, 0, 25, rcClient.right - 2,
            rcWndOldClient.bottom, SWP_NOZORDER);
    }



    RECT rcClientContainer, rcClientUIElements;

    GetClientRect(&rcClientContainer);
    ::GetClientRect((HWND) tci.lParam, &rcClientUIElements);

    if ((rcClientUIElements.bottom - rcClientUIElements.top) + 175 > rcClientContainer.bottom - rcClientContainer.top)
    {
        m_cScrollBar.ShowWindow(SW_SHOW);
    }
    else
    {
        m_cScrollBar.ShowWindow(SW_HIDE);
    }

    /*
        SCROLLINFO info;

      info.cbSize = sizeof(SCROLLINFO);
      info.fMask = SIF_ALL;
      info.nMin = 0;
      info.nMax = 100;
      info.nPage = 2;
      info.nPos = 5;
      info.nTrackPos = 0;
        m_cScrollBar.SetScrollInfo(&info);
    */

    m_cScrollBar.SetScrollRange(0, 100);



    //m_cScrollBar.SetScrollRange(25, 25 + (rcClientUIElements.bottom - rcClientUIElements.top) + 175);
}

void CToolBarTab::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
    ////////////////////////////////////////////////////////////////////////
    // Show the toolbar associated with the current tab
    ////////////////////////////////////////////////////////////////////////

    // TODO: Add your control notification handler code here

    TCITEM tci;

    tci.mask = TCIF_PARAM;
    GetItem(GetCurSel(), &tci);
    ASSERT(::IsWindow((HWND) tci.lParam));

    // Show the current toolbar window
    ::ShowWindow((HWND) tci.lParam, SW_NORMAL);

    // Make sure the window is placed correctly
    ResizeAllContainers();

    *pResult = 0;
}

void CToolBarTab::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
    ////////////////////////////////////////////////////////////////////////
    // Current tab is about to change, hide the toolbar associated with the
    // current tab
    ////////////////////////////////////////////////////////////////////////

    // TODO: Add your control notification handler code here

    TCITEM tci;

    // Hide the current toolbar
    tci.mask = TCIF_PARAM;
    GetItem(GetCurSel(), &tci);
    ASSERT(::IsWindow((HWND) tci.lParam));
    ::ShowWindow((HWND) tci.lParam, SW_HIDE);

    *pResult = 0;
}

BOOL CToolBarTab::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    ////////////////////////////////////////////////////////////////////////
    // Route the toolbars events to the main window
    ////////////////////////////////////////////////////////////////////////

    // TODO: Add your specialized code here and/or call the base class

    if (AfxGetMainWnd())
    {
        AfxGetMainWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    }

    return CTabCtrl::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CToolBarTab::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // Change the position of the UI elements to reflect the new scrollbar position
    //ResizeAllContainers();

    switch (nSBCode)
    {
    case SB_LINEDOWN:
        pScrollBar->SetScrollPos(nPos + 1);
        break;
    }

    CTabCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}
