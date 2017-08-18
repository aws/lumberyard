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

// Description : This is the source file for the general utility dialog for
//               string input. The purpose of this dialog, as one might imagine, is to get
//               string input for any purpose necessary.
//               The recomended way to call this dialog is through DoModal()
//               method.


#include "stdafx.h"
#include "StringInputDialog.h"

//////////////////////////////////////////////////////////////////////////
CStringInputDialog::CStringInputDialog()
    : CDialog(IDD_DIALOG_RENAME)
    , m_strText("")
    , m_strTitle("Please type your text in the text box bellow")
{
}
//////////////////////////////////////////////////////////////////////////
CStringInputDialog::CStringInputDialog(CString strName, CString strCaption)
    : CDialog(IDD_DIALOG_RENAME)
    , m_strText(strName)
    , m_strTitle(strCaption)
{
}
//////////////////////////////////////////////////////////////////////////
void CStringInputDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_NAME, m_nameEdit);
    DDX_Text(pDX, IDC_EDIT_NAME, m_strText);
}
//////////////////////////////////////////////////////////////////////////
BOOL CStringInputDialog::OnInitDialog()
{
    if (CDialog::OnInitDialog() == FALSE)
    {
        return FALSE;
    }

    SetWindowText(m_strTitle);

    UpdateData(FALSE);

    // set the focus on the edit control
    m_nameEdit.SetFocus();
    // move the cursor at the end of the text; ( 0, -1 ) would select the entire text
    m_nameEdit.SetSel(m_strText.GetLength(), -1);

    return FALSE;
}
//////////////////////////////////////////////////////////////////////////
void CStringInputDialog::OnOK()
{
    UpdateData(TRUE);
    CDialog::OnOK();
}
//////////////////////////////////////////////////////////////////////////
void CStringInputDialog::SetText(CString strName)
{
    m_strText = strName;
}
//////////////////////////////////////////////////////////////////////////
void CStringInputDialog::SetTitle(CString strCaption)
{
    m_strTitle = strCaption;
}
//////////////////////////////////////////////////////////////////////////
CString CStringInputDialog::GetResultingText()
{
    return m_strText;
}
//////////////////////////////////////////////////////////////////////////
