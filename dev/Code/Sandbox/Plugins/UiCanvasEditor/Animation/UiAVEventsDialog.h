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

#pragma once


#include "afxcmn.h"
#include "Resource.h"
#include <LyShine/Animation/IUiAnimation.h>

// CUiAVEventsDialog dialog

class CUiAVEventsDialog
    : public CDialog
{
    DECLARE_DYNAMIC(CUiAVEventsDialog)

public:
    CUiAVEventsDialog(CWnd* pParent = NULL);   // standard constructor
    virtual ~CUiAVEventsDialog();

    // Dialog Data
    enum
    {
        IDD = IDD_TV_EVENTS
    };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonAddEvent();
    afx_msg void OnBnClickedButtonRemoveEvent();
    afx_msg void OnBnClickedButtonRenameEvent();
    afx_msg void OnBnClickedButtonUpEvent();
    afx_msg void OnBnClickedButtonDownEvent();
    afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

protected:
    virtual BOOL OnInitDialog();

    void UpdateButtons();

private:
    // list of events
    CListCtrl m_List;

    int GetNumberOfUsageAndFirstTimeUsed(const char* eventName, float& timeFirstUsed) const;
};
