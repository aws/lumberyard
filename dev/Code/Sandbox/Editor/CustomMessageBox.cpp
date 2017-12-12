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

#include "stdafx.h"
#include "CustomMessageBox.h"


/////////////////////////////////////////////////////////////////////////////
// CCustomMessageBox dialog

#define MAX_BUFLEN 1000

CCustomMessageBox::CCustomMessageBox(CWnd* pParent, LPCSTR lpText, LPCSTR lpCaption, LPCSTR lpButtonName1, LPCSTR lpButtonName2, LPCSTR lpButtonName3, LPCSTR lpButtonName4)
    : CDialog(IDD_CUSTOMMESSAGEBOX, pParent)
{
    //{{AFX_DATA_INIT(CTipDlg)
    //}}AFX_DATA_INIT
    m_nValue = 0;

    if (lpText)
    {
        m_message = lpText;
    }
    if (lpCaption)
    {
        m_caption = lpCaption;
    }
    if (lpButtonName1)
    {
        m_buttonName1 = lpButtonName1;
    }
    if (lpButtonName2)
    {
        m_buttonName2 = lpButtonName2;
    }
    if (lpButtonName3)
    {
        m_buttonName3 = lpButtonName3;
    }
    if (lpButtonName4)
    {
        m_buttonName4 = lpButtonName4;
    }
}

CCustomMessageBox::~CCustomMessageBox()
{
}

void CCustomMessageBox::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCustomMessageBox)
    //  DDX_Text(pDX, IDC_TIPSTRING, m_strTip);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCustomMessageBox, CDialog)
//{{AFX_MSG_MAP(CCustomMessageBox)
ON_BN_CLICKED(IDC_KEY1, OnButton1)
ON_BN_CLICKED(IDC_KEY2, OnButton2)
ON_BN_CLICKED(IDC_KEY3, OnButton3)
ON_BN_CLICKED(IDC_KEY4, OnButton4)
ON_WM_CTLCOLOR()
ON_WM_PAINT()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustomMessageBox message handlers

void CCustomMessageBox::OnButton1()
{
    m_nValue = 1;
    CDialog::OnOK();
}

void CCustomMessageBox::OnButton2()
{
    m_nValue = 2;
    CDialog::OnOK();
}

void CCustomMessageBox::OnButton3()
{
    m_nValue = 3;
    CDialog::OnOK();
}

void CCustomMessageBox::OnButton4()
{
    m_nValue = 4;
    CDialog::OnOK();
}

void CCustomMessageBox::OnOK()
{
    //CDialog::OnOK();
}

BOOL CCustomMessageBox::OnInitDialog()
{
    CDialog::OnInitDialog();
    // If Tips file does not exist then disable NextTip
    //if (m_pStream == NULL)
    //GetDlgItem(IDC_NEXTTIP)->EnableWindow(FALSE);

    SetWindowText(m_caption);

    SetDlgItemText(IDC_KEY1, m_buttonName1);

    if (!m_buttonName2.IsEmpty())
    {
        SetDlgItemText(IDC_KEY2, m_buttonName2);
        GetDlgItem(IDC_KEY2)->ShowWindow(SW_SHOW);
    }
    if (!m_buttonName3.IsEmpty())
    {
        SetDlgItemText(IDC_KEY3, m_buttonName3);
        GetDlgItem(IDC_KEY3)->ShowWindow(SW_SHOW);
    }
    if (!m_buttonName4.IsEmpty())
    {
        SetDlgItemText(IDC_KEY4, m_buttonName4);
        GetDlgItem(IDC_KEY4)->ShowWindow(SW_SHOW);
    }

    SetDlgItemText(IDC_STRMESSAGE, m_message);

    return TRUE;  // return TRUE unless you set the focus to a control
}


int CCustomMessageBox::Show(LPCSTR lpText, LPCSTR lpCaption, LPCSTR lpButtonName1, LPCSTR lpButtonName2, LPCSTR lpButtonName3, LPCSTR lpButtonName4)
{
    CCustomMessageBox box(CWnd::FromHandle(AfxGetMainWnd()->GetSafeHwnd()), lpText, lpCaption, lpButtonName1, lpButtonName2, lpButtonName3, lpButtonName4);
    box.DoModal();
    return box.GetValue();
}