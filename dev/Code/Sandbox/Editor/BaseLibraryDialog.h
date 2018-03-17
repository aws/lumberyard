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

#ifndef CRYINCLUDE_EDITOR_BASELIBRARYDIALOG_H
#define CRYINCLUDE_EDITOR_BASELIBRARYDIALOG_H

#pragma once


#include "DataBaseDialog.h"
#include "IDataBaseManager.h"
#include "BaseLibraryManager.h"
#include "Util/AbstractGroupProxyModel.h"

#include <QTreeView>

class CBaseLibrary;
class IToolbar;
class QComboBox;


class BaseLibraryDialogTree
    : public QTreeView
{
    Q_OBJECT
public:
    BaseLibraryDialogTree(QWidget* parent = nullptr);

signals:
    void droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport);

protected:
    bool viewportEvent(QEvent* event) override;
    void startDrag(Qt::DropActions supportedDropActions) override;
};

/** Basic interface contract class for data models used by the CBaseLibraryDialog.
        Only necessary because the CGameTokenDialog uses GameTokenModel, which inherits from QAbstractListModel.
        CPrefabDialog, CParticleDialog, and CEntityProtLibDialog all use DefaultBaseLibraryDialogModel,
        which inherits from QAbstractItemModel.
        If all 4 of those dialogs used QAbstractListModel, we could have one base class instead of
        this interface.
*/
class BaseLibraryDialogModel
{
public:
    //! Called by CBaseLibraryDialog to inform the data model of the currently active base library (which it should be displaying items from)
    virtual void setLibrary(CBaseLibrary* lib) = 0;

    //! Should return the index for the particular base library item.
    virtual QModelIndex index(CBaseLibraryItem* item) const = 0;

    //! Sets the sort recursion type. recursionType is an enum of type CBaseLibraryDialog::SortRecursionType.
    virtual void setSortRecursionType(int recursionType) = 0;
};

class DefaultBaseLibraryDialogModel
    : public QAbstractItemModel
    , public BaseLibraryDialogModel
{
    Q_OBJECT
public:
    DefaultBaseLibraryDialogModel(QObject* parent = nullptr);

    void setLibrary(CBaseLibrary* lib) override;
    void setSortRecursionType(int recursionType) override;

    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(CBaseLibraryItem* item) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    bool removeRows(int first, int count, const QModelIndex& parent = QModelIndex()) override;

    void sort(int column, Qt::SortOrder = Qt::AscendingOrder) override;

private:
    //! Selected library.
    CBaseLibrary* m_library;
    std::vector<IDataBaseItem*> m_items;

    int m_sortRecursionType;

    QVector<QPixmap> m_imageListPreFab;
    QVector<QPixmap> m_imageListParticles;
};

class BaseLibraryDialogProxyModel
    : public AbstractGroupProxyModel
{
    Q_OBJECT
public:
    BaseLibraryDialogProxyModel(QObject* parent, QAbstractItemModel* model, BaseLibraryDialogModel* libraryModel);

    using AbstractGroupProxyModel::index;
    QModelIndex index(CBaseLibraryItem* item) const;

    QVariant data(const QModelIndex& index, int role = Qt::UserRole) const override;

    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override;
    bool IsGroupIndex(const QModelIndex& sourceIndex) const;

private:
    BaseLibraryDialogModel* m_libraryModel;
};

/** Base class for all BasLibrary base dialogs.
        Provides common methods for handling library items.
*/
class CBaseLibraryDialog
    : public CDataBaseDialogPage
    , public IEditorNotifyListener
    , public IDataBaseManagerListener
    , public IUndoManagerListener
{
    Q_OBJECT
    friend class ::BaseLibraryDialogTree;
    friend class ::BaseLibraryDialogModel;
    friend class ::BaseLibraryDialogProxyModel;
    friend class ::DefaultBaseLibraryDialogModel;
public:
    enum TreeItemDataRole
    {
        BaseLibraryItemRole = Qt::UserRole + 1
    };

    CBaseLibraryDialog(UINT nID, QWidget* pParent);
    CBaseLibraryDialog(QWidget* pParent);
    ~CBaseLibraryDialog();

    //! Reload all data in dialog.
    virtual void Reload();

    // Called every frame.
    virtual void Update();

    //! This dialog is activated.
    virtual void SetActive(bool bActive);
    virtual void SelectLibrary(const QString& library, bool bForceSelect = false);
    virtual void SelectItem(CBaseLibraryItem* item, bool bForceReload = false);
    virtual bool CanSelectItem(CBaseLibraryItem* pItem);

    //! Returns menu for this dialog.
    virtual UINT GetDialogMenuID() { return 0; };

    CBaseLibrary* GetCurrentLibrary()
    {
        return m_pLibrary;
    }

    void SignalNumUndoRedo(const unsigned int& numUndo, const unsigned int& numRedo) override;

protected slots:
    virtual void OnUpdateActions();
    void OnSelChangedItemTree(const QModelIndex& current, const QModelIndex& previous);

    virtual void OnAddLibrary();
    virtual void OnRemoveLibrary();
    virtual void OnAddItem();
    virtual void OnRemoveItem();
    virtual void OnRenameItem();
    virtual void OnChangedLibrary();
    virtual void OnExportLibrary();
    virtual void OnSave();
    virtual void OnReloadLib();
    virtual void OnLoadLibrary();
    //////////////////////////////////////////////////////////////////////////
    // Copying and cloning of items.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnCopy() = 0;
    virtual void OnPaste() = 0;
    virtual void OnCut();
    virtual void OnClone();
    virtual void OnCopyPath();

protected:
    virtual void OnOK() {};
    virtual void OnCancel() {};

    virtual void LoadLibrary();

    void OnInitDialog();

    virtual void keyPressEvent(QKeyEvent* e) override;

    //////////////////////////////////////////////////////////////////////////
    // Must be overloaded in derived classes.
    //////////////////////////////////////////////////////////////////////////
    virtual CBaseLibrary* FindLibrary(const QString& libraryName);
    virtual CBaseLibrary* NewLibrary(const QString& libraryName);
    virtual void DeleteLibrary(CBaseLibrary* pLibrary);
    virtual void DeleteItem(CBaseLibraryItem* pItem);

    //////////////////////////////////////////////////////////////////////////
    // Some functions can be overridden to modify standard functionality.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitToolbar(UINT nToolbarResID);
    virtual void InitLibraryToolbar();
    virtual void InitItemToolbar();
    virtual void ReloadLibs();
    virtual void ReloadItems();
    virtual bool SetItemName(CBaseLibraryItem* item, const QString& groupName, const QString& itemName);

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener listener implementation
    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IDataBaseManagerListener listener implementation
    //////////////////////////////////////////////////////////////////////////
    void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) override;
    //////////////////////////////////////////////////////////////////////////

    bool IsExistItem(const QString& itemName) const;
    QString MakeValidName(const QString& candidateName) const;

    QString GetItemFullName(const QModelIndex& index, QString const& sep);

    // CONFETTI START: Leroy Sikkes
    // Notification to everything except self.
    void NotifyExceptSelf(EEditorNotifyEvent ev);
    // CONFETTI END

    enum SortRecursionType
    {
        SORT_RECURSION_NONE = 1,
        SORT_RECURSION_ITEM = 2,
        SORT_RECURSION_FULL = 9999
    };

    void SortItems(SortRecursionType recursionType);
    virtual void ReleasePreviewControl(){}

    //! Returns the tree control used to display this dialogs contents
    virtual QTreeView* GetTreeCtrl() = 0;

    //! Returns the AbstractGroupProxyModel that proxies retrieval of the data model. Must be set as the data source for the QTreeView returned by GetTreeCtrl()!
    virtual AbstractGroupProxyModel* GetProxyModel() = 0;

    //! Returns the actual source model, which must be wrapped by the AbstractGroupProxyModel returned by GetProxyModel.
    virtual QAbstractItemModel* GetSourceModel() = 0;

    //! Returns the interface object so that the BaseLibraryDialog can set the currently active library and get a QIndex from a BaseLibraryItem.
    virtual BaseLibraryDialogModel* GetSourceDialogModel() = 0;

    //! Returns asset type name for importing library
    virtual const char* GetLibraryAssetTypeName() const = 0;

    // Menu and toolbar actions
    QMenu* m_treeContextMenu;
    QAction* m_cutAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_pasteAction = nullptr;
    QAction* m_cloneAction = nullptr;
    QAction* m_assignAction = nullptr;
    QAction* m_selectAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;

    // Library control.
    QComboBox* m_libraryCtrl;
    // Item toolbar
    IToolbar* m_pItemToolBar;
    // Standard toolbar
    IToolbar* m_pStdToolBar;

    //CXTToolBar m_toolbar;
    // Name of currently selected library.
    QString m_selectedLib;
    QString m_selectedGroup;
    bool m_bLibsLoaded;

    //! Selected library.
    TSmartPtr<CBaseLibrary> m_pLibrary;

    //! Last selected Item. (kept here for compatibility reasons)
    // See comments on m_cpoSelectedLibraryItems for more details.
    TSmartPtr<CBaseLibraryItem> m_pCurrentItem;

    // A set containing all the currently selected items
    // (it's disabled for MOST, but not ALL cases).
    // This should be the new standard way of storing selections as
    // opposed to the former mean, it allows us to store multiple selections.
    // The migration to this new style should be done according to the needs
    // for multiple selection.
    QSet<CBaseLibraryItem*> m_cpoSelectedLibraryItems;

    //! Pointer to item manager.
    CBaseLibraryManager* m_pItemManager;

    bool m_bIgnoreSelectionChange;

    SortRecursionType m_sortRecursionType;
};

#endif // CRYINCLUDE_EDITOR_BASELIBRARYDIALOG_H
