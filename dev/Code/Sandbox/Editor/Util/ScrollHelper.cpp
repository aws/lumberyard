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

// Description : Class to assist with setting easy to use scroll bars for
//               any CWnd inherited control


#include "stdafx.h"
#include "ScrollHelper.h"

//////////////////////////////////////////////////////////////////////////
CScrollHelper::CScrollHelper()
    : m_desiredClientSize(0, 0, 1, 1)
    , m_autoScrollWindow(true)
    , m_pWnd(NULL)
{
    m_visible[ HORIZONTAL ] = false;
    m_visible[ VERTICAL ] = false;

    m_totalScroll[ HORIZONTAL ] = 0;
    m_totalScroll[ VERTICAL ] = 0;

    m_winBarId[ HORIZONTAL ] = SB_HORZ;
    m_winBarId[ VERTICAL ] = SB_VERT;

    m_allowed[ HORIZONTAL ] = true;
    m_allowed[ VERTICAL ] = true;
}


//////////////////////////////////////////////////////////////////////////
CScrollHelper::~CScrollHelper()
{
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::SetWnd(CWnd* pWnd)
{
    m_pWnd = pWnd;
    if (m_pWnd != NULL)
    {
        const DWORD style = pWnd->GetStyle();

        m_visible[ HORIZONTAL ] = (style & WS_HSCROLL);
        m_visible[ VERTICAL ] = (style & WS_VSCROLL);
    }
    UpdateScrollBars();
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    OnScroll(HORIZONTAL, nSBCode, nPos);
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    OnScroll(VERTICAL, nSBCode, nPos);
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::ResetScroll()
{
    OnScroll(HORIZONTAL, SB_LEFT, 0);
    OnScroll(VERTICAL, SB_LEFT, 0);
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::SetAllowed(BarId barId, bool allowed)
{
    m_allowed[ barId ] = allowed;
    if (m_pWnd != NULL)
    {
        UpdateScrollBars();
    }
}


//////////////////////////////////////////////////////////////////////////
int CScrollHelper::GetScrollAmount(BarId barId) const
{
    if (m_pWnd == NULL)
    {
        return 0;
    }

    const int scrollAmount = m_pWnd->GetScrollPos(m_winBarId[ barId ]);

    return scrollAmount;
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::OnScroll(BarId barId, UINT nSBCode, UINT nPos)
{
    assert(m_pWnd != NULL);
    if (m_pWnd == NULL)
    {
        return;
    }

    const int SCROLL_AMOUNT = 6;

    const int winBarId = m_winBarId[ barId ];

    int nCurPos = m_pWnd->GetScrollPos(winBarId);
    const int nPrevPos = nCurPos;

    CRect clientArea;
    m_pWnd->GetClientRect(clientArea);

    int viewSizeVec[ 2 ];
    viewSizeVec[ HORIZONTAL ] = clientArea.Width();
    viewSizeVec[ VERTICAL ] = clientArea.Height();

    const int viewSize = viewSizeVec[ barId ];

    switch (nSBCode)
    {
    case SB_LEFT:
        nCurPos = 0;
        break;
    case SB_RIGHT:
        nCurPos = m_pWnd->GetScrollLimit(winBarId) - 1;
        break;
    case SB_LINELEFT:
        nCurPos = max(nCurPos - SCROLL_AMOUNT, 0);
        break;
    case SB_LINERIGHT:
        nCurPos = min(nCurPos + SCROLL_AMOUNT, m_pWnd->GetScrollLimit(winBarId) - 1);
        break;
    case SB_PAGELEFT:
        nCurPos = max(nCurPos - viewSize, 0);
        break;
    case SB_PAGERIGHT:
        nCurPos = min(nCurPos + viewSize, m_pWnd->GetScrollLimit(winBarId) - 1);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
        SCROLLINFO info;
        if (m_pWnd->GetScrollInfo(winBarId, &info, SIF_TRACKPOS))
        {
            nCurPos = info.nTrackPos;
        }
    }
    break;
    }

    m_pWnd->SetScrollPos(winBarId, nCurPos);

    if (m_autoScrollWindow)
    {
        const int scrollInc = nPrevPos - nCurPos;

        int scrollVec[ 2 ];
        scrollVec[ 0 ] = 0;
        scrollVec[ 1 ] = 0;

        scrollVec[ barId ] = scrollInc;

        m_pWnd->ScrollWindow(scrollVec[ HORIZONTAL ], scrollVec[ VERTICAL ]);

        m_totalScroll[ barId ] += scrollInc;
    }
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::OnSize(UINT nType, int cx, int cy)
{
    Update();
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::Update()
{
    if (m_pWnd == NULL)
    {
        return;
    }

    int currentScrollPos[ 2 ];
    currentScrollPos[ HORIZONTAL ] = m_pWnd->GetScrollPos(SB_HORZ);
    currentScrollPos[ VERTICAL ] = m_pWnd->GetScrollPos(SB_VERT);

    UpdateScrollBars();

    currentScrollPos[ HORIZONTAL ] = std::min< int >(currentScrollPos[ HORIZONTAL ], m_pWnd->GetScrollLimit(SB_HORZ) - 1);
    currentScrollPos[ VERTICAL ] = std::min< int >(currentScrollPos[ VERTICAL ], m_pWnd->GetScrollLimit(SB_VERT) - 1);

    m_pWnd->SetScrollPos(SB_HORZ, currentScrollPos[ HORIZONTAL ]);
    m_pWnd->SetScrollPos(SB_VERT, currentScrollPos[ VERTICAL ]);

    int scrollIncrement[ 2 ];
    scrollIncrement[ HORIZONTAL ] = -m_totalScroll[ HORIZONTAL ] - currentScrollPos[ HORIZONTAL ];
    scrollIncrement[ VERTICAL ] = -m_totalScroll[ VERTICAL ] - currentScrollPos[ VERTICAL ];

    m_pWnd->ScrollWindow(scrollIncrement[ HORIZONTAL ], scrollIncrement[ VERTICAL ]);
    m_totalScroll[ HORIZONTAL ] += scrollIncrement[ HORIZONTAL ];
    m_totalScroll[ VERTICAL ] += scrollIncrement[ VERTICAL ];
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::SetDesiredClientSize(const CRect& desiredClientSize)
{
    m_desiredClientSize = desiredClientSize;

    Update();
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::SetAutoScrollWindow(bool autoScrollWindow)
{
    m_autoScrollWindow = autoScrollWindow;
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::OnClientSizeUpdated()
{
    UpdateScrollBars();
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::UpdateScrollBars()
{
    assert(m_pWnd != NULL);
    if (m_pWnd == NULL)
    {
        return;
    }

    CRect clientArea;
    m_pWnd->GetClientRect(clientArea);

    int cx = clientArea.Width();
    int cy = clientArea.Height();

    const CRect currentRect = m_desiredClientSize;

    if (m_visible[ HORIZONTAL ])
    {
        cy += GetSystemMetrics(SM_CYHSCROLL);
    }

    if (m_visible[ VERTICAL ])
    {
        cx += GetSystemMetrics(SM_CXVSCROLL);
    }

    bool newHVis = false;
    bool newVVis = false;

    if (cx < currentRect.Width())
    {
        newHVis = true;
    }
    if (cy < currentRect.Height())
    {
        newVVis = true;
    }

    if (newVVis && !newHVis && cx - GetSystemMetrics(SM_CXVSCROLL) < currentRect.Width())
    {
        newHVis = true;
    }

    if (newHVis && !newVVis && cy - GetSystemMetrics(SM_CYHSCROLL) < currentRect.Height())
    {
        newVVis = true;
    }

    if (m_visible[ HORIZONTAL ])
    {
        cy -= GetSystemMetrics(SM_CYHSCROLL);
    }

    if (m_visible[ VERTICAL ])
    {
        cx -= GetSystemMetrics(SM_CXVSCROLL);
    }

    if (newHVis && !m_visible[ HORIZONTAL ])
    {
        cx -= GetSystemMetrics(SM_CXVSCROLL);
    }

    if (newVVis && !m_visible[ VERTICAL ])
    {
        cy -= GetSystemMetrics(SM_CYHSCROLL);
    }

    UpdateScrollBar(HORIZONTAL, currentRect.Width(), cx);
    UpdateScrollBar(VERTICAL, currentRect.Height(), cy);
}


//////////////////////////////////////////////////////////////////////////
void CScrollHelper::UpdateScrollBar(BarId barId, int desiredSize, int viewSize)
{
    assert(m_pWnd != NULL);

    if (!m_allowed[ barId ])
    {
        return;
    }

    const bool showScrollBar = (viewSize < desiredSize);

    const int winBarId = m_winBarId[ barId ];
    const bool isVisible = m_visible[ barId ];

    if (isVisible != showScrollBar)
    {
        m_pWnd->EnableScrollBarCtrl(winBarId, showScrollBar);

        m_pWnd->ShowScrollBar(winBarId, showScrollBar);

        SCROLLINFO  si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE;

        si.nMin = 0;
        si.nMax = desiredSize;
        si.nPage = viewSize;

        m_pWnd->SetScrollInfo(winBarId, &si, FALSE);
    }

    if (showScrollBar)
    {
        SCROLLINFO  si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE;

        si.nMin = 0;
        si.nMax = desiredSize;
        si.nPage = viewSize;

        m_pWnd->SetScrollInfo(winBarId, &si, TRUE);
    }

    m_visible[ barId ] = showScrollBar;
}