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

#include "StdAfx.h"
#include "ListCtrlEx.h"

#define COLUMN_MULTIPLIER 0xFFFF

//////////////////////////////////////////////////////////////////////////
CListCtrlEx::CListCtrlEx()
{
}

//////////////////////////////////////////////////////////////////////////
CListCtrlEx::~CListCtrlEx()
{
    for (ControlsMap::iterator it = m_controlsMap.begin(); it != m_controlsMap.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
        }
    }
    m_controlsMap.clear();
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CListCtrlEx, CXTListCtrl)
ON_WM_HSCROLL()
ON_WM_VSCROLL()
ON_WM_SIZE()
ON_WM_WINDOWPOSCHANGED()
//ON_NOTIFY(NM_HEADER_ENDTRACK,0,OnHeaderEndTrack )
//ON_NOTIFY(HDN_BEGINTRACKA,0,OnHeaderEndTrack )
//ON_NOTIFY(HDN_BEGINTRACKW,0,OnHeaderEndTrack )
ON_NOTIFY(HDN_ENDTRACKA, 0, OnHeaderEndTrack)
ON_NOTIFY(HDN_ENDTRACKW, 0, OnHeaderEndTrack)
ON_WM_CREATE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
int CListCtrlEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    // Create
    int res = CXTListCtrl::OnCreate(lpCreateStruct);
    //if (res == 0)
    //m_headerCtrl.SubclassWindow( GetHeaderCtrl()->GetSafeHwnd() );
    return res;
}

//////////////////////////////////////////////////////////////////////////
BOOL CListCtrlEx::DeleteAllItems()
{
    BOOL bRes = CXTListCtrl::DeleteAllItems();
    for (ControlsMap::iterator it = m_controlsMap.begin(); it != m_controlsMap.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
        }
    }
    m_controlsMap.clear();
    return bRes;
}

//////////////////////////////////////////////////////////////////////////
BOOL CListCtrlEx::SubclassWindow(HWND hWnd)
{
    BOOL res = CXTListCtrl::SubclassWindow(hWnd);
    return res;
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CXTListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CXTListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::OnSize(UINT nType, int cx, int cy)
{
    CXTListCtrl::OnSize(nType, cx, cy);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos)
{
    CXTListCtrl::OnWindowPosChanged(lpwndpos);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::OnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult)
{
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::RepositionControls()
{
    CRect rcItem;

    int nHeaderHeight = 0;
    if (GetHeaderCtrl())
    {
        CRect rc;
        GetHeaderCtrl()->GetWindowRect(rc);
        nHeaderHeight = rc.Height();
    }

    int nNumItems = GetItemCount();
    HDWP hdwp = BeginDeferWindowPos(nNumItems);
    for (ControlsMap::iterator it = m_controlsMap.begin(); it != m_controlsMap.end(); ++it)
    {
        int nId = it->first;
        CWnd* pWnd = it->second;
        int nIndex = nId % COLUMN_MULTIPLIER;
        int nColumn = nId / COLUMN_MULTIPLIER;

        bool bVisible = false;
        if (nIndex < nNumItems)
        {
            CRect rcItem;
            if (GetSubItemRect(nIndex, nColumn, LVIR_BOUNDS, rcItem))
            {
                if (rcItem.top >= nHeaderHeight)
                {
                    if (hdwp)
                    {
                        hdwp = DeferWindowPos(hdwp, pWnd->GetSafeHwnd(), NULL,
                                rcItem.left, rcItem.top, rcItem.Width(), rcItem.Height(),
                                SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    }
                    pWnd->Invalidate(FALSE);
                    bVisible = true;
                }
            }
            if (!bVisible)
            {
                if (hdwp)
                {
                    hdwp = DeferWindowPos(hdwp, pWnd->GetSafeHwnd(), NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
                }
            }
        }
    }
    if (hdwp)
    {
        EndDeferWindowPos(hdwp);
    }
}

//////////////////////////////////////////////////////////////////////////
CWnd* CListCtrlEx::GetItemControl(int nIndex, int nColumn)
{
    int nId = nColumn * COLUMN_MULTIPLIER + nIndex;
    return stl::find_in_map(m_controlsMap, nId, 0);
}

//////////////////////////////////////////////////////////////////////////
void CListCtrlEx::SetItemControl(int nIndex, int nColumn, CWnd* pWnd)
{
    int nId = nColumn * COLUMN_MULTIPLIER + nIndex;
    CWnd* pOldWnd = stl::find_in_map(m_controlsMap, nId, 0);
    if (pOldWnd != pWnd)
    {
        if (pOldWnd)
        {
            delete pOldWnd;
        }
        if (pWnd)
        {
            pWnd->SetParent(this);
            m_controlsMap[nId] = pWnd;
            // can only create progress for an existing item
            if (nIndex < GetItemCount())
            {
                CRect rcItem;
                if (GetSubItemRect(nIndex, nColumn, LVIR_BOUNDS, rcItem))
                {
                    pWnd->MoveWindow(rcItem);
                }
            }
        }
        else
        {
            m_controlsMap.erase(nId);
        }
    }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CControlsListBox::CControlsListBox()
{
}

//////////////////////////////////////////////////////////////////////////
CControlsListBox::~CControlsListBox()
{
    for (ControlsMap::iterator it = m_controlsMap.begin(); it != m_controlsMap.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
        }
    }
    m_controlsMap.clear();
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CControlsListBox, CListBox)
ON_WM_HSCROLL()
ON_WM_VSCROLL()
ON_WM_SIZE()
ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::ResetContent()
{
    CListBox::ResetContent();
    for (ControlsMap::iterator it = m_controlsMap.begin(); it != m_controlsMap.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
        }
    }
    m_controlsMap.clear();
}

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CListBox::OnHScroll(nSBCode, nPos, pScrollBar);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CListBox::OnVScroll(nSBCode, nPos, pScrollBar);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::OnSize(UINT nType, int cx, int cy)
{
    CListBox::OnSize(nType, cx, cy);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos)
{
    CListBox::OnWindowPosChanged(lpwndpos);
    RepositionControls();
}

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::RepositionControls()
{
    CRect rcItem;

    int nNumItems = GetCount();
    for (ControlsMap::iterator it = m_controlsMap.begin(); it != m_controlsMap.end(); ++it)
    {
        int nIndex = it->first;
        CWnd* pWnd = it->second;

        if (nIndex < nNumItems)
        {
            CRect rcItem;
            if (GetItemRect(nIndex, rcItem))
            {
                pWnd->MoveWindow(rcItem);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CWnd* CControlsListBox::GetItemControl(int nIndex)
{
    return stl::find_in_map(m_controlsMap, nIndex, 0);
}

//////////////////////////////////////////////////////////////////////////
void CControlsListBox::SetItemControl(int nIndex, CWnd* pWnd)
{
    CWnd* pOldWnd = stl::find_in_map(m_controlsMap, nIndex, 0);
    if (pOldWnd != pWnd)
    {
        if (pOldWnd)
        {
            delete pOldWnd;
        }
        if (pWnd)
        {
            pWnd->SetParent(this);
            m_controlsMap[nIndex] = pWnd;
            // can only create progress for an existing item
            if (nIndex < GetCount())
            {
                CRect rcItem;
                if (GetItemRect(nIndex, rcItem))
                {
                    pWnd->MoveWindow(rcItem);
                }
            }
        }
        else
        {
            m_controlsMap.erase(nIndex);
        }
    }
}
