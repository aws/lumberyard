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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_CIE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_CIE_H
#pragma once


#include <Cry_Color.h>

struct CIE
{
    // (I)ntensity, (R)ed chromaticity, (B)lue chromaticity, (A)lpha
    // NOTE: http://en.wikipedia.org/wiki/CIE_1931_color_space
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
        const float iTmp = i * 3.0f;

        // LUV/Yxz 565-storeable chromaticity space
        ret.b = bTmp * iTmp;
        ret.r = rTmp * iTmp;
        ret.g = iTmp - (ret.r + ret.b);

        ret.r = saturate(ret.r);
        ret.g = saturate(ret.g);
        ret.b = saturate(ret.b);

        ret.a = a;

        return ret;
    }

    CIE& operator =(const ColorF& rgbx)
    {
        const float iTmp = rgbx.g + (rgbx.r + rgbx.b);
        const float rTmp = (iTmp != 0.0f) ? rgbx.r / iTmp : (0.0f);
        const float bTmp = (iTmp != 0.0f) ? rgbx.b / iTmp : (0.0f);

        i = saturate(iTmp / 3.0f);
        r = saturate(rTmp);
        b = saturate(bTmp);

        a = rgbx.a;

        return *this;
    }
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_CIE_H
