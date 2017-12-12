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
#include "SelectionTreeGraphManager.h"

#include "SelectionTreeGraph.h"

#include "Nodes/SelectionTree_TreeNode.h"
#include "Nodes/SelectionTree_StateMachineNode.h"
#include "Nodes/SelectionTree_StateNode.h"

CSelectionTreeGraphManager::CSelectionTreeGraphManager()
{
	ReloadClasses();
}


CSelectionTreeGraphManager::~CSelectionTreeGraphManager()
{

}


void CSelectionTreeGraphManager::Init()
{
	__super::Init();

	ReloadClasses();
}


void CSelectionTreeGraphManager::ReloadClasses()
{
	ClearPrototypes();

	AddNodePrototype( new CSelectionTree_TreeNode() );
	AddNodePrototype( new CSelectionTree_StateMachineNode() );
	AddNodePrototype( new CSelectionTree_StateNode() );
}


CHyperNode* CSelectionTreeGraphManager::CreateNode( CHyperGraph* pGraph, const char* nodeClass, HyperNodeID nodeId, const Gdiplus::PointF& position, CBaseObject* pObj )
{
	CHyperNode* pNewNode = __super::CreateNode( pGraph, nodeClass, nodeId, position, pObj );
	return pNewNode;
}

CHyperGraph* CSelectionTreeGraphManager::CreateGraph()
{
	CSelectionTreeGraph* pSelectionTreeGraph = new CSelectionTreeGraph( this );
	return pSelectionTreeGraph;
}

void CSelectionTreeGraphManager::ClearPrototypes()
{
	m_prototypes.clear();
}

void CSelectionTreeGraphManager::AddNodePrototype( _smart_ptr< CSelectionTree_BaseNode > pPrototypeNode )
{
	assert( pPrototypeNode.get() != NULL );
	if ( pPrototypeNode.get() == NULL )
	{
		return;
	}

	const char* nodeClassName = pPrototypeNode->GetClassName();
	assert( nodeClassName != NULL );
	if ( nodeClassName == NULL )
	{
		return;
	}
	string prototypeName( nodeClassName );

	m_prototypes[ prototypeName ] = pPrototypeNode;
}