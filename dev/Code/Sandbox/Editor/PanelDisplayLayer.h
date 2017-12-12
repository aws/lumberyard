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

#ifndef CRYINCLUDE_EDITOR_PANELDISPLAYLAYER_H
#define CRYINCLUDE_EDITOR_PANELDISPLAYLAYER_H

#pragma once

#include <QWidget>

class CObjectLayer;
class CObjectLayerManager;
class QToolBar;

namespace Ui {
    class PanelDisplayLayer;
}

#define LAYER_EDITOR_VER "1.00"

class CPanelDisplayLayer
    : public QWidget
{
public:
    explicit CPanelDisplayLayer(QWidget* parent = nullptr);
    virtual ~CPanelDisplayLayer();

    static const char* ClassName() { return LyViewPane::LegacyLayerEditor; }
    static const GUID& GetClassID();
    static void RegisterViewClass();

protected:
    //! Callback called when layer is updated.
    void OnLayerUpdate(int event, CObjectLayer* pLayer);
    void SelectLayer(CObjectLayer* pLayer);

    void ReloadLayers();

    void UpdateStates();

private:
    void UpdateLayerItem(CObjectLayer* pLayer);
    void ChangeControlSize();

    void OnSelChanged();
    void OnShowFullMenu();
    void OnBnClickedNew();
    void OnLoadVisPreset();
    void OnSaveVisPreset();
    void OnBnClickedDelete();
    void OnBnClickedExport();
    void OnBnClickedReload();
    void OnBnClickedImport();
    void OnBnClickedSaveExternalLayers();
    void OnBnClickedFreezeROnly();
    void OnBnClickedRename();
    void OnVisibleButtonClicked(bool bForceVisableState);
    void OnUsableButtonClicked(bool bForceUsableState);

    void contextMenuEvent(QContextMenuEvent*) override;
    void keyPressEvent(QKeyEvent* e) override;

    void ShowContextMenu(const QStringList& extraActions = {});

    bool m_bLayersValid;

    CObjectLayer* m_currentLayer;
    CObjectLayerManager* m_pLayerManager;

    int m_nPrevHeight;

    Ui::PanelDisplayLayer* ui;
};

#endif // CRYINCLUDE_EDITOR_PANELDISPLAYLAYER_H
