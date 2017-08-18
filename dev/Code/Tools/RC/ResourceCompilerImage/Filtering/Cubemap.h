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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_FILTERING_CUBEMAP_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_FILTERING_CUBEMAP_H
#pragma once

//--------------------------------------------------------------------------------------------------
//structure used to store current filtering settings
struct SCubemapFilterParams
{
    float  BaseFilterAngle;
    float  InitialMipAngle;
    float  MipAnglePerLevelScale;
    float  BRDFGlossScale;
    float  BRDFGlossBias;
    int32  FilterType;
    int32  FixupType;
    int32  FixupWidth;
    int32  SampleCountGGX;
    bool8  bUseSolidAngle;

    SCubemapFilterParams()
    {
        BaseFilterAngle = 20.0;
        InitialMipAngle = 1.0;
        MipAnglePerLevelScale = 1.0;
        BRDFGlossScale = 1.0f;
        BRDFGlossBias = 0.0f;
        FilterType = CP_FILTER_TYPE_ANGULAR_GAUSSIAN;
        FixupType = CP_FIXUP_PULL_LINEAR;
        FixupWidth = 3;
        bUseSolidAngle = TRUE;
    }
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_FILTERING_CUBEMAP_H
