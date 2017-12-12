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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_SELECTIONTREEERRORDIALOG_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_SELECTIONTREEERRORDIALOG_H
#pragma once

#include "QtWinMigrate/qwinwidget.h"

#include "BSTEditor/SelectionTreeErrorReport.h"

class SelectionTreeErrorModel;
class ColumnGroupTreeView;

class CSelectionTreeErrorDialog : public QWinWidget
{
	Q_OBJECT
public:
	CSelectionTreeErrorDialog( );   // standard constructor
	virtual ~CSelectionTreeErrorDialog();

	static void CSelectionTreeErrorDialog::RegisterViewClass();

	static void Open( CSelectionTreeErrorReport *pReport );
	static void Close();
	static void Clear();
	static void Reload();

	// you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
	// make sure you pick a unique GUID
	static const GUID& GetClassID()
	{
		// {dbe31558-3807-4076-bacc-10097153141e}
		static const GUID guid =
		{ 0xdbe31558, 0x3807, 0x4076, { 0xba, 0xcc, 0x10, 0x09, 0x71, 0x53, 0x14, 0x1e } };
		return guid;
	}

protected:
	void SetReport(CSelectionTreeErrorReport *report) {m_pErrorReport = report;}
	void UpdateErrors();

	void ReloadErrors();

	CSelectionTreeErrorReport *m_pErrorReport;

	static CSelectionTreeErrorDialog* m_instance;

	SelectionTreeErrorModel* m_model;
	ColumnGroupTreeView* m_wndReport;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_SELECTIONTREEERRORDIALOG_H
