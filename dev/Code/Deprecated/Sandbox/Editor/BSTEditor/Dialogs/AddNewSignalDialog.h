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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWSIGNALDIALOG_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWSIGNALDIALOG_H
#pragma once



class CAddNewSignalDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewSignalDialog)

public:
	CAddNewSignalDialog(CWnd* pParent = NULL);
	~CAddNewSignalDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_NEW_SIGNAL };

	void Init( const string& signalName="", const string& variableName="", bool bVariableValue=true )
	{
		m_signalName = signalName;
		m_variableName = variableName;
		m_bVariableValue = bVariableValue;
	}

	string GetSignalName() { return m_signalName; }
	string GetVariableName() { return m_variableName; }
	bool GetVariableValue() {return m_bVariableValue; }
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();

private:
	void SetSelection( CComboBox& comboBox, const string& selection );
	void GetVariables( std::list< string >& strList );
	void GetSignals( std::list< string >& strList );
	void LoadVarsFromXml( std::list< string >& strList, const XmlNodeRef& xmlNode );

private:

	CComboBox				m_comboBoxVariableName;
	CComboBox				m_comboBoxVariableValue;
	CComboBox				m_comboBoxSignals;
	string					m_variableName;
	bool					m_bVariableValue;
	string					m_signalName;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWSIGNALDIALOG_H
