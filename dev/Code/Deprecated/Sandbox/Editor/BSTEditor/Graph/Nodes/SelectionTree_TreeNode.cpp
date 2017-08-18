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
#include "SelectionTree_TreeNode.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Dialogs/DialogCommon.h"

enum CSelectionTree_TreeNode_ContextCommands
{
	eSTTNCM_ChangeToLeaf = 0,
	eSTTNCM_ChangeToPriority,
	eSTTNCM_ChangeToStateMachine,
	eSTTNCM_ChangeToSequence,
	eSTTNCM_Edit,
	eSTTNCM_MakeRef,
	eSTTNCM_EditLeafTranslation,
};


CSelectionTree_TreeNode::CSelectionTree_TreeNode()
: CSelectionTree_BaseNode( )
, m_TranslatedName("")
, m_TranslationId(-1)
{
	SetClass( "TreeNode" );

	CreateDefaultInPort( ePCT_SingleConnectionAllowed );
	CreateDefaultOutPort( ePCT_MultiConnectionAllowed );
}

CSelectionTree_TreeNode::~CSelectionTree_TreeNode()
{
}

void CSelectionTree_TreeNode::Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar )
{
	CSelectionTree_BaseNode::Serialize( node, bLoading, ar );
}

CHyperNode* CSelectionTree_TreeNode::Clone()
{
	CSelectionTree_TreeNode* pNode = new CSelectionTree_TreeNode();

	pNode->CopyFrom( *this );

	return pNode;
}

void CSelectionTree_TreeNode::PopulateContextMenu( CMenu& menu, int baseCommandId )
{
	if ( !IsReadOnly() )
	{
		static CMenu subMenu;
		static bool bInit = false;
		if ( bInit )
		{
			subMenu.DestroyMenu();
		}
		bInit = true;
		subMenu.CreatePopupMenu();;
		
		//TODO: re-implement a way to change nodes
	//	menu.AppendMenu( MF_POPUP, reinterpret_cast< UINT_PTR >( subMenu.GetSafeHmenu() ), "Change Type" );
	//	menu.AppendMenu( MF_STRING, baseCommandId + eSTTNCM_Edit, "Edit Name" );
	//	menu.AppendMenu( MF_SEPARATOR );
	}
}

bool CSelectionTree_TreeNode::OnContextMenuCommandEx( int nCmd, string& undoDesc )
{
	if ( nCmd == eSTTNCM_Edit )
	{
		string oldname = GetName();
		bool res = DefaultEditDialog( true );
		undoDesc.Format( "Renamed Node (from %s to %s)", oldname.c_str(), GetName() );
		return res && oldname != GetName();
	}

	if ( nCmd >= eSTTNCM_ChangeToLeaf && nCmd <= eSTTNCM_ChangeToSequence )
	{
		/*ESelectionTreeNodeSubType oldType = m_nodeSubType;
		m_nodeSubType = (ESelectionTreeNodeSubType) nCmd;
		Invalidate(true);
		undoDesc.Format("Changed Node Type from %s to %s for Node %s", GetSubTypeStr( oldType ), GetSubTypeStr( m_nodeSubType ), GetName() );*/
		return true;
	}

	if ( nCmd == eSTTNCM_EditLeafTranslation)
	{
		GetTreeGraph()->StartMappingEditMode(this);
	}
	return false;
}

bool CSelectionTree_TreeNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	const char* name = xmlNode->getTag();;
	
	SetAttributes(xmlNode);
	
	if ( name && name[0] )
	{
		SetName( name );
		Invalidate( true );
		return LoadChildrenFromXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_TreeNode::SaveToXml( XmlNodeRef& xmlNode )
{
	string name = GetName();
	if ( name.length() > 0 )
	{
		xmlNode->setTag( name );
		XmlNodeRef attribNode = GetAttributes();
		for (int i = 0; i < attribNode->getNumAttributes(); i++)
		{
			const char* key;
			const char* value;
			attribNode->getAttributeByIndex(i, &key, &value);
			xmlNode->setAttr(key, value);
		}
		return SaveChildrenToXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_TreeNode::AcceptChild( CSelectionTree_BaseNode* pChild )
{
	if ( IsReadOnly() || pChild->IsReadOnly() )
	{
		return false;
	}
	return true;
}

bool CSelectionTree_TreeNode::DefaultEditDialog( bool bCreate, bool* pHandled, string* pUndoDesc )
{
	if (pHandled) *pHandled = true;
	if ( IsReadOnly() )
	{
		return false;
	}

	//for now, return since we don't have a good implementation to rename/replace nodes
	return false;

	CBSTStringDlg dlg ( _T("Edit node name") );
	dlg.SetString( GetName() );
	if ( dlg.DoModal() == IDOK )
	{
		string newName = dlg.GetString().GetString();
		if (newName.length() > 0 && (bCreate || newName != GetName()) )
		{
			if (pUndoDesc) pUndoDesc->Format("Renamed node %s to %s", GetName(), newName.c_str());
			SetAttributes(GetIEditor()->FindTemplate(newName.c_str()));
			SetName( newName.c_str() );
			return true;
		}
	}
	return false;
}

void CSelectionTree_TreeNode::UpdateTranslations()
{
	m_TranslatedName = "";
	m_TranslationId = -1;
	
	CSelectionTree_BaseNode::UpdateTranslations();
	
}

void CSelectionTree_TreeNode::PropertyChanged( XmlNodeRef var )
{
	XmlNodeRef attribNode = GetAttributes();
	const char* value;
	var->getAttr("value", &value);
	attribNode->setAttr(var->getTag(), value);
}

void CSelectionTree_TreeNode::FillPropertyTable( XmlNodeRef var )
{
	XmlNodeRef xmlNode = GetAttributes();
	for (int i = 0; i < xmlNode->getNumAttributes(); i++)
	{
		const char* key;
		const char* value;
		xmlNode->getAttributeByIndex(i, &key, &value);
		XmlNodeRef propertyNode = var->findChild(key);
		if(propertyNode)
		{
			propertyNode->setAttr("value", value);
		}
	}

}
