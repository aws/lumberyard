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

// Description : implementation file


#include "stdafx.h"
#include "TerrainFormulaDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainFormulaDlg dialog


CTerrainFormulaDlg::CTerrainFormulaDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTerrainFormulaDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CTerrainFormulaDlg)
    m_dParam1 = 0.0;
    m_dParam2 = 0.0;
    m_dParam3 = 0.0;
    //}}AFX_DATA_INIT
}


void CTerrainFormulaDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTerrainFormulaDlg)
    DDX_Text(pDX, IDC_EDIT_FORMULA_PARAM1, m_dParam1);
    DDV_MinMaxDouble(pDX, m_dParam1, 0., 10.);
    DDX_Text(pDX, IDC_EDIT_FORMULA_PARAM2, m_dParam2);
    DDV_MinMaxDouble(pDX, m_dParam2, 1., 5000.);
    DDX_Text(pDX, IDC_EDIT_FORMULA_PARAM3, m_dParam3);
    DDV_MinMaxDouble(pDX, m_dParam3, 0., 128.);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTerrainFormulaDlg, CDialog)
//{{AFX_MSG_MAP(CTerrainFormulaDlg)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTerrainFormulaDlg message handlers
