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

#ifndef CRYINCLUDE_EDITOR_TERRAINDIALOG_H
#define CRYINCLUDE_EDITOR_TERRAINDIALOG_H
#pragma once

#include <QMainWindow>
#include <LmbrCentral/Physics/WaterNotificationBus.h>

struct SNoiseParams;
class CHeightmap;
class CTerrainModifyTool;
class CTopRendererWnd;
class QLabel;

namespace Ui {
    class TerrainDialog;
}

/////////////////////////////////////////////////////////////////////////////
// CTerrainDialog dialog

class CTerrainDialog
    : public QMainWindow
    , public IEditorNotifyListener
    , private LmbrCentral::WaterNotificationBus::Handler
{
    Q_OBJECT

public:
    explicit CTerrainDialog(QWidget* parent = nullptr);
    ~CTerrainDialog();

    SNoiseParams* GetLastParam() { return m_sLastParam; };

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    static const GUID& GetClassID();
    static void RegisterViewClass();

protected:
    void CreateModifyTool();
    void Flatten(float fFactor);
    float ExpCurve(float v, unsigned int iCover, float fSharpness);

    void InvalidateViewport();
    void InvalidateTerrain();

    void OnTerrainLower();
    void OnTerrainRaise();
    void OnInitDialog();
    void OnTerrainLoad();
    void OnTerrainErase();
    void OnBrush1();
    void OnBrush2();
    void OnBrush3();
    void OnBrush4();
    void OnBrush5();
    void OnTerrainResize();
    void OnTerrainLight();
    void OnTerrainTextureImportExport();
    void OnTerrainSurface();
    void OnTerrainGenerate();
    void OnTerrainInvert();
    void OnExportHeightmap();
    void OnModifyMakeisle();
    void OnModifyFlatten();
    void OnModifySmooth();
    void OnModifyRemoveOcean();
    void OnModifySmoothSlope();
    void OnHeightmapShowLargePreview();
    void OnModifyNormalize();
    void OnModifyReduceRange();
    void OnModifyReduceRangeLight();
    void OnLowOpacity();
    void OnMediumOpacity();
    void OnHighOpacity();
    void OnHold();
    void OnFetch();
    void OnOptionsShowMapObjects();
    void OnOptionsShowWater();
    void OnOptionsAutoScaleGreyRange();
    void OnOptionsEditTerrainCurve();
    void OnOptionsShowGrid();
    void OnSetOceanLevel();
    void OnSetMaxHeight();
    void OnSetUnitSize();

    void UpdateTerrainDimensions();

    //void OnCustomize();
    //void OnExportShortcuts();
    //void OnImportShortcuts();

    ////////////////////////////////////////////////////////////////////////////
    // WaterNotificationBus
    void OceanHeightChanged(float height) override;

    // Ensure that we prevent close events while in the middle of lengthy operations.
    void closeEvent(QCloseEvent* ev) override;

    QScopedPointer<Ui::TerrainDialog> m_ui;

    SNoiseParams* m_sLastParam;
    CHeightmap* m_pHeightmap;

    _smart_ptr<CTerrainModifyTool> m_pTerrainTool;

    QLabel* m_terrainDimensions;
    bool m_processing = false;
};

#endif // CRYINCLUDE_EDITOR_TERRAINDIALOG_H
