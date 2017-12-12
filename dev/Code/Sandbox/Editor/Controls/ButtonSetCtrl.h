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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_BUTTONSETCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_BUTTONSETCTRL_H
#pragma once

struct SButtonItem
    : public CRefCountBase
{
    SButtonItem()
        : m_Name("")
        , m_Color(1, 1, 1)
    {
    }
    SButtonItem(const CString& name, const Vec3& color)
        : m_Name(name)
        , m_Color(color)
    {
    }
    SButtonItem(const SButtonItem& item)
        : m_Name(item.m_Name)
        , m_Color(item.m_Color)
    {
    }

    CString m_Name;
    Vec3 m_Color;
};

class IButtonEventHandler
{
public:
    virtual void OnMouseEventFromButtons(EMouseEvent event, CButton* pButton, UINT nFlags, CPoint point) = 0;
};

class CButtonEx
    : public CButton
{
public:

    CButtonEx(IButtonEventHandler* pEventHandler)
        : CButton()
    {
        m_pEventHandler = pEventHandler;
    }

protected:

    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:

    IButtonEventHandler* m_pEventHandler;
};

typedef _smart_ptr<SButtonItem> ButtonItemPtr;

class CButtonSetCtrl
    : public CWnd
    , public IButtonEventHandler
{
public:

    CButtonSetCtrl();
    ~CButtonSetCtrl();

    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

    ButtonItemPtr AddButton(const SButtonItem& buttonItem);
    ButtonItemPtr HitTest(const CPoint& point) const;

    virtual void DeleteButton(const ButtonItemPtr& pButtonItem);
    virtual void RemoveAll();

protected:

    afx_msg void OnDestroy();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg virtual void OnLButtonUp(UINT nFlags, CPoint point) {}

    virtual void OnMouseEventFromButtons(EMouseEvent event, CButton* pButton, UINT nFlags, CPoint point) override;

    DECLARE_MESSAGE_MAP()

    CString NewButtonName(const char*  baseName) const;

    void UpdateButtons();

    void InsertButton(int nIndex, ButtonItemPtr pButtonItem);
    ButtonItemPtr InsertButton(int nIndex, const SButtonItem& buttonItem);

    bool MoveButton(int nSrcIndex, int nDestIndex);

    CRect GetButtonRect(int nIndex, const CString& name);
    int GetButtonIndex(ButtonItemPtr pButton);
    ButtonItemPtr GetButtonItem(CButton* pButton) const;
    ButtonItemPtr GetButtonItem(int nIndex)   {   return m_ButtonList[nIndex].m_pItem;    }
    int GetButtonListCount() const {    return m_ButtonList.size(); }

    bool FindGapRect(CPoint point, CRect& outRect, int& nOutIndex) const;
    void GetGapRects(int nIndex, std::vector<CRect>& outRects) const;

    bool CreateDragImage(ButtonItemPtr pButtonItem);
    void SortAlphabetically();

    void UpdateScrollRange();

    void ApplyScrollPosToRect(CRect& rect) const;

private:

    struct SButtonElement
    {
        CButtonEx* m_pButton;
        CRect m_Rect;
        ButtonItemPtr m_pItem;
    };
    std::vector<SButtonElement> m_ButtonList;
    CFont m_Font;

    CPoint m_CursorPosLButtonDown;

    bool m_bDragging;
    ButtonItemPtr m_pDraggedButtonItem;

    CImageList m_DragImage;

    CRect m_SelectedGapRect;
    int m_SelectedGapIndex;
    bool m_bInGapDragging;
};
#endif // CRYINCLUDE_EDITOR_CONTROLS_BUTTONSETCTRL_H
