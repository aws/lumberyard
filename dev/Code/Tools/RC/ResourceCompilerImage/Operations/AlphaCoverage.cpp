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

#include "IRCLog.h"                         // IRCLog

///////////////////////////////////////////////////////////////////////////////////

void ImageObject::TransferAlphaCoverage(const CImageProperties* pProps, const ImageObject* srcImg, int srcMip, int dstMip)
{
    const float fAlphaRef = 0.5f;  // Seems to give good overall results
    const float fDesiredAlphaCoverage = srcImg->ComputeAlphaCoverage(srcMip, fAlphaRef);

    const float fAlphaOffset = pProps->ComputeMIPAlphaOffset(dstMip);
    const float fAlphaScale = ComputeAlphaCoverageScaleFactor(dstMip, fDesiredAlphaCoverage, fAlphaRef);

    const uint32 pixelCount = GetWidth(dstMip) * GetHeight(dstMip);
    ColorF* const pPixels = GetPixelsPointer<ColorF>(dstMip);
    for (uint32 i = 0; i < pixelCount; ++i)
    {
        pPixels[i].a = Util::getMin(pPixels[i].a * fAlphaScale + fAlphaOffset, 1.0f);
    }
}

float ImageObject::ComputeAlphaCoverageScaleFactor(int iMip, const float fDesiredCoverage, const float fAlphaRef) const
{
    float minAlphaRef = 0.0f;
    float maxAlphaRef = 1.0f;
    float midAlphaRef = 0.5f;

    // Find best alpha test reference value using a binary search
    for (int i = 0; i < 10; i++)
    {
        const float currentCoverage = ComputeAlphaCoverage(iMip, midAlphaRef);

        if (currentCoverage > fDesiredCoverage)
        {
            minAlphaRef = midAlphaRef;
        }
        else if (currentCoverage < fDesiredCoverage)
        {
            maxAlphaRef = midAlphaRef;
        }
        else
        {
            break;
        }

        midAlphaRef = (minAlphaRef + maxAlphaRef) * 0.5f;
    }

    return fAlphaRef / midAlphaRef;
}

float ImageObject::ComputeAlphaCoverage(int iMip, const float fAlphaRef) const
{
    uint32 coverage = 0;

    const uint32 pixelCount = GetWidth(iMip) * GetHeight(iMip);
    const ColorF* const pPixels = GetPixelsPointer<ColorF>(iMip);
    for (uint32 i = 0; i < pixelCount; ++i)
    {
        const float fAlpha = pPixels[i].a;
        coverage += fAlpha > fAlphaRef;
    }

    return (float)coverage / (float)(pixelCount);
}
