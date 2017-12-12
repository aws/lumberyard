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
//               progress display  The purpose of this dialog  as one might imagine  is to
//               display the progress of any process


#ifndef CRYINCLUDE_EDITOR_DIALOGS_GENERIC_PROGRESSDIALOG_H
#define CRYINCLUDE_EDITOR_DIALOGS_GENERIC_PROGRESSDIALOG_H
#pragma once


class CProgressDialog
    : public CDialog
{
    //////////////////////////////////////////////////////////////////////////
    // Types & Typedefs
public:
    typedef Functor0        TDCancelCallback;
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Methods
public:
    CProgressDialog(CWnd*   poParentWindow = NULL);
    CProgressDialog(CString strText, CString strTitle, bool bMarquee = false, int nMinimum = 0, int nMaximum = 100, CWnd*  poParentWindow = NULL);

    BOOL    Create();

    void DoDataExchange(CDataExchange* pDX);
    BOOL OnInitDialog();

    afx_msg void OnCancel();

    void SetRange(int nMinimum, int nMaximum);
    void SetPosition(int nPosition);
    void SetMarquee(bool onOff, int interval);
    void SetText(CString strText);
    void SetTitle(CString strTitle);
    void SetCancelCallback(TDCancelCallback pfnCancelCallback);


    DECLARE_MESSAGE_MAP();
    //}}AFX_MSG
protected:
private:
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    // Data fields
public:
protected:
    CString                         m_strTitle;
    CString                         m_strText;

    CProgressCtrl               m_oProgressControl;
    CButton                         m_oCancelButton;

    int                                 m_nMinimum;
    int                                 m_nMaximum;
    int                                 m_nPosition;

    TDCancelCallback        m_pfnCancelCallback;

    bool                                m_boCreated;
private:
    //////////////////////////////////////////////////////////////////////////
};


#endif // CRYINCLUDE_EDITOR_DIALOGS_GENERIC_PROGRESSDIALOG_H
