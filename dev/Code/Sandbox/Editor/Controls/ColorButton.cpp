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
#include "ColorButton.h"


CColorButton::CColorButton()
    : m_color(QtUtil::GetSysColorPort(COLOR_BTNFACE))
    , m_showText(true)
{
}


CColorButton::CColorButton(COLORREF color, bool showText)
    : m_color(color)
    , m_showText(showText)
{
}


CColorButton::~CColorButton()
{
}


BEGIN_MESSAGE_MAP(CColorButton, CButton)
//ON_WM_CREATE()
END_MESSAGE_MAP()
/*
void CColorButton::::PreSubclassWindow()
{
    ChangeStyle();
    BASE_TYPE::PreSubclassWindow();
}

int CColorButton::::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (BASE_TYPE::OnCreate(lpCreateStruct) == -1)
        return -1;
    ChangeStyle();
    return 0;
}
*/

void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    assert(0 != lpDrawItemStruct);

    CDC* pDC(CDC::FromHandle(lpDrawItemStruct->hDC));
    assert(0 != pDC);

    DWORD style = GetStyle();

    UINT uiDrawState(DFCS_ADJUSTRECT);
    UINT state(lpDrawItemStruct->itemState);

    /*
    switch (lpDrawItemStruct->itemAction)
    {
    case ODA_DRAWENTIRE:
        {
            uiDrawState |= DFCS_BUTTONPUSH;
            break;
        }
    case ODA_FOCUS:
        {
            uiDrawState |= DFCS_BUTTONPUSH;
            break;
        }
    case ODA_SELECT:
        {
            uiDrawState |= DFCS_BUTTONPUSH;
            if( 0 != ( state & ODS_SELECTED ) )
            {
                uiDrawState |= DFCS_PUSHED;
            }
            break;
        }
    }

    if( 0 != ( state & ODS_SELECTED ) )
    {
        uiDrawState |= DFCS_PUSHED;
    }
    */

    CRect rc;
    rc.CopyRect(&lpDrawItemStruct->rcItem);


    // draw frame
    if (((state & ODS_DEFAULT) || (state & ODS_FOCUS)))
    {
        // draw def button black rectangle
        pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_WINDOWFRAME), QtUtil::GetSysColorPort(COLOR_WINDOWFRAME));
        rc.DeflateRect(1, 1);
    }

    if (style & BS_FLAT)
    {
        pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_WINDOWFRAME), QtUtil::GetSysColorPort(COLOR_WINDOWFRAME));
        //pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_INACTIVEBORDER), QtUtil::GetSysColorPort(COLOR_INACTIVEBORDER));
        rc.DeflateRect(1, 1);
        pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_WINDOW), QtUtil::GetSysColorPort(COLOR_WINDOW));
        rc.DeflateRect(1, 1);
    }
    else
    {
        if (state & ODS_SELECTED)
        {
            pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_3DSHADOW), QtUtil::GetSysColorPort(COLOR_3DSHADOW));
            rc.DeflateRect(1, 1);
            //pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_3DFACE), QtUtil::GetSysColorPort(COLOR_3DFACE));
            //rc.DeflateRect(1, 1);
        }
        else
        {
            pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_3DHILIGHT), QtUtil::GetSysColorPort(COLOR_3DDKSHADOW));
            rc.DeflateRect(1, 1);
            //pDC->Draw3dRect(rc, QtUtil::GetSysColorPort(COLOR_3DFACE), QtUtil::GetSysColorPort(COLOR_3DSHADOW));
            //rc.DeflateRect(1, 1);
        }
    }
    if ((m_color & 0xff000000) == 0xff000000)
    {
        pDC->SetBkMode(TRANSPARENT);
        pDC->FillSolidRect(&rc, m_color & 0xffffff);
    }
    else
    {
        // this code will draw a color button with transparency if alpha != 255
        COLORREF r = GetRValue(m_color);
        COLORREF g = GetGValue(m_color);
        COLORREF b = GetBValue(m_color);
        COLORREF dr = 200; COLORREF dg = 200; COLORREF db = 200;
        COLORREF lr = 255; COLORREF lg = 255; COLORREF lb = 255;
        int lerp = m_color >> 24;
        int invlerp = 255 - lerp;

        r *= lerp; g *= lerp; b *= lerp;
        dr *= invlerp; dg *= invlerp; db *= invlerp;
        lr *= invlerp; lg *= invlerp; lb *= invlerp;
        dr += r; dg += g; db += b;
        lr += r; lg += g; lb += b;
        dr >>= 8; dg >>= 8; db >>= 8;
        lr >>= 8; lg >>= 8; lb >>= 8;

        COLORREF dgrey = RGB(dr, dg, db);
        COLORREF lgrey = RGB(lr, lg, lb);

        CRect r0, r1, r2, r3;
        r0.bottom = r2.bottom = rc.bottom;
        r1.bottom = r3.bottom = rc.top + (rc.bottom - rc.top)/2;
        r0.top = r2.top = rc.top + (rc.bottom - rc.top) / 2;
        r1.top = r3.top = rc.top;
        r0.left = r1.left = rc.left;
        r2.left = r3.left = rc.left + (rc.right - rc.left) / 2;
        r0.right = r1.right = rc.left + (rc.right - rc.left) / 2;
        r2.right = r3.right = rc.right;
        pDC->SetBkMode(TRANSPARENT);
        pDC->FillSolidRect(&r0, dgrey);
        pDC->FillSolidRect(&r1, m_color & 0xffffff);
        pDC->FillSolidRect(&r2, lgrey);
        pDC->FillSolidRect(&r3, dgrey);

    }
    /*

    pDC->DrawFrameControl( &rc, DFC_BUTTON, uiDrawState );

    rc.DeflateRect( 1, 1 );
    pDC->FillSolidRect( &rc, m_color );

    if( 0 != ( state & ODS_FOCUS ) )
    {
        pDC->DrawFocusRect( &rc );
    }
    */

    if (false != m_showText)
    {
        CString wndText;
        GetWindowText(wndText);

        if (!wndText.IsEmpty())
        {
            //COLORREF txtCol( RGB( ~GetRValue( m_color ), ~GetGValue( m_color ), ~GetBValue( m_color ) ) );
            pDC->SetTextColor(m_textColor);
            pDC->DrawText(wndText, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
}


void
CColorButton::PreSubclassWindow()
{
    CButton::PreSubclassWindow();
    SetButtonStyle(BS_PUSHBUTTON | BS_OWNERDRAW);
}


void CColorButton::SetColor(const COLORREF& col)
{
    m_color = col;
    Invalidate();
}

void CColorButton::SetTextColor(const COLORREF& col)
{
    m_textColor = col;
    Invalidate();
}


COLORREF
CColorButton::GetColor() const
{
    return(m_color);
}


void
CColorButton::SetShowText(bool showText)
{
    m_showText = showText;
    Invalidate();
}


bool
CColorButton::GetShowText() const
{
    return(m_showText);
}
