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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEVARIABLESVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEVARIABLESVIEW_H
#pragma once



#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTreeVariablesView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
	, public IXmlUndoEventHandler
{
	DECLARE_DYNAMIC( CSelectionTreeVariablesView )

public:
	CSelectionTreeVariablesView();
	virtual ~CSelectionTreeVariablesView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId );

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	void SetDebugging(bool debug);
	void SetVarVal(const char* varName, bool val);

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	bool LoadVarsNode( const XmlNodeRef& xmlNode, HTREEITEM hItem );
	
	bool CreateVarItem( const char* name, HTREEITEM hItem, HTREEITEM hInsertAfter );
	void DeleteVarItem( HTREEITEM hItem );

	void AddVar( HTREEITEM hItem );
	void RenameVar( HTREEITEM hItem );
	void DeleteVar( HTREEITEM hItem );

	bool IsVarValid( const string& name );
	bool IsRefValid( const string& name );

	void ClearList();

private:
	CTreeCtrl m_variables;
	bool m_bLoaded;
	bool m_bDebugging;

	typedef std::list< HTREEITEM > TItemList;
	typedef std::map< HTREEITEM, TItemList > TItemMap;
	TItemMap m_VarMap;

	struct SItemInfo
	{
		SItemInfo( const string& name = "", bool isRef = false )
			: Name( name )
			, IsRef( isRef )
			, DebugVal(-1)
		{}

		string Name;
		bool IsRef;
		int DebugVal;
	};

	typedef std::map< HTREEITEM, SItemInfo > TItemInfoMap;
	TItemInfoMap m_ItemInfoMap;
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEVARIABLESVIEW_H
