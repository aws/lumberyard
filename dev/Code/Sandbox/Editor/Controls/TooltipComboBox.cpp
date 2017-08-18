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

// Description : A combo box with a tooltip support

#include "stdafx.h"
#include "TooltipComboBox.h"
#include "UnicodeFunctions.h"

/////////////////////////////////////////////////////////////////////////////
// CTooltipComboBox

CTooltipComboBox::CTooltipComboBox()
{
    m_nDroppedWidth = 0;
    m_nDroppedHeight = 0;
}

CTooltipComboBox::~CTooltipComboBox()
{
}

IMPLEMENT_DYNAMIC(CTooltipComboBox, CComboBox)

BEGIN_MESSAGE_MAP(CTooltipComboBox, CComboBox)
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONDBLCLK()
ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTooltipComboBox message handlers

void CTooltipComboBox::OnLButtonDown(UINT nFlags, CPoint point)
{
    DisplayList(!m_lstCombo.IsWindowVisible());
}

void CTooltipComboBox::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    OnLButtonDown(nFlags, point);
}

void CTooltipComboBox::DisplayList(BOOL bDisplay /* = TRUE*/)
{
    if (!bDisplay)
    {
        m_lstCombo.ShowWindow(SW_HIDE);

        return;
    }

    CRect rc;
    GetWindowRect(rc);
    rc.top = rc.bottom;
    rc.right = rc.left + GetDroppedWidth();
    rc.bottom = rc.top + GetDroppedHeight();

    m_lstCombo.Display(rc);
}

int CTooltipComboBox::GetDroppedHeight() const
{
    return m_nDroppedHeight;
}

int CTooltipComboBox::GetDroppedWidth() const
{
    return m_nDroppedWidth;
}

int CTooltipComboBox::SetDroppedHeight(UINT nHeight)
{
    m_nDroppedHeight = nHeight;

    return m_nDroppedHeight;
}

int CTooltipComboBox::SetDroppedWidth(UINT nWidth)
{
    m_nDroppedWidth = nWidth;

    return m_nDroppedWidth;
}

BOOL CTooltipComboBox::GetDroppedState() const
{
    return m_lstCombo.IsWindowVisible();
}

void CTooltipComboBox::PreSubclassWindow()
{
    CRect rc(0, 0, 10, 10);
    DWORD dwStyle =  WS_POPUP | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL | LVS_OWNERDATA;
    m_lstCombo.CWnd::CreateEx(0, WC_LISTVIEW, NULL, dwStyle, rc, this, 0);
    DWORD dwStyleEx = m_lstCombo.GetExtendedStyle();
    m_lstCombo.SetExtendedStyle(dwStyleEx | LVS_EX_FULLROWSELECT);
    m_lstCombo.Init(this);

    CRect rcAll;
    GetDroppedControlRect(&rcAll);
    GetWindowRect(&rc);
    SetDroppedWidth(rcAll.Width());
    SetDroppedHeight(rcAll.Height() - rc.Height());

    CComboBox::PreSubclassWindow();

    EnableToolTips();
}

INT_PTR CTooltipComboBox::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
    pTI->hwnd = m_hWnd;
    pTI->uId = ( UINT )1;
    pTI->lpszText = LPSTR_TEXTCALLBACK;

    CRect rc;
    GetWindowRect(&rc);
    ScreenToClient(&rc);
    pTI->rect = rc;

    return pTI->uId;
}

BOOL CTooltipComboBox::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    CString sTipText;
    UINT nID = pNMHDR->idFrom;

    if (nID == 0)       // Notification in NT from automatically
    {
        return FALSE;       // created tooltip
    }
    sTipText = GetComboTip();

#ifndef _UNICODE
    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        lstrcpyn(pTTTA->szText, sTipText, 80);
    }
    else
    {
        Unicode::Convert(pTTTW->szText, sTipText.GetBuffer());
    }
#else
    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        Unicode::Convert(pTTTA->szText, sTipText.GetBuffer());
    }
    else
    {
        lstrcpyn(pTTTW->szText, sTipText, 80);
    }
#endif
    *pResult = 0;

    return TRUE;    // message was handled
}

CString CTooltipComboBox::GetComboTip() const
{
    return m_sTip;
}

void CTooltipComboBox::SetComboTip(CString sTip)
{
    m_sTip = sTip;
}

int CTooltipComboBox::SetItemTip(int nRow, CString sTip)
{
    return m_lstCombo.SetItemTip(nRow, sTip);
}
