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
#include "SelectionTreeListView.h"
#include "BSTEditor/SelectionTreeModifier.h"

#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)|*.xml"

IMPLEMENT_DYNAMIC( CSelectionTreeListView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeListView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_CLICK, ID_TREE_CONTROL, OnTvnClick )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
	ON_NOTIFY( NM_DBLCLK , ID_TREE_CONTROL, OnTvnDblClick )
END_MESSAGE_MAP()

typedef enum
{
	eSTLVMC_DoNothing = 0,

	eSTLVMC_AddTree,
	eSTLVMC_EditTree,
	eSTLVMC_RemoveTree,

	eSTLVMC_AddTreeToBlock,

} SelectionTreeListViewMenuCommands;

CSelectionTreeListView::CSelectionTreeListView()
{
}

CSelectionTreeListView::~CSelectionTreeListView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterEventListener( this );
}

BOOL CSelectionTreeListView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect( rc );

	m_treelist.Create( WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_treelist.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterEventListener( this );

	return TRUE;
}

void CSelectionTreeListView::OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_treelist.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_treelist.HitTest( clientPoint );

	if ( clickedItemHandle != 0 )
	{
		int treeIndex = 0;
		const char* name = GetDataByItem( clickedItemHandle, &treeIndex );
		if ( name )
		{
			const HTREEITEM parentItemHandle = m_treelist.GetParentItem( clickedItemHandle );
			if ( parentItemHandle == m_treeRoot ) // display tree
			{
				GetIEditor()->GetSelectionTreeManager()->DisplayTree( name );
			}
			else if ( parentItemHandle ) // display ref group (with selected tree)
			{
				name = GetDataByItem( parentItemHandle );
				if ( name )
				{
					//GetIEditor()->GetSelectionTreeManager()->DisplayRefGroup( name, treeIndex );
				}
			}
		}
	}
}

void CSelectionTreeListView::OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_treelist.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_treelist.HitTest( clientPoint );

	const char* name = NULL;
	const char* group = NULL;
	int treeIndex = 0;

	if ( clickedItemHandle != 0 )
	{
		group = name = GetDataByItem( clickedItemHandle, &treeIndex );
		if ( name )
		{
			const HTREEITEM parentItemHandle = m_treelist.GetParentItem( clickedItemHandle );
			if ( parentItemHandle == m_treeRoot ) //  tree
			{
				string nameStr = name;
				GetIEditor()->GetSelectionTreeManager()->GetModifier()->EditTree( nameStr.c_str() );
			}
			else if ( parentItemHandle ) // ref group (with selected tree)
			{
				/*group = GetDataByItem( parentItemHandle );
				if ( group )
				{
					string nameStr = name;
					string groupStr = group;
					GetIEditor()->GetSelectionTreeManager()->GetModifier()->RenameTreeFromBlock( nameStr.c_str(), groupStr.c_str() );
				}*/
			}
		}
	}
}

void CSelectionTreeListView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	const HTREEITEM selectedItemHandle = m_treelist.GetSelectedItem();

	CPoint screenPoint;
	::GetCursorPos( &screenPoint );

	CPoint clientPoint = screenPoint;
	m_treelist.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_treelist.HitTest( clientPoint );

	CMenu menu;
	menu.CreatePopupMenu();
	const char* name = NULL;
	const char* group = NULL;
	int treeIndex = 0;
	if ( clickedItemHandle != 0 )
	{
		group = name = GetDataByItem( clickedItemHandle, &treeIndex );
		if ( name )
		{
			const HTREEITEM parentItemHandle = m_treelist.GetParentItem( clickedItemHandle );
			if ( parentItemHandle == m_treeRoot ) //  tree
			{
				menu.AppendMenu( MF_STRING, eSTLVMC_EditTree, "Edit tree" );
				menu.AppendMenu( MF_STRING, eSTLVMC_RemoveTree, "Delete tree" );
			}
			else if ( parentItemHandle ) // ref group (with selected tree)
			{
		/*		group = GetDataByItem( parentItemHandle );
				if ( group )
				{
					menu.AppendMenu( MF_STRING, eSTLVMC_RenameTreeFromBlock, "Edit tree" );
					menu.AppendMenu( MF_STRING, eSTLVMC_RemoveTreeFromBlock, "Delete tree from ref block" );
				}*/
			}
		}
		else if ( clickedItemHandle == m_treeRoot )
		{
			menu.AppendMenu( MF_STRING, eSTLVMC_AddTree, "Add new tree" );
		}

	}

	string nameStr = name ? name : "";
	string groupStr = group ? group : "";

	const int commandId = ::TrackPopupMenuEx( menu.GetSafeHmenu(), TPM_LEFTBUTTON | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetSafeHwnd(), NULL );

	if ( commandId == eSTLVMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTLVMC_AddTree )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->CreateNewTree();
		return;
	}

	if ( commandId == eSTLVMC_EditTree )
	{
		GetIEditor()->GetSelectionTreeManager()->GetModifier()->EditTree( nameStr.c_str() );
		return;
	}

	if ( commandId == eSTLVMC_RemoveTree )
	{
		GetIEditor()->GetSelectionTreeManager()->DeleteTree( nameStr.c_str() );
		return;
	}
}

void CSelectionTreeListView::OnEvent( EHistoryEventType event, void* pData )
{
	switch ( event )
	{
	case eHET_HistoryDeleted:
		ClearList();
		break;
	case eHET_HistoryGroupAdded:
	case eHET_HistoryGroupRemoved:
	case eHET_HistoryInvalidate:
	case eHET_HistoryCleared:
		ClearList();
	default:
		UpdateSelectionTrees();
		break;
	}

}

void CSelectionTreeListView::UpdateSelectionTrees()
{
	TSelectionTreeInfoList infoList;
	GetIEditor()->GetSelectionTreeManager()->GetInfoList( infoList );

	HTREEITEM selectedItem = 0;
	for ( TSelectionTreeInfoList::const_iterator it = infoList.begin(); it != infoList.end(); ++it )
	{

		const SSelectionTreeInfo& info = *it;

		HTREEITEM item = GetOrCreateItemForInfo(info);
		if ( info.IsLoaded ) selectedItem = item;

		if ( !info.IsTree && info.GetBlockCountById(eSTTI_Tree) > 1 )
		{
			int treeCount = 0;
			for ( int i = 0; i < info.GetBlockCount(); ++i )
			{

				if (info.Blocks[ i ].Type != eSTTI_Tree)
					continue;

				HTREEITEM subitem = GetOrCreateItemForRefTreeInfo(info.Blocks[ i ], item, treeCount);
				if ( info.IsLoaded && treeCount == info.CurrTreeIndex ) selectedItem = subitem;
				treeCount++;
			}
		}
	}
	m_treelist.SelectItem( selectedItem );
}

void CSelectionTreeListView::ClearList()
{
	m_treelist.DeleteAllItems();
	m_ItemList.clear();
	m_treeRoot = m_treelist.InsertItem( "SelectionTrees", 0, 0, TVI_ROOT, TVI_LAST );
}

const char* CSelectionTreeListView::GetDataByItem( HTREEITEM item, int* pIndex /*= NULL*/ )

{
	for ( TItemList::const_iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		if ( it->Item == item )
		{
			if(pIndex) *pIndex = it->Index;
			return it->Name.c_str();
		}
	}
	return NULL;
}

HTREEITEM CSelectionTreeListView::GetItemByInfo(const SSelectionTreeInfo& info)
{
	for ( TItemList::const_iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		if (it->Name == info.Name)
		{
			if (info.IsTree && m_treelist.GetParentItem(it->Item) == m_treeRoot)
				return it->Item;
		}
	}
	return 0;
}

HTREEITEM CSelectionTreeListView::GetItemByRefTreeInfo(const SSelectionTreeBlockInfo &info, HTREEITEM parent)
{
	for ( TItemList::const_iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		if (it->Name == info.Name)
		{
			if (m_treelist.GetParentItem(it->Item) == parent)
				return it->Item;
		}
	}
	return 0;
}

HTREEITEM CSelectionTreeListView::GetOrCreateItemForInfo( const SSelectionTreeInfo &info )
{
	HTREEITEM item = GetItemByInfo(info);

	string name = GetDisplayString(info);
	if ( info.IsModified )
		name += "*";

	if (!item)
	{
		HTREEITEM parent = info.IsTree ? m_treeRoot : NULL;
		item = m_treelist.InsertItem( name, 0, 0, parent, TVI_SORT );
		SItemInfo iteminfo;
		iteminfo.Name = info.Name;
		iteminfo.Item = item;
		iteminfo.Index = 0;
		m_ItemList.push_back(iteminfo);
		m_treelist.Expand(parent, TVE_EXPAND);
	}
	else
	{
		m_treelist.SetItemText( item,  name.c_str() );
	}
	return item;
}

HTREEITEM CSelectionTreeListView::GetOrCreateItemForRefTreeInfo( const SSelectionTreeBlockInfo &info, HTREEITEM parent, int index )
{
	HTREEITEM item = GetItemByRefTreeInfo(info, parent);

	string name = info.Name;
	if ( info.IsModified )
		name += "*";

	if (!item)
	{
		item = m_treelist.InsertItem( name, 0, 0, parent, TVI_SORT );
		SItemInfo iteminfo;
		iteminfo.Name = info.Name;
		iteminfo.Item = item;
		iteminfo.Index = index;
		m_ItemList.push_back(iteminfo);
		m_treelist.Expand(parent, TVE_EXPAND);
	}
	else
	{
		m_treelist.SetItemText( item,  name.c_str() );
	}
	return item;
}


string CSelectionTreeListView::GetDisplayString(const SSelectionTreeInfo& info) const
{
	string out;
	if (info.IsTree)
	{
		out = info.Name;
	}
	else
	{
		int treecount = info.GetBlockCountById(eSTTI_Tree);
		SSelectionTreeBlockInfo tree;
		info.GetBlockById(tree, eSTTI_Tree);
		if (treecount == 1) out = tree.Name;
		else out = info.Name;
		if (treecount > 1) out += string().Format(" (%i Trees)", treecount);
	}
	return out;
}

