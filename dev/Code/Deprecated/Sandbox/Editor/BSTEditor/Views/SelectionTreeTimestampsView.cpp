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
#include "SelectionTreeTimestampsView.h"

#include "StringDlg.h"
#include "CustomMessageBox.h"

#include "BSTEditor/Dialogs/SelectionTree_RefBrowser.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/SelectionTreeModifier.h"
#include "BSTEditor/Dialogs/AddNewTimestampDialog.h"

IMPLEMENT_DYNAMIC( CSelectionTreeTimestampsView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeTimestampsView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
	ON_NOTIFY( NM_DBLCLK , ID_TREE_CONTROL, OnTvnDblClick )
END_MESSAGE_MAP()

typedef enum
{
	eSTTMC_DoNothing = 0,

	eSTTMC_AddTimestamp,
	eSTTMC_RemoveTimestamp,

	eSTTMC_AddSignalVariable,
	eSTTMC_RemoveSignalVariable,

} SelectionTreeTimestampsMenuCommands;


CSelectionTreeTimestampsView::CSelectionTreeTimestampsView()
	: m_bLoaded( false )
{
}

CSelectionTreeTimestampsView::~CSelectionTreeTimestampsView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( this, eSTTI_TimeStamp, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetTimestampView( NULL );
}

BOOL CSelectionTreeTimestampsView::OnInitDialog()
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
	GetIEditor()->GetSelectionTreeManager()->SetTimestampView( this );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( this, eSTTI_TimeStamp );

	return TRUE;
}


void CSelectionTreeTimestampsView::OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult )
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

		if (type == eIT_TimeStamp && depth == 1)
		{
			//ToggleValue( clickedItemHandle );
		}
	}
}

void CSelectionTreeTimestampsView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
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
		case eIT_Signal:
			selectionType = depth == 0 ? eSIT_Signal : eSIT_InReference;
			break;
		case eIT_TimeStamp:
			selectionType = depth == 1 ? eSIT_SignalVariable : eSIT_InReference;
			break;
		}
	}

	switch(selectionType)
	{
	case eSIT_NothingSelected:
		menu.AppendMenu( MF_STRING, eSTTMC_AddTimestamp, "Add New Timestamp" );
		break;
	case eSIT_Signal:
		menu.AppendMenu( MF_STRING, eSTTMC_AddSignalVariable, "Add new Timestamp" );
		menu.AppendMenu( MF_STRING, eSTTMC_RemoveTimestamp, "Remove this Timestamp" );	
		break;
	case eSIT_SignalVariable:
		menu.AppendMenu( MF_STRING, eSTTMC_AddSignalVariable, "Add timestamp Variable" );
		menu.AppendMenu( MF_STRING, eSTTMC_RemoveTimestamp, "Remove this Timestamp" );	
		break;
	case eSIT_InReference:
		clickedItemHandle = rootItem;
	}

	const int commandId = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetSafeHwnd(), NULL );

	if ( commandId == eSTTMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTTMC_AddTimestamp )
	{
		AddSignal( clickedItemHandle );
		return;
	}

	if ( commandId == eSTTMC_RemoveTimestamp )
	{
		RemoveSignalVars( clickedItemHandle );
		return;
	}

	if ( commandId == eSTTMC_AddSignalVariable )
	{
		AddSigVar( clickedItemHandle );
		return;
	}

	if ( commandId == eSTTMC_RemoveSignalVariable )
	{
		DeleteSigVar( clickedItemHandle );
		return;
	}
}

bool CSelectionTreeTimestampsView::LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex )
{
	bool res = false;
	if ( typeId == eSTTI_TimeStamp)
	{
		m_bLoaded = true;
		pUndoEventHandler = this;
		res = LoadFromXml( xmlNode );
	}
	return res;
}

void CSelectionTreeTimestampsView::UnloadXml( int typeId )
{
	if ( typeId == eSTTI_TimeStamp || typeId == eSTTI_All )
	{
		m_bLoaded = false;
		ClearList();
	}
}

void CSelectionTreeTimestampsView::ClearList()
{
	m_TreeInfo.Clear();
	m_signals.DeleteAllItems();
}

HTREEITEM CSelectionTreeTimestampsView::GetParentItem(HTREEITEM hItem)
{
	HTREEITEM parent = m_signals.GetParentItem(hItem);
	return parent ? parent : TVI_ROOT;
}

bool CSelectionTreeTimestampsView::SaveToXml( XmlNodeRef& xmlNode )
{
	xmlNode->setTag("Timestamps");
	const TChildItemList& rootChilds = GetChildList(TVI_ROOT);
	for ( TChildItemList::const_iterator it = rootChilds.begin(); it != rootChilds.end(); ++it )
	{
		const HTREEITEM& child = it->Item;
		assert(it->Type != eIT_TimeStamp);
	  if (it->Type == eIT_Signal)
		{
			const TChildItemList& signalChilds = GetChildList(child);
			for ( TChildItemList::const_iterator it = signalChilds.begin(); it != signalChilds.end(); ++it )
			{
				const HTREEITEM& variable = it->Item;
				assert(it->Type == eIT_TimeStamp);

				const SVarInfo& varinfo =GetItemInfo(m_TreeInfo.VarInfo, variable);
				XmlNodeRef newNode = gEnv->pSystem->CreateXmlNode( "Timestamp" );
				newNode->setAttr( "name", varinfo.stampName );
				newNode->setAttr( "setOnEvent", varinfo.eventName );
				newNode->setAttr( "exclusiveTo", varinfo.exclusiveToStamp );
				xmlNode->addChild( newNode );
			}	
		}
	}
	return true;
}

bool CSelectionTreeTimestampsView::LoadFromXml( const XmlNodeRef& xmlNode )
{
	ClearList();
	return LoadTimestampNodes( xmlNode, TVI_ROOT );
}

bool CSelectionTreeTimestampsView::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	return LoadFromXml( xmlNode );
}

bool CSelectionTreeTimestampsView::LoadTimestampNodes( const XmlNodeRef& xmlNode, HTREEITEM hItem )
{
	// Timestamps
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name && strcmpi( tag, "Timestamp" ) == 0 )
		{
			const char* eventName = xmlChild->getAttr( "setOnEvent" );
			const char* exclusiveTo = xmlChild->getAttr( "exclusiveTo" );
			
			if ( !CreateSignalItem( name, eventName, exclusiveTo, hItem, TVI_SORT ) )
				return false;
		}
	}

	return true;
}

bool CSelectionTreeTimestampsView::CreateSignalItem( const char* stampName, const char* eventName, const char* exclusiveTo, HTREEITEM hItem, HTREEITEM hInsertAfter )
{
	HTREEITEM hSignal = NULL;

	TSignalInfoMap::iterator it = m_TreeInfo.SignalInfo.begin();
	for (;it != m_TreeInfo.SignalInfo.end(); ++it)
	{
		if (it->second.Name == stampName && GetParentItem(it->first) == hItem) break;
	}

	if ( it != m_TreeInfo.SignalInfo.end() )
	{
		hSignal = it->first;
	}
	else
	{
		hSignal = InsertItem( hItem, hInsertAfter, stampName, m_TreeInfo.SignalInfo, SSignalInfo(stampName) );
	}

	string namestr;
	namestr.Format("%s [%s]", eventName, exclusiveTo );
	InsertItem( hSignal, hInsertAfter, namestr.c_str(), m_TreeInfo.VarInfo, SVarInfo(stampName, eventName, exclusiveTo) );

	return true;
}

void CSelectionTreeTimestampsView::DeleteSignalItem( HTREEITEM hItem )
{
	HTREEITEM hSignal = GetParentItem( hItem );

	DeleteItem( hItem, m_TreeInfo.VarInfo );

	// if the signal var was last in signal group remove also the signal group
	if ( GetChildList(hSignal).size() == 0 )
	{
		DeleteItem( hSignal, m_TreeInfo.SignalInfo );
	}
}

void CSelectionTreeTimestampsView::DeleteSignalItems( HTREEITEM hItem )
{
	SSignalInfo info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);
	TChildItemList childs = GetChildList(hItem);
	for ( TChildItemList::const_iterator it = childs.begin(); it != childs.end(); ++it )
	{
		assert(it->Type == eIT_TimeStamp);
		DeleteSignalItem( it->Item );
	}
}

void CSelectionTreeTimestampsView::AddSignal( HTREEITEM hItem )
{
	CAddNewTimestampDialog newDlg;
	newDlg.Init();
	if(newDlg.DoModal() == IDOK)
	{
		string stampName = newDlg.GetName();
		string eventName = newDlg.GetEventName();
		string exclusiveTo = newDlg.GetExclusiveToTimestampName();
		if ( stampName.length() > 0 && eventName.length() > 0 )
		{
			CreateSignalItem( stampName, eventName, exclusiveTo, TVI_ROOT, TVI_LAST );
			string desc;
			desc.Format( "Added Timestamp Variable \"%s\" to Timestamp \"%s\"", eventName, stampName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

void CSelectionTreeTimestampsView::RemoveSignalVars( HTREEITEM hItem )
{
	SSignalInfo info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);

	DeleteSignalItems( hItem );

	string desc;
	desc.Format( "All Signal Variables deleted for \"%s\"", info.Name );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}

void CSelectionTreeTimestampsView::AddSigVar( HTREEITEM hItem )
{
	const SSignalInfo& info = GetItemInfo(m_TreeInfo.SignalInfo, hItem);
	
	CAddNewTimestampDialog newDlg;
	newDlg.Init( info.Name );
	if(newDlg.DoModal() == IDOK)
	{
		string stampName = newDlg.GetName();
		string eventName = newDlg.GetEventName();
		string exclusiveTo = newDlg.GetExclusiveToTimestampName();
		if ( stampName.length() > 0 && eventName.length() > 0 )
		{
			CreateSignalItem( stampName, eventName, exclusiveTo, TVI_ROOT, TVI_LAST );
			string desc;
			desc.Format( "Added Timestamp Variable \"%s\" to Timestamp \"%s\"", eventName, stampName );
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
		}
	}
}

void CSelectionTreeTimestampsView::DeleteSigVar( HTREEITEM hItem )
{
	SVarInfo info = GetItemInfo(m_TreeInfo.VarInfo, hItem);

	DeleteSignalItem( hItem );

	string desc;
	desc.Format( "Variable \"%s\" deleted from Signal \"%s\" ", info.eventName, info.stampName);
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc.c_str() );
}
