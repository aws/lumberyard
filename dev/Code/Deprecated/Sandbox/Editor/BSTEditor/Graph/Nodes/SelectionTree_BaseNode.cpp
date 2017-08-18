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
#include "SelectionTree_BaseNode.h"
#include "BSTEditor/Graph/SelectionTreeGraph.h"
#include "BSTEditor/Views/SelectionTreeGraphView.h"

CSelectionTree_BaseNode_Painter CSelectionTree_BaseNode::ms_painter;

CSelectionTree_BaseNode::CSelectionTree_BaseNode( )
: m_pParent( NULL )
, m_iOrder(0)
, m_fTreeWidth( 0 )
, m_fTreeHeight( 0 )
, m_fNodeWidth( 100.f )
, m_fNodeHeight( 150.f )
, m_bIsVisible( true )
, m_bReadOnly(false)
, m_Background(eSTBS_None)
, m_fLerp( 0 )
, m_bRegistered( false )
{
	SetClass( "Base" );
	SetName( "Unnamed" );

	SetPainter( &ms_painter );
}

CSelectionTree_BaseNode::~CSelectionTree_BaseNode() 
{
	if ( !GetTreeGraph()->IsCreatingGraph() )
	{
		if (m_pParent) m_pParent->RemoveChildNode(this);
		ClearChildNodes();
	}
	m_children.clear();
	if (m_bRegistered) GetIEditor()->UnregisterNotifyListener( this );
}

void CSelectionTree_BaseNode::Init()
{

}

void CSelectionTree_BaseNode::Done()
{

}

CHyperNode* CSelectionTree_BaseNode::Clone()
{
	assert( false );
	return NULL;
}

void CSelectionTree_BaseNode::CreateDefaultInPort( const PortConnectionType connectionType )
{
	CreateDefaultPort( "in", ePT_Input, connectionType );
}


void CSelectionTree_BaseNode::CreateDefaultOutPort( const PortConnectionType connectionType )
{
	CreateDefaultPort( "out", ePT_Output, connectionType );
}

void CSelectionTree_BaseNode::SetSelected( bool bSelected )
{
	if (GetTreeGraph() && bSelected && !GetTreeGraph()->IsClearGraph() )
	{
		if (GetTreeGraph()->NodeClicked(this))
			return;
	}
	CHyperNode::SetSelected(bSelected);
}

CSelectionTreeGraph* CSelectionTree_BaseNode::GetTreeGraph()
{
	return static_cast< CSelectionTreeGraph* >( GetGraph() );
}

void CSelectionTree_BaseNode::CreateDefaultPort( const CString& portName, const PortType portType, const PortConnectionType connectionType )
{
	const bool isInputPort = ( portType == ePT_Input );

	CHyperNodePort* pPort = FindPort( portName, isInputPort );
	bool portAlreadyCreated = ( pPort != NULL );

	assert( ! portAlreadyCreated );
	if ( portAlreadyCreated )
	{
		return;
	}

	if ( connectionType == ePCT_NoConnectionAllowed )
	{
		return;
	}

	IVariablePtr pVar = new CVariableVoid();
	pVar->SetHumanName( portName );
	pVar->SetName( "" );
	CHyperNodePort port( pVar, isInputPort );
	port.bAllowMulti = ( connectionType == ePCT_MultiConnectionAllowed );
	AddPort( port );
}

void CSelectionTree_BaseNode::SetParentNode( CSelectionTree_BaseNode* pParent )
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	if ( !GetTreeGraph() || (IsReadOnly() && !GetTreeGraph()->IsCreatingGraph()) )
	{
		assert( false );
		return;
	}

	if ( m_pParent == pParent )
	{
		return;
	}

	if ( m_pParent != NULL )
	{
		m_pParent->InvalidateNodePos();
		m_pParent->RemoveChildNode( this );
	}

	m_pParent = pParent;

	if ( m_pParent != NULL )
	{
		m_pParent->AddChildNode( this );
	}

	InvalidateNodePos();
}

CSelectionTree_BaseNode* CSelectionTree_BaseNode::GetParentNode()
{
	return m_pParent;
}

const CSelectionTree_BaseNode* CSelectionTree_BaseNode::GetParentNode() const
{
	return m_pParent;
}

void CSelectionTree_BaseNode::RemoveChildNode( CSelectionTree_BaseNode* pChild )
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	if ( IsReadOnly() && !GetTreeGraph()->IsCreatingGraph() )
	{
		assert( false );
		return;
	}

	assert( pChild != NULL );
	if ( pChild == NULL )
	{
		return;
	}

	ChildrenList::iterator it = std::find( m_children.begin(), m_children.end(), pChild );
	const bool childFound = ( it != m_children.end() );
	if ( ! childFound )
	{
		return;
	}

	m_children.erase( it );
	pChild->SetParentNode( NULL );
	UpdateChildOrder();
}

void CSelectionTree_BaseNode::AddChildNode( CSelectionTree_BaseNode* pChild )
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	assert( pChild != NULL );
	if ( pChild == NULL )
	{
		return;
	}

	ChildrenList::iterator it = std::find( m_children.begin(), m_children.end(), pChild );
	const bool childFound = ( it != m_children.end() );
	if ( childFound )
	{
		return;
	}

	m_children.push_back( pChild );
	pChild->SetParentNode( this );
	UpdateChildOrder();
}

size_t CSelectionTree_BaseNode::GetChildNodeCount() const
{
	return m_children.size();
}

CSelectionTree_BaseNode* CSelectionTree_BaseNode::GetChildNode( const size_t childIndex )
{
	assert( childIndex < GetChildNodeCount() );
	return childIndex < GetChildNodeCount() ? m_children[ childIndex ] : NULL;
}

const CSelectionTree_BaseNode* CSelectionTree_BaseNode::GetChildNode( const size_t childIndex ) const
{
	assert( childIndex < GetChildNodeCount() );
	return childIndex < GetChildNodeCount() ? m_children[ childIndex ] : NULL;
}

void CSelectionTree_BaseNode::ClearChildNodes()
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	ChildrenList childrenCopy = m_children;
	for ( size_t i = 0; i < childrenCopy.size(); ++i )
	{
		CSelectionTree_BaseNode* pChild = childrenCopy[ i ];
		pChild->SetParentNode( NULL );
	}
}

void CSelectionTree_BaseNode::UpdateChildOrder()
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	if ( IsReadOnly() && !GetTreeGraph()->IsCreatingGraph() )
	{
		assert( false );
		return;
	}

	if ( m_children.size() == 0 )
	{
		return;
	}

	struct ChildOrderPred
	{
		bool operator () ( CHyperNode* lhs, CHyperNode* rhs ) const
		{
			return ( lhs->GetPos().X < rhs->GetPos().X );
		}
	};

	ChildrenList listBefore = m_children;
	std::sort( m_children.begin(), m_children.end(), ChildOrderPred() );

	for ( size_t i = 0; i < m_children.size(); ++i )
	{
		CSelectionTree_BaseNode* pChildNode = static_cast< CSelectionTree_BaseNode* >( m_children[ i ] );
		pChildNode->m_iOrder = i;
		pChildNode->Invalidate( true );
	}

	if ( listBefore != m_children && !GetTreeGraph()->IsCreatingGraph() )
	{
		InvalidateNodePos();
		string desc;
		desc.Format("Child order modified \"%s\"", GetName() );

		GetTreeGraph()->OnUndoEvent( desc, true );
	}
}

void CSelectionTree_BaseNode::SetPos( Gdiplus::PointF pos )
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	CHyperNode::SetPos( pos );

	if ( GetTreeGraph() && !GetTreeGraph()->IsCreatingGraph() && !IsReadOnly() )
	{
		if ( m_pParent != NULL )
		{
			m_pParent->UpdateChildOrder();
		}
	}
}

void CSelectionTree_BaseNode::InvalidateNodePos( bool bForceNotSmooth )
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	if ( GetTreeGraph() && !GetTreeGraph()->IsCreatingGraph() )
	{
		if ( m_pParent )
		{
			m_pParent->InvalidateNodePos();
		}
		else
		{
			RecalculateTreeSize();
			Gdiplus::PointF pos = GetPos();
			UpdatePosition( GetTreeGraph()->IsReorderSmooth() && !bForceNotSmooth , pos.X, pos.Y );
			GetTreeGraph()->GetGraphView()->Invalidate();
		}
		Invalidate( true );
	}
}

void CSelectionTree_BaseNode::SetTreeReadOnly( bool bReadOnly )
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	SetReadOnly( bReadOnly );
	for ( int i = 0; i < GetChildNodeCount(); ++i )
		GetChildNode( i )->SetTreeReadOnly( bReadOnly );

	Invalidate( true );
}

void CSelectionTree_BaseNode::Lock()
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	SetReadOnly( true );
	for ( int i = 0; i < GetChildNodeCount(); ++i )
		GetChildNode( i )->Lock();

	Invalidate( true );
}

void CSelectionTree_BaseNode::Unlock()
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	SetReadOnly( false );
	for ( int i = 0; i < GetChildNodeCount(); ++i )
		GetChildNode( i )->Unlock();

	Invalidate( true );
}

void CSelectionTree_BaseNode::UpdateTranslations()
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	for ( int i = 0; i < GetChildNodeCount(); ++i )
		GetChildNode( i )->UpdateTranslations();
}

void CSelectionTree_BaseNode::RemoveFromGraph()
{
	if ( GetTreeGraph()->IsClearGraph() ) return;

	ChildrenList childrenCopy = m_children;
	for ( size_t i = 0; i < childrenCopy.size(); ++i )
	{
		CSelectionTree_BaseNode* pChild = childrenCopy[ i ];
		pChild->RemoveFromGraph();
	}

	if ( m_pParent )
		m_pParent->RemoveChildNode( this );

	GetTreeGraph()->RemoveNode( this );
}

bool CSelectionTree_BaseNode::LoadFromXml( const XmlNodeRef& xmlNode )
{
	assert( false );
	return LoadChildrenFromXml( xmlNode );
}

bool CSelectionTree_BaseNode::SaveToXml( XmlNodeRef& xmlNode )
{
	assert( false );
	return SaveChildrenToXml( xmlNode );
}

bool CSelectionTree_BaseNode::LoadChildrenFromXml( const XmlNodeRef& xmlNode )
{
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );
		CSelectionTree_BaseNode* pChild = GetTreeGraph()->CreateNewNode( xmlChild );
		if ( pChild )
		{
			GetTreeGraph()->ConnectNodes( this, pChild->IsRoot() ? pChild : pChild->GetParentNode()  );
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CSelectionTree_BaseNode::SaveChildrenToXml( XmlNodeRef& xmlNode )
{
	for ( int i = 0; i < GetChildNodeCount(); ++i )
	{
		XmlNodeRef childXml = gEnv->pSystem->CreateXmlNode();
		if ( GetChildNode( i )->SaveToXml( childXml ) )
		{
			xmlNode->addChild( childXml );
		}
		else
		{
			return false;
		}
	}
	return true;
}

float CSelectionTree_BaseNode::GetChildNodesWidth() const
{
	float res = 0;
	for ( int i = 0; i < GetChildNodeCount(); ++i )
		res += GetChildNode( i )->GetTreeWidth();
	return res;
}

void CSelectionTree_BaseNode::UpdatePosition( bool bSmooth, float px, float py )
{
	Gdiplus::PointF position;
	if ( IsRoot() )
	{
		position.X = px;
		px -= GetTreeWidth() / 2.f + GetNodeWidth() / 2.f;
	}
	else
	{
		position.X = px + GetTreeWidth() / 2.f - GetNodeWidth() / 2.f;
	}

	if ( !IsRoot() )
	{
		CSelectionTree_BaseNode* pParent = GetParentNode();
		float totalWidth = pParent->GetChildNodesWidth();

		if ( totalWidth < pParent->GetNodeWidth() )
			position.X += ( pParent->GetNodeWidth() - totalWidth ) / 2.f;
	}
	position.Y = py;

	if ( CanSetPos() && GetTreeGraph()->IsReorderEnabled() )
	{
		Gdiplus::PointF currPos = GetPos();
		if (bSmooth && !currPos.Equals(position))
		{
			m_fLerp = 0;
			m_DestPos = position;
			if (!m_bRegistered)
			{
				GetIEditor()->RegisterNotifyListener( this );
				m_bRegistered = true;
			}
		}
		else
		{
			CHyperNode::SetPos( position );
			if (m_bRegistered) 
			{
				GetIEditor()->UnregisterNotifyListener( this );
				m_bRegistered = false;
			}
		}
	}

	for ( int i = 0; i < GetChildNodeCount(); ++i )
	{
		CSelectionTree_BaseNode* pChild = GetChildNode( i );

		float vSpace = 1.f;
		pChild->UpdatePosition( bSmooth, px, py + GetNodeHeight() * vSpace );
		px += pChild->GetTreeWidth();
	}
}

void CSelectionTree_BaseNode::RecalculateTreeSize()
{
	float width = 0;
	float height = 0;

	for ( int i = 0; i < GetChildNodeCount(); ++i )
	{
		CSelectionTree_BaseNode* pChild = GetChildNode( i );

		pChild->RecalculateTreeSize();
		width += pChild->GetTreeWidth();

		float childHeight = pChild->GetTreeHeight();
		if ( childHeight > height  )
			height = childHeight;
	}

	m_fTreeWidth = max( width, GetNodeWidth() );
	m_fTreeHeight = height + GetNodeHeight();
}

bool CSelectionTree_BaseNode::CanSetPos()
{
	return !IsSelected() || !GetTreeGraph()->IsSelectionLocked();
}


void CSelectionTree_BaseNode::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if( event == eNotify_OnIdleUpdate && CanSetPos() )
	{
		if (!GetTreeGraph()->IsReorderEnabled() && m_bRegistered)
		{
			GetIEditor()->UnregisterNotifyListener( this );
			m_bRegistered = false;
			return;
		}

		Vec2 p( m_DestPos.X, m_DestPos.Y );
		if (GetTreeGraph()->IsReorderSmooth())
		{
			Gdiplus::PointF currPos = GetPos();

			p.SetLerp( Vec2(currPos.X, currPos.Y), Vec2( m_DestPos.X, m_DestPos.Y ), m_fLerp );
			m_fLerp += 0.1f;
		}
		else
		{
			m_fLerp = 1;
		}
		CHyperNode::SetPos( Gdiplus::PointF( p.x, p.y ) );
		if ( m_fLerp >= 1 && m_bRegistered)
		{
			GetIEditor()->UnregisterNotifyListener( this );
			m_bRegistered = false;
		}
		GetTreeGraph()->GetGraphView()->Invalidate();
	}
}

void CSelectionTree_BaseNode::PropertyChanged( XmlNodeRef var )
{
	assert(false);
}

void CSelectionTree_BaseNode::FillPropertyTable( XmlNodeRef var )
{
	assert(false);
}

void CSelectionTree_BaseNode::Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar/*=0 */ )
{
	CHyperNode::Serialize( node, bLoading, ar );
	if(!bLoading)
		node->addChild(m_attributes);
	else
	{
		for (int i = 0; i < node->getChildCount(); i++)
		{
			if(strcmp(GetName(), node->getChild(i)->getTag()) == 0)
			{
				m_attributes = node->getChild(i);
				break;
			}
		}
	}
}
