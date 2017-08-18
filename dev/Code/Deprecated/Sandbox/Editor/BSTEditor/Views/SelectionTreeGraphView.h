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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEGRAPHVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEGRAPHVIEW_H
#pragma once


#include "SelectionTreeBaseDockView.h"
#include "BSTEditor/Graph/SelectionTreeHyperGraphView.h"

#include "Util/IXmlHistoryManager.h"

class CSelectionTreeGraphManager;
class CSelectionTreeGraph;

class CSelectionTreeGraphView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
{
	DECLARE_DYNCREATE( CSelectionTreeGraphView )

public:
	CSelectionTreeGraphView();
	virtual ~CSelectionTreeGraphView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId );

	CSelectionTreeGraph* GetGraph() const { return m_pGraph; }
protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

private:
	CSelectionTreeHyperGraphView m_hyperGraphView;
	CSelectionTreeGraphManager* m_pGraphManager;
	CSelectionTreeGraph* m_pGraph;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEGRAPHVIEW_H
