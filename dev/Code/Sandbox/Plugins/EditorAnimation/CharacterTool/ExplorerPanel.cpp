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

#include "pch.h"
#include "Strings.h"
#include <QApplication>
#include <QTreeView>
#include <QBoxLayout>
#include <QClipboard>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QDockWidget>
#include <QHeaderView>
#include <QAbstractItemModel>
#include <QEvent>
#include <QFocusEvent>
#include <ICryAnimation.h>
#include "CharacterDocument.h"
#include "Explorer.h"
#include "ExplorerModel.h"
#include "ExplorerPanel.h"
#include "Expected.h"
#include "Serialization.h"
#include "CharacterToolSystem.h"
#include "CharacterToolForm.h"
#include "SceneContent.h"
#include "AnimationList.h"
#include "PlaybackPanel.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

namespace CharacterTool {
    TreeViewWithClickOnSelectedItem::TreeViewWithClickOnSelectedItem()
        : m_treeSelectionChanged(false)
    {
    }

    void TreeViewWithClickOnSelectedItem::setModel(QAbstractItemModel* model)
    {
        if (m_selectionHandler)
        {
            QObject::disconnect(m_selectionHandler);
            m_selectionHandler = QMetaObject::Connection();
        }

        QTreeView::setModel(model);

        m_selectionHandler = connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
                [this](const QItemSelection&, const QItemSelection&)
                {
                    m_treeSelectionChanged = true;
                }
                );
    }

    void TreeViewWithClickOnSelectedItem::mousePressEvent(QMouseEvent* ev)
    {
        if (ev->button() == Qt::LeftButton)
        {
            m_treeSelectionChanged = false;
            QTreeView::mousePressEvent(ev);
            if (!m_treeSelectionChanged)
            {
                QModelIndex currentIndex = selectionModel()->currentIndex();
                if (currentIndex.isValid())
                {
                    SignalClickOnSelectedItem(currentIndex);
                }
            }
        }
        else
        {
            QTreeView::mousePressEvent(ev);
        }
    }

    // ---------------------------------------------------------------------------


    static QModelIndex FindIndexByEntry(QTreeView* treeView, QSortFilterProxyModel* filterModel, ExplorerModel* model, ExplorerEntry* entry)
    {
        if (model->GetRootIndex() > 0 && model->GetActiveRoot()->subtree != entry->subtree)
        {
            return QModelIndex();
        }
        QModelIndex sourceIndex = model->ModelIndexFromEntry(entry, 0);
        QModelIndex index;
        if (treeView->model() == filterModel)
        {
            index = filterModel->mapFromSource(sourceIndex);
        }
        else
        {
            index = sourceIndex;
        }
        return index;
    }


    class ExplorerFilterProxyModel
        : public QSortFilterProxyModel
    {
    public:
        ExplorerFilterProxyModel(QObject* parent, int columnAudio, int columnPak, AnimationList* animationList)
            : QSortFilterProxyModel(parent)
            , m_columnAudio(columnAudio)
            , m_columnPak(columnPak)
            , m_animationList(animationList)
        {
        }

        void setFilterString(const QString& filter)
        {
            m_filter = filter;
            m_filterParts = m_filter.split(' ', QString::SkipEmptyParts);
            m_acceptCache.clear();
        }

        void setFilterOptions(const FilterOptions& options)
        {
            if (m_filterOptions != options)
            {
                m_filterOptions = options;
                m_acceptCache.clear();
                invalidate();
            }
        }

        void invalidate()
        {
            QSortFilterProxyModel::invalidate();
            m_acceptCache.clear();
        }

        void setFilterWildcard(const QString& pattern)
        {
            m_acceptCache.clear();
            QSortFilterProxyModel::setFilterWildcard(pattern);
        }

        bool matchFilter(int source_row, const QModelIndex& source_parent) const
        {
            QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
            ExplorerEntry* entry = ExplorerModel::GetEntry(index);
            if (entry && entry->type == ENTRY_LOADING)
            {
                return true;
            }
            QVariant data = sourceModel()->data(index, Qt::DisplayRole);
            QString label(data.toString());
            QString path(QString::fromLocal8Bit(entry->path.c_str()));
            if (label.isEmpty() && path.isEmpty())
            {
                return m_filterParts.empty();
            }
            for (size_t i = 0; i < m_filterParts.size(); ++i)
            {
                if (!label.contains(m_filterParts[i], Qt::CaseInsensitive) && !path.contains(m_filterParts[i], Qt::CaseInsensitive))
                {
                    return false;
                }
            }
            if (!m_filterOptions.inPak || !m_filterOptions.onDisk)
            {
                if (!m_filterOptions.inPak && !m_filterOptions.onDisk)
                {
                    return false;
                }
                int pakState = entry->GetColumnValue(m_columnPak);
                if (m_filterOptions.inPak && (pakState & PAK_STATE_PAK) == 0)
                {
                    return false;
                }
                if (m_filterOptions.onDisk && (pakState & PAK_STATE_LOOSE_FILES) == 0)
                {
                    return false;
                }
            }
            if (!m_filterOptions.withAudio || !m_filterOptions.withoutAudio)
            {
                int audio = entry->GetColumnValue(m_columnAudio);
                if (!m_filterOptions.withAudio && audio == ENTRY_AUDIO_PRESENT)
                {
                    return false;
                }
                if (!m_filterOptions.withoutAudio && audio == ENTRY_AUDIO_NONE)
                {
                    return false;
                }
            }
            if (m_filterOptions.onlyNew)
            {
                if (entry->type != ENTRY_ANIMATION)
                {
                    return false;
                }
                if (!m_animationList->IsNewAnimation(entry->id))
                {
                    return false;
                }
            }
            return true;
        }

        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
        {
            if (left.column() == right.column())
            {
                QVariant valueLeft = sourceModel()->data(left, Qt::UserRole);
                QVariant valueRight = sourceModel()->data(right, Qt::UserRole);
                if (valueLeft.type() == valueRight.type())
                {
                    switch (valueLeft.type())
                    {
                    case QVariant::Int:
                        return valueLeft.toInt() < valueRight.toInt();
                    case QVariant::String:
                        return valueLeft.toString() < valueRight.toString();
                    case QVariant::Double:
                        return valueLeft.toDouble() < valueRight.toDouble();
                    default:
                        break; // fall back to default comparison
                    }
                }
            }

            return QSortFilterProxyModel::lessThan(left, right);
        }

        bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
        {
            if (matchFilter(source_row, source_parent))
            {
                return true;
            }

            if (hasAcceptedChildrenCached(source_row, source_parent))
            {
                return true;
            }

            return false;
        }

        bool hasAcceptedChildrenCached(int source_row, const QModelIndex& source_parent) const
        {
            std::pair<QModelIndex, int> indexId = std::make_pair(source_parent, source_row);
            TAcceptCache::iterator it = m_acceptCache.find(indexId);
            if (it == m_acceptCache.end())
            {
                bool result = hasAcceptedChildren(source_row, source_parent);
                m_acceptCache[indexId] = result;
                return result;
            }
            else
            {
                return it->second;
            }
        }

        bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const
        {
            QModelIndex item = sourceModel()->index(source_row, 0, source_parent);
            if (!item.isValid())
            {
                return false;
            }

            int childCount = item.model()->rowCount(item);
            if (childCount == 0)
            {
                return false;
            }

            for (int i = 0; i < childCount; ++i)
            {
                if (filterAcceptsRow(i, item))
                {
                    return true;
                }
            }

            return false;
        }

    private:
        QString m_filter;
        QStringList m_filterParts;
        FilterOptions m_filterOptions;
        typedef std::map<std::pair<QModelIndex, int>, bool> TAcceptCache;
        mutable TAcceptCache m_acceptCache;
        AnimationList* m_animationList;
        int m_columnPak;
        int m_columnAudio;
    };

    // ---------------------------------------------------------------------------

    static void ExpandFirstLevel(QTreeView* treeView, const QModelIndex& root)
    {
        int numChildren = treeView->model()->rowCount(root);
        for (int i = 0; i < numChildren; ++i)
        {
            QModelIndex index = treeView->model()->index(i, 0, root);
            treeView->expand(index);
        }
    }

    static void ExpandChildren(QTreeView* treeView, const QModelIndex& root)
    {
        treeView->expand(root);
        int numChildren = treeView->model()->rowCount(root);
        for (int i = 0; i < numChildren; ++i)
        {
            QModelIndex index = treeView->model()->index(i, 0, root);
            if (!treeView->isExpanded(index))
            {
                ExpandChildren(treeView, index);
            }
        }
    }

    ExplorerPanel::ExplorerPanel(QWidget* parent, System* system, QMainWindow* mainWindow)
        : QWidget(parent)
        , m_system(system)
        , m_filterModel(0)
        , m_explorerRootIndex(0)
        , m_dockWidget(0)
        , m_model(0)
        , m_filterEdit(0)
        , m_ignoreTreeSelectionChange(false)
        , m_filterMode(false)
        , m_mainWindow(static_cast<CharacterToolForm*>(mainWindow))
        , m_batchChangesRunning(0)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->setMargin(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        setLayout(layout);
        {
            QBoxLayout* hlayout = new QBoxLayout(QBoxLayout::LeftToRight);
            hlayout->setSpacing(4);

            m_filterEdit = new QLineEdit();
            EXPECTED(connect(m_filterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnFilterTextChanged(const QString&))));
            hlayout->addWidget(m_filterEdit, 1);

            m_rootButton = new QPushButton("");
            EXPECTED(connect(m_rootButton, SIGNAL(pressed()), this, SLOT(OnRootButtonPressed())));
            m_rootMenu = new QMenu(this);

            m_rootButton->setMenu(m_rootMenu);
            UpdateRootMenu();
            hlayout->addWidget(m_rootButton, 0);

            m_filterButton = new QToolButton();
            m_filterButton->setIcon(QIcon("Editor/Icons/animation/filter_16.png"));
            m_filterButton->setCheckable(true);
            m_filterButton->setChecked(m_filterMode);
            m_filterButton->setAutoRaise(true);
            m_filterButton->setToolTip("Enable Filter Options");
            EXPECTED(connect(m_filterButton, SIGNAL(toggled(bool)), this, SLOT(OnFilterButtonToggled(bool))));
            hlayout->addWidget(m_filterButton, 0);

            layout->addLayout(hlayout);
        }

        m_filterOptionsTree = new QPropertyTree(this);
        m_filterOptionsTree->setCompact(true);
        m_filterOptionsTree->setSizeToContent(true);
        m_filterOptionsTree->setHideSelection(true);
        m_filterOptionsTree->attach(Serialization::SStruct(m_filterOptions));
        EXPECTED(connect(m_filterOptionsTree, SIGNAL(signalChanged()), this, SLOT(OnFilterOptionsChanged())));
        layout->addWidget(m_filterOptionsTree);
        m_filterOptionsTree->hide();

        m_treeView = new TreeViewWithClickOnSelectedItem();

        {
            m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
            m_treeView->setIndentation(12);
            m_treeView->setSortingEnabled(true);
            m_treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
            m_treeView->installEventFilter(this);
            m_treeView->setDragEnabled(true);
            m_treeView->setDragDropMode(QAbstractItemView::DragDropMode::DragOnly);
            EXPECTED(connect(m_treeView->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnHeaderContextMenu(const QPoint&))));
            EXPECTED(connect(m_treeView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnContextMenu(const QPoint&))));
            EXPECTED(connect(m_treeView, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnActivated(const QModelIndex&))));
            EXPECTED(connect(m_treeView, &TreeViewWithClickOnSelectedItem::SignalClickOnSelectedItem, this, [this](const QModelIndex& i) { OnClickedSelectedItem(i); }));

            m_model = new ExplorerModel(m_system->explorer.get(), this);
            m_model->SetRootByIndex(m_explorerRootIndex);

            layout->addWidget(m_treeView);

            OnFilterTextChanged(QString());
        }

        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalExplorerSelectionChanged()), this, SLOT(OnExplorerSelectionChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalCharacterLoaded()), this, SLOT(OnCharacterLoaded())));

        EXPECTED(connect(m_system->explorer.get(), SIGNAL(SignalEntryImported(ExplorerEntry*, ExplorerEntry*)), this, SLOT(OnEntryImported(ExplorerEntry*, ExplorerEntry*))));
        EXPECTED(connect(m_system->explorer.get(), SIGNAL(SignalEntryLoaded(ExplorerEntry*)), this, SLOT(OnEntryLoaded(ExplorerEntry*))));
        EXPECTED(connect(m_system->explorer.get(), SIGNAL(SignalRefreshFilter()), this, SLOT(OnRefreshFilter())));
        EXPECTED(connect(m_system->explorer.get(), &Explorer::SignalBeginBatchChange, this, &ExplorerPanel::OnExplorerBeginBatchChange));
        EXPECTED(connect(m_system->explorer.get(), &Explorer::SignalEndBatchChange, this, &ExplorerPanel::OnExplorerEndBatchChange));
        EXPECTED(connect(m_system->explorer.get(), &Explorer::SignalEntryModified, this, &ExplorerPanel::OnExplorerEntryModified));
    }

    ExplorerPanel::~ExplorerPanel()
    {
        m_treeView->setModel(0);
    }

    void ExplorerPanel::SetDockWidget(QDockWidget* dockWidget)
    {
        m_dockWidget = dockWidget;
        UpdateRootMenu();
    }

    void ExplorerPanel::SetRootIndex(int rootIndex)
    {
        m_explorerRootIndex = rootIndex;
        UpdateRootMenu();
    }

    void ExplorerPanel::OnExplorerSelectionChanged()
    {
        ExplorerEntries selectedEntries;
        m_system->document->GetSelectedExplorerEntries(&selectedEntries);

        QModelIndex singleIndex;
        QItemSelection newSelection;
        for (size_t i = 0; i < selectedEntries.size(); ++i)
        {
            QModelIndex index = FindIndexByEntry(m_treeView, m_filterModel, m_model, selectedEntries[i]);
            if (!index.isValid())
            {
                continue;
            }

            if (singleIndex.isValid())
            {
                singleIndex = QModelIndex();
            }
            else
            {
                singleIndex = index;
            }
            newSelection.append(QItemSelectionRange(index));
        }

        m_ignoreTreeSelectionChange = true;
        m_treeView->selectionModel()->select(newSelection, QItemSelectionModel::ClearAndSelect);
        if (singleIndex.isValid())
        {
            m_treeView->scrollTo(singleIndex);
        }
        m_ignoreTreeSelectionChange = false;
    }

    void ExplorerPanel::OnRootButtonPressed()
    {
        UpdateRootMenu();
    }

    void ExplorerPanel::OnFilterButtonToggled(bool filterMode)
    {
        m_filterMode = filterMode;
        OnFilterOptionsChanged();
        m_filterOptionsTree->setVisible(m_filterMode);
    }

    void ExplorerPanel::OnFilterOptionsChanged()
    {
        m_filterModel->setFilterOptions(m_filterMode ? m_filterOptions : FilterOptions());
        ExpandTree();
    }

    void ExplorerPanel::ExpandTree()
    {
        if (!m_filterEdit->text().isEmpty() || m_filterOptions.onlyNew)
        {
            m_treeView->expandAll();
        }
        else
        {
            ExpandFirstLevel(m_treeView, m_treeView->rootIndex());
        }
    }

    void ExplorerPanel::OnRootSelected(bool)
    {
        int newRoot = 0;
        QAction* action = qobject_cast<QAction*>(sender());
        if (action)
        {
            newRoot = action->data().toInt();
        }

        m_explorerRootIndex = newRoot;
        m_model->SetRootByIndex(newRoot);
        if (m_filterModel)
        {
            m_filterModel->invalidate();
        }
        ExpandTree();
        UpdateRootMenu();
    }

    bool ActionLessByName(const QAction* a, const QAction* b)
    {
        return a->text() < b->text();
    }

    void ExplorerPanel::UpdateRootMenu()
    {
        m_rootMenu->clear();
        m_rootMenuActions.clear();
        const char* everythingText = "All Types";
        {
            QAction* action = new QAction(everythingText, m_rootMenu);
            action->setCheckable(true);
            action->setChecked(m_explorerRootIndex == 0);
            action->setData(QVariant(0));
            EXPECTED(connect(action, SIGNAL(triggered(bool)), this, SLOT(OnRootSelected(bool))));
            m_rootMenuActions.push_back(action);
        }

        ExplorerEntry* root = m_system->explorer->GetRoot();
        int numRootItems = root->children.size();
        for (int i = 0; i < numRootItems; ++i)
        {
            QString text = QString(root->children[i]->name.c_str()).remove(QRegExp(" \\(.*"));
            QAction* action = new QAction(text, m_rootMenu);
            EXPECTED(connect(action, SIGNAL(triggered(bool)), this, SLOT(OnRootSelected(bool))));
            action->setCheckable(true);
            action->setChecked(i + 1 == m_explorerRootIndex);
            action->setData(QVariant(i + 1));
            m_rootMenuActions.push_back(action);
        }

        m_rootMenu->addAction(m_rootMenuActions[0]);
        m_rootMenu->addSeparator();

        std::sort(m_rootMenuActions.begin() + 1, m_rootMenuActions.end(), &ActionLessByName);

        for (size_t i = 1; i < m_rootMenuActions.size(); ++i)
        {
            m_rootMenu->addAction(m_rootMenuActions[i]);
        }

        QString buttonText = everythingText;
        if (m_explorerRootIndex > 0 && m_explorerRootIndex <= root->children.size())
        {
            buttonText = QString(root->children[m_explorerRootIndex - 1]->name.c_str()).remove(QRegExp(" \\(.*"));
        }

        m_rootButton->setText(buttonText);

        if (m_dockWidget)
        {
            if (m_explorerRootIndex == 0)
            {
                m_dockWidget->setWindowTitle("Assets");
            }
            else
            {
                m_dockWidget->setWindowTitle(QString("Assets: ") + buttonText);
            }
        }
    }


    void ExplorerPanel::OnFilterTextChanged(const QString& str)
    {
        bool modelChanged = m_treeView->model() != m_filterModel || m_filterModel == 0;
        if (modelChanged)
        {
            if (!m_filterModel)
            {
                m_filterModel = new ExplorerFilterProxyModel(this, m_system->explorerColumnAudio, m_system->explorerColumnPak, m_system->animationList.get());
                m_filterModel->setSourceModel(m_model);
                m_filterModel->setDynamicSortFilter(false);
            }
            m_filterModel->setFilterString(str);
            SetTreeViewModel(m_filterModel);
        }
        else
        {
            m_filterModel->setFilterString(str);
            m_filterModel->invalidate();
        }
        ExpandTree();
    }

    void ExplorerPanel::OnTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (m_ignoreTreeSelectionChange)
        {
            return;
        }

        ExplorerEntries selectedEntries;

        QItemSelection selection = m_treeView->selectionModel()->selection();
        QItemSelection selectedOriginal;
        if (m_treeView->model() == m_filterModel)
        {
            selectedOriginal = m_filterModel->mapSelectionToSource(selection);
        }
        else
        {
            selectedOriginal = selection;
        }

        const QModelIndexList& indices = selectedOriginal.indexes();
        size_t numIndices = indices.size();
        for (size_t i = 0; i < numIndices; ++i)
        {
            const QModelIndex& index = indices[i];
            ExplorerEntry* entry = (ExplorerEntry*)index.internalPointer();
            selectedEntries.push_back(entry);
        }

        // as we may select cells in multiple columns we may have duplicated entries at this point
        std::sort(selectedEntries.begin(), selectedEntries.end());
        selectedEntries.erase(std::unique(selectedEntries.begin(), selectedEntries.end()), selectedEntries.end());

        // Perform deselect action.
        const char* loadedCharacterFileName = m_system->document->LoadedCharacterFilename();
        if (loadedCharacterFileName[0] != '\0')
        {
            for (ExplorerEntry* deselectedEntry : m_selectedEntries)
            {
                if (deselectedEntry->modified)
                {
                    bool reselect = false;
                    for (ExplorerEntry* selectedEntry : selectedEntries)
                    {
                        if (selectedEntry == deselectedEntry)
                        {
                            reselect = true;
                            break;
                        }
                    }

                    if (!reselect)
                    {
                        ExplorerPanel::OnDeselectEntry(deselectedEntry);
                    }
                }
            }
        }
        m_selectedEntries = selectedEntries;

        m_system->document->SetSelectedExplorerEntries(selectedEntries, 0);
    }

    void ExplorerPanel::OnDeselectEntry(ExplorerEntry* deselectedEntry)
    {
        if (deselectedEntry && deselectedEntry->type == ENTRY_COMPRESSION_PRESETS && deselectedEntry->name == "Skeleton List")
        {
            ExecuteEntryAction(deselectedEntry, "Save");
        }
    }

    void ExplorerPanel::ExecuteEntryAction(ExplorerEntry* entry, const string actionName)
    {
        ExplorerActions actions;
        m_system->explorer->GetActionsForEntry(&actions, entry);
        for (ExplorerAction action : actions)
        {
            if (actionName == action.text)
            {
                ExplorerEntries entries;
                entries.push_back(entry);
                m_mainWindow->ExecuteExplorerAction(action, entries);
                break;
            }
        }
    }

    void ExplorerPanel::OnHeaderContextMenu(const QPoint& pos)
    {
        QMenu menu;

        Explorer* explorer = m_system->explorer.get();

        int columnCount = explorer->GetColumnCount();
        for (int i = 1; i < columnCount; ++i)
        {
            QAction* action = menu.addAction(explorer->GetColumnLabel(i), this, SLOT(OnHeaderColumnToggle()));
            action->setCheckable(true);
            action->setChecked(!m_treeView->header()->isSectionHidden(i));
            action->setData(QVariant(i));
        }

        menu.exec(QCursor::pos());
    }

    void ExplorerPanel::OnHeaderColumnToggle()
    {
        QObject* senderPtr = QObject::sender();

        if (QAction* action = qobject_cast<QAction*>(senderPtr))
        {
            int column = action->data().toInt();
            m_treeView->header()->setSectionHidden(column, !m_treeView->header()->isSectionHidden(column));
        }

        int numVisibleSections = 0;
        for (int i = 0; i < m_treeView->model()->columnCount(); ++i)
        {
            if (!m_treeView->header()->isSectionHidden(i))
            {
                ++numVisibleSections;
            }
        }

        if (numVisibleSections == 0)
        {
            m_treeView->header()->setSectionHidden(0, false);
        }
    }

    void ExplorerPanel::OnContextMenu(const QPoint& pos)
    {
        bool hasModifiedSelection = m_system->document->HasModifiedExporerEntriesSelected();
        bool hasSelection = m_system->document->HasAnimationsSelected();

        QMenu menu;
        menu.addAction("Copy Name", this, SLOT(OnMenuCopyName()));
        menu.addAction("Copy Path", this, SLOT(OnMenuCopyPath()), QKeySequence("Ctrl+C"));
        menu.addAction("Paste Selection", this, SLOT(OnMenuPasteSelection()), QKeySequence("Ctrl+V"));
        menu.addSeparator();

        vector<ExplorerAction> actions;
        m_system->document->GetSelectedExplorerActions(&actions);
        m_explorerActionHandlers.clear();
        for (size_t i = 0; i < actions.size(); ++i)
        {
            const ExplorerAction& action = actions[i];
            if (!action.func)
            {
                menu.addSeparator();
                continue;
            }

            ExplorerActionHandler* handler(new ExplorerActionHandler(action));
            EXPECTED(connect(handler, SIGNAL(SignalAction(const ExplorerAction&)), this, SLOT(OnExplorerAction(const ExplorerAction&))));

            QAction* qaction = new QAction(QIcon(QString(action.icon)), action.text, &menu);
            qaction->setPriority(((action.flags & ACTION_IMPORTANT) != 0) ? QAction::NormalPriority : QAction::LowPriority);
            qaction->setToolTip(action.description);
            qaction->setEnabled((action.flags & ACTION_DISABLED) == 0);
            if (action.flags & ACTION_IMPORTANT)
            {
                QFont boldFont;
                boldFont.setBold(true);
                qaction->setFont(boldFont);
            }
            connect(qaction, SIGNAL(triggered()), handler, SLOT(OnTriggered()));

            menu.addAction(qaction);
            m_explorerActionHandlers.push_back(std::unique_ptr<ExplorerActionHandler>(handler));
        }

        menu.exec(QCursor::pos());
    }

    void ExplorerPanel::OnMenuCopyName()
    {
        ExplorerEntries entries;
        m_system->document->GetSelectedExplorerEntries(&entries);

        QString str;

        if (entries.size() == 1)
        {
            str = QString::fromLocal8Bit(entries[0]->name.c_str());
        }
        else
        {
            for (size_t i = 0; i < entries.size(); ++i)
            {
                str += QString::fromLocal8Bit(entries[i]->name.c_str());
                str += "\n";
            }
        }

        QApplication::clipboard()->setText(str);
    }

    void ExplorerPanel::OnMenuCopyPath()
    {
        ExplorerEntries entries;
        m_system->document->GetSelectedExplorerEntries(&entries);

        QString str;

        if (entries.size() == 1)
        {
            str = QString::fromLocal8Bit(entries[0]->path.c_str());
        }
        else
        {
            for (size_t i = 0; i < entries.size(); ++i)
            {
                str += QString::fromLocal8Bit(entries[i]->path.c_str());
                str += "\n";
            }
        }

        QApplication::clipboard()->setText(str);
    }

    void ExplorerPanel::OnMenuPasteSelection()
    {
        ExplorerEntries entries;

        QString str = QApplication::clipboard()->text();
        QStringList items = str.split("\n", QString::SkipEmptyParts);
        for (int subtree = 0; subtree < NUM_SUBTREES; ++subtree)
        {
            for (size_t i = 0; i < items.size(); ++i)
            {
                ExplorerEntry* entry = m_system->explorer->FindEntryByPath((EExplorerSubtree)subtree, items[i].toLocal8Bit().data());
                if (entry)
                {
                    entries.push_back(entry);
                }
            }
        }

        m_system->document->SetSelectedExplorerEntries(entries, 0);
        if (!entries.empty())
        {
            QModelIndex index = FindIndexByEntry(m_treeView, m_filterModel, m_model, entries[0]);
            if (index.isValid())
            {
                m_treeView->selectionModel()->select(index, QItemSelectionModel::Current);
                m_treeView->scrollTo(index);
            }
        }
    }

    void ExplorerPanel::OnExplorerAction(const ExplorerAction& action)
    {
        if (!action.func)
        {
            return;
        }

        ExplorerEntries entries;
        m_system->document->GetSelectedExplorerEntries(&entries);
        m_mainWindow->ExecuteExplorerAction(action, entries);
    }

    void ExplorerPanel::OnActivated(const QModelIndex& index)
    {
        QModelIndex realIndex;
        if (m_treeView->model() == m_filterModel)
        {
            realIndex = m_filterModel->mapToSource(index);
        }
        else
        {
            realIndex = index;
        }

        ExplorerEntry* entry = ((ExplorerEntry*)realIndex.internalPointer());
        if (entry && entry->type == ENTRY_CHARACTER)
        {
            m_system->scene->characterPath = entry->path;
            m_system->scene->SignalCharacterChanged();
        }
    }

    void ExplorerPanel::OnClickedSelectedItem(const QModelIndex& index)
    {
        QModelIndex realIndex;
        if (m_treeView->model() == m_filterModel)
        {
            realIndex = m_filterModel->mapToSource(index);
        }
        else
        {
            realIndex = index;
        }

        ExplorerEntry* entry = ((ExplorerEntry*)realIndex.internalPointer());
        if (entry)
        {
            m_system->explorer->SignalSelectedEntryClicked(entry);
        }
    }

    void ExplorerPanel::OnEntryImported(ExplorerEntry* entry, ExplorerEntry* oldEntry)
    {
        // this invalidation is hack which is needed because our model implementation
        // doesn't propogate updates from the children of filtered out items
        if (m_filterModel)
        {
            m_filterModel->invalidate();
        }

        QModelIndex newIndex = FindIndexByEntry(m_treeView, m_filterModel, m_model, entry);
        if (newIndex.isValid())
        {
            m_treeView->scrollTo(newIndex);
            m_treeView->selectionModel()->select(newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
        }
    }

    void ExplorerPanel::OnCharacterLoaded()
    {
        // hack
        m_filterModel->invalidate();
    }

    void ExplorerPanel::OnRefreshFilter()
    {
        OnFilterTextChanged(m_filterEdit->text());
    }

    void ExplorerPanel::OnEntryLoaded(ExplorerEntry* entry)
    {
        // this invalidation is hack which is needed because our model implementation
        // doesn't propogate updates from the children of filtered out items
        if (m_filterModel)
        {
            m_filterModel->invalidate();
        }

        QModelIndex newIndex = FindIndexByEntry(m_treeView, m_filterModel, m_model, entry);
        if (newIndex.isValid())
        {
            if (m_filterEdit->text().isEmpty())
            {
                ExpandFirstLevel(m_treeView, newIndex);
            }
            else
            {
                ExpandChildren(m_treeView, newIndex);
            }
        }
    }

    void ExplorerPanel::SetTreeViewModel(QAbstractItemModel* model)
    {
        if (m_treeView->model() != model)
        {
            m_treeView->setModel(model);
            connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnTreeSelectionChanged(const QItemSelection&, const QItemSelection&)));
            m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
            m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

            m_treeView->header()->setStretchLastSection(false);
#if QT_VERSION >= 0x50000
            m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
#else
            m_treeView->header()->setResizeMode(0, QHeaderView::Stretch);
#endif
            for (int i = 1; i < m_system->explorer->GetColumnCount(); ++i)
            {
#if QT_VERSION >= 0x50000
                m_treeView->header()->setSectionResizeMode(i, QHeaderView::Interactive);
#else
                m_treeView->header()->setResizeMode(i, QHeaderView::Interactive);
#endif
                const ExplorerColumn* column = m_system->explorer->GetColumn(i);
                if (column)
                {
                    if (column->format == ExplorerColumn::ICON)
                    {
                        m_treeView->header()->resizeSection(i, 24);
                    }
                    else
                    {
                        m_treeView->header()->resizeSection(i, 50);
                    }
                    if (!column->visibleByDefault)
                    {
                        m_treeView->header()->hideSection(i);
                    }
                }
            }
            m_treeView->header()->setSortIndicator(0, Qt::AscendingOrder);

            for (int i = m_system->explorer->GetColumnCount(); i < model->columnCount(); ++i)
            {
                m_treeView->header()->hideSection(i);
            }
        }
    }

    void ExplorerPanel::OnExplorerEndReset()
    {
        ExpandFirstLevel(m_treeView, m_treeView->rootIndex());
    }

    void ExplorerPanel::Serialize(Serialization::IArchive& ar)
    {
        ar(m_explorerRootIndex, "explorerRootIndex");
        ar(m_filterMode, "filterMode");
        ar(m_filterOptions, "filterOptions");
        if (ar.IsInput() && m_model)
        {
            m_model->SetRootByIndex(m_explorerRootIndex);
            if (m_filterModel)
            {
                m_filterModel->invalidate();
            }
        }

        QString text = m_filterEdit->text();
        ar(text, "filter");
        if (ar.IsInput())
        {
            m_filterEdit->setText(text);
        }

        if (ar.IsInput())
        {
            for (int i = 0; i < m_treeView->model()->columnCount(); ++i)
            {
                const ExplorerColumn* column = m_system->explorer->GetColumn(i);
                if (column)
                {
                    m_treeView->header()->setSectionHidden(i, !column->visibleByDefault);
                }
            }
        }
        ar(static_cast<QTreeView*>(m_treeView), "treeView");
        if (ar.IsInput())
        {
            m_treeView->header()->showSection(0);
        }

        if (ar.IsInput())
        {
            UpdateRootMenu();
            m_filterOptionsTree->revert();
            m_filterButton->setChecked(m_filterMode);
            m_filterModel->setFilterOptions(m_filterMode ? m_filterOptions : FilterOptions());
        }
    }

    bool ExplorerPanel::eventFilter(QObject* sender, QEvent* ev)
    {
        if (sender == m_treeView)
        {
            if (ev->type() == QEvent::KeyPress)
            {
                QKeyEvent* kev = static_cast<QKeyEvent*>(ev);
                int key = kev->key() | kev->modifiers();
                if (key == (Qt::Key_C | Qt::CTRL))
                {
                    OnMenuCopyPath();
                    return true;
                }
                else if (key == (Qt::Key_V | Qt::CTRL))
                {
                    OnMenuPasteSelection();
                    return true;
                }
                else if (key == Qt::Key_Space ||
                    key == Qt::Key_Comma ||
                    key == Qt::Key_Period)
                {
                    m_mainWindow->GetPlaybackPanel()->HandleKeyEvent(key);
                    return true;
                }
            }
            else if (ev->type() == QEvent::ShortcutOverride)
            {
                // When a shortcut is matched, Qt's event processing sends out a shortcut override event
                // to allow other systems to override it. If it's not overridden, then the key events
                // get processed as a shortcut, even if the widget that's the target has a keyPress event
                // handler. So, we need to communicate that we've processed the shortcut override
                // which will tell Qt not to process it as a shortcut and instead pass along the
                // keyPressEvent.

                QKeyEvent* kev = static_cast<QKeyEvent*>(ev);
                int key = kev->key() | kev->modifiers();
                switch (key)
                {
                    // same cases as for the keyPress filter above
                    case (Qt::Key_C | Qt::CTRL):
                    case (Qt::Key_V | Qt::CTRL):
                    case Qt::Key_Space:
                    case Qt::Key_Comma:
                    case Qt::Key_Period:
                        ev->accept();
                        return true;
                    break;

                    default:
                    break;
                }
            }
        }
        return QWidget::eventFilter(sender, ev);
    }

    void ExplorerPanel::OnExplorerBeginBatchChange(int subtree)
    {
        ++m_batchChangesRunning;
    }

    void ExplorerPanel::OnExplorerEndBatchChange(int subtree)
    {
        --m_batchChangesRunning;
        if (m_batchChangesRunning == 0 && m_filterModel)
        {
            m_filterModel->invalidate();
        }
    }

    void ExplorerPanel::OnExplorerEntryModified(ExplorerEntryModifyEvent& ev)
    {
        if (m_filterModel)
        {
            if (m_batchChangesRunning == 0)
            {
                m_filterModel->invalidate();
            }
        }
    }
}

#include <CharacterTool/ExplorerPanel.moc>
