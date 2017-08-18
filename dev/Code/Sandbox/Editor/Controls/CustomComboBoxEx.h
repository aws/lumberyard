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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_CUSTOMCOMBOBOXEX_H
#define CRYINCLUDE_EDITOR_CONTROLS_CUSTOMCOMBOBOXEX_H
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CCustomComboBoxEx template control

template<class BASE_TYPE>
class CCustomComboBoxEx
    : public BASE_TYPE
{
protected:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg UINT OnGetDlgCode();

    DECLARE_TEMPLATE_MESSAGE_MAP()
};

BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(class BASE_TYPE, CCustomComboBoxEx<BASE_TYPE>, BASE_TYPE)
//{{AFX_MSG_MAP(CColorPushButton)
ON_WM_GETDLGCODE()
//}}AFX_MSG_MAP
END_TEMPLATE_MESSAGE_MAP_CUSTOM()

//////////////////////////////////////////////////////////////////////////
template<class BASE_TYPE>
UINT CCustomComboBoxEx<BASE_TYPE>::OnGetDlgCode()
{
    return DLGC_WANTMESSAGE;
}

//////////////////////////////////////////////////////////////////////////
template<class BASE_TYPE>
BOOL CCustomComboBoxEx<BASE_TYPE>::PreTranslateMessage(MSG* pMsg)
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

    return BASE_TYPE::PreTranslateMessage(pMsg);
}


#endif // CRYINCLUDE_EDITOR_CONTROLS_CUSTOMCOMBOBOXEX_H
