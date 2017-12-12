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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_SCROLLABLEDIALOG_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_SCROLLABLEDIALOG_H
#pragma once


#include <atlbase.h>
#include <atlwin.h>
#include "Util.h"

template <class T>
class CScrollableDialog
    : public CDialogImpl<T>
{
public:
    typedef T DescendentType;

    BEGIN_MSG_MAP(CScrollableDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    int m_nCurHeight;
    int m_nScrollPos;
    RECT m_rect;
};

#include "stdafx.h"
#include "ScrollableDialog.h"

template <class T>
LRESULT CScrollableDialog<T>::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    GetWindowRect(&m_rect);
    m_nScrollPos = 0;
    return FALSE;
}

template <class T>
LRESULT CScrollableDialog<T>::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT nSBCode = LOWORD(wParam);
    UINT nPos = HIWORD(wParam); // Only valid for SB_THUMBPOSITION or SB_THUMBTRACK.
    HWND hwndScrollBar = (HWND)lParam;

    int nDelta;
    int nMaxPos = m_rect.bottom - m_rect.top - m_nCurHeight;

    switch (nSBCode)
    {
    case SB_LINEDOWN:
        if (m_nScrollPos >= nMaxPos)
        {
            return 0;
        }
        nDelta = Util::getMin(nMaxPos / 10, nMaxPos - m_nScrollPos);
        break;

    case SB_LINEUP:
        if (m_nScrollPos <= 0)
        {
            return 0;
        }
        nDelta = -Util::getMin(nMaxPos / 10, m_nScrollPos);
        break;

    case SB_PAGEDOWN:
        if (m_nScrollPos >= nMaxPos)
        {
            return 0;
        }
        nDelta = Util::getMin(nMaxPos / 4, nMaxPos - m_nScrollPos);
        break;

    case SB_THUMBPOSITION:
        nDelta = (int)nPos - m_nScrollPos;
        break;

    case SB_PAGEUP:
        if (m_nScrollPos <= 0)
        {
            return 0;
        }
        nDelta = -Util::getMin(nMaxPos / 4, m_nScrollPos);
        break;

    default:
        return 0;
    }

    m_nScrollPos += nDelta;
    SetScrollPos(SB_VERT, m_nScrollPos, TRUE);
    ScrollWindow(0, -nDelta);

    return 0;
}

template <class T>
LRESULT CScrollableDialog<T>::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int cx = LOWORD(lParam);
    int cy = HIWORD(lParam);

    m_nCurHeight = cy;
    int nScrollMax;
    if (cy < m_rect.bottom - m_rect.top)
    {
        nScrollMax = m_rect.bottom - m_rect.top - cy;
    }
    else
    {
        nScrollMax = 0;
    }

    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL; // SIF_ALL = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nMin = 0;
    si.nMax = nScrollMax;
    si.nPage = si.nMax / 4;
    si.nPos = 0;
    SetScrollInfo(SB_VERT, &si, TRUE);

    return 0;
}

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_SCROLLABLEDIALOG_H
