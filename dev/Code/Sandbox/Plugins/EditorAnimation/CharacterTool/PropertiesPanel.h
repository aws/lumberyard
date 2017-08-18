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
#include "Explorer.h"

#include <IEditor.h>

class QDockWidget;
class QToolBar;
class QPropertyTree;
class ExplorerActionHandler;

namespace Serialization
{
    class IArchive;
    class CContextList;
}

namespace CharacterTool
{
    using std::unique_ptr;
    using Serialization::IArchive;
    class CharacterToolForm;
    struct ExplorerAction;
    struct System;

    enum FollowMode
    {
        FOLLOW_SELECTION,
        FOLLOW_SOURCE_ASSET,
        FOLLOW_LOCK,
        NUM_FOLLOW_MODES
    };

    enum OutlineMode
    {
        OUTLINE_ON_LEFT,
        OUTLINE_ON_TOP,
        OUTLINE_ONE_TREE,
        NUM_OUTLINE_MODES
    };

    struct InspectorLocation
    {
        vector<ExplorerEntryId> entries;
        vector<char> tree;

        void Serialize(IArchive& ar);
        bool IsValid() const { return !entries.empty(); }

        bool operator==(const InspectorLocation& rhs) const;
        bool operator!=(const InspectorLocation& rhs) const{ return !operator==(rhs); }
    };

    class PropertiesPanel
        : public QWidget
        , public IEditorNotifyListener
    {
        Q_OBJECT
    public:
        PropertiesPanel(QWidget* parent, System* system, QMainWindow* mainWindow);
        ~PropertiesPanel();
        QPropertyTree* PropertyTree() { return m_propertyTree; }
        void Serialize(Serialization::IArchive& ar);
        void SetDockWidget(QDockWidget* widget) {}

    protected:
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    private slots:
        void OnPropertyTreeSelected();
        void OnPropertyTreeChanged();
        void OnPropertyTreeContinuousChange();
        void OnExplorerSelectionChanged();
        void OnExplorerEntryModified(ExplorerEntryModifyEvent& ev);
        void OnExplorerAction(const ExplorerAction& _action);
        void OnCharacterLoaded();
        void OnSubselectionChanged(int changedLayer);
        void OnFollowMenu();
        void OnSettingMenu();
        void OnLocationButton();
        void OnBackButton();
        void OnForwardButton();
        void OnUndo();
        void OnUndoMenu();
        void OnRedo();
        void SetOutlineMode(OutlineMode outlineMode);
    private:
        void AttachPropertyTreeToLocation();
        void StorePropertyTreeState();
        void ExecuteExplorerAction(const ExplorerAction& action);
        void UpdateToolbar();
        void UpdateUndoButtons();
        void UpdateLocationBar();
        void Undo(int count);

        FollowMode m_followMode;
        OutlineMode m_outlineMode;
        OutlineMode m_outlineModeUsed;
        InspectorLocation m_location;
        vector<InspectorLocation> m_previousLocations;
        vector<InspectorLocation> m_nextLocations;

        QToolButton* m_backButton;
        QToolButton* m_forwardButton;
        QToolButton* m_locationButton;
        QToolButton* m_followButton;
        QMenu* m_followMenu;
        QToolButton* m_settingsButton;
        QMenu* m_settingsMenu;
        QAction* m_followActions[NUM_FOLLOW_MODES];

        QToolBar* m_toolbar;
        QAction* m_actionUndo;
        QAction* m_actionRedo;
        QMenu* m_undoMenu;

        QSplitter* m_splitter;
        QPropertyTree* m_propertyTree;
        QPropertyTree* m_detailTree;
        QAction* m_settingActions[NUM_OUTLINE_MODES];

        System* m_system;
        bool m_ignoreSubselectionChange;
        bool m_ignoreExplorerSelectionChange;
        std::vector<std::unique_ptr<ExplorerActionHandler> > m_explorerActionHandlers;
        CharacterToolForm* m_mainWindow;
    };
}

