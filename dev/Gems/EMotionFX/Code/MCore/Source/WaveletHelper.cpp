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

// include required headers
#include "StandardHeaders.h"
#include "WaveletHelper.h"


namespace MCore
{
    // pad the wavelet
    void WaveletHelper::Pad(MCore::Array<float>& signal)
    {
        // resize it to the next power of two
        uint32 numElements = signal.GetLength();
        uint32 nextPowerOfTwo = MCore::Math::NextPowerOfTwo(numElements);
        signal.Resize(nextPowerOfTwo);
        for (uint32 i = numElements; i < nextPowerOfTwo; ++i)
        {
            signal[i] = 0;
        }
    }



    // find the min and max value of a given data set
    void WaveletHelper::FindMinMax(const float* data, uint32 numSamples, float& outMin, float& outMax)
    {
        MCORE_ASSERT(numSamples > 0);
        float minValue = FLT_MAX;
        float maxValue = -FLT_MAX;
        for (uint32 i = 0; i < numSamples; ++i)
        {
            if (data[i] < minValue)
            {
                minValue = data[i];
            }

            if (data[i] > maxValue)
            {
                maxValue = data[i];
            }
        }

        outMin = minValue;
        outMax = maxValue;
    }


    // normalize
    void WaveletHelper::Normalize(float* coefficients, uint32 numCoeffs, float& outLength)
    {
        float minValue;
        float maxValue;
        FindMinMax(coefficients, numCoeffs, minValue, maxValue);

        const float length = MCore::Max<float>(Math::Abs(maxValue), Math::Abs(minValue));
        if (length > MCore::Math::epsilon)
        {
            const float oneOverLength = 1.0f / length;
            for (uint32 i = 0; i < numCoeffs; ++i)
            {
                coefficients[i] *= oneOverLength;
            }
        }

        outLength = length;
    }


    // quantize the wavelet coefficients
    void WaveletHelper::Quantize(float* coefficients, uint32 numCoeffs, int16* outBuffer, float& outScale, float quantFactor)
    {
        // normalize the coefficients
        Normalize(coefficients, numCoeffs, outScale);

        // quantize
        const int32 factor = (1 << 15) - 1;
        for (uint32 i = 0; i < numCoeffs; ++i)
        {
            outBuffer[i] = (int16)((coefficients[i] * factor) / (float)quantFactor);
        }
    }


    // dequantitize the coefficients array
    void WaveletHelper::Dequantize(const int16* quantBuffer, uint32 numCoeffs, float* outBuffer, float scale, float quantFactor)
    {
        // unpack all coefficients
        const float factor = 1.0f / ((float)(1 << 15) - 1) * scale;
        for (uint32 i = 0; i < numCoeffs; ++i)
        {
            outBuffer[i] = (quantBuffer[i] * factor) * quantFactor;
        }
    }



    // zero coefficients below a given threshold
    uint32 WaveletHelper::ZeroCoefficients(float* coefficients, uint32 numInputs, float threshold)
    {
        uint32 numZeros = 0;
        for (uint32 i = 0; i < numInputs; ++i)
        {
            if (MCore::Math::Abs(coefficients[i]) < threshold)
            {
                coefficients[i] = 0.0f;
                numZeros++;
            }
        }

        return numZeros;
    }
}   // namespace MCore
