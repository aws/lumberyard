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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALPREVIEWOPTIONSDIALOG_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALPREVIEWOPTIONSDIALOG_H
#pragma once


#include "PropertiesPanel.h"

class QModelViewportCE;
class CFacialPreviewDialog;
class CFacialEdContext;

class CFacialPreviewOptionsDialog : public CDialog
{
	DECLARE_DYNAMIC(CFacialPreviewOptionsDialog)
public:
	enum { IDD = IDD_DATABASE };

	CFacialPreviewOptionsDialog();
	~CFacialPreviewOptionsDialog();

	void SetViewport(CFacialPreviewDialog* pPreviewDialog);
	void SetContext(CFacialEdContext* pContext);

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	CPropertiesPanel* m_panel;
	QModelViewportCE* m_pModelViewportCE;
	CFacialPreviewDialog* m_pPreviewDialog;
	HACCEL m_hAccelerators;
	CFacialEdContext* m_pContext;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALPREVIEWOPTIONSDIALOG_H
