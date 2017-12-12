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
#include "CrytekTheme.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCrytekTheme::CCrytekTheme()
{
}

CCrytekTheme::~CCrytekTheme()
{
}


void CCrytekTheme::DrawImage(CDC* pDC, CPoint pt, CSize sz, CXTPImageManagerIcon* pImage, BOOL bSelected, BOOL bPressed, BOOL bEnabled, BOOL bChecked, BOOL bPopuped, BOOL)
{
    if (!bEnabled)
    {
        pImage->Draw(pDC, pt, pImage->GetIcon(xtpImageDisabled), sz);
    }
    else if (bPopuped || bChecked)
    {
        pImage->Draw(pDC, pt, pImage->GetIcon(bChecked && (bSelected || bPressed) ? xtpImageHot : xtpImageNormal), sz);
    }
    else
    {
        pImage->Draw(pDC, pt, pImage->GetIcon(bSelected || bPressed ? xtpImageHot : xtpImageNormal), sz);
    }
}