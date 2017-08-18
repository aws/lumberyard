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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_LISTCTRLEX_H
#define CRYINCLUDE_EDITOR_CONTROLS_LISTCTRLEX_H
#pragma once

#include "HeaderCtrlEx.h"

//////////////////////////////////////////////////////////////////////////
class CListCtrlEx
    : public CXTListCtrl
{
    // Construction
public:
    CListCtrlEx();

public:
    void RepositionControls();
    void InsertSomeItems();
    void CreateColumns();
    virtual ~CListCtrlEx();

    CWnd* GetItemControl(int nIndex, int nColumn);
    void  SetItemControl(int nIndex, int nColumn, CWnd* pWnd);
    BOOL    DeleteAllItems();

    // Generated message map functions
protected:
    virtual BOOL SubclassWindow(HWND hWnd);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
    afx_msg LRESULT OnNotifyMsg(WPARAM wParam, LPARAM lParam);
    afx_msg void OnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

    //////////////////////////////////////////////////////////////////////////
    typedef std::map<int, CWnd*> ControlsMap;
    ControlsMap m_controlsMap;

    //CHeaderCtrlEx m_headerCtrl;
};


//////////////////////////////////////////////////////////////////////////
class CControlsListBox
    : public CListBox
{
    // Construction
public:
    CControlsListBox();
    virtual ~CControlsListBox();

public:
    void RepositionControls();

    CWnd* GetItemControl(int nIndex);
    void SetItemControl(int nIndex, CWnd* pWnd);
    void ResetContent();

    // Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
    afx_msg LRESULT OnNotifyMsg(WPARAM wParam, LPARAM lParam);
    afx_msg void OnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult);

    //////////////////////////////////////////////////////////////////////////
    typedef std::map<int, CWnd*> ControlsMap;
    ControlsMap m_controlsMap;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_LISTCTRLEX_H
