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
#include "YCbCr.h"                          // YCbCr

void ImageToProcess::ConvertBetweenRGB32FAndYCbCr32F(bool bEncode)
{
    if (get()->GetPixelFormat() != ePixelFormat_A32B32G32R32F)
    {
        assert(0);
        set(0);
        return;
    }

    if (bEncode)
    {
        assert((get()->GetImageFlags() & CImageExtensionHelper::EIF_Colormodel) == CImageExtensionHelper::EIF_Colormodel_RGB);

        std::unique_ptr<ImageObject> pRet(get()->AllocateImage(0, ePixelFormat_A32B32G32R32F));

        const uint32 dwMips = pRet->GetMipCount();
        for (uint32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            const ColorF* const pPixelsIn = get()->GetPixelsPointer<ColorF>(dwMip);
            YCbCr* const pPixelsOut = pRet->GetPixelsPointer<YCbCr>(dwMip);

            const uint32 pixelCount = get()->GetPixelCount(dwMip);

            for (uint32 i = 0; i < pixelCount; ++i)
            {
                pPixelsOut[i] = pPixelsIn[i];
            }
        }

        set(pRet.release());

        get()->RemoveImageFlags(CImageExtensionHelper::EIF_Colormodel);
        get()->AddImageFlags(CImageExtensionHelper::EIF_Colormodel_YCC);
    }
    else
    {
        assert((get()->GetImageFlags() & CImageExtensionHelper::EIF_Colormodel) == CImageExtensionHelper::EIF_Colormodel_YCC);

        std::unique_ptr<ImageObject> pRet(get()->AllocateImage(0, ePixelFormat_A32B32G32R32F));

        const uint32 dwMips = pRet->GetMipCount();
        for (uint32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            const YCbCr* const pPixelsIn = get()->GetPixelsPointer<YCbCr>(dwMip);
            ColorF* const pPixelsOut = pRet->GetPixelsPointer<ColorF>(dwMip);

            const uint32 pixelCount = get()->GetPixelCount(dwMip);

            for (uint32 i = 0; i < pixelCount; ++i)
            {
                pPixelsOut[i] = (ColorF)pPixelsIn[i];
            }
        }

        set(pRet.release());

        get()->RemoveImageFlags(CImageExtensionHelper::EIF_Colormodel);
        get()->AddImageFlags(CImageExtensionHelper::EIF_Colormodel_RGB);
    }
}
