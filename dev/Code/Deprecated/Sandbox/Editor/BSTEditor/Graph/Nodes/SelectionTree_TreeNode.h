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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_TREENODE_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_TREENODE_H
#pragma once


#define INVALID_HELPER_NODE "__HELPER_NODE_INVALIDE__"

#include "SelectionTree_BaseNode.h"

class CSelectionTree_TreeNode
	: public CSelectionTree_BaseNode
{
public:

	CSelectionTree_TreeNode( );
	virtual ~CSelectionTree_TreeNode();

	// CHyperNode
	virtual CHyperNode* Clone();
	virtual void PopulateContextMenu( CMenu& menu, int baseCommandId );
	virtual void Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar=0 );

	// CSelectionTree_BaseNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool AcceptChild( CSelectionTree_BaseNode* pChild );
	virtual bool DefaultEditDialog( bool bCreate = false, bool* pHandled = NULL, string* pUndoDesc = NULL );
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc );
	virtual void UpdateTranslations();

	// CSelectionTree_TreeNode
	const char* GetTranslation() const { return m_TranslatedName.c_str(); }
	bool IsTranslated() const { return m_TranslationId != -1; }
	int GetTranslationId() const { return m_TranslationId; }

	virtual void PropertyChanged(XmlNodeRef var);
	virtual void FillPropertyTable(XmlNodeRef var);

private:
	string m_TranslatedName;
	int m_TranslationId;
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_TREENODE_H
