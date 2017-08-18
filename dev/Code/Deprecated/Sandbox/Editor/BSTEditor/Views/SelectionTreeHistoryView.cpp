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
#include "SelectionTreeHistoryView.h"

#include "BSTEditor/SelectionTreeManager.h"


IMPLEMENT_DYNAMIC( CSelectionTreeHistoryView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1

BEGIN_MESSAGE_MAP( CSelectionTreeHistoryView, CSelectionTreeBaseDockView )
	ON_NOTIFY( NM_CLICK , ID_TREE_CONTROL, OnTvnClick )
	ON_NOTIFY( NM_RCLICK , ID_TREE_CONTROL, OnTvnRightClick )
END_MESSAGE_MAP()

CSelectionTreeHistoryView::CSelectionTreeHistoryView()
: m_bNeedReload( true )
{
}

CSelectionTreeHistoryView::~CSelectionTreeHistoryView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterEventListener( this );
}

BOOL CSelectionTreeHistoryView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect( rc );

	m_history.Create( WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_history.ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterEventListener( this );

	return TRUE;
}

void CSelectionTreeHistoryView::LoadHistory()
{
	m_history.DeleteAllItems();
	m_HistoryMap.clear();

	CSelectionTreeManager* pMan = GetIEditor()->GetSelectionTreeManager();
	int currVersion = pMan->GetHistory()->GetCurrentVersionNumber();
	for ( int i = 0; i <= pMan->GetHistory()->GetVersionCount(); ++i )
	{
		m_HistoryMap[ i ] = m_history.InsertItem( pMan->GetHistory()->GetVersionDesc( i ).c_str(), 0, 0, TVI_ROOT, TVI_LAST );
	}
	m_history.SelectItem( m_HistoryMap[ currVersion ] );

}

void CSelectionTreeHistoryView::OnEvent( EHistoryEventType event, void* pData )
{
	switch ( event )
	{
	case eHET_HistoryDeleted:
		m_history.DeleteAllItems();
		m_HistoryMap.clear();
		break;
	default:
		if ( m_bNeedReload )
			LoadHistory();
	}
}

void CSelectionTreeHistoryView::OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_history.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_history.HitTest( clientPoint );

	if ( clickedItemHandle != 0 )
	{
		ChangeHistory(clickedItemHandle);
	}
}

void CSelectionTreeHistoryView::OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	CPoint clientPoint;
	::GetCursorPos( &clientPoint );
	m_history.ScreenToClient( &clientPoint );

	const HTREEITEM clickedItemHandle = m_history.HitTest( clientPoint );

	if ( clickedItemHandle != 0 )
	{
		ChangeHistory(clickedItemHandle);
	}
}

void CSelectionTreeHistoryView::ChangeHistory( const HTREEITEM clickedItemHandle )
{
	for ( THistoryMap::const_iterator it = m_HistoryMap.begin(); it != m_HistoryMap.end(); ++it)
	{
		if ( it->second == clickedItemHandle )
		{
			m_history.SelectItem( clickedItemHandle );
			m_bNeedReload = false;
			GetIEditor()->GetSelectionTreeManager()->GetHistory()->Goto( it->first );
			m_bNeedReload = true;
			return;
		}
	}
}
