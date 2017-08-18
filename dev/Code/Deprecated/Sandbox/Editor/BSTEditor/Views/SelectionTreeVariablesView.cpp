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
#include "SelectionTreeVariablesView.h"
#include "BSTEditor/Dialogs/SelectionTree_RefBrowser.h"
#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/SelectionTreeModifier.h"

#include "BSTEditor/Dialogs/DialogCommon.h"
#include "CustomMessageBox.h"

IMPLEMENT_DYNAMIC( CSelectionTreeVariablesView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeVariablesView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
	ON_NOTIFY( NM_DBLCLK , ID_TREE_CONTROL, OnTvnDblClick )
END_MESSAGE_MAP()

typedef enum
{
	eSTVMC_DoNothing = 0,

	eSTVMC_AddVariable,
	eSTVMC_RenameVariable,
	eSTVMC_DeleteVariable,

} SelectionTreeVariablesMenuCommands;


CSelectionTreeVariablesView::CSelectionTreeVariablesView()
	: m_bLoaded( false )
	, m_bDebugging( false )
{
}

CSelectionTreeVariablesView::~CSelectionTreeVariablesView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( this, eSTTI_Variables, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetVarView( NULL );
}

BOOL CSelectionTreeVariablesView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect( rc );

	m_variables.Create( WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_variables.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetVarView( this );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( this, eSTTI_Variables );

	return TRUE;
}


void CSelectionTreeVariablesView::OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !m_bLoaded || m_bDebugging) return;

	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_variables.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_variables.HitTest( clientPoint );

	if ( clickedItemHandle != 0 && m_variables.GetParentItem( clickedItemHandle ) == 0 )
	{
		m_variables.SelectItem( clickedItemHandle );
		if ( !m_ItemInfoMap[ clickedItemHandle ].IsRef )
		{
			RenameVar( clickedItemHandle );
		}
	}
}


void CSelectionTreeVariablesView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !m_bLoaded || m_bDebugging) return;

	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_variables.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_variables.HitTest( clientPoint );


	CMenu menu;
	menu.CreatePopupMenu();

	if ( clickedItemHandle != 0 && m_variables.GetParentItem( clickedItemHandle ) == 0 )
	{
		m_variables.SelectItem( clickedItemHandle );
		if ( m_ItemInfoMap[ clickedItemHandle ].IsRef )
		{
			//menu.AppendMenu( MF_STRING, eSTVMC_RemoveReference, "Remove Reference" );
		}
		else
		{
			menu.AppendMenu( MF_STRING, eSTVMC_RenameVariable, "Rename Variable" );
			menu.AppendMenu( MF_STRING, eSTVMC_DeleteVariable, "Delete Variable" );
		}
		menu.AppendMenu( MF_SEPARATOR );
		menu.AppendMenu( MF_STRING, eSTVMC_AddVariable, "Add Variable" );
	}

	if ( clickedItemHandle == 0 )
	{
		menu.AppendMenu( MF_STRING, eSTVMC_AddVariable, "Add Variable" );
	}

	const int commandId = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetSafeHwnd(), NULL );

	if ( commandId == eSTVMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTVMC_AddVariable )
	{
		AddVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_RenameVariable )
	{
		RenameVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTVMC_DeleteVariable )
	{
		DeleteVar( clickedItemHandle );
		return;
	}
}

bool CSelectionTreeVariablesView::LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex )
{
	bool res = false;
	if ( typeId == eSTTI_Variables )
	{
		m_bLoaded = true;
		pUndoEventHandler = this;
		res = LoadFromXml( xmlNode );
	}
	return res;
}

void CSelectionTreeVariablesView::UnloadXml( int typeId )
{
	if ( typeId == eSTTI_Variables || typeId == eSTTI_All )
	{
		m_bLoaded = false;
		ClearList();
	}
}


void CSelectionTreeVariablesView::ClearList()
{
	m_variables.DeleteAllItems();
	m_VarMap.clear();
	m_ItemInfoMap.clear();
}

bool CSelectionTreeVariablesView::SaveToXml( XmlNodeRef& xmlNode )
{
	xmlNode->setTag("Variables");
	const TItemList& items = m_VarMap[ TVI_ROOT ];
	for ( TItemList::const_iterator it = items.begin(); it != items.end(); ++it )
	{
		XmlNodeRef newNode = gEnv->pSystem->CreateXmlNode( m_ItemInfoMap[ *it ].IsRef ? "Ref" : "Variable" );
		newNode->setAttr( "name", m_ItemInfoMap[ *it ].Name );
		xmlNode->addChild( newNode );
	}
	return true;
}

bool CSelectionTreeVariablesView::LoadFromXml( const XmlNodeRef& xmlNode )
{
	ClearList();
	return LoadVarsNode( xmlNode, TVI_ROOT );
}

bool CSelectionTreeVariablesView::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	return LoadFromXml( xmlNode );
}

bool CSelectionTreeVariablesView::LoadVarsNode( const XmlNodeRef& xmlNode, HTREEITEM hItem )
{
	// Variables
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name && strcmpi( tag, "Variable" ) == 0 )
		{
			if ( !CreateVarItem( name, hItem, TVI_SORT ) )
				return false;
		}
	}
	return true;
}

bool CSelectionTreeVariablesView::CreateVarItem( const char* name, HTREEITEM hItem, HTREEITEM hInsertAfter )
{
// 	if ( IsVarValid( name ) )
// 	{
		HTREEITEM item = m_variables.InsertItem( name, 0, 0, hItem, hInsertAfter);
		m_ItemInfoMap[ item ] = SItemInfo( name, false );
		m_VarMap[ hItem ].push_back( item );
		return true;
// 	}
// 	return false;
}

void CSelectionTreeVariablesView::DeleteVarItem( HTREEITEM hItem )
{
	stl::member_find_and_erase( m_ItemInfoMap, hItem );

	HTREEITEM hParent = m_variables.GetParentItem( hItem );
	if ( !hParent )
		hParent = TVI_ROOT;

	m_VarMap[ hParent ].remove( hItem );

	m_variables.DeleteItem( hItem );
}

void CSelectionTreeVariablesView::AddVar( HTREEITEM hItem )
{
	CBSTStringDlg dlg ( _T("Variable name") );
	dlg.SetString( "NewVar" );
	if ( dlg.DoModal() == IDOK )
	{
		string newName = dlg.GetString().GetString();

		CMakeNameUnique<CSelectionTreeVariablesView> nameMaker( this, &CSelectionTreeVariablesView::IsVarValid );
		nameMaker.MakeNameUnique( newName );

		if ( CreateVarItem( newName, TVI_ROOT, TVI_LAST ) )
		{
			string desc;
			desc.Format( "New Variable created \"%s\"", newName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
			return;
		}
		assert( false );
	}	
}

void CSelectionTreeVariablesView::RenameVar( HTREEITEM hItem )
{
	string oldName = m_ItemInfoMap[ hItem ].Name;
	CBSTStringDlg dlg ( _T("Variable name") );
	dlg.SetString( oldName.c_str() );
	if ( dlg.DoModal() == IDOK )
	{
		string newName = dlg.GetString().GetString();
		if ( oldName != newName )
		{
			CMakeNameUnique<CSelectionTreeVariablesView> nameMaker( this, &CSelectionTreeVariablesView::IsVarValid );
			nameMaker.MakeNameUnique( newName );

			m_ItemInfoMap[ hItem ].Name = newName;
			m_variables.SetItemText( hItem, newName.c_str() );

			string desc;
			desc.Format( "Variable renamed (from \"%s\" to \"%s\")", oldName, newName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

bool CSelectionTreeVariablesView::IsVarValid( const string& name )
{
	for ( TItemInfoMap::iterator it = m_ItemInfoMap.begin(); it != m_ItemInfoMap.end(); ++it )
	{
		if ( !it->second.IsRef )
		{
			if ( it->second.Name == name )
				return false;
		}
	}
	return true;
}

bool CSelectionTreeVariablesView::IsRefValid( const string& name )
{
	for ( TItemInfoMap::iterator it = m_ItemInfoMap.begin(); it != m_ItemInfoMap.end(); ++it )
	{
		if ( it->second.IsRef )
		{
			if ( it->second.Name == name )
				return false;
		}
	}
	return true;
}

void CSelectionTreeVariablesView::SetDebugging(bool debug)
{
	m_bDebugging = debug;
	if (debug)
	{
		for (TItemInfoMap::iterator it = m_ItemInfoMap.begin(); it != m_ItemInfoMap.end(); ++it)
		{
			m_variables.Expand(it->first, TVE_EXPAND);
		}
	}
	else
	{
		for (TItemInfoMap::iterator it = m_ItemInfoMap.begin(); it != m_ItemInfoMap.end(); ++it)
		{
			it->second.DebugVal = -1;
			if (!it->second.IsRef)
			{
				m_variables.SetItemText(it->first, it->second.Name.c_str());
				m_variables.SetItemState( it->first, 0, TVIS_SELECTED | TVIS_BOLD );
			}
		}
	}
}

void CSelectionTreeVariablesView::DeleteVar( HTREEITEM hItem )
{
	string oldName = m_ItemInfoMap[ hItem ].Name;

	DeleteVarItem( hItem );

	string desc;
	desc.Format( "Variable \"%s\" deleted ", oldName );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeVariablesView::SetVarVal(const char* varName, bool val)
{
	assert( m_bDebugging && m_bLoaded);

	bool ok = false;
	for (TItemInfoMap::iterator it = m_ItemInfoMap.begin(); it != m_ItemInfoMap.end(); ++it)
	{
		if (!it->second.IsRef && it->second.Name == varName)
		{
			const bool prevVal = it->second.DebugVal == 1;
			const bool isNew = it->second.DebugVal == -1;
			m_variables.SetItemText(it->first, string().Format("%s [%s]", it->second.Name.c_str(), val ? "TRUE" : "FALSE").c_str());
			if (prevVal != val && !isNew)
				m_variables.SetItemState( it->first, TVIS_SELECTED | TVIS_BOLD, TVIS_SELECTED | TVIS_BOLD );
			else
				m_variables.SetItemState( it->first, 0, TVIS_SELECTED | TVIS_BOLD );
			it->second.DebugVal = val ? 1 : 0;
			ok = true;
			break;
		}
	}
	assert( ok );
}
