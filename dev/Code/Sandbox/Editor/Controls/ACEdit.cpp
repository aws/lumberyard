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
// Notice      : Based on http:  www.codeproject.com KB combobox akautocomplete.aspx


#include "stdafx.h"
#include "ACEdit.h"
#include  <io.h>

#include "QtUtil.h"

#define _EDIT_ 1
#define _COMBOBOX_ 2

/////////////////////////////////////////////////////////////////////////////
// CACEdit

CACEdit::CACEdit()
{
    m_iMode = _MODE_STANDARD_;
    m_iType = -1;
    m_pEdit = NULL;
    m_CursorMode = false;
    m_PrefixChar = 0;
}

/*********************************************************************/

CACEdit::~CACEdit()
{
    DestroyWindow();
}

/*********************************************************************/

BEGIN_MESSAGE_MAP(CACEdit, CWnd)
//{{AFX_MSG_MAP(CACEdit)
ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
ON_CONTROL_REFLECT(CBN_KILLFOCUS, OnKillfocus)
ON_WM_KEYDOWN()
ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
ON_CONTROL_REFLECT(CBN_EDITCHANGE, OnChange)
ON_CONTROL_REFLECT(CBN_DROPDOWN, OnCloseList)
//}}AFX_MSG_MAP
ON_MESSAGE(ENAC_UPDATE, OnUpdateFromList)
END_MESSAGE_MAP()

/*********************************************************************/

void CACEdit::SetMode(int iMode)
{
    if (m_iType == -1)
    {
        Init();
    }

    m_iMode = iMode;

    /*
    ** Vers. 1.1
    ** NEW: _MODE_CURSOR_O_LIST_
    */
    if (iMode == _MODE_CURSOR_O_LIST_)
    {
        m_iMode |= _MODE_STANDARD_;
        m_iMode &= ~_MODE_HISTORY_;
    }

    if (iMode == _MODE_HISTORY_)
    {
        m_iMode |= _MODE_STANDARD_;
        m_iMode &= ~_MODE_CURSOR_O_LIST_;
    }

    if (iMode & _MODE_FILESYSTEM_)
    {
        m_SeparationStr = _T("\\");
    }

    // Vers. 1.2
    if (iMode & _MODE_FIND_ALL_)
    {
        m_Liste.m_lMode |= _MODE_FIND_ALL_;
    }
}

/*********************************************************************/

void CACEdit::Init()
{
    CString szClassName = AfxRegisterWndClass(CS_CLASSDC | CS_SAVEBITS | CS_HREDRAW | CS_VREDRAW,
            0, (HBRUSH) (COLOR_WINDOW), 0);
    CRect rcWnd, rcWnd1;
    GetWindowRect(rcWnd);

    VERIFY(m_Liste.CreateEx(WS_EX_TOOLWINDOW,
            szClassName, NULL,
            WS_THICKFRAME | WS_CHILD | WS_BORDER |
            WS_CLIPSIBLINGS | WS_OVERLAPPED,
            CRect(rcWnd.left, rcWnd.top + 20, rcWnd.left + 200, rcWnd.top + 200),
            GetDesktopWindow(),
            0x3E8, NULL));

    VERIFY(m_HistoryListe.CreateEx(WS_EX_TOOLWINDOW,
            szClassName, NULL,
            WS_THICKFRAME | WS_CHILD | WS_BORDER |
            WS_CLIPSIBLINGS | WS_OVERLAPPED,
            CRect(rcWnd.left, rcWnd.top + 20, rcWnd.left + 200, rcWnd.top + 200),
            GetDesktopWindow(),
            0x3E9, NULL));

    CString m_ClassName;
    ::GetClassName(GetSafeHwnd(), m_ClassName.GetBuffer(32), 32);
    m_ClassName.ReleaseBuffer();

    if (m_ClassName.Compare(_T("Edit")) == 0)
    {
        m_iType = _EDIT_;
    }
    else
    {
        if (m_ClassName.Compare(_T("ComboBox")) == 0)
        {
            m_iType = _COMBOBOX_;

            m_pEdit = (CEdit*)GetWindow(GW_CHILD);
            VERIFY(m_pEdit);
            ::GetClassName(m_pEdit->GetSafeHwnd(), m_ClassName.GetBuffer(32), 32);
            m_ClassName.ReleaseBuffer();
            VERIFY(m_ClassName.Compare(_T("Edit")) == 0);
        }
    }

    if (m_iType == -1)
    {
        assert(0);
        return;
    }

    m_Liste.Init(this);
    m_HistoryListe.Init(this);
    m_HistoryListe.m_lMode |= _MODE_HISTORY_;
}

/*********************************************************************/

void CACEdit::AddSearchStrings(LPCTSTR Strings[])
{
    int i = 0;
    LPCTSTR str;
    if (m_iType == -1)
    {
        assert(0);
        return;
    }

    m_Liste.RemoveAll();

    do
    {
        str = Strings[i];
        if (str)
        {
            m_Liste.AddSearchString(str);
        }

        i++;
    }
    while (str);

    m_Liste.SortSearchList();
}

/*********************************************************************/

void CACEdit::AddSearchString(LPCTSTR lpszString)
{
    if (m_iType == -1)
    {
        assert(0);
        return;
    }

    m_Liste.AddSearchString(lpszString);
}

/*********************************************************************/

void CACEdit::RemoveSearchAll()
{
    if (m_iType == -1)
    {
        assert(0);
        return;
    }

    m_Liste.RemoveAll();
}

/*********************************************************************/

void CACEdit::AddHistoryString(LPCTSTR lpszString)
{
    if (m_iType == -1)
    {
        assert(0);
        return;
    }
    if ((m_iMode & _MODE_HISTORY_) == 0)
    {
        assert(0);
        return;
    }

    m_HistoryListe.AddHistoryString(lpszString);
}

/*********************************************************************/

void CACEdit::RemoveHistoryAll()
{
    if (m_iType == -1)
    {
        assert(0);
        return;
    }
    if ((m_iMode & _MODE_HISTORY_) == 0)
    {
        assert(0);
        return;
    }

    m_HistoryListe.RemoveAll();
}

/*********************************************************************/

void CACEdit::SetHistoryCount(int count)
{
    if (m_iType == -1)
    {
        assert(0);
        return;
    }
    if ((m_iMode & _MODE_HISTORY_) == 0)
    {
        assert(0);
        return;
    }

    m_HistoryListe.SetHistoryCount(count);
}

/*********************************************************************/

void CACEdit::OnKillfocus()
{
    if (m_Liste.GetSafeHwnd()) // fix Vers 1.1
    {
        m_Liste.ShowWindow(false);
    }
    if (m_HistoryListe.GetSafeHwnd())
    {
        m_HistoryListe.ShowWindow(false);
    }
}

/*********************************************************************/

void CACEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (!HandleKey(nChar, false))
    {
        CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
    }
}

/*********************************************************************/

bool CACEdit::HandleKey(UINT nChar, bool m_bFromChild)
{
    if (nChar == VK_ESCAPE || nChar == VK_RETURN)
    {
        m_Liste.ShowWindow(false);
        m_HistoryListe.ShowWindow(false);
        return true;
    }

    if (nChar == VK_DOWN || nChar == VK_UP
        || nChar == VK_PRIOR || nChar == VK_NEXT)
    {
        // _MODE_HISTORY_
        if (!m_Liste.IsWindowVisible()
            && !m_HistoryListe.IsWindowVisible() && (m_iMode & _MODE_HISTORY_))
        {
            m_HistoryListe.CopyList();
            return true;
        }

        if (m_HistoryListe.IsWindowVisible())
        {
            int pos;


            if (m_iMode & _MODE_STANDARD_
                || m_iMode & _MODE_FILESYSTEM_
                || m_iMode & _MODE_FS_START_DIR_)
            {
                m_CursorMode = true;

                if (!m_bFromChild)
                {
                    m_EditText = m_HistoryListe.GetNextString(nChar);
                }
                else
                {
                    m_EditText = m_HistoryListe.GetString();
                }

                if (m_iMode & _MODE_FILESYSTEM_)
                {
                    if (m_EditText.Right(1) == _T('\\'))
                    {
                        m_EditText = m_EditText.Mid(0, m_EditText.GetLength() - 1);
                    }
                }

                m_HistoryListe.SelectItem(-1);
                SetWindowText(m_EditText);
                pos = m_EditText.GetLength();

                if (m_iType == _COMBOBOX_)
                {
                    m_pEdit->SetSel(pos, pos, true);
                    m_pEdit->SetModify(true);
                }

                if (m_iType == _EDIT_)
                {
                    ((CEdit*)this)->SetSel(pos, pos, true);
                    ((CEdit*)this)->SetModify(true);
                }

                GetParent()->SendMessage(ENAC_UPDATE, WM_KEYDOWN, GetDlgCtrlID());
                m_CursorMode = false;
                return true;
            }
        }

        /*
        ** Vers. 1.1
        ** NEW: _MODE_CURSOR_O_LIST_
        */
        if (!m_Liste.IsWindowVisible() && (m_iMode & _MODE_CURSOR_O_LIST_))
        {
            GetWindowText(m_EditText);
            if (m_EditText.IsEmpty())
            {
                m_Liste.CopyList();
                return true;
            }
        }

        if (m_Liste.IsWindowVisible())
        {
            int pos;


            if (m_iMode & _MODE_STANDARD_
                || m_iMode & _MODE_FILESYSTEM_
                || m_iMode & _MODE_FS_START_DIR_)
            {
                m_CursorMode = true;

                if (!m_bFromChild)
                {
                    m_EditText = m_Liste.GetNextString(nChar);
                }
                else
                {
                    m_EditText = m_Liste.GetString();
                }

                if (m_iMode & _MODE_FILESYSTEM_)
                {
                    if (m_EditText.Right(1) == _T('\\'))
                    {
                        m_EditText = m_EditText.Mid(0, m_EditText.GetLength() - 1);
                    }
                }

                m_Liste.SelectItem(-1);
                SetWindowText(m_EditText);
                pos = m_EditText.GetLength();

                if (m_iType == _COMBOBOX_)
                {
                    m_pEdit->SetSel(pos, pos, true);
                    m_pEdit->SetModify(true);
                }

                if (m_iType == _EDIT_)
                {
                    ((CEdit*)this)->SetSel(pos, pos, true);
                    ((CEdit*)this)->SetModify(true);
                }

                GetParent()->SendMessage(ENAC_UPDATE, WM_KEYDOWN, GetDlgCtrlID());
                m_CursorMode = false;
                return true;
            }

            if (m_iMode & _MODE_SEPARATION_)
            {
                CString m_Text, m_Left, m_Right;
                int left, right, pos = 0, len;

                m_CursorMode = true;

                GetWindowText(m_EditText);

                if (m_iType == _EDIT_)
                {
                    pos = LOWORD(((CEdit*)this)->CharFromPos(GetCaretPos()));
                }

                if (m_iType == _COMBOBOX_)
                {
                    pos = m_pEdit->CharFromPos(m_pEdit->GetCaretPos());
                }

                left  = FindSepLeftPos(pos - 1, true);
                right = FindSepRightPos(pos);

                m_Text = m_EditText.Left(left);

                if (!m_bFromChild)
                {
                    m_Text += m_Liste.GetNextString(nChar);
                }
                else
                {
                    m_Text += m_Liste.GetString();
                }

                m_Liste.SelectItem(-1);
                m_Text += m_EditText.Mid(right);
                len = m_Liste.GetString().GetLength();

                SetWindowText(m_Text);
                GetParent()->SendMessage(ENAC_UPDATE, WM_KEYDOWN, GetDlgCtrlID());

                right = FindSepLeftPos2(pos - 1);
                left -= right;
                len += right;

                if (m_iType == _EDIT_)
                {
                    ((CEdit*)this)->SetModify(true);
                    ((CEdit*)this)->SetSel(left, left + len, false);
                }

                if (m_iType == _COMBOBOX_)
                {
                    m_pEdit->SetModify(true);
                    m_pEdit->SetSel(left, left + len, true);
                }

                m_CursorMode = false;
                return true;
            }
        }
    }
    return false;
}

/*********************************************************************/

void CACEdit::OnChange()
{
    CString m_Text;
    int pos = 0, len;

    if (m_iType == -1)
    {
        assert(0);
        return;
    }

    if (!m_CursorMode)
    {
        m_HistoryListe.ShowWindow(false);
    }

    GetWindowText(m_EditText);
    len = m_EditText.GetLength();
    //----------------------------------------------
    if (m_iMode & _MODE_FILESYSTEM_ || m_iMode & _MODE_FS_START_DIR_)
    {
        if (!m_CursorMode)
        {
            if (m_iType == _EDIT_)
            {
                pos = LOWORD(((CEdit*)this)->CharFromPos(GetCaretPos()));
            }

            if (m_iType == _COMBOBOX_)
            {
                pos = m_pEdit->CharFromPos(m_pEdit->GetCaretPos());
            }

            if (m_iMode & _MODE_FS_START_DIR_)
            {
                if (len)
                {
                    m_Liste.FindString(-1, m_EditText);
                }
                else
                {
                    m_Liste.ShowWindow(false);
                }
            }
            else
            {
                if (len > 2 && pos == len)
                {
                    if (_access(m_EditText, 0) == 0)
                    {
                        ReadDirectory(m_EditText);
                    }
                    m_Liste.FindString(-1, m_EditText);
                }
                else
                {
                    m_Liste.ShowWindow(false);
                }
            }
        } // m_CursorMode
    }
    //----------------------------------------------
    if (m_iMode & _MODE_SEPARATION_)
    {
        if (!m_CursorMode)
        {
            if (m_iType == _EDIT_)
            {
                pos = LOWORD(((CEdit*)this)->CharFromPos(GetCaretPos()));
            }

            if (m_iType == _COMBOBOX_)
            {
                pos = m_pEdit->CharFromPos(m_pEdit->GetCaretPos());
            }

            int left, right;
            left  = FindSepLeftPos(pos - 1);
            right = FindSepRightPos(pos);
            m_Text = m_EditText.Mid(left, right - left);
            m_Liste.FindString(-1, m_Text);
        }
    }
    //----------------------------------------------
    if (m_iMode & _MODE_STANDARD_)
    {
        if (!m_CursorMode)
        {
            m_Liste.FindString(-1, m_EditText);
        }
    }
    //----------------------------------------------
    GetParent()->SendMessage(ENAC_UPDATE, EN_UPDATE, GetDlgCtrlID());
}

/*********************************************************************/

int CACEdit::FindSepLeftPos(int pos, bool m_bIncludePrefix)
{
    int len = m_EditText.GetLength();
    TCHAR ch;

    if (pos >= len && len != 1)
    {
        pos =  len - 1;
    }

    int i = 0;
    for (i = pos; i >= 0; i--)
    {
        ch = m_EditText.GetAt(i);
        if (m_PrefixChar == ch)
        {
            return i + (m_bIncludePrefix ? 1 : 0);
        }
        if (m_SeparationStr.Find(ch) != -1)
        {
            break;
        }
    }

    return i + 1;
}

/*********************************************************************/

int CACEdit::FindSepLeftPos2(int pos)
{
    int len = m_EditText.GetLength();
    TCHAR ch;

    if (pos >= len && len != 1)
    {
        pos =  len - 1;
    }

    if (len == 1)
    {
        return 0;
    }

    for (int i = pos; i >= 0; i--)
    {
        ch = m_EditText.GetAt(i);
        if (m_PrefixChar == ch)
        {
            return 1;
        }
    }

    return 0;
}

/*********************************************************************/

int CACEdit::FindSepRightPos(int pos)
{
    int len = m_EditText.GetLength();
    TCHAR ch;

    int i = 0;
    for (i = pos; i < len; i++)
    {
        ch = m_EditText.GetAt(i);
        if (m_SeparationStr.Find(ch) != -1)
        {
            break;
        }
    }

    return i;
}

/*********************************************************************/

LRESULT CACEdit::OnUpdateFromList(WPARAM wParam, LPARAM /*lParam*/)
{
    UpdateData(true);

    if (wParam == WM_KEYDOWN)
    {
        HandleKey(VK_DOWN, true);
    }
    return 0;
}

/*********************************************************************/

void CACEdit::OnCloseList()
{
    m_Liste.ShowWindow(false);
}

/*********************************************************************/

BOOL CACEdit::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        if (m_Liste.IsWindowVisible()
            || m_HistoryListe.IsWindowVisible())
        {
            if (m_iType == _COMBOBOX_)
            {
                if (pMsg->wParam == VK_DOWN || pMsg->wParam == VK_UP)
                {
                    if (HandleKey(pMsg->wParam, false))
                    {
                        return true;
                    }
                }
            }

            if (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN)
            {
                if (HandleKey(pMsg->wParam, false))
                {
                    return true;
                }
            }
        }
    }
    return CWnd::PreTranslateMessage(pMsg);
}

/*********************************************************************/

void CACEdit::ReadDirectory(CString m_Dir)
{
    CFileFind FoundFiles;
    TCHAR ch;
    QWaitCursor hg;

    // if file in path,
    // read previous directory
    if (m_Dir.Right(1) != _T('\\'))
    {
        _splitpath(m_Dir, m_szDrive, m_szDir, m_szFname, m_szExt);
        m_Dir.Format("%s%s", m_szDrive, m_szDir);
    }

    // force upper case
    ch = (TCHAR)towupper(m_Dir.GetAt(0));
    m_Dir.SetAt(0, ch);

    CString m_Name, m_File, m_Dir1 = m_Dir;
    if (m_Dir.Right(1) != _T('\\'))
    {
        m_Dir += _T("\\");
    }

    if (m_LastDirectory.CompareNoCase(m_Dir) == 0 && m_Liste.m_SearchList.GetSize())
    {
        return;
    }

    m_LastDirectory = m_Dir;
    m_Dir += _T("*.*");

    BOOL bContinue = FoundFiles.FindFile(m_Dir);
    if (bContinue)
    {
        RemoveSearchAll();
    }

    while (bContinue == TRUE)
    {
        bContinue = FoundFiles.FindNextFile();
        m_File = FoundFiles.GetFileName();

        if (FoundFiles.IsHidden() || FoundFiles.IsSystem())
        {
            continue;
        }
        if (FoundFiles.IsDirectory())
        {
            if (m_iMode & _MODE_ONLY_FILES)
            {
                continue;
            }
            if (FoundFiles.IsDots())
            {
                continue;
            }

            if (m_File.Right(1) != _T('\\'))
            {
                m_File += _T("\\");
            }
        }

        if (!FoundFiles.IsDirectory())
        {
            if (m_iMode & _MODE_ONLY_DIRS)
            {
                continue;
            }
        }

        if (m_iMode & _MODE_FS_START_DIR_)
        {
            m_Name = m_File;
        }
        else
        {
            m_Name = m_Dir1;
            if (m_Name.Right(1) != _T('\\'))
            {
                m_Name += _T("\\");
            }

            m_Name += m_File;
        }

        AddSearchString(m_Name);
    }
    FoundFiles.Close();
    return;
}

/*********************************************************************/

void CACEdit::SetStartDirectory(LPCTSTR lpszString)
{
    if (m_iType == -1)
    {
        assert(0);
        return;
    }

    if (m_iMode & _MODE_FS_START_DIR_)
    {
        ReadDirectory(lpszString);
    }
}

/*********************************************************************
** CComboBox
** NEW:V1.1
*********************************************************************/

int CACEdit::AddString(LPCTSTR lpszString)
{
    if (m_iType == _COMBOBOX_)
    {
        return ((CComboBox*)this)->AddString(lpszString);
    }
    return CB_ERR;
}

/*********************************************************************/

int CACEdit::SetDroppedWidth(UINT nWidth)
{
    if (m_iType == _COMBOBOX_)
    {
        return ((CComboBox*)this)->SetDroppedWidth(nWidth);
    }
    return CB_ERR;
}

/*********************************************************************/

int CACEdit::FindString(int nStartAfter, LPCTSTR lpszString)
{
    if (m_iType == _COMBOBOX_)
    {
        return ((CComboBox*)this)->FindString(nStartAfter, lpszString);
    }
    return CB_ERR;
}

/*********************************************************************/

int CACEdit::SelectString(int nStartAfter, LPCTSTR lpszString)
{
    if (m_iType == _COMBOBOX_)
    {
        return ((CComboBox*)this)->SelectString(nStartAfter, lpszString);
    }
    return CB_ERR;
}

/*********************************************************************/

void CACEdit::ShowDropDown(BOOL bShowIt)
{
    if (m_iType == _COMBOBOX_)
    {
        ((CComboBox*)this)->ShowDropDown(bShowIt);
    }
}

/*********************************************************************/

void CACEdit::ResetContent()
{
    if (m_iType == _COMBOBOX_)
    {
        ((CComboBox*)this)->ResetContent();
    }
}

/*********************************************************************/

int CACEdit::GetCurSel()
{
    if (m_iType == _COMBOBOX_)
    {
        return ((CComboBox*)this)->GetCurSel();
    }
    return CB_ERR;
}

/*********************************************************************/

int CACEdit::GetLBText(int nIndex, LPTSTR lpszText)
{
    if (m_iType == _COMBOBOX_)
    {
        return ((CComboBox*)this)->GetLBText(nIndex, lpszText);
    }
    return CB_ERR;
}

/*********************************************************************/

void CACEdit::GetLBText(int nIndex, CString& rString)
{
    if (m_iType == _COMBOBOX_)
    {
        ((CComboBox*)this)->GetLBText(nIndex, rString);
    }
}

/*********************************************************************/
