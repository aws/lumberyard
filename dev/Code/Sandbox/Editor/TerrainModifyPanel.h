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

#ifndef CRYINCLUDE_EDITOR_TERRAINMODIFYPANEL_H
#define CRYINCLUDE_EDITOR_TERRAINMODIFYPANEL_H

#pragma once
// TerrainModifyPanel.h : header file
//

#include <QWidget>
#include <QLabel>
#include "TerrainModifyTool.h"

#define MAX_TERRAIN_BRUSH_TYPES 4

class CTerrainModifyTool;

namespace Ui
{
    class TerrainModifyPanel;
}

/////////////////////////////////////////////////////////////////////////////
// Brush Preview Picture Box
// Visualize the brush settings controlled by sliders
class CBrushPreviewPictureBox
    : public QLabel
{
    Q_OBJECT

public:
    CBrushPreviewPictureBox(QWidget* parent = nullptr);

    void SetBrush(const CTerrainBrush& brush);

    QSize sizeHint() const override;

private:
    void render(const QSize& bounds);

    CTerrainBrush m_brush;
};


/////////////////////////////////////////////////////////////////////////////
// CTerrainModifyPanel dialog
class CTerrainModifyPanel
    : public QWidget
{
    Q_OBJECT

    // Construction
public:
    CTerrainModifyPanel(QWidget* parent = nullptr);   // standard constructor

    void SetModifyTool(CTerrainModifyTool* tool);

    void SetBrush(CTerrainBrush* pBrush, bool bSyncRadiuses);

private:
    void EnableRadius(bool isEnable);
    void EnableRadiusInner(bool isEnable);
    void EnableHardness(bool isEnable);
    void EnableHeight(bool isEnable);
    void EnsureActiveEditTool();

    // Implementation
protected:
    void OnInitDialog();
    void OnUpdateNumbers();
    void OnBrushTypeCmd(const QString& brushType);

    QScopedPointer<Ui::TerrainModifyPanel> m_ui;

    CTerrainModifyTool* m_tool;
    bool m_shouldEnsureActiveEditTool;
    bool m_inSyncCallback;

protected slots:
    void OnBrushNoise();
    void OnRepositionObjects();
    void OnRepositionVegetation();
    void OnSyncRadius();
};

#endif // CRYINCLUDE_EDITOR_TERRAINMODIFYPANEL_H
