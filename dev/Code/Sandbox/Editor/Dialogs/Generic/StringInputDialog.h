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

// Description : This is the header file for the general utility dialog for
//               string input. The purpose of this dialog, as one might imagine, is to get
//               string input for any purpose necessary.
//               The recomended way to call this dialog is through DoModal() method.
//
//               Usage Hint: rename dialog.

#ifndef CRYINCLUDE_EDITOR_DIALOGS_GENERIC_STRINGINPUTDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGS_GENERIC_STRINGINPUTDIALOG_H
#pragma once


class CStringInputDialog
    : public CDialog
{
    //////////////////////////////////////////////////////////////////////////
    // Types & Typedefs
public:
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Methods
public:
    CStringInputDialog();
    CStringInputDialog(CString strText, CString strTitle);
    void DoDataExchange(CDataExchange* pDX);
    BOOL OnInitDialog();
    void OnOK();

    void SetText(CString strText);
    void SetTitle(CString strTitle);

    CString GetResultingText();
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Data fields
public:
protected:
    CString   m_strTitle;
    CString   m_strText;
    CEdit     m_nameEdit;
private:
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_EDITOR_DIALOGS_GENERIC_STRINGINPUTDIALOG_H
