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

#include "StdAfx.h"
#include "SelectionTreeLeafMapping.h"
#include "SelectionTreeGraph.h"

#include "Nodes/SelectionTree_TreeNode.h"
#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/Views/SelectionTreeGraphView.h"

#include <ArgumentParser.h> // for easy string splitting
#include "CustomMessageBox.h"

///////////////////////////////////////////////////////////////////////////
/////////////////////////// SLeafTranslation //////////////////////////////
///////////////////////////////////////////////////////////////////////////
SLeafTranslation& CLeafMappingManager::GetTranslation(int idx)
{
	if (idx >= 0 && idx < m_Mappings.size())
		return m_Mappings[idx];

	static SLeafTranslation dummy;
	return dummy;
}

bool SLeafTranslation::LoadFromXml( const XmlNodeRef& xmlNode )
{
	if (xmlNode)
	{
		const char* target = xmlNode->getAttr("target");
		Target = target ? target : "";

		const char* node = xmlNode->getAttr("node");
		if (node)
		{
			SArgumentParser tmp;
			tmp.SetDelimiter(":");
			tmp.AddArguments(node);
			for (int i = 0; i < tmp.GetArgCount(); ++i)
			{
				string nodeName;
				if (tmp.GetArg(i, nodeName))
					NodeStack.push_back(nodeName);
			}
		}
		return Target.length() > 0 && NodeStack.size() > 0;
	}
	return false;
}

bool SLeafTranslation::SaveToXml( const XmlNodeRef& xmlNode )
{
	if (xmlNode)
	{
		SArgumentParser tmp;
		tmp.SetDelimiter(":");
		for (TNodeNameStack::iterator it = NodeStack.begin(); it != NodeStack.end(); ++it)
		{
			tmp.AddArgument(*it);
		}
		xmlNode->setAttr("node", tmp.GetAsString());
		xmlNode->setAttr("target", Target.c_str());
		return Target.length() > 0 && NodeStack.size() > 0;
	}
	return false;
}

bool SLeafTranslation::CheckNode(CSelectionTree_TreeNode* pNode, string& translatedBehavior, CSelectionTree_TreeNode** pEndNode) const
{
	assert(NodeStack.size() > 0);
	bool res = CheckNodeInternal(pNode, NodeStack.size() - 1, pEndNode);
	if (res) translatedBehavior = Target;
	return res;
}

bool SLeafTranslation::CheckNodeInternal(CSelectionTree_TreeNode* pNode, int index, CSelectionTree_TreeNode** pEndNode) const
{
	assert(NodeStack.size() > index && index >= 0);

	if (pNode->GetName() == NodeStack[index])
	{
		if (index == 0)
		{
			if (pEndNode) *pEndNode = pNode;
			return true;
		}
		else
		{
			CSelectionTree_BaseNode* pParent = pNode->GetParentNode();
			/*while (pParent && pParent->GetNodeType() != CSelectionTree_BaseNode::eSTNT_TreeNode)
			{
				pParent = pParent->GetParentNode();
			}*/
			if (pParent /*&& pParent->GetNodeType() == CSelectionTree_BaseNode::eSTNT_TreeNode*/)
			{
				return CheckNodeInternal(static_cast<CSelectionTree_TreeNode*>(pParent), index - 1, pEndNode);
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////// CLeafMappingManager /////////////////////////////
///////////////////////////////////////////////////////////////////////////
CLeafMappingManager::CLeafMappingManager(CSelectionTreeGraph* pGraph)
: m_pGraph(pGraph)
, m_pEditor(new CLeafMappingEditor(pGraph))
{

}

CLeafMappingManager::~CLeafMappingManager()
{
	delete m_pEditor;
}

void CLeafMappingManager::ClearAll()
{
	m_Mappings.clear();
}

int CLeafMappingManager::CreateTranslation()
{
	SLeafTranslation translation;
	m_Mappings.push_back(translation);
	return m_Mappings.size() - 1;
}

void CLeafMappingManager::DeleteLeafTranslation(int idx)
{
	if (idx >= 0 && idx < m_Mappings.size())
	{
		TMappings::iterator it = m_Mappings.begin();
		for (int i = 0; i < idx; ++i, ++it);
		m_Mappings.erase(it);
	}
}

bool CLeafMappingManager::SaveToXml( XmlNodeRef& xmlNode )
{
	if (xmlNode)
	{
		xmlNode->setTag("LeafTranslations");
		for (TMappings::iterator it = m_Mappings.begin(); it != m_Mappings.end(); ++it)
		{
			XmlNodeRef mapNode = gEnv->pSystem->CreateXmlNode("Map");
			bool ok = it->SaveToXml(mapNode);
			assert( ok );
			xmlNode->addChild(mapNode);
		}
		return true;
	}
	return false;
}

bool CLeafMappingManager::LoadFromXml( const XmlNodeRef& xmlNode )
{
	m_Mappings.clear();
	if (xmlNode)
	{
		for (int i = 0; i < xmlNode->getChildCount(); ++i)
		{
			XmlNodeRef child = xmlNode->getChild(i);
			SLeafTranslation translation;
			bool ok = translation.LoadFromXml(child);
			assert( ok );
			m_Mappings.push_back(translation);
		}
		m_pGraph->UpdateTranslations();
		return true;
	}
	return false;
}

bool CLeafMappingManager::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	return LoadFromXml(xmlNode);
}

bool CLeafMappingManager::CheckNode(CSelectionTree_TreeNode* pNode, string& translatedBehavior, int& translationId) const
{
	translationId = 0;
	for (TMappings::const_iterator it = m_Mappings.begin(); it != m_Mappings.end(); ++it)
	{
		if (it->CheckNode(pNode, translatedBehavior)) return true;
		translationId++;
	}
	translationId = -1;
	return false;
}

///////////////////////////////////////////////////////////////////////////
/////////////////////////// CLeafMappingEditor ////////////////////////////
///////////////////////////////////////////////////////////////////////////
CLeafMappingEditor::CLeafMappingEditor(CSelectionTreeGraph* pGraph)
: m_pGraph(pGraph)
, m_pMappingEditNode( NULL )
, m_pMappingEndEditNode( NULL )
, m_CurrentLeafMappingIdx(-1)
, m_bIsCurrentLeafMappingNew(false)
{

}

CLeafMappingEditor::~CLeafMappingEditor()
{
}

void CLeafMappingEditor::StartEdit(CSelectionTree_TreeNode* pNode)
{
	assert( !IsInEditMode() );
	
	string target;

	//CEditBehaviorDialog newDlg;
	//newDlg.Init( "Select mapped Behavior", pNode->IsTranslated() ? pNode->GetTranslation() : "" );
	//if(newDlg.DoModal() == IDOK)
	//{
	//	target = newDlg.GetBehaviorName();
	//}

	//if (target.length() > 0)
	//{
	//	m_pMappingEditNode = pNode;
	//	if (pNode->IsTranslated())
	//	{
	//		m_CurrentLeafMappingIdx = pNode->GetTranslationId();
	//		SLeafTranslation& activeTranslation = m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx);
	//		m_LeafMappingBackup = activeTranslation;
	//		activeTranslation.Target = target;
	//		m_bIsCurrentLeafMappingNew = false;
	//	}
	//	else
	//	{
	//		m_CurrentLeafMappingIdx = m_pGraph->GetLeafTranslationMan()->CreateTranslation();
	//		SLeafTranslation& newTranslation = m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx);
	//		newTranslation.Target = target;
	//		newTranslation.NodeStack.push_back( pNode->GetName() );
	//		m_bIsCurrentLeafMappingNew = true;
	//		m_pGraph->UpdateTranslations();
	//	}

		////// find end node
		//bool ok = m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx).CheckNode(m_pMappingEditNode, target, &m_pMappingEndEditNode);
		//assert(ok && m_pMappingEndEditNode);

		//// lock tree and mark translations
		//LockTree();
		//MarkTranslation();
//	}
}

void CLeafMappingEditor::StopEdit(bool abord)
{
	if( !IsInEditMode() ) return;

	if (!abord)
	{
		SLeafTranslation& newMapping = m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx);
		if (newMapping != m_LeafMappingBackup)
		{
			SArgumentParser newMappingStr;
			newMappingStr.SetDelimiter(":");
			for (SLeafTranslation::TNodeNameStack::iterator it = newMapping.NodeStack.begin(); it != newMapping.NodeStack.end(); ++it)
				newMappingStr.AddArgument(*it);

			string msg;
			if (m_bIsCurrentLeafMappingNew)
			{
				msg = "Create new Behavior mapping?";
				msg += "\nTarget Behavior: ";
				msg += newMapping.Target;
				msg += "\nNode path to map: ";
				msg += newMappingStr.GetAsString();
			}
			else
			{
				SArgumentParser oldMappingStr;
				oldMappingStr.SetDelimiter(":");
				for (SLeafTranslation::TNodeNameStack::iterator it = m_LeafMappingBackup.NodeStack.begin(); it != m_LeafMappingBackup.NodeStack.end(); ++it)
					oldMappingStr.AddArgument(*it);

				msg = "Create new Behavior mapping?";
				msg += "\nTarget Behavior: ";
				msg += newMapping.Target;
				msg += " (was: ";
				msg += m_LeafMappingBackup.Target;
				msg += ")\nNode path to map: ";
				msg += newMappingStr.GetAsString();
				msg += "\nWas: ";
				msg += oldMappingStr.GetAsString();
			}
			int idx = CCustomMessageBox::Show( msg.c_str(), "Save Behavior Mapping?", "Cancel" /*idx 1*/, "Save" /*idx 2*/);
			abord = !(idx == 2);
		}
		else
		{
			abord = true; // nothing changed
		}
	}

	if (abord)
	{
		if (m_bIsCurrentLeafMappingNew)
		{
			m_pGraph->GetLeafTranslationMan()->DeleteLeafTranslation(m_CurrentLeafMappingIdx);
		}
		else
		{
			m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx) = m_LeafMappingBackup;
		}
	}
	else
	{
		GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo(m_pGraph->GetLeafTranslationMan(), "Modified Behavior Mapping");
	}

	m_pGraph->UpdateTranslations();
	ResetMarkings();
	UnlockTree();

	m_pMappingEditNode = NULL;
	m_pMappingEndEditNode = NULL;
	m_bIsCurrentLeafMappingNew = false;
	m_CurrentLeafMappingIdx = -1;
}

void CLeafMappingEditor::ChangeEndNode(CSelectionTree_BaseNode* pNode)
{
	assert( m_pMappingEditNode && m_pMappingEndEditNode );
	if (m_pMappingEndEditNode == pNode) 
	{
		StopEdit();
	}
	else// if (pNode->GetNodeType() == CSelectionTree_BaseNode::eSTNT_TreeNode)
	{
		CommitChanges(static_cast<CSelectionTree_TreeNode*>(pNode));
	}
}

bool CLeafMappingEditor::IsInEditMode() const
{
	if (m_pMappingEditNode != NULL)
	{
		assert(m_pMappingEditNode && m_pMappingEndEditNode && m_CurrentLeafMappingIdx != -1);
		return true;
	}
	return false;
}

void CLeafMappingEditor::CommitChanges(CSelectionTree_TreeNode* pEndNode)
{
	TBSTNodeList nodesInbetween;
	if (m_pGraph->IsInHierarchy(m_pMappingEditNode, pEndNode, &nodesInbetween))
	{
		SLeafTranslation& currentTranslation = m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx);
		currentTranslation.NodeStack.clear();

		currentTranslation.NodeStack.push_back( pEndNode->GetName() );
		for (TBSTNodeList::reverse_iterator it = nodesInbetween.rbegin(); it != nodesInbetween.rend(); ++it)
		{
			//if ( (*it)->GetNodeType() == CSelectionTree_BaseNode::eSTNT_TreeNode )
				currentTranslation.NodeStack.push_back( (*it)->GetName() );
		}
		if (m_pMappingEditNode != pEndNode)
			currentTranslation.NodeStack.push_back( m_pMappingEditNode->GetName() );

		m_pMappingEndEditNode = pEndNode;
		m_pGraph->UpdateTranslations();
		MarkTranslation();
	}
}

void CLeafMappingEditor::MarkTranslation()
{
	assert( IsInEditMode() );

	// mark passive nodes, reset other nodes
	std::list<CSelectionTree_TreeNode*> nodes;
	IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();

	for ( IHyperNode* pHyperNode = pEnum->GetFirst(); pHyperNode != NULL; pHyperNode = pEnum->GetNext() )
	{
		CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( pHyperNode );
		pBaseNode->SetBackground(CSelectionTree_BaseNode::eSTBS_None);
		//if ( pBaseNode->GetNodeType() == CSelectionTree_BaseNode::eSTNT_TreeNode )
		{
			CSelectionTree_TreeNode* pTreeNode = static_cast< CSelectionTree_TreeNode* >( pBaseNode );
			if (pTreeNode->IsTranslated() && pTreeNode->GetTranslationId() == m_CurrentLeafMappingIdx)
			{
				pTreeNode->SetBackground(CSelectionTree_BaseNode::eSTBS_Blue);
				// find end node and mark path as passive nodes
				CSelectionTree_TreeNode* pEndNode;
				string tmp;
				bool ok = m_pGraph->GetLeafTranslationMan()->GetTranslation(m_CurrentLeafMappingIdx).CheckNode(pTreeNode, tmp, &pEndNode);
				assert(ok && pEndNode);
				pEndNode->SetBackground(CSelectionTree_BaseNode::eSTBS_Blue);
				TBSTNodeList nodesInbetween;
				ok = m_pGraph->IsInHierarchy(pTreeNode, pEndNode, &nodesInbetween);
				assert( ok );
				for (TBSTNodeList::iterator it = nodesInbetween.begin(); it != nodesInbetween.end(); ++it)
					(*it)->SetBackground(CSelectionTree_BaseNode::eSTBS_Blue);

			}
		}
	}
	pEnum->Release();

	// mark path
	TBSTNodeList nodesInbetween;
	bool ok = m_pGraph->IsInHierarchy(m_pMappingEditNode, m_pMappingEndEditNode, &nodesInbetween);
	assert( ok );
	for (TBSTNodeList::iterator it = nodesInbetween.begin(); it != nodesInbetween.end(); ++it)
		(*it)->SetBackground(CSelectionTree_BaseNode::eSTBS_Yellow);

	// mark active node
	m_pMappingEditNode->SetBackground(CSelectionTree_BaseNode::eSTBS_Red);

	// mark end node
	if (m_pMappingEndEditNode != m_pMappingEditNode)
		m_pMappingEndEditNode->SetBackground(CSelectionTree_BaseNode::eSTBS_Yellow);
}

void CLeafMappingEditor::ResetMarkings()
{
	std::list<CSelectionTree_TreeNode*> nodes;
	IHyperGraphEnumerator* pEnum = m_pGraph->GetNodesEnumerator();

	for ( IHyperNode* pHyperNode = pEnum->GetFirst(); pHyperNode != NULL; pHyperNode = pEnum->GetNext() )
	{
		CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( pHyperNode );
		pBaseNode->SetBackground(CSelectionTree_BaseNode::eSTBS_None);
	}
	pEnum->Release();
	m_pGraph->GetGraphView()->InvalidateView(true);
}

void CLeafMappingEditor::LockTree()
{
	TBSTNodeList rootNodes;
	m_pGraph->GetRootNodes( rootNodes );
	for ( TBSTNodeList::iterator it = rootNodes.begin(); it != rootNodes.end(); ++it )
		(*it)->Lock();
}

void CLeafMappingEditor::UnlockTree()
{
	TBSTNodeList rootNodes;
	m_pGraph->GetRootNodes( rootNodes );
	for ( TBSTNodeList::iterator it = rootNodes.begin(); it != rootNodes.end(); ++it )
		(*it)->Unlock();
}
