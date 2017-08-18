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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_TABCTRLEX_H
#define CRYINCLUDE_EDITOR_CONTROLS_TABCTRLEX_H
#pragma once


//////////////////////////////////////////////////////////////////////////
class CTabCtrlEx
    : public CTabCtrl
{
    DECLARE_DYNCREATE(CTabCtrlEx);

public:
    CTabCtrlEx();
    virtual ~CTabCtrlEx();

    int AddPage(const char* sCaption, CWnd* pWnd, bool bAutoDestroy = true, bool bKeepWndHeight = false);
    void RemovePage(int nPageID);
    CWnd* SelectPage(int nPageId);

    CWnd* SelectPageByIndex(int index);

    // Dialog Data
    enum
    {
        IDD = IDD_PANEL_PROPERTIES
    };

protected:

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTabSelect(NMHDR* pNMHDR, LRESULT* pResult);
    void RepositionPages();

    DECLARE_MESSAGE_MAP()

    struct SPage
    {
        int nId;
        CWnd* pWnd;
        bool bAutoDestroy;
        bool bKeepHeight;
        SPage()
            : pWnd(0)
            , bAutoDestroy(false)
            , bKeepHeight(false) {}
    };
    std::vector<SPage> m_pages;
    int m_selectedCtrl;
    int m_lastID;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TABCTRLEX_H
