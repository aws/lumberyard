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

// Description : Edit control with a built-in auto-completion/history support
//
// Notice      : Based on http   www codeproject com KB combobox akautocomplete aspx


#ifndef CRYINCLUDE_EDITOR_CONTROLS_ACEDIT_H
#define CRYINCLUDE_EDITOR_CONTROLS_ACEDIT_H
#pragma once

// ACEdit.h : Header-Datei
//

/*********************************************************************
*
* CACEdit
* Copyright (c) 2003 by Andreas Kapust
* All rights reserved.
* info@akinstaller.de
*
*********************************************************************/


#define _MODE_ONLY_FILES        (1L << 16)
#define _MODE_ONLY_DIRS         (1L << 17)

#define _MODE_STANDARD_         (1L << 0)
#define _MODE_SEPARATION_       (1L << 1)
#define _MODE_FILESYSTEM_       (1L << 2)
#define _MODE_FS_START_DIR_     (1L << 3)
#define _MODE_CURSOR_O_LIST_    (1L << 4)
#define _MODE_FIND_ALL_         (1L << 5)
#define _MODE_HISTORY_          (1L << 6)

#define _MODE_FS_ONLY_FILE_ (_MODE_FILESYSTEM_ | _MODE_ONLY_FILES)
#define _MODE_FS_ONLY_DIR_  (_MODE_FILESYSTEM_ | _MODE_ONLY_DIRS)
#define _MODE_SD_ONLY_FILE_ (_MODE_FS_START_DIR_ | _MODE_ONLY_FILES)
#define _MODE_SD_ONLY_DIR_  (_MODE_FS_START_DIR_ | _MODE_ONLY_DIRS)  //Fix 1.2

/////////////////////////////////////////////////////////////////////////////
// Fenster CACEdit
#include "ACListWnd.h"


class CACEdit
    : public CWnd           //CEdit
{
    // Konstruktion
public:
    CACEdit();
    void SetMode(int iMode = _MODE_STANDARD_);
    void SetSeparator(LPCTSTR lpszString, TCHAR lpszPrefixChar = 0)
    {
        m_SeparationStr = lpszString;
        m_Liste.m_PrefixChar = m_PrefixChar = lpszPrefixChar;
        SetMode(_MODE_SEPARATION_);
    }

    // CComboBox
    int AddString(LPCTSTR lpszString);
    int GetLBText(int nIndex, LPTSTR lpszText);
    void GetLBText(int nIndex, CString& rString);
    int SetDroppedWidth(UINT nWidth);
    int FindString(int nStartAfter, LPCTSTR lpszString);
    int SelectString(int nStartAfter, LPCTSTR lpszString);
    void ShowDropDown(BOOL bShowIt = TRUE);
    void ResetContent();
    int GetCurSel();
    // Attribute
public:
    void Init();
    void AddSearchString(LPCTSTR lpszString);
    void AddSearchStrings(LPCTSTR Strings[]);
    void RemoveSearchAll();
    void AddHistoryString(LPCTSTR lpszString);
    void RemoveHistoryAll();
    void SetHistoryCount(int count);
    void SetStartDirectory(LPCTSTR lpszString);
    // Operationen
public:

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CACEdit)
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CACEdit();
    CACListWnd m_Liste;
    CACListWnd m_HistoryListe;
    // Generated message map functions
protected:
    CString m_EditText, m_SeparationStr, m_LastDirectory;
    TCHAR m_PrefixChar;
    int m_iMode;
    //{{AFX_MSG(CACEdit)
    afx_msg void OnKillfocus();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChange();
    afx_msg void OnCloseList();
    //}}AFX_MSG
    afx_msg LRESULT OnUpdateFromList(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()


    void ReadDirectory(CString m_Dir);
    int FindSepLeftPos(int pos, bool FindSepLeftPos = false);
    int FindSepLeftPos2(int pos);
    int FindSepRightPos(int pos);
    bool HandleKey(UINT nChar, bool m_bFromChild);

    bool m_CursorMode;
    int m_iType;
    CEdit* m_pEdit;

    char m_szDrive[_MAX_DRIVE], m_szDir[_MAX_DIR], m_szFname[_MAX_FNAME], m_szExt[_MAX_EXT];
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_ACEDIT_H
