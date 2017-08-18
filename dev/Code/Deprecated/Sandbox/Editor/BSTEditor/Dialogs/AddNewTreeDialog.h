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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWTREEDIALOG_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWTREEDIALOG_H
#pragma once



class CAddNewTreeDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewTreeDialog)

public:
	CAddNewTreeDialog(CWnd* pParent = NULL);
	~CAddNewTreeDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_NEW_TREE };

	void Init( const string& title, const string& name) { m_title = title; m_name = name; };

	string GetName() { return m_name; }
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();

private:

	CEdit						m_editName;
	string					m_name;
	string					m_title;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWTREEDIALOG_H
