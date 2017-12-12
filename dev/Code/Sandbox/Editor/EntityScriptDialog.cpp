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

#include "stdafx.h"
#include "EntityScriptDialog.h"

#include "Objects/EntityScript.h"
#include "StringDlg.h"

// CEntityScriptDialog dialog

IMPLEMENT_DYNAMIC(CEntityScriptDialog, CXTResizeDialog)
CEntityScriptDialog::CEntityScriptDialog(CWnd* pParent /*=NULL*/)
    : CXTResizeDialog(CEntityScriptDialog::IDD, pParent)
{
    m_script = 0;
    m_grayBrush.CreateSolidBrush(RGB(0xE0, 0xE0, 0xE0));
}

//////////////////////////////////////////////////////////////////////////
CEntityScriptDialog::~CEntityScriptDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::DoDataExchange(CDataExchange* pDX)
{
    CXTResizeDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RUN_METHOD, m_runButton);
    DDX_Control(pDX, IDC_GOTO_METHOD, m_gotoMethodBtn);
    DDX_Control(pDX, IDC_EDITSCRIPT, m_editScriptBtn);
    DDX_Control(pDX, IDC_ADD_METHOD, m_addMethodBtn);
    DDX_Control(pDX, IDC_RELOADSCRIPT, m_reloadScriptBtn);
    DDX_Control(pDX, IDC_METHODS, m_methods);
    DDX_LBString(pDX, IDC_METHODS, m_selectedMethod);
    DDX_Control(pDX, IDC_NAME, m_scriptName);
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CEntityScriptDialog, CXTResizeDialog)
ON_BN_CLICKED(IDC_EDITSCRIPT, OnEditScript)
ON_BN_CLICKED(IDC_RELOADSCRIPT, OnReloadScript)
ON_WM_CTLCOLOR()
ON_LBN_DBLCLK(IDC_METHODS, OnDblclkMethods)
ON_BN_CLICKED(IDC_GOTO_METHOD, OnGotoMethod)
ON_BN_CLICKED(IDC_ADD_METHOD, OnAddMethod)
ON_BN_CLICKED(IDC_RUN_METHOD, OnRunMethod)
END_MESSAGE_MAP()


// CEntityScriptDialog message handlers
//////////////////////////////////////////////////////////////////////////
BOOL CEntityScriptDialog::OnInitDialog()
{
    CXTResizeDialog::OnInitDialog();

    SetResize(IDC_METHODS, SZ_RESIZE(1));
    SetResize(IDC_METHODS_FRAME, SZ_RESIZE(1));

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CEntityScriptDialog message handlers
void CEntityScriptDialog::SetScript(CEntityScript* script, IEntity* entity)
{
    if (script)
    {
        m_scriptName.SetWindowText(script->GetName());
    }
    else
    {
        m_scriptName.SetWindowText("");
    }

    m_script = script;
    m_entity = entity;
    ReloadMethods();
    //ReloadEvents();
}


//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::OnEditScript()
{
    if (!m_script)
    {
        return;
    }

    CEntityScript* script = m_script;
    CFileUtil::EditTextFile(script->GetFile());
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::ReloadMethods()
{
    if (!m_script)
    {
        return;
    }

    // Parse entity lua file.
    CEntityScript* script = m_script;

    m_methods.ResetContent();
    ///if (script->Load( m_entity->GetEntityClass() ))
    {
        for (int i = 0; i < script->GetMethodCount(); i++)
        {
            m_methods.AddString(script->GetMethod(i));
        }
        for (int i = 0; i < script->GetEventCount(); i++)
        {
            m_methods.AddString(CString(EVENT_PREFIX) + script->GetEvent(i));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::OnReloadScript()
{
    if (!m_script)
    {
        return;
    }

    if (m_OnReloadScript)
    {
        m_OnReloadScript();
    }

    ReloadMethods();
}

//////////////////////////////////////////////////////////////////////////
HBRUSH CEntityScriptDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    // CWnd::GetDlgCtrlID() to perform the most efficient test.
    if (pWnd->GetDlgCtrlID() == IDC_METHODS)
    {
        // Set the text color to red
        //pDC->SetTextColor(RGB(255, 0, 0));

        // Set the background mode for text to transparent
        // so background will show thru.
        pDC->SetBkMode(TRANSPARENT);

        // Return handle to our CBrush object
        //hbr = (HBRUSH)GetStockObject( LTGRAY_BRUSH );
        hbr = m_grayBrush;
    }


    // TODO: Return a different brush if the default is not desired
    return hbr;
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::OnDblclkMethods()
{
    //UpdateData(TRUE);
    //GotoMethod( m_selectedMethod );

    OnRunMethod();
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::OnRunMethod()
{
    if (!m_script)
    {
        return;
    }
    if (!m_entity)
    {
        return;
    }

    UpdateData(TRUE);
    CEntityScript* script = m_script;
    script->RunMethod(m_entity, m_selectedMethod);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::GotoMethod(const CString& method)
{
    if (!m_script)
    {
        return;
    }

    CEntityScript* script = m_script;
    script->GotoMethod(method);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::OnGotoMethod()
{
    UpdateData(TRUE);
    GotoMethod(m_selectedMethod);
}

//////////////////////////////////////////////////////////////////////////
void CEntityScriptDialog::OnAddMethod()
{
    if (!m_script)
        return;

    StringDlg dlg(tr("Enter Method Name"));
    if (dlg.exec() == QDialog::Accepted)
    {
        CString method = QtUtil::ToQString(dlg.GetString());
        if (m_methods.FindString(-1, method) == LB_ERR)
        {
            CEntityScript *script = m_script;
            script->AddMethod(method);
            script->GotoMethod(method);
            OnReloadScript();
        }
    }
}
