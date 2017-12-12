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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_YCBCR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_YCBCR_H
#pragma once


#include <Cry_Color.h>

struct YCbCr
{
    float cr;
    float y;
    float cb;
    float a;

    operator ColorF() const
    {
        ColorF ret;

        const float crTmp = this->cr * 2.0f - 1.0f;
        const float  yTmp = this->y;
        const float cbTmp = this->cb * 2.0f - 1.0f;

        ret.r = yTmp + 1.402000f * crTmp;
        ret.g = yTmp - 0.344136f * crTmp - 0.714136f * cbTmp;
        ret.b = yTmp                     + 1.772000f * cbTmp;

        ret.a = a;

        return ret;
    }

    YCbCr& operator =(const ColorF& rgbx)
    {
        float  yTmp =  0.299000f * rgbx.r + 0.587000f * rgbx.g + 0.114000f * rgbx.b;
        float crTmp = -0.168736f * rgbx.r - 0.331264f * rgbx.g + 0.500000f * rgbx.b;
        float cbTmp =  0.500000f * rgbx.r - 0.418688f * rgbx.g - 0.081312f * rgbx.b;

        this->cr = crTmp * 0.5f + 0.5f;
        this->y  =  yTmp;
        this->cb = cbTmp * 0.5f + 0.5f;

        a  = rgbx.a;

        return *this;
    }
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_YCBCR_H
