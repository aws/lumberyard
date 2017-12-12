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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_CRYTEKTHEME_H
#define CRYINCLUDE_EDITOR_CONTROLS_CRYTEKTHEME_H
#pragma once


class CCrytekTheme
    : public CXTPNativeXPTheme
{
public:
    CCrytekTheme();
    virtual ~CCrytekTheme();

    virtual void DrawImage(CDC* pDC, CPoint pt, CSize sz, CXTPImageManagerIcon* pImage, BOOL bSelected, BOOL bPressed, BOOL bEnabled = TRUE, BOOL bChecked = FALSE, BOOL bPopuped = FALSE, BOOL bToolBarImage = TRUE);
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_CRYTEKTHEME_H
