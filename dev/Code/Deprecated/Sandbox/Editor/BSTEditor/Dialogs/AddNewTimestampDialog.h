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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWTIMESTAMPDIALOG_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWTIMESTAMPDIALOG_H
#pragma once



class CAddNewTimestampDialog: public CDialog
{
	DECLARE_DYNAMIC(CAddNewTimestampDialog)

public:
	CAddNewTimestampDialog(CWnd* pParent = NULL);
	~CAddNewTimestampDialog(){}

	// Dialog Data
	enum { IDD = IDD_BST_ADD_NEW_TIMESTAMP };

	void Init( const string& timestampName="", const string& eventName="", string exclusiveToTimestampName="" )
	{
		m_timestampName = timestampName;
		m_eventName = eventName;
		m_exclusiveToTimestampName = exclusiveToTimestampName;
	}

	string GetName() { return m_timestampName; }
	string GetEventName() { return m_eventName; }
	string GetExclusiveToTimestampName() {return m_exclusiveToTimestampName; }
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

	CComboBox				m_cbTimeStamp;
	CComboBox				m_cbEventName;
	CComboBox				m_cbExclusiveTo;
	string					m_timestampName;
	string					m_eventName;
	string					m_exclusiveToTimestampName;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_ADDNEWTIMESTAMPDIALOG_H
