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

#ifndef CRYINCLUDE_EDITOR_PROPERTIESDIALOG_H
#define CRYINCLUDE_EDITOR_PROPERTIESDIALOG_H

#pragma once
// PropertiesDialog.h : header file
//


#include "Controls/PropertyCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertiesDialog dialog

class CPropertiesDialog
    : public CXTResizeDialog
{
    // Construction
public:
    CPropertiesDialog(const CString& title, XmlNodeRef& node, CWnd* pParent = NULL, bool bShowSearchBar = false);   // standard constructor
    typedef Functor1<IVariable*> UpdateVarCallback;

    // Dialog Data
    //{{AFX_DATA(CPropertiesDialog)
    enum
    {
        IDD = IDD_PROPERTIES
    };
    // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    void SetUpdateCallback(UpdateVarCallback cb) { m_varCallback = cb; };
    CPropertyCtrl* GetPropertyCtrl() { return &m_wndProps; };

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPropertiesDialog)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    virtual void OnOK() {};
    virtual void OnCancel();

    void OnPropertyChange(IVariable* pVar);
    void ConfigureLayout();

    // Generated message map functions
    //{{AFX_MSG(CPropertiesDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    CPropertyCtrl m_wndProps;
    XmlNodeRef m_node;
    CString m_title;
    UpdateVarCallback m_varCallback;

    bool m_bShowSearchBar;
    CEdit m_input;
    CStatic m_searchLabel;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_PROPERTIESDIALOG_H
