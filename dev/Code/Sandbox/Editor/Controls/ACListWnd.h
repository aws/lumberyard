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

// Description : List window for an auto-completion/history suggestion
// Notice      : Used in CACEdit


#ifndef CRYINCLUDE_EDITOR_CONTROLS_ACLISTWND_H
#define CRYINCLUDE_EDITOR_CONTROLS_ACLISTWND_H
#pragma once

// ACWnd.h : Header-Datei
//

/*********************************************************************
*
* CACListWnd
* Copyright (c) 200 by Andreas Kapust
* All rights reserved.
* info@akinstaller.de
*
*********************************************************************/

#include <afxtempl.h>       // CArray
#include "UserMessageDefines.h"

/////////////////////////////////////////////////////////////////////////////
// Window CACListWnd
#define IDTimerInstall 10
class CACListWnd
    : public CWnd
{
public:
    CACListWnd();
    void Init(CWnd* pWnd);
    bool EnsureVisible(int item, bool m_bWait);
    bool SelectItem(int item);
    int FindString(int nStartAfter, LPCTSTR lpszString, bool m_bDisplayOnly = false);
    int FindStringExact(int nIndexStart, LPCTSTR lpszFind);
    int SelectString(LPCTSTR lpszString);
    bool GetText(int item, CString& m_Text);
    void AddSearchString(LPCTSTR lpszString){m_SearchList.Add(lpszString); }
    void AddHistoryString(LPCTSTR lpszString);
    void RemoveAll(){m_SearchList.RemoveAll(); m_DisplayList.RemoveAll(); }
    CString GetString();
    CString GetNextString(int m_iChar);

    void CopyList();
    void SortSearchList(){SortList(m_SearchList); }

    void SetHistoryCount(int count)
    { m_nHistory = count; }

    // Attributes
public:
    CListCtrl m_List;
    CString m_DisplayStr;
    TCHAR m_PrefixChar;
    long m_lMode;
    // Functions
public:
    CStringArray m_SearchList;
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CACListWnd)
    //}}AFX_VIRTUAL

    // Implementierung
public:
    virtual ~CACListWnd();
    void DrawItem(CDC* pDC, long m_lItem, long width);

    // Generated Message map functions
protected:
    //{{AFX_MSG(CACListWnd)
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnNcPaint();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CStringArray m_DisplayList;
    CScrollBar m_VertBar, m_HoriBar;
    CRect m_LastSize, m_ParentRect;
    CFont* pFontDC;
    CFont fontDC, boldFontDC;
    CEdit* m_pEditParent;
    LOGFONT logfont;

    int m_nIDTimer;
    long m_lTopIndex, m_lCount, m_ItemHeight, m_VisibleItems, m_lSelItem;
    int m_nHistory;

    int HitTest(CPoint point);
    void SetScroller();
    void SetProp();
    long ScrollBarWidth();
    void InvalidateAndScroll();
    void SortList(CStringArray& m_List);
    static int CompareString(const void* p1, const void* p2);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ adds this immediately before the previous line additional declarations.

#endif // CRYINCLUDE_EDITOR_CONTROLS_ACLISTWND_H
