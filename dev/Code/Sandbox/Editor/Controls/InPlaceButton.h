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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_INPLACEBUTTON_H
#define CRYINCLUDE_EDITOR_CONTROLS_INPLACEBUTTON_H
#pragma once


// CInPlaceButton

#include "ColorButton.h"

/*
class CXTPButtonNoBorderTheme : public CXTPButtonUltraFlatTheme
{
public:
    CXTPButtonNoBorderTheme()
    {
        m_bFlatGlyphs = TRUE;
        m_nBorderWidth = 3;
        m_bHiglightButtons = TRUE;
    }
    void RefreshMetrics(CXTPButton* pButton)
    {
        CXTPButtonUltraFlatTheme::RefreshMetrics(pButton);
    }
};
*/

//////////////////////////////////////////////////////////////////////////
class CInPlaceButton
    : public CXTPButton
{
    DECLARE_DYNAMIC(CInPlaceButton)

public:
    typedef Functor0 OnClick;

    CInPlaceButton(OnClick onClickFunctor);
    CInPlaceButton(OnClick onClickFunctor, bool bNoTheme);

    // Simulate on click.
    void Click() { OnBnClicked();   }

    void SetColorFace(int clr) {};

    void SetColor(COLORREF col) {};
    void SetTextColor(COLORREF col) {};
    COLORREF GetColor() const { return 0; };

protected:
    DECLARE_MESSAGE_MAP()
    int OnCreate(LPCREATESTRUCT lpCreateStruct);

public:
    afx_msg void OnBnClicked()
    {
        if (m_onClick)
        {
            m_onClick();
        }
    }

    OnClick m_onClick;
};

class CInPlaceCheckBox
    : public CInPlaceButton
{
public:
    CInPlaceCheckBox(CInPlaceButton::OnClick onClickFunctor)
        : CInPlaceButton(onClickFunctor, true) {};
};

//////////////////////////////////////////////////////////////////////////
class CInPlaceColorButton
    : public CColorButton
{
    DECLARE_DYNAMIC(CInPlaceColorButton)

public:
    typedef Functor0 OnClick;

    CInPlaceColorButton(OnClick onClickFunctor) { m_onClick = onClickFunctor; }

    // Simuale on click.
    void Click() { OnBnClicked();   }

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

public:
    afx_msg void OnBnClicked()
    {
        if (m_onClick)
        {
            m_onClick();
        }
    }

    OnClick m_onClick;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_INPLACEBUTTON_H
