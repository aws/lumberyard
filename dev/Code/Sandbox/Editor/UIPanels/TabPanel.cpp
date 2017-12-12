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

// Description : Terrain modification tool.


#include "StdAfx.h"
#include "TabPanel.h"

#define IDC_TASKPANEL 1
/////////////////////////////////////////////////////////////////////////////
// CTabPanel dialog

IMPLEMENT_DYNCREATE(CTabPanel, CDialog)

CString CTabPanel::m_lastSelectedPage;

CTabPanel::CTabPanel(CWnd* pParent)
    : CDialog(CTabPanel::IDD, pParent)
{
    Create(IDD, pParent);
}

//////////////////////////////////////////////////////////////////////////
CTabPanel::~CTabPanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CTabPanel::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTabPanel, CDialog)
ON_WM_SIZE()
ON_NOTIFY((TCN_SELCHANGE - 10), IDC_TAB, OnTabSelect)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabPanel message handlers
BOOL CTabPanel::OnInitDialog()
{
    CDialog::OnInitDialog();

    {
        CRect wrc;
        GetWindowRect(wrc);
        wrc.bottom = wrc.top + 500;
        MoveWindow(wrc, FALSE);
    }

    CRect rc;
    GetClientRect(rc);

    //  m_tab.CreateEx(NULL, AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,
    //      AfxGetApp()->LoadStandardCursor(IDC_ARROW),NULL,NULL),"", WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,CRect(0,0,10,10),this,IDC_TAB);
    m_tab.Create(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_MULTILINE | TCS_HOTTRACK, rc, this, IDC_TAB);
    m_tab.SetFont(CFont::FromHandle((HFONT)gSettings.gui.hSystemFont));

    //m_tab.GetPaintManager()->m_bHotTracking = true;
    //m_tab.GetPaintManager()->m_bOneNoteColors = true;
    //m_tab.GetPaintManager()->SetAppearance(xtpTabAppearancePropertyPage);
    //m_tab.GetPaintManager()->SetColor(xtpTabColorOffice2003);
    //m_tab.GetPaintManager()->SetLayout(xtpTabLayoutAutoSize);
    //m_tab.GetPaintManager()->m_rcButtonMargin = CRect(2,0,2,2);
    //m_tab.GetPaintManager()->m_rcClientMargin = CRect(0,4,0,0);

    // Clip children of the tab control from paint routines to reduce flicker.
    //m_tab.ModifyStyle(0L,WS_CLIPCHILDREN|WS_CLIPSIBLINGS);

    return TRUE;  // return TRUE unless you set the focus to a control
}

//////////////////////////////////////////////////////////////////////////
void CTabPanel::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);

    if (m_tab.m_hWnd)
    {
        m_tab.MoveWindow(rc, TRUE);
    }
}

//////////////////////////////////////////////////////////////////////////
int CTabPanel::AddPage(const char* sCaption, CDialog* pDlg, bool bAutoDestroy)
{
    int nPageId = m_tab.AddPage(sCaption, pDlg, bAutoDestroy);
    if (sCaption == m_lastSelectedPage)
    {
        m_tab.SelectPage(nPageId);
    }
    return nPageId;
}

//////////////////////////////////////////////////////////////////////////
void CTabPanel::RemovePage(int nPageId)
{
    m_tab.RemovePage(nPageId);
}

//////////////////////////////////////////////////////////////////////////
void CTabPanel::SelectPage(int nPageId)
{
    m_tab.SelectPage(nPageId);
}

//////////////////////////////////////////////////////////////////////////
int CTabPanel::GetPageCount()
{
    return m_tab.GetItemCount();
}

//////////////////////////////////////////////////////////////////////////
void CTabPanel::OnTabSelect(NMHDR* pNMHDR, LRESULT* pResult)
{
    int i = m_tab.GetCurSel();
    if (i >= 0)
    {
        m_tab.SelectPageByIndex(i);

        TCITEM item;
        ZeroStruct(item);
        item.mask = TCIF_TEXT;
        char buf[1024];
        item.pszText = buf;
        item.cchTextMax = sizeof(buf);
        if (m_tab.GetItem(i, &item))
        {
            m_lastSelectedPage = item.pszText;
        }
    }
}
