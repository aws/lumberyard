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

#pragma once

//
#include "StandardHeaders.h"
#include "Array.h"


namespace MCore
{
    /**
     *
     *
     */
    class MCORE_API WaveletHelper
    {
    public:
        static void Pad(MCore::Array<float>& signal);
        static uint32 ZeroCoefficients(float* coefficients, uint32 numInputs, float threshold);
        static void Quantize(float* coefficients, uint32 numCoeffs, int16* outBuffer, float& outScale, float quantFactor);
        static void Dequantize(const int16* quantBuffer, uint32 numCoeffs, float* outBuffer, float scale, float quantFactor);
        static void FindMinMax(const float* data, uint32 numSamples, float& outMin, float& outMax);
        static void Normalize(float* coefficients, uint32 numCoeffs, float& outLength);
    };
} // namespace MCore
