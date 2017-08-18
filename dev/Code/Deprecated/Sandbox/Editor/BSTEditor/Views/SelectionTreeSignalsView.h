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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREESIGNALSVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREESIGNALSVIEW_H
#pragma once



#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTreeSignalsView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
	, public IXmlUndoEventHandler
{
	DECLARE_DYNAMIC( CSelectionTreeSignalsView )

public:
	CSelectionTreeSignalsView();
	virtual ~CSelectionTreeSignalsView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId );

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );
	
protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	bool LoadSignalNodes( const XmlNodeRef& xmlNode, HTREEITEM hItem );
	
	bool CreateSignalItem( const char* name, const char* varname, bool val, HTREEITEM hItem, HTREEITEM hInsertAfter );
	void DeleteSignalItem( HTREEITEM hItem );
	void DeleteSignalItems( HTREEITEM hItem );

	void AddSignal( HTREEITEM hItem );
	void RemoveSignalVars( HTREEITEM hItem );

	void AddSigVar( HTREEITEM hItem );
	void DeleteSigVar( HTREEITEM hItem );
	void ToggleValue( HTREEITEM hItem );

	void ClearList();

	HTREEITEM GetParentItem(HTREEITEM hItem);

private:
	CTreeCtrl m_signals;
	bool m_bLoaded;

	enum EItemType
	{
		eIT_UNDEFINED = -1,
		eIT_Ref = 0,
		eIT_Signal,
		eIT_SignalVariable,
	};

	struct SVarInfo
	{
		SVarInfo( const string& signalname, const string& varname, bool val ) : Signal( signalname ), Var( varname ), Value( val ) {}
		static EItemType GetType() { return eIT_SignalVariable; }

		string Signal;
		string Var;
		bool Value;
	};
	typedef std::map< HTREEITEM, SVarInfo > TVarInfoMap;

	struct SRefInfo
	{
		SRefInfo( const string& refName ) : Name( refName ) {}
		static EItemType GetType() { return eIT_Ref; }
		string Name;
	};
	typedef std::map< HTREEITEM, SRefInfo > TRefInfoMap;

	struct SSignalInfo
	{
		SSignalInfo( const string& signalName ) : Name( signalName ) {}
		static EItemType GetType() { return eIT_Signal; }
		string Name;
	};
	typedef std::map< HTREEITEM, SSignalInfo > TSignalInfoMap;


	struct SChildItem
	{
		SChildItem(HTREEITEM item, EItemType type) : Item(item), Type(type) {}
		bool operator==(const HTREEITEM& item) const { return Item == item; }
		HTREEITEM Item;
		EItemType Type;
	};

	typedef std::list< SChildItem > TChildItemList;
	typedef std::map< HTREEITEM, TChildItemList > TChildItemMap;

	struct STreeInfo
	{
		void Clear()
		{
			ChildItems.clear();
			SignalInfo.clear();
			VarInfo.clear();
			RefInfo.clear();
			ChildItems[TVI_ROOT] = TChildItemList();
		}
		TChildItemMap ChildItems;
		TSignalInfoMap SignalInfo;
		TVarInfoMap VarInfo;
		TRefInfoMap RefInfo;
	};

	STreeInfo m_TreeInfo;

	template <class T>
	const T& GetItemInfo(const std::map< HTREEITEM, T >& map, HTREEITEM hItem) const
	{
		std::map< HTREEITEM, T >::const_iterator it = map.find(hItem);
		assert(it != map.end());
		return it->second;
	}

	template <class T>
	T& GetItemInfo(std::map< HTREEITEM, T >& map, HTREEITEM hItem)
	{
		std::map< HTREEITEM, T >::iterator it = map.find(hItem);
		assert(it != map.end());
		return it->second;
	}

	EItemType GetItemType(HTREEITEM hItem)
	{
		if (m_TreeInfo.VarInfo.find(hItem) != m_TreeInfo.VarInfo.end()) return eIT_SignalVariable;
		if (m_TreeInfo.SignalInfo.find(hItem) != m_TreeInfo.SignalInfo.end()) return eIT_Signal;
		if (m_TreeInfo.RefInfo.find(hItem) != m_TreeInfo.RefInfo.end()) return eIT_Ref;
		return eIT_UNDEFINED;
	}

	template <class T> 
	void DeleteItem(HTREEITEM hItem, T& map)
	{
		HTREEITEM hParent = GetParentItem( hItem );
		assert(m_TreeInfo.ChildItems.find(hParent) != m_TreeInfo.ChildItems.end());
		bool ok = stl::find_and_erase(m_TreeInfo.ChildItems[hParent], hItem);
		assert( ok );

		m_signals.DeleteItem( hItem );
		assert(map.find(hItem) != map.end());
		map.erase(map.find(hItem));
	}

	template <class T>
	HTREEITEM InsertItem(HTREEITEM hParent, HTREEITEM hInsertAfter, const char* displayName, std::map< HTREEITEM, T >& map, const T& item)
	{
		HTREEITEM hNewItem = m_signals.InsertItem( displayName, 0, 0, hParent, hInsertAfter);
		map.insert( std::make_pair(hNewItem, item) );
		m_TreeInfo.ChildItems[ hParent ].push_back( SChildItem(hNewItem, T::GetType()) );
		return hNewItem;
	}

	TChildItemList& GetChildList(HTREEITEM hItem)
	{
// 		assert(m_TreeInfo.ChildItems.find(hItem) != m_TreeInfo.ChildItems.end());
		return m_TreeInfo.ChildItems[ hItem ];
	}

	const TChildItemList& GetChildList(HTREEITEM hItem) const
	{
		assert(m_TreeInfo.ChildItems.find(hItem) != m_TreeInfo.ChildItems.end());
		return m_TreeInfo.ChildItems.find(hItem)->second;
	}

};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREESIGNALSVIEW_H
