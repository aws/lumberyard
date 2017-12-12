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
#include "SelectionTreePropertiesView.h"

#include "StringDlg.h"
#include "CustomMessageBox.h"

#include "BSTEditor/Dialogs/SelectionTree_RefBrowser.h"

#include "BSTEditor/SelectionTreeManager.h"
#include "BSTEditor/SelectionTreeModifier.h"
#include "IEditor.h"
#include "CryEditDoc.h"
#include "../Graph/SelectionTreeGraph.h"


IMPLEMENT_DYNAMIC( CSelectionTreePropertiesView, CSelectionTreeBaseDockView )

#define ID_TREE_CONTROL 1


CSelectionTreePropertiesView::CSelectionTreePropertiesView()
	: m_bLoading ( false ),
	m_CurrentSelectedNode( NULL )
{
}

CSelectionTreePropertiesView::~CSelectionTreePropertiesView()
{
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UndoEventHandlerDestroyed( this, eSTTI_Properties, false );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->UnregisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetPropertiesView( NULL );
}

BOOL CSelectionTreePropertiesView::OnInitDialog()
{
	BOOL baseInitSuccess = __super::OnInitDialog();
	if ( ! baseInitSuccess )
	{
		return FALSE;
	}

	CRect rc;
	GetClientRect(rc);
	m_propsCtrl.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS, rc, this, ID_TREE_CONTROL );
	m_propsCtrl.SetUpdateCallback( functor(*this,&CSelectionTreePropertiesView::OnUpdateProperties) );
	m_propsCtrl.SetCallbackOnNonModified(false);

	SetResize( ID_TREE_CONTROL, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RegisterView( this );
	GetIEditor()->GetSelectionTreeManager()->SetPropertiesView( this );
	GetIEditor()->GetSelectionTreeManager()->GetHistory()->RestoreUndoEventHandler( this, eSTTI_Properties );
	
	return TRUE;
}

void CSelectionTreePropertiesView::Clear()
{
	m_CurrentSelectedNode = nullptr;
	m_propsCtrl.DeleteAllItems();
}

bool CSelectionTreePropertiesView::LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex )
{
	Clear();

	bool res = false;
	if ( typeId == eSTTI_Tree)
	{
		pUndoEventHandler = this;
		res = LoadFromXml(xmlNode);
	}
	return res;
}


bool CSelectionTreePropertiesView::ReloadFromXml( const XmlNodeRef& xmlNode )
{
	Clear();
	return LoadFromXml(xmlNode);
}

void CSelectionTreePropertiesView::OnNodeClicked( CSelectionTree_BaseNode* pNode )
{
	m_bLoading = true;
	m_CurrentSelectedNode = pNode;

	XmlNodeRef xmlNode = pNode->GetAttributes();

	if(pNode)
	{
		XmlNodeRef templateNode = GetIEditor()->FindTemplate( xmlNode->getTag() );
		if(templateNode)
		{
			XmlNodeRef templateNodeCopy = templateNode->clone();
			XmlNodeRef loggingNode = GetIEditor()->FindTemplate( MBT_LOGGING_NODE );

			if(loggingNode && pNode->ShouldAppendLoggingProps())
			{
				XmlNodeRef loggingNodeCopy = loggingNode->clone();
				for (int i = 0; i < loggingNode->getChildCount(); i++)
				{
					templateNodeCopy->addChild(loggingNodeCopy->getChild(i));
				}
			}
			
			pNode->FillPropertyTable(templateNodeCopy);
			m_propsCtrl.CreateItems(templateNodeCopy);
		}
	}

	m_bLoading = false;
}

void CSelectionTreePropertiesView::OnUpdateProperties( XmlNodeRef var )
{
	if(m_bLoading)
		return;

	m_CurrentSelectedNode->PropertyChanged(var);

	char buff[256];
	sprintf_s(buff, "Property [%s:%s] Changed", m_CurrentSelectedNode->GetName(), var->getTag());
	GetIEditor()->GetSelectionTreeManager()->GetTreeGraph()->RecordUndo(buff);
}