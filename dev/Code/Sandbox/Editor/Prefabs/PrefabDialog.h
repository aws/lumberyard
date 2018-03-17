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

#ifndef CRYINCLUDE_EDITOR_PREFABS_PREFABDIALOG_H
#define CRYINCLUDE_EDITOR_PREFABS_PREFABDIALOG_H
#pragma once

#include "BaseLibraryDialog.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>
#include "Controls/PreviewModelCtrl.h"

class CPrefabItem;
class CPrefabManager;
class QSplitter;

/** Dialog which hosts entity prototype library.
*/
class CPrefabDialog
    : public CBaseLibraryDialog
{
    Q_OBJECT

public:
    CPrefabDialog(QWidget* pParent);
    ~CPrefabDialog();

    // Called every frame.
    void Update() override;

    virtual UINT GetDialogMenuID() override;

    CPrefabItem* GetPrefabFromSelection();
    void OnObjectEvent(CBaseObject* pObject, int nEvent);

protected:
    void OnInitDialog();
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport);

    void OnAddItem();
    void OnNotifyMtlTreeRClick(const QPoint& point);
    void OnUpdateActions() override;

    void OnCopy();
    void OnPaste();
    void OnSelectAssignedObjects();
    void OnAssignToSelection();
    void OnGetFromSelection();
    void OnMakeFromSelection();

    //! Update objects to which this prefab is assigned.
    void UpdatePrefabObjects(CPrefabItem* pPrefab);

    //////////////////////////////////////////////////////////////////////////
    // Some functions can be overriden to modify standart functionality.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitToolbar(UINT nToolbarResID) override;
    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false) override;

    virtual QTreeView* GetTreeCtrl() override;
    virtual AbstractGroupProxyModel* GetProxyModel() override;
    virtual QAbstractItemModel* GetSourceModel() override;
    virtual BaseLibraryDialogModel* GetSourceDialogModel() override;

    const char* GetLibraryAssetTypeName() const override;

    //////////////////////////////////////////////////////////////////////////
    CPrefabItem* GetSelectedPrefab();
    void OnUpdateProperties(IVariable* var);

    void DropToItem(const QModelIndex& item, const QModelIndex& dragged, CPrefabItem* pMtl);

    enum EDrawType
    {
        DRAW_BOX,
        DRAW_SPHERE,
        DRAW_TEAPOT,
        DRAW_SELECTION,
    };

    QSplitter* m_wndVSplitter;

    QAction* m_itemAssignAction = nullptr;
    QAction* m_itemGetPropertiesAction = nullptr;

    ReflectedPropertyControl* m_propsCtrl;
    QVector<QPixmap> m_imageList;

    // Manager.
    CPrefabManager* m_pPrefabManager;

    EDrawType m_drawType;

    BaseLibraryDialogTree* m_treeCtrl;
    DefaultBaseLibraryDialogModel* m_treeModel;
    BaseLibraryDialogProxyModel* m_proxyModel;

    //! Prompt for a name of a prefab.  Includes retry for blank or duplicate names
    bool PromptForPrefabName(QString& inOutGroupName, QString& inOutItemName);
};

#endif // CRYINCLUDE_EDITOR_PREFABS_PREFABDIALOG_H
