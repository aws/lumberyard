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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_IMAGEHISTOGRAMCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_IMAGEHISTOGRAMCTRL_H
#pragma once

#include "Util/ImageHistogram.h"

#include <QWidget>

class SANDBOX_API CImageHistogramCtrl
    : public QWidget
    , public CImageHistogram
{
    Q_OBJECT
public:
    enum EDrawMode
    {
        eDrawMode_Luminosity,
        eDrawMode_OverlappedRGB,
        eDrawMode_SplitRGB,
        eDrawMode_RedChannel,
        eDrawMode_GreenChannel,
        eDrawMode_BlueChannel,
        eDrawMode_AlphaChannel
    };

    CImageHistogramCtrl(QWidget* parent = nullptr);
    virtual ~CImageHistogramCtrl();

    void paintEvent(QPaintEvent* event) override;

    int             m_graphMargin;
    float           m_graphHeightPercent;
    EDrawMode       m_drawMode;
    QColor          m_backColor;

    void mousePressEvent(QMouseEvent* event) override;
};



#endif // CRYINCLUDE_EDITOR_CONTROLS_IMAGEHISTOGRAMCTRL_H
