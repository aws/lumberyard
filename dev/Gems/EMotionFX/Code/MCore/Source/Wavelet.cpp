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
#include "Wavelet.h"


namespace MCore
{
    // constructor
    Wavelet::Wavelet()
    {
        // init the temp buffer
        mTempBuffer         = nullptr;
        mTempBufferLength   = 0;
    }


    // destructor
    Wavelet::~Wavelet()
    {
        ReleaseTempBuffer();
    }


    // release memory used by the temp buffer
    void Wavelet::ReleaseTempBuffer()
    {
        MCore::Free(mTempBuffer);
    }


    // make sure our temp buffer is big enough
    void Wavelet::AssureTempBufferSize(uint32 length)
    {
        if (mTempBufferLength < length)
        {
            mTempBuffer = (float*)MCore::Realloc(mTempBuffer, length * sizeof(float), MCORE_MEMCATEGORY_WAVELETS);
            mTempBufferLength = length;
        }
    }


    // transform a signal, processing all levels
    void Wavelet::Transform(float* signal, uint32 signalLength)
    {
        // make sure our temp buffer is big enough
        AssureTempBufferSize(signalLength);

        for (uint32 n = signalLength; n >= 4; n >>= 1)
        {
            PartialTransform(signal, n);
        }
    }


    // inverse transform a signal, processing all levels
    void Wavelet::InverseTransform(float* coefficients, uint32 signalLength)
    {
        // make sure our temp buffer is big enough
        AssureTempBufferSize(signalLength);

        for (uint32 n = 4; n <= signalLength; n <<= 1)
        {
            PartialInverseTransform(coefficients, n);
        }
    }



    // transform a signal, processing all levels
    void Wavelet::LossyTransform(float* signal, uint32 signalLength, float lossyFactor)
    {
        // make sure our temp buffer is big enough
        AssureTempBufferSize(signalLength);

        for (uint32 n = signalLength; n >= 4; n >>= 1)
        {
            PartialTransform(signal, n);

            // make the the coefficients smaller, based on their wavelet layer level
            const float mulFactor = 1.0f / ((n >> 1) * lossyFactor);
            for (uint32 i = 0; i < n; ++i)
            {
                signal[i] *= mulFactor;
            }
        }
    }


    // inverse transform a signal, processing all levels
    void Wavelet::LossyInverseTransform(float* coefficients, uint32 signalLength, float lossyFactor)
    {
        // make sure our temp buffer is big enough
        AssureTempBufferSize(signalLength);

        for (uint32 n = 4; n <= signalLength; n <<= 1)
        {
            // restore the values
            const float mulFactor = (n >> 1) * lossyFactor;
            for (uint32 i = 0; i < n; ++i)
            {
                coefficients[i] *= mulFactor;
            }

            PartialInverseTransform(coefficients, n);
        }
    }
}   // namespace MCore
