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

#ifndef CRYINCLUDE_EDITOR_TERRAINPAINTERPANEL_H
#define CRYINCLUDE_EDITOR_TERRAINPAINTERPANEL_H

#pragma once

#include <QWidget>

struct CTextureBrush;
class CLayer;
class CTerrainTexturePainter;
class TerrainTextureLayerModel;

namespace Ui
{
    class TerrainPainterPanel;
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainPainterPanel dialog

class CTerrainPainterPanel
    : public QWidget
    , public IEditorNotifyListener
{
    Q_OBJECT

    // Construction
public:
    CTerrainPainterPanel(CTerrainTexturePainter& tool, QWidget* pParent = nullptr);
    virtual ~CTerrainPainterPanel();

    void SetBrush(CTextureBrush& brush);
    CLayer* GetSelectedLayer();
    // Arguments:
    //   pLayer - can be 0 to deselect layer
    void SelectLayer(const CLayer* pLayer);

    // Overrides
protected:
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    // Implementation
protected:

    void ReloadLayers();
    bool SetMaskLayer(uint32 layerId);

protected slots:
    void OnSliderChange();
    void UpdateTextureBrushSettings();
    void OnBrushResetBrightness();
    void SetLayerMaskSettingsToLayer();
    void OnLayersDblClk();
    void OnLayersClick();
    void GetLayerMaskSettingsFromLayer();
    void OnFloodLayer();
    void OnBnClickedBrushSettolayer();

private:
    QScopedPointer<Ui::TerrainPainterPanel> m_ui;
    TerrainTextureLayerModel* m_model;

    CTerrainTexturePainter& m_tool;

    bool m_bIgnoreNotify;
};

#endif // CRYINCLUDE_EDITOR_TERRAINPAINTERPANEL_H
