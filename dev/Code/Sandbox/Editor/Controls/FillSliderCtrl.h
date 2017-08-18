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

// Description : Slider like control, with bar filled from the left.


#ifndef CRYINCLUDE_EDITOR_CONTROLS_FILLSLIDERCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_FILLSLIDERCTRL_H
#pragma once

#include "SliderCtrlEx.h"

// This notification (Sent with WM_COMMAND) sent when slider changes position.
#include "UserMessageDefines.h"

/////////////////////////////////////////////////////////////////////////////
// CFillSliderCtrl window
class SANDBOX_API CFillSliderCtrl
    : public CSliderCtrlEx
{
public:
    enum EFillStyle
    {
        // Fill vertically instead of horizontally.
        eFillStyle_Vertical = 1 << 0,

        // Fill with a gradient instead of a solid.
        eFillStyle_Gradient = 1 << 1,

        // Fill the background instead of the slider.
        eFillStyle_Background = 1 << 2,

        // Fill with color hue gradient instead of a solid.
        eFillStyle_ColorHueGradient = 1 << 3,
    };

private:
    DECLARE_DYNCREATE(CFillSliderCtrl);
    // Construction
public:
    typedef Functor1<CFillSliderCtrl*>  UpdateCallback;

    CFillSliderCtrl();
    ~CFillSliderCtrl();

    void SetFilledLook(bool bFilled);
    void SetFillStyle(uint32 style) { m_fillStyle = style; }
    void SetFillColors(COLORREF start, COLORREF end) { m_fillColorStart = start; m_fillColorEnd = end; }

    // Operations
public:
    //! Set current value.
    virtual void SetValue(float val);

    // Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    void NotifyUpdate(bool tracking);
    void ChangeValue(int sliderPos, bool bTracking);

private:
    void DrawFill(CDC& dc, CRect& rect);

protected:
    bool m_bFilled;
    uint32 m_fillStyle;
    COLORREF m_fillColorStart;
    COLORREF m_fillColorEnd;
    CPoint m_mousePos;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_FILLSLIDERCTRL_H
