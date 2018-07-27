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

#if defined(TOOLS_SUPPORT_ETC2COMP)

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ImageObject.h>       // ImageToProcess
#include <EtcConfig.h>
#include <Etc.h>
#include <EtcImage.h>
#include <EtcColorFloatRGBA.h>



static const int  MAX_COMP_JOBS = 1024;
static const int MIN_COMP_JOBS = 8;
static const float  ETC_LOW_EFFORT_LEVEL = 25.0f;
static const float  ETC_MED_EFFORT_LEVEL = 40.0f;
static const float  ETC_HIGH_EFFORT_LEVEL = 80.0f;


//Grab the Etc2Comp specific pixel format enum
static Etc::Image::Format FindEtc2PixelFormat(EPixelFormat fmt)
{
    switch (fmt)
    {        
    case ePixelFormat_EAC_RG11:
        return Etc::Image::Format::RG11;
    case ePixelFormat_EAC_R11:
        return Etc::Image::Format::R11;
    case ePixelFormat_ETC2:
        return Etc::Image::Format::RGB8;
    case ePixelFormat_ETC2a:
        return Etc::Image::Format::RGBA8;    
    default:
        return Etc::Image::Format::FORMATS;
    }
}

//Get the errmetric required for the compression
static Etc::ErrorMetric FindErrMetric(Etc::Image::Format fmt)
{
    switch (fmt)
    {
    case Etc::Image::Format::RG11:
        return Etc::ErrorMetric::NORMALXYZ;
    case Etc::Image::Format::R11:
        return Etc::ErrorMetric::NUMERIC;
    case Etc::Image::Format::RGB8:
        return Etc::ErrorMetric::RGBX;
    case  Etc::Image::Format::RGBA8:
        return Etc::ErrorMetric::RGBA;
    default:
        return Etc::ErrorMetric::ERROR_METRICS;
    }

}

//Convert to sRGB format
static Etc::Image::Format FindGammaEtc2PixelFormat(Etc::Image::Format fmt)
{
    switch (fmt)
    {   
    case Etc::Image::Format::RGB8:
        return Etc::Image::Format::SRGB8;
    case Etc::Image::Format::RGBA8:
        return Etc::Image::Format::SRGBA8;
    case Etc::Image::Format::RGB8A1:
        return Etc::Image::Format::SRGB8A1;
    default:
        return Etc::Image::Format::FORMATS;
    }
}


//compress to etc2 formats using etc2comp
ImageToProcess::EResult ImageToProcess::ConvertFormatWithETC2Compressor(const CImageProperties* pProps, EPixelFormat fmtDst, EQuality quality)
{
    AZ_Assert(get(), "Image does not exist");

    if ((get()->GetPixelFormat() == fmtDst))
    {
        return eResult_Success;
    }

    if (FindEtc2PixelFormat(fmtDst) != Etc::Image::Format::FORMATS)
    {
        if (get()->GetPixelFormat() != ePixelFormat_A8R8G8B8)
        {
            ConvertFormat(pProps, ePixelFormat_A8R8G8B8);
        }
                
        const size_t srcPixelSize = 4;
        AZStd::unique_ptr<ImageObject> pRet(get()->AllocateImage(0, fmtDst));

        //Calculate the compression speed
        float qualityEffort = 0.0f;
        switch (quality)
        {
        case eQuality_Preview:
        {
            qualityEffort = ETC_LOW_EFFORT_LEVEL;
            break;
        }
        case eQuality_Fast:
        {
            qualityEffort = ETC_LOW_EFFORT_LEVEL;
            break;
        }
        case eQuality_Normal:
        {
            qualityEffort = ETC_MED_EFFORT_LEVEL;
            break;
        }
        default:
        {
            qualityEffort = ETC_HIGH_EFFORT_LEVEL;
        }
        }
        
        Etc::Image::Format dstEtc2Format = FindEtc2PixelFormat(fmtDst);
        if (get()->GetImageFlags() & CImageExtensionHelper::EIF_SRGBRead)
        {
            dstEtc2Format = FindGammaEtc2PixelFormat(dstEtc2Format);
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
                                       
                        
            Etc::ColorFloatRGBA * pafrgbaPixels = new Etc::ColorFloatRGBA[dwLocalWidth * dwLocalHeight];           
            Etc::ColorFloatRGBA *rgbaPixelPtr = pafrgbaPixels;
            AZ_Assert(pafrgbaPixels, "Couldnt allocated memory for the source texture");
            for (uint32 dwY = 0; dwY < dwLocalHeight; ++dwY)
            {
                for (uint32 dwX = 0; dwX < dwLocalWidth; ++dwX)
                {
                    const Color4<uint8>* const pPixelsSrc = &get()->GetPixel<Color4<uint8> >(dwMip, dwX, dwY);
                    *rgbaPixelPtr = Etc::ColorFloatRGBA::ConvertFromRGBA8(pPixelsSrc->components[2], pPixelsSrc->components[1], pPixelsSrc->components[0], pPixelsSrc->components[3]);
                    rgbaPixelPtr++;
                }
            }
             
            //Call into etc2Comp lib to compress. https://medium.com/@duhroach/building-a-blazing-fast-etc2-compressor-307f3e9aad99
            Etc::ErrorMetric errMetric = FindErrMetric(dstEtc2Format);
            unsigned char *paucEncodingBits;
            unsigned int uiEncodingBitsBytes;
            unsigned int uiExtendedWidth;
            unsigned int uiExtendedHeight;
            int iEncodingTime_ms;

            Etc::Encode(reinterpret_cast<float*>(pafrgbaPixels),
                dwLocalWidth, dwLocalHeight,
                dstEtc2Format,
                errMetric,
                qualityEffort,
                MIN_COMP_JOBS,
                MAX_COMP_JOBS,
                &paucEncodingBits, &uiEncodingBitsBytes,
                &uiExtendedWidth, &uiExtendedHeight,
                &iEncodingTime_ms);
                              
            char* pDstMem;
            uint32 dwDstPitch;
            pRet->GetImagePointer(dwMip, pDstMem, dwDstPitch);
           
            memcpy(pDstMem, paucEncodingBits, uiEncodingBitsBytes);
            delete[] pafrgbaPixels;
            pafrgbaPixels = NULL;
        }
            
        // check if we lost the alpha-channel in the conversion 
        if (CPixelFormats::IsPixelFormatWithoutAlpha(fmtDst))
        {
            // save alpha channel as *attached* image, because it's impossible to store it in the 2-channel normal map image
            if (pProps->m_bPreserveAlpha && get()->HasNonOpaqueAlpha() && !pProps->GetDiscardAlpha())
            {              
                const EPixelFormat destinationAlphaFormat = pProps->GetDestAlphaPixelFormat();
                // we are limiting alpha mip count because A8/R32F may contain more mips than a block compressed format
                ImageToProcess tempImageToProcess(
                    get()->GetPixelFormat() == ePixelFormat_A32B32G32R32F && destinationAlphaFormat != ePixelFormat_A8
                    ? get()->CopyAnyAlphaIntoR32FImage(pRet->GetMipCount())
                    : get()->CopyAnyAlphaIntoA8Image(pRet->GetMipCount())
                    );

                const EPixelFormat currentFormat = tempImageToProcess.get()->GetPixelFormat();
                if (destinationAlphaFormat != currentFormat)
                {
                    tempImageToProcess.ConvertFormat(pProps, destinationAlphaFormat);
                }

                pRet->SetAttachedImage(tempImageToProcess.get());
                tempImageToProcess.forget();                
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
        set(pRet.release());
        return eResult_Success;
    }  
    return eResult_UnsupportedFormat;
}

#endif
