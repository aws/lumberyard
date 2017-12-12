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

// Description : Implements base dialog for task panel embedded dialogs.


#ifndef CRYINCLUDE_EDITOR_UIPANELS_TASKPANELDLG_H
#define CRYINCLUDE_EDITOR_UIPANELS_TASKPANELDLG_H
#pragma once


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
class SANDBOX_API CTaskPanelDlg
    : public CDialog
{
public:
    //-------------------------------------------------------------------------
    CTaskPanelDlg(CWnd* pParent = NULL);

    //-------------------------------------------------------------------------
    void SetItem(CXTPTaskPanelGroupItem* pItem)
    {
        m_pItem = pItem;
    }

    //-------------------------------------------------------------------------
    virtual void OnOK() { }
    virtual void OnCancel() { }
    virtual BOOL PreTranslateMessage(MSG* pMsg) { return CDialog::PreTranslateMessage(pMsg); }

    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam)
    {
        // Notify task panel's parent
        CWnd* pParent = GetParent();
        if (pParent)
        {
            pParent = pParent->GetParent();
            if (pParent)
            {
                pParent->SendMessage(WM_COMMAND, wParam, lParam);
                return TRUE;
            }
        }
        return FALSE;
    }

protected:
    //-------------------------------------------------------------------------
    void UpdateColors();

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

protected:
    CXTPTaskPanelGroupItem* m_pItem;
    CBrush      m_brushBack;
    COLORREF    m_clrBack;

    DECLARE_MESSAGE_MAP()
};

#endif // CRYINCLUDE_EDITOR_UIPANELS_TASKPANELDLG_H
