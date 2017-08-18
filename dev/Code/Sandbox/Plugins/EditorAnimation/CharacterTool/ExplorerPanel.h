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

#include <memory>
#include <QWidget>
#include <QItemSelection>
#include <QTreeView>
#include <QMetaObject>
#include <Serialization/IArchive.h>
#include "../EditorCommon/QPropertyTree/QPropertyTree.h"

class QToolButton;
class QLineEdit;
class QPushButton;
class QString;
class QItemSelection;
class QModelIndex;
class QAbstractItemModel;
class QMenu;
class QDockWidget;

namespace Serialization {
    class IArchive;
}

namespace CharacterTool {
    class TreeViewWithClickOnSelectedItem
        : public QTreeView
    {
        Q_OBJECT
    public:
        TreeViewWithClickOnSelectedItem();

        void setModel(QAbstractItemModel* model);
        void mousePressEvent(QMouseEvent* ev) override;

    signals:
        void SignalClickOnSelectedItem(const QModelIndex& index);
    private:
        QMetaObject::Connection m_selectionHandler;
        bool m_treeSelectionChanged;
    };

    class CharacterToolForm;
    class Explorer;
    class ExplorerModel;
    class ExplorerActionHandler;
    struct System;
    class ExplorerFilterProxyModel;
    struct ExplorerAction;
    struct ExplorerEntry;

    struct FilterOptions
    {
        bool inPak;
        bool onDisk;
        bool onlyNew;
        bool withAudio;
        bool withoutAudio;

        FilterOptions()
            : inPak(true)
            , onDisk(true)
            , onlyNew(false)
            , withAudio(true)
            , withoutAudio(true)
        {
        }

        bool operator!=(const FilterOptions& rhs) const
        {
            return inPak != rhs.inPak ||
                   onDisk != rhs.onDisk ||
                   onlyNew != rhs.onlyNew ||
                   withAudio != rhs.withAudio ||
                   withoutAudio != rhs.withoutAudio;
        }

        void Serialize(Serialization::IArchive& ar)
        {
            if (ar.OpenBlock("files", "Files:"))
            {
                ar(inPak, "inPak", "^In Pak");
                ar(onDisk, "onDisk", "^On Disk");
                ar(onlyNew, "onlyNew", "^Only New");
                ar.CloseBlock();
            }

            if (ar.OpenBlock("audioEvents", "Audio Events:"))
            {
                ar(withAudio, "withAudio", "^With");
                ar(withoutAudio, "withoutAudio", "^Without");
                ar.CloseBlock();
            }
        }
    };

    class ExplorerPanel
        : public QWidget
    {
        Q_OBJECT
    public:
        ExplorerPanel(QWidget* parent, System* system, QMainWindow* mainWindow);
        ~ExplorerPanel();
        void SetDockWidget(QDockWidget* dockWidget);
        void SetRootIndex(int rootIndex);
        int RootIndex() const{ return m_explorerRootIndex; }

        void Serialize(Serialization::IArchive& ar);
        QSize sizeHint() const override { return QSize(240, 400); }
    public slots:
        void OnExplorerSelectionChanged();
        void OnFilterTextChanged(const QString& str);
        void OnTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDeselectEntry(ExplorerEntry* deselectedEntry);
        void ExecuteEntryAction(ExplorerEntry* deselectedEntry, const string actionName);

        void OnClickedSelectedItem(const QModelIndex& index);
        void OnHeaderContextMenu(const QPoint& pos);
        void OnHeaderColumnToggle();
        void OnContextMenu(const QPoint& pos);
        void OnMenuCopyName();
        void OnMenuCopyPath();
        void OnMenuPasteSelection();
        void OnActivated(const QModelIndex& index);
        void OnExplorerAction(const ExplorerAction& action);
        void OnEntryImported(ExplorerEntry* entry, ExplorerEntry* oldEntry);
        void OnEntryLoaded(ExplorerEntry* entry);
        void OnRootButtonPressed();
        void OnRootSelected(bool);
        void OnExplorerEndReset();
        void OnExplorerBeginBatchChange(int subtree);
        void OnExplorerEndBatchChange(int subtree);
        void OnExplorerEntryModified(ExplorerEntryModifyEvent& ev);
        void OnCharacterLoaded();
        void OnRefreshFilter();
        void OnFilterButtonToggled(bool filterMode);
        void OnFilterOptionsChanged();
    protected:
        bool eventFilter(QObject* sender, QEvent* ev) override;
    private:
        void ExecuteExplorerAction(const ExplorerAction& action);
        void SetTreeViewModel(QAbstractItemModel* model);
        void FillAnimations();
        void UpdateRootMenu();
        void ExpandTree();

        FilterOptions m_filterOptions;
        QDockWidget* m_dockWidget;
        TreeViewWithClickOnSelectedItem* m_treeView;
        System* m_system;
        ExplorerModel* m_model;
        ExplorerFilterProxyModel* m_filterModel;
        QLineEdit* m_filterEdit;
        QPushButton* m_rootButton;
        QToolButton* m_filterButton;
        QMenu* m_rootMenu;

        std::vector<QAction*> m_rootMenuActions;
        int m_explorerRootIndex;
        bool m_ignoreTreeSelectionChange;
        bool m_filterMode;
        unsigned int m_batchChangesRunning;
        std::vector<std::unique_ptr<ExplorerActionHandler> > m_explorerActionHandlers;
        QPropertyTree* m_filterOptionsTree;
        CharacterToolForm* m_mainWindow;
        ExplorerEntries m_selectedEntries;
    };
}
