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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWBLOCKGROUPDIALOG_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWBLOCKGROUPDIALOG_H
#pragma once



class CAddNewBlockGroupDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewBlockGroupDialog)

public:
	CAddNewBlockGroupDialog(CWnd* pParent = NULL);
	~CAddNewBlockGroupDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_BLOCKGROUPDIALOG };

	void Init( const string& title, const string& name, bool vars=true, bool issignals=true, bool tree=true, string varname="", string signame="", string treename="" )
	{
		m_title = title;
		m_Name = name;
		m_bVars = vars;
		m_bSignals = issignals;
		m_bTree = tree;
		m_VarName = varname;
		m_SigName = signame;
		m_TreeName = treename;
	};

	string GetName() { return m_Name; }
	string GetVarsName() { return m_VarName; }
	string GetSignalsName() { return m_SigName; }
	string GetTreeName() { return m_TreeName; }
	bool HasVars() {return m_bVars; }
	bool HasSignals() {return m_bSignals; }
	bool HasTree() {return m_bTree; }
	bool IsAdvanced() { return m_sIsAdvanced; }

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnToggleAdvanced();

private:
	void SetAdvanced(bool IsAdvanced);

private:
	CEdit		m_NameCtrl;

	CButton	m_VarsCtrl;
	CButton	m_SignalsCtrl;
	CButton	m_TreeCtrl;

	CEdit		m_VarNameCtrl;
	CEdit		m_SigNameCtrl;
	CEdit		m_TreeNameCtrl;

	CButton	m_AdvancedCtrl;

	string m_title;
	string m_Name;
	string m_VarName;
	string m_SigName;
	string m_TreeName;

	bool m_bVars;
	bool m_bSignals;
	bool m_bTree;

	static bool m_sIsAdvanced;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWBLOCKGROUPDIALOG_H
