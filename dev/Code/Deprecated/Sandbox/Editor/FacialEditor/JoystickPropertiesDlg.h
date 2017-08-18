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

#ifndef CRYINCLUDE_EDITOR_FACIALEDITOR_JOYSTICKPROPERTIESDLG_H
#define CRYINCLUDE_EDITOR_FACIALEDITOR_JOYSTICKPROPERTIESDLG_H
#pragma once


class CJoystickPropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CJoystickPropertiesDlg)

public:
	CJoystickPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CJoystickPropertiesDlg();

	void SetChannelName(int axis, const string& channelName);
	void SetChannelFlipped(int axis, bool flipped);
	bool GetChannelFlipped(int axis) const;
	void SetVideoScale(int axis, float offset);
	float GetVideoScale(int axis) const;
	void SetVideoOffset(int axis, float offset);
	float GetVideoOffset(int axis) const;

	void SetChannelEnabled(int axis, bool enabled);

	// Dialog Data
	enum { IDD = IDD_FACEED_JOYSTICK_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	virtual void OnOK();

	CEdit m_nameEdits[2];
	CNumberCtrl m_scaleEdits[2];
	CButton m_flippedButtons[2];
	CNumberCtrl m_offsetEdits[2];

	string m_names[2];
	float m_scales[2];
	bool m_flippeds[2];
	float m_offsets[2];
	bool m_enableds[2];
};

#endif // CRYINCLUDE_EDITOR_FACIALEDITOR_JOYSTICKPROPERTIESDLG_H
