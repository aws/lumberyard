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
#include "stdafx.h"

#if defined(TOOLS_SUPPORT_POWERVR)

#include <assert.h>               // assert()

#include "../ImageObject.h"       // ImageToProcess

#include "IRCLog.h"               // RCLogError
#include "ThreadUtils.h"          // ThreadUtils
#include "MathHelpers.h"          // MathHelpers
#include "SuffixUtil.h"                     // SuffixUtil

//////////////////////////////////////////////////////////////////////////
// PowerVR PVRTexLib

//  this is required by the latest PTRTexTool
#if defined(AZ_PLATFORM_WINDOWS)
#define _WINDLL_IMPORT
#endif

#include <PVRTexture.h>
#include <PVRTextureUtilities.h>

//////////////////////////////////////////////////////////////////////////

static EPVRTPixelFormat FindPvrPixelFormat(EPixelFormat fmt)
{
    switch (fmt)
    {

    case ePixelFormat_ASTC_4x4:
        return ePVRTPF_ASTC_4x4;
    case ePixelFormat_ASTC_5x4:
        return ePVRTPF_ASTC_5x4;
    case ePixelFormat_ASTC_5x5:
        return ePVRTPF_ASTC_5x5;
    case ePixelFormat_ASTC_6x5:
        return ePVRTPF_ASTC_6x5;
    case ePixelFormat_ASTC_6x6:
        return ePVRTPF_ASTC_6x6;
    case ePixelFormat_ASTC_8x5:
        return ePVRTPF_ASTC_8x5;
    case ePixelFormat_ASTC_8x6:
        return ePVRTPF_ASTC_8x6;
    case ePixelFormat_ASTC_8x8:
        return ePVRTPF_ASTC_8x8;
    case ePixelFormat_ASTC_10x5:
        return ePVRTPF_ASTC_10x5;
    case ePixelFormat_ASTC_10x6:
        return ePVRTPF_ASTC_10x6;
    case ePixelFormat_ASTC_10x8:
        return ePVRTPF_ASTC_10x8;
    case ePixelFormat_ASTC_10x10:
        return ePVRTPF_ASTC_10x10;
    case ePixelFormat_ASTC_12x10:
        return ePVRTPF_ASTC_12x10;
    case ePixelFormat_ASTC_12x12:
        return ePVRTPF_ASTC_12x12;
    case ePixelFormat_PVRTC2:
        return ePVRTPF_PVRTCI_2bpp_RGBA; //IOS drivers do not support pvrtc version 2
    case ePixelFormat_PVRTC4:
        return ePVRTPF_PVRTCI_4bpp_RGBA; //IOS drivers do not support pvrtc version 2
    case ePixelFormat_EAC_R11:
        return ePVRTPF_EAC_R11;
    case ePixelFormat_EAC_RG11:
        return ePVRTPF_EAC_RG11;
    case ePixelFormat_ETC2:
        return ePVRTPF_ETC2_RGB;
    case ePixelFormat_ETC2a:
        return ePVRTPF_ETC2_RGBA;
    default:
        return ePVRTPF_NumCompressedPFs;
    }
}

ImageToProcess::EResult ImageToProcess::CompressPVRTCTexture(const CImageProperties* pProps, EPixelFormat fmtDst, EQuality quality, ImageObject* pRet)
{
    const pvrtexture::PixelType srcPixelType('b', 'g', 'r', 'a', 8, 8, 8, 8);
    const size_t srcPixelSize = 4;
    
    pvrtexture::ECompressorQuality cquality = pvrtexture::eETCFast;
    
    if (fmtDst == ePixelFormat_ETC2 || fmtDst == ePixelFormat_ETC2a || fmtDst == ePixelFormat_EAC_R11 || fmtDst == ePixelFormat_EAC_RG11)
    {
        const string rgbweights = pProps->GetConfigAsString("rgbweights", "uniform", "uniform");
        
        if ((quality <= eQuality_Normal) && StringHelpers::EqualsIgnoreCase(rgbweights, "uniform"))
        {
            cquality = pvrtexture::eETCFast;
        }
        else if (quality <= eQuality_Normal)
        {
            cquality = pvrtexture::eETCFastPerceptual;
        }
        else if (StringHelpers::EqualsIgnoreCase(rgbweights, "uniform"))
        {
            cquality = pvrtexture::eETCSlow;
        }
        else
        {
            cquality = pvrtexture::eETCSlowPerceptual;
        }
    }
    else if (fmtDst >= ePixelFormat_ASTC_4x4 && fmtDst <= ePixelFormat_ASTC_12x12)
    {
        if (quality == eQuality_Preview)
        {
            cquality = pvrtexture::eASTCVeryFast;
        }
        else if (quality == eQuality_Fast)
        {
            cquality = pvrtexture::eASTCFast;
        }
        else if (quality == eQuality_Normal)
        {
            cquality = pvrtexture::eASTCMedium;
        }
        else
        {
            cquality = pvrtexture::eASTCThorough;
        }
    }
    else
    {
        if (quality == eQuality_Preview)
        {
            cquality = pvrtexture::ePVRTCFastest;
        }
        else if (quality == eQuality_Fast)
        {
            cquality = pvrtexture::ePVRTCFast;
        }
        else if (quality == eQuality_Normal)
        {
            cquality = pvrtexture::ePVRTCNormal;
        }
        else
        {
            cquality = pvrtexture::ePVRTCHigh;
        }
    }
    
    EPVRTColourSpace cspace = ePVRTCSpacelRGB;
    if (get()->GetImageFlags() & CImageExtensionHelper::EIF_SRGBRead)
    {
        cspace = ePVRTCSpacesRGB;
    }
    
    const uint32 dwDstMips = pRet->GetMipCount();
    for (uint32 dwMip = 0; dwMip < dwDstMips; ++dwMip)
    {
        const uint32 dwLocalWidth = get()->GetWidth(dwMip);
        const uint32 dwLocalHeight = get()->GetHeight(dwMip);
        
        // Prepare source data
        
        char* pSrcMem;
        uint32 dwSrcPitch;
        get()->GetImagePointer(dwMip, pSrcMem, dwSrcPitch);
        if (dwSrcPitch != dwLocalWidth * srcPixelSize)
        {
            RCLogError("%s : Unexpected problem. Inform an RC programmer.", __FUNCTION__);
            set(0);
            return eResult_Failed;
        }
        
        const pvrtexture::CPVRTextureHeader srcHeader(
                                                      srcPixelType.PixelTypeID,          // uint64            u64PixelFormat,
                                                      dwLocalHeight,                     // uint32            u32Height=1,
                                                      dwLocalWidth,                      // uint32            u32Width=1,
                                                      1,                                 // uint32            u32Depth=1,
                                                      1,                                 // uint32            u32NumMipMaps=1,
                                                      1,                                 // uint32            u32NumArrayMembers=1,
                                                      1,                                 // uint32            u32NumFaces=1,
                                                      cspace,                            // EPVRTColourSpace  eColourSpace=ePVRTCSpacelRGB,
                                                      ePVRTVarTypeUnsignedByteNorm,      // EPVRTVariableType eChannelType=ePVRTVarTypeUnsignedByteNorm,
                                                      false);                            // bool              bPreMultiplied=false);
        
        pvrtexture::CPVRTexture cTexture(srcHeader, pSrcMem);
        
        // Compress
        {
#if defined(AZ_PLATFORM_WINDOWS)
            // Sokov: PVRTexLib's compressor has internal bugs (at least in PowerVR SDK 2.07.27.0471),
            // so we must disable _EM_OVERFLOW floating point exception before calling it
            MathHelpers::AutoFloatingPointExceptions autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW));
#endif
            
            bool bOk = false;
            try
            {
                bOk = pvrtexture::Transcode(
                                            cTexture,
                                            pvrtexture::PixelType(FindPvrPixelFormat(fmtDst)),
                                            ePVRTVarTypeUnsignedByteNorm,
                                            cspace,
                                            cquality);
            }
            catch (...)
            {
                RCLogError("%s : Unknown exception in PVRTexLib. Inform an RC programmer.", __FUNCTION__);
                set(0);
                return eResult_Failed;
            }
            
            if (!bOk)
            {
                RCLogError("%s : Failed to compress an image by using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
                set(0);
                return eResult_Failed;
            }
        }
        
        // Getting compressed data
        
        const void* const pCompressedData = cTexture.getDataPtr();
        if (!pCompressedData)
        {
            RCLogError("%s : Failed to obtain compressed image data by using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
            set(0);
            return eResult_Failed;
        }
        
        const uint32 compressedDataSize = cTexture.getDataSize();
        if (pRet->GetMipDataSize(dwMip) != compressedDataSize)
        {
            RCLogError("%s : Compressed image data size mismatch while using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
            set(0);
            return eResult_Failed;
        }
        
        char* pDstMem;
        uint32 dwDstPitch;
        pRet->GetImagePointer(dwMip, pDstMem, dwDstPitch);
        
        memcpy(pDstMem, pCompressedData, compressedDataSize);
    }
    return eResult_Success;
}

ImageToProcess::EResult ImageToProcess::ConvertFormatWithPVRTCCompressor(const CImageProperties* pProps, EPixelFormat fmtDst, EQuality quality)
{
    assert(get());

    if ((get()->GetPixelFormat() == fmtDst))
    {
        return eResult_Success;
    }    

    if (FindPvrPixelFormat(fmtDst) != ePVRTPF_NumCompressedPFs)
    {
        if (get()->GetPixelFormat() != ePixelFormat_A8R8G8B8)
        {
            ConvertFormat(pProps, ePixelFormat_A8R8G8B8);
        }
        
        bool isNonOpaqueAlpha = get()->HasNonOpaqueAlpha();
        bool isNormalGlossTexture = SuffixUtil::HasSuffix(pProps->GetSourceFileName(), '_', "ddna");
        const EPixelFormat destinationAlphaFormat = pProps->GetDestAlphaPixelFormat();       
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        std::unique_ptr<ImageObject> pRet(get()->AllocateImage(0, fmtDst));
        AZ_POP_DISABLE_WARNING
        //Create a new texture and copy the alpha channel to it. This will be used to create the smoothness texture for m_pAttachedImage.
        ImageToProcess tempAlphaImageToProcess(
            get()->GetPixelFormat() == ePixelFormat_A32B32G32R32F && destinationAlphaFormat != ePixelFormat_A8
            ? get()->CopyAnyAlphaIntoR32FImage(pRet->GetMipCount())
            : get()->CopyAnyAlphaIntoA8Image(pRet->GetMipCount())
        );


        //Clear out the alpha as it can corrupt the normal map compression. PVRTC compression ignores the alpha if it is masked out so all the available data will be used for RGB channels. 
        if (isNormalGlossTexture)
        {
            get()->SetConstantAlpha(255);
        }        
        
        //Compress to the appropriate pvrtc compression
        EResult compressionResult = CompressPVRTCTexture(pProps, fmtDst, quality, pRet.get());
        assert(compressionResult == eResult_Success);

        // Check if we lost the alpha-channel in the conversion or if we want to force alpha channel for normal-gloss maps   
        // isNormalGlossTexture allows us to use pvrtc/astc compressions(on ios/android) for normal-gloss maps even though they support an alpha channel.
        if (CPixelFormats::IsPixelFormatWithoutAlpha(fmtDst) || isNormalGlossTexture)
        {
            // save alpha channel as *attached* image, because it's impossible to store it in the 2-channel normal map image
            if (pProps->m_bPreserveAlpha && isNonOpaqueAlpha && !pProps->GetDiscardAlpha())
            {                    
                const EPixelFormat currentFormat = tempAlphaImageToProcess.get()->GetPixelFormat();                
                if (destinationAlphaFormat != currentFormat)
                {
                    //For pvrtc compression format do it right away instead of calling ConvertFormat as we do not know when to stop the recursion.
                    if (FindPvrPixelFormat(destinationAlphaFormat) != ePVRTPF_NumCompressedPFs)
                    {
                        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
                        std::unique_ptr<ImageObject> smoothnesAttachedImage(tempAlphaImageToProcess.get()->AllocateImage(0, destinationAlphaFormat));
                        AZ_POP_DISABLE_WARNING
                        //Convert to ePixelFormat_A8R8G8B8 before being compressed
                        tempAlphaImageToProcess.ConvertFromA8ToARGB8();
                        tempAlphaImageToProcess.get()->SetConstantAlpha(255); //Mask out the alpha for better compression
                        //Create a new texture which will hold the smoothness data and then compress it to the appropriate pvrtc compression                       
                        compressionResult = tempAlphaImageToProcess.CompressPVRTCTexture(pProps, destinationAlphaFormat, quality, smoothnesAttachedImage.get());
                        assert(compressionResult == eResult_Success);

                        pRet->SetAttachedImage(smoothnesAttachedImage.release());                        
                    }
                    else
                    {
                        tempAlphaImageToProcess.ConvertFormat(pProps, destinationAlphaFormat);
                    }                    
                }
                else
                {
                    pRet->SetAttachedImage(tempAlphaImageToProcess.get());
                }                
            }
        }
        else
        {
            // restore alpha channel from attached image
            ImageObject* pAttached = get()->GetAttachedImage();
            if (pAttached)
            {
                const EPixelFormat currentFormat = pAttached->GetPixelFormat();
                const EPixelFormat destinationFormat = (fmtDst == ePixelFormat_A32B32G32R32F ? ePixelFormat_R32F : ePixelFormat_A8);
                if (destinationFormat != currentFormat)
                {
                    ImageToProcess tempImageToProcess(pAttached->CopyImage());

                    tempImageToProcess.ConvertFormat(pProps, destinationFormat);

                    pAttached = tempImageToProcess.get();
                    tempImageToProcess.forget();
                }

                if (pAttached->GetPixelFormat() == ePixelFormat_R32F)
                {
                    pRet->TakeoverAnyAlphaFromR32FImage(pAttached);
                }
                else if (pAttached->GetPixelFormat() == ePixelFormat_A8)
                {
                    pRet->TakeoverAnyAlphaFromA8Image(pAttached);
                }

                if (pAttached != get()->GetAttachedImage())
                {
                    delete  pAttached;
                }
            }
        }
        tempAlphaImageToProcess.forget();
        set(pRet.release());
        return eResult_Success;
    }

    if (FindPvrPixelFormat(get()->GetPixelFormat()) != ePVRTPF_NumCompressedPFs)
    {
        //Decompress the pvrtc texture so that it can be viewed through imagedialog
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        std::unique_ptr<ImageObject> finalDecompressedImg(get()->AllocateImage(0, ePixelFormat_A8R8G8B8));
        AZ_POP_DISABLE_WARNING
        EResult decompressionResult = DecompressPVRTCTexture(pProps, fmtDst, finalDecompressedImg.get());
        assert(decompressionResult == eResult_Success);
        
        ImageObject* smoothnesAttachedImage = get()->GetAttachedImage();
       
        //Since we blanked out the alpha during the compression phase we will need to recreate the alpha which holds smoothness data. 
        //Check if an attached image exists as it represents the smoothness data. This will then need to be decompressed and copied into the alpha channel of finalDecompressedImg
        if (smoothnesAttachedImage )
        {
            AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
            std::unique_ptr<ImageObject> decompressedSmoothnessImgObj(smoothnesAttachedImage->AllocateImage(0, ePixelFormat_A8R8G8B8));            
            AZ_POP_DISABLE_WARNING
            bool isCompressed = smoothnesAttachedImage->GetPixelFormat() != ePixelFormat_A8;
            if (isCompressed)
            {                
                ImageToProcess tempSmoothnessImageToProcess(smoothnesAttachedImage->CopyImage());                
                decompressionResult = tempSmoothnessImageToProcess.DecompressPVRTCTexture(pProps, fmtDst, decompressedSmoothnessImgObj.get());
                assert(decompressionResult == eResult_Success);
            }

            //Copy the decompressed data into the alpha channel of finalDecompressedImg
            ImageToProcess tempDecompressedSmoothnessImageToProcess(decompressedSmoothnessImgObj.release());
            tempDecompressedSmoothnessImageToProcess.get()->Swizzle("rgbr"); //Copy red channel into alpha
            tempDecompressedSmoothnessImageToProcess.ConvertFromAnyRGB8ToA8();
            finalDecompressedImg->TakeoverAnyAlphaFromA8Image(isCompressed ? tempDecompressedSmoothnessImageToProcess.get() : smoothnesAttachedImage);

        }
        
        finalDecompressedImg.get()->Swizzle("bgra");
        set(finalDecompressedImg.release());
        if (fmtDst != ePixelFormat_A8R8G8B8)
        {
            ConvertFormat(pProps, fmtDst);
        }
        return eResult_Success;;
    }

    return eResult_UnsupportedFormat;
}

ImageToProcess::EResult ImageToProcess::DecompressPVRTCTexture(const CImageProperties* pProps, EPixelFormat fmtDst, ImageObject* pRet)
{
    const size_t decompressedPixelSize = 4;    
    EPVRTColourSpace colorSpace = ePVRTCSpacelRGB;

    if (get()->GetImageFlags() & CImageExtensionHelper::EIF_SRGBRead)
    {
        colorSpace = ePVRTCSpacesRGB;
    }

    const uint32 dwDstMips = pRet->GetMipCount();
    const uint32 dwSrcMips = get()->GetMipCount();
    const uint32 minAllowedMips = Util::getMin(dwDstMips, dwSrcMips);
    for (uint32 dwMip = 0; dwMip < minAllowedMips; ++dwMip)
    {
        const uint32 dwLocalWidth = get()->GetWidth(dwMip);
        const uint32 dwLocalHeight = get()->GetHeight(dwMip);

        // Preparing source compressed data
        const pvrtexture::CPVRTextureHeader compressedHeader(
            FindPvrPixelFormat(get()->GetPixelFormat()),    // uint64         u64PixelFormat,
            dwLocalHeight,                                  // uint32            u32Height=1,
            dwLocalWidth,                                   // uint32            u32Width=1,
            1,                                              // uint32            u32Depth=1,
            1,                                              // uint32            u32NumMipMaps=1,
            1,                                              // uint32            u32NumArrayMembers=1,
            1,                                              // uint32            u32NumFaces=1,
            colorSpace,                                     // EPVRTColourSpace  eColourSpace=ePVRTCSpacelRGB,
            ePVRTVarTypeUnsignedByteNorm,                   // EPVRTVariableType eChannelType=ePVRTVarTypeUnsignedByteNorm,
            false);                                         // bool              bPreMultiplied=false);

        const uint32 compressedDataSize = compressedHeader.getDataSize();
        if (get()->GetMipDataSize(dwMip) != compressedDataSize)
        {
            RCLogError("%s : Compressed image data size mismatch. Inform an RC programmer.", __FUNCTION__);
            set(0);
            return eResult_Failed;
        }

        char* pSrcMem;
        uint32 dwSrcPitch;
        get()->GetImagePointer(dwMip, pSrcMem, dwSrcPitch);

        pvrtexture::CPVRTexture cTexture(compressedHeader, pSrcMem);

        // Decompress
        {
#if defined(AZ_PLATFORM_WINDOWS)
            // Sokov: PVRTexLib's compressor has internal bugs (at least in PowerVR SDK 2.07.27.0471),
            // so we must disable _EM_OVERFLOW floating point exception before calling it
            MathHelpers::AutoFloatingPointExceptions autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW));
#endif

            bool bOk = false;
            try
            {
                bOk = pvrtexture::Transcode(
                    cTexture,
                    pvrtexture::PVRStandard8PixelType,
                    ePVRTVarTypeUnsignedByteNorm,
                    colorSpace,
                    pvrtexture::ePVRTCHigh);
            }
            catch (...)
            {
                RCLogError("%s : Unknown exception in PVRTexLib. Inform an RC programmer.", __FUNCTION__);
                set(0);
                return eResult_Failed;
            }

            if (!bOk)
            {
                RCLogError("%s : Failed to decompress an image by using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
                set(0);
                return eResult_Failed;
            }
        }

        // Getting decompressed data
        const void* const pDecompressedData = cTexture.getDataPtr();
        if (!pDecompressedData)
        {
            RCLogError("%s : Failed to obtain decompressed image data by using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
            set(0);
            return eResult_Failed;
        }

        const uint32 decompressedDataSize = cTexture.getDataSize();
        if (pRet->GetMipDataSize(dwMip) != decompressedDataSize)
        {
            RCLogError("%s : Decompressed image data size mismatch while using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
            set(0);
            return eResult_Failed;
        }

        char* pDstMem;
        uint32 dwDstPitch;
        pRet->GetImagePointer(dwMip, pDstMem, dwDstPitch);

        memcpy(pDstMem, pDecompressedData, decompressedDataSize);
    }    
    return eResult_Success;
}


bool ConvertToEtcPvrtcFormatARGB(const ImageObject& src, ImageObject& dst)
{
    assert(src.GetPixelFormat() == ePixelFormat_A8R8G8B8);

    auto fmtDst = dst.GetPixelFormat();

    if (FindPvrPixelFormat(fmtDst) != ePVRTPF_NumCompressedPFs)
    {
        const pvrtexture::PixelType srcPixelType('b', 'g', 'r', 'a', 8, 8, 8, 8);

        const size_t srcPixelSize = 4;

        // ETC2 compressor is extremely slow, so let's use "Fast" quality setting for it.
        // Note that visual quality in this case is not as good as it could be.
        const pvrtexture::ECompressorQuality quality =
            (fmtDst == ePixelFormat_ETC2 || fmtDst == ePixelFormat_ETC2a || fmtDst == ePixelFormat_EAC_R11 || fmtDst == ePixelFormat_EAC_RG11)
            ? pvrtexture::eETCFast
            : pvrtexture::ePVRTCNormal;

        //this function only for simple conversions, mips not supported.
        assert(src.GetMipCount() == 1 && dst.GetMipCount() == 1);

        const uint32 width = src.GetWidth(0);
        const uint32 height = src.GetHeight(0);
        assert(width == dst.GetWidth(0) && height == dst.GetHeight(0));

        // Prepare source data
        char* srcMem;
        uint32 srcPitch;
        src.GetImagePointer(0, srcMem, srcPitch);

        //This function should only be used for a tightly packed src ARGB image.
        assert(srcPitch == width * srcPixelSize);

        const pvrtexture::CPVRTextureHeader srcHeader(
            srcPixelType.PixelTypeID,          // uint64            u64PixelFormat,
            height,                     // uint32           u32Height=1,
            width,                      // uint32           u32Width=1,
            1,                                 // uint32            u32Depth=1,
            1,                                 // uint32            u32NumMipMaps=1,
            1,                                 // uint32            u32NumArrayMembers=1,
            1,                                 // uint32            u32NumFaces=1,
            ePVRTCSpacelRGB,                   // EPVRTColourSpace  eColourSpace=ePVRTCSpacelRGB,
            ePVRTVarTypeUnsignedByteNorm,      // EPVRTVariableType eChannelType=ePVRTVarTypeUnsignedByteNorm,
            false);                            // bool              bPreMultiplied=false);

        pvrtexture::CPVRTexture cTexture(srcHeader, srcMem);

        // Compress
        {
#if defined(AZ_PLATFORM_WINDOWS)
            // Sokov: PVRTexLib's compressor has internal bugs (at least in PowerVR SDK 2.07.27.0471),
            // so we must disable _EM_OVERFLOW floating point exception before calling it
            MathHelpers::AutoFloatingPointExceptions autoFpe(~(_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW));
#endif
            
            bool bOk = false;
            try
            {
                bOk = pvrtexture::Transcode(
                        cTexture,
                        pvrtexture::PixelType(FindPvrPixelFormat(fmtDst)),
                        ePVRTVarTypeUnsignedByteNorm,
                        ePVRTCSpacelRGB,
                        quality);
            }
            catch (...)
            {
                RCLogError("%s : Unknown exception in PVRTexLib. Inform an RC programmer.", __FUNCTION__);
                return false;
            }

            if (!bOk)
            {
                RCLogError("%s : Failed to compress an image by using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
                return false;
            }
        }

        // Getting compressed data

        const void* const pCompressedData = cTexture.getDataPtr();
        if (!pCompressedData)
        {
            RCLogError("%s : Failed to obtain compressed image data by using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
            return false;
        }

        const uint32 compressedDataSize = cTexture.getDataSize();
        if (dst.GetMipDataSize(0) != compressedDataSize)
        {
            RCLogError("%s : Compressed image data size mismatch while using PVRTexLib. Inform an RC programmer.", __FUNCTION__);
            return false;
        }

        char* dstMem;
        uint32 dstPitch;
        dst.GetImagePointer(0, dstMem, dstPitch);

        memcpy(dstMem, pCompressedData, compressedDataSize);

        return true;
    }

    return false;
}

#endif
