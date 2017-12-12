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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_STATENODE_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_STATENODE_H
#pragma once


#define INVALID_HELPER_NODE "__HELPER_NODE_INVALIDE__"

#include "SelectionTree_TreeNode.h"

class CSelectionTree_StateNode
	: public CSelectionTree_TreeNode
{
public:
	struct Transition
	{
		Transition(string toState, string onEvent)
		{
			m_toState = toState;
			m_onEvent = onEvent;
		}
		Transition()
		{
			m_toState = "";
			m_onEvent = "";
		}
		string m_toState;
		string m_onEvent;
	};

	enum ESelectionTreeNodeSubType
	{
		eSTNST_Invalid = -1,
		eSTNST_Leaf = 0,
		eSTNST_Priority,
		eSTNST_StateMachine,
		eSTNST_Sequence,
		eSTNST_COUNT
	};

	CSelectionTree_StateNode( );
	virtual ~CSelectionTree_StateNode();

	// CHyperNode
	virtual CHyperNode* Clone();
	virtual void PopulateContextMenu( CMenu& menu, int baseCommandId );
	virtual void Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar=0 );
	
	// CSelectionTree_BaseNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool DefaultEditDialog( bool bCreate = false, bool* pHandled = NULL, string* pUndoDesc = NULL );
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc );
	
	// CSelectionTree_TreeNode
	virtual void PropertyChanged(XmlNodeRef var);
	virtual void FillPropertyTable(XmlNodeRef var);
	virtual bool ShouldAppendLoggingProps() { return false; }

protected:
	bool LoadChildrenFromXml( const XmlNodeRef& xmlNode );
	void LoadTransitions(XmlNodeRef transitionsNode);

private:
	std::map<string, Transition> m_Transitions;
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_STATENODE_H
