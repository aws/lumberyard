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
#include "OverwriteConfirmDialog.h"

BEGIN_MESSAGE_MAP(COverwriteConfirmDialog, CDialog)
ON_COMMAND_EX(IDYES, OnCommand)
ON_COMMAND_EX(IDNO, OnCommand)
ON_COMMAND_EX(ID_YES_ALL, OnCommand)
ON_COMMAND_EX(ID_NO_ALL, OnCommand)
END_MESSAGE_MAP()

COverwriteConfirmDialog::COverwriteConfirmDialog(CWnd* pParentWindow, const char* szMessage, const char* szCaption)
    :   CDialog(IDD, pParentWindow)
    , message(szMessage)
    , caption(szCaption)
{
}

BOOL COverwriteConfirmDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    messageEdit.SetWindowText(message.c_str());
    SetWindowText(message.c_str());

    return TRUE;
}

void COverwriteConfirmDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_MESSAGE, messageEdit);
}

BOOL COverwriteConfirmDialog::OnCommand(UINT uID)
{
    EndDialog(uID);

    return TRUE;
}
