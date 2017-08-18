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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_OPERATIONS_COLORCHART_COLORCHARTBASE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_OPERATIONS_COLORCHART_COLORCHARTBASE_H
#pragma once



#include "ColorChart.h"


class ImageObject;


const int CHART_IMAGE_WIDTH = 78;
const int CHART_IMAGE_HEIGHT = 66;


class CColorChartBase
    : public IColorChart
{
public:
    // IColorChart interface
    virtual void Release() = 0;
    virtual void GenerateDefault() = 0;
    virtual bool GenerateFromInput(ImageObject* pImg);
    virtual ImageObject* GenerateChartImage() = 0;

protected:
    CColorChartBase();
    virtual ~CColorChartBase();

    bool ExtractFromImage(ImageObject* pImg);
    virtual bool ExtractFromImageAt(ImageObject* pImg, uint32 x, uint32 y) = 0;
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_OPERATIONS_COLORCHART_COLORCHARTBASE_H
