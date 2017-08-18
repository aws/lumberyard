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

#ifndef CRYINCLUDE_EDITOR_NUMBERDLG_H
#define CRYINCLUDE_EDITOR_NUMBERDLG_H

#pragma once
// NumberDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNumberDlg dialog

class CNumberDlg
    : public CDialog
{
    // Construction
public:
    CNumberDlg(CWnd* pParent = NULL, float defValue = 0, const char* title = NULL);   // standard constructor

    // Dialog Data
    //{{AFX_DATA(CNumberDlg)
    enum
    {
        IDD = IDD_NUMBER
    };
    //}}AFX_DATA

    float GetValue();
    void SetRange(float min, float max);
    void SetInteger(bool bEnable);

    CNumberCtrl& GetNumControl() { return m_num; }

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNumberDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    float m_value;
    bool m_bInteger;

    // Generated message map functions
    //{{AFX_MSG(CNumberDlg)
    afx_msg void OnClose();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CNumberCtrl m_num;
    CString m_title;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_NUMBERDLG_H
