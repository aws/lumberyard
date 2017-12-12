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
#include "CDF97Wavelet.h"
#include "FastMath.h"


namespace MCore
{
    // constructor
    CDF97Wavelet::CDF97Wavelet()
        : Wavelet()
    {
    }


    // destructor
    CDF97Wavelet::~CDF97Wavelet()
    {
    }


    // partial forward transform
    void CDF97Wavelet::PartialTransform(float* signal, uint32 signalLength)
    {
        float a;
        uint32 i;

        // Predict 1
        a = -1.586134342f;
        for (i = 1; i < signalLength - 2; i += 2)
        {
            signal[i] += a * (signal[i - 1] + signal[i + 1]);
        }

        signal[signalLength - 1] += 2 * a * signal[signalLength - 2];

        // Update 1
        a = -0.05298011854f;
        for (i = 2; i < signalLength; i += 2)
        {
            signal[i] += a * (signal[i - 1] + signal[i + 1]);
        }

        signal[0] += 2 * a * signal[1];

        // Predict 2
        a = 0.8829110762f;
        for (i = 1; i < signalLength - 2; i += 2)
        {
            signal[i] += a * (signal[i - 1] + signal[i + 1]);
        }

        signal[signalLength - 1] += 2 * a * signal[signalLength - 2];

        // Update 2
        a = 0.4435068522f;
        for (i = 2; i < signalLength; i += 2)
        {
            signal[i] += a * (signal[i - 1] + signal[i + 1]);
        }

        signal[0] += 2 * a * signal[1];

        // Scale
        a = 1.0f / 1.149604398f;
        for (i = 0; i < signalLength; i++)
        {
            if (i % 2)
            {
                signal[i] *= a;
            }
            else
            {
                signal[i] /= a;
            }
        }

        // Pack
        for (i = 0; i < signalLength; i++)
        {
            if (i % 2 == 0)
            {
                mTempBuffer[i / 2] = signal[i];
            }
            else
            {
                mTempBuffer[signalLength / 2 + i / 2] = signal[i];
            }
        }

        // copy the temp buffer results into the buffer
        MCore::MemCopy(signal, mTempBuffer, sizeof(float) * signalLength);
    }


    // partial inverse transform
    void CDF97Wavelet::PartialInverseTransform(float* coefficients, uint32 signalLength)
    {
        float a;
        uint32 i;

        // Unpack
        for (i = 0; i < signalLength / 2; i++)
        {
            mTempBuffer[i * 2] = coefficients[i];
            mTempBuffer[i * 2 + 1] = coefficients[i + signalLength / 2];
        }

        // copy the temp buffer results into the buffer
        MCore::MemCopy(coefficients, mTempBuffer, sizeof(float) * signalLength);

        // Undo scale
        a = 1.149604398f;
        for (i = 0; i < signalLength; i++)
        {
            if (i % 2)
            {
                coefficients[i] *= a;
            }
            else
            {
                coefficients[i] /= a;
            }
        }

        // Undo update 2
        a = -0.4435068522f;
        for (i = 2; i < signalLength; i += 2)
        {
            coefficients[i] += a * (coefficients[i - 1] + coefficients[i + 1]);
        }

        coefficients[0] += 2 * a * coefficients[1];

        // Undo predict 2
        a = -0.8829110762f;
        for (i = 1; i < signalLength - 2; i += 2)
        {
            coefficients[i] += a * (coefficients[i - 1] + coefficients[i + 1]);
        }

        coefficients[signalLength - 1] += 2 * a * coefficients[signalLength - 2];

        // Undo update 1
        a = 0.05298011854f;
        for (i = 2; i < signalLength; i += 2)
        {
            coefficients[i] += a * (coefficients[i - 1] + coefficients[i + 1]);
        }

        coefficients[0] += 2 * a * coefficients[1];

        // Undo predict 1
        a = 1.586134342f;
        for (i = 1; i < signalLength - 2; i += 2)
        {
            coefficients[i] += a * (coefficients[i - 1] + coefficients[i + 1]);
        }

        coefficients[signalLength - 1] += 2 * a * coefficients[signalLength - 2];

        // copy the temp buffer results into the buffer
        //MCore::MemCopy(coefficients, mTempBuffer, sizeof(float) * signalLength );
    }
}   // namespace MCore
