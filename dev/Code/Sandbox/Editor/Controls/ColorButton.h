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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_COLORBUTTON_H
#define CRYINCLUDE_EDITOR_CONTROLS_COLORBUTTON_H
#pragma once



#include <afxwin.h>


// To use:
// 1) Add button to dialog (optionally set "Owner draw" property to "True",
//    but it's been enforced by the code already -- see CColorButton::PreSubclassWindow() )
// 2) Create member variable for button in dialog (via wizard or manually) .
// 3) Initialize button (color, text display) cia c/tor or Set*** in dialog's Constructor/OnInitDialog.
// 4) If added manually (step 2) add a something like "DDX_Control( pDX, IDC_COLOR_BUTTON, m_myColButton );"
//    to your dialog's DoDataExchange( ... ) method.


class CColorButton
    : public CButton
{
public:

public:
    CColorButton();
    CColorButton(COLORREF color, bool showText);
    virtual ~CColorButton();

    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    virtual void PreSubclassWindow();

    void SetColor(const COLORREF& col);
    COLORREF GetColor() const;

    void SetTextColor(const COLORREF& col);
    COLORREF GetTexColor() const { return m_textColor; };

    void SetShowText(bool showText);
    bool GetShowText() const;

protected:

    DECLARE_MESSAGE_MAP()

    COLORREF m_color;
    COLORREF m_textColor;
    bool m_showText;
};


#endif // CRYINCLUDE_EDITOR_CONTROLS_COLORBUTTON_H
