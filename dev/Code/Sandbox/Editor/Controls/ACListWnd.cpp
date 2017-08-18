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


#include "stdafx.h"
#include "ACListWnd.h"

#define _MAX_ENTRYS_ 8
#define _MODE_FIND_ALL_         (1L << 5)
#define _MODE_HISTORY_          (1L << 6)

/////////////////////////////////////////////////////////////////////////////
// CACListWnd

void DoPaintMessageLoop()
{
    MSG message1;
    while (::PeekMessage(&message1, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
    {
        ::TranslateMessage(&message1);
        ::DispatchMessage(&message1);
    }
}

/**********************************************************************/

CACListWnd::CACListWnd()
{
    m_lTopIndex = 0;
    m_lCount = 0;
    m_ItemHeight = 16;
    m_lSelItem = -1;
    m_VisibleItems = 0;
    m_pEditParent = NULL;
    m_LastSize.SetRectEmpty();
    m_PrefixChar = 0;
    m_lMode = 0;
    m_nHistory = 100;
}

/**********************************************************************/

CACListWnd::~CACListWnd()
{
    m_SearchList.RemoveAll();
    m_DisplayList.RemoveAll();
    DestroyWindow();
}

/*********************************************************************/

void CACListWnd::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
#if (_MSC_VER >= 1300)
    CWnd::OnActivateApp(bActive, dwThreadID);  //vc7    FIX 1.2
#else
    CWnd::OnActivateApp(bActive, (HTASK)dwThreadID); //vc6        FIX 1.2
#endif

    ShowWindow(false);
}


BEGIN_MESSAGE_MAP(CACListWnd, CWnd)
//{{AFX_MSG_MAP(CACListWnd)
ON_WM_PAINT()
ON_WM_SIZE()
ON_WM_ERASEBKGND()
ON_WM_NCPAINT()
ON_WM_KEYDOWN()
ON_WM_NCCALCSIZE()
ON_WM_VSCROLL()
ON_WM_ACTIVATEAPP()
ON_WM_NCHITTEST()
ON_WM_LBUTTONDOWN()
ON_WM_RBUTTONDOWN()
ON_WM_SETCURSOR()
ON_WM_SHOWWINDOW()
ON_WM_NCLBUTTONDOWN()
ON_WM_MOUSEMOVE()
ON_WM_TIMER()
ON_WM_GETMINMAXINFO()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CACListWnd Handlers

void CACListWnd::DrawItem(CDC* pDC, long m_lItem, long width)
{
    long y = m_lItem - m_lTopIndex;
    CRect rcLabel(2, y * m_ItemHeight, width, (y + 1) * m_ItemHeight);

    pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

    if (m_lItem == m_lSelItem)
    {
        rcLabel.left = 0;
        pDC->FillSolidRect(rcLabel, ::GetSysColor(COLOR_HIGHLIGHT));
        pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
        rcLabel.left = 2;
    }

    if (m_PrefixChar)
    {
        m_DisplayStr = m_PrefixChar + m_DisplayList.GetAt(m_lItem);
    }
    else
    {
        m_DisplayStr = m_DisplayList.GetAt(m_lItem);
    }

    pDC->DrawText(m_DisplayStr, -1, rcLabel, DT_LEFT | DT_SINGLELINE |
        DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS);
}

/*********************************************************************/

void CACListWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rcWnd, m_rect, rc;
    CDC MemDC, * pDC = NULL;
    CBitmap  m_bitmap, * m_pOldBitmap;

    GetClientRect(rc);
    rcWnd = m_rect = rc;

    rc.left = rc.right - GetSystemMetrics(SM_CXHSCROLL);
    rc.top = rc.bottom - GetSystemMetrics(SM_CYVSCROLL);

    m_rect.right -= ScrollBarWidth();

    MemDC.CreateCompatibleDC(&dc);

    m_bitmap.CreateCompatibleBitmap(&dc, m_rect.Width(), m_rect.Height());
    m_pOldBitmap = MemDC.SelectObject(&m_bitmap);

    MemDC.SetWindowOrg(m_rect.left, m_rect.top);

    long width = rcWnd.Width() - ScrollBarWidth();

    MemDC.FillSolidRect(rcWnd, ::GetSysColor(COLOR_WINDOW));
    MemDC.SelectObject(GetStockObject(DEFAULT_GUI_FONT));
    MemDC.SetBkMode(TRANSPARENT);

    for (int i = m_lTopIndex; i < m_lCount; i++)
    {
        DrawItem(&MemDC, i, width);
    }


    CPen m_Pen1(PS_SOLID, 1, ::GetSysColor(COLOR_WINDOW));
    CPen m_Pen2(PS_SOLID, 1, ::GetSysColor(COLOR_BTNFACE));
    CPen m_Pen3(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));

    pDC = &dc;

    if (m_VertBar.IsWindowVisible())
    {
        dc.FillSolidRect(rc, ::GetSysColor(COLOR_BTNFACE));
    }
    else
    {
        pDC = &MemDC;
    }

    CPen* pOldPen = (CPen*)pDC->SelectObject(&m_Pen1);
    int a = 1, bottom;

    width = GetSystemMetrics(SM_CXHSCROLL);
    bottom = (rcWnd.bottom - GetSystemMetrics(SM_CXHSCROLL)) - 1;

    //gripper
    for (int i = 0; i < 20; i++, a++)
    {
        if (a == 1)
        {
            pDC->SelectObject(&m_Pen1);
        }
        if (a == 2)
        {
            pDC->SelectObject(&m_Pen2);
        }
        if (a == 3)
        {
            pDC->SelectObject(&m_Pen3);
        }
        if (a > 3)
        {
            a = 0;
        }

        pDC->MoveTo(rc.left + i - 1, rcWnd.bottom);
        pDC->LineTo(rc.left + i + width, bottom);
    }

    dc.BitBlt(m_rect.left, m_rect.top, m_rect.Width(), m_rect.Height(),
        &MemDC, m_rect.left, m_rect.top, SRCCOPY);

    pDC->SelectObject(pOldPen);
    MemDC.SelectObject(m_pOldBitmap);
}

/*********************************************************************/

void CACListWnd::Init(CWnd* pWnd)
{
    VERIFY(m_VertBar.Create(WS_VISIBLE | SBS_VERT | SBS_LEFTALIGN,
            CRect(0, 0, GetSystemMetrics(SM_CYVSCROLL), 100), this, 0));

    SetScroller();
    m_pEditParent = (CEdit*)pWnd;

    m_lCount = m_DisplayList.GetSize();
    m_VertBar.SetScrollPos(0, false);
    SetProp();

    CDC* m_pDC;
    m_pDC = GetDC();
    if (m_pDC)
    {
        m_pDC->SelectObject(GetStockObject(DEFAULT_GUI_FONT));
        CSize m_Size = m_pDC->GetOutputTextExtent("Hg");
        m_ItemHeight = m_Size.cy;
        ReleaseDC(m_pDC);
    }
}

/*********************************************************************/

void CACListWnd::SetScroller()
{
    CRect rcWnd, rcBar;
    GetClientRect(rcWnd);

    if (m_VertBar.GetSafeHwnd())
    {
        rcBar = rcWnd;
        rcBar.top = -1;
        rcBar.left = (rcWnd.Width() - GetSystemMetrics(SM_CYVSCROLL));
        rcBar.bottom -= GetSystemMetrics(SM_CYHSCROLL);
        m_VertBar.MoveWindow(rcBar);
        rcBar.top = rcWnd.bottom - 20;
        rcBar.bottom = rcWnd.bottom;

        m_VertBar.SetScrollPos(m_lTopIndex, true);
    }
}

/*********************************************************************/

void CACListWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    SetScroller();
    SetProp();

    if (!m_LastSize.IsRectEmpty())
    {
        GetWindowRect(m_LastSize);
    }
}

/*********************************************************************/

long CACListWnd::ScrollBarWidth()
{
    if (m_VertBar.IsWindowVisible())
    {
        return GetSystemMetrics(SM_CYVSCROLL);
    }
    else
    {
        return 0;
    }
}

/*********************************************************************/

void CACListWnd::SetProp()
{
    CRect rcWnd, rcBar;

    if (!m_lCount)
    {
        return;
    }

    CWnd::GetWindowRect(rcWnd);
    ScreenToClient(rcWnd);

    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask =  SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    si.nMax =  m_lCount - 1;
    m_VisibleItems = si.nPage = rcWnd.Height() / m_ItemHeight;
    si.nTrackPos = 2;
    m_VertBar.SetScrollRange(0, m_lCount - 1);
    m_VertBar.SetScrollInfo(&si);

    if (m_VisibleItems > m_lCount - 1)
    {
        m_VertBar.ShowWindow(false);
    }
    else
    {
        m_VertBar.ShowWindow(true);
    }

    if (m_lTopIndex + m_VisibleItems > m_lCount)
    {
        m_lTopIndex = m_lCount - m_VisibleItems;
        if (m_lTopIndex < 0)
        {
            m_lTopIndex = 0;
        }
        m_VertBar.SetScrollPos(m_lTopIndex, true);
    }
}

/*********************************************************************/

BOOL CACListWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
    return false;
}

/*********************************************************************/

void CACListWnd::OnNcPaint()
{
    CWindowDC dc(this);
    CRect rectClient, rectWindow, rcWnd;

    GetClientRect(rectClient);
    GetWindowRect(rectWindow);
    ScreenToClient(rectWindow);

    rectClient.OffsetRect(-(rectWindow.left), -(rectWindow.top));
    dc.ExcludeClipRect(rectClient);

    rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top);

    dc.FillSolidRect(rectWindow, ::GetSysColor(COLOR_WINDOWTEXT));
}

/*********************************************************************/

void CACListWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);

    if (nChar == VK_ESCAPE)
    {
        ShowWindow(false);
    }
}

/*********************************************************************/

void CACListWnd::OnNcCalcSize(BOOL /*bCalcValidRects*/, NCCALCSIZE_PARAMS FAR* lpncsp)
{
    ::InflateRect(lpncsp->rgrc,
        -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
}

/*********************************************************************/

int CACListWnd::HitTest(CPoint point)
{
    CRect rcItem;
    CRect rcWnd;

    GetClientRect(rcWnd);
    long width = rcWnd.Width() - ScrollBarWidth();

    for (int i = m_lTopIndex; i < m_lCount; i++)
    {
        long y = i - m_lTopIndex;
        rcItem.SetRect(2, y * m_ItemHeight, width, (y + 1) * m_ItemHeight);

        if (PtInRect(&rcItem, point))
        {
            return (m_lSelItem = (y + m_lTopIndex));
        }
    }

    return -1;
}

/*********************************************************************/

LRESULT CACListWnd::OnNcHitTest(CPoint point)
{
    CRect rectClient;
    GetWindowRect(rectClient);

    rectClient.left = rectClient.right - GetSystemMetrics(SM_CYVSCROLL);
    rectClient.top = rectClient.bottom - GetSystemMetrics(SM_CXVSCROLL);

    if (rectClient.PtInRect(point))
    {
        return HTBOTTOMRIGHT;
    }
    else
    {
        return HTCLIENT;
    }
}

/*********************************************************************/

void CACListWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    CWnd::OnLButtonDown(nFlags, point);
    int sel = HitTest(point);

    if (sel >= 0)
    {
        if (!EnsureVisible(sel, true))
        {
            Invalidate();
        }
        m_lSelItem = sel;
        m_pEditParent->SendMessage(ENAC_UPDATE, WM_KEYDOWN, GetDlgCtrlID());
        DoPaintMessageLoop();
        Sleep(500);
        ShowWindow(false);
    }
    else
    {
        CRect rc;
        GetClientRect(rc);
        if (!rc.PtInRect(point))
        {
            ShowWindow(false);
        }
    }
}

/*********************************************************************/

void CACListWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
    CWnd::OnRButtonDown(nFlags, point);
    ShowWindow(false);
}

/*********************************************************************/

BOOL CACListWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    CRect rectClient;
    CPoint ptCursor;

    GetWindowRect(rectClient);
    ScreenToClient(&rectClient);

    rectClient.left = rectClient.right - GetSystemMetrics(SM_CYVSCROLL);
    rectClient.top = rectClient.bottom - GetSystemMetrics(SM_CXVSCROLL);


    GetCursorPos(&ptCursor);
    ScreenToClient(&ptCursor);

    if (rectClient.PtInRect(ptCursor)) // Vergrößerungs-Cursor
    {
        return CWnd::OnSetCursor(pWnd, nHitTest, message);
    }

    ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
    return TRUE;
}

/*********************************************************************/

void CACListWnd::InvalidateAndScroll()
{
    m_VertBar.SetScrollPos(m_lTopIndex, true);
    Invalidate();
    DoPaintMessageLoop();
}

/*********************************************************************/

bool CACListWnd::EnsureVisible(int item, bool m_bWait)
{
    if (item > m_lTopIndex && item < m_lTopIndex + m_VisibleItems)
    {
        return false; // ist visible
    }
    if (item > m_lTopIndex)  // scroll down
    {
        long m_len = item;
        for (int i = m_lTopIndex; i < m_len; i++)
        {
            if (i >= m_lCount - m_VisibleItems)
            {
                break;
            }
            if (i >= m_lCount - m_VisibleItems || i + m_VisibleItems > item)
            {
                break;
            }

            m_lTopIndex++;

            if (m_bWait)
            {
                InvalidateAndScroll();
                Sleep(10);
                DoPaintMessageLoop();
            }
        }
        InvalidateAndScroll();
        return true;
    }

    if (item < m_lTopIndex)  // scroll up
    {
        while (item < m_lTopIndex)
        {
            if (m_lTopIndex > 0)
            {
                m_lTopIndex--;
            }
            else
            {
                break;
            }

            if (m_bWait)
            {
                InvalidateAndScroll();
                Sleep(10);
                DoPaintMessageLoop();
            }
        }

        InvalidateAndScroll();
        return true;
    }

    return false;
}

/*********************************************************************/

bool CACListWnd::SelectItem(int item)
{
    if (item > m_lCount)
    {
        return false;
    }

    if (item == -1)
    {
        EnsureVisible(m_lSelItem, false);
        Invalidate();
        return false;
    }

    m_lSelItem = item;

    if (!EnsureVisible(item, true))
    {
        Invalidate();
    }

    return true;
}

/*********************************************************************/

int CACListWnd::FindStringExact(int nStartAfter, LPCTSTR lpszString)
{
    if (nStartAfter > m_SearchList.GetSize())
    {
        return -1;
    }

    for (int i = nStartAfter + 1; i < m_SearchList.GetSize(); i++)
    {
        if (m_SearchList.GetAt(i).Compare(lpszString) == 0)
        {
            return i;
        }
    }
    return -1;
}

/*********************************************************************/
/*
** Vers. 1.1
* NEW: m_bDisplayOnly
*/
int CACListWnd::FindString(int nStartAfter, LPCTSTR lpszString, bool m_bDisplayOnly)
{
    long m_AktCount = m_DisplayList.GetSize();

    if (!m_bDisplayOnly)
    {
        CString m_Str1, m_Str2 = lpszString;
        if (!m_pEditParent)
        {
            ShowWindow(false);
            return -1;
        }

        if (nStartAfter > m_SearchList.GetSize())
        {
            ShowWindow(false);
            return -1;
        }

        if (m_Str2.IsEmpty())
        {
            ShowWindow(false);
            return -1;
        }

        m_DisplayList.RemoveAll();

        m_Str2.MakeUpper();

        for (int i = nStartAfter + 1; i < m_SearchList.GetSize(); i++)
        {
            if (m_PrefixChar)
            {
                m_Str1 = m_PrefixChar;
            }
            else
            {
                m_Str1 = _T("");
            }

            m_Str1 += m_SearchList.GetAt(i);

            m_Str1.MakeUpper();

            if (m_lMode & _MODE_FIND_ALL_)
            {
                if (m_Str1.Find(m_Str2) >= 0)
                {
                    m_DisplayList.Add(m_SearchList.GetAt(i));
                }
            }
            else  // _MODE_FIND_EXACT_
            {
                if (m_Str1.Find(m_Str2) == 0)
                {
                    m_DisplayList.Add(m_SearchList.GetAt(i));
                }
            }
        }
    }
    m_lCount = m_DisplayList.GetSize();

    if (m_lCount)
    {
        CRect rcWnd;
        int iHeight, iWight;

        m_pEditParent->GetWindowRect(rcWnd);

        SetScroller();
        SetProp();

        ShowWindow(true);
        Invalidate();

        iHeight = m_lCount * m_ItemHeight + (GetSystemMetrics(SM_CYBORDER) * 2);

        if (m_lCount > _MAX_ENTRYS_)
        {
            iHeight = _MAX_ENTRYS_ * m_ItemHeight + (GetSystemMetrics(SM_CYBORDER) * 2);
        }

        if (!m_LastSize.IsRectEmpty())
        {
            iWight = m_LastSize.Width();
            iHeight = m_LastSize.Height();
            rcWnd.top += rcWnd.Height();
            rcWnd.right = rcWnd.left + iWight;
            rcWnd.bottom = rcWnd.top + iHeight;

            SetWindowPos(&CWnd::wndTopMost, rcWnd.left,
                rcWnd.top,
                rcWnd.Width(),
                rcWnd.Height(), 0);
        }
        else
        {
            SetWindowPos(&CWnd::wndTopMost, rcWnd.left,
                rcWnd.top + rcWnd.Height(),
                rcWnd.Width(),
                iHeight, 0);
        }

        if (m_AktCount != m_DisplayList.GetSize())
        {
            m_lSelItem = -1;
        }

        if ((m_lMode & _MODE_HISTORY_) == 0)
        {
            SortList(m_DisplayList);
        }
    }
    else
    {
        ShowWindow(false);
    }

    return 1;
}

/*********************************************************************/

int CACListWnd::SelectString(LPCTSTR lpszString)
{
    int item = FindString(-1, lpszString);
    SelectItem(item);
    return item;
}

/*********************************************************************/

bool CACListWnd::GetText(int item, CString& m_Text)
{
    if (item < 0 || item > m_SearchList.GetSize())
    {
        return false;
    }
    m_Text = m_SearchList.GetAt(item);
    return true;
}

/*********************************************************************/

void CACListWnd::OnShowWindow(BOOL bShow, UINT nStatus)
{
    if (bShow)
    {
        m_nIDTimer =  SetTimer(IDTimerInstall, 200, NULL);
        m_pEditParent->GetParent()->GetWindowRect(m_ParentRect);
    }
    else
    {
        if (m_nIDTimer)
        {
            KillTimer(IDTimerInstall);
        }
        m_nIDTimer = 0;
        m_lSelItem = -1;
        m_lTopIndex = 0;
    }

    CWnd::OnShowWindow(bShow, nStatus);
    ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
}

/*********************************************************************/

void CACListWnd::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
    if (OnNcHitTest(point) == HTBOTTOMRIGHT)
    {
        GetWindowRect(m_LastSize);
    }
    CWnd::OnNcLButtonDown(nHitTest, point);
}

/*********************************************************************/

CString CACListWnd::GetString()
{
    int i = m_DisplayList.GetSize();

    if (!i)
    {
        return _T("");
    }
    if (i <= m_lSelItem || m_lSelItem == -1)
    {
        i = 0;
    }
    else
    {
        i = m_lSelItem;
    }

    return m_DisplayList.GetAt(i);
}

/*********************************************************************/

void CACListWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
    long m_oldlTopIndex = m_lTopIndex;

    switch (nSBCode)
    {
    case SB_ENDSCROLL:
        break;

    case SB_PAGEUP:
        m_lTopIndex -= m_VisibleItems;
        if (m_lTopIndex < 0)
        {
            m_lTopIndex = 0;
        }
        break;

    case SB_PAGEDOWN:
        m_lTopIndex += m_VisibleItems;
        if (m_lTopIndex >= m_lCount - m_VisibleItems)
        {
            m_lTopIndex = m_lCount - m_VisibleItems;
        }
        break;

    case SB_LINEUP:
        m_lTopIndex--;
        if (m_lTopIndex < 0)
        {
            m_lTopIndex = 0;
        }
        break;

    case SB_LINEDOWN:
        m_lTopIndex++;
        if (m_lTopIndex >= m_lCount - m_VisibleItems)
        {
            m_lTopIndex = m_lCount - m_VisibleItems;
        }
        break;

    case SB_THUMBTRACK:
        m_lTopIndex = nPos;
        break;
    }

    m_VertBar.SetScrollPos(m_lTopIndex, true);

    if (m_oldlTopIndex != m_lTopIndex)
    {
        Invalidate();
    }
}

/*********************************************************************/

CString CACListWnd::GetNextString(int nChar)
{
    switch (nChar)
    {
    case VK_DOWN:
        m_lSelItem++;
        break;

    case VK_UP:
        m_lSelItem--;
        break;

    case VK_PRIOR:
        m_lSelItem -= m_VisibleItems;
        if (m_lSelItem < 0)
        {
            m_lSelItem = 0;
        }
        break;

    case VK_NEXT:
        m_lSelItem += m_VisibleItems;
        if (m_lSelItem >= m_lCount - 1)
        {
            m_lSelItem = m_lCount - 1;
        }
        break;

    case VK_HOME:
        m_lSelItem = 0;
        break;

    case VK_END:
        m_lSelItem = m_lCount - 1;
        break;
    }

    if (m_lSelItem < 0)
    {
        m_lSelItem = m_lCount - 1;
    }

    if (m_lSelItem >= m_lCount)
    {
        m_lSelItem = 0;
    }

    if (EnsureVisible(m_lSelItem, (m_lCount > 50) ? false : true))
    {
        InvalidateAndScroll();
    }

    return GetString();
}

/*********************************************************************/

void CACListWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    CWnd::OnMouseMove(nFlags, point);
    int sel = HitTest(point);
    if (sel >= 0)
    {
        Invalidate();
    }
}

/*********************************************************************/

void CACListWnd::OnTimer(UINT_PTR nIDEvent)
{
    CWnd::OnTimer(nIDEvent);

    CRect m_ParentRect1;
    m_pEditParent->GetParent()->GetWindowRect(m_ParentRect1);
    if (!m_ParentRect1.EqualRect(m_ParentRect))
    {
        ShowWindow(false);
    }
}

/*********************************************************************/

void CACListWnd::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
    if (GetSafeHwnd())
    {
        // Vers. 1.2
        long m_lMinY1 = GetSystemMetrics(SM_CYHSCROLL) * 2 + GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXHTHUMB),
             m_lMinY2 = m_lCount * m_ItemHeight + (GetSystemMetrics(SM_CYBORDER) * 2);

        if (m_VisibleItems > m_lCount - 1 &&  m_lMinY2 < m_lMinY1)
        {
            lpMMI->ptMinTrackSize.y = m_lMinY2;
        }
        else
        {
            lpMMI->ptMinTrackSize.y = m_lMinY1;
        }
        //---------

        lpMMI->ptMinTrackSize.x = GetSystemMetrics(SM_CXHSCROLL) * 4;


        // Vers. 1.2
        if (m_pEditParent != NULL)
        {
            RECT rc;
            m_pEditParent->GetWindowRect (&rc);
            lpMMI->ptMinTrackSize.x = rc.right - rc.left;
        }
    }
    else
    {
        CWnd::OnGetMinMaxInfo(lpMMI);
    }
}

/*********************************************************************/

int CACListWnd::CompareString(const void* p1, const void* p2)
{
    return _stricmp(*( char** ) p1, *( char** ) p2);
}

/*********************************************************************/

void CACListWnd::SortList(CStringArray& m_List)
{
    int m_Count = m_List.GetSize();

    if (m_Count > 1)
    {
        CStringArray m_Liste1;
        m_Liste1.Copy(m_List);

        LPCTSTR* ppSortArray = new LPCTSTR[m_Count + 1];


        for (int i = 0; i < m_Count; i++)
        {
            ppSortArray[i] = (LPCTSTR)m_Liste1.GetAt(i);
        }

        m_List.RemoveAll();

        qsort(ppSortArray, m_Count, sizeof(LPCTSTR), CompareString);

        for (int i = 0; i < m_Count; i++)
        {
            m_List.Add((LPCTSTR) ppSortArray[i]);
        }
        m_Liste1.RemoveAll();
        delete [] ppSortArray;
    }
}

/*********************************************************************/
/*
** Vers. 1.1
** NEW: CopyList()
*/
void CACListWnd::CopyList()
{
    m_DisplayList.Copy(m_SearchList);
    m_lCount = m_DisplayList.GetSize();
    if (m_lCount)
    {
        FindString(0, "", true);
    }
}

/*********************************************************************/

void CACListWnd::AddHistoryString(LPCTSTR lpszString)
{
    // Remove duplicates first.
    for (int i = 0; i < m_SearchList.GetCount(); )
    {
        if (m_SearchList[i] == lpszString)
        {
            m_SearchList.RemoveAt(i);
        }
        else
        {
            ++i;
        }
    }

    // Insert the new item to the head.
    m_SearchList.InsertAt(0, lpszString);

    // Limit the history count.
    while (m_SearchList.GetCount() > m_nHistory)
    {
        m_SearchList.RemoveAt(m_SearchList.GetUpperBound());
    }
}

/*********************************************************************/

