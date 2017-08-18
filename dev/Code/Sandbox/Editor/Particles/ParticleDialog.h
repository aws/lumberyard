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

#ifndef CRYINCLUDE_EDITOR_PARTICLES_PARTICLEDIALOG_H
#define CRYINCLUDE_EDITOR_PARTICLES_PARTICLEDIALOG_H
#pragma once

#include "BaseLibraryDialog.h"

class CParticleItem;
class CParticleManager;
class CParticleUIDefinition;
class CPreviewModelCtrl;

namespace Ui
{
    class ParticleDialog;
}

/** Dialog which hosts entity prototype library.
*/
class CParticleDialog
    : public CBaseLibraryDialog
{
    Q_OBJECT
public:
    CParticleDialog(QWidget* pParent);
    ~CParticleDialog();

    // Called every frame.
    void Update();

    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false);
    virtual UINT GetDialogMenuID();

    static CParticleDialog* s_poCurrentInstance;
    static CParticleDialog* GetCurrentInstance();
    CParticleItem* GetSelectedParticle();

    void OnAssignToSelection();
    void OnGetFromSelection();
    void OnResetParticles();
    void OnReloadParticles();
    void OnEnable() { Enable(false); }
    void OnEnableAll() { Enable(true); }
    void OnExpandAll();
    void OnCollapseAll();

    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void OnUpdateActions() override;

    void ExpandCollapseAll(bool expand = true);
    void OnInitDialog();

    void OnDestroy();
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport);

    void showEvent(QShowEvent* event) override;

    void OnAddItem() { AddItem(false); }
    void OnUpdateObjectSelected();
    void OnUpdateAssignToSelection();
    void keyPressEvent(QKeyEvent* event) override;
    void OnNotifyTreeRClick(const QPoint& point);
    void OnNotifyTreeLClick(const QModelIndex& item);
    void OnTvnSelchangedTree(const QModelIndex& current, const QModelIndex& previous);
    void OnPick();
    void OnUpdatePick();
    void OnCopy();
    void OnPaste();
    void OnClone();
    void OnSelectAssignedObjects();

    void Enable(bool bAll);
    bool AddItem(bool bFromParent);

    //////////////////////////////////////////////////////////////////////////
    // Some functions can be overriden to modify standard functionality.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitToolbar(UINT nToolbarResID);
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    virtual QTreeView* GetTreeCtrl() override;
    virtual AbstractGroupProxyModel* GetProxyModel() override;
    virtual QAbstractItemModel* GetSourceModel() override;
    virtual BaseLibraryDialogModel* GetSourceDialogModel() override;

    //////////////////////////////////////////////////////////////////////////
    void OnUpdateProperties(IVariable* var);

    bool AssignToEntity(CParticleItem* pItem, CBaseObject* pObject, Vec3 const* pPos = NULL);
    void AssignToSelection();

    CBaseObject* CreateParticleEntity(CParticleItem* pItem, Vec3 const& pos, CBaseObject* pParent = NULL);

    void ReleasePreviewControl();
    void DropToItem(const QModelIndex& item, const QModelIndex& dragged, CParticleItem* pItem);
    class CEntityObject* GetItemFromEntity();

    const char* GetLibraryAssetTypeName() const override;

    QVector<QPixmap> m_imageList;

    bool m_bForceReloadPropsCtrl;
    bool m_bAutoAssignToSelection;
    bool m_bRealtimePreviewUpdate;
    int m_iRefreshDelay;

    // Particle manager.
    IEditorParticleManager* m_pPartManager;

    CVarBlockPtr m_vars;
    CParticleUIDefinition* pParticleUI;

    TSmartPtr<CParticleItem> m_pDraggedItem;
    bool m_bBlockParticleUpdate;

    BaseLibraryDialogTree* m_treeCtrl;
    DefaultBaseLibraryDialogModel* m_treeModel;
    BaseLibraryDialogProxyModel* m_proxyModel;

    QScopedPointer<Ui::ParticleDialog> m_ui;
};

#endif // CRYINCLUDE_EDITOR_PARTICLES_PARTICLEDIALOG_H
