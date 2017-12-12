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

// Description : A list control with a tooltip support
// Notice      : Also see CTootipComboBox


#ifndef CRYINCLUDE_EDITOR_CONTROLS_TOOLTIPLISTCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_TOOLTIPLISTCTRL_H
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTooltipListCtrl window

class CTooltipListCtrl
    : public CListCtrl
{
    DECLARE_DYNAMIC(CTooltipListCtrl)
public:
    CTooltipListCtrl();
    CString GetItemTip(int nRow) const;
    int SetItemTip(int nRow, CString sTip);
    void Display(CRect rc);
    void Init(CComboBox* pComboParent);
    virtual ~CTooltipListCtrl();

    virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
    virtual void PreSubclassWindow();
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;

    BOOL OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

    int m_nLastItem;
    CComboBox* m_pComboParent;
    CMap< int, int&, CString, CString& > m_mpItemToTip;

    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TOOLTIPLISTCTRL_H
