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

#ifndef CRYINCLUDE_EDITOR_PANELTREEBROWSER_H
#define CRYINCLUDE_EDITOR_PANELTREEBROWSER_H

#pragma once
// PanelTreeBrowser.h : header file
//

#include <QFlags>
#include <QTreeView>
#include <QSortFilterProxyModel>

#include "Controls/PathTreeModel.h"
#include "Objects/ParticleEffectObject.h"


class CProceduralCreationDialog;
class CPanelPreview;

class QItemSelection;
class QModelIndex;

namespace Ui
{
    class PanelTreeBrowser;
}

class CObjectBrowserFileScanner
{
public:

    static const uint32 kDefaultAllocFileCount = 5000;

    CObjectBrowserFileScanner();
    void Scan(const QString& searchSpec);
    bool HasNewFiles();
    void GetScannedFiles(IFileUtil::FileArray& files);
    bool IsScanningForFiles();

protected:

    QString                             m_searchSpec;
    CryCriticalSection      m_lock;
    IFileUtil::FileArray    m_filesForUser;
    IFileUtil::FileArray    m_files;
    bool                                    m_bScanning;
    bool                                    m_bNewFiles;
};

/////////////////////////////////////////////////////////////////////////////
// Custom QTreeView, to support drag and drop from the panel browser's
// tree view without using custom mime-data.
class CPanelTreeBrowserTreeView
    : public QTreeView
{
    Q_OBJECT

public:

    CPanelTreeBrowserTreeView(QWidget* p)
        : QTreeView(p) {}

    void endDrop() { emit dropFinished(selectedIndexes()); }

signals:
    void dropFinished(const QModelIndexList& indexes);

protected:

    void startDrag(Qt::DropActions supportedActions) override;
};

/////////////////////////////////////////////////////////////////////////////
// CPanelTreeBrowser dialog
class SANDBOX_API CPanelTreeBrowser
    : public QWidget
    , public IEditorNotifyListener
    , public IEntityClassRegistryListener
{
    Q_OBJECT

    // Construction
public:

    typedef Functor1<QString>                                   TSelectCallback;

    // Dialog Data

    enum EFlag
    {
        NO_DRAGDROP = 0x001,            // Disable drag&drop of items to view,
        NO_PREVIEW = 0x002,
        SELECT_ONCLICK = 0x004, // Select callback when item is selected in tree view.
    };
    Q_DECLARE_FLAGS(EFlags, EFlag)

    enum class DialogType
    {
        DEFAULT_DIALOG = 0,
        ME_SELECTION_DIALOG = 1, // Custom Modular Editor dialog
        ME_REPLACE_DIALOG = 2, // Custom Modular Editor dialog
    };

    CPanelTreeBrowser(const TSelectCallback& cb, const QString& searchSpec, const EFlags& flags = EFlags(), bool noExt = true, QWidget* pParent = nullptr);
    virtual ~CPanelTreeBrowser();

    void    SetSelectCallback(TSelectCallback cb) { m_selectCallback = cb; };
    void    SetDragAndDropCallback(TSelectCallback cb) { m_dragAndDropCallback = cb; };
    void    SelectFile(const QString& filename);
    void    AddPreviewPanel();
    void    SetDialogType(DialogType dialogTypeFlag) { m_dialogType = dialogTypeFlag; };
    void    Refresh(bool bReloadFiles);

    // IEntityClassRegistryListener
    virtual void OnEntityClassRegistryEvent(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass);
    // ~IEntityClassRegistryListener

    // Implementation
protected:
    class SortFilterProxyModel
        : public QSortFilterProxyModel
    {
    public:
        SortFilterProxyModel(QObject* parent = nullptr);
        virtual ~SortFilterProxyModel();
    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    };

    typedef std::map<QString, PathTreeModel*> TFileHistory;

    void OnInitDialog();
    void OnCustomBtnClick();
    void OnFilterChange();

    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

    void DropFinished(const QModelIndexList& indexes);
    void AcceptFile(const QString& file, bool bDragAndDrop);

    void FillEntityScripts(bool bReload);
    void FillDBLibrary(EDataBaseItemType dbType);
    void FillParticleEffects();
    void FillPrefabs();
    void FillTreeWithFiles(IFileUtil::FileArray& files);
    void UpdateFileCountLabel();
    void SortTree();
    void LoadFilesFromScanning();
    void OnReload();
    void OnNewEntity();

protected slots:
    void OnSelectionChanged();
    void OnDblclkBrowserTree(const QModelIndex& index);
    void OnObjectEvent(CBaseObject* object, int event);
    void OnSelectFile(QString filename); // for queued execution
    //////////////////////////////////////////////////////////////////////////

private:
    int AddExtractedParticles(CParticleEffectObject::CFastParticleParser& parser);

    QScopedPointer<Ui::PanelTreeBrowser> m_ui;

    PathTreeModel* m_model;
    SortFilterProxyModel* m_proxyModel;

    EFlags m_flags;
    DialogType m_dialogType;

    CObjectBrowserFileScanner   m_fileScanner;
    QString m_searchSpec;

    TSelectCallback m_selectCallback;
    TSelectCallback m_dragAndDropCallback;

    CPanelPreview* m_panelPreview;
    int m_panelPreviewId;

    bool m_bSelectOnClick;
    QString m_addingObject;

    static TFileHistory         sm_fileHistory;

    bool m_bNoExtension;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CPanelTreeBrowser::EFlags)

#endif // CRYINCLUDE_EDITOR_PANELTREEBROWSER_H
