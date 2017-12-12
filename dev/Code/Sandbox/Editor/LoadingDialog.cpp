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
#include "LoadingDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CLoadingDialog dialog

CLoadingDialog::CLoadingDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CLoadingDialog::IDD, pParent)
{
    //{{AFX_DATA_INIT(CLoadingDialog)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CLoadingDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLoadingDialog)
    DDX_Control(pDX, IDC_CONSOLE_OUTPUT, m_lstConsoleOutput);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoadingDialog, CDialog)
//{{AFX_MSG_MAP(CLoadingDialog)
ON_WM_CTLCOLOR()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadingDialog message handlers

HBRUSH CLoadingDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    ////////////////////////////////////////////////////////////////////////
    // Change the background color of the listbox to the window background
    // color. Also change the text background color
    ////////////////////////////////////////////////////////////////////////

    HBRUSH hbr;

    if (nCtlColor == CTLCOLOR_LISTBOX)
    {
        // Get the background color and return this brush
        hbr = (HBRUSH) ::GetSysColorBrush(COLOR_BTNFACE);

        // Modify the text background mode
        pDC->SetBkMode(TRANSPARENT);
    }
    else
    {
        // Use default from base class
        hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    }

    // TODO: Return a different brush if the default is not desired
    return hbr;
}
