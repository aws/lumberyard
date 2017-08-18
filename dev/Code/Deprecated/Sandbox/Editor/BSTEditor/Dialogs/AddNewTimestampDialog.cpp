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
#include "AddNewTimestampDialog.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "DialogCommon.h"

IMPLEMENT_DYNAMIC(CAddNewTimestampDialog, CDialog)
CAddNewTimestampDialog::CAddNewTimestampDialog(CWnd* pParent /*=NULL*/)
: CDialog(CAddNewTimestampDialog::IDD, pParent)
{
}

void CAddNewTimestampDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDD_BST_TIMESTAMP_NAME_COMBO, m_cbTimeStamp);
	DDX_Control(pDX, IDC_BST_EVENTS_NAME_COMBO, m_cbEventName);
	DDX_Control(pDX, IDC_BST_EXCLUSIVETO_NAME_COMBO, m_cbExclusiveTo);
}

BEGIN_MESSAGE_MAP(CAddNewTimestampDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CAddNewTimestampDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDialogPositionHelper::SetToMouseCursor(*this);

	// Value
	//m_comboBoxVariableValue.AddString("True");
	//m_comboBoxVariableValue.AddString("False");
	//SetSelection( m_comboBoxVariableValue, m_variableValue ? "True" : "False" );

	//m_comboBoxVariableValue.SetCurSel( m_variableValue ? 1 : 0);
	/*
	// Vars
	std::list< string > varList;
	GetVariables( varList );
	for ( std::list< string >::iterator it = varList.begin(); it != varList.end(); ++it )
	{
		m_cbTimeStamp.AddString( *it );
	}
	SetSelection( m_cbTimeStamp, m_timestampName );


	// signals
	std::list< string > sigList;
	GetSignals( sigList );
	bool inList = false;
	for ( std::list< string >::iterator it = sigList.begin(); it != sigList.end(); ++it )
	{
		m_cbEventName.AddString( *it );
		if (m_eventName == *it) inList = true;
	}
	if (!inList && m_eventName.length() > 0)
		m_cbEventName.AddString( m_eventName );
	SetSelection( m_cbEventName, m_eventName );

	*/

	return TRUE;

}

void CAddNewTimestampDialog::OnBnClickedOk()
{
	CString tempString;
	m_cbTimeStamp.GetWindowText(tempString);
	m_timestampName = tempString;
	m_cbEventName.GetWindowText(tempString);
	m_eventName = tempString;
	m_cbExclusiveTo.GetWindowText(tempString);
	m_exclusiveToTimestampName = tempString;
	OnOK();
}


void CAddNewTimestampDialog::SetSelection( CComboBox& comboBox, const string& selection )
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

void CAddNewTimestampDialog::GetVariables( std::list< string >& strList )
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

void CAddNewTimestampDialog::LoadVarsFromXml( std::list< string >& strList, const XmlNodeRef& xmlNode )
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


void CAddNewTimestampDialog::GetSignals( std::list< string >& strList )
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
