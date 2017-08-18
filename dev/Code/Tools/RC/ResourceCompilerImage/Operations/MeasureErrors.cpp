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

#include "../ImageCompiler.h"               // CImageCompiler
#include "../ImageObject.h"                 // ImageToProcess

///////////////////////////////////////////////////////////////////////////////////

static inline float GammaToLinear(float x)
{
    return (x <= 0.04045f) ? x / 12.92f : powf((x + 0.055f) / 1.055f, 2.4f);
}

static inline float LinearToGamma(float x)
{
    return (x <= 0.0031308f) ? x * 12.92f : 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
}

static inline ColorF GammaToLinear(const ColorF& x)
{
    return ColorF(
        GammaToLinear(x.r),
        GammaToLinear(x.g),
        GammaToLinear(x.b),
        x.a);
}

static inline ColorF LinearToGamma(const ColorF& x)
{
    return ColorF(
        LinearToGamma(x.r),
        LinearToGamma(x.g),
        LinearToGamma(x.b),
        x.a);
}

///////////////////////////////////////////////////////////////////////////////////

// converts to DXT1, extracts normals from both source and DXT1 compressed image and gets sum of angle differences divided by number of pixels
float ImageObject::GetDXT1NormalsCompressionError(const CImageProperties* pProps) const
{
    ImageToProcess image1(this->CopyImage());
    image1.get()->CopyPropertiesFrom(*this);
    image1.get()->SetCubemap(ImageObject::eCubemap_No);
    image1.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);

    ImageToProcess image2(this->CopyImage());
    image2.get()->CopyPropertiesFrom(*this);
    image2.get()->SetCubemap(ImageObject::eCubemap_No);
    image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
    image2.ConvertFormat(pProps, ePixelFormat_DXT1, ImageToProcess::eQuality_Preview);
    image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);

    uint32 width, height, mips;
    image1.get()->GetExtent(width, height, mips);
    assert(mips == 1);

    char* pSrcMem;
    uint32 srcPitch;
    image1.get()->GetImagePointer(0, pSrcMem, srcPitch);

    char* pDstMem;
    uint32 dstPitch;
    image2.get()->GetImagePointer(0, pDstMem, dstPitch);

    // extract each pair of normals and add angle between them
    float fSumDeltaSq = 0;
    for (uint32 y = 0; y < height; ++y)
    {
        const ColorF* const pSrc = (const ColorF*)(pSrcMem + y * srcPitch);
        const ColorF* const pDst = (const ColorF*)(pDstMem + y * dstPitch);
        for (uint32 x = 0; x < width; ++x)
        {
            const ColorF& srcCol = pSrc[x];
            const ColorF& dxtCol = pDst[x];

            Vec3 norm1(srcCol.r * 2.f - 1.f, srcCol.g * 2.f - 1.f, 0.f);
            norm1.z = sqrtf(1.f - Util::getClamped(norm1.x * norm1.x + norm1.y * norm1.y, 0.f, 1.f));

            Vec3 norm2(dxtCol.r * 2.f - 1.f, dxtCol.g * 2.f - 1.f, 0.f);
            norm2.z = sqrtf(1.f - Util::getClamped(norm2.x * norm2.x + norm2.y * norm2.y, 0.f, 1.f));

            fSumDeltaSq += acosf(Util::getClamped(norm1.Dot(norm2), 0.f, 1.f));
        }
    }

    return fSumDeltaSq / (width * height);
}

// converts to DXT1, extracts color from both source and DXT1 compressed image and gets sum of square differences divided by number of pixels
void ImageObject::GetDXT1GammaCompressionError(const CImageProperties* pProps, float* pLinearDXT1, float* pSRGBDXT1) const
{
    const bool bGammaSpaceMetric = false;
    const Vec3 weights = pProps->GetColorWeights();

    const ImageObject::EColorNormalization eColorNorm = ImageObject::eColorNormalization_Normalize;
    const ImageObject::EAlphaNormalization eAlphaNorm = ImageObject::eAlphaNormalization_Normalize;

    ImageToProcess image1(this->CopyImage());
    image1.get()->CopyPropertiesFrom(*this);
    image1.get()->SetCubemap(ImageObject::eCubemap_No);
    image1.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
    image1.get()->ExpandImageRange(eColorNorm, eAlphaNorm, 0);
    image1.ConvertModel(pProps, CImageProperties::eColorModel_RGB);

    {
        ImageToProcess image2(this->CopyImage());
        image2.get()->CopyPropertiesFrom(*this);
        image2.get()->SetCubemap(ImageObject::eCubemap_No);
        image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
        image2.ConvertFormat(pProps, ePixelFormat_DXT1, ImageToProcess::eQuality_Preview);
        image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
        image2.get()->ExpandImageRange(eColorNorm, eAlphaNorm, 0);
        image2.ConvertModel(pProps, CImageProperties::eColorModel_RGB);

        uint32 width, height, mips;
        uint32 width2, height2, mips2;
        image1.get()->GetExtent(width, height, mips);
        image2.get()->GetExtent(width2, height2, mips2);
        // only compare valid mips that exist in both images
        mips = Util::getMin(mips, mips2);

        // extract each pair of normals and add angle between them
        float fSumDeltaSqLinear = 0;
        float fSum = 0;


        for (uint32 m = 0; m < mips; ++m)
        {
            char* pSrcMem;
            uint32 srcPitch;
            image1.get()->GetImagePointer(m, pSrcMem, srcPitch);

            char* pDstMem;
            uint32 dstPitch;
            image2.get()->GetImagePointer(m, pDstMem, dstPitch);

            height = image1.get()->GetHeight(m);
            width  = image1.get()->GetWidth (m);

            for (uint32 y = 0; y < height; ++y)
            {
                const ColorF* const pSrc = (const ColorF*)(pSrcMem + y * srcPitch);
                const ColorF* const pDst = (const ColorF*)(pDstMem + y * dstPitch);
                for (uint32 x = 0; x < width; ++x)
                {
                    const ColorF& srcCol = pSrc[x];
                    const ColorF& dxtCol = pDst[x];

                    const ColorF srcColG = (bGammaSpaceMetric ? LinearToGamma(srcCol) : srcCol) * weights;
                    const ColorF dxtColG = (bGammaSpaceMetric ? LinearToGamma(dxtCol) : dxtCol) * weights;

                    fSumDeltaSqLinear += Vec3(srcColG.r, srcColG.g, srcColG.b).GetSquaredDistance(Vec3(dxtColG.r, dxtColG.g, dxtColG.b));
                }
            }

            fSum += width * height;
            if ((width <= 4) || (height <= 4))
            {
                break;
            }
        }

        *pLinearDXT1 = fSumDeltaSqLinear / fSum;
    }

    {
        ImageToProcess image2(this->CopyImage());
        image2.get()->CopyPropertiesFrom(*this);
        image2.get()->SetCubemap(ImageObject::eCubemap_No);
        image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
        image2.LinearRGBAAnyFToGammaRGBAAnyF();
        image2.ConvertFormat(pProps, ePixelFormat_DXT1, ImageToProcess::eQuality_Preview);
        image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
        image2.GammaToLinearRGBA32F(true);
        image2.get()->ExpandImageRange(eColorNorm, eAlphaNorm, 0);
        image2.ConvertModel(pProps, CImageProperties::eColorModel_RGB);

        uint32 width, height, mips;
        uint32 width2, height2, mips2;
        image1.get()->GetExtent(width, height, mips);
        image2.get()->GetExtent(width2, height2, mips2);
        // only compare valid mips that exist in both images
        mips = Util::getMin(mips, mips2);

        // extract each pair of normals and add angle between them
        float fSumDeltaSqDXT1 = 0;
        float fSum = 0;

        for (uint32 m = 0; m < mips; ++m)
        {
            char* pSrcMem;
            uint32 srcPitch;
            image1.get()->GetImagePointer(m, pSrcMem, srcPitch);

            char* pDstMem;
            uint32 dstPitch;
            image2.get()->GetImagePointer(m, pDstMem, dstPitch);

            height = image1.get()->GetHeight(m);
            width  = image1.get()->GetWidth (m);

            for (uint32 y = 0; y < height; ++y)
            {
                const ColorF* const pSrc = (const ColorF*)(pSrcMem + y * srcPitch);
                const ColorF* const pDst = (const ColorF*)(pDstMem + y * dstPitch);
                for (uint32 x = 0; x < width; ++x)
                {
                    const ColorF& srcCol = pSrc[x];
                    const ColorF& dxtCol = pDst[x];

                    const ColorF srcColG = (bGammaSpaceMetric ? LinearToGamma(srcCol) : srcCol) * weights;
                    const ColorF dxtColG = (bGammaSpaceMetric ? LinearToGamma(dxtCol) : dxtCol) * weights;

                    fSumDeltaSqDXT1 += Vec3(srcColG.r, srcColG.g, srcColG.b).GetSquaredDistance(Vec3(dxtColG.r, dxtColG.g, dxtColG.b));
                }
            }

            fSum += width * height;
            if ((width <= 4) || (height <= 4))
            {
                break;
            }
        }

        *pSRGBDXT1 = fSumDeltaSqDXT1 / fSum;
    }
}

// converts to DXT1, extracts color from both source and DXT1 compressed image and gets sum of square differences divided by number of pixels
void ImageObject::GetDXT1ColorspaceCompressionError(const CImageProperties* pProps, float(&pDXT1)[20]) const
{
    const bool bGammaSpaceMetric = false;
    const Vec3 weights = pProps->GetColorWeights();

    ImageToProcess image1(this->CopyImage());
    image1.get()->CopyPropertiesFrom(*this);
    image1.get()->SetCubemap(ImageObject::eCubemap_No);
    image1.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);

    for (int gm = 0; gm <= 1; ++gm)
    {
        for (int nm = 0; nm <= 1; ++nm)
        {
            for (int cs = CImageProperties::eColorModel_RGB; cs <= CImageProperties::eColorModel_IRB; ++cs)
            {
                ImageToProcess image2(this->CopyImage());
                image2.get()->CopyPropertiesFrom(*this);
                image2.get()->SetCubemap(ImageObject::eCubemap_No);
                image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);
                image2.ConvertModel(pProps, (CImageProperties::EColorModel)cs);

                if (nm)
                {
                    image2.get()->NormalizeImageRange(eColorNormalization_Normalize, eAlphaNormalization_PassThrough, false, 0);
                }
                if (gm)
                {
                    image2.LinearRGBAAnyFToGammaRGBAAnyF();
                }

                image2.ConvertFormat(pProps, ePixelFormat_DXT1, ImageToProcess::eQuality_Normal);
                //      image2.ConvertFormat(pProps, ePixelFormat_A8R8G8B8);
                image2.ConvertFormat(pProps, ePixelFormat_A32B32G32R32F);

                if (gm)
                {
                    image2.GammaToLinearRGBA32F(true);
                }
                if (nm)
                {
                    image2.get()->ExpandImageRange(eColorNormalization_Normalize, eAlphaNormalization_PassThrough, 0);
                }

                image2.ConvertModel(pProps, CImageProperties::eColorModel_RGB);

                uint32 width, height, mips;
                image1.get()->GetExtent(width, height, mips);

                // extract each pair of normals and add angle between them
                float fSumDeltaSqDXT1 = 0;
                float fSum = 0;

                for (uint32 m = 0; m < mips; ++m)
                {
                    char* pSrcMem;
                    uint32 srcPitch;
                    image1.get()->GetImagePointer(m, pSrcMem, srcPitch);

                    char* pDstMem;
                    uint32 dstPitch;
                    image2.get()->GetImagePointer(m, pDstMem, dstPitch);

                    height = image1.get()->GetHeight(m);
                    width  = image1.get()->GetWidth (m);

                    for (uint32 y = 0; y < height; ++y)
                    {
                        const ColorF* const pSrc = (const ColorF*)(pSrcMem + y * srcPitch);
                        const ColorF* const pDst = (const ColorF*)(pDstMem + y * dstPitch);
                        for (uint32 x = 0; x < width; ++x)
                        {
                            const ColorF& srcCol = pSrc[x];
                            const ColorF& dxtCol = pDst[x];

                            const ColorF srcColG = (bGammaSpaceMetric ? LinearToGamma(srcCol) : srcCol) * weights;
                            const ColorF dxtColG = (bGammaSpaceMetric ? LinearToGamma(dxtCol) : dxtCol) * weights;

                            fSumDeltaSqDXT1 += Vec3(srcColG.r, srcColG.g, srcColG.b).GetSquaredDistance(Vec3(dxtColG.r, dxtColG.g, dxtColG.b));
                        }
                    }

                    fSum += width * height;
                    if ((width <= 4) || (height <= 4))
                    {
                        break;
                    }
                }

                pDXT1[cs * 4 + nm * 2 + gm * 1] = fSumDeltaSqDXT1 / fSum;
            }
        }
    }
}
