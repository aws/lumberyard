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

#ifndef CRYINCLUDE_EDITOR_ENTITYPROTLIBDIALOG_H
#define CRYINCLUDE_EDITOR_ENTITYPROTLIBDIALOG_H
#pragma once

#include "BaseLibraryDialog.h"

class QLabel;
class QPreviewModelCtrl;
class ReflectedPropertyControl;
class QSplitter;
class CEntityPrototypeManager;
class CEntityPrototype;
struct IEntitySystem;
struct IEntity;
class CPreviewModelCtrl;

/** Dialog which hosts entity prototype library.
*/
class CEntityProtLibDialog
    : public CBaseLibraryDialog
{
    Q_OBJECT

public:
    CEntityProtLibDialog(QWidget* pParent);
    ~CEntityProtLibDialog();

    // Called every frame.
    void Update();
    virtual UINT GetDialogMenuID();

protected:
    void InitToolbar(UINT nToolbarResID);
    void OnInitDialog();
    void OnPlay();
    void OnUpdatePlay();
    void droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport);

protected slots:
    void OnAddPrototype();
    void OnAssignToSelection();
    void OnShowDescription();
    void OnNotifyTreeRClick(const QPoint& point);
    void OnSelectAssignedObjects();
    void OnReloadEntityScript();
    void OnChangeEntityClass();
    void OnCopy();
    void OnPaste();
    void OnRemoveLibrary();

protected:

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    void SelectItem(CBaseLibraryItem* item, bool bForceReload = false);

    //////////////////////////////////////////////////////////////////////////
    void SpawnEntity(CEntityPrototype* prototype);
    void ReleaseEntity();
    void OnUpdateProperties(IVariable* var);
    QString SelectEntityClass();
    CEntityPrototype* GetSelectedPrototype();
    void UpdatePreview();

    virtual QTreeView* GetTreeCtrl() override;
    virtual AbstractGroupProxyModel* GetProxyModel() override;
    virtual QAbstractItemModel* GetSourceModel() override;
    virtual BaseLibraryDialogModel* GetSourceDialogModel() override;

    bool MarkOrUnmarkPropertyField(IVariable* varArchetype, IVariable* varEntScript);
    void MarkFieldsThatDifferFromScriptDefaults(IVariable* varArchetype, IVariable* varEntScript);
    void MarkFieldsThatDifferFromScriptDefaults(CVarBlock* pArchetypeVarBlock, CVarBlock* pEntScriptVarBlock);

    const char* GetLibraryAssetTypeName() const override;

    QWidget* m_wndHSplitter;
    QSplitter* m_wndVSplitter;
    QSplitter* m_wndScriptPreviewSplitter;

    QSplitter* m_wndSplitter4;

    //CModelPreview
    CPreviewModelCtrl* m_previewCtrl;
    ReflectedPropertyControl* m_propsCtrl;
    ReflectedPropertyControl* m_objectPropsCtrl;
    QLabel* m_wndCaptionEntityClass;

    //! Selected Prototype.
    IEntity* m_entity;
    IEntitySystem* m_pEntitySystem;
    QString m_visualObject;
    QString m_PrototypeMaterial;

    bool m_bEntityPlaying;
    bool m_bShowDescription;

    BaseLibraryDialogTree* m_treeCtrl;
    DefaultBaseLibraryDialogModel* m_treeModel;
    BaseLibraryDialogProxyModel* m_proxyModel;

    // Prototype manager.
    CEntityPrototypeManager* m_pEntityManager;
};

#endif // CRYINCLUDE_EDITOR_ENTITYPROTLIBDIALOG_H
