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
#include "AddNewSignalDialog.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "DialogCommon.h"

IMPLEMENT_DYNAMIC(CAddNewSignalDialog, CDialog)
CAddNewSignalDialog::CAddNewSignalDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewSignalDialog::IDD, pParent)
{
}

void CAddNewSignalDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BST_VARIABLES_NAME_COMBO, m_comboBoxVariableName);
	DDX_Control(pDX, IDC_BST_VARIABLE_VALUE_COMBO, m_comboBoxVariableValue);
	DDX_Control(pDX, IDD_BST_ADD_NEW_SIGNAL, m_comboBoxSignals);
}

BEGIN_MESSAGE_MAP(CAddNewSignalDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddNewSignalDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this);

	// Value
	m_comboBoxVariableValue.AddString("True");
	m_comboBoxVariableValue.AddString("False");
	SetSelection( m_comboBoxVariableValue, m_bVariableValue ? "True" : "False" );

	m_comboBoxVariableValue.SetCurSel( m_bVariableValue ? 1 : 0);

	// Vars
	std::list< string > varList;
	GetVariables( varList );
	for ( std::list< string >::iterator it = varList.begin(); it != varList.end(); ++it )
	{
		m_comboBoxVariableName.AddString( *it );
	}
	SetSelection( m_comboBoxVariableName, m_variableName );


	// signals
	std::list< string > sigList;
	GetSignals( sigList );
	bool inList = false;
	for ( std::list< string >::iterator it = sigList.begin(); it != sigList.end(); ++it )
	{
		m_comboBoxSignals.AddString( *it );
		if (m_signalName == *it) inList = true;
	}
	if (!inList && m_signalName.length() > 0)
		m_comboBoxSignals.AddString( m_signalName );
	SetSelection( m_comboBoxSignals, m_signalName );

	return TRUE;

}

void CAddNewSignalDialog::OnBnClickedOk()
{
	CString tempString;
	m_comboBoxSignals.GetWindowText(tempString);
	m_signalName = tempString;
	m_comboBoxVariableValue.GetWindowText(tempString);
	m_bVariableValue = (tempString == "True") ? true : false;
	m_comboBoxVariableName.GetWindowText(tempString);
	m_variableName = tempString;
	OnOK();
}


void CAddNewSignalDialog::SetSelection( CComboBox& comboBox, const string& selection )
{
	for ( int i = 0; i < comboBox.GetCount(); ++i )
	{
		CString str;
		comboBox.GetLBText( i, str );
		if ( string(str.GetString()) == selection )
		{
			comboBox.SetCurSel( i );
			break;
		}
	}
}

void CAddNewSignalDialog::GetVariables( std::list< string >& strList )
{
	SSelectionTreeInfo currTreeInfo;
	if ( GetIEditor()->GetSelectionTreeManager()->GetCurrentTreeInfo(currTreeInfo) )
	{
		SSelectionTreeBlockInfo varInfo;
		bool ok = currTreeInfo.GetBlockById(varInfo, eSTTI_Variables );
		if ( ok )
		{
			LoadVarsFromXml( strList, varInfo.XmlData );
		}
	}
}

void CAddNewSignalDialog::LoadVarsFromXml( std::list< string >& strList, const XmlNodeRef& xmlNode )
{
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		const XmlNodeRef& child = xmlNode->getChild( i );
		const char* tag = child->getTag();
		if ( tag )
		{
			if ( strcmpi( tag, "Variable" ) == 0)
			{
				strList.push_back( child->getAttr( "name" ) );
			}
			else if ( strcmpi( tag, "Ref" ) == 0)
			{
				SSelectionTreeBlockInfo refInfo;
				bool found = false;//GetIEditor()->GetSelectionTreeManager()->GetRefInfoByName( eSTTI_Variables, child->getAttr( "name" ), refInfo );

				assert( found );
				if ( found )
				{
					LoadVarsFromXml( strList, refInfo.XmlData );
				}
			}
		}
	}
}


void CAddNewSignalDialog::GetSignals( std::list< string >& strList )
{
	XmlNodeRef signalsNode = gEnv->pSystem->LoadXmlFromFile("Game/Scripts/AI/SystemSignals.xml");
	if (signalsNode)
	{
		XmlNodeRef systemsignals = signalsNode->findChild("SystemSignals");
		for ( int i = 0; i < systemsignals->getChildCount(); ++i )
		{
			XmlNodeRef syssig = systemsignals->getChild( i );
			if ( strcmpi(syssig->getTag(), "Signal") == 0 )
			{
				const char* name = syssig->getAttr("name");
				if ( name )
				{
					strList.push_back( name );
				}
			}
		}
	}
}
