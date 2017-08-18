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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_RGBL_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_RGBL_H
#pragma once


#include <Cry_Color.h>

namespace RGBL
{
    template<typename T>
    inline T GetLuminance(const T& r, const T& g, const T& b)
    {
        return (r * 30 + g * 59 + b * 11 + 50) / 100;
    }

    template<>
    inline float GetLuminance<float>(const float& r, const float& g, const float& b)
    {
        return (r * 0.30f + g * 0.59f + b * 0.11f);
    }

    static inline uint8 GetLuminance(const ColorB& rgbx)
    {
        const ColorB col = rgbx;

        return GetLuminance(col.r, col.g, col.b);
    }

    static inline float GetLuminance(const ColorF& rgbx)
    {
        const ColorF col = rgbx;

        return GetLuminance(col.r, col.g, col.b);
    }
}

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_COLORSPACES_RGBL_H
