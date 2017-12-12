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

#include "StdAfx.h"
#include "SrcSafeSettingsDialog.h"


// CSrcSafeSettingsDialog dialog

IMPLEMENT_DYNAMIC(CSrcSafeSettingsDialog, CDialog)
CSrcSafeSettingsDialog::CSrcSafeSettingsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSrcSafeSettingsDialog::IDD, pParent)
	, m_username(_T(""))
	, m_exeFile(_T(""))
	, m_database(_T(""))
	, m_project(_T(""))
{
}

CSrcSafeSettingsDialog::~CSrcSafeSettingsDialog()
{
}

void CSrcSafeSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDX_Text(pDX, IDC_EXE, m_exeFile);
	DDX_Text(pDX, IDC_DATABASE, m_database);
	DDX_Text(pDX, IDC_PROJECT, m_project);
}


BEGIN_MESSAGE_MAP(CSrcSafeSettingsDialog, CDialog)
	ON_BN_CLICKED(IDC_BROWSE_EXE, OnBnClickedBrowseExe)
	ON_BN_CLICKED(IDC_BROWSE_DATABASE, OnBnClickedBrowseDatabase)
END_MESSAGE_MAP()


// CSrcSafeSettingsDialog message handlers

void CSrcSafeSettingsDialog::OnBnClickedBrowseExe()
{
	CString szFilters = "SourceSafe Executable (ss.exe)|ss.exe||";
	CAutoDirectoryRestoreFileDialog dlgFile( TRUE, NULL, m_exeFile, OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, szFilters );
	if (dlgFile.DoModal() == IDOK)
	{
		m_exeFile = dlgFile.GetPathName();
		UpdateData(FALSE);
	}
	// TODO: Add your control notification handler code here
}

//////////////////////////////////////////////////////////////////////////
void CSrcSafeSettingsDialog::OnBnClickedBrowseDatabase()
{
	CString szFilters = "SourceSafe Databases (srcsafe.ini)|srcsafe.ini||";
	CAutoDirectoryRestoreFileDialog dlgFile( TRUE, NULL, m_database, OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, szFilters );
	if (dlgFile.DoModal() == IDOK)
	{
		m_database = Path::GetPath( dlgFile.GetPathName() );
		m_database = Path::RemoveBackslash(m_database);
		UpdateData(FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////
BOOL CSrcSafeSettingsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	/*
	m_username = gSettings.ssafeParams.user;
	m_database = gSettings.ssafeParams.databasePath;
	m_exeFile = gSettings.ssafeParams.exeFile;
	m_project = gSettings.ssafeParams.project;
	*/
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CSrcSafeSettingsDialog::OnOK()
{
	UpdateData(TRUE);
	/*
	gSettings.ssafeParams.user = m_username;
	gSettings.ssafeParams.databasePath = m_database;
	gSettings.ssafeParams.exeFile = m_exeFile;
	gSettings.ssafeParams.project = m_project;
	*/

	CDialog::OnOK();
}