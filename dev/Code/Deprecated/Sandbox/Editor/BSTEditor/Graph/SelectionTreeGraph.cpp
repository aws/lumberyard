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
#include "SelectionTreeGraph.h"

#include "SelectionTreeEdge.h"
#include "SelectionTreeGraphManager.h"
#include "SelectionTreeLeafMapping.h"

#include "BSTEditor/Views/SelectionTreeGraphView.h"

#include "Nodes/SelectionTree_TreeNode.h"

#include "BSTEditor/SelectionTreeManager.h"

CSelectionTreeGraph::CSelectionTreeGraph( CSelectionTreeGraphManager* pSelectionTreeGraphManager )
: CHyperGraph( pSelectionTreeGraphManager )
, m_iCreateGraphCount( 0 )
, m_bRecordUndoOnDrop( false )
, m_pGraphView( NULL )
, m_bClearGraph( false )
, m_bForceOrder( false )
, m_bLockSelected( false )
{
	m_pLeafTranslationMan = new CLeafMappingManager(this);
}

CSelectionTreeGraph::~CSelectionTreeGraph()
{
	delete m_pLeafTranslationMan;
}

void CSelectionTreeGraph::ClearGraph()
{
	StopMappingEditMode();
	m_pLeafTranslationMan->ClearAll();
	ClearAll();
}

void CSelectionTreeGraph::ClearAll()
{
	m_bClearGraph = true;
	SetCreateGraph( true );
	__super::ClearAll();
	SetCreateGraph( false );
	m_bClearGraph = false;
}

void CSelectionTreeGraph::OnUndoEvent( const char* desc, bool bRecordOnDrop )
{
	if ( !IsCreatingGraph() )
	{
		StopMappingEditMode();
		if ( bRecordOnDrop )
		{
			m_bRecordUndoOnDrop = true;
			m_sUndoOnDropDesc = desc;
		}
		else
		{
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc );
		}
	}
}

void CSelectionTreeGraph::OnMouseUp()
{
	m_bLockSelected = false;
	if ( m_bRecordUndoOnDrop )
	{
		OnUndoEvent( m_sUndoOnDropDesc );
		m_sUndoOnDropDesc = "UNKNOWN";
		m_bRecordUndoOnDrop = false;
	}
}


void CSelectionTreeGraph::OnMouseDown()
{
	m_bLockSelected = true;
}

///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Edge ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

CHyperEdge* CSelectionTreeGraph::CreateEdge()
{
	return new CSelectionTreeEdge();
}

void CSelectionTreeGraph::RegisterEdge( CHyperEdge* pEdge, bool fromSpecialDrag )
{
	CHyperGraph::RegisterEdge( pEdge, fromSpecialDrag );

	CSelectionTreeEdge* pSelectionTreeEdge = static_cast< CSelectionTreeEdge* >( pEdge );

	HyperNodeID nodeInId = pEdge->nodeIn;
	IHyperNode* pNodeIn = FindNode( nodeInId );
	CSelectionTree_BaseNode* pSelectionTreeNodeIn = static_cast< CSelectionTree_BaseNode* >( pNodeIn );

	pSelectionTreeEdge->SetConditionProvider( pSelectionTreeNodeIn );
	UpdateTranslations();
}

void CSelectionTreeGraph::RemoveEdge( CHyperEdge* pEdge )
{
	RemoveEdgeInt(pEdge);
	return;
}

void CSelectionTreeGraph::RemoveEdgeInt( CHyperEdge* pEdge, bool bForce )
{
	HyperNodeID nodeOutId = pEdge->nodeOut;
	CSelectionTree_BaseNode* pParentNode = static_cast< CSelectionTree_BaseNode* >( FindNode( nodeOutId ) );

	HyperNodeID nodeInId = pEdge->nodeIn;
	CSelectionTree_BaseNode* pChildNode = static_cast< CSelectionTree_BaseNode* >( FindNode( nodeInId ) );

	if ( !bForce && ( (pParentNode && pParentNode->IsReadOnly()) || (pChildNode && pChildNode->IsReadOnly()) ) )
		return;

	CHyperGraph::RemoveEdge( pEdge );

	if ( pChildNode )
		pChildNode->SetParentNode( NULL );
	UpdateTranslations();
}

void CSelectionTreeGraph::RemoveEdgeSilent( CHyperEdge* pEdge )
{
	HyperNodeID nodeOutId = pEdge->nodeOut;
	CSelectionTree_BaseNode* pParentNode = static_cast< CSelectionTree_BaseNode* >( FindNode( nodeOutId ) );

	HyperNodeID nodeInId = pEdge->nodeIn;
	CSelectionTree_BaseNode* pChildNode = static_cast< CSelectionTree_BaseNode* >( FindNode( nodeInId ) );

	CHyperGraph::RemoveEdgeSilent( pEdge );

	if ( pChildNode )
		pChildNode->SetParentNode( NULL );
	UpdateTranslations();
}

CHyperEdge* CSelectionTreeGraph::MakeEdge( CHyperNode* pNodeOut, CHyperNodePort* pPortOut, CHyperNode* pNodeIn, CHyperNodePort* pPortIn, bool bEnabled, bool fromSpecialDrag )
{
	CHyperEdge* pEdge = CHyperGraph::MakeEdge( pNodeOut, pPortOut, pNodeIn, pPortIn, bEnabled, fromSpecialDrag );

	CSelectionTree_BaseNode* pParentNode = static_cast< CSelectionTree_BaseNode* >( pNodeOut );
	CSelectionTree_BaseNode* pChildNode = static_cast< CSelectionTree_BaseNode* >( pNodeIn );
	pChildNode->SetParentNode( pParentNode );

	UpdateTranslations();
	return pEdge;
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Connections //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void CSelectionTreeGraph::ConnectNodes( CSelectionTree_BaseNode* pNodeFrom, CSelectionTree_BaseNode* pNodeTo )
{
	assert( pNodeFrom && pNodeFrom );
	if ( !pNodeFrom || !pNodeFrom ) return;

	CHyperNodePort* pPortFrom = pNodeFrom->FindPort( "", false );
	CHyperNodePort* pPortTo = pNodeTo->FindPort( "", true );
	const bool connectionSuccess = ConnectPorts( pNodeFrom, pPortFrom, pNodeTo, pPortTo, false );
	//assert( connectionSuccess );
}

void CSelectionTreeGraph::DisconnectNodes( CSelectionTree_BaseNode* pNodeFrom, CSelectionTree_BaseNode* pNodeTo, bool bForce )
{
	std::vector< CHyperEdge* > edges;
	GetAllEdges( edges );
	for ( std::vector< CHyperEdge* >::iterator it = edges.begin(); it != edges.end(); ++it )
	{
		if ( (*it)->nodeOut == pNodeFrom->GetId() && (*it)->nodeIn == pNodeTo->GetId() )
			RemoveEdgeInt( *it, bForce );
	}
}

void CSelectionTreeGraph::DisconnectAllChilds( CSelectionTree_BaseNode* pNodeFrom )
{
	std::vector< CHyperEdge* > edges;
	GetAllEdges( edges );
	for ( std::vector< CHyperEdge* >::iterator it = edges.begin(); it != edges.end(); ++it )
	{
		if ( (*it)->nodeOut == pNodeFrom->GetId() )
			RemoveEdge( *it );
	}
}

bool CSelectionTreeGraph::CanConnectPorts( CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge *pExistingEdge )
{
	if ( !CHyperGraph::CanConnectPorts( pSrcNode, pSrcPort, pTrgNode, pTrgPort, pExistingEdge ) )
		return false;

	CSelectionTree_BaseNode* pSelectionTreeSrcNode = static_cast< CSelectionTree_BaseNode* >( pSrcNode );
	CSelectionTree_BaseNode* pSelectionTreeTrgNode = static_cast< CSelectionTree_BaseNode* >( pTrgNode );

	if ( !IsCreatingGraph() && !pSelectionTreeSrcNode->AcceptChild( pSelectionTreeTrgNode ) )
		return false;

	if ( IsInHierarchy( pSrcNode, pTrgNode ) )
		return false;

	return true;
}

bool CSelectionTreeGraph::IsInHierarchy( const CHyperNode* pNodeToSearchFor, CHyperNode* pNode, TBSTNodeList* pNodesInbetween ) const
{
	if ( pNode == NULL )
		return false;

	if ( pNode == pNodeToSearchFor )
		return true;

	bool isInHierarchy = false;

	CSelectionTree_BaseNode* pSelectionTreeNode = static_cast< CSelectionTree_BaseNode* >( pNode );
	for ( size_t i = 0; i < pSelectionTreeNode->GetChildNodeCount() && ! isInHierarchy; ++i )
	{
		CSelectionTree_BaseNode* pChild = pSelectionTreeNode->GetChildNode( i );
		bool found = IsInHierarchy( pNodeToSearchFor, pChild, pNodesInbetween );
		if (found && pNodesInbetween && pChild != pNodeToSearchFor) pNodesInbetween->push_back(pChild);
		isInHierarchy |= found;
	}

	return isInHierarchy;
}
///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Nodes //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void CSelectionTreeGraph::RemoveNode( IHyperNode *pNode )
{
	__super::RemoveNode( pNode );
}

CSelectionTree_BaseNode* CSelectionTreeGraph::CreateRootNode( const XmlNodeRef& xmlNode )
{
	CSelectionTree_BaseNode* rootNode = CreateNewNode( xmlNode );
	assert( rootNode && rootNode->IsRoot() );

	if  (  !rootNode 
		|| !rootNode->IsRoot() )
	{
		ClearAll();
		rootNode = NULL;
	}

	return rootNode;
}

CSelectionTree_BaseNode* CSelectionTreeGraph::CreateNewNode( const XmlNodeRef& xmlNode )
{
	CSelectionTree_BaseNode* pNewNode = NULL;

	const char* tag = xmlNode->getTag();
	XmlNodeRef templateNode = GetIEditor()->FindTemplate(tag);
	assert(templateNode && "template not found in MbtTemplates.xml");
	if(templateNode)
	{
		const char* className;
		templateNode->getAttr("class", &className);
		Gdiplus::PointF position( 0, 0 );
		pNewNode = static_cast<CSelectionTree_BaseNode*> (CreateNode(className, position));
	}
	
	if ( pNewNode )
	{
		if ( !pNewNode->LoadFromXml( xmlNode ) )
		{
			RemoveNode( pNewNode );
			pNewNode = NULL;
		}
	}

	return pNewNode;
}

CSelectionTree_BaseNode* CSelectionTreeGraph::CreateNewNode( string nodeName )
{
	CSelectionTree_BaseNode* pNewNode = NULL;

	Gdiplus::PointF position( 0, 0 );
	XmlNodeRef templateNode = GetIEditor()->FindTemplate(nodeName.c_str());
	if(templateNode)
	{
		const char* className;
		templateNode->getAttr("class", &className);
		pNewNode = static_cast<CSelectionTree_BaseNode*>(CreateNode(className, position));
	}
	return  pNewNode;
}

CSelectionTree_BaseNode* CSelectionTreeGraph::GetRootNode()
{
	TBSTNodeList rootNodeList;
	GetRootNodes( rootNodeList );
	return rootNodeList.size() > 0 ? *rootNodeList.begin() : NULL;
}

void CSelectionTreeGraph::GetRootNodes( TBSTNodeList& rootNodeList )
{
	IHyperGraphEnumerator* pEnum = GetNodesEnumerator();

	for ( IHyperNode* pNode = pEnum->GetFirst(); pNode != NULL; pNode = pEnum->GetNext() )
	{
		CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( pNode );
		if ( pBaseNode->IsRoot() )
		{
			rootNodeList.push_back( pBaseNode );
		}
	}
	pEnum->Release();
}

bool CSelectionTreeGraph::IsReorderEnabled() const
{
	return GetGraphView()->GetOrderMode() != eBOM_Disabled || m_bForceOrder;
}

bool CSelectionTreeGraph::IsReorderSmooth() const
{
	return GetGraphView()->GetOrderMode() == eBOM_AutoSmooth && !m_bForceOrder;
}


void CSelectionTreeGraph::SetCreateGraph( bool bCreate )
{
	if ( bCreate )
		m_iCreateGraphCount++;
	else
		m_iCreateGraphCount--;

	assert( m_iCreateGraphCount >= 0 );
}

bool CSelectionTreeGraph::CheckNodeMapping(CSelectionTree_TreeNode* pNode, string& translatedBehavior, int& translationId ) const
{
	return m_pLeafTranslationMan->CheckNode(pNode, translatedBehavior, translationId);
}


void CSelectionTreeGraph::UpdateTranslations()
{
	if (IsCreatingGraph()) return;
	TBSTNodeList rootNodes;
	GetRootNodes( rootNodes );
	for ( TBSTNodeList::iterator it = rootNodes.begin(); it != rootNodes.end(); ++it )
		(*it)->UpdateTranslations();
	InvalidateNodes();
}

bool CSelectionTreeGraph::NodeClicked(CSelectionTree_BaseNode* pNode)
{
	GetIEditor()->GetSelectionTreeManager()->GetPropertiesView()->OnNodeClicked(pNode);

	if (m_pLeafTranslationMan->GetEditor()->IsInEditMode())
	{
		m_pLeafTranslationMan->GetEditor()->ChangeEndNode(pNode);
		return true;
	}
	return false;
}

void CSelectionTreeGraph::StartMappingEditMode(CSelectionTree_TreeNode* pNode)
{
	m_pLeafTranslationMan->GetEditor()->StartEdit(pNode);
}

void CSelectionTreeGraph::StopMappingEditMode(bool abord)
{
	m_pLeafTranslationMan->GetEditor()->StopEdit(abord);
}

bool CSelectionTreeGraph::IsInTranslationEditMode() const
{
	return m_pLeafTranslationMan->GetEditor()->IsInEditMode();
}

void CSelectionTreeGraph::InvalidateNodes()
{
	TBSTNodeList rootNodes;
	GetRootNodes( rootNodes );
	for ( TBSTNodeList::iterator it = rootNodes.begin(); it != rootNodes.end(); ++it )
	{
		(*it)->InvalidateNodePos();
	}
}

void CSelectionTreeGraph::OrderRootNodes()
{
	m_bForceOrder = true;
	CSelectionTree_BaseNode* pRootNode = GetRootNode();
	if ( pRootNode )
	{
		TBSTNodeList rootNodes;
		GetRootNodes( rootNodes );
		pRootNode->InvalidateNodePos();
		int py = pRootNode->GetPos().Y + pRootNode->GetTreeHeight() + 250.f;
		int px = pRootNode->GetPos().X;
		for ( TBSTNodeList::iterator it = rootNodes.begin(); it != rootNodes.end(); ++it )
		{
			if ( *it != pRootNode )
			{
				(*it)->SetPos( Gdiplus::PointF( px, py ) );
				(*it)->InvalidateNodePos( true );
				py += (*it)->GetTreeHeight() + 250.f;
			}
		}
	}
	m_bForceOrder = false;
}

///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// Serialization //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

bool CSelectionTreeGraph::LoadFromXml( const XmlNodeRef& node )
{
	ClearAll();
	SetCreateGraph( true );

	CSelectionTree_BaseNode* pRootNode = CreateRootNode( node );

	if ( pRootNode )
	{
		DeleteHelperNodes();
	}
	SetCreateGraph( false );
	UpdateTranslations();
	OrderRootNodes();
	GetGraphView()->InvalidateView(true);
	return pRootNode != NULL;
}

bool CSelectionTreeGraph::SaveToXml( XmlNodeRef& xmlNode )
{
	// todo:
	// at least the editor should forbid to create a case where the root node is a condition without a child!!
	// we should also choose a better new root node on delete the current root node instead takeing the first found root node
	// see RemoveNode( IHyperNode *pNode )
	// we also should forbid to delete the root node if there is no other node at all

	CSelectionTree_BaseNode* pRootNode = GetRootNode();
	if ( !pRootNode )
	{
		string name = "Root";
		CSelectionTree_TreeNode* pNewRootNode = static_cast< CSelectionTree_TreeNode* >( CreateNewNode( name ) );
		pNewRootNode->SetName(name.c_str());
		pRootNode = pNewRootNode;
	}

	if ( pRootNode )
	{
		SetCreateGraph( true );

		// michiel floating nodes make helpernodes crash - investigate
		//CreateHelperNodes();
		const bool res = pRootNode->SaveToXml( xmlNode );
		//DeleteHelperNodes();

		SetCreateGraph( false );
		return res;
	}
	return false;
}

bool CSelectionTreeGraph::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	return LoadFromXml( xmlNode );
}

void CSelectionTreeGraph::CreateHelperNodes()
{
	// to validate the tree before serialization, we attach all root nodes to the one and only m_pRootNode
	// we will add a condition that is always false, so it should not affect the tree when executed
	// we also add an empty priority node to all condition nodes that have no child
	CSelectionTree_BaseNode* pRootNode = GetRootNode();
	if ( pRootNode )
	{
	/*	IHyperGraphEnumerator* pEnum = GetNodesEnumerator();
		std::list< IHyperNode* > nodeList;
		for ( IHyperNode* pNode = pEnum->GetFirst(); pNode != NULL; pNode = pEnum->GetNext() )
			nodeList.push_back( pNode );

		CSelectionTree_ConditionNode* pHelperCondition = static_cast< CSelectionTree_ConditionNode* >( CreateNewNode( CSelectionTree_BaseNode::eSTNT_Condition ) );
		pHelperCondition->SetConditionText(INVALID_HELPER_NODE);

		CSelectionTree_TreeNode* pHelperRoot = static_cast< CSelectionTree_TreeNode* >( CreateNewNode( CSelectionTree_BaseNode::eSTNT_TreeNode ) );
		pHelperRoot->SetNodeSubType( CSelectionTree_TreeNode::eSTNST_Priority );
		pHelperRoot->SetName( INVALID_HELPER_NODE );

		bool bNeedHelper = false;
		for ( std::list< IHyperNode* >::iterator it = nodeList.begin(); it != nodeList.end(); ++it )
		{
			CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( *it );

			if ( pBaseNode != pRootNode && pBaseNode->IsRoot() )
			{
				ConnectNodes( pHelperRoot, pBaseNode );
				bNeedHelper = true;
			}

			if ( pBaseNode->GetNodeType() == CSelectionTree_BaseNode::eSTNT_Condition && pBaseNode->GetChildNodeCount() == 0 )
			{
				CSelectionTree_TreeNode* pHelperNode = static_cast< CSelectionTree_TreeNode* >( CreateNewNode( CSelectionTree_BaseNode::eSTNT_TreeNode ) );
				pHelperNode->SetNodeSubType( CSelectionTree_TreeNode::eSTNST_Priority );
				pHelperNode->SetName( INVALID_HELPER_NODE );
				ConnectNodes( pBaseNode, pHelperNode );
			}
		}
		if ( bNeedHelper )
		{
			ConnectNodes( pRootNode->GetNodeType() != CSelectionTree_BaseNode::eSTNT_Condition ? pRootNode : pRootNode->GetChildNode(0), pHelperCondition );
			ConnectNodes( pHelperCondition, pHelperRoot );
		}
		else
		{
			RemoveNode( pHelperCondition );
			RemoveNode( pHelperRoot );
		}*/
	}
}

void CSelectionTreeGraph::DeleteHelperNodes()
{
	// simply remove all helper nodes after loading
	IHyperGraphEnumerator* pEnum = GetNodesEnumerator();
	std::list< IHyperNode* > removeList;
	for ( IHyperNode* pNode = pEnum->GetFirst(); pNode != NULL; pNode = pEnum->GetNext() )
	{
		if ( strcmpi( pNode->GetName(), INVALID_HELPER_NODE ) == 0 )
			removeList.push_back( pNode );
	}
	pEnum->Release();

	for ( std::list< IHyperNode* >::iterator it = removeList.begin(); it != removeList.end(); ++it )
		RemoveNode( *it );
}

bool CSelectionTreeGraph::RemoveHelperNodesFromRef(CSelectionTree_BaseNode* pNode)
{
	assert( pNode );
	if (!pNode) return false;

	bool ok = true;
	for (int i = 0; i < pNode->GetChildNodeCount(); ++i)
	{
		CSelectionTree_BaseNode* pChild = pNode->GetChildNode(i);
		if ( strcmpi( pChild->GetName(), INVALID_HELPER_NODE ) == 0 )
		{
			pChild->RemoveFromGraph();
			i = 0;
			ok = false;
		}
		else
		{
			ok &= RemoveHelperNodesFromRef(pChild);
		}
	}
	return ok;
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Debugging ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

bool CSelectionTreeGraph::InitDebugTree(const ISelectionTreeObserver::STreeNodeInfo& nodeInfo)
{
	m_NodeIds.clear();
	TBSTNodeList rootNodes;
	GetRootNodes( rootNodes );
	if (rootNodes.size() == 1)
	{
		return ReadNodeIds(*rootNodes.begin(), nodeInfo);
	}
	return false;
}

bool CSelectionTreeGraph::ReadNodeIds(CSelectionTree_BaseNode* pNode, const ISelectionTreeObserver::STreeNodeInfo& info)
{
	m_NodeIds[ info.Id ] = pNode;
	const int childCount = pNode->GetChildNodeCount();
	if (childCount == info.Childs.size())
	{
		bool ok = true;
		for (int i = 0; i < childCount; ++i)
		{
			CSelectionTree_BaseNode* pChild = pNode->GetChildNode(i);
			/*while (pChild && pChild->GetNodeType() != CSelectionTree_BaseNode::eSTNT_TreeNode)
				pChild = pChild->GetChildNode(0);*/
			if (!pChild) return false;
			ok &= ReadNodeIds(pChild, info.Childs[i]);
		}
		return ok;
	}
	return false;
}

void CSelectionTreeGraph::StartEval()
{
	UnmarkNodes();
}

void CSelectionTreeGraph::EvalNode(uint16 nodeId)
{
	TNodeIds::iterator it = m_NodeIds.find(nodeId);
	assert( it != m_NodeIds.end() );
	if (it != m_NodeIds.end())
	{
		it->second->SetBackground(CSelectionTree_BaseNode::eSTBS_Blue);
	}
}

void CSelectionTreeGraph::EvalNodeCondition(uint16 nodeId, bool condition)
{
	TNodeIds::iterator it = m_NodeIds.find(nodeId);
	assert( it != m_NodeIds.end() );
	if (it != m_NodeIds.end())
	{
		CSelectionTree_BaseNode* pCondition = it->second->GetParentNode();
		//if (pCondition && pCondition->GetNodeType() == CSelectionTree_BaseNode::eSTNT_Condition)
		//	pCondition->SetBackground(condition ? CSelectionTree_BaseNode::eSTBS_Green : CSelectionTree_BaseNode::eSTBS_Yellow);
	}
}

void CSelectionTreeGraph::EvalStateCondition(uint16 nodeId, bool condition)
{
}

void CSelectionTreeGraph::StopEval(uint16 nodeId)
{
	TNodeIds::iterator it = m_NodeIds.find(nodeId);
	assert( it != m_NodeIds.end() );
	if (it != m_NodeIds.end())
	{
		it->second->SetBackground(CSelectionTree_BaseNode::eSTBS_Red);
		if (GetIEditor()->GetSelectionTreeManager()->CV_bst_debug_centernode > 0)
			GetGraphView()->CenterAroundNode(it->second);
	}
}

void CSelectionTreeGraph::UnmarkNodes()
{
	std::list<CSelectionTree_TreeNode*> nodes;
	IHyperGraphEnumerator* pEnum = GetNodesEnumerator();

	for ( IHyperNode* pHyperNode = pEnum->GetFirst(); pHyperNode != NULL; pHyperNode = pEnum->GetNext() )
	{
		CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( pHyperNode );
		pBaseNode->SetBackground(CSelectionTree_BaseNode::eSTBS_None);
	}
	pEnum->Release();
}

void CSelectionTreeGraph::RecordUndo( const char* desc )
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RecordUndo( this, desc );
}
