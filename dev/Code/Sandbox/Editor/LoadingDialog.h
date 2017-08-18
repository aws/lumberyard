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

#ifndef CRYINCLUDE_EDITOR_LOADINGDIALOG_H
#define CRYINCLUDE_EDITOR_LOADINGDIALOG_H

#pragma once
// LoadingDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoadingDialog dialog

class CLoadingDialog
    : public CDialog
{
    // Construction
public:
    CLoadingDialog(CWnd* pParent = NULL);   // standard constructor

    // Dialog Data
    //{{AFX_DATA(CLoadingDialog)
    enum
    {
        IDD = IDD_LOADING
    };
    CListBox    m_lstConsoleOutput;
    //}}AFX_DATA


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLoadingDialog)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CLoadingDialog)
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_LOADINGDIALOG_H
