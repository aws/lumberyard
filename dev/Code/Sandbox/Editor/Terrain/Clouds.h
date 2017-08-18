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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_CLOUDS_H
#define CRYINCLUDE_EDITOR_TERRAIN_CLOUDS_H
#pragma once

#include <time.h>
#include "Noise.h"

class CXmlArchive;

class QImage;
class QLabel;
class QPainter;
class QRect;

class CClouds
    : public QObject
{
    Q_OBJECT
public:
    bool GenerateClouds(SNoiseParams* sParam, QLabel* pWndStatus);
    void DrawClouds(QPainter& pDC, const QRect& prcPosition);
    void Serialize(CArchive& ar);
    void Serialize(CXmlArchive& xmlAr);

    SNoiseParams* GetLastParam() { return &m_sLastParam; };
    CClouds();
    virtual ~CClouds();

protected:
    float CloudExpCurve(float v, unsigned int iCover, float fSharpness);

    unsigned int m_iWidth;
    unsigned int m_iHeight;

    QImage m_bmpClouds;

    // Used to hold the last used generation parameters
    SNoiseParams m_sLastParam;

private:

    void CalcCloudsPerlin(SNoiseParams* sParam, QLabel* pWndStatus);
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_CLOUDS_H
