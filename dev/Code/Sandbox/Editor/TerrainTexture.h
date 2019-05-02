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

#pragma once
#ifndef CRYINCLUDE_EDITOR_TERRAINTEXTURE_H
#define CRYINCLUDE_EDITOR_TERRAINTEXTURE_H

#include <QDialog>
#include <QAtomicInteger>
#include <QAbstractTableModel>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <IEditor.h>

#include "IDataBaseManager.h"

// forward declarations.
class CLayer;

namespace Ui 
{
    class TerrainTextureDialog;
}

namespace Physics
{
    class MaterialSelection;
}

class QItemSelection;

// Internal resolution of the final texture preview
#define FINAL_TEX_PREVIEW_PRECISION_CX 256
#define FINAL_TEX_PREVIEW_PRECISION_CY 256

// Hold / fetch temp file
//#define HOLD_FETCH_FILE_TTS "Temp\\HoldStateTemp.lay"

/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureDialog dialog

// KDAB this looks dead
enum SurfaceGenerateFlags
{
    GEN_USE_LIGHTING        = 1,
    GEN_SHOW_WATER          = 1 << 2,
    GEN_SHOW_WATERCOLOR = 1 << 3,
    GEN_KEEP_LAYERMASKS = 1 << 4,
    GEN_ABGR            = 1 << 5,
    GEN_STATOBJ_SHADOWS = 1 << 6
};

class TerrainTextureLayerEditModel;

class CTerrainTextureDialog
    : public QDialog
    , public IEditorNotifyListener
    , public IDataBaseManagerListener
    , private AzToolsFramework::IPropertyEditorNotify
{
    Q_OBJECT

public:
    CTerrainTextureDialog(QWidget* parent = nullptr);   // standard constructor
    ~CTerrainTextureDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID();

    void OnUndoUpdate();

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener implementation
    //////////////////////////////////////////////////////////////////////////
    // Called by the editor to notify the listener about the specified event.
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

    // IDataBaseManagerListener
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);
    // ~IDataBaseManagerListener

protected:
    typedef std::vector<CLayer*> Layers;

    void ReloadLayerList();

    void ClearData();

    // Dialog control related functions
    void UpdateControlData();
    void EnableControls();

    void SelectLayer(CLayer* pLayer);

    // Assign selected material to the selected layers.
    void OnAssignMaterial();
    void UpdateAssignMaterialItem();

    // Assign a mask to the selected layers.
    void OnAssignSplatMap();
    void UpdateAssignSplatMapItem();

    void OnImportSplatMaps();
    void ImportSplatMaps();

    Layers GetSelectedLayers();

    // Generated message map functions
    void OnInitDialog();

    void OnLoadTexture();
    void OnImport();
    void OnExport();
    void OnFileExportLargePreview();
    void OnGeneratePreview();
    void OnApplyLighting();
    void OnSetOceanLevel();
    void OnLayerExportTexture();
    void OnShowWater();
    void OnRefineTerrainTextureTiles();

    void OnLayersNewItem();
    void OnLayersDeleteItem();
    void OnLayersMoveItemUp();
    void OnLayersMoveItemDown();

    void OnReportSelChange(const QItemSelection& selected, const QItemSelection& deselected);
    void OnReportHyperlink(CLayer* layer);

    void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
    void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
    void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
    void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
    void SealUndoStack() override;

private:

    QScopedPointer<Ui::TerrainTextureDialog> m_ui;
    AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
    AZStd::unique_ptr<Physics::MaterialSelection> m_selection;

    bool m_alive = 1;

    TerrainTextureLayerEditModel* m_model;

    bool m_bIgnoreNotify;
};
#endif // CRYINCLUDE_EDITOR_TERRAINTEXTURE_H
