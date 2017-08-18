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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_ANIMATIONTOOLBAR_H
#define CRYINCLUDE_EDITOR_CONTROLS_ANIMATIONTOOLBAR_H
#pragma once

// AnimationToolBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAnimationToolBar window

#include "AnimSequences.h"

class CAnimationToolBar
    : public CToolBar
{
    // Construction
public:
    CAnimationToolBar();

    // Attributes
public:

    // Operations
public:
    CString GetAnimName();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAnimationToolBar)
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CAnimationToolBar();

    CSliderCtrl m_cKeyframes;        // IDR_KEYFRAMES
    CAnimSequences m_cAnimSequences; // IDR_ANIM_SEQUENCES

    // Generated message map functions
protected:
    //{{AFX_MSG(CAnimationToolBar)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_ANIMATIONTOOLBAR_H
