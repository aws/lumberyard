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

//#include "GenerationProgress.h"
#include "ImageCompiler.h"                  // CImageCompiler
#include "ImageUserDialog.h"                // CImageUserDialog

// ------------------------------------------------------------------

//constructor
CGenerationProgress::CGenerationProgress(CImageCompiler& rImageCompiler)
    : m_rImageCompiler(rImageCompiler)
    , m_fProgress(0)
{
}

float CGenerationProgress::Get()
{
    return m_fProgress;
}

void CGenerationProgress::Start()
{
    Set(0.0f);
}

void CGenerationProgress::Finish()
{
    Set(1.0f);
}

void CGenerationProgress::Set(float fProgress)
{
#if defined(AZ_PLATFORM_WINDOWS)
    if (m_fProgress != fProgress && m_rImageCompiler.m_bInternalPreview)
    {
        m_rImageCompiler.m_pImageUserDialog->UpdateProgressBar(fProgress);
    }
#else
    //TODO: Needs cross platform support!!
#endif
    m_fProgress = fProgress;
}

void CGenerationProgress::Increment(float fProgress)
{
    m_fProgress += fProgress;
    
#if defined(AZ_PLATFORM_WINDOWS)
    if (fProgress > 0 && m_rImageCompiler.m_bInternalPreview)
    {
        m_rImageCompiler.m_pImageUserDialog->UpdateProgressBar(m_fProgress);
    }
#else
    //TODO: Needs cross platform support!!
#endif
}

// ------------------------------------------------------------------

//the tuned "magic" numbers for progress increments in the generation phases

//phase 1: 10%
void CGenerationProgress::Phase1()
{
    Increment(0.1f);
}

//phase 2: 25%
void CGenerationProgress::Phase2(const uint32 dwY, const uint32 dwHeight)
{
    if (dwY % 8 == 0)
    {
        Increment(0.249f / (dwHeight / 8.0f));
    }
}

//phase 3: 45%
void CGenerationProgress::Phase3(const uint32 dwY, const uint32 dwHeight, const int iTemp)
{
    if (dwY % 8 == 0)
    {
        assert(iTemp >= 0);
        if (iTemp < 3)
        {
            const float increment = (3 - iTemp) * (3 - iTemp) * (3 - iTemp) / 36.0f;
            Increment((0.449f * increment) / (dwHeight / 8.0f));
        }
    }
}

//phase 4: 20%
void CGenerationProgress::Phase4(const uint32 dwMip, const int iBlockLines)
{
    if (dwMip < 2)
    {
        const float increment = (3 - dwMip) * (3 - dwMip) * (3 - dwMip) / 36.0f;
        Increment((0.199f * increment) / (float)iBlockLines);
    }
}

