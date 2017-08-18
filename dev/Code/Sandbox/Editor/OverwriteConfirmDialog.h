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

#ifndef CRYINCLUDE_EDITOR_OVERWRITECONFIRMDIALOG_H
#define CRYINCLUDE_EDITOR_OVERWRITECONFIRMDIALOG_H
#pragma once


class COverwriteConfirmDialog
    : public CDialog
{
public:
    enum
    {
        IDD = IDD_CONFIRM_OVERWRITE
    };

    COverwriteConfirmDialog(CWnd* pParentWindow, const char* szMessage, const char* szCaption);

private:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    afx_msg BOOL OnCommand(UINT uID);

    DECLARE_MESSAGE_MAP()

    string message;
    string caption;
    CEdit messageEdit;
};

#endif // CRYINCLUDE_EDITOR_OVERWRITECONFIRMDIALOG_H
