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

#ifndef CRYINCLUDE_EDITOR_TERRAINHOLEPANEL_H
#define CRYINCLUDE_EDITOR_TERRAINHOLEPANEL_H

#pragma once
// TerrainHolePanel.h : header file
//

//#include "Controls/SliderCtrlEx.h"                // CSliderCtrlEx

/////////////////////////////////////////////////////////////////////////////
// CTerrainHolePanel dialog
#include <QWidget>
#include <QScopedPointer>
class QRadioButton;
class QSlider;

namespace Ui {
    class CTerrainHolePanel;
}

class CTerrainHolePanel
    : public QWidget
{
    // Construction
public:
    CTerrainHolePanel(class CTerrainHoleTool* tool, QWidget* pParent = nullptr);   // standard constructor

    // Dialog Data
    QRadioButton*   m_removeHole;
    QRadioButton*   m_makeHole;

    void SetMakeHole(bool bEnable);
    void SetRadius();

    // Implementation
protected:
    void OnHoleMake(bool v);
    void OnReleasedcaptureRadius(int value);

    QSlider*    m_radius;
    CTerrainHoleTool* m_tool;
    QScopedPointer<Ui::CTerrainHolePanel> ui;
};

#endif // CRYINCLUDE_EDITOR_TERRAINHOLEPANEL_H
