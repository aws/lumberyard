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
#include "D4Wavelet.h"
#include "FastMath.h"


namespace MCore
{
    // constructor
    D4Wavelet::D4Wavelet()
        : Wavelet()
    {
        const float sqrt3 = MCore::Math::Sqrt(3.0f);
        const float denom = MCore::Math::Sqrt(2.0f) * 4.0f;

        // forward transform scaling (smoothing) coefficients
        mH[0] = (1 + sqrt3) / denom;
        mH[1] = (3 + sqrt3) / denom;
        mH[2] = (3 - sqrt3) / denom;
        mH[3] = (1 - sqrt3) / denom;

        // forward transform wavelet coefficients
        mG[0] =  mH[3];
        mG[1] = -mH[2];
        mG[2] =  mH[1];
        mG[3] = -mH[0];

        // inverse
        mInvH[0] = mH[2];
        mInvH[1] = mG[2];
        mInvH[2] = mH[0];
        mInvH[3] = mG[0];

        mInvG[0] = mH[3];
        mInvG[1] = mG[3];
        mInvG[2] = mH[1];
        mInvG[3] = mG[1];
    }


    // destructor
    D4Wavelet::~D4Wavelet()
    {
    }


    // partial forward transform
    void D4Wavelet::PartialTransform(float* signal, uint32 signalLength)
    {
        // process the signal
        uint32 offset = 0;
        const uint32 halfLength = signalLength >> 1;
        const uint32 numSamples = signalLength - 3;
        for (uint32 j = 0; j < numSamples; j += 2)
        {
            mTempBuffer[offset]             = signal[j] * mH[0] + signal[j + 1] * mH[1] + signal[j + 2] * mH[2] + signal[j + 3] * mH[3];
            mTempBuffer[offset + halfLength]  = signal[j] * mG[0] + signal[j + 1] * mG[1] + signal[j + 2] * mG[2] + signal[j + 3] * mG[3];
            offset++;
        }

        // handle the end, act like the data is wrapping around
        mTempBuffer[offset]             = signal[signalLength - 2] * mH[0] + signal[signalLength - 1] * mH[1] + signal[0] * mH[2] + signal[1] * mH[3];
        mTempBuffer[offset + halfLength]  = signal[signalLength - 2] * mG[0] + signal[signalLength - 1] * mG[1] + signal[0] * mG[2] + signal[1] * mG[3];

        // copy the temp buffer results into the buffer
        MCore::MemCopy(signal, mTempBuffer, sizeof(float) * signalLength);
    }


    // partial inverse transform
    void D4Wavelet::PartialInverseTransform(float* coefficients, uint32 signalLength)
    {
        // precalc some things
        const uint32 halfLength     = signalLength >> 1;
        const uint32 halfPlusOne    = halfLength + 1;
        const uint32 halfMinusOne   = halfLength - 1;

        // handle boundaries
        mTempBuffer[0] = coefficients[halfMinusOne] * mInvH[0] + coefficients[signalLength - 1] * mInvH[1] + coefficients[0] * mInvH[2] + coefficients[halfLength] * mInvH[3];
        mTempBuffer[1] = coefficients[halfMinusOne] * mInvG[0] + coefficients[signalLength - 1] * mInvG[1] + coefficients[0] * mInvG[2] + coefficients[halfLength] * mInvG[3];

        // process the signal
        uint32 offset = 2;
        for (uint32 i = 0; i < halfMinusOne; ++i)
        {
            mTempBuffer[offset++] = coefficients[i] * mInvH[0] + coefficients[i + halfLength] * mInvH[1] + coefficients[i + 1] * mInvH[2] + coefficients[i + halfPlusOne] * mInvH[3];
            mTempBuffer[offset++] = coefficients[i] * mInvG[0] + coefficients[i + halfLength] * mInvG[1] + coefficients[i + 1] * mInvG[2] + coefficients[i + halfPlusOne] * mInvG[3];
        }

        // copy the temp buffer results into the buffer
        MCore::MemCopy(coefficients, mTempBuffer, sizeof(float) * signalLength);
    }
}   // namespace MCore
