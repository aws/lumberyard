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
#include "SelectionTreeGraphManager.h"
#include "SelectionTreeHyperGraphView.h"

#include "Nodes/SelectionTree_TreeNode.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "clipboard.h"

IMPLEMENT_DYNAMIC( CSelectionTreeHyperGraphView, CHyperGraphView )

BEGIN_MESSAGE_MAP( CSelectionTreeHyperGraphView, CHyperGraphView )
	ON_COMMAND( ID_UNDO, OnUndo )
	ON_COMMAND( ID_REDO, OnRedo )
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
END_MESSAGE_MAP()


typedef enum
{
	eSTGMC_DoNothing,

	eSTGMC_CreateLeaf,
	eSTGMC_CreatePriority,
	eSTGMC_CreateSequence,
	eSTGMC_CreateStateMachine,

	eSTGMC_Delete,
	eSTGMC_Cut,
	eSTGMC_Copy,
	eSTGMC_Paste,
	eSTGMC_PasteWithoutLinks,

	eSTGMC_FitGraphToView,
	eSTGMC_ToggleReorder,
	eSTGMC_ToggleSmoothReorder,

	eSTGMC_User = 3000,
} SelectionTreeGraphMenuCommands;

bool StringSorter(string a, string b) {return 1;}

CSelectionTreeHyperGraphView::CSelectionTreeHyperGraphView()
: m_NodeOrderMode( eBOM_Auto )
, m_InternalNodeOrderMode( eBOM_Auto )
, m_bCtrlPressed( false )
{
	LoadTemplates();

}

void CSelectionTreeHyperGraphView::LoadTemplates()
{
	XmlNodeRef mbtTemplatesNode = GetISystem()->LoadXmlFromFile("Editor\\MbtTemplates.xml");

	m_TemplateMap.clear();
	m_TemplateMapSorted.clear();
	int startIndex = 1000;
	if(mbtTemplatesNode)
	{
		for (int i = 0; i < mbtTemplatesNode->getChildCount(); i++)
		{
			if(strcmp(mbtTemplatesNode->getChild(i)->getTag(), MBT_LOGGING_NODE) == 0)
				continue;

			m_TemplateMap[startIndex+i] = mbtTemplatesNode->getChild(i)->getTag();
			m_TemplateMapSorted[mbtTemplatesNode->getChild(i)->getTag()] = startIndex+i;
		}
	}
}

CSelectionTreeHyperGraphView::~CSelectionTreeHyperGraphView()
{

}

void CSelectionTreeHyperGraphView::ResetView()
{
	OnCommandFitAll();
}

CSelectionTreeGraph* CSelectionTreeHyperGraphView::GetSelectionTreeGraph()
{
	CHyperGraph* pGraph = GetGraph();
	return static_cast< CSelectionTreeGraph* >( pGraph );
}

const CSelectionTreeGraph* CSelectionTreeHyperGraphView::GetSelectionTreeGraph() const
{
	const CHyperGraph* pGraph = GetGraph();
	return static_cast< const CSelectionTreeGraph* >( pGraph );
}

CSelectionTreeGraphManager* CSelectionTreeHyperGraphView::GetSelectionTreeGraphManager()
{
	CHyperGraph* pGraph = GetGraph();
	if ( pGraph == NULL )
	{
		return NULL;
	}

	CHyperGraphManager* pGraphManager = pGraph->GetManager();
	return static_cast< CSelectionTreeGraphManager* >( pGraphManager );
}

const CSelectionTreeGraphManager* CSelectionTreeHyperGraphView::GetSelectionTreeGraphManager() const
{
	const CHyperGraph* pGraph = GetGraph();
	if ( pGraph == NULL )
	{
		return NULL;
	}

	const CHyperGraphManager* pGraphManager = pGraph->GetManager();
	return static_cast< const CSelectionTreeGraphManager* >( pGraphManager );
}

bool CSelectionTreeHyperGraphView::IsGraphLoaded()
{
	return GetSelectionTreeGraph() != NULL;
}

void CSelectionTreeHyperGraphView::ShowContextMenu( CPoint point, CHyperNode* pNode )
{
	if ( !IsGraphLoaded() )
		return;

	CSelectionTreeGraph* pGraph = GetSelectionTreeGraph();
	if ( pGraph == NULL || pGraph->IsInTranslationEditMode() )
	{
		return;
	}

	CSelectionTreeGraphManager* pGraphManager = GetSelectionTreeGraphManager();
	if ( pGraphManager == NULL )
	{
		return;
	}

	CMenu menu;
	CMenu createNodeMenu;
	menu.CreatePopupMenu();

	std::vector<CHyperNode*> selectedNodes;
	GetSelectedNodes( selectedNodes );
	const bool multiNodesSelected = selectedNodes.size() > 1;
	const bool nodeSelected = ( pNode != NULL );
	if ( nodeSelected )
	{
		UnselectAll();
		pNode->SetSelected( true );
	}

	CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( pNode );

	// Create New Node
	bool bShowCreateMenu = true;
	bool bIsLeafNode = false;
	if ( nodeSelected )
	{
		bShowCreateMenu = false;
		if ( !pBaseNode->IsReadOnly() )
		{
			CSelectionTree_TreeNode* pTreeNode = static_cast< CSelectionTree_TreeNode* >( pBaseNode );
			bIsLeafNode = false;
			bShowCreateMenu = true;
		}
	}

	if ( bShowCreateMenu )
	{
		createNodeMenu.CreatePopupMenu();
		
		for (std::map<string, int>::iterator it = m_TemplateMapSorted.begin(); it != m_TemplateMapSorted.end(); ++it)
		{
			createNodeMenu.AppendMenu( MF_STRING, (*it).second, (*it).first);
		}

		if ( !bIsLeafNode )
		{
			/*createNodeMenu.AppendMenu( MF_STRING, eSTGMC_CreateLeaf, "Leaf" );
			createNodeMenu.AppendMenu( MF_STRING, eSTGMC_CreatePriority, "Priority" );
			createNodeMenu.AppendMenu( MF_STRING, eSTGMC_CreateSequence, "Sequence" );
			createNodeMenu.AppendMenu( MF_STRING, eSTGMC_CreateStateMachine, "StateMachine" );*/
		}
		if ( !bIsLeafNode || nodeSelected )
		{
			if (!bIsLeafNode)
				menu.AppendMenu( MF_POPUP, reinterpret_cast< UINT_PTR >( createNodeMenu.GetSafeHmenu() ), nodeSelected ? "Add Child" : "Create" );
			menu.AppendMenu( MF_SEPARATOR );
		}
	}

	// node specific
	if ( nodeSelected && !multiNodesSelected )
	{
		pNode->PopulateContextMenu( menu, eSTGMC_User );
	}
	if ( multiNodesSelected || ( nodeSelected && !pBaseNode->IsReadOnly() ) )
	{
		// node common
		menu.AppendMenu( MF_STRING, eSTGMC_Delete, multiNodesSelected ? "Delete Nodes" : "Delete Node" );
		menu.AppendMenu( MF_STRING, eSTGMC_Cut, multiNodesSelected ? "Cut Nodes" : "Cut Node" );
		menu.AppendMenu( MF_STRING, eSTGMC_Copy, multiNodesSelected ? "Copy Nodes" : "Copy Node" );
		menu.AppendMenu( MF_SEPARATOR );
	}
	else
	{
		CClipboard clipboard;
		XmlNodeRef node = clipboard.Get();

		if (node != NULL && node->isTag("Graph"))
		{
			if (node->getChildCount() > 0)
			{
				//menu.AppendMenu( MF_STRING, eSTGMC_Paste, "Paste Node(s)" );
				menu.AppendMenu( MF_STRING, eSTGMC_PasteWithoutLinks, "Paste Node" );
				menu.AppendMenu( MF_SEPARATOR );
			}
		}
	}

	// graph
	menu.AppendMenu( MF_STRING, eSTGMC_FitGraphToView, "Fit To View" );
	menu.AppendMenu( MF_SEPARATOR );
	menu.AppendMenu( MF_STRING | ( ( m_NodeOrderMode != eBOM_Disabled ) ? MF_CHECKED : MF_UNCHECKED ), eSTGMC_ToggleReorder, "Automatic Node Ordering" );
	menu.AppendMenu( MF_STRING | ( ( m_InternalNodeOrderMode == eBOM_AutoSmooth ) ? MF_CHECKED : MF_UNCHECKED ), eSTGMC_ToggleSmoothReorder, "Smooth Node Ordering" );

	CPoint screenPoint( point );
	ClientToScreen( &screenPoint );
	const int commandId = menu.TrackPopupMenu( TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY, screenPoint.x, screenPoint.y, this );

	if ( commandId == eSTGMC_DoNothing )
	{
		return;
	}

	if ( commandId == eSTGMC_Cut )
	{
		CutNodesInt();
		return;
	}

	if ( commandId == eSTGMC_Copy )
	{
		CopyNodesInt();
		return;
	}

	if ( commandId == eSTGMC_Paste )
	{
		PasteNodesInt( point );
		return;
	}

	if ( commandId == eSTGMC_PasteWithoutLinks )
	{
		PasteNodesInt( point, false );
		return;
	}

	if ( commandId == eSTGMC_Delete )
	{
		DeleteNodesInt();
		return;
	}

	if ( commandId == eSTGMC_FitGraphToView )
	{
		pGraph->OrderRootNodes();
		OnCommandFitAll();
		return;
	}

	if ( commandId == eSTGMC_ToggleReorder )
	{
		m_NodeOrderMode = m_NodeOrderMode == eBOM_Disabled ? m_InternalNodeOrderMode : eBOM_Disabled;
		ReorderAllNodes();
		return;
	}

	if ( commandId == eSTGMC_ToggleSmoothReorder )
	{
		m_InternalNodeOrderMode = m_InternalNodeOrderMode == eBOM_Auto ? eBOM_AutoSmooth : eBOM_Auto;
		m_NodeOrderMode = m_NodeOrderMode != eBOM_Disabled ? m_InternalNodeOrderMode : eBOM_Disabled;
		ReorderAllNodes();
		return;
	}

	if ( eSTGMC_User <= commandId )
	{
		assert( pNode != NULL );
		string desc = "NOT DEFINED YET!";
		if ( pBaseNode->OnContextMenuCommandEx( commandId - eSTGMC_User, desc ) )
		{
			GetSelectionTreeGraph()->OnUndoEvent( desc );
			assert(desc != "NOT DEFINED YET!");
		}
		return;
	}
	
	if(m_TemplateMap.find(commandId) != m_TemplateMap.end())
	{
		string newName = m_TemplateMap.find(commandId)->second;
		CSelectionTree_TreeNode* pNewNode = static_cast< CSelectionTree_TreeNode* >( GetSelectionTreeGraph()->CreateNewNode( newName ) );
		assert( pNewNode );
		if ( pNewNode )
		{
			pNewNode->SetVisible( false );
			Gdiplus::PointF p = ViewToLocal(point);
			pNewNode->SetPos( p );
			pNewNode->SetVisible( true );

			pNewNode->SetAttributes(GetIEditor()->FindTemplate(newName.c_str()));
			pNewNode->SetName( newName.c_str() );

			if ( nodeSelected )
			{
				GetSelectionTreeGraph()->ConnectNodes( pBaseNode, pNewNode );
			}
			
			UnselectAll();
			pNewNode->SetSelected( true );
			string desc;
			desc.Format("New node created \"%s\"", pNewNode->GetName() );
			GetSelectionTreeGraph()->OnUndoEvent( desc );
			ReorderAllNodes();
		}
		return;
	}

	if ( eSTGMC_CreateLeaf <= commandId && commandId <= eSTGMC_CreateStateMachine )
	{
		CSelectionTree_TreeNode* pNewNode = static_cast< CSelectionTree_TreeNode* >( GetSelectionTreeGraph()->CreateNewNode( "TreeNode" ) );
		assert( pNewNode );
		if ( pNewNode )
		{
			pNewNode->SetVisible( false );
			Gdiplus::PointF p = ViewToLocal(point);
			pNewNode->SetPos( p );
			/*CSelectionTree_TreeNode::ESelectionTreeNodeSubType subType = CSelectionTree_TreeNode::eSTNST_Invalid;
			switch ( commandId )
			{
			case eSTGMC_CreateLeaf:			subType = CSelectionTree_TreeNode::eSTNST_Leaf;							break;
			case eSTGMC_CreatePriority:			subType = CSelectionTree_TreeNode::eSTNST_Priority;			break;
			case eSTGMC_CreateSequence:			subType = CSelectionTree_TreeNode::eSTNST_Sequence;			break;
			case eSTGMC_CreateStateMachine: subType = CSelectionTree_TreeNode::eSTNST_StateMachine;	break;
			}
			pNewNode->SetNodeSubType( subType );*/
			if ( !pNewNode->DefaultEditDialog( true ) )
			{
				GetSelectionTreeGraph()->RemoveNode( pNewNode );
				return;
			}
			pNewNode->SetVisible( true );
			if ( nodeSelected )
			{
				GetSelectionTreeGraph()->ConnectNodes( pBaseNode, pNewNode );
			}
			UnselectAll();
			pNewNode->SetSelected( true );
			string desc;
			desc.Format("New node created \"%s\"", pNewNode->GetName() );
			GetSelectionTreeGraph()->OnUndoEvent( desc );
			ReorderAllNodes();
		}

		return;
	}
}

bool CSelectionTreeHyperGraphView::HandleRenameNode( CHyperNode *pNode )
{
	if ( !pNode || GetSelectionTreeGraph()->IsInTranslationEditMode()) return true;

	// todo: this is very hacky
	CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( pNode );
	string desc = "TODO: UNDO DESC!";
	bool handled = false;
	bool res = pBaseNode->DefaultEditDialog(false, &handled, &desc);
	if ( res )
	{
		assert(desc != "TODO: UNDO DESC!");
		GetSelectionTreeGraph()->OnUndoEvent( desc );
	}
	return handled;
}

void CSelectionTreeHyperGraphView::OnNodesConnected()
{
	GetSelectionTreeGraph()->OnUndoEvent( "Changed Nodes Connection" );
}

void CSelectionTreeHyperGraphView::OnEdgeRemoved()
{
	GetSelectionTreeGraph()->OnUndoEvent( "Removed Nodes Connection" );
}

void CSelectionTreeHyperGraphView::OnNodeRenamed()
{
	GetSelectionTreeGraph()->OnUndoEvent( "Renamed Node" );
}

void CSelectionTreeHyperGraphView::OnUndo()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->Undo();
}

void CSelectionTreeHyperGraphView::OnRedo()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->Redo();
}

void CSelectionTreeHyperGraphView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( !IsGraphLoaded() )
		return;

	// Link nodes when they are dragged over each other
	std::vector<CHyperNode*> selectedNodes;
	std::multimap<int, CHyperNode*> nodes;
	if (GetSelectedNodes( selectedNodes ) && GetNodesInRect( CRect(point.x,point.y,point.x,point.y),nodes ))
	{
		// Only allowed to drag 1 node on top of the other to connect children
		if(selectedNodes.size() == 1 && nodes.size() == 2)
		{
			CSelectionTree_BaseNode* selectedNode = static_cast<CSelectionTree_BaseNode*>(selectedNodes[0]);
			for (std::multimap<int, CHyperNode*>::iterator it = nodes.begin(); it != nodes.end(); it++)
			{
				CSelectionTree_BaseNode* node = static_cast<CSelectionTree_BaseNode*>(it->second);

				if(selectedNode && node && node != selectedNode)
				{
					GetSelectionTreeGraph()->ConnectNodes(node, selectedNode);

					GetSelectionTreeGraph()->OnUndoEvent( "Nodes Re-linked" );
				}
			}			
		}
	}
	//

	ReorderAllNodes();
	GetSelectionTreeGraph()->OnMouseUp();
	CHyperGraphView::OnLButtonUp(nFlags, point);
}

void CSelectionTreeHyperGraphView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ( !IsGraphLoaded() )
		return;
	GetSelectionTreeGraph()->OnMouseDown();

	// Cancel shift clicks
	nFlags &= ~MK_SHIFT;

	CHyperGraphView::OnLButtonDown(nFlags, point);
}

void CSelectionTreeHyperGraphView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( !IsGraphLoaded() )
		return;

	switch (nChar)
	{
	case VK_CONTROL:
		m_bCtrlPressed = true;
		break;
	}
	CHyperGraphView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSelectionTreeHyperGraphView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ( !IsGraphLoaded() )
		return;

	switch (nChar)
	{
	case VK_ESCAPE:
		if (GetSelectionTreeGraph() && GetSelectionTreeGraph()->IsInTranslationEditMode())
			GetSelectionTreeGraph()->StopMappingEditMode(true);
		break;
	case VK_CONTROL:
		m_bCtrlPressed = false;
		break;
	case VK_DELETE:
			DeleteNodesInt();
		break;
	case 'C':
		if ( m_bCtrlPressed )
			CopyNodesInt();
		break;
	case 'X':
		if ( m_bCtrlPressed )
		{
			CutNodesInt();
		}
		break;
	case 'V':
		if ( m_bCtrlPressed )
		{
			CPoint point;
			GetCursorPos(&point);
			ScreenToClient(&point);
			PasteNodesInt(point);
		}
		break;
	case 'Z':
		if ( m_bCtrlPressed )
			OnUndo();
		break;
	case 'Y':
		if ( m_bCtrlPressed )
			OnRedo();
		break;
	}
	CHyperGraphView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CSelectionTreeHyperGraphView::ReorderAllNodes()
{
	std::vector<CHyperNode*> selectedNodes;
	GetSelectedNodes( selectedNodes );
	for ( std::vector<CHyperNode*>::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it  )
		(*it)->SetSelected( false );

	GetSelectionTreeGraph()->InvalidateNodes();

	for ( std::vector<CHyperNode*>::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it  )
		(*it)->SetSelected( true );
}

void CSelectionTreeHyperGraphView::UnselectAll()
{
	std::vector<CHyperNode*> selectedNodes;
	GetSelectedNodes( selectedNodes );
	for ( std::vector<CHyperNode*>::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it  )
		(*it)->SetSelected( false );
}

void CSelectionTreeHyperGraphView::DeleteNodesInt()
{
	if ( CheckForReadOnlyNodes() )
	{
		OnCommandDelete();
		GetSelectionTreeGraph()->OnUndoEvent( "Nodes deleted" );
	}
}

void CSelectionTreeHyperGraphView::CutNodesInt()
{
	if ( CheckForReadOnlyNodes() )
	{
		OnCommandCut();
		GetSelectionTreeGraph()->OnUndoEvent( "Nodes deleted" );
	}
}

void CSelectionTreeHyperGraphView::CopyNodesInt()
{
	OnCommandCopy();
}

void CSelectionTreeHyperGraphView::PasteNodesInt( CPoint point, bool withLinks )
{
	GetSelectionTreeGraph()->SetCreateGraph( true );
	std::vector<CHyperNode*> pastedNodes;
	InternalPaste( withLinks, point, &pastedNodes );

	if (withLinks)
	{
		for ( std::vector<CHyperNode*>::iterator it = pastedNodes.begin(); it != pastedNodes.end(); ++it )
		{
			CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( *it );
			CSelectionTree_BaseNode* pParent = pBaseNode->GetParentNode();
			if ( pParent )
			{
				bool bParentIsNew = false;
				for ( std::vector<CHyperNode*>::iterator pit = pastedNodes.begin(); pit != pastedNodes.end(); ++pit )
				{
					CSelectionTree_BaseNode* pBaseNodeParent = static_cast< CSelectionTree_BaseNode* >( *pit );
					if ( pParent == pBaseNodeParent )
					{
						bParentIsNew = true;
						break;
					}
				}
				if ( !bParentIsNew || pParent->IsReadOnly() )
					GetSelectionTreeGraph()->DisconnectNodes( pParent, pBaseNode, true );
			}
		}
	}
	GetSelectionTreeGraph()->SetCreateGraph( false );
	GetSelectionTreeGraph()->OnUndoEvent( "Nodes Pasted" );
}

bool CSelectionTreeHyperGraphView::CheckForReadOnlyNodes()
{
	std::vector<CHyperNode*> selectedNodes;
	GetSelectedNodes( selectedNodes );
	for ( std::vector<CHyperNode*>::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it  )
	{
		CSelectionTree_BaseNode* pBaseNode = static_cast< CSelectionTree_BaseNode* >( *it );
		if ( pBaseNode->IsReadOnly() )
			return false;
	}
	return true;
}
