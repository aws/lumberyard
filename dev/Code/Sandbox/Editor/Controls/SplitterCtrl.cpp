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

#include "stdafx.h"
#include "SplitterCtrl.h"

IMPLEMENT_DYNAMIC(CSplitterCtrl, CSplitterWnd)
CSplitterCtrl::CSplitterCtrl()
{
    m_cxSplitter = m_cySplitter = 3 + 1 + 1;
    m_cxBorderShare = m_cyBorderShare = 0;
    m_cxSplitterGap = m_cySplitterGap = 3 + 1 + 1;
    m_cxBorder = m_cyBorder = 1;
    m_bTrackable = true;
}

CSplitterCtrl::~CSplitterCtrl()
{
}


BEGIN_MESSAGE_MAP(CSplitterCtrl, CSplitterWnd)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
void CSplitterCtrl::SetNoBorders()
{
    m_cxSplitter = m_cySplitter = 0;
    m_cxBorderShare = m_cyBorderShare = 0;
    m_cxSplitterGap = m_cySplitterGap = 1;
    m_cxBorder = m_cyBorder = 0;
}

//////////////////////////////////////////////////////////////////////////
void CSplitterCtrl::SetTrackable(bool bTrackable)
{
    m_bTrackable = bTrackable;
}

// CSplitterCtrl message handlers

void CSplitterCtrl::SetPane(int row, int col, CWnd* pWnd, SIZE sizeInit)
{
    assert(pWnd != NULL);

    // set the initial size for that pane
    m_pColInfo[col].nIdealSize = sizeInit.cx;
    m_pRowInfo[row].nIdealSize = sizeInit.cy;

    pWnd->ModifyStyle(0, WS_BORDER, WS_CHILD | WS_VISIBLE);
    pWnd->SetParent(this);

    CRect rect(CPoint(0, 0), sizeInit);
    pWnd->MoveWindow(0, 0, sizeInit.cx, sizeInit.cy, FALSE);
    pWnd->SetDlgCtrlID(IdFromRowCol(row, col));

    ASSERT((int)::GetDlgCtrlID(pWnd->m_hWnd) == IdFromRowCol(row, col));
}

void CSplitterCtrl::OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg)
{
    // Let CSplitterCtrl handle everything but the border-drawing
    if ((nType != splitBorder) || (pDC == NULL))
    {
        CSplitterWnd::OnDrawSplitter(pDC, nType, rectArg);
        return;
    }

    ASSERT_VALID(pDC);

    // Draw border
    pDC->Draw3dRect(rectArg, QtUtil::GetSysColorPort(COLOR_BTNSHADOW), QtUtil::GetSysColorPort(COLOR_BTNHIGHLIGHT));
}

//////////////////////////////////////////////////////////////////////////
void CSplitterCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_bTrackable)
    {
        CSplitterWnd::OnLButtonDown(nFlags, point);
    }
    else
    {
        CWnd::OnLButtonDown(nFlags, point);
    }
}

//////////////////////////////////////////////////////////////////////////
CWnd* CSplitterCtrl::GetActivePane(int* pRow, int* pCol)
{
    return GetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWnd command routing

BOOL CSplitterCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (CWnd::OnCommand(wParam, lParam))
    {
        return TRUE;
    }

    // route commands to the splitter to the parent window
    return !GetParent()->SendMessage(WM_COMMAND, wParam, lParam);
}

BOOL CSplitterCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    if (CWnd::OnNotify(wParam, lParam, pResult))
    {
        return TRUE;
    }

    // route commands to the splitter to the parent window
    *pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
    return TRUE;
}
