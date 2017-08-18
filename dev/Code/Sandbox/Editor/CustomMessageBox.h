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

#ifndef CRYINCLUDE_EDITOR_CUSTOMMESSAGEBOX_H
#define CRYINCLUDE_EDITOR_CUSTOMMESSAGEBOX_H

#pragma once
// CustomBox.h : header file
//

// Custom Message box

/////////////////////////////////////////////////////////////////////////////
// Custom Message box Dialog

class CCustomMessageBox
    : public CDialog
{
    // Construction
public:
    CCustomMessageBox(CWnd* pParent, LPCSTR lpText, LPCSTR lpCaption, LPCSTR lpButtonName1, LPCSTR lpButtonName2 = 0, LPCSTR lpButtonName3 = 0, LPCSTR lpButtonName4 = 0);

    // Dialog Data
    //{{AFX_DATA(CCustomMessageBox)
    //}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCustomMessageBox)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CCustomMessageBox();
    int GetValue() {   return m_nValue; }
    static int Show(LPCSTR lpText, LPCSTR lpCaption, LPCSTR lpButtonName1, LPCSTR lpButtonName2 = 0, LPCSTR lpButtonName3 = 0, LPCSTR lpButtonName4 = 0);

protected:
    // Generated message map functions
    //{{AFX_MSG(CCustomMessageBox)
    afx_msg void OnButton1();
    afx_msg void OnButton2();
    afx_msg void OnButton3();
    afx_msg void OnButton4();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()


    CString m_message;
    CString m_caption;
    CString m_buttonName1, m_buttonName2, m_buttonName3, m_buttonName4;
    int m_nValue;
};

#endif // CRYINCLUDE_EDITOR_CUSTOMMESSAGEBOX_H
