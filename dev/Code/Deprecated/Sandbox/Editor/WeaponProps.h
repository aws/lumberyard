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

#ifndef CRYINCLUDE_EDITOR_WEAPONPROPS_H
#define CRYINCLUDE_EDITOR_WEAPONPROPS_H


#include "Controls\PropertyCtrl.h"

#pragma once

// CWeaponProps dialog

class CWeaponProps : public CPropertyPage
{
	DECLARE_DYNAMIC(CWeaponProps)

public:
	CWeaponProps();
	virtual ~CWeaponProps();

// Dialog Data
	enum { IDD = IDD_WEAPONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	CPropertyCtrl	m_propWnd;
	XmlNodeRef m_node;
	CString m_title;

protected:
	CListBox m_availableWeapons;
	CListBox m_usedWeapons;

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnQueryCancel();
	virtual void OnReset();
	virtual void OnCancel();

	void OnPropertyChanged( XmlNodeRef node );

	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedRemove();
};

#endif // CRYINCLUDE_EDITOR_WEAPONPROPS_H
