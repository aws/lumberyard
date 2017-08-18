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

#ifndef CRYINCLUDE_EDITOR_CUSTOMCOLORDIALOG_H
#define CRYINCLUDE_EDITOR_CUSTOMCOLORDIALOG_H

#pragma once
// CustomColorDialog.h : header file
//
// This class extents CMFCColorDialog for two reasons:
//  1. Enables the 'ColorChangeCallback' registration
//  2. Makes the color picking tool work across multiple monitors

#include <afxcolordialog.h>

/////////////////////////////////////////////////////////////////////////////
// CCustomColorDialog dialog

class CCustomColorDialog
    : public CMFCColorDialog
{
    DECLARE_DYNAMIC(CCustomColorDialog)
public:
    typedef Functor1<COLORREF> ColorChangeCallback;

    CCustomColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0, CWnd* pParentWnd = NULL);

    void SetColorChangeCallback(ColorChangeCallback cb)
    { m_callback = cb; }

protected:
    //{{AFX_MSG(CCustomColorDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg LRESULT OnKickIdle(WPARAM, LPARAM lCount);
    afx_msg void OnColorSelect();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void OnColorChange( COLORREF col );
    BOOL PreTranslateMessage(MSG* pMsg) override;

    ColorChangeCallback m_callback;
    COLORREF m_colorPrev;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CUSTOMCOLORDIALOG_H
