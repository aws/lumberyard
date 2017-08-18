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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_OPERATIONS_COLORCHART_COLORCHART_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_OPERATIONS_COLORCHART_COLORCHART_H
#pragma once



class ImageObject;


struct IColorChart
{
public:
    virtual void Release() = 0;
    virtual void GenerateDefault() = 0;
    virtual bool GenerateFromInput(ImageObject* pImg) = 0;
    virtual ImageObject* GenerateChartImage() = 0;

protected:
    virtual ~IColorChart() {};
};


IColorChart* Create3dLutColorChart();
IColorChart* CreateLeastSquaresLinColorChart();

inline uint32 GetAt(uint32 x, uint32 y, void* pData, uint32 pitch)
{
    uint32* p = (uint32*) ((size_t) pData + pitch * y + x * sizeof(uint32));
    return *p;
}

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_OPERATIONS_COLORCHART_COLORCHART_H
