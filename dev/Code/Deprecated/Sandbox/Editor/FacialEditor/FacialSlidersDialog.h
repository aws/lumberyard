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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALSLIDERSDIALOG_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALSLIDERSDIALOG_H
#pragma once

#include "FacialSlidersCtrl.h"

//////////////////////////////////////////////////////////////////////////
class CFacialSlidersDialog : public CDialog
{
	DECLARE_DYNAMIC(CFacialSlidersDialog)
public:
	static void RegisterViewClass();

	CFacialSlidersDialog();
	~CFacialSlidersDialog();

	enum { IDD = IDD_DATABASE };

	void SetContext( CFacialEdContext *pContext );

	void SetMorphWeight(IFacialEffector* pEffector, float fWeight);
	void ClearAllMorphs();

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTabSelect(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnClearAllSliders();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	void RecalcLayout();

private:
	CXTPToolBar m_wndToolBar;
	CFacialSlidersCtrl m_slidersCtrl[2];
	CTabCtrl m_tabCtrl;

	CFacialEdContext *m_pContext;
	HACCEL m_hAccelerators;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALSLIDERSDIALOG_H
