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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEHYPERGRAPHVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEHYPERGRAPHVIEW_H
#pragma once


#include "HyperGraph/HyperGraphView.h"

class CSelectionTreeGraph;
class CSelectionTreeGraphManager;
class CSelectionTree_BaseNode;
class CSelectionTreeEditor;

enum EBSTOrderMode
{
	eBOM_Disabled,
	eBOM_Auto,
	eBOM_AutoSmooth,
};

class CSelectionTreeHyperGraphView
	: public CHyperGraphView
{
	DECLARE_DYNAMIC( CSelectionTreeHyperGraphView )

public:
	CSelectionTreeHyperGraphView();
	virtual ~CSelectionTreeHyperGraphView();

	void ResetView();
	EBSTOrderMode GetOrderMode() const { return m_NodeOrderMode; }

	void CenterAroundNode(CHyperNode* pNode) { CenterViewAroundNode(pNode); }
protected:
	afx_msg void OnUndo();
	afx_msg void OnRedo();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()

	virtual void ShowContextMenu( CPoint point, CHyperNode* pNode );
	virtual void UpdateTooltip( CHyperNode* pNode, CHyperNodePort* pPort ) {}
	virtual bool HandleRenameNode( CHyperNode *pNode );
	virtual void OnNodesConnected();
	virtual void OnEdgeRemoved();
	virtual void OnNodeRenamed();


	void ReorderAllNodes();
	void UnselectAll();
private:
	void LoadTemplates();
	CSelectionTreeGraph* GetSelectionTreeGraph();
	const CSelectionTreeGraph* GetSelectionTreeGraph() const;

	CSelectionTreeGraphManager* GetSelectionTreeGraphManager();
	const CSelectionTreeGraphManager* GetSelectionTreeGraphManager() const;

	void DeleteNodesInt();
	void CutNodesInt();
	void CopyNodesInt();
	void PasteNodesInt( CPoint point, bool withLinks = true );

	bool CheckForReadOnlyNodes();

	bool IsGraphLoaded();
private:
	EBSTOrderMode m_NodeOrderMode;
	EBSTOrderMode m_InternalNodeOrderMode;
	bool m_bCtrlPressed;

	std::map<int, string> m_TemplateMap;
	std::map<string, int> m_TemplateMapSorted;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREEHYPERGRAPHVIEW_H

