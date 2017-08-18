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
#include <QToolBar>
#include <QToolButton>
#include <QIcon>
#include <QAction>
#include <QBoxLayout>
#include <QMenu>
#include <QSplitter>
#include "PropertiesPanel.h"
#include "CharacterDocument.h"
#include "Explorer.h"
#include "ExplorerDataProvider.h"
#include "Expected.h"
#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "../EditorCommon/QPropertyTree/ContextList.h"
#include <ICryAnimation.h>
#include "CharacterToolForm.h"
#include "CharacterToolSystem.h"
#include "CharacterGizmoManager.h"
#include "SceneContent.h"
#include "Serialization.h"

struct ICharacterInstance;
struct ICharacterModel;

namespace CharacterTool
{
    SERIALIZATION_ENUM_BEGIN(FollowMode, "Follow Mode")
    SERIALIZATION_ENUM(FOLLOW_SELECTION, "selection", "Follow Selection")
    SERIALIZATION_ENUM(FOLLOW_SOURCE_ASSET, "sourceAsset", "Source Asset")
    SERIALIZATION_ENUM(FOLLOW_LOCK, "lock", "Lock")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN(OutlineMode, "Outline Mode")
    SERIALIZATION_ENUM(OUTLINE_ON_LEFT, "onLeft", "On Left")
    SERIALIZATION_ENUM(OUTLINE_ON_TOP, "onTop", "On Top")
    SERIALIZATION_ENUM(OUTLINE_ONE_TREE, "oneTree", "One Tree")
    SERIALIZATION_ENUM_END()

    bool InspectorLocation::operator==(const InspectorLocation& rhs) const
    {
        if (entries.size() != rhs.entries.size())
        {
            return false;
        }
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (!(entries[i] == rhs.entries[i]))
            {
                return false;
            }
        }
        return true;
    }


    void InspectorLocation::Serialize(IArchive& ar)
    {
        ar(entries, "entries", "Entries");
    }

    static void GetActiveEntries(ExplorerEntries* entries, const vector<ExplorerEntryId>& ids, System* system)
    {
        entries->clear();
        for (size_t i = 0; i < ids.size(); ++i)
        {
            const ExplorerEntryId& id = ids[i];
            ExplorerEntry* entry = system->explorer->FindEntryById(id);
            if (!entry)
            {
                continue;
            }
            entries->push_back(entry);
        }
    }

    PropertiesPanel::PropertiesPanel(QWidget* parent, System* system, QMainWindow* mainWindow)
        : QWidget(parent)
        , m_system(system)
        , m_ignoreSubselectionChange(false)
        , m_ignoreExplorerSelectionChange(false)
        , m_mainWindow(static_cast<CharacterToolForm*>(mainWindow))
        , m_backButton()
        , m_forwardButton()
        , m_locationButton()
        , m_followButton()
        , m_followMenu()
        , m_followMode(FOLLOW_SELECTION)
        , m_outlineMode(OUTLINE_ONE_TREE)
        , m_outlineModeUsed(OUTLINE_ONE_TREE)
    {
        memset(m_followActions, 0, sizeof(m_followActions));
        memset(m_settingActions, 0, sizeof(m_settingActions));

        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalExplorerSelectionChanged()), SLOT(OnExplorerSelectionChanged())));
        EXPECTED(connect(m_system->document.get(), SIGNAL(SignalCharacterLoaded()), SLOT(OnCharacterLoaded())));
        EXPECTED(connect(m_system->explorer.get(), SIGNAL(SignalEntryModified(ExplorerEntryModifyEvent &)), this, SLOT(OnExplorerEntryModified(ExplorerEntryModifyEvent &))));
        EXPECTED(connect(m_system->characterGizmoManager.get(), SIGNAL(SignalSubselectionChanged(int)), SLOT(OnSubselectionChanged(int))));

        setContentsMargins(0, 0, 0, 0);

        m_propertyTree = new QPropertyTree(this);
        m_propertyTree->setCompact(false);
        m_propertyTree->setShowContainerIndices(true);
        m_propertyTree->setExpandLevels(1);
        m_propertyTree->setSliderUpdateDelay(0);
        m_propertyTree->setValueColumnWidth(0.59f);
        m_propertyTree->setArchiveContext(m_system->contextList->Tail());
        m_propertyTree->setSizeHint(QSize(240, 400));
        m_propertyTree->setMultiSelection(true);
        m_propertyTree->setAutoRevert(false);
        m_propertyTree->setUndoEnabled(false);
        m_propertyTree->setAggregateMouseEvents(true);
        m_propertyTree->setAutoHideAttachedPropertyTree(true);

        EXPECTED(connect(m_propertyTree, SIGNAL(signalSelected()), this, SLOT(OnPropertyTreeSelected())));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalChanged()), this, SLOT(OnPropertyTreeChanged())));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalContinuousChange()), this, SLOT(OnPropertyTreeContinuousChange())));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalUndo()), this, SLOT(OnUndo())));
        EXPECTED(connect(m_propertyTree, SIGNAL(signalRedo()), this, SLOT(OnRedo())));

        QBoxLayout* locationLayout = new QBoxLayout(QBoxLayout::LeftToRight);
        locationLayout->setContentsMargins(2, 0, 2, 0);
        locationLayout->setSpacing(2);
        {
            m_backButton = new QToolButton();
            m_backButton->setToolTip("Go back to previously selected item.");
            m_backButton->setIcon(QIcon("Editor/Icons/animation/back_16.png"));
            EXPECTED(connect(m_backButton, SIGNAL(clicked()), this, SLOT(OnBackButton())));
            m_backButton->setAutoRaise(true);
            locationLayout->addWidget(m_backButton);

            m_forwardButton = new QToolButton();
            m_forwardButton->setToolTip("Go forward to next selected item.");
            m_forwardButton->setIcon(QIcon("Editor/Icons/animation/forward_16.png"));
            m_forwardButton->setAutoRaise(true);
            EXPECTED(connect(m_forwardButton, SIGNAL(clicked()), this, SLOT(OnForwardButton())));
            locationLayout->addWidget(m_forwardButton);

            m_locationButton = new QToolButton();
            m_locationButton->setText("None");
            m_locationButton->setIcon(QIcon());
            m_locationButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            m_locationButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            m_locationButton->setMinimumWidth(32);
            EXPECTED(connect(m_locationButton, SIGNAL(clicked()), this, SLOT(OnLocationButton())));
            m_locationButton->setCheckable(true);
            locationLayout->addWidget(m_locationButton);

            m_followMenu = new QMenu();
            m_followActions[FOLLOW_SELECTION] = m_followMenu->addAction(QIcon("Editor/Icons/animation/selection_16.png"), "Follow Selection", this, SLOT(OnFollowMenu()));
            m_followActions[FOLLOW_SELECTION]->setData(int(FOLLOW_SELECTION));
            if (m_system->explorer->HasProvider(SUBTREE_SOURCE_ASSETS))
            {
                m_followActions[FOLLOW_SOURCE_ASSET] = m_followMenu->addAction(QIcon("Editor/Icons/animation/source_asset_16.png"), "Import Assets", this, SLOT(OnFollowMenu()));
                ;
                m_followActions[FOLLOW_SOURCE_ASSET]->setData(int(FOLLOW_SOURCE_ASSET));
            }
            m_followMenu->addSeparator();
            m_followActions[FOLLOW_LOCK] = m_followMenu->addAction(QIcon("Editor/Icons/animation/lock_16.png"), "Lock", this, SLOT(OnFollowMenu()));
            m_followActions[FOLLOW_LOCK]->setData(int(FOLLOW_LOCK));

            m_followButton = new QToolButton();
            m_followButton->setAutoRaise(true);
            m_followButton->setIcon(QIcon("Editor/Icons/animation/selection_16.png"));
            m_followButton->setMenu(m_followMenu);
            m_followButton->setPopupMode(QToolButton::InstantPopup);
            locationLayout->addWidget(m_followButton);

            m_settingsMenu = new QMenu();
            m_settingActions[OUTLINE_ONE_TREE] = m_settingsMenu->addAction("Single Tree", this, SLOT(OnSettingMenu()));
            m_settingActions[OUTLINE_ONE_TREE]->setData(int(OUTLINE_ONE_TREE));
            m_settingActions[OUTLINE_ON_TOP] = m_settingsMenu->addAction("Outline: Top", this, SLOT(OnSettingMenu()));
            m_settingActions[OUTLINE_ON_TOP]->setData(int(OUTLINE_ON_TOP));
            m_settingActions[OUTLINE_ON_LEFT] = m_settingsMenu->addAction("Outline: Left", this, SLOT(OnSettingMenu()));
            m_settingActions[OUTLINE_ON_LEFT]->setData(int(OUTLINE_ON_LEFT));

            m_settingsButton = new QToolButton();
            m_settingsButton->setAutoRaise(true);
            m_settingsButton->setIcon(QIcon("Editor/Icons/animation/playback_options.png"));
            m_settingsButton->setMenu(m_settingsMenu);
            m_settingsButton->setPopupMode(QToolButton::InstantPopup);
            locationLayout->addWidget(m_settingsButton);

            SetOutlineMode(m_outlineMode);
        }

        m_splitter = new QSplitter(Qt::Horizontal);
        m_detailTree = new QPropertyTree(this);
        m_detailTree->setCompact(true);
        m_detailTree->setMultiSelection(true);
        m_detailTree->setArchiveContext(m_system->contextList->Tail());
        m_detailTree->setSliderUpdateDelay(0);
        m_detailTree->setValueColumnWidth(0.7f);
        m_detailTree->setVisible(false);
        m_detailTree->setAggregateMouseEvents(true);
        m_detailTree->setPackCheckboxes(false);
        EXPECTED(connect(m_detailTree, SIGNAL(signalChanged()), this, SLOT(OnPropertyTreeChanged())));
        EXPECTED(connect(m_detailTree, SIGNAL(signalContinuousChange()), this, SLOT(OnPropertyTreeContinuousChange())));
        EXPECTED(connect(m_detailTree, SIGNAL(signalUndo()), this, SLOT(OnUndo())));
        EXPECTED(connect(m_detailTree, SIGNAL(signalRedo()), this, SLOT(OnRedo())));

        m_splitter->addWidget(m_propertyTree);
        m_splitter->addWidget(m_detailTree);

        m_toolbar = new QToolBar(this);
        m_toolbar->setIconSize(QSize(32, 32));
        m_toolbar->setStyleSheet("QToolBar { border: 0px }");
        m_toolbar->setFloatable(false);

        m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->setSpacing(4);
        layout->addLayout(locationLayout, 0);
        layout->addWidget(m_toolbar, 0);
        layout->addWidget(m_splitter, 1);
        layout->setMargin(0);
        setLayout(layout);

        OnExplorerSelectionChanged();
        GetIEditor()->RegisterNotifyListener(this);
    }


    void PropertiesPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        if (event == eNotify_OnIdleUpdate)
        {
            m_propertyTree->flushAggregatedMouseEvents();
            m_detailTree->flushAggregatedMouseEvents();
        }
    }

    PropertiesPanel::~PropertiesPanel()
    {
        GetIEditor()->UnregisterNotifyListener(this);
    }

    void PropertiesPanel::SetOutlineMode(OutlineMode outlineMode)
    {
        if (m_outlineMode != outlineMode)
        {
            m_outlineMode = outlineMode;

            AttachPropertyTreeToLocation();
        }

        for (int i = 0; i < NUM_OUTLINE_MODES; ++i)
        {
            QAction* a = m_settingActions[i];
            if (!a)
            {
                continue;
            }
            a->setCheckable(true);
            a->setChecked(i == int(m_outlineMode));
        }
    }

    void PropertiesPanel::OnLocationButton()
    {
        if (!m_location.IsValid())
        {
            return;
        }

        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);

        if (entries.empty())
        {
            return;
        }

        m_ignoreExplorerSelectionChange = true;
        m_system->document->SetSelectedExplorerEntries(entries, 0);
        m_ignoreExplorerSelectionChange = false;

        UpdateLocationBar();
    }

    void PropertiesPanel::OnFollowMenu()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action)
        {
            m_followMode = (FollowMode)action->data().toInt();
            if (m_followMode == FOLLOW_SELECTION)
            {
                OnExplorerSelectionChanged();
            }
            UpdateLocationBar();
        }
    }

    void PropertiesPanel::OnSettingMenu()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action)
        {
            SetOutlineMode((OutlineMode)action->data().toInt());
            AttachPropertyTreeToLocation();
        }
    }

    void PropertiesPanel::OnExplorerEntryModified(ExplorerEntryModifyEvent& ev)
    {
        if (!ev.entry)
        {
            return;
        }
        ExplorerEntryId id(ev.entry->subtree, ev.entry->id);
        if (std::find(m_location.entries.begin(), m_location.entries.end(), id) != m_location.entries.end())
        {
            if (ev.continuousChange)
            {
                m_propertyTree->revertNoninterrupting();
            }
            else
            {
                m_propertyTree->revert();
            }

            if (!ev.continuousChange)
            {
                UpdateLocationBar();
                UpdateToolbar();
                UpdateUndoButtons();
            }
        }
    }

    void PropertiesPanel::OnExplorerSelectionChanged()
    {
        if (m_ignoreExplorerSelectionChange)
        {
            m_propertyTree->update(); // redraw selection-dependent buttons
            return;
        }

        InspectorLocation location = m_location;
        if (m_followMode == FOLLOW_SELECTION)
        {
            ExplorerEntries entries;
            m_system->document->GetSelectedExplorerEntries(&entries);
            location.entries.clear();
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ExplorerEntry* e = entries[i];
                ExplorerEntryId id(e->subtree, e->id);
                if (e->type != ENTRY_SUBTREE_ROOT &&
                    e->type != ENTRY_NONE &&
                    e->type != ENTRY_GROUP &&
                    e->type != ENTRY_LOADING)
                {
                    location.entries.push_back(id);
                }
            }
        }
        if (m_followMode == FOLLOW_SOURCE_ASSET)
        {
            ExplorerEntries entries;
            m_system->document->GetSelectedExplorerEntries(&entries);

            InspectorLocation sourceAssetLocation;
            for (size_t i = 0; i < entries.size(); ++i)
            {
                ExplorerEntry* e = entries[i];
                ExplorerEntryId id(e->subtree, e->id);
                if (e->type == ENTRY_SOURCE_ASSET)
                {
                    sourceAssetLocation.entries.push_back(id);
                }
            }
            if (sourceAssetLocation.IsValid())
            {
                location = sourceAssetLocation;
            }
        }

        if (location != m_location)
        {
            if (m_location.IsValid())
            {
                StorePropertyTreeState();
                m_previousLocations.push_back(m_location);
            }
            m_location = location;
            m_nextLocations.clear();

            AttachPropertyTreeToLocation();
            UpdateToolbar();
            UpdateUndoButtons();
        }

        UpdateLocationBar(); // calling this always to update selection state of the location button
        m_propertyTree->update(); // calling this to redraw selection-dependent buttons
    }

    static bool EntrySupportsOutlineMode(EExplorerEntryType type)
    {
        switch (type)
        {
        case ENTRY_CHARACTER:
        case ENTRY_DBA_TABLE:
        case ENTRY_COMPRESSION_PRESETS:
            return true;
        default:
            return false;
        }
    }

    void PropertiesPanel::AttachPropertyTreeToLocation()
    {
        bool allEntriesSupportOutlineMode = true;

        vector<ExplorerEntry*> entries;
        for (size_t i = 0; i < m_location.entries.size(); ++i)
        {
            const ExplorerEntryId& id = m_location.entries[i];
            ExplorerEntry* entry = m_system->explorer->FindEntryById(id);
            if (!entry)
            {
                continue;
            }
            entries.push_back(entry);

            if (!EntrySupportsOutlineMode(entry->type))
            {
                allEntriesSupportOutlineMode = false;
            }
        }


        OutlineMode outlineMode = m_outlineMode;
        if (!allEntriesSupportOutlineMode)
        {
            outlineMode = OUTLINE_ONE_TREE;
        }

        if (outlineMode != m_outlineModeUsed)
        {
            m_propertyTree->setOutlineMode(outlineMode != OUTLINE_ONE_TREE);
            if (outlineMode == OUTLINE_ON_TOP || outlineMode == OUTLINE_ON_LEFT)
            {
                m_propertyTree->attachPropertyTree(m_detailTree);
            }
            else
            {
                m_propertyTree->detachPropertyTree();
            }

            m_detailTree->setVisible(outlineMode != OUTLINE_ONE_TREE);

            if (outlineMode == OUTLINE_ON_LEFT)
            {
                m_splitter->setOrientation(Qt::Horizontal);
                m_propertyTree->setAutoHideAttachedPropertyTree(false);
                m_detailTree->show();
            }
            else
            {
                m_splitter->setOrientation(Qt::Vertical);
                m_propertyTree->setAutoHideAttachedPropertyTree(true);
            }
            m_propertyTree->detach();

            m_outlineModeUsed = outlineMode;
        }

        Serialization::SStructs structs;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];

            Serialization::SStruct ref;
            if (m_system->explorer->GetSerializerForEntry(&ref, entry))
            {
                structs.push_back(ref);
            }
        }

        m_propertyTree->attach(structs);
        if (!m_location.tree.empty())
        {
            SerializeFromMemory(Serialization::SStruct(*m_propertyTree), m_location.tree);
        }
    }

    void PropertiesPanel::StorePropertyTreeState()
    {
        SerializeToMemory(&m_location.tree, Serialization::SStruct(*m_propertyTree));
    }

    void PropertiesPanel::OnUndo()
    {
        Undo(1);
    }

    void PropertiesPanel::OnRedo()
    {
        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);
        if (entries.empty())
        {
            return;
        }
        m_system->explorer->Redo(entries);
        UpdateUndoButtons();
    }

    void PropertiesPanel::UpdateUndoButtons()
    {
        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);

        m_undoMenu->clear();
        m_actionUndo->setEnabled(m_system->explorer->CanUndo(entries));
        m_actionRedo->setEnabled(m_system->explorer->CanRedo(entries));

        if (entries.empty())
        {
            return;
        }

        int maxActions = 20;
        vector<string> undoActions;
        m_system->explorer->GetUndoActions(&undoActions, maxActions, entries);
        for (size_t i = 0; i < undoActions.size(); ++i)
        {
            const char* actionText = undoActions[i].c_str();
            QAction* action = m_undoMenu->addAction(QString::fromLocal8Bit(actionText), this, SLOT(OnUndoMenu()));
            action->setData(int(i + 1));
        }
    }

    void PropertiesPanel::Undo(int count)
    {
        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);
        if (entries.empty())
        {
            return;
        }
        m_system->explorer->Undo(entries, count);
        UpdateUndoButtons();
    }

    void PropertiesPanel::OnUndoMenu()
    {
        if (QAction* action = qobject_cast<QAction*>(sender()))
        {
            int count = action->data().toInt();
            if (count == 0)
            {
                return;
            }
            Undo(count);
        }
    }

    void PropertiesPanel::UpdateLocationBar()
    {
        bool allEntriesSelected = !m_location.entries.empty();

        ExplorerEntry* singleEntry = 0;
        for (size_t i = 0; i < m_location.entries.size(); ++i)
        {
            const ExplorerEntryId& id = m_location.entries[i];
            ExplorerEntry* entry = m_system->explorer->FindEntryById(id);
            if (!singleEntry)
            {
                singleEntry = entry;
            }
            if (!m_system->document->IsExplorerEntrySelected(singleEntry))
            {
                allEntriesSelected = false;
            }
        }

        string text;
        const char* icon;

        if (m_location.entries.size() > 1)
        {
            bool atLeastOneEntryModified = false;
            for (int i = 0; i < m_location.entries.size(); ++i)
            {
                if (ExplorerEntry* entry = m_system->explorer->FindEntryById(m_location.entries[i]))
                {
                    if (entry->modified)
                    {
                        atLeastOneEntryModified = true;
                        break;
                    }
                }
            }

            char buf[64] = "";
            sprintf_s(buf, "%s%d Items", (atLeastOneEntryModified ? "*" : ""), (int)m_location.entries.size());
            text = buf;
            icon = "Editor/Icons/animation/mutliple_items_16.png";
        }
        else if (singleEntry)
        {
            if (singleEntry->modified)
            {
                text = "*";
            }
            text += singleEntry->name;
            icon = Explorer::IconForEntry(singleEntry->type, singleEntry);
        }
        else
        {
            text = "No Selection";
            icon = "Editor/Icons/animation/no_selection_16.png";
        }

        m_locationButton->setText(text.c_str());
        m_locationButton->setIcon(QIcon(icon));
        m_locationButton->setChecked(allEntriesSelected);

        const char* followIcon = "";
        switch (m_followMode)
        {
        case FOLLOW_SELECTION:
            followIcon = "Editor/Icons/animation/selection_16.png";
            break;
        case FOLLOW_SOURCE_ASSET:
            followIcon = "Editor/Icons/animation/source_asset_16.png";
            break;
        case FOLLOW_LOCK:
            followIcon = "Editor/Icons/animation/lock_16.png";
            break;
        }
        m_followButton->setIcon(QIcon(followIcon));
        for (int mode = 0; mode < NUM_FOLLOW_MODES; ++mode)
        {
            QAction* action = m_followActions[mode];
            if (!action)
            {
                continue;
            }
            action->setCheckable(true);
            action->setChecked(m_followMode == mode);
        }

        m_backButton->setEnabled(!m_previousLocations.empty());
        m_forwardButton->setEnabled(!m_nextLocations.empty());
    }

    void PropertiesPanel::UpdateToolbar()
    {
        ExplorerEntries selectedEntries;
        for (size_t i = 0; i < m_location.entries.size(); ++i)
        {
            const ExplorerEntryId& id = m_location.entries[i];
            ExplorerEntry* entry = m_system->explorer->FindEntryById(id);
            if (!entry)
            {
                continue;
            }
            selectedEntries.push_back(entry);
        }

        vector<ExplorerAction> actions;
        m_system->explorer->GetCommonActions(&actions, selectedEntries);
        m_toolbar->clear();
        m_explorerActionHandlers.clear();
        m_actionUndo = m_toolbar->addAction(QIcon("Editor/Icons/animation/undo.png"), "Undo", this, SLOT(OnUndo()));
        m_actionUndo->setEnabled(m_system->explorer->CanUndo(selectedEntries));
        m_undoMenu = new QMenu(this);
        m_actionUndo->setMenu(m_undoMenu);
        m_actionUndo->setPriority(QAction::LowPriority);
        m_actionRedo = m_toolbar->addAction(QIcon("Editor/Icons/animation/redo.png"), "Redo", this, SLOT(OnRedo()));
        m_actionRedo->setEnabled(m_system->explorer->CanRedo(selectedEntries));
        m_actionRedo->setPriority(QAction::LowPriority);
        m_toolbar->addSeparator();

        if (!actions.empty())
        {
            m_toolbar->setVisible(true);
            for (size_t i = 0; i < actions.size(); ++i)
            {
                const ExplorerAction& action = actions[i];
                if (!action.icon || action.icon[0] == '\0')
                {
                    continue;
                }
                if (action.func)
                {
                    ExplorerActionHandler* handler(new ExplorerActionHandler(action));
                    m_explorerActionHandlers.push_back(std::unique_ptr<ExplorerActionHandler>(handler));

                    EXPECTED(connect(handler, SIGNAL(SignalAction(const ExplorerAction&)), this, SLOT(OnExplorerAction(const ExplorerAction&))));

                    QAction* qaction = new QAction(QIcon(QString(action.icon)), action.text, this);
                    qaction->setPriority((action.flags & ACTION_IMPORTANT) ? QAction::NormalPriority : QAction::LowPriority);
                    qaction->setEnabled((action.flags & ACTION_DISABLED) == 0);
                    if (action.description[0] != '\0')
                    {
                        qaction->setToolTip(QString(action.text) + ": " +  action.description);
                    }
                    if (action.flags & ACTION_IMPORTANT)
                    {
                        QFont boldFont;
                        boldFont.setBold(true);
                        qaction->setFont(boldFont);
                    }
                    EXPECTED(connect(qaction, SIGNAL(triggered()), handler, SLOT(OnTriggered())));

                    m_toolbar->addAction(qaction);
                }
                else
                {
                    m_toolbar->addSeparator();
                }
            }
        }
        else
        {
            m_toolbar->setVisible(false);
        }
    }

    static GizmoLayer LayerByExplorerEntry(ExplorerEntry* entry, System* system)
    {
        if (entry == system->document->GetActiveAnimationEntry())
        {
            return GIZMO_LAYER_ANIMATION;
        }
        if (entry == system->document->GetActiveCharacterEntry())
        {
            return GIZMO_LAYER_CHARACTER;
        }

        return GIZMO_LAYER_NONE;
    }

    void PropertiesPanel::OnPropertyTreeSelected()
    {
        ExplorerEntries selectedEntries;
        GetActiveEntries(&selectedEntries, m_location.entries, m_system);
        if (selectedEntries.size() != 1)
        {
            return;
        }

        GizmoLayer layer = LayerByExplorerEntry(selectedEntries[0], m_system);
        if (layer == GIZMO_LAYER_NONE)
        {
            return;
        }

        std::vector<PropertyRow*> rows;
        rows.resize(m_propertyTree->selectedRowCount());
        for (int i = 0; i < rows.size(); ++i)
        {
            rows[i] = m_propertyTree->selectedRowByIndex(i);
        }
        if (rows.empty())
        {
            return;
        }

        vector<const void*> handles;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            PropertyRow* row = rows[i];
            while (row->parent())
            {
                if (row->serializer())
                {
                    handles.push_back(row->searchHandle());
                }
                row = row->parent();
            }
        }

        m_ignoreSubselectionChange = true;
        m_system->characterGizmoManager->SetSubselection(layer, handles);
        m_ignoreSubselectionChange = false;
    }

    void PropertiesPanel::OnSubselectionChanged(int changedLayer)
    {
        if (m_ignoreSubselectionChange)
        {
            return;
        }
        ExplorerEntries selectedEntries;
        GetActiveEntries(&selectedEntries, m_location.entries, m_system);
        if (selectedEntries.size() != 1)
        {
            return;
        }
        GizmoLayer layer = LayerByExplorerEntry(selectedEntries[0], m_system);
        if (layer == GIZMO_LAYER_NONE)
        {
            return;
        }
        if (layer != changedLayer)
        {
            return;
        }

        const vector<const void*>& handles = m_system->characterGizmoManager->Subselection(layer);
        m_propertyTree->selectByAddresses(handles.data(), handles.size(), true);
    }

    void PropertiesPanel::OnPropertyTreeChanged()
    {
        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            m_system->explorer->CheckIfModified(entry, "Property Change", false);
        }
    }

    void PropertiesPanel::OnPropertyTreeContinuousChange()
    {
        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);
        for (size_t i = 0; i < entries.size(); ++i)
        {
            ExplorerEntry* entry = entries[i];
            m_system->explorer->CheckIfModified(entry, "Property Change", true);
        }
    }

    void PropertiesPanel::Serialize(Serialization::IArchive& ar)
    {
        Serialization::SContext<Explorer> explorerContext(ar, m_system ? m_system->explorer.get() : 0);
        ar(*m_propertyTree, "propertyTree");
        ar(m_followMode, "followMode");

        OutlineMode outlineMode = m_outlineMode;
        ar(outlineMode, "outlineMode");
        if (outlineMode != m_outlineMode)
        {
            SetOutlineMode(outlineMode);
        }

        ar(m_location, "location");
        ar(m_previousLocations, "previousLocations");
        ar(m_nextLocations, "nextLocations");

        if (ar.IsInput())
        {
            UpdateLocationBar();
            UpdateToolbar();
            UpdateUndoButtons();
        }
    }

    void PropertiesPanel::OnCharacterLoaded()
    {
        m_propertyTree->revert();
    }

    void PropertiesPanel::OnExplorerAction(const ExplorerAction& _action)
    {
        // we need a local copy as m_explorerActionHandlers may
        // change during action execution
        const ExplorerAction action = _action;

        m_propertyTree->applyInplaceEditor();

        ExplorerEntries entries;
        GetActiveEntries(&entries, m_location.entries, m_system);
        m_mainWindow->ExecuteExplorerAction(action, entries);
    }

    void PropertiesPanel::OnBackButton()
    {
        if (m_previousLocations.empty())
        {
            return;
        }

        StorePropertyTreeState();
        if (m_location.IsValid())
        {
            m_nextLocations.push_back(m_location);
        }

        m_location = m_previousLocations.back();
        m_previousLocations.pop_back();

        AttachPropertyTreeToLocation();

        if (m_followMode == FOLLOW_SELECTION)
        {
            ExplorerEntries entries;
            GetActiveEntries(&entries, m_location.entries, m_system);
            m_ignoreExplorerSelectionChange = true;
            m_system->document->SetSelectedExplorerEntries(entries, 0);
            m_ignoreExplorerSelectionChange = false;
            UpdateToolbar();
        }

        UpdateLocationBar();
    }

    void PropertiesPanel::OnForwardButton()
    {
        if (m_nextLocations.empty())
        {
            return;
        }

        StorePropertyTreeState();
        m_previousLocations.push_back(m_location);

        m_location = m_nextLocations.back();
        m_nextLocations.pop_back();

        AttachPropertyTreeToLocation();

        if (m_followMode == FOLLOW_SELECTION)
        {
            ExplorerEntries entries;
            GetActiveEntries(&entries, m_location.entries, m_system);
            m_ignoreExplorerSelectionChange = true;
            m_system->document->SetSelectedExplorerEntries(entries, 0);
            m_ignoreExplorerSelectionChange = false;
            UpdateToolbar();
        }

        UpdateLocationBar();
    }
}

#include <CharacterTool/PropertiesPanel.moc>
