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

// Description : Terrain modification tool.


#ifndef CRYINCLUDE_EDITOR_TASKPANEL_H
#define CRYINCLUDE_EDITOR_TASKPANEL_H

#pragma once



//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CTaskPanel
    : public CDialog
{
    DECLARE_DYNCREATE(CTaskPanel);

public:
    CTaskPanel(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTaskPanel();

    // Dialog Data
    enum
    {
        IDD = IDD_PANEL_PROPERTIES
    };

protected:
    virtual void OnOK() {};
    virtual void OnCancel() {};
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg LRESULT OnTaskPanelNotify(WPARAM wParam, LPARAM lParam);

    void UpdateTools();

    DECLARE_MESSAGE_MAP()

    CXTPTaskPanel m_wndTaskPanel;
    class CTaskPanelTheme* m_pTheme;
};

#endif // CRYINCLUDE_EDITOR_TASKPANEL_H
