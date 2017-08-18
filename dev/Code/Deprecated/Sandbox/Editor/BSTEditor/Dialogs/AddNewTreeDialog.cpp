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
#include "AddNewTreeDialog.h"

#include "DialogCommon.h"

#include "BSTEditor/SelectionTreeManager.h"


IMPLEMENT_DYNAMIC(CAddNewTreeDialog, CDialog)
CAddNewTreeDialog::CAddNewTreeDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewTreeDialog::IDD, pParent)
{
}

void CAddNewTreeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BST_TREE_NAME, m_editName);
}

BEGIN_MESSAGE_MAP(CAddNewTreeDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddNewTreeDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this);

	SetWindowText(m_title.c_str());

	m_editName.SetWindowText( m_name.c_str() );

	return TRUE;

}

void CAddNewTreeDialog::OnBnClickedOk()
{
	CString tempString;
	m_editName.GetWindowText(tempString);
	m_name = tempString;
	OnOK();
}

