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
//               overwrite confirmation  The purpose of this dialog  as one might imagine  is to
//               get check if the user really wants to overwrite some item  allowing him her to
//               apply the same choices to all the remaining items
//
//               The recomended way to call this dialog is through DoModal() method.
//
// Usage Hint  : Use the UserOptions.h file.



#ifndef CRYINCLUDE_EDITOR_DIALOGS_GENERIC_GENERICOVERWRITEDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGS_GENERIC_GENERICOVERWRITEDIALOG_H
#pragma once


class CGenericOverwriteDialog
    : public CDialog
{
    //////////////////////////////////////////////////////////////////////////
    // Types & typedefs
public:
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Methods
public:
    CGenericOverwriteDialog(const CString& strTitle, const CString& strText, bool boMultipleFiles = true, CWnd* parent = nullptr);
    void DoDataExchange(CDataExchange* pDX);
    BOOL OnInitDialog();
    void OnYes();
    void OnNo();
    void OnCancel();

    // Call THIS method to start this dialog.
    // The return values can be IDYES, IDNO and IDCANCEL
    INT_PTR DoModal();

    bool IsToAllToggled();

    DECLARE_MESSAGE_MAP();
    //}}AFX_MSG
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Data
public:
protected:
    CString   m_strTitle;
    CString   m_strText;
    bool            m_boToAll;

    bool            m_boMultipleFiles;
    CStatic     m_strTextMessage;
    CButton     m_oToAll;

    bool            m_boIsModal;
private:
    //////////////////////////////////////////////////////////////////////////
};


#endif // CRYINCLUDE_EDITOR_DIALOGS_GENERIC_GENERICOVERWRITEDIALOG_H
