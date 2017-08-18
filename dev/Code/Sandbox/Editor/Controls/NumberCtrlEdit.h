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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_NUMBERCTRLEDIT_H
#define CRYINCLUDE_EDITOR_CONTROLS_NUMBERCTRLEDIT_H
#pragma once


#include <functor.h>

class CNumberCtrlEdit
    : public CEdit
{
    CNumberCtrlEdit(const CNumberCtrlEdit& d);
    CNumberCtrlEdit& operator=(const CNumberCtrlEdit& d);

public:
    typedef Functor0 UpdateCallback;

    CNumberCtrlEdit()
        : m_bUpdateOnKillFocus(true)
        , m_bSwallowReturn(false)
    {};

    // Attributes
    void SetText(const CString& strText);

    //! Set callback function called when number edit box is really updated.
    void SetUpdateCallback(UpdateCallback func) { m_onUpdate = func; }
    void EnableUpdateOnKillFocus(bool bEnable)    { m_bUpdateOnKillFocus = bEnable;   }
    void SetSwallowReturn(bool bDoSwallow) { m_bSwallowReturn = bDoSwallow; }

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNumberCtrlEdit)
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

    // Generated message map functions
protected:
    //{{AFX_MSG(CNumberCtrlEdit)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    bool IsValidChar(UINT nChar);

    DECLARE_MESSAGE_MAP()

    // Data
protected:

    bool m_bUpdateOnKillFocus;
    bool m_bSwallowReturn;
    CString m_strInitText;
    UpdateCallback m_onUpdate;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_NUMBERCTRLEDIT_H
