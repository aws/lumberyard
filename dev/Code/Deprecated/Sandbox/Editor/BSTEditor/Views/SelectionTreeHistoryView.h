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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEHISTORYVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEHISTORYVIEW_H
#pragma once



#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTreeHistoryView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryEventListener
{
	DECLARE_DYNAMIC( CSelectionTreeHistoryView )

public:
	CSelectionTreeHistoryView();
	virtual ~CSelectionTreeHistoryView();

	void LoadHistory();

	// ISelectionTreeUndoEventListener
	virtual void OnEvent( EHistoryEventType event, void* pData = NULL );

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	void ChangeHistory( const HTREEITEM clickedItemHandle );

private:
	CTreeCtrl m_history;

	typedef std::map< int, HTREEITEM > THistoryMap;
	THistoryMap m_HistoryMap;
	bool m_bNeedReload;
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEHISTORYVIEW_H
