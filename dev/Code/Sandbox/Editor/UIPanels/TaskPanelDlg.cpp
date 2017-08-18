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

// Description : Implements character parts dialog.


#include "stdafx.h"
#include "TaskPanelDlg.h"

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CTaskPanelDlg, CDialog)
ON_WM_SIZE()
ON_WM_ERASEBKGND()
ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

//-------------------------------------------------------------------------
CTaskPanelDlg::CTaskPanelDlg(CWnd* pParent /* =NULL */)
    : CDialog(UINT(0), pParent)
    , m_pItem(NULL)
{
}

//-------------------------------------------------------------------------
void CTaskPanelDlg::UpdateColors()
{
    if (!m_pItem)
    {
        return;
    }

    COLORREF clrBack = m_pItem->GetBackColor();

    if (!m_brushBack.GetSafeHandle() || clrBack != m_clrBack)
    {
        m_brushBack.DeleteObject();
        m_brushBack.CreateSolidBrush(clrBack);
        m_clrBack = clrBack;
    }
}

//-------------------------------------------------------------------------
void CTaskPanelDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    Invalidate(FALSE);
}

//-------------------------------------------------------------------------
BOOL CTaskPanelDlg::OnEraseBkgnd(CDC* pDC)
{
    UpdateColors();
    pDC->FillSolidRect(CXTPClientRect(this), m_clrBack);

    return TRUE;
}

//-------------------------------------------------------------------------
HBRUSH CTaskPanelDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor != CTLCOLOR_BTN && nCtlColor != CTLCOLOR_STATIC)
    {
        return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    }

    UpdateColors();
    pDC->SetBkMode(TRANSPARENT);

    return m_brushBack;
}