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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEEDITOR_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEEDITOR_H
#pragma once

//#define USING_SELECTION_TREE_EDITOR

#include "Dialogs/BaseFrameWnd.h"

#include "Views/SelectionTreeGraphView.h"
#include "Views/SelectionTreeVariablesView.h"
#include "Views/SelectionTreeSignalsView.h"
#include "Views/SelectionTreeListView.h"
#include "Views/SelectionTreePropertiesView.h"
#include "Views/SelectionTreeHistoryView.h"

class CSelectionTreeEditor
	: public CBaseFrameWnd
{
	DECLARE_DYNCREATE( CSelectionTreeEditor )

public:
	CSelectionTreeEditor();
	virtual ~CSelectionTreeEditor();

	void SetTreeName(const char* name);
	void SetVariablesName(const char* name);
	void SetSignalsName(const char* name);
	void SetTimestampsName(const char* name);

	static CSelectionTreeEditor* GetInstance();

protected:
	virtual BOOL OnInitDialog();

	void AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction = xtpPaneDockLeft, CXTPDockingPaneBase* pNeighbour = NULL );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBtnClickedReload();
	afx_msg void OnBtnClickedSaveAll();
	afx_msg void OnBtnClickedNewTree();
	afx_msg void OnBtnClickedClearHistory();
	afx_msg void OnBtnDisplayWarnings();

private:
	CSelectionTreeGraphView m_graphView;
	CSelectionTreeTimestampsView m_timestampView;
	CSelectionTreePropertiesView m_propertiesView;
	CSelectionTreeVariablesView m_variablesView;
	CSelectionTreeSignalsView m_signalsView;
	CSelectionTreeListView m_treeListView;
	CSelectionTreeHistoryView m_historyView;

	CXTPDockingPane* m_pGraphViewDockingPane;
	CXTPDockingPane* m_pTreeListDockingPane;
	CXTPDockingPane* m_pVariablesDockingPane;
	CXTPDockingPane* m_pSignalsDockingPane;
	CXTPDockingPane* m_pPropertiesDockingPane;
	CXTPDockingPane* m_pTimestampsDockingPane;
	CXTPDockingPane* m_pHistoryDockingPane;

	static CryCriticalSection m_sLock;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEEDITOR_H
