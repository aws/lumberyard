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
#include "AddNewBlockGroupDialog.h"

#include "DialogCommon.h"

bool CAddNewBlockGroupDialog::m_sIsAdvanced = false;

IMPLEMENT_DYNAMIC(CAddNewBlockGroupDialog, CDialog)
CAddNewBlockGroupDialog::CAddNewBlockGroupDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewBlockGroupDialog::IDD, pParent)
{
}

void CAddNewBlockGroupDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_BST_EDIT_GROUPNAME, m_NameCtrl );
	DDX_Control( pDX, IDC_BST_CHECK_VARS, m_VarsCtrl );
	DDX_Control( pDX, IDC_BST_CHECK_SIGNALS, m_SignalsCtrl );
	DDX_Control( pDX, IDC_BST_CHECK_TREE, m_TreeCtrl );
	DDX_Control( pDX, IDC_BST_CHECK_MODE, m_AdvancedCtrl );
	DDX_Control( pDX, IDC_BST_EDIT_VARSNAME, m_VarNameCtrl );
	DDX_Control( pDX, IDC_BST_EDIT_SIGSNAME, m_SigNameCtrl );
	DDX_Control( pDX, IDC_BST_EDIT_TREENAME, m_TreeNameCtrl );
}

BEGIN_MESSAGE_MAP(CAddNewBlockGroupDialog, CDialog)
	ON_BN_CLICKED(IDC_BST_CHECK_MODE, OnToggleAdvanced)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddNewBlockGroupDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this);

	CDialog::SetWindowText(m_title.c_str());
	m_NameCtrl.SetWindowText( m_Name.c_str() );

	m_VarNameCtrl.SetWindowText( m_VarName.c_str() );
	m_SigNameCtrl.SetWindowText( m_SigName.c_str() );
	m_TreeNameCtrl.SetWindowText( m_TreeName.c_str() );

	m_VarsCtrl.SetCheck( m_bVars );
	m_SignalsCtrl.SetCheck( m_bSignals );
	m_TreeCtrl.SetCheck( m_bTree );

	SetAdvanced(m_sIsAdvanced);

	return TRUE;

}

void CAddNewBlockGroupDialog::SetAdvanced(bool IsAdvanced)
{
	m_VarsCtrl.ShowWindow( IsAdvanced );
	m_VarNameCtrl.ShowWindow( IsAdvanced );
	m_SignalsCtrl.ShowWindow( IsAdvanced );
	m_SigNameCtrl.ShowWindow( IsAdvanced );
	m_TreeCtrl.ShowWindow( IsAdvanced );
	m_TreeNameCtrl.ShowWindow( IsAdvanced );
	m_AdvancedCtrl.SetCheck( IsAdvanced );

	CRect formRect;
	CDialog::GetWindowRect(&formRect);
	const CPoint& topleft = formRect.TopLeft();

	CButton* rbok = ( CButton* )GetDlgItem( IDOK );
	CButton* rbcancel = ( CButton* )GetDlgItem( IDCANCEL );

	if (IsAdvanced)
	{
		CDialog::MoveWindow(topleft.x, topleft.y, (long)formRect.Width(), (long)310, TRUE); 
		m_AdvancedCtrl.MoveWindow(32, 250, 80, 20, TRUE);

		rbok->MoveWindow(145, 250, 80, 20, TRUE);
		rbcancel->MoveWindow(228, 250, 80, 20, TRUE);

	}
	else
	{
		CDialog::MoveWindow(topleft.x, topleft.y, (long)formRect.Width(), (long)150, TRUE);
		m_AdvancedCtrl.MoveWindow(32, 80, 80, 20, TRUE);

		rbok->MoveWindow(145, 80, 80, 20, TRUE);
		rbcancel->MoveWindow(228, 80, 80, 20, TRUE);
	}
	m_sIsAdvanced = IsAdvanced;
}

void CAddNewBlockGroupDialog::OnToggleAdvanced()
{
	SetAdvanced( m_AdvancedCtrl.GetCheck() );
}

void CAddNewBlockGroupDialog::OnBnClickedOk()
{
	CString tempString;
	m_NameCtrl.GetWindowText(tempString);
	m_Name = tempString;

	m_VarNameCtrl.GetWindowText(tempString);
	m_VarName = tempString;
	m_SigNameCtrl.GetWindowText(tempString);
	m_SigName = tempString;
	m_TreeNameCtrl.GetWindowText(tempString);
	m_TreeName = tempString;

	m_bVars = m_VarsCtrl.GetCheck();
	m_bSignals = m_SignalsCtrl.GetCheck();
	m_bTree = m_TreeCtrl.GetCheck();
	OnOK();
}