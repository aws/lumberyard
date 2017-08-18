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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREELISTVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREELISTVIEW_H
#pragma once


#include "SelectionTreeBaseDockView.h"
#include "BSTEditor/SelectionTreeManager.h"

class CSelectionTreeListView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryEventListener
{
	DECLARE_DYNAMIC( CSelectionTreeListView )

public:
	CSelectionTreeListView();
	virtual ~CSelectionTreeListView();

	void UpdateSelectionTrees();
	void ClearList();

	// IXmlHistoryEventListener
	virtual void OnEvent( EHistoryEventType event, void* pData = NULL );

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	CTreeCtrl m_treelist;
	HTREEITEM m_treeRoot;

	struct SItemInfo
	{
		string Name;
		HTREEITEM Item;
		int Index;
	};
	typedef std::vector< SItemInfo > TItemList;
	TItemList m_ItemList;

private:
	const char* GetDataByItem( HTREEITEM item, int* pIndex = NULL );

	HTREEITEM GetItemByInfo(const SSelectionTreeInfo& info);
	HTREEITEM GetItemByRefTreeInfo(const SSelectionTreeBlockInfo &info, HTREEITEM parent);
	HTREEITEM GetOrCreateItemForInfo( const SSelectionTreeInfo &info );
	HTREEITEM GetOrCreateItemForRefTreeInfo( const SSelectionTreeBlockInfo &info, HTREEITEM parent, int index );

	string GetDisplayString(const SSelectionTreeInfo& info) const;
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREELISTVIEW_H
