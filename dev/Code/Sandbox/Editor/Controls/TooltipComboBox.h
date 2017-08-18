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
// Notice      : See CTooltipListCtrl also


#ifndef CRYINCLUDE_EDITOR_CONTROLS_TOOLTIPCOMBOBOX_H
#define CRYINCLUDE_EDITOR_CONTROLS_TOOLTIPCOMBOBOX_H
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTooltipComboBox window
#include "TooltipListCtrl.h"

class CTooltipComboBox
    : public CComboBox
{
    DECLARE_DYNAMIC(CTooltipComboBox)
public:
    CTooltipComboBox();
    CString GetComboTip() const;
    void SetComboTip(CString sTip);
    int SetItemTip(int nRow, CString sTip);
    BOOL GetDroppedState() const;
    int GetDroppedHeight() const;
    int GetDroppedWidth() const;
    int SetDroppedHeight(UINT nHeight);
    int SetDroppedWidth(UINT nWidth);
    void DisplayList(BOOL bDisplay = TRUE);
    virtual ~CTooltipComboBox();

protected:
    virtual void PreSubclassWindow();
    virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
    BOOL OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

    int m_nDroppedHeight;
    int m_nDroppedWidth;
    CTooltipListCtrl m_lstCombo;

    CString m_sTip;
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TOOLTIPCOMBOBOX_H
