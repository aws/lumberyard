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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALJOYSTICKDIALOG_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALJOYSTICKDIALOG_H
#pragma once


#include "FacialEdContext.h"
#include "Controls/JoystickCtrl.h"
#include "ToolbarDialog.h"

class CFacialJoystickDialog : public CToolbarDialog, public IFacialEdListener, IJoystickCtrlContainer, IEditorNotifyListener
{
	DECLARE_DYNAMIC(CFacialJoystickDialog)
	friend class CJoystickDialogDropTarget;
public:
	enum { IDD = IDD_DATABASE };

	CFacialJoystickDialog();
	~CFacialJoystickDialog();

	void SetContext(CFacialEdContext* pContext);

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnFacialEdEvent(EFacialEdEvent event, IFacialEffector* pEffector,int nChannelCount,IFacialAnimChannel **ppChannels);
	void RecalcLayout();
	void UpdateFreezeLayoutStatus();
	void UpdateAutoCreateKeyStatus();
	void ReadDisplayedSnapMargin();
	void Update();

	// IJoystickCtrlContainer
	virtual void OnAction(JoystickAction action);
	virtual void OnFreezeLayoutChanged();
	virtual IJoystickChannel* GetPotentialJoystickChannel();
	virtual float GetCurrentEvaluationTime();
	virtual float GetMaxEvaluationTime();
	virtual void OnSplineChanged();
	virtual void OnJoysticksChanged();
	virtual void OnBeginDraggingJoystickKnob(IJoystick* pJoystick);
	virtual void OnJoystickSelected(IJoystick* pJoystick, bool exclusive);
	virtual bool GetPlaying() const;
	virtual void SetPlaying(bool playing);

	// IEditorNotifyListener implementation
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

	void RefreshControlJoystickSet();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnFreezeLayout();
	afx_msg void OnSnapMarginChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateSnapMarginSizeUI(CCmdUI* pCmdUI);
	afx_msg void OnAutoCreateKeyChanged();
	afx_msg void OnKeyAll();
	afx_msg void OnZeroAll();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnDestroy();

	CFacialEdContext* m_pContext;
	CJoystickCtrl m_ctrl;
	CXTPToolBar m_toolbar;
	CXTPControlButton* m_pFreezeLayoutButton;
	CXTPControlButton* m_pAutoCreateKeyButton;

	typedef std::vector<int> SnapMarginList;
	SnapMarginList m_snapMargins;
	int m_displayedSnapMargin;
	bool m_bIgnoreSplineChangeEvents;
	HACCEL m_hAccelerators;
	COleDropTarget* m_pDropTarget;
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_FACIALJOYSTICKDIALOG_H
