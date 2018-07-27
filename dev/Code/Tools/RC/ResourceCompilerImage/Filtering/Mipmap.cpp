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
#include <assert.h>                         // assert()

#include "../ImageCompiler.h"               // CImageCompiler
#include "../ImageUserDialog.h"             // CImageUserDialog
#include "../ImageObject.h"                 // ImageToProcess

#include "IRCLog.h"                         // IRCLog
#include "FIR-Windows.h"                    // EWindowFunction

///////////////////////////////////////////////////////////////////////////////////

void CImageCompiler::CreateMipMaps(
    ImageToProcess& image,
    uint32 indwReduceResolution,
    const bool inbRemoveMips,
    const bool inbRenormalize)
{
    const EPixelFormat srcPixelFormat = image.get()->GetPixelFormat();

    if (srcPixelFormat != ePixelFormat_A32B32G32R32F)
    {
        assert(0);
        image.set(0);
        return;
    }

    {
        const uint32 dwMinWidth = CPixelFormats::GetPixelFormatInfo(srcPixelFormat)->minWidth;
        const uint32 dwMinHeight = CPixelFormats::GetPixelFormatInfo(srcPixelFormat)->minHeight;

        if (dwMinWidth != 1 || dwMinHeight != 1)
        {
            assert(0);
            image.set(0);
            return;
        }
    }

    uint32 srcWidth, srcHeight, srcMips;
    image.get()->GetExtent(srcWidth, srcHeight, srcMips);

    if (srcMips != 1)
    {
        assert(0);
        RCLogError("%s called for a mipmapped image. Inform an RC programmer.", __FUNCTION__);
        image.set(0);
        return;
    }

    if (image.get()->GetCubemap() == ImageObject::eCubemap_Yes)
    {
        assert(0);
        RCLogError("%s called for a cubemap. CreateCubemapMipMaps() should be used instead. Inform an RC programmer.", __FUNCTION__);
        image.set(0);
        return;
    }

    //ImageUserDialog is windows based. Need to be made cross platform.
#if defined(AZ_PLATFORM_WINDOWS)
    // skipping unnecessary big MIPs
    if (m_bInternalPreview)
    {
        indwReduceResolution += m_pImageUserDialog->GetPreviewReduceResolution(srcWidth, srcHeight);

        if (m_pImageUserDialog->PreviewGenerationCanceled())
        {
            image.set(0);
            return;
        }
    }
#endif
    
    std::unique_ptr<ImageObject> pRet;
    pRet.reset(image.get()->AllocateImage(&m_Props, inbRemoveMips, indwReduceResolution));

    if (m_bInternalPreview)
    {
        m_Progress.Phase1();
    }

    uint32 dstMips = pRet->GetMipCount();
    for (uint32 dstMip = 0; dstMip < dstMips; ++dstMip)
    {
        QRect  prvRect;
        QRect* srcRect = NULL;
        QRect* dstRect = NULL;

        // Alpha coverage can't be maintained if not all of the image is calculated
        if (m_bInternalPreview && !m_Props.GetMaintainAlphaCoverage())
        {
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE)
            // Preview-rectangle calculation is based on the original size of the image
            prvRect = m_pImageUserDialog->GetPreviewRectangle(dstMip + indwReduceResolution);
#else
            //TODO: ImageUserDialog is windows based. Need to be made cross platform.
            assert(0);
#endif
            dstRect = &prvRect;

            // Initialize out-of-rectangle region with zeros
            uint32 dstWidth, dstHeight;
            char* pDestMem;
            uint32 dwDestPitch;

            pRet->GetImagePointer(dstMip, pDestMem, dwDestPitch);

            dstWidth = pRet->GetWidth(dstMip);
            dstHeight = pRet->GetHeight(dstMip);

            memset(pDestMem, 0, dwDestPitch * dstHeight);
        }

        pRet->FilterImage(&m_Props, image.get(), 0, int(dstMip), srcRect, dstRect);
        if (m_Props.GetMaintainAlphaCoverage())
        {
            pRet->TransferAlphaCoverage(&m_Props, image.get(), 0, int(dstMip));
        }

        if (m_bInternalPreview)
        {
            m_Progress.Phase3(srcHeight, srcHeight, dstMip + indwReduceResolution);
        }
    }

    if (inbRenormalize)
    {
        pRet->NormalizeVectors(0, dstMips);
    }

    image.set(pRet.release());
}

void ImageToProcess::CreateHighPass(uint32 dwMipDown)
{
    const EPixelFormat ePixelFormat = get()->GetPixelFormat();

    if (ePixelFormat != ePixelFormat_A32B32G32R32F)
    {
        assert(0);
        set(0);
        return;
    }

    uint32 dwWidth, dwHeight, dwMips;
    get()->GetExtent(dwWidth, dwHeight, dwMips);

    if (dwMipDown >= dwMips)
    {
        RCLogWarning("CImageCompiler::CreateHighPass can't go down %i MIP levels for high pass as there are not enough MIP levels available, going down by %i instead", dwMipDown, dwMips - 1);
        dwMipDown = dwMips - 1;
    }

    std::unique_ptr<ImageObject> pRet(new ImageObject(dwWidth, dwHeight, dwMips, ePixelFormat, get()->GetCubemap()));
    assert(pRet->GetMipCount() == dwMips);
    pRet->CopyPropertiesFrom(*get());

    uint32 dstMips = pRet->GetMipCount();
    for (uint32 dstMip = 0; dstMip < dwMips; ++dstMip)
    {
        // linear interpolation
        pRet->FilterImage(eWindowFunction_Triangle, 0, 0.0f, 0.0f, get(), int(dwMipDown), int(dstMip), NULL, NULL);

        const uint32 pixelCountIn = get()->GetWidth(dstMip) * get()->GetHeight(dstMip);
        const uint32 pixelCountOut = pRet->GetWidth(dstMip) * pRet->GetHeight(dstMip);
        ColorF* pPixelsIn = get()->GetPixelsPointer<ColorF>(dstMip);
        ColorF* pPixelsOut = pRet->GetPixelsPointer<ColorF>(dstMip);
        for (uint32 i = 0; i < pixelCountIn; ++i)
        {
            pPixelsOut[i] = pPixelsIn[i] - pPixelsOut[i] + ColorF(0.5f, 0.5f, 0.5f, 0.5f);
            pPixelsOut[i].clamp(0.0f, 1.0f);
        }
    }

    for (uint32 dstMip = dwMips; dstMip < dstMips; ++dstMip)
    {
        // mips below the chosen highpass mip are grey
        const uint32 pixelCount = pRet->GetWidth(dstMip) * pRet->GetHeight(dstMip);
        ColorF* pPixels = pRet->GetPixelsPointer<ColorF>(dstMip);
        for (uint32 i = 0; i < pixelCount; ++i)
        {
            pPixels[i] = ColorF(0.5f, 0.5f, 0.5f, 1.0f);
        }
    }

    set(pRet.release());
}
