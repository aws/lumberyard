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
#include "HaarWavelet.h"
#include "FastMath.h"


namespace MCore
{
    // constructor
    HaarWavelet::HaarWavelet()
        : Wavelet()
    {
    }


    // destructor
    HaarWavelet::~HaarWavelet()
    {
    }


    // partial forward transform
    void HaarWavelet::PartialTransform(float* signal, uint32 signalLength)
    {
        // process the signal
        uint32 offset = 0;
        const uint32 halfLength = signalLength >> 1;
        for (uint32 j = 0; j < signalLength; j += 2)
        {
            mTempBuffer[offset]             = (signal[j] + signal[j + 1]) * 0.5f;
            mTempBuffer[offset + halfLength]  = signal[j] - signal[j + 1];
            offset++;
        }

        // copy the temp buffer results into the buffer
        MCore::MemCopy(signal, mTempBuffer, sizeof(float) * signalLength);
    }


    // partial inverse transform
    void HaarWavelet::PartialInverseTransform(float* coefficients, uint32 signalLength)
    {
        // precalc some things
        const uint32 halfLength = signalLength >> 1;
        uint32 offset = 0;

        // process the signal
        for (uint32 i = 0; i < halfLength; ++i)
        {
            mTempBuffer[offset++] = coefficients[i] + coefficients[i + halfLength] * 0.5f;
            mTempBuffer[offset++] = coefficients[i] - coefficients[i + halfLength] * 0.5f;
        }

        // copy the temp buffer results into the buffer
        MCore::MemCopy(coefficients, mTempBuffer, sizeof(float) * signalLength);
    }
}   // namespace MCore
