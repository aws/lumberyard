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

#ifndef CRYINCLUDE_EDITOR_VEGETATIONDATABASEPAGE_H
#define CRYINCLUDE_EDITOR_VEGETATIONDATABASEPAGE_H

#pragma once



#include "DataBaseDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CVegetationDataBasePage dialog

class CVegetationDataBaseModel;
class CVegetationObject;
class CVegetationMap;
class CPanelPreview;
class CPreviewModelCtrl;

namespace Ui {
    class CVegetationDataBasePage;
}

class CVegetationDataBasePage
    : public CDataBaseDialogPage
    , public IEditorNotifyListener
{
    Q_OBJECT
    // Construction
public:
    CVegetationDataBasePage(QWidget* pParent = NULL);   // standard constructor
    ~CVegetationDataBasePage();

    static void RegisterViewClass();
    static const GUID& GetClassID();

    //////////////////////////////////////////////////////////////////////////
    // CDataBaseDialogPage implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetActive(bool bActive);
    virtual void Update();
    //////////////////////////////////////////////////////////////////////////

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    void SelectObject(CVegetationObject* object);
    void UpdateInfo();

    void ReloadObjects();

    bool eventFilter(QObject* watched, QEvent* event) override;

    // Implementation
protected:
    void DoIdleUpdate();
    void SyncSelection();

    void OnSelectionChange();

    void GetSelectedObjects(std::vector<CVegetationObject*>& objects);
    void AddLayerVars(CVarBlock* pVarBlock, CVegetationObject* pObject = NULL);
    void OnLayerVarChange(IVariable* pVar);
    void GotoObjectMaterial();

    CVegetationObject* FindObject(REFGUID guid);
    void ClearSelection();

    // Generated message map functions
    void OnAdd();
    void OnClone();
    void OnReplace();
    void OnRemove();

    void OnNewCategory();
    void OnDistribute();
    void OnClear();
    void OnBnClickedMerge();
    void OnBnClickedCreateSel();
    void OnBnClickedImport();
    void OnBnClickedExport();
    void OnBnClickedScale();

    void OnHide();
    void OnUnhide();
    void OnHideAll();
    void OnUnhideAll();

    void OnReportClick(const QModelIndex& index);
    void OnReportRClick(const QPoint& pos);
    void OnReportSelChange();

    typedef std::vector<CVegetationObject*> Selection;
    CVegetationMap* m_vegetationMap;

    bool m_bIgnoreSelChange;
    bool m_bActive;

    TSmartPtr<CVarBlock> m_varBlock;

    bool m_bSyncSelection;

    //Command for correct sync. Used in DoIdleUpdate()
    int m_nCommand;

    QScopedPointer<Ui::CVegetationDataBasePage> m_ui;

    CVegetationDataBaseModel* m_vegetationDataBaseModel;
};

#endif // CRYINCLUDE_EDITOR_VEGETATIONDATABASEPAGE_H
