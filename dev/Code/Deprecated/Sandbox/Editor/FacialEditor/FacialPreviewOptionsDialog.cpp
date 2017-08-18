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
#include "FacialPreviewOptionsDialog.h"
#include "CharacterEditor/ModelViewportCE.h"
#include "FacialPreviewDialog.h"
#include "FacialEdContext.h"

IMPLEMENT_DYNAMIC(CFacialPreviewOptionsDialog,CDialog)

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CFacialPreviewOptionsDialog, CDialog)
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()
	
//////////////////////////////////////////////////////////////////////////
CFacialPreviewOptionsDialog::CFacialPreviewOptionsDialog()
:	m_panel(0),
	m_pModelViewportCE(0),
	m_hAccelerators(0),
	m_pContext(0)
{
}

//////////////////////////////////////////////////////////////////////////
CFacialPreviewOptionsDialog::~CFacialPreviewOptionsDialog()
{
	if (m_panel)
	{
		delete m_panel;
		m_panel = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
void CFacialPreviewOptionsDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

//////////////////////////////////////////////////////////////////////////
void CFacialPreviewOptionsDialog::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_panel->GetSafeHwnd())
	{
		CRect clientRect;
		GetClientRect(&clientRect);
		m_panel->MoveWindow(clientRect);
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CFacialPreviewOptionsDialog::OnInitDialog()
{
	BOOL bRes = __super::OnInitDialog();

	m_hAccelerators = LoadAccelerators(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDR_FACED_MENU));

	this->m_panel = new CPropertiesPanel(this);
	this->m_panel->AddVars(m_pPreviewDialog->GetVarObject()->GetVarBlock());
	//this->m_panel->AddVars( m_pModelViewportCE->GetVarObject()->GetVarBlock() );
	this->m_panel->ShowWindow(SW_SHOWDEFAULT);

	//this->ShowWindow(SW_SHOWDEFAULT);

	return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CFacialPreviewOptionsDialog::SetViewport(CFacialPreviewDialog* pPreviewDialog)
{
	m_pPreviewDialog = pPreviewDialog;
	m_pModelViewportCE = m_pPreviewDialog->GetViewport();
}

//////////////////////////////////////////////////////////////////////////
BOOL CFacialPreviewOptionsDialog::PreTranslateMessage(MSG* pMsg)
{
   if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST && m_hAccelerators)
      return ::TranslateAccelerator(m_hWnd, m_hAccelerators, pMsg);
   return CDialog::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
BOOL CFacialPreviewOptionsDialog::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CFacialEdContext::MoveToKeyDirection direction = (zDelta < 0 ? CFacialEdContext::MoveToKeyDirectionBackward : CFacialEdContext::MoveToKeyDirectionForward);
	if (m_pContext)
		m_pContext->MoveToFrame(direction);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CFacialPreviewOptionsDialog::SetContext(CFacialEdContext* pContext)
{
	m_pContext = pContext;
}
