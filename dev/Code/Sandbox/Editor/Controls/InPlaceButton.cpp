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
#include "InPlaceButton.h"

// CInPlaceButton

IMPLEMENT_DYNAMIC(CInPlaceButton, CXTButton)

BEGIN_MESSAGE_MAP(CInPlaceButton, CXTPButton)
ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
ON_WM_CREATE()
END_MESSAGE_MAP()

CInPlaceButton::CInPlaceButton(OnClick onClickFunctor)
{
    m_onClick = onClickFunctor;
    SetTheme(xtpButtonThemeOffice2003);
}

CInPlaceButton::CInPlaceButton(OnClick onClickFunctor, bool bNoTheme)
{
    m_onClick = onClickFunctor;
}
int CInPlaceButton::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    SetFont(CFont::FromHandle(gSettings.gui.hSystemFont));
    return 0;
}


IMPLEMENT_DYNAMIC(CInPlaceColorButton, CColorButton)
BEGIN_MESSAGE_MAP(CInPlaceColorButton, CColorButton)
ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
ON_WM_CREATE()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
int CInPlaceColorButton::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CColorButton::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }
    SetFont(CFont::FromHandle(gSettings.gui.hSystemFont));
    return 0;
}

//////////////////////////////////////////////////////////////////////////
BOOL CInPlaceColorButton::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}