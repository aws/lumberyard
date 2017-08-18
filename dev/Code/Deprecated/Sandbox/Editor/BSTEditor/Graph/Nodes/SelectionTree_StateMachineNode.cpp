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
#include "SelectionTree_StateMachineNode.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Dialogs/DialogCommon.h"

enum CSelectionTree_StateMachineNode_ContextCommands
{
	eSTSMNCC_ChangeToLeaf = 0,
	eSTSMNCC_ChangeToPriority,
	eSTSMNCC_ChangeToStateMachine,
	eSTSMNCC_ChangeToSequence,
	eSTSMNCC_Edit,
	eSTSMNCC_MakeRef,
	eSTSMNCC_EditLeafTranslation,
};


CSelectionTree_StateMachineNode::CSelectionTree_StateMachineNode()
: CSelectionTree_TreeNode(  )
{
	SetClass( "StateMachineNode" );
}

CSelectionTree_StateMachineNode::~CSelectionTree_StateMachineNode()
{
}

CHyperNode* CSelectionTree_StateMachineNode::Clone()
{
	CSelectionTree_StateMachineNode* pNode = new CSelectionTree_StateMachineNode();

	pNode->CopyFrom( *this );

	return pNode;
}

void CSelectionTree_StateMachineNode::PopulateContextMenu( CMenu& menu, int baseCommandId )
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
		
	//	menu.AppendMenu( MF_POPUP, reinterpret_cast< UINT_PTR >( subMenu.GetSafeHmenu() ), "Change Type" );
	//	menu.AppendMenu( MF_STRING, baseCommandId + eSTSMNCC_Edit, "Edit Name" );
	//	menu.AppendMenu( MF_SEPARATOR );
	}
}

bool CSelectionTree_StateMachineNode::OnContextMenuCommandEx( int nCmd, string& undoDesc )
{
	if ( nCmd == eSTSMNCC_Edit )
	{
		string oldname = GetName();
		bool res = DefaultEditDialog( true );
		undoDesc.Format( "Renamed Node (from %s to %s)", oldname.c_str(), GetName() );
		return res && oldname != GetName();
	}

	return false;
}

bool CSelectionTree_StateMachineNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	const char* name = xmlNode->getAttr( "name" );
	
	SetAttributes(xmlNode);
	name =  xmlNode->getTag();

	if ( name && name[0] )
	{
		SetName( name );
		Invalidate( true );
		return LoadChildrenFromXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_StateMachineNode::SaveToXml( XmlNodeRef& xmlNode )
{
	return CSelectionTree_TreeNode::SaveToXml(xmlNode);
}
