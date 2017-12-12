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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_EDITWITHBUTTON_H
#define CRYINCLUDE_EDITOR_CONTROLS_EDITWITHBUTTON_H
#pragma once

//DISCLAIMER:
//The code in this project is Copyright (C) 2006 by Gautam Jain. You have the right to
//use and distribute the code in any way you see fit as long as this paragraph is included
//with the distribution. No warranties or claims are made as to the validity of the
//information and code contained herein, so use it at your own risk.

// CEditWithButton

class CEditWithButton
    : public CEdit
{
    DECLARE_DYNAMIC(CEditWithButton)

protected:
    static const int BUTTON_COUNT = 2;
    CBitmap m_bmpEmptyEdit;
    CBitmap m_bmpFilledEdit;
    CSize   m_sizeEmptyBitmap;
    CSize   m_sizeFilledBitmap;
    CRect   m_rcEditArea;
    CRect   m_rcButtonArea[BUTTON_COUNT];
    BOOL    m_bButtonExistsAlways[BUTTON_COUNT];
    UINT    m_iButtonClickedMessageId;

public:
    BOOL SetBitmaps(UINT iEmptyEdit, UINT iFilledEdit);
    void SetFirstButtonArea(CRect rcButtonArea);
    void SetSecondButtonArea(CRect rcButtonArea);
    BOOL SetEditArea(CRect rcEditArea);
    void SetButtonClickedMessageId(UINT iButtonClickedMessageId);
    void SetFirstButtonExistsAlways(BOOL bButtonExistsAlways);
    void SetSecondButtonExistsAlways(BOOL bButtonExistsAlways);

    CEditWithButton();
    virtual ~CEditWithButton();
    virtual void PreSubclassWindow();
    virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
    void ResizeWindow();

    DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam); // Maps to WM_SETFONT
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_EDITWITHBUTTON_H
