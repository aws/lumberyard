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
#include "ColorCheckBox.h"

void CCustomButton::SetIcon(LPCSTR lpszIconResource, int nIconAlign, bool bDrawText)
{
    SetWindowText("");
    HICON hIcon = (HICON)::LoadImage(AfxGetInstanceHandle(), lpszIconResource, IMAGE_ICON, 0, 0, 0);
    CXTPButton::SetIcon(CSize(32, 32), hIcon, NULL, FALSE);
}

void CCustomButton::SetIcon(HICON hIcon, int nIconAlign, bool bDrawText)
{
    SetWindowText("");
    CXTPButton::SetIcon(CSize(32, 32), hIcon, NULL, FALSE);
}

CCustomButton::CCustomButton()
{
    SetTheme(xtpButtonThemeOffice2003);
}