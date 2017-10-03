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

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"


namespace MCore
{
    /**
     *
     *
     *
     */
    class MCORE_API Wavelet
    {
        MCORE_MEMORYOBJECTCATEGORY(Wavelet, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_WAVELETS);

    public:
        Wavelet();
        virtual ~Wavelet();

        virtual uint32 GetType() const = 0;
        virtual const char* GetName() const = 0;

        // main functions
        void Transform(float* signal, uint32 signalLength);
        void InverseTransform(float* coefficients, uint32 signalLength);

        void LossyTransform(float* signal, uint32 signalLength, float lossyFactor);
        void LossyInverseTransform(float* coefficients, uint32 signalLength, float lossyFactor);

    protected:
        float*  mTempBuffer;        // the temp buffer
        uint32  mTempBufferLength;  // temp buffer length

        // single level transforms
        virtual void PartialTransform(float* signal, uint32 signalLength) = 0;
        virtual void PartialInverseTransform(float* coefficients, uint32 signalLength) = 0;

        // misc low-level
        void ReleaseTempBuffer();
        void AssureTempBufferSize(uint32 length);
    };
}   // namespace MCore
