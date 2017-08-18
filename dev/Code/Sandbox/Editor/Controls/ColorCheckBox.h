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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_COLORCHECKBOX_H
#define CRYINCLUDE_EDITOR_CONTROLS_COLORCHECKBOX_H
#pragma once

// ColorCheckBox.h : header file
//

#include "ColorCtrl.h"

class SANDBOX_API CCustomButton
    : public CXTPButton
{
public:
    CCustomButton();
    //! Set icon to display on button, (must not be shared).
    void SetIcon(HICON hIcon, int nIconAlign = BS_CENTER, bool bDrawText = false);
    void SetIcon(LPCSTR lpszIconResource, int nIconAlign = BS_CENTER, bool bDrawText = false);
};
typedef CCustomButton CColoredPushButton;

class CColorCheckBox
    : public CCustomButton
{
public:
    CColorCheckBox() { SetTheme(xtpButtonThemeOffice2003); }
    int GetCheck() { return GetChecked(); };
    void SetCheck(int nCheck) { SetChecked(nCheck); };
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // CRYINCLUDE_EDITOR_CONTROLS_COLORCHECKBOX_H
