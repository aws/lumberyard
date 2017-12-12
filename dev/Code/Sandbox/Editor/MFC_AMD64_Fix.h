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

// Description : Fixes of problems using MFC on AMD64 machines.


#ifndef CRYINCLUDE_EDITOR_MFC_AMD64_FIX_H
#define CRYINCLUDE_EDITOR_MFC_AMD64_FIX_H
#pragma once

//////////////////////////////////////////////////////////////////////////
// In case of AMD64 overcome crash bug when using CFileDialog.
//////////////////////////////////////////////////////////////////////////
#ifdef WIN64
class CCryFileDialog
{
public:
    ////////////////////////////////////////////////////////////////////////////
    // FileOpen/FileSaveAs common dialog helper
    OPENFILENAME m_ofn;
    BOOL m_bOpenFileDialog;       // TRUE for file open, FALSE for file save
    CString m_strFilter;          // filter string
    // separate fields with '|', terminate with '||\0'
    TCHAR m_szFileTitle[64];       // contains file title after return
    TCHAR m_szFileName[_MAX_PATH]; // contains full path name after return

    explicit CCryFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
        LPCTSTR lpszDefExt = NULL,
        LPCTSTR lpszFileName = NULL,
        DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        LPCTSTR lpszFilter = NULL,
        CWnd* pParentWnd = NULL,
        DWORD dwSize = sizeof(OPENFILENAME))
    {
        memset(&m_ofn, 0, sizeof(m_ofn)); // initialize structure to 0/NULL
        m_szFileName[0] = '\0';
        m_szFileTitle[0] = '\0';
        m_bOpenFileDialog = bOpenFileDialog;

        m_ofn.lStructSize = sizeof(m_ofn);
        m_ofn.lpstrFile = m_szFileName;
        m_ofn.nMaxFile = sizeof(m_szFileName);
        m_ofn.lpstrDefExt = lpszDefExt;
        m_ofn.lpstrFileTitle = (LPTSTR)m_szFileTitle;
        m_ofn.nMaxFileTitle = sizeof(m_szFileTitle);
        m_ofn.Flags |= dwFlags | OFN_ENABLESIZING;
        if (pParentWnd)
        {
            m_ofn.hwndOwner = pParentWnd->GetSafeHwnd();
        }

        m_ofn.Flags |= OFN_EXPLORER;
        m_ofn.hInstance = AfxGetResourceHandle();

        // setup initial file name
        if (lpszFileName != NULL)
        {
            lstrcpyn(m_szFileName, lpszFileName, sizeof(m_szFileName));
        }

        // Translate filter into commdlg format (lots of \0)
        if (lpszFilter != NULL)
        {
            m_strFilter = lpszFilter;
            LPTSTR pch = m_strFilter.GetBuffer(0); // modify the buffer in place
            // MFC delimits with '|' not '\0'
            while ((pch = _tcschr(pch, '|')) != NULL)
            {
                *pch++ = '\0';
            }
            m_ofn.lpstrFilter = m_strFilter;
            // do not call ReleaseBuffer() since the string contains '\0' characters
        }
    }

    INT_PTR DoModal()
    {
        // zero out the file buffer for consistent parsing later
        assert(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
        DWORD nOffset = lstrlen(m_ofn.lpstrFile) + 1;
        assert(nOffset <= m_ofn.nMaxFile);
        memset(m_ofn.lpstrFile + nOffset, 0, (m_ofn.nMaxFile - nOffset) * sizeof(TCHAR));

        // WINBUG: This is a special case for the file open/save dialog,
        //  which sometimes pumps while it is coming up but before it has
        //  disabled the main window.
        HWND hWndFocus = ::GetFocus();
        BOOL bEnableParent = FALSE;
        if (m_ofn.hwndOwner != NULL && ::IsWindowEnabled(m_ofn.hwndOwner))
        {
            bEnableParent = TRUE;
            ::EnableWindow(m_ofn.hwndOwner, FALSE);
        }

        int nResult;
        if (m_bOpenFileDialog)
        {
            nResult = ::GetOpenFileName(&m_ofn);
        }
        else
        {
            nResult = ::GetSaveFileName(&m_ofn);
        }

        // WINBUG: Second part of special case for file open/save dialog.
        if (bEnableParent)
        {
            ::EnableWindow(m_ofn.hwndOwner, TRUE);
        }
        if (::IsWindow(hWndFocus))
        {
            ::SetFocus(hWndFocus);
        }

        return nResult ? nResult : IDCANCEL;
    }

    CString GetPathName() const
    {
        return m_ofn.lpstrFile;
    }

    POSITION GetStartPosition() const
    {
        return NULL;
    }

    CString GetNextPathName(POSITION& pos) const
    {
        BOOL bExplorer = m_ofn.Flags & OFN_EXPLORER;
        TCHAR chDelimiter;
        if (bExplorer)
        {
            chDelimiter = '\0';
        }
        else
        {
            chDelimiter = ' ';
        }

        LPTSTR lpsz = (LPTSTR)pos;
        if (lpsz == m_ofn.lpstrFile) // first time
        {
            if ((m_ofn.Flags & OFN_ALLOWMULTISELECT) == 0)
            {
                pos = NULL;
                return m_ofn.lpstrFile;
            }

            // find char pos after first Delimiter
            while (*lpsz != chDelimiter && *lpsz != '\0')
            {
                lpsz = _tcsinc(lpsz);
            }
            lpsz = _tcsinc(lpsz);

            // if single selection then return only selection
            if (*lpsz == 0)
            {
                pos = NULL;
                return m_ofn.lpstrFile;
            }
        }

        CString strPath = m_ofn.lpstrFile;
        if (!bExplorer)
        {
            LPTSTR lpszPath = m_ofn.lpstrFile;
            while (*lpszPath != chDelimiter)
            {
                lpszPath = _tcsinc(lpszPath);
            }
            strPath = strPath.Left(int(lpszPath - m_ofn.lpstrFile));
        }

        LPTSTR lpszFileName = lpsz;
        CString strFileName = lpsz;

        // find char pos at next Delimiter
        while (*lpsz != chDelimiter && *lpsz != '\0')
        {
            lpsz = _tcsinc(lpsz);
        }

        if (!bExplorer && *lpsz == '\0')
        {
            pos = NULL;
        }
        else
        {
            if (!bExplorer)
            {
                strFileName = strFileName.Left(int(lpsz - lpszFileName));
            }

            lpsz = _tcsinc(lpsz);
            if (*lpsz == '\0') // if double terminated then done
            {
                pos = NULL;
            }
            else
            {
                pos = (POSITION)lpsz;
            }
        }

        // only add '\\' if it is needed
        if (!strPath.IsEmpty())
        {
            // check for last back-slash or forward slash (handles DBCS)
            LPCTSTR lpsz = _tcsrchr(strPath, '\\');
            if (lpsz == NULL)
            {
                lpsz = _tcsrchr(strPath, '/');
            }
            // if it is also the last character, then we don't need an extra
            if (lpsz != NULL &&
                (lpsz - (LPCTSTR)strPath) == strPath.GetLength() - 1)
            {
                assert(*lpsz == '\\' || *lpsz == '/');
                return strPath + strFileName;
            }
        }
        return strPath + '\\' + strFileName;
    }
};

//! Override CFileDialog with custom version.
#define CFileDialog CCryFileDialog

#endif //WIN64

#endif // CRYINCLUDE_EDITOR_MFC_AMD64_FIX_H
