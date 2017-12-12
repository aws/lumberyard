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
#include "SelectionTreeSignalsView.h"

#include "StringDlg.h"
#include "CustomMessageBox.h"

#include "BSTEditor/Dialogs/SelectionTree_RefBrowser.h"
#include "BSTEditor/Dialogs/AddNewSignalDialog.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/SelectionTreeModifier.h"

IMPLEMENT_DYNAMIC( CSelectionTreeSignalsView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeSignalsView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
	ON_NOTIFY( NM_DBLCLK , ID_TREE_CONTROL, OnTvnDblClick )
END_MESSAGE_MAP()

typedef enum
{
	eSTSMC_DoNothing = 0,

	eSTSMC_AddSignal,
	eSTSMC_RemoveSignal,

	eSTSMC_AddSignalVariable,
	eSTSMC_RemoveSignalVariable,
	eSTSMC_ToggleValue,

} SelectionTreeSignalsMenuCommands;


CSelectionTreeSignalsView::CSelectionTreeSignalsView()
	: m_bLoaded( false )
{
}

CSelectionTreeSignalsView::~CSelectionTreeSignalsView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( this, eSTTI_Signals, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetSigView( NULL );
}

BOOL CSelectionTreeSignalsView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect( rc );

	m_signals.Create( WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_signals.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetSigView( this );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( this, eSTTI_Signals );

	return TRUE;
}


void CSelectionTreeSignalsView::OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !m_bLoaded ) return;

	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_signals.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_signals.HitTest( clientPoint );
	HTREEITEM rootItem = clickedItemHandle;

	if ( clickedItemHandle != 0 )
	{
		m_signals.SelectItem( clickedItemHandle );

		EItemType type = GetItemType(clickedItemHandle);

		int depth = 0;
		for (;GetParentItem(rootItem) != TVI_ROOT; rootItem = GetParentItem(rootItem), depth++);

		if (type == eIT_SignalVariable && depth == 1)
		{
			ToggleValue( clickedItemHandle );
		}
	}
}

void CSelectionTreeSignalsView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	if ( !m_bLoaded ) return;

	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_signals.ScreenToClient( &clientPoint );

	HTREEITEM clickedItemHandle = m_signals.HitTest( clientPoint );

	enum ESelectedItemType
	{
		eSIT_NothingSelected = 0,
		eSIT_Signal,
		eSIT_SignalVariable,
		eSIT_Reference,
		eSIT_InReference
	};

	ESelectedItemType selectionType = eSIT_NothingSelected;
	bool bVal = false;
	bool isInRef = false; 
	HTREEITEM rootItem = clickedItemHandle;

	CMenu menu;
	menu.CreatePopupMenu();

	if ( clickedItemHandle != 0 )
	{
		m_signals.SelectItem( clickedItemHandle );

		EItemType type = GetItemType(clickedItemHandle);

		int depth = 0;
		for (;GetParentItem(rootItem) != TVI_ROOT; rootItem = GetParentItem(rootItem), depth++);

		switch (type)
		{
		case eIT_Ref:
			selectionType = depth == 0 ? eSIT_Reference : eSIT_InReference;
			break;
		case eIT_Signal:
			selectionType = depth == 0 ? eSIT_Signal : eSIT_InReference;
			break;
		case eIT_SignalVariable:
			selectionType = depth == 1 ? eSIT_SignalVariable : eSIT_InReference;
			{
				const SVarInfo& info = GetItemInfo(m_TreeInfo.VarInfo, clickedItemHandle);
				bVal = info.Value;
			}
			break;
		}
	}

	switch(selectionType)
	{
	case eSIT_NothingSelected:
		menu.AppendMenu( MF_STRING, eSTSMC_AddSignal, "Add New Signal" );
		break;
	case eSIT_Signal:
		menu.AppendMenu( MF_STRING, eSTSMC_AddSignalVariable, "Add Signal Variable" );
		menu.AppendMenu( MF_STRING, eSTSMC_RemoveSignal, "Remove All Signal Variables" );	
		break;
	case eSIT_SignalVariable:
		menu.AppendMenu( MF_STRING, eSTSMC_RemoveSignalVariable, "Remove Signal Variable" );
		menu.AppendMenu( MF_STRING, eSTSMC_ToggleValue, string().Format("Set Value to %s", bVal ? "false" : "true" ).c_str() );
		break;
	case eSIT_InReference:
		clickedItemHandle = rootItem;
		break;
	}

	const int commandId = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetSafeHwnd(), NULL );

	if ( commandId == eSTSMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTSMC_AddSignal )
	{
		AddSignal( clickedItemHandle );
		return;
	}

	if ( commandId == eSTSMC_RemoveSignal )
	{
		RemoveSignalVars( clickedItemHandle );
		return;
	}

	if ( commandId == eSTSMC_AddSignalVariable )
	{
		AddSigVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTSMC_RemoveSignalVariable )
	{
		DeleteSigVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTSMC_ToggleValue )
	{
		ToggleValue( clickedItemHandle );
		return;
	}
}

bool CSelectionTreeSignalsView::LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex )
{
	bool res = false;
	if ( typeId == eSTTI_Signals )
	{
		m_bLoaded = true;
		pUndoEventHandler = this;
		res = LoadFromXml( xmlNode );
	}
	return res;
}

void CSelectionTreeSignalsView::UnloadXml( int typeId )
{
	if ( typeId == eSTTI_Signals || typeId == eSTTI_All )
	{
		m_bLoaded = false;
		ClearList();
	}
}

void CSelectionTreeSignalsView::ClearList()
{
	m_TreeInfo.Clear();
	m_signals.DeleteAllItems();
}

HTREEITEM CSelectionTreeSignalsView::GetParentItem(HTREEITEM hItem)
{
	HTREEITEM parent = m_signals.GetParentItem(hItem);
	return parent ? parent : TVI_ROOT;
}

bool CSelectionTreeSignalsView::SaveToXml( XmlNodeRef& xmlNode )
{
	xmlNode->setTag("SignalVariables");
	const TChildItemList& rootChilds = GetChildList(TVI_ROOT);
	for ( TChildItemList::const_iterator it = rootChilds.begin(); it != rootChilds.end(); ++it )
	{
		const HTREEITEM& child = it->Item;
		assert(it->Type != eIT_SignalVariable);
		if (it->Type == eIT_Ref)
		{
			const SRefInfo& info = GetItemInfo(m_TreeInfo.RefInfo, child);

			XmlNodeRef newNode = gEnv->pSystem->CreateXmlNode( "Ref" );
			newNode->setAttr( "name", info.Name );
			xmlNode->addChild( newNode );
		}
		else if (it->Type == eIT_Signal)
		{
			const TChildItemList& signalChilds = GetChildList(child);
			for ( TChildItemList::const_iterator it = signalChilds.begin(); it != signalChilds.end(); ++it )
			{
				const HTREEITEM& variable = it->Item;
				assert(it->Type == eIT_SignalVariable);

				const SVarInfo& varinfo =GetItemInfo(m_TreeInfo.VarInfo, variable);
				XmlNodeRef newNode = gEnv->pSystem->CreateXmlNode( "Signal" );
				newNode->setAttr( "name", varinfo.Signal );
				newNode->setAttr( "variable", varinfo.Var );
				newNode->setAttr( "value", varinfo.Value ? "true" : "false" );
				xmlNode->addChild( newNode );
			}	
		}
	}
	return true;
}

bool CSelectionTreeSignalsView::LoadFromXml( const XmlNodeRef& xmlNode )
{
	ClearList();
	return LoadSignalNodes( xmlNode, TVI_ROOT );
}

bool CSelectionTreeSignalsView::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	return LoadFromXml( xmlNode );
}

bool CSelectionTreeSignalsView::LoadSignalNodes( const XmlNodeRef& xmlNode, HTREEITEM hItem )
{
	// Signals
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name && strcmpi( tag, "Signal" ) == 0 )
		{
			const char* variable = xmlChild->getAttr( "variable" );
			const char* value = xmlChild->getAttr( "value" );
			bool bValue = strcmpi( value, "true" ) == 0;

			if ( !CreateSignalItem( name, variable, bValue, hItem, TVI_SORT ) )
				return false;
		}
	}

	return true;
}

bool CSelectionTreeSignalsView::CreateSignalItem( const char* name, const char* varname, bool val, HTREEITEM hItem, HTREEITEM hInsertAfter )
{
	HTREEITEM hSignal = NULL;

	TSignalInfoMap::iterator it = m_TreeInfo.SignalInfo.begin();
	for (;it != m_TreeInfo.SignalInfo.end(); ++it)
	{
		if (it->second.Name == name && GetParentItem(it->first) == hItem) break;
	}

	if ( it != m_TreeInfo.SignalInfo.end() )
	{
		hSignal = it->first;
	}
	else
	{
		hSignal = InsertItem( hItem, hInsertAfter, name, m_TreeInfo.SignalInfo, SSignalInfo(name) );
	}

	string namestr;
	namestr.Format("%s [%s]", varname, val ? "true" : "false" );
	InsertItem( hSignal, hInsertAfter, namestr.c_str(), m_TreeInfo.VarInfo, SVarInfo(name, varname, val) );

	return true;
}

void CSelectionTreeSignalsView::DeleteSignalItem( HTREEITEM hItem )
{
	HTREEITEM hSignal = GetParentItem( hItem );

	DeleteItem( hItem, m_TreeInfo.VarInfo );

	// if the signal var was last in signal group remove also the signal group
	if ( GetChildList(hSignal).size() == 0 )
	{
		DeleteItem( hSignal, m_TreeInfo.SignalInfo );
	}
}

void CSelectionTreeSignalsView::DeleteSignalItems( HTREEITEM hItem )
{
	SSignalInfo info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);
	TChildItemList childs = GetChildList(hItem);
	for ( TChildItemList::const_iterator it = childs.begin(); it != childs.end(); ++it )
	{
		assert(it->Type == eIT_SignalVariable);
		DeleteSignalItem( it->Item );
	}
}

void CSelectionTreeSignalsView::AddSignal( HTREEITEM hItem )
{
	CAddNewSignalDialog newDlg;
	newDlg.Init();
	if(newDlg.DoModal() == IDOK)
	{
		string signalName = newDlg.GetSignalName();
		string variableName = newDlg.GetVariableName();
		bool bVariableValue = newDlg.GetVariableValue();
		if ( variableName.size() > 0 && signalName.size() > 0)
		{
			CreateSignalItem( signalName, variableName, bVariableValue, TVI_ROOT, TVI_LAST );
			string desc;
			desc.Format( "Added Signal Variable \"%s\" to Signal \"%s\"", variableName, signalName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

void CSelectionTreeSignalsView::RemoveSignalVars( HTREEITEM hItem )
{
	SSignalInfo info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);

	DeleteSignalItems( hItem );

	string desc;
	desc.Format( "All Signal Variables deleted for \"%s\"", info.Name );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeSignalsView::AddSigVar( HTREEITEM hItem )
{
	const SSignalInfo& info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);
	
	CAddNewSignalDialog newDlg;
	newDlg.Init( info.Name );
	if(newDlg.DoModal() == IDOK)
	{
		string signalName = newDlg.GetSignalName();
		string variableName = newDlg.GetVariableName();
		bool bVariableValue = newDlg.GetVariableValue();
		if ( variableName.length() > 0 && signalName.length() > 0 )
		{
			CreateSignalItem( signalName, variableName, bVariableValue, TVI_ROOT, TVI_LAST );
			string desc;
			desc.Format( "Added Signal Variable \"%s\" to Signal \"%s\"", variableName, signalName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

void CSelectionTreeSignalsView::DeleteSigVar( HTREEITEM hItem )
{
	SVarInfo info = GetItemInfo(m_TreeInfo.VarInfo, hItem);

	DeleteSignalItem( hItem );

	string desc;
	desc.Format( "Variable \"%s\" deleted from Signal \"%s\" ", info.Var, info.Signal );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeSignalsView::ToggleValue( HTREEITEM hItem )
{
	SVarInfo& info = GetItemInfo(m_TreeInfo.VarInfo, hItem);
	info.Value = !info.Value;

	string name;
	name.Format("%s [%s]", info.Var, info.Value ? "true" : "false" );
	m_signals.SetItemText( hItem, name.c_str() );

	string desc;
	desc.Format( "Signal Var \"%s -> %s\" toggled to \"%s\" ", info.Signal, info.Var, info.Value ? "true" : "false" );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}
