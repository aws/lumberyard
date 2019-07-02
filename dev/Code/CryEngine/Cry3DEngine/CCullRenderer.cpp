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
#include "CCullRenderer.h"

#include <3dEngine.h>
#include <cmath>

namespace NAsyncCull
{


    int UpdateCullDebugData(const float* __restrict pVMemZ, uint32 x, float* colorData, int numElem)
    {
        // level details near or far from camera could be done by adjusting color curve
        const float fColorCurvePow = C3DEngine::GetCVars()->e_CoverageBufferOccludersViewDistRatio * 0.8f;
        const int multPerChannelSize = 3;
        const float multPerChannel[multPerChannelSize]{ 0.03125f, 0.0625f, 0.013125f };
        const int colorArraySize = 4;
        float fValueColor[colorArraySize]{ pow((pVMemZ[x + 0]), fColorCurvePow) , pow((pVMemZ[x + 1]),
            fColorCurvePow), pow((pVMemZ[x + 2]), fColorCurvePow), pow((pVMemZ[x + 3]), fColorCurvePow) };

        for (int colorIndex = 0; colorIndex < colorArraySize; ++colorIndex)
        {
            for (int multIndex = 0; multIndex < multPerChannelSize; ++multIndex)
            {
                colorData[numElem] = fValueColor[colorIndex] * multPerChannel[multIndex];
                ++numElem;
            }
            colorData[numElem] = 255; // alpha is a constant value
            ++numElem;
        }

        return numElem;
    }

}
