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

#ifndef CRYINCLUDE_EDITOR_VEGETATIONPANEL_H
#define CRYINCLUDE_EDITOR_VEGETATIONPANEL_H

#pragma once

#include <QWidget>
#include <QSet>

class CVegetationObject;
class CVegetationMap;
class CPanelPreview;
class CVegetationTool;
class QItemSelection;
class QMenu;
class VegetationCategoryTreeModel;

namespace Ui
{
    class VegetationPanel;
}

/////////////////////////////////////////////////////////////////////////////
// CVegetationPanel dialog

class CVegetationPanel
    : public QWidget
    , public IEditorNotifyListener
{
    // Construction
public:
    CVegetationPanel(CVegetationTool* tool, QWidget* parent = nullptr);     // standard constructor
    ~CVegetationPanel();

    void SetPreviewPanel(CPanelPreview* panel) { m_previewPanel = panel; }
    void SelectObject(CVegetationObject* object, bool bAddToSelection);
    void UnselectAll();
    void UpdateObjectInTree(CVegetationObject* object, bool bUpdateInfo = true);
    void UpdateAllObjectsInTree();
    void SetBrush(float r);

    void UpdateUI();
    void ReloadObjects();

    // Implementation
private:
    using Selection = std::vector<CVegetationObject*>;

    void OnInitDialog();
    void CreateObjectsContextMenu();

    void UpdateInfo();

    void AddObjectToTree(CVegetationObject* object, bool bExpandCategory = true);

    void GetObjectsInCategory(const QString& category, Selection& objects);
    void SendToControls();

    void AddLayerVars(CVarBlock* pVarBlock, CVegetationObject* pObject = NULL);
    void OnLayerVarChange(IVariable* pVar);
    void SendTextureLayersToControls();
    bool GetTerrainLayerNames(QStringList& layerNames);
    void OnGetSettingFromTerrainLayer(int layerId);
    void GotoObjectMaterial();
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    void OnBrushRadiusChange();
    void OnBrushRadiusSliderChange(int value);
    void OnRemoveDuplVegetation();

    void OnAdd();
    void OnClone();
    void OnReplace();
    void OnRemove();
    void OnInstancesToCategory();
    void OnPaint();

    void OnNewCategory();
    void OnRenameCategory();
    void OnRemoveCategory();
    void OnDistribute();
    void OnClear();
    void OnBnClickedImport();
    void OnBnClickedMerge();
    void OnBnClickedExport();
    void OnObjectSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void OnBnClickedScale();
    void OnBnClickedRandomRotate();
    void OnBnClickedClearRotate();
    void OnObjectsRClick(const QPoint& point);
    void OnUpdateNumbers();

    //////////////////////////////////////////////////////////////////////////
    QScopedPointer<Ui::VegetationPanel> m_ui;
    QMenu* m_menu;
    VegetationCategoryTreeModel* m_model;

    CVegetationTool* m_tool;

    CPanelPreview* m_previewPanel;

    CVegetationMap* m_vegetationMap;

    bool m_bIgnoreSelChange;

    TSmartPtr<CVarBlock> m_varBlock;

    QSet<QObject*> m_enabledOnObjectSelected;
    QSet<QObject*> m_enabledOnCategorySelected;
    QSet<QObject*> m_enabledOnOneObjectSelected;
};

#endif // CRYINCLUDE_EDITOR_VEGETATIONPANEL_H
