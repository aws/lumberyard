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
#include "ThreadUtils.h"                                        // ThreadUtils
#include "StringHelpers.h"                  // StringHelpers

#include "Cubemap.h"
#include <AzCore/std/parallel/mutex.h>

static AZStd::mutex s_atiCubemapLock;

///////////////////////////////////////////////////////////////////////////////////

static void AtiCubeMapGen_MessageOutputFunc(const WCHAR* pTitle, const WCHAR* pMessage)
{
    const string t = StringHelpers::ConvertAsciiUtf16ToAscii(pTitle);
    const string m = StringHelpers::ConvertAsciiUtf16ToAscii(pMessage);
    RCLogWarning("ATI CubeMapGen: %s: %s", t.c_str(), m.c_str());
}

void CImageCompiler::CreateCubemapMipMaps(
    ImageToProcess& image,
    const SCubemapFilterParams& params,
    uint32 indwReduceResolution,
    const bool inbRemoveMips)
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

    if (image.get()->GetCubemap() != ImageObject::eCubemap_Yes)
    {
        assert(0);
        RCLogError("%s called for an image which is not a cubemap. CreateMipMaps() should be used instead. Inform an RC programmer.", __FUNCTION__);
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
    for (uint32 dstMip = 0; dstMip < 1; ++dstMip)
    {
        if (m_bInternalPreview)
        {
            QRect prvRect;
            QRect dstRect;

#if defined(AZ_PLATFORM_WINDOWS)
            // Preview-rectangle calculation is based on the original size of the image
            prvRect = m_pImageUserDialog->GetPreviewRectangle(dstMip + indwReduceResolution);
#else
            //TODO: ImageUserDialog is windows based. Need to be made cross platform.
            assert(0);
#endif
            dstRect = prvRect;

            // Initialize out-of-rectangle region with zeros
            uint32 dstWidth, dstHeight;
            char* pDestMem;
            uint32 dwDestPitch;

            pRet->GetImagePointer(dstMip, pDestMem, dwDestPitch);

            dstWidth = pRet->GetWidth(dstMip);
            dstHeight = pRet->GetHeight(dstMip);

            memset(pDestMem, 0, dwDestPitch * dstHeight);
        }

        // Filter each cubemap's face independently to prevent color bleeding
        // across face-boundaries
        // TODO: we would have to clip the face's regions against the
        // preview-rectangle, currently all of the cubemap is calculated
        for (int iSide = 0; iSide < 6; ++iSide)
        {
            QRect srcRect;
            QRect dstRect;

            srcRect.setLeft((iSide + 0) * (image.get()->GetWidth(0) / 6));
            dstRect.setLeft((iSide + 0) * (pRet->GetWidth(0) / 6));
            srcRect.setRight((iSide + 1) * (image.get()->GetWidth(0) / 6));
            dstRect.setRight((iSide + 1) * (pRet->GetWidth(0) / 6));
            srcRect.setTop(0);
            dstRect.setTop(0);
            srcRect.setBottom(image.get()->GetHeight(0));
            dstRect.setBottom(pRet->GetHeight(0));

            pRet->FilterImage(&m_Props, image.get(), int(0), int(dstMip), &srcRect, &dstRect);
        }

        if (m_bInternalPreview)
        {
            m_Progress.Phase3(srcHeight, srcHeight, dstMip + indwReduceResolution);
        }
    }

    SetErrorMessageCallback(AtiCubeMapGen_MessageOutputFunc);

    // clear cubemap processor for filtering cubemap
    m_AtiCubemanGen.Clear();

    // use background thread if we are using dialogs, else there is no need to create additional threads
    const bool bInteractive = m_Props.GetConfigAsBool("userdialog", false, true);
    m_AtiCubemanGen.m_NumFilterThreads = bInteractive ? 1 : 0;//max(1, min(m_CC.threads, CP_MAX_FILTER_THREADS));

    // input and output cubemap set to have save dimensions,
    m_AtiCubemanGen.Init(pRet->GetWidth(0) / 6, pRet->GetHeight(0), dstMips, 4);

    // Load the 6 faces of the input cubemap and copy them into the cubemap processor
    char* pMem;
    uint32 nPitch;

    pRet->GetImagePointer(0, pMem, nPitch);

    const uint32 nSidePitch = nPitch / 6;
    assert(nPitch % 6 == 0);
    for (int iSide = 0; iSide < 6; ++iSide)
    {
        m_AtiCubemanGen.SetInputFaceData(
            iSide,                      // FaceIdx,
            CP_VAL_FLOAT32,             // SrcType,
            4,                          // SrcNumChannels,
            nPitch,                     // SrcPitch,
            pMem + nSidePitch * iSide,  // SrcDataPtr,
            1000000.0f,                 // MaxClamp,
            1.0f,                       // Degamma,
            1.0f);                      // Scale
    }

    if (m_AtiCubemanGen.m_NumFilterThreads > 0)
    {
        s_atiCubemapLock.lock();
    }

    //Filter cubemap
    m_AtiCubemanGen.InitiateFiltering(
        params.BaseFilterAngle,         //BaseFilterAngle,
        params.InitialMipAngle,         //InitialMipAngle,
        params.MipAnglePerLevelScale,   //MipAnglePerLevelScale,
        params.FilterType,              //FilterType,
        params.FixupType,               //FixupType,
        params.FixupWidth,              //FixupWidth,
        params.bUseSolidAngle,          //bUseSolidAngle,
        params.BRDFGlossScale,          //GlossScale,
        params.BRDFGlossBias,           //GlossBias
        params.SampleCountGGX);         //SampleCountGGX

    if (m_AtiCubemanGen.m_NumFilterThreads > 0)
    {
        s_atiCubemapLock.unlock();

        //Report status of filtering , and loop until filtering is complete
        while (m_AtiCubemanGen.GetStatus() == CP_STATUS_PROCESSING)
        {
#if defined(AZ_PLATFORM_WINDOWS)
            // allow user to abort the operation
            if (bInteractive && (GetKeyState(VK_ESCAPE) & 0x80))
            {
                m_AtiCubemanGen.TerminateActiveThreads();
            }
#else
            //TODO: Need cross platform support.
#endif
            Sleep(100);
        }
    }

    // Download data into it
    for (int iSide = 0; iSide < 6; ++iSide)
    {
        assert(m_AtiCubemanGen.m_NumMipLevels == dstMips);
        for (int dstMip = 0; dstMip < dstMips; ++dstMip)
        {
            pRet->GetImagePointer(dstMip, pMem, nPitch);
            const uint32 nSidePitch = nPitch / 6;

            char* const pDestMem = pMem + iSide * nSidePitch;
            m_AtiCubemanGen.GetOutputFaceData(iSide, dstMip, CP_VAL_FLOAT32, 4, nPitch, pDestMem, 1.0f, 1.0f);
        }
    }

    image.set(pRet.release());
}
