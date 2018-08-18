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
#include <assert.h>             // assert()
#include "WeightFilterSet.h"    // CWeightFilter
#include "IRCLog.h"             // RCLog()
#include "StringHelpers.h"
#include "StringUtils.h"        // cry_strcpy()


CWeightFilter::CWeightFilter()
{
    cry_strcpy(m_description, "Empty");
}


void CWeightFilter::CreateAverage2x2()
{
    m_FilterKernelBlock.SetSize(2, 2);

    m_FilterKernelBlock.Fill(0.25f);

    assert(ComputeSum() == 1.0f);

    azsnprintf(m_description, sizeof(m_description),
        "%ix%i, %s",
        GetWidth(), GetHeight(), "Average2x2");
}


void CWeightFilter::CreateNearest2x2(const float sharpness)
{
    assert(sharpness >= 0 && sharpness <= 1);

    m_FilterKernelBlock.SetSize(2, 2);

    const float s = 0.25f + 0.75f * sharpness;
    m_FilterKernelBlock.Fill((1 - s) / 3);
    m_FilterKernelBlock.Set(1, 1, s);

    assert(ComputeSum() <= 1.0f);

    azsnprintf(m_description, sizeof(m_description),
        "%ix%i (sharpness:%g), %s",
        GetWidth(), GetHeight(), sharpness, "Nearest2x2");
}


void CWeightFilter::CreateSharpen3x3(const float sharpness)
{
    assert(sharpness > 0 && sharpness <= 1);

    m_FilterKernelBlock.SetSize(3, 3);

    m_FilterKernelBlock.Fill(-0.125f * sharpness);
    m_FilterKernelBlock.Set(1, 1, 0.999999f + sharpness);

    assert(ComputeSum() <= 1.0f);

    azsnprintf(m_description, sizeof(m_description),
        "%ix%i (sharpness:%g), %s",
        GetWidth(), GetHeight(), sharpness, "Sharpen3x3");
}


bool CWeightFilter::CreateFromSatFilterKernel(const CSummedAreaTableFilterKernel& satFilterKernel, const float fRadius, const bool bCenter)
{
    FreeData();

    satFilterKernel.ComputeWeightFilterBlock(m_FilterKernelBlock, fRadius, bCenter);

    azsnprintf(m_description, sizeof(m_description),
        "%ix%i (r:%g, center:%s), %s",
        GetWidth(), GetHeight(), fRadius, (bCenter ? "yes" : "no"), satFilterKernel.GetDescription());

    return true;
}


void CWeightFilter::FreeData()
{
    m_FilterKernelBlock.FreeData();
}


float CWeightFilter::ComputeSum() const
{
    const int n = GetWidth() * GetHeight();
    const float* const p = GetWeights();

    float fWeightSum = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        fWeightSum += p[i];
    }

    return fWeightSum;
}


void CWeightFilter::Print(bool bDetailed) const
{
    RCLog("Filter: %s", m_description);

    if (bDetailed)
    {
        const int filtW = GetWidth();
        const int filtH = GetHeight();

        float totalWeight = 0;
        string s;
        const float* const pWeights = GetWeights();
        for (int y = 0; y < filtH; ++y)
        {
            s = "";
            for (int x = 0; x < filtW; ++x)
            {
                const float weight = pWeights[y * filtW + x];
                totalWeight += weight;
                s += StringHelpers::Format(" %+7f", weight);
            }
            RCLog("%s", s.c_str());
        }
        RCLog("Total weight: %g", totalWeight);
    }
}
