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
#include "AnimationToolBar.h"

/////////////////////////////////////////////////////////////////////////////
// CAnimationToolBar

CAnimationToolBar::CAnimationToolBar()
{
}

CAnimationToolBar::~CAnimationToolBar()
{
}


BEGIN_MESSAGE_MAP(CAnimationToolBar, CToolBar)
//{{AFX_MSG_MAP(CAnimationToolBar)
ON_WM_SIZE()
ON_WM_CTLCOLOR()
ON_WM_CREATE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnimationToolBar message handlers

void CAnimationToolBar::OnSize(UINT nType, int cx, int cy)
{
    ////////////////////////////////////////////////////////////////////////
    // Resize the keyframe slider
    ////////////////////////////////////////////////////////////////////////

    unsigned int iIndex;
    RECT rcCtrl, rcToolbar;

    CToolBar::OnSize(nType, cx, cy);

    if (!GetToolBarCtrl().GetButtonCount())
    {
        return;
    }

    // Get the index of the keyframe slider position in the toolbar
    iIndex = 0;
    while (GetItemID(iIndex) != IDR_KEYFRAMES)
    {
        iIndex++;
    }

    if (GetItemID(iIndex) != IDR_KEYFRAMES)
    {
        return;
    }

    // Get size and position of toolbar and slider control
    GetItemRect(iIndex, &rcCtrl);
    GetClientRect(&rcToolbar);

    // Get new slider width
    rcCtrl.right = rcToolbar.right;

    // Set in if the slider is created
    if (m_cKeyframes.m_hWnd)
    {
        m_cKeyframes.SetWindowPos(NULL, 0, 0, (rcToolbar.right - rcCtrl.left) / 2,
            18, SWP_NOMOVE | SWP_NOOWNERZORDER);
    }

    // Get the index of the animation sequence combo box position in the toolbar
    iIndex = 0;
    while (GetItemID(iIndex) != IDR_ANIM_SEQUENCES)
    {
        iIndex++;
    }

    if (GetItemID(iIndex) != IDR_ANIM_SEQUENCES)
    {
        return;
    }

    // Get size and position of toolbar and combo box control
    GetItemRect(iIndex, &rcCtrl);
    GetClientRect(&rcToolbar);

    // Get new combo box width
    rcCtrl.right = rcToolbar.right;

    // Set in if the combo box is created
    if (m_cAnimSequences.m_hWnd)
    {
        m_cAnimSequences.SetWindowPos(NULL, 0, 0, (rcToolbar.right - rcCtrl.left) / 2,
            18, SWP_NOMOVE | SWP_NOOWNERZORDER);

        // Place the combox box right of the keyframe slider
        GetItemRect(iIndex, &rcCtrl);
        rcCtrl.top += 3;
        rcCtrl.bottom += 200;
        rcCtrl.left = (rcToolbar.right) / 2 + 65;
        rcCtrl.right = (rcToolbar.right) / 2 - 75;
        ::SetWindowPos(m_cAnimSequences.m_hWnd, NULL, rcCtrl.left, rcCtrl.top,
            rcCtrl.right, rcCtrl.bottom, SWP_NOZORDER);
    }
}

HBRUSH CAnimationToolBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CToolBar::OnCtlColor(pDC, pWnd, nCtlColor);

    CFont cComboBoxFont;

    if (nCtlColor == CTLCOLOR_EDIT)
    {
        VERIFY(cComboBoxFont.CreatePointFont(60, "Terminal"));
        pDC->SelectObject(&cComboBoxFont);
    }

    // TODO: Return a different brush if the default is not desired
    return hbr;
}

int CAnimationToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CToolBar::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    if (!LoadToolBar(IDR_ANIMATION))
    {
        return -1;
    }

    // Create controls in the animation bar
    CRect rect;
    GetClientRect(&rect);

    // Get the index of the keyframe slider position in the toolbar
    int iIndex = 0;
    while (GetItemID(iIndex) != IDR_KEYFRAMES)
    {
        iIndex++;
    }

    // Convert that button to a seperator and get its position
    SetButtonInfo(iIndex, IDR_KEYFRAMES, TBBS_SEPARATOR, 0);
    GetItemRect(iIndex, &rect);

    // Expand the rectangle
    rect.top += 2;
    rect.bottom -= 2;

    // TODO: Remove WS_DISABLED when animation is implemented
    m_cKeyframes.Create(WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_BOTTOM | TBS_AUTOTICKS, rect, this, NULL);

    // Get the index of the animation sequence combo box position in the toolbar
    iIndex = 0;
    while (GetItemID(iIndex) != IDR_ANIM_SEQUENCES)
    {
        iIndex++;
    }

    // Convert that button to a seperator and get its position
    //SetButtonInfo(iIndex, IDR_ANIM_SEQUENCES, TBBS_SEPARATOR, 252);
    SetButtonInfo(iIndex, IDR_ANIM_SEQUENCES, TBBS_SEPARATOR, 0);
    GetItemRect(iIndex, &rect);

    // Expand the rectangle
    rect.top += 2;
    rect.bottom += 75;

    // TODO: Remove WS_DISABLED when animation is implemented
    m_cAnimSequences.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, rect, this, NULL);
    m_cKeyframes.SetRange(0, 50);

    return 0;
}

CString CAnimationToolBar::GetAnimName()
{
    int sel = m_cAnimSequences.GetCurSel();
    if (sel == CB_ERR)
    {
        return "";
    }
    CString str;
    m_cAnimSequences.GetLBText(sel, str);
    return str;
}
