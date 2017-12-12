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
#include "ButtonSetCtrl.h"

static const CPoint StartButtonPos(2, 2);

BEGIN_MESSAGE_MAP(CButtonEx, CButton)
ON_WM_LBUTTONDOWN()
ON_WM_RBUTTONDOWN()
ON_WM_MOUSEMOVE()
ON_WM_LBUTTONUP()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CButtonSetCtrl, CWnd)
ON_WM_CREATE()
ON_WM_DESTROY()
ON_WM_DRAWITEM()
ON_WM_PAINT()
ON_WM_ERASEBKGND()
ON_WM_VSCROLL()
END_MESSAGE_MAP()

CButtonSetCtrl::CButtonSetCtrl()
{
    m_bInGapDragging = false;
    m_bDragging = false;
    m_SelectedGapIndex = -1;
}

CButtonSetCtrl::~CButtonSetCtrl()
{
    RemoveAll();
}

void CButtonSetCtrl::OnDestroy()
{
    RemoveAll();
}

int CButtonSetCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    CDC* pDC = GetDC();
    m_Font.CreatePointFont(90, "Arial");
    pDC->SelectObject(m_Font);

    return CWnd::OnCreate(lpCreateStruct);
}

BOOL CButtonSetCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
    return CWnd::Create(NULL, NULL, dwStyle, rect, pParentWnd, nID);
}

ButtonItemPtr CButtonSetCtrl::AddButton(const SButtonItem& buttonItem)
{
    return InsertButton(m_ButtonList.size(), buttonItem);
}

void CButtonSetCtrl::InsertButton(int nIndex, ButtonItemPtr pButtonItem)
{
    SButtonElement element;

    element.m_Rect = GetButtonRect(nIndex, pButtonItem->m_Name);
    element.m_pButton = new CButtonEx(this);
    element.m_pButton->Create(pButtonItem->m_Name, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_FLAT | BS_PUSHBUTTON, element.m_Rect, this, 0);
    element.m_pItem = pButtonItem;

    if (nIndex >= m_ButtonList.size())
    {
        m_ButtonList.push_back(element);
    }
    else
    {
        m_ButtonList.insert(m_ButtonList.begin() + nIndex, element);
    }

    UpdateScrollRange();
}

ButtonItemPtr CButtonSetCtrl::InsertButton(int nIndex, const SButtonItem& buttonItem)
{
    ButtonItemPtr pNewItem = new SButtonItem(buttonItem);
    InsertButton(nIndex, pNewItem);
    return pNewItem;
}

bool CButtonSetCtrl::MoveButton(int nSrcIndex, int nDestIndex)
{
    if (nSrcIndex < 0 || nSrcIndex >= m_ButtonList.size())
    {
        return false;
    }

    if (nDestIndex < 0 || nDestIndex > m_ButtonList.size())
    {
        return false;
    }

    if (nSrcIndex == nDestIndex)
    {
        return false;
    }

    m_ButtonList.insert(m_ButtonList.begin() + nDestIndex, m_ButtonList[nSrcIndex]);

    int nAdjustedSrcIndex = nSrcIndex;
    if (nSrcIndex > nDestIndex)
    {
        nAdjustedSrcIndex = (nSrcIndex + 1);
    }

    m_ButtonList.erase(m_ButtonList.begin() + nAdjustedSrcIndex);

    UpdateButtons();

    return true;
}

void CButtonSetCtrl::DeleteButton(const ButtonItemPtr& pButtonItem)
{
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        if (m_ButtonList[i].m_pItem == pButtonItem)
        {
            m_ButtonList.erase(m_ButtonList.begin() + i);
            break;
        }
    }
    UpdateScrollRange();
    Invalidate();
}

void CButtonSetCtrl::UpdateScrollRange()
{
    CRect rect;
    GetClientRect(&rect);
    int nLastIdx = m_ButtonList.size() - 1;

    if (m_ButtonList.empty() || m_ButtonList[nLastIdx].m_Rect.bottom < rect.bottom)
    {
        SetScrollRange(SB_VERT, 0, 0);
        return;
    }

    SetScrollRange(SB_VERT, 0, m_ButtonList[nLastIdx].m_Rect.bottom - rect.bottom);
}

int CButtonSetCtrl::GetButtonIndex(ButtonItemPtr pButton)
{
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        if (m_ButtonList[i].m_pItem == pButton)
        {
            return i;
        }
    }
    return -1;
}

void CButtonSetCtrl::RemoveAll()
{
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        if (m_ButtonList[i].m_pButton)
        {
            delete m_ButtonList[i].m_pButton;
        }
    }
    m_ButtonList.clear();

    if (GetSafeHwnd())
    {
        UpdateScrollRange();
        Invalidate();
    }
}

void CButtonSetCtrl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    int nIndex = -1;
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        if (nIDCtl == m_ButtonList[i].m_pButton->GetDlgCtrlID())
        {
            nIndex = i;
            break;
        }
    }

    if (nIndex == -1)
    {
        return;
    }

    SButtonElement& buttonElement = m_ButtonList[nIndex];

    CDC dc;
    dc.Attach(lpDrawItemStruct->hDC);
    dc.SelectObject(m_Font);
    CRect rect;
    rect = lpDrawItemStruct->rcItem;

    int nScrollVPos = GetScrollPos(SB_VERT);
    rect.top -= nScrollVPos;
    rect.bottom -= nScrollVPos;

    ColorF itemColor(buttonElement.m_pItem->m_Color);
    dc.FillSolidRect(&rect, itemColor.pack_bgr888());
    UINT state = lpDrawItemStruct->itemState;

    dc.SetBkColor(itemColor.pack_bgr888());
    dc.SetTextColor(RGB(0, 0, 0));
    dc.SetBkMode(TRANSPARENT);

    dc.DrawText(buttonElement.m_pItem->m_Name, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    dc.SetDCPenColor(RGB(0, 0, 0));
    dc.MoveTo(rect.left, rect.top);
    dc.LineTo(rect.right - 1, rect.top);
    dc.LineTo(rect.right - 1, rect.bottom - 1);
    dc.LineTo(rect.left, rect.bottom - 1);
    dc.LineTo(rect.left, rect.top);

    dc.Detach();
}

CRect CButtonSetCtrl::GetButtonRect(int nIndex, const CString& name)
{
    CSize textSize = GetDC()->GetTextExtent(name);
    textSize += CSize(2, 2);

    CRect rect;
    GetClientRect(&rect);

    CPoint offset(StartButtonPos);
    if (!m_ButtonList.empty() && nIndex > 0)
    {
        if (nIndex > m_ButtonList.size())
        {
            nIndex = m_ButtonList.size();
        }
        offset.y = m_ButtonList[nIndex - 1].m_Rect.top;
        offset.x += m_ButtonList[nIndex - 1].m_Rect.right + 1;
    }

    if (offset.x + textSize.cx > rect.right)
    {
        offset.y += textSize.cy + 1;
        offset.x = StartButtonPos.x;
    }

    return CRect(offset, textSize);
}

CString CButtonSetCtrl::NewButtonName(const char* baseName) const
{
    assert(baseName);

    int nNewIndex = 0;
    int nCount = 0;

    while (nCount++ < 100000000)
    {
        CString extendedName;
        extendedName.Format("%s%d", baseName, nNewIndex++);

        bool bSameExist = false;
        for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
        {
            if (!_stricmp(m_ButtonList[i].m_pItem->m_Name.GetBuffer(), extendedName.GetBuffer()))
            {
                bSameExist = true;
                break;
            }
        }

        if (!bSameExist)
        {
            return extendedName;
        }
    }

    assert(0);
    return "";
}


ButtonItemPtr CButtonSetCtrl::HitTest(const CPoint& point) const
{
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        CRect winRect;
        m_ButtonList[i].m_pButton->GetWindowRect(&winRect);
        ApplyScrollPosToRect(winRect);
        if (winRect.PtInRect(point))
        {
            return m_ButtonList[i].m_pItem;
        }
    }

    return NULL;
}

BOOL CButtonSetCtrl::OnEraseBkgnd(CDC* pDC)
{
    CRect rect;

    GetClientRect(&rect);

    HBRUSH hbr = (HBRUSH)::GetSysColorBrush(COLOR_BTNFACE);
    CBrush brush;
    brush.Attach(hbr);
    pDC->FillRect(&rect, &brush);
    brush.Detach();
    return CWnd::OnEraseBkgnd(pDC);
}

void CButtonSetCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    int nNewPos = -1;

    if (nSBCode == SB_LINEDOWN)
    {
        nNewPos = GetScrollPos(SB_VERT) + 2;
    }
    else if (nSBCode == SB_LINEUP)
    {
        nNewPos = GetScrollPos(SB_VERT) - 2;
    }
    else if (nSBCode == SB_THUMBPOSITION || nSBCode == SB_THUMBTRACK)
    {
        nNewPos = nPos;
    }
    else if (nSBCode == SB_PAGEDOWN)
    {
        CRect rect;
        GetClientRect(&rect);
        nNewPos = GetScrollPos(SB_VERT) + rect.Height();
    }
    else if (nSBCode == SB_PAGEUP)
    {
        CRect rect;
        GetClientRect(&rect);
        nNewPos = GetScrollPos(SB_VERT) - rect.Height();
    }
    else if (nSBCode == SB_TOP)
    {
        nNewPos = 0;
    }
    else if (nSBCode == SB_BOTTOM)
    {
        int nWidth = 0;
        int nHeight = 0;
        GetScrollRange(SB_VERT, &nWidth, &nHeight);
        CRect rect;
        GetClientRect(&rect);
        nNewPos = nHeight - rect.Height();
    }

    if (nNewPos != -1)
    {
        SetScrollPos(SB_VERT, nNewPos);
        Invalidate();
    }

    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CButtonSetCtrl::OnPaint()
{
    CWnd::OnPaint();

    CRect clientRect;
    GetClientRect(&clientRect);

    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        CRect rect(m_ButtonList[i].m_Rect);
        ApplyScrollPosToRect(rect);
        if (rect.bottom < 0 || rect.top > clientRect.bottom)
        {
            continue;
        }

        DRAWITEMSTRUCT dis;
        dis.CtlType = ODT_BUTTON;
        dis.CtlID = m_ButtonList[i].m_pButton->GetDlgCtrlID();
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = ODS_DEFAULT;
        dis.hwndItem = m_ButtonList[i].m_pButton->GetSafeHwnd();
        dis.hDC = m_ButtonList[i].m_pButton->GetDC()->GetSafeHdc();
        m_ButtonList[i].m_pButton->GetClientRect(&dis.rcItem);
        dis.itemData = 0;

        SendMessage(WM_DRAWITEM, (WPARAM)dis.CtlID, (LPARAM)&dis);
    }

    if (m_bInGapDragging)
    {
        CDC* pDC = GetDC();

        int nCenterX = (m_SelectedGapRect.left + m_SelectedGapRect.right) * 0.5f;
        CRect gapRect;
        gapRect.left = nCenterX - 2;
        gapRect.right = nCenterX + 2;
        gapRect.top = m_SelectedGapRect.top;
        gapRect.bottom = m_SelectedGapRect.bottom;
        pDC->FillSolidRect(gapRect, RGB(100, 200, 100));
    }
}

void CButtonSetCtrl::UpdateButtons()
{
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        m_ButtonList[i].m_Rect = GetButtonRect(i, m_ButtonList[i].m_pItem->m_Name);
    }

    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        m_ButtonList[i].m_pButton->MoveWindow(&m_ButtonList[i].m_Rect, false);
    }

    Invalidate();
}

bool CButtonSetCtrl::FindGapRect(CPoint point, CRect& outRect, int& nOutIndex) const
{
    ScreenToClient(&point);
    for (int i = 0, iSize(m_ButtonList.size()); i <= iSize; ++i)
    {
        std::vector<CRect> gapRects;
        GetGapRects(i, gapRects);
        for (int k = 0, iRectSize(gapRects.size()); k < iRectSize; ++k)
        {
            if (gapRects[k].PtInRect(point))
            {
                nOutIndex = i;
                outRect = gapRects[k];
                return true;
            }
        }
    }
    return false;
}

ButtonItemPtr CButtonSetCtrl::GetButtonItem(CButton* pButton) const
{
    for (int i = 0, iSize(m_ButtonList.size()); i < iSize; ++i)
    {
        if (m_ButtonList[i].m_pButton == pButton)
        {
            return m_ButtonList[i].m_pItem;
        }
    }
    return NULL;
}

void CButtonSetCtrl::GetGapRects(int nIndex, std::vector<CRect>& outRects) const
{
    int nPrevIndex = nIndex - 1;
    const int rectHalfWidth = 8;
    int nListCount = m_ButtonList.size();

    CRect gapRect(0, 0, 0, 0);
    if (nPrevIndex < 0 || nIndex >= nListCount || m_ButtonList[nPrevIndex].m_Rect.top == m_ButtonList[nIndex].m_Rect.top)
    {
        if (nPrevIndex >= 0)
        {
            gapRect.left = m_ButtonList[nPrevIndex].m_Rect.right - rectHalfWidth;
            gapRect.top = m_ButtonList[nPrevIndex].m_Rect.top;
        }
        else
        {
            gapRect.left = m_ButtonList[nIndex].m_Rect.left - rectHalfWidth;
            gapRect.top = m_ButtonList[nIndex].m_Rect.top;
        }

        if (nIndex < nListCount)
        {
            gapRect.right = m_ButtonList[nIndex].m_Rect.left + rectHalfWidth;
            gapRect.bottom = m_ButtonList[nIndex].m_Rect.bottom;
        }
        else if (nPrevIndex >= 0)
        {
            gapRect.right = m_ButtonList[nPrevIndex].m_Rect.right + rectHalfWidth;
            gapRect.bottom = m_ButtonList[nPrevIndex].m_Rect.bottom;
        }

        ApplyScrollPosToRect(gapRect);
        outRects.push_back(gapRect);
    }
    else
    {
        gapRect.left = m_ButtonList[nPrevIndex].m_Rect.right;
        gapRect.top  = m_ButtonList[nPrevIndex].m_Rect.top;
        gapRect.right = gapRect.left + rectHalfWidth;
        gapRect.bottom = m_ButtonList[nPrevIndex].m_Rect.bottom;
        ApplyScrollPosToRect(gapRect);
        outRects.push_back(gapRect);

        gapRect.left = m_ButtonList[nIndex].m_Rect.left - rectHalfWidth;
        gapRect.top  = m_ButtonList[nIndex].m_Rect.top;
        gapRect.right = gapRect.left + rectHalfWidth;
        gapRect.bottom = m_ButtonList[nIndex].m_Rect.bottom;
        ApplyScrollPosToRect(gapRect);
        outRects.push_back(gapRect);
    }
}

void CButtonSetCtrl::OnMouseEventFromButtons(EMouseEvent event, CButton* pButton, UINT nFlags, CPoint point)
{
    if (event == eMouseMove)
    {
        if (nFlags == MK_LBUTTON)
        {
            CPoint cursorPos;
            GetCursorPos(&cursorPos);

            if (m_bDragging)
            {
                CRect rcGap;
                int nGapIndex;
                if (FindGapRect(cursorPos, rcGap, nGapIndex))
                {
                    if (m_SelectedGapIndex != nGapIndex)
                    {
                        m_SelectedGapRect = rcGap;
                        m_SelectedGapIndex = nGapIndex;
                        m_bInGapDragging = true;
                        Invalidate();
                    }
                }
                else
                {
                    int nPrevGapIndex = m_SelectedGapIndex;
                    m_SelectedGapIndex = -1;
                    m_bInGapDragging = false;
                    if (nPrevGapIndex != -1)
                    {
                        Invalidate();
                    }
                }

                CRect wndRect;
                GetWindowRect(&wndRect);
                if (cursorPos.x > wndRect.left && cursorPos.x < wndRect.right)
                {
                    if (cursorPos.y > wndRect.top - 3 && cursorPos.y < wndRect.top + 3)
                    {
                        OnVScroll(SB_LINEUP, 0, NULL);
                    }
                    else if (cursorPos.y > wndRect.bottom - 3 && cursorPos.y < wndRect.bottom + 3)
                    {
                        OnVScroll(SB_LINEDOWN, 0, NULL);
                    }
                }

                m_DragImage.DragMove(cursorPos);
            }
            else if (abs(m_CursorPosLButtonDown.x - cursorPos.x) > 2 || abs(m_CursorPosLButtonDown.y - cursorPos.y) > 2)
            {
                if (m_pDraggedButtonItem)
                {
                    m_bDragging = true;
                    CreateDragImage(m_pDraggedButtonItem);
                    CPoint posInButton(m_CursorPosLButtonDown);
                    pButton->ScreenToClient(&posInButton);
                    m_DragImage.BeginDrag(0, posInButton);
                    m_DragImage.DragEnter(NULL, m_CursorPosLButtonDown);
                }
            }
        }
    }
    else if (event == eMouseLDown)
    {
        GetCursorPos(&m_CursorPosLButtonDown);
        m_pDraggedButtonItem = HitTest(m_CursorPosLButtonDown);
        m_bDragging = false;
    }
    else if (event == eMouseLUp)
    {
        if (m_bDragging)
        {
            m_DragImage.DragLeave(this);
            m_DragImage.EndDrag();
            if (m_bInGapDragging && m_pDraggedButtonItem)
            {
                MoveButton(GetButtonIndex(m_pDraggedButtonItem), m_SelectedGapIndex);
            }
            Invalidate();
            m_bDragging = false;
            m_bInGapDragging = false;
            m_SelectedGapIndex = -1;
        }
        else
        {
            OnLButtonUp(nFlags, point);
        }
    }
}

bool CButtonSetCtrl::CreateDragImage(ButtonItemPtr pButtonItem)
{
    if (pButtonItem == NULL)
    {
        return false;
    }

    CBitmap bitmap;
    CDC* pDC = GetDC();
    if (pDC == NULL)
    {
        return false;
    }

    CString itemText = pButtonItem->m_Name;
    CSize stringSize = pDC->GetTextExtent(itemText);
    CRect itemRect(0, 0, stringSize.cx, stringSize.cy);
    if (!bitmap.CreateCompatibleBitmap(pDC, itemRect.Width(), itemRect.Height()))
    {
        return false;
    }

    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    memDC.SelectObject(bitmap);
    memDC.SetBkColor(ColorF(pButtonItem->m_Color).pack_bgr888());
    memDC.Rectangle(itemRect);
    memDC.DrawText(itemText, itemRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    m_DragImage.DeleteImageList();
    m_DragImage.Create(itemRect.Width(), itemRect.Height(), ILC_COLOR24, 1, 2);
    if (m_DragImage.Add(&bitmap, RGB(255, 255, 255)) < 0)
    {
        return false;
    }

    return true;
}

void CButtonSetCtrl::SortAlphabetically()
{
    std::map<CString, int> sortedButtonMap;

    for (int i = 0, iSize(GetButtonListCount()); i < iSize; ++i)
    {
        ButtonItemPtr pButtonItem = GetButtonItem(i);
        sortedButtonMap[pButtonItem->m_Name] = i;
    }

    std::vector<SButtonElement> sortedList;
    std::map<CString, int>::iterator ii = sortedButtonMap.begin();
    for (; ii != sortedButtonMap.end(); ++ii)
    {
        sortedList.push_back(m_ButtonList[ii->second]);
    }

    m_ButtonList = sortedList;
}

void CButtonSetCtrl::ApplyScrollPosToRect(CRect& rect) const
{
    int nVScrollPos = GetScrollPos(SB_VERT);

    rect.top -= nVScrollPos;
    rect.bottom -= nVScrollPos;
}

void CButtonEx::OnMouseMove(UINT nFlags, CPoint point)
{
    CButton::OnMouseMove(nFlags, point);
    if (m_pEventHandler)
    {
        m_pEventHandler->OnMouseEventFromButtons(eMouseMove, this, nFlags, point);
    }
}

void CButtonEx::OnLButtonDown(UINT nFlags, CPoint point)
{
    SetCapture();
    CButton::OnLButtonDown(nFlags, point);
    if (m_pEventHandler)
    {
        m_pEventHandler->OnMouseEventFromButtons(eMouseLDown, this, nFlags, point);
    }
}

void CButtonEx::OnLButtonUp(UINT nFlags, CPoint point)
{
    CButton::OnLButtonUp(nFlags, point);
    if (m_pEventHandler)
    {
        m_pEventHandler->OnMouseEventFromButtons(eMouseLUp, this, nFlags, point);
    }
    ReleaseCapture();
}

void CButtonEx::OnRButtonDown(UINT nFlags, CPoint point)
{
    CButton::OnRButtonDown(nFlags, point);
    if (m_pEventHandler)
    {
        m_pEventHandler->OnMouseEventFromButtons(eMouseRDown, this, nFlags, point);
    }
}

BOOL CButtonEx::OnEraseBkgnd(CDC* pDC)
{
    return false;
}