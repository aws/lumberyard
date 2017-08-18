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

#ifndef CRYINCLUDE_EDITOR_ENTITYSCRIPTDIALOG_H
#define CRYINCLUDE_EDITOR_ENTITYSCRIPTDIALOG_H
#pragma once


// CEntityScriptDialog dialog

class CEntityScript;

class CEntityScriptDialog
    : public CXTResizeDialog
{
    DECLARE_DYNAMIC(CEntityScriptDialog)
public:
    typedef Functor0 Callback;

    CEntityScriptDialog(CWnd* pParent = NULL);   // standard constructor
    virtual ~CEntityScriptDialog();

    // Dialog Data
    enum
    {
        IDD = IDD_DB_ENTITY_METHODS
    };

    void SetScript(CEntityScript* script, IEntity* m_entity);

    void SetOnReloadScript(Callback cb) { m_OnReloadScript = cb; };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    virtual void OnOK() {};
    virtual void OnCancel() {};

    void    ReloadMethods();
    void    ReloadEvents();

    void GotoMethod(const CString& method);

    afx_msg void OnEditScript();
    afx_msg void OnReloadScript();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnDblclkMethods();
    afx_msg void OnGotoMethod();
    afx_msg void OnAddMethod();
    afx_msg void OnRunMethod();

    DECLARE_MESSAGE_MAP()

    CCustomButton m_addMethodBtn;
    CCustomButton   m_runButton;
    CCustomButton   m_gotoMethodBtn;
    CCustomButton   m_editScriptBtn;
    CCustomButton   m_reloadScriptBtn;
    CCustomButton   m_removeButton;
    CStatic m_scriptName;
    CListBox    m_methods;
    CString m_selectedMethod;

    TSmartPtr<CEntityScript> m_script;
    IEntity* m_entity;
    CBrush m_grayBrush;

    Callback m_OnReloadScript;
};

#endif // CRYINCLUDE_EDITOR_ENTITYSCRIPTDIALOG_H
