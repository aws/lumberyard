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

#include "stdafx.h"
#include "SubeditListCtrl.h"

#ifdef SubclassWindow
#undef SubclassWindow
#endif

CSubEdit::CSubEdit()
{
}

CSubEdit::~CSubEdit()
{
}


BEGIN_MESSAGE_MAP(CSubEdit, CEdit)
//{{AFX_MSG_MAP(CSubEdit)
ON_WM_WINDOWPOSCHANGING()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSubEdit message handlers

void CSubEdit::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos)
{
    lpwndpos->x = m_x;
    lpwndpos->cx = m_cx;

    CEdit::OnWindowPosChanging(lpwndpos);

    // TODO: Add your message handler code here
}

/////////////////////////////////////////////////////////////////////////////
// CSubeditListCtrl

//IMPLEMENT_DYNCREATE(CSubeditListCtrl, CListCtrl)

CSubeditListCtrl::CSubeditListCtrl()
{
    m_subitem = 0;
}

CSubeditListCtrl::~CSubeditListCtrl()
{
}


BEGIN_MESSAGE_MAP(CSubeditListCtrl, CListCtrl)
//{{AFX_MSG_MAP(CSubeditListCtrl)
ON_WM_LBUTTONDOWN()
ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, OnBeginLabelEdit)
ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndLabelEdit)
ON_WM_PAINT()
ON_WM_SIZE()
ON_CONTROL(CBN_SELCHANGE, ComboBoxEditSession::ID_COMBO_BOX, OnComboSelChange)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSubeditListCtrl drawing

/////////////////////////////////////////////////////////////////////////////
// CSubeditListCtrl diagnostics

#ifdef _DEBUG
void CSubeditListCtrl::AssertValid() const
{
    CListCtrl::AssertValid();
}

void CSubeditListCtrl::Dump(CDumpContext& dc) const
{
    CListCtrl::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSubeditListCtrl message handlers

BOOL CSubeditListCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= LVS_REPORT | LVS_EDITLABELS;
    return CListCtrl::PreCreateWindow(cs);
}

void CSubeditListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    LVHITTESTINFO   lvhit;
    lvhit.pt = point;
    int item = this->SubItemHitTest(&lvhit);

    //if (over a subitem)
    if (item != -1 && lvhit.iSubItem && (lvhit.flags & LVHT_ONITEM))
    {
        //mouse click outside the editbox in an allready editing cell cancels editing
        if (m_subitem != lvhit.iSubItem || item != m_item)
        {
            m_subitem = lvhit.iSubItem;
            m_item = item;
            switch (GetEditStyle(m_item, m_subitem))
            {
            case EDIT_STYLE_NONE:
            {
            }
            break;

            case EDIT_STYLE_EDIT:
            {
                this->SetFocus();
                this->EditLabel(item);
            }
            break;

            case EDIT_STYLE_COMBO:
            {
                std::vector<string> options;
                string currentOption;
                this->GetOptions(m_item, m_subitem, options, currentOption);
                this->EditLabelCombo(m_item, m_subitem, options, currentOption);
            }
            break;
            }
        }
    }
    else
    {
        CListCtrl::OnLButtonDown(nFlags, point);
    }
}

void CSubeditListCtrl::EditLabelCombo(int itemIndex, int subItemIndex, const std::vector<string>& options, const string& currentOption)
{
    m_comboBoxEditSession.Begin(this, itemIndex, subItemIndex, options, currentOption);
    MSG msg;
    while (m_comboBoxEditSession.IsRunning())
    {
        if (!::GetMessage(&msg, NULL, NULL, NULL))
        {
            break;
        }
        if (msg.message == WM_LBUTTONDOWN)
        {
            CPoint cursorPos;
            ::GetCursorPos(&cursorPos);
            m_comboBoxEditSession.ScreenToClient(&cursorPos);
            bool inCombo = false;
            CRect comboClientRect;
            m_comboBoxEditSession.GetClientRect(&comboClientRect);
            if (comboClientRect.PtInRect(cursorPos))
            {
                inCombo = true;
            }
            COMBOBOXINFO comboBoxInfo = {0};
            comboBoxInfo.cbSize = sizeof(comboBoxInfo);
            m_comboBoxEditSession.GetComboBoxInfo(&comboBoxInfo);
            ::GetWindowRect(comboBoxInfo.hwndList, &comboClientRect);
            m_comboBoxEditSession.ScreenToClient(&comboClientRect);
            if (comboClientRect.PtInRect(cursorPos))
            {
                inCombo = true;
            }
            if (!inCombo)
            {
                m_comboBoxEditSession.End(false);
            }
        }

        if (m_comboBoxEditSession.IsRunning())
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

void CSubeditListCtrl::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

    // Assume editting is not permitted.
    *pResult = 1;

    if (pDispInfo->item.iItem >= 0 && m_subitem >= 0 && GetEditStyle(pDispInfo->item.iItem, m_subitem) != EDIT_STYLE_NONE)
    {
        if (m_subitem)
        {
            ASSERT(m_item == pDispInfo->item.iItem);

            CRect   subrect;
            this->GetSubItemRect(pDispInfo->item.iItem, m_subitem, LVIR_BOUNDS, subrect);

            //get edit control and subclass
            HWND hWnd = (HWND)SendMessage(LVM_GETEDITCONTROL);
            ASSERT(hWnd != NULL);
            VERIFY(m_editWnd.SubclassWindow(hWnd));

            //move edit control text 1 pixel to the right of org label, as Windows does it...
            m_editWnd.m_x = subrect.left + 6;
            m_editWnd.m_cx = subrect.right - subrect.left - 6;
            m_editWnd.SetWindowText(this->GetItemText(pDispInfo->item.iItem, m_subitem));

            //hide subitem text so it don't show if we delete some text in the editcontrol
            //OnPaint handles other issues also regarding this
            CRect   rect;
            this->GetSubItemRect(pDispInfo->item.iItem, m_subitem, LVIR_LABEL, rect);
            CDC* hDc = GetDC();
            hDc->FillRect(rect, &CBrush(::GetSysColor(COLOR_WINDOW)));
            ReleaseDC(hDc);
        }

        // Allow editting.
        *pResult = 0;
    }
}

void CSubeditListCtrl::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO* plvDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM* plvItem = &plvDispInfo->item;

    //if (end of sub-editing) do cleanup
    //plvItem->pszText is NULL if editing canceled
    if (plvItem->pszText != NULL)
    {
        this->TextChanged(plvItem->iItem, m_subitem, plvItem->pszText);
    }

    if (m_subitem != 0 && m_editWnd.GetSafeHwnd() != nullptr)
    {
        VERIFY(m_editWnd.UnsubclassWindow());
    }

    m_subitem = 0;
    //allways revert to org label (Windows thinks we are editing the leftmost item)
    *pResult = 0;
}

void CSubeditListCtrl::OnPaint()
{
    //if (subitem editing)
    if (m_subitem && !m_comboBoxEditSession.IsRunning() && m_editWnd.GetSafeHwnd())
    {
        CRect   rect;
        CRect   editrect;

        this->GetSubItemRect(m_item, m_subitem, LVIR_LABEL, rect);

        m_editWnd.GetWindowRect(editrect);
        ScreenToClient(editrect);

        //block text redraw of the subitems text (underneath the editcontrol)
        //if we didn't do this and deleted some text in the edit control,
        //the subitems original label would show
        if (editrect.right < rect.right)
        {
            rect.left = editrect.right;
            ValidateRect(rect);
        }

        //block filling redraw of leftmost item (caused by FillRect)
        this->GetItemRect(m_item, rect, LVIR_LABEL);
        ValidateRect(rect);
    }

    CListCtrl::OnPaint();
}

void CSubeditListCtrl::OnSize(UINT nType, int cx, int cy)
{
    //stop editing if resizing
    if (GetFocus() != this)
    {
        SetFocus();
    }

    CListCtrl::OnSize(nType, cx, cy);
}

void CSubeditListCtrl::OnComboSelChange()
{
    m_comboBoxEditSession.End(true);
}

void CSubeditListCtrl::TextChanged(int item, int subitem, const char* szText)
{
    this->SetItemText(item, subitem, szText);
}

CSubeditListCtrl::ComboBoxEditSession::ComboBoxEditSession()
    :   m_list(0)
    , m_itemIndex(-1)
    , m_subItemIndex(-1)
{
}

bool CSubeditListCtrl::ComboBoxEditSession::IsRunning() const
{
    return m_itemIndex >= 0;
}

void CSubeditListCtrl::ComboBoxEditSession::Begin(CSubeditListCtrl* list, int itemIndex, int subItemIndex, const std::vector<string>& options, const string& currentOption)
{
    m_list = list;
    CRect rect;
    m_list->GetSubItemRect(itemIndex, subItemIndex, LVIR_LABEL, rect);
    rect.top -= 4;
    rect.left += 2;
    Create(WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_DISABLENOSCROLL | CBS_SORT | WS_VISIBLE | WS_TABSTOP, rect, m_list, ID_COMBO_BOX);
    for (std::vector<string>::const_iterator itOption = options.begin(); itOption != options.end(); ++itOption)
    {
        AddString((*itOption).c_str());
    }
    SelectString(0, currentOption.c_str());
    m_itemIndex = itemIndex;
    m_subItemIndex = subItemIndex;
    SetFont(m_list->GetFont());
    SetFocus();
    ShowDropDown();
}

void CSubeditListCtrl::ComboBoxEditSession::End(bool accept)
{
    CString text;
    GetWindowText(text);
    if (accept)
    {
        m_list->TextChanged(m_itemIndex, m_subItemIndex, text.GetString());
    }
    DestroyWindow();
    m_itemIndex = -1;
    m_subItemIndex = -1;
    m_list->m_subitem = 0;
    m_list->m_item = -1;
    m_list = 0;
}

BEGIN_MESSAGE_MAP(CSubeditListCtrl::ComboBoxEditSession, CComboBox)
END_MESSAGE_MAP()
