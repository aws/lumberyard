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
#include "TaskPanel.h"

#define IDC_TASKPANEL 1
/////////////////////////////////////////////////////////////////////////////
// CTaskPanel dialog

IMPLEMENT_DYNCREATE(CTaskPanel, CDialog)

//////////////////////////////////////////////////////////////////////////
/// CXTPTaskPanelOffice2003ThemePlain

class CTaskPanelTheme
    : public CXTPTaskPanelPaintManager
{
public:
    CTaskPanelTheme()
    {
        m_nCaptionHeight = 0;
        m_bOfficeHighlight = 2;
        RefreshMetrics();
    }
    virtual int DrawGroupCaption(CDC* pDC, CXTPTaskPanelGroup* pGroup, BOOL bDraw)
    {
        return CXTPTaskPanelPaintManager::DrawGroupCaption(pDC, pGroup, bDraw);
    }
    void RefreshMetrics()
    {
        CXTPTaskPanelPaintManager::RefreshMetrics();

        m_eGripper = xtpTaskPanelGripperNone;

        XTPCurrentSystemTheme systemTheme = XTPColorManager()->GetCurrentSystemTheme();

        COLORREF clrBackground;
        COLORREF clr3DShadow;

        switch (systemTheme)
        {
        case xtpSystemThemeBlue:
        /*
        case xtpSystemThemeBlue:
        clrBackground = RGB(221, 236, 254);
        clr3DShadow = RGB(123, 164, 224);
        break;

        case xtpSystemThemeOlive:
        clrBackground = RGB(243, 242, 231);
        clr3DShadow = RGB(188, 187, 177);
        break;

        case xtpSystemThemeSilver:
        clrBackground = RGB(238, 238, 244);
        clr3DShadow = RGB(161, 160, 187);
        break;
        */
        default:
            //clrBackground = XTPColorManager()->LightColor(GetSysColor(COLOR_3DFACE), GetSysColor(COLOR_WINDOW), 50);
            clrBackground = GetSysColor(COLOR_3DFACE);//, GetSysColor(COLOR_WINDOW), 50);
            clr3DShadow = GetXtremeColor(COLOR_3DSHADOW);
        }

        m_clrBackground = CXTPPaintManagerColorGradient(clrBackground, clrBackground);

        m_groupNormal.clrClient = clrBackground;
        m_groupNormal.clrHead = CXTPPaintManagerColorGradient(clr3DShadow, clr3DShadow);
        m_groupNormal.clrClientLink = m_groupNormal.clrClientLinkHot = RGB(0, 0, 0);

        m_groupSpecial = m_groupNormal;
    }
};

CTaskPanel::CTaskPanel(CWnd* pParent)
    : CDialog(CTaskPanel::IDD, pParent)
{
    m_pTheme = 0;
    Create(IDD, pParent);
}

//////////////////////////////////////////////////////////////////////////
CTaskPanel::~CTaskPanel()
{
}

//////////////////////////////////////////////////////////////////////////
void CTaskPanel::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTaskPanel, CDialog)
ON_WM_SIZE()
ON_MESSAGE(XTPWM_TASKPANEL_NOTIFY, OnTaskPanelNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTaskPanel message handlers
BOOL CTaskPanel::OnInitDialog()
{
    CDialog::OnInitDialog();

    CRect rc;
    GetClientRect(rc);

    m_wndTaskPanel.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rc, this, IDC_TASKPANEL);

    m_pTheme = new CTaskPanelTheme;
    m_wndTaskPanel.SetBehaviour(xtpTaskPanelBehaviourExplorer);
    m_wndTaskPanel.SetCustomTheme(m_pTheme);
    //m_wndTaskPanel.SetTheme(xtpTaskPanelThemeNativeWinXP);
    m_wndTaskPanel.SetHotTrackStyle(xtpTaskPanelHighlightItem);
    //m_wndTaskPanel.SetHotTrackStyle(xtpTaskPanelHighlightText);
    m_wndTaskPanel.AllowDrag(FALSE);

    m_wndTaskPanel.GetPaintManager()->m_rcGroupOuterMargins.SetRect(0, 0, 0, 0);
    m_wndTaskPanel.GetPaintManager()->m_rcGroupInnerMargins.SetRect(0, 0, 0, 0);
    m_wndTaskPanel.GetPaintManager()->m_rcItemOuterMargins.SetRect(0, 0, 0, 0);
    m_wndTaskPanel.GetPaintManager()->m_rcItemInnerMargins.SetRect(1, 3, 1, 3);
    m_wndTaskPanel.GetPaintManager()->m_rcControlMargins.SetRect(2, 0, 2, 0);
    m_wndTaskPanel.GetPaintManager()->m_nGroupSpacing = 0;

    //m_wndTaskPanel.GetPaintManager()->m_bOfficeHighlight = TRUE;
    m_wndTaskPanel.SetAnimation(xtpTaskPanelAnimationNo);

    UpdateTools();

    return TRUE;  // return TRUE unless you set the focus to a control
}

//////////////////////////////////////////////////////////////////////////
void CTaskPanel::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);

    if (m_wndTaskPanel.m_hWnd)
    {
        m_wndTaskPanel.MoveWindow(rc, TRUE);
    }
}

//////////////////////////////////////////////////////////////////////////
LRESULT CTaskPanel::OnTaskPanelNotify(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case XTP_TPN_CLICK:
    {
        CXTPTaskPanelGroupItem* pItem = (CXTPTaskPanelGroupItem*)lParam;
        UINT nCmdID = pItem->GetID();
        //SelectCmd( nCmdID );
    }
    break;

    case XTP_TPN_RCLICK:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTaskPanel::UpdateTools()
{
    CXTPTaskPanelGroup* pFolder = m_wndTaskPanel.AddGroup(1);
    pFolder->SetItemLayout(xtpTaskItemLayoutDefault);
    pFolder->SetExpandable(FALSE);
    pFolder->SetExpanded(TRUE);
    pFolder->ShowCaption(FALSE);

    CXTPTaskPanelGroupItem* pItem;
    int nCommand = 1;
    pItem =  pFolder->AddLinkItem(nCommand++);
    pItem->SetType(xtpTaskItemTypeLink);
    pItem->SetCaption(CString(MAKEINTRESOURCE(IDS_CLIPFRONTBRUSH_CAPTION)));

    pItem =  pFolder->AddLinkItem(nCommand++);
    pItem->SetType(xtpTaskItemTypeLink);
    pItem->SetCaption(CString(MAKEINTRESOURCE(IDS_CLIPBACKBRUSH_CAPTION)));

    pItem =  pFolder->AddLinkItem(nCommand++);
    pItem->SetType(xtpTaskItemTypeLink);
    pItem->SetCaption(CString(MAKEINTRESOURCE(IDS_SPLITBRUSH_CAPTION)));

    m_wndTaskPanel.Reposition(TRUE);

    CRect rc = pFolder->GetClientRect();
    rc.bottom += 10;
    SetWindowPos(NULL, 0, 0, rc.Width(), rc.Height(), SWP_NOMOVE);
}
