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
#include <assert.h>                         // assert()

#include "../../ImageCompiler.h"            // CImageCompiler
#include "../../ImageObject.h"              // ImageToProcess

#include "IRCLog.h"                         // IRCLog
#include "RGBL.h"                           // RGBL

void ImageObject::GetLuminanceInAlpha()
{
    if (GetPixelFormat() != ePixelFormat_A32B32G32R32F)
    {
        assert(0);
        RCLogError("%s: unsupported source format. Contact an RC programmer.", __FUNCTION__);
        return;
    }

    uint32 dwWidth, dwHeight, dwMips;

    GetExtent(dwWidth, dwHeight, dwMips);
    assert(dwMips == 1);

    char* pSrcMem;
    uint32 dwSrcPitch;
    GetImagePointer(0, pSrcMem, dwSrcPitch);

    for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
    {
        ColorF* const pPix = (ColorF*)&pSrcMem[dwY * dwSrcPitch];

        for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
        {
            pPix[dwX].a = Util::getClamped(RGBL::GetLuminance(pPix[dwX]), 0.0f, 1.0f);
        }
    }
}
