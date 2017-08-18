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
#include "SelectionCombo.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectionCombo

CSelectionCombo::CSelectionCombo()
{
}

CSelectionCombo::~CSelectionCombo()
{
}


BEGIN_MESSAGE_MAP(CSelectionCombo, CXTFlatComboBox)
//{{AFX_MSG_MAP(CSelectionCombo)
ON_WM_GETDLGCODE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectionCombo message handlers

BOOL CSelectionCombo::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
    return CXTFlatComboBox::Create(dwStyle, rect, pParentWnd, nID);
}

UINT CSelectionCombo::OnGetDlgCode()
{
    return DLGC_WANTMESSAGE;
}

BOOL CSelectionCombo::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_RETURN)
        {
            NMCBEENDEDIT endEdit;
            endEdit.hdr.code = CBEN_ENDEDIT;
            endEdit.hdr.hwndFrom = m_hWnd;
            endEdit.hdr.idFrom = GetDlgCtrlID();
            endEdit.fChanged = true;
            endEdit.iNewSelection = CB_ERR;
            endEdit.iWhy = CBENF_RETURN;

            CString text;
            GetWindowText(text);
            strcpy(endEdit.szText, text);

            GetParent()->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)(&endEdit));
            return TRUE;
        }
        if (pMsg->wParam == VK_ESCAPE)
        {
            SetWindowText("");
            return TRUE;
        }
    }

    if (pMsg->message == WM_KILLFOCUS)
    {
        NMCBEENDEDIT endEdit;
        endEdit.hdr.code = CBEN_ENDEDIT;
        endEdit.hdr.hwndFrom = m_hWnd;
        endEdit.hdr.idFrom = GetDlgCtrlID();
        endEdit.fChanged = true;
        endEdit.iNewSelection = CB_ERR;
        endEdit.iWhy = CBENF_KILLFOCUS;

        CString text;
        GetWindowText(text);
        strcpy(endEdit.szText, text);

        GetParent()->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)(&endEdit));
        return TRUE;
    }

    return CXTFlatComboBox::PreTranslateMessage(pMsg);
}
