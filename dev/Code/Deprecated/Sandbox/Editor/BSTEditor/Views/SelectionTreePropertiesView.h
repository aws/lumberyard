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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEPROPERTIESVIEW_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEPROPERTIESVIEW_H
#pragma once



#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"
#include "Controls\PropertyCtrl.h"
#include "..\Graph\Nodes\SelectionTree_BaseNode.h"

class CSelectionTreePropertiesView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
	, public IXmlUndoEventHandler
{
	DECLARE_DYNAMIC( CSelectionTreePropertiesView )

public:
	CSelectionTreePropertiesView();
	virtual ~CSelectionTreePropertiesView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId ) {}

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode ) {return true;}
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode ) {return true;}
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	void OnNodeClicked(CSelectionTree_BaseNode* pNode);
	void OnUpdateProperties( XmlNodeRef var );

protected:
	virtual BOOL OnInitDialog();

private:
	CPropertyCtrl m_propsCtrl;
	CSelectionTree_BaseNode* m_CurrentSelectedNode;
	bool m_bLoading;

	void Clear();

};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_VIEWS_SELECTIONTREEPROPERTIESVIEW_H
