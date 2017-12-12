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

#pragma once


// CMissionProps dialog
#ifndef CRYINCLUDE_EDITOR_MISSIONPROPS_H
#define CRYINCLUDE_EDITOR_MISSIONPROPS_H

class CMissionProps : public CPropertyPage
{
	DECLARE_DYNAMIC(CMissionProps)

private:
	CButton m_btnScript;
	CButton m_btnEquipPack;
	CListBox m_lstMethods;
	CListBox m_lstEvents;
	CEdit m_editMusicScript;

public:
	CMissionProps();
	virtual ~CMissionProps();

// Dialog Data
	enum { IDD = IDD_MISSIONMAINPROPS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	void LoadScript(CString sFilename);
public:
	afx_msg void OnBnClickedEquippack();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedRemove();
	afx_msg void OnBnClickedCreate();
	afx_msg void OnBnClickedReload();
	afx_msg void OnBnClickedEdit();
	afx_msg void OnBnClickedBrowsemusic();
	afx_msg void OnBnClickedNomusic();
};

#endif // CRYINCLUDE_EDITOR_MISSIONPROPS_H
