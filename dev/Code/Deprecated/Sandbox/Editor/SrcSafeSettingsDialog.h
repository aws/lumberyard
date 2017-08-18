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

#ifndef CRYINCLUDE_EDITOR_SRCSAFESETTINGSDIALOG_H
#define CRYINCLUDE_EDITOR_SRCSAFESETTINGSDIALOG_H
#pragma once

// CSrcSafeSettingsDialog dialog

class CSrcSafeSettingsDialog : public CDialog
{
	DECLARE_DYNAMIC(CSrcSafeSettingsDialog)

public:
	CSrcSafeSettingsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSrcSafeSettingsDialog();

// Dialog Data
	enum { IDD = IDD_SOURCESAFESETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedBrowseExe();
	afx_msg void OnBnClickedBrowseDatabase();

private:
	CString m_username;
	CString m_exeFile;
	CString m_database;
	CString m_project;
public:
	virtual BOOL OnInitDialog();
};

#endif // CRYINCLUDE_EDITOR_SRCSAFESETTINGSDIALOG_H
