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

// Description : This is the source file for the general utility dialog for
//               progress display. The purpose of this dialog, as one might imagine, is to
//               display the progress of any process.


#include "StdAfx.h"
#include "ProgressDialog.h"

BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
ON_COMMAND(IDC_GENERAL_PROGRESS_CANCEL_BUTTON, &CProgressDialog::OnCancel)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CProgressDialog::CProgressDialog(CWnd*  poParentWindow)
    : CDialog(IDD_GENERAL_PROGRESS_DIALOG)
    , m_strText("")
    , m_strTitle("Current progress:")
    , m_nMinimum(0)
    , m_nMaximum(100)
    , m_nPosition(0)
{
    CDialog::Create(IDD_GENERAL_PROGRESS_DIALOG, poParentWindow);
}
//////////////////////////////////////////////////////////////////////////
CProgressDialog::CProgressDialog(CString strText, CString strTitle, bool bMarquee, int nMinimum, int nMaximum, CWnd* poParentWindow)
    : CDialog(IDD_GENERAL_PROGRESS_DIALOG)
    , m_strText(strText)
    , m_strTitle(strTitle)
    , m_nMinimum(nMinimum)
    , m_nMaximum(nMaximum)
    , m_nPosition(nMinimum)
{
    CDialog::Create(IDD_GENERAL_PROGRESS_DIALOG, poParentWindow);
    if (bMarquee)
    {
        ::SetWindowLong(m_oProgressControl.GetSafeHwnd(), GWL_STYLE,
                ::GetWindowLong(m_oProgressControl.GetSafeHwnd(), GWL_STYLE) | PBS_MARQUEE);
    }
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_GENERAL_PROGRESS_PROGRESS_BAR, m_oProgressControl);
    DDX_Control(pDX, IDC_GENERAL_PROGRESS_CANCEL_BUTTON, m_oCancelButton);
    DDX_Text(pDX, IDC_GENERAL_PROGRESS_TEXT, m_strText);
}
//////////////////////////////////////////////////////////////////////////
BOOL CProgressDialog::OnInitDialog()
{
    if (CDialog::OnInitDialog() == FALSE)
    {
        return FALSE;
    }

    SetWindowText(m_strTitle);

    UpdateData(FALSE);

    if (!m_pfnCancelCallback)
    {
        m_oCancelButton.ShowWindow(SW_HIDE);
    }

    return TRUE;
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::OnCancel()
{
    if (m_pfnCancelCallback)
    {
        m_pfnCancelCallback();
    }
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetRange(int nMinimum, int nMaximum)
{
    m_oProgressControl.SetRange32(nMinimum, nMaximum);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetPosition(int nPosition)
{
    m_oProgressControl.SetPos(nPosition);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetMarquee(bool onOff, int interval)
{
    m_oProgressControl.SendMessage(PBM_SETMARQUEE, onOff ? TRUE : FALSE, interval);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetText(CString strText)
{
    m_strText = strText;
    SetDlgItemText(IDC_GENERAL_PROGRESS_TEXT, strText);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetTitle(CString strTitle)
{
    m_strTitle = strTitle;
    SetWindowText(m_strTitle);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetCancelCallback(TDCancelCallback pfnCancelCallback)
{
    m_pfnCancelCallback = pfnCancelCallback;
    m_oCancelButton.ShowWindow(SW_SHOW);
}
//////////////////////////////////////////////////////////////////////////
