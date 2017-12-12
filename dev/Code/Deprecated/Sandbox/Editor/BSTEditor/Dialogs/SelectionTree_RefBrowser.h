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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_SELECTIONTREE_REFBROWSER_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_SELECTIONTREE_REFBROWSER_H
#pragma once


#include "BSTEditor/SelectionTreeManager.h"

class CSelectionTree_RefBrowser : public CDialog
{
	DECLARE_DYNAMIC(CSelectionTree_RefBrowser)

public:
	CSelectionTree_RefBrowser(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelectionTree_RefBrowser();

	void Init( ESelectionTreeTypeId type );
	const char* GetSelectedRef();

// Dialog Data
	enum { IDD = IDD_BST_REFBROWSER };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL		OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	CTreeCtrl m_RefTree;
	ESelectionTreeTypeId m_refType;

	afx_msg void OnSelChanged( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnShowContents();

private:
	void LoadReferences();

	typedef std::map< HTREEITEM, string > TRefNames;
	TRefNames m_RefNames;
	string m_SelectedRef;

};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_DIALOGS_SELECTIONTREE_REFBROWSER_H
