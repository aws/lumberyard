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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_INPLACEEDIT_H
#define CRYINCLUDE_EDITOR_CONTROLS_INPLACEEDIT_H
#pragma once


class CInPlaceEdit
    : public CXTPEdit
{
public:
    typedef Functor0 OnChange;

    CInPlaceEdit(const CString& srtInitText, OnChange onchange);
    virtual ~CInPlaceEdit();

    // Attributes
    void SetText(const CString& strText);

    void EnableUpdateOnKillFocus(bool bEnable)    { m_bUpdateOnKillFocus = bEnable;   }
    void SetUpdateOnEnChange(bool bEnable) { m_bUpdateOnEnChange = bEnable; }

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CInPlaceEdit)
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

    // Generated message map functions
protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnEnChange();

    DECLARE_MESSAGE_MAP()

    // Data
protected:

    bool m_bUpdateOnKillFocus;
    bool m_bUpdateOnEnChange;
    CString m_strInitText;
    OnChange m_onChange;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_INPLACEEDIT_H
