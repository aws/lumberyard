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
#include "SelectionTree_StateNode.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Dialogs/DialogCommon.h"

enum CSelectionTree_StateNode_ContextCommands
{
	eSTSNCC_ChangeToLeaf = 0,
	eSTSNCC_ChangeToPriority,
	eSTSNCC_ChangeToStateMachine,
	eSTSNCC_ChangeToSequence,
	eSTSNCC_Edit,
	eSTSNCC_AddTransition,
	eSTSNCC_MakeRef,
	eSTSNCC_EditLeafTranslation,
};


CSelectionTree_StateNode::CSelectionTree_StateNode()
: CSelectionTree_TreeNode(  )
{
	SetClass( "StateNode" );
}

CSelectionTree_StateNode::~CSelectionTree_StateNode()
{
}

CHyperNode* CSelectionTree_StateNode::Clone()
{
	CSelectionTree_StateNode* pNode = new CSelectionTree_StateNode();

	pNode->CopyFrom( *this );

	return pNode;
}

void CSelectionTree_StateNode::PopulateContextMenu( CMenu& menu, int baseCommandId )
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
	//	menu.AppendMenu( MF_STRING, baseCommandId + eSTSNCC_Edit, "Edit Name" );
		menu.AppendMenu( MF_STRING, baseCommandId + eSTSNCC_AddTransition, "Add Transition" );

		menu.AppendMenu( MF_SEPARATOR );
	}
}

bool CSelectionTree_StateNode::OnContextMenuCommandEx( int nCmd, string& undoDesc )
{
	if ( nCmd == eSTSNCC_Edit )
	{
		string oldname = GetName();
		bool res = DefaultEditDialog( true );
		undoDesc.Format( "Renamed Node (from %s to %s)", oldname.c_str(), GetName() );
		return res && oldname != GetName();
	}

	if ( nCmd == eSTSNCC_AddTransition )
	{
		char buff[128];
		sprintf_s(buff, "Transition%i", m_Transitions.size()+1);
		m_Transitions[buff] = Transition();

		GetIEditor()->GetSelectionTreeManager()->GetPropertiesView()->OnNodeClicked(this);
	}

	return false;
}

bool CSelectionTree_StateNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	const char* name = xmlNode->getAttr( "name" );
	
	SetAttributes(xmlNode);
	
	if ( name && name[0] )
	{
		SetName( name );
		Invalidate( true );
		return LoadChildrenFromXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_StateNode::SaveToXml( XmlNodeRef& xmlNode )
{
	string name = GetName();
	if ( name.length() > 0 )
	{
		xmlNode->setTag( "State" );
		
		XmlNodeRef attribNode = GetAttributes();
		for (int i = 0; i < attribNode->getNumAttributes(); i++)
		{
			const char* key;
			const char* value;
			attribNode->getAttributeByIndex(i, &key, &value);
			xmlNode->setAttr(key, value);
		}

		XmlNodeRef transitionsNode = gEnv->pSystem->CreateXmlNode( "Transitions" );

		for (std::map<string, Transition>::iterator it = m_Transitions.begin(); it != m_Transitions.end(); it++)
		{
			XmlNodeRef transitionNode = gEnv->pSystem->CreateXmlNode( "Transition" );
			transitionNode->setAttr("to", (*it).second.m_toState);
			transitionNode->setAttr("onEvent", (*it).second.m_onEvent);
			transitionsNode->addChild(transitionNode);
		}
		
		if(transitionsNode->getChildCount() > 0)
			xmlNode->addChild(transitionsNode);

		return SaveChildrenToXml( xmlNode );
	}
	return false;
}

bool CSelectionTree_StateNode::LoadChildrenFromXml( const XmlNodeRef& xmlNode )
{
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );
		if(strcmp(xmlChild->getTag(), "Transitions") == 0)
		{
			LoadTransitions(xmlChild);
		}
		else
		{
			CSelectionTree_BaseNode* pChild = GetTreeGraph()->CreateNewNode( xmlChild );
			if ( pChild )
			{
				{
					GetTreeGraph()->ConnectNodes( this, pChild->IsRoot() ? pChild : pChild->GetParentNode()  );				
				}
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

void CSelectionTree_StateNode::LoadTransitions(XmlNodeRef transitionsNode)
{
	if (transitionsNode)
	{
		for (int i = 0; i < transitionsNode->getChildCount(); i++)
		{
			char buff[128];
			sprintf_s(buff, "Transition%i", i+1);
			

			XmlNodeRef XmlChild = transitionsNode->getChild(i);
			string toState = XmlChild->getAttr("to");
			string onEvent = XmlChild->getAttr("onEvent");
			m_Transitions[buff] = Transition(toState, onEvent);
		}
	}
}

bool CSelectionTree_StateNode::DefaultEditDialog( bool bCreate, bool* pHandled, string* pUndoDesc )
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
			SetName( newName.c_str() );
			return true;
		}
	}
	return false;
}

void CSelectionTree_StateNode::PropertyChanged( XmlNodeRef var )
{
	bool isTransition = false;
	XmlNodeRef parentNode = var->getParent();
	if(parentNode)
	{
		std::map<string, Transition>::iterator it = m_Transitions.find(parentNode->getTag());

		if(it != m_Transitions.end())
		{
			isTransition = true;
			
			if(strcmp(var->getTag(), "toState") == 0)
				(*it).second.m_toState = var->getAttr("value");
			if(strcmp(var->getTag(), "onEvent") == 0)
				(*it).second.m_onEvent = var->getAttr("value");
		}
	}
	
	if(!isTransition)
	{
		XmlNodeRef attribNode = GetAttributes();
		const char* value;
		var->getAttr("value", &value);
		attribNode->setAttr(var->getTag(), value);

		if(strcmp(var->getTag(), "name") == 0)
		{
			SetName(value);
		}
	}
}

void CSelectionTree_StateNode::FillPropertyTable( XmlNodeRef var )
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
	
	for (int i = 0; i < m_Transitions.size(); i++)
	{
		char buff[128];
		sprintf_s(buff, "Transition%i", i+1);
		XmlNodeRef transitionsNode = var->newChild(buff);

		XmlNodeRef ToStateNode = transitionsNode->newChild("ToState");
		ToStateNode->setTag("toState");
		ToStateNode->setAttr("type", "string");
		ToStateNode->setAttr("value", m_Transitions[buff].m_toState);

		XmlNodeRef OnEventNode = transitionsNode->newChild("OnEvent");
		OnEventNode->setTag("onEvent");
		OnEventNode->setAttr("type", "string");
		OnEventNode->setAttr("value", m_Transitions[buff].m_onEvent);
	}

}

void CSelectionTree_StateNode::Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar/*=0 */ )
{
	CHyperNode::Serialize( node, bLoading, ar );
	if(!bLoading)
	{
		node->addChild(m_attributes);
	}
	else
	{
		for (int i = 0; i < node->getChildCount(); i++)
		{
			if(strcmp("State", node->getChild(i)->getTag()) == 0)
			{
				XmlNodeRef childNode = node->getChild(i);
				for (int j = 0; j < childNode->getChildCount(); j++)
				{
					if(strcmp("Transitions", childNode->getChild(j)->getTag()) == 0)
					{
						LoadTransitions(childNode->getChild(j));
						break;
					}
				}
				m_attributes = node->getChild(i);
				break;
			}
		}
	}
}
