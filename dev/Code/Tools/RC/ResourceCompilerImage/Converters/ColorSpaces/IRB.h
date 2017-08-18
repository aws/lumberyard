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

#ifndef IRB_h
#define IRB_h

#include <Cry_Color.h>

struct IRB
{
    // (I)ntensity, (R)ed chromaticity, (B)lue chromaticity, (A)lpha
    // NOTE: Crytek original research
    float i;
    float r;
    float b;
    float a;

    static inline float saturate(float x)
    {
        return std::max(0.0f, std::min(x, 1.0f));
    }

    operator ColorF() const
    {
        ColorF ret;

        const float bTmp = b;
        const float rTmp = r;
        const float iTmp = i * 2.0f;

        // LUV inspired 565-storeable chromaticity space
        ret.b = bTmp * iTmp;
        ret.r = rTmp * iTmp;
        ret.g = iTmp - std::max(ret.r, ret.b);

        ret.r = saturate(ret.r);
        ret.g = saturate(ret.g);
        ret.b = saturate(ret.b);

        ret.a = a;

        return ret;
    }

    IRB& operator =(const ColorF& rgbx)
    {
        const float iTmp = rgbx.g + std::max(rgbx.r, rgbx.b);
        const float rTmp = (iTmp != 0.0f) ? rgbx.r / iTmp : (0.0f);
        const float bTmp = (iTmp != 0.0f) ? rgbx.b / iTmp : (0.0f);

        i = saturate(iTmp * 0.5f);
        r = saturate(rTmp);
        b = saturate(bTmp);

        a = rgbx.a;

        return *this;
    }
};

#endif
