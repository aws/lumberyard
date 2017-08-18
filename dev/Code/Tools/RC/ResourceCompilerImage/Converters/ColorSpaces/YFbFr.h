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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_YFBFR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_YFBFR_H
#pragma once


#include <Cry_Color.h>

struct YFbFr
{
    float fr;
    float y;
    float fb;
    float a;

    operator ColorF() const
    {
        ColorF ret;

        const float frTmp = fr * 2.0f - 1.0f;
        const float  yTmp = y;
        const float fbTmp = fb * 2.0f - 1.0f;

        ret.r = yTmp - 3.0f / 8.0f * fbTmp + 1.0f / 2.0f * frTmp;
        ret.g = yTmp + 5.0f / 8.0f * fbTmp;
        ret.b = yTmp - 3.0f / 8.0f * fbTmp - 1.0f / 2.0f * frTmp;

        ret.a = a;

        return ret;
    }

    YFbFr& operator =(const ColorF& rgbx)
    {
        const float  yTmp =  5.0f / 16.0f * rgbx.r + 3.0f / 8.0f * rgbx.g + 5.0f / 16.0f * rgbx.b;
        const float fbTmp = -1.0f /  2.0f * rgbx.r +        1.0f * rgbx.g - 1.0f /  2.0f * rgbx.b;
        const float frTmp =          1.0f * rgbx.r                        -         1.0f * rgbx.b;

        fr = frTmp * 0.5f + 0.5f;
        y  =  yTmp;
        fb = fbTmp * 0.5f + 0.5f;

        a  = rgbx.a;

        return *this;
    }
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_YFBFR_H
