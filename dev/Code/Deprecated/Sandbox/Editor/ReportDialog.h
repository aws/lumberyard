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

// Description : Generic report dialog.


#ifndef CRYINCLUDE_EDITOR_REPORTDIALOG_H
#define CRYINCLUDE_EDITOR_REPORTDIALOG_H
#pragma once



#include "Report.h"

class CReportDialog : public CXTResizeDialog
{
	DECLARE_DYNAMIC(CReportDialog)

public:
	CReportDialog(const char* title, CWnd* pParent = NULL);
	virtual ~CReportDialog();

	void Open(CReport* report);
	void Close();

	// Dialog Data
	enum { IDD = IDD_ERROR_REPORT };

protected:
	void Load(CReport* report);

	virtual void OnOK();
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	afx_msg void OnSelectObjects();
	afx_msg void OnSize( UINT nType,int cx,int cy );
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	afx_msg void OnReportItemClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportItemRClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportColumnRClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportItemDblClick(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportHyperlink(NMHDR * pNotifyStruct, LRESULT * result);
	afx_msg void OnReportKeyDown(NMHDR * pNotifyStruct, LRESULT * result);

	DECLARE_MESSAGE_MAP()

	CXTPReportControl m_wndReport;
	CXTPReportSubListControl m_wndSubList;
	CXTPReportFilterEditControl m_wndFilterEdit;
	string m_title;
	string m_profileTitle;
};

#endif // CRYINCLUDE_EDITOR_REPORTDIALOG_H
