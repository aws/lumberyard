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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALPREVIEWDIALOG_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALPREVIEWDIALOG_H
#pragma once

#include "ToolbarDialog.h"
#include "IFacialEditor.h"
#include "FacialEdContext.h"

class CFacialEdContext;
class QModelViewportCE;
class QModelViewportFE;
class QWinWidget;

class QWinWidget;

//////////////////////////////////////////////////////////////////////////
class CFacialPreviewDialog : public CToolbarDialog, public IFacialEdListener
{
	DECLARE_DYNAMIC(CFacialPreviewDialog)
public:
	static void RegisterViewClass();

	CFacialPreviewDialog();
	~CFacialPreviewDialog();

	enum { IDD = IDD_DATABASE };

	void SetContext( CFacialEdContext *pContext );
	void RedrawPreview();

	QModelViewportCE* GetViewport();
	const CVarObject* GetVarObject() const { return &m_vars; }

	void SetForcedNeckRotation(const Quat& rotation);
	void SetForcedEyeRotation(const Quat& rotation, IFacialEditor::EyeType eye);

	void SetAnimateCamera(bool bAnimateCamera);
	bool GetAnimateCamera() const;

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	afx_msg void OnCenterOnHead();

private:
	virtual void OnFacialEdEvent(EFacialEdEvent event, IFacialEffector* pEffector, int nChannelCount, IFacialAnimChannel** ppChannels);

	void OnLookIKChanged(IVariable* var);
	void OnLookIKEyesOnlyChanged(IVariable* var);
	void OnShowEyeVectorsChanged(IVariable* var);
	void OnLookIKOffsetChanged(IVariable* var);
	void OnProceduralAnimationChanged(IVariable* var);

	QWinWidget* m_pModelViewportContainer;
	QModelViewportFE* m_pModelViewport;
	CXTPToolBar m_wndToolBar;

	CFacialEdContext *m_pContext;

	CVariable<bool> m_bLookIK;
	CVariable<bool> m_bLookIKEyesOnly;
	CVariable<bool> m_bShowEyeVectors;
	CVariable<float> m_fLookIKOffsetX;
	CVariable<float> m_fLookIKOffsetY;
	CVariable<bool> m_bProceduralAnimation;
	CVarObject m_vars;
	HACCEL m_hAccelerators;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALPREVIEWDIALOG_H
