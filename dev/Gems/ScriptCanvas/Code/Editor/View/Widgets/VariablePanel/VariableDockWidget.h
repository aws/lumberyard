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
#pragma once

#include <QAbstractListModel>
#include <QAbstractItemView>
#include <QListView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QFocusEvent>
#include <QMenu>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasTreeModel.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <Editor/View/Widgets/VariablePanel/VariablePaletteTableView.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

class QAction;
class QLineEdit;
class QPushButton;

namespace Ui
{
    class VariableDockWidget;
}

namespace ScriptCanvasEditor
{
    class VariablePropertiesComponent
        : public GraphCanvas::GraphCanvasPropertyComponent
        , protected ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(VariablePropertiesComponent, "{885F276B-9633-42F7-85BD-10869E606873}", GraphCanvasPropertyComponent);

        static void Reflect(AZ::ReflectContext*);
        static AZ::Entity* CreateVariablePropertiesEntity();

        VariablePropertiesComponent();
        ~VariablePropertiesComponent() = default;

        const char* GetTitle();

        void SetVariable(const ScriptCanvas::VariableId& variableId);
        void SetScriptCanvasGraphId(AZ::EntityId scriptCanvasGraphId);

    private:
        void OnNameChanged();

        // VariableNotificationBus::Handler
        void OnVariableRemoved() override;
        void OnVariableRenamed(AZStd::string_view variableName) override;
        void OnVariableValueChanged() override;
        void OnVariableExposureChanged() override;
        void OnVariableExposureGroupChanged() override;
        ////

        ScriptCanvas::VariableId m_variableId;
        AZStd::string m_variableName;
        ScriptCanvas::VariableDatum* m_variableDatum;
        AZ::EntityId m_scriptCanvasGraphId;

        AZStd::string m_componentTitle;
    };

    class VariableDockWidget;

    class VariablePanelContextMenu
        : public QMenu
    {
    public:
        VariablePanelContextMenu(VariableDockWidget* contextMenu, const AZ::EntityId& scriptCanvasGraphId, ScriptCanvas::VariableId varId);
    };

    class VariableDockWidget
        : public AzQtComponents::StyledDockWidget
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(VariableDockWidget, AZ::SystemAllocator, 0);

        static AZStd::string ConstructDefaultVariableName(AZ::u32 variableCounter);
        static AZStd::string FindDefaultVariableName(const AZ::EntityId& scriptCanvasGraphId);

        VariableDockWidget(QWidget* parent = nullptr);
        ~VariableDockWidget();

        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);

        // GraphCanvas::AssetEditorNotificationBus::Handler
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        ////

        // AzToolsFramework::EditorEvents::Bus
        void OnEscape() override;
        ////

        // QWidget
        void focusOutEvent(QFocusEvent* focusEvent) override;
        ////

        const AZ::EntityId& GetActiveScriptCanvasGraphId() const;

    public slots:
        void OnCreateVariable(ScriptCanvas::Data::Type varType);
        void OnCreateNamedVariable(const AZStd::string& variableName, ScriptCanvas::Data::Type varType);
        void OnSelectionChanged(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds);
        void OnDuplicateVariable(const ScriptCanvas::VariableId& variableId);
        void OnDeleteVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds);
        void OnHighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds);

        void OnRemoveUnusedVariables();

    Q_SIGNALS:
        void OnVariableSelectionChanged(const AZStd::vector<AZ::EntityId>& variableIds);

    private:

        bool IsShowingVariablePalette() const;
        void ShowVariablePalette();

        bool IsShowingGraphVariables() const;
        void ShowGraphVariables();

        void FocusOnSearchFilter();

        void ClearFilter();
        void UpdateFilter();

        void OnReturnPressed();
        void OnQuickFilterChanged();

        void RefreshModel();

        void OnAddVariableButton();
        void OnContextMenuRequested(const QPoint &pos);

        bool CanDeleteVariable(const ScriptCanvas::VariableId& variableId);

        bool m_manipulatingSelection;

        AZStd::unordered_map<ScriptCanvas::VariableId, AZStd::unique_ptr<AZ::Entity>> m_propertyHelpersMap;
        AZ::EntityId m_scriptCanvasGraphId;
        AZ::EntityId m_graphCanvasGraphId;

        AZStd::unique_ptr<Ui::VariableDockWidget> ui;

        QTimer m_filterTimer;
    };
}