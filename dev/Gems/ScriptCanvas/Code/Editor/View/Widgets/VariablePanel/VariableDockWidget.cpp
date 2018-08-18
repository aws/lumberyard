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
#include <precompiled.h>

#include <QCompleter>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QScopedValueRollback>
#include <QLineEdit>
#include <QTimer>
#include <QPushButton>
#include <QHeaderView>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/ui_VariableDockWidget.h>

#include <Data/Data.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/QtMetaTypes.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>


#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////
    // VariablePropertiesComponent
    ////////////////////////////////

    void VariablePropertiesComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<VariablePropertiesComponent, GraphCanvas::GraphCanvasPropertyComponent>()
                ->Version(0)
                ->Field("VariableId", &VariablePropertiesComponent::m_variableId)
                ->Field("VariableName", &VariablePropertiesComponent::m_variableName)
                ->Field("VariableDatum", &VariablePropertiesComponent::m_variableDatum)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<VariablePropertiesComponent>("Variable Properties", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &VariablePropertiesComponent::GetTitle)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VariablePropertiesComponent::m_variableName, "Name", "")
                        ->Attribute(AZ::Edit::Attributes::StringLineEditingCompleteNotify, &VariablePropertiesComponent::OnNameChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VariablePropertiesComponent::m_variableDatum, "Datum", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    AZ::Entity* VariablePropertiesComponent::CreateVariablePropertiesEntity()
    {
        AZ::Entity* entity = aznew AZ::Entity("VariablePropertiesHelper");

        entity->CreateComponent<VariablePropertiesComponent>();

        return entity;
    }

    VariablePropertiesComponent::VariablePropertiesComponent()
        : m_variableId(ScriptCanvas::VariableId())
        , m_variableName(AZStd::string())
        , m_variableDatum(nullptr)
        , m_componentTitle("Variable")
    {
    }

    const char* VariablePropertiesComponent::GetTitle()
    {
        return m_componentTitle.c_str();
    }

    void VariablePropertiesComponent::SetVariable(const ScriptCanvas::VariableId& variableId)
    {
        if (variableId.IsValid())
        {
            ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();

            m_variableId = variableId;
            m_componentTitle.clear();
            m_variableName.clear();
            m_variableDatum = nullptr;

            ScriptCanvas::VariableRequestBus::EventResult(m_variableDatum, m_variableId, &ScriptCanvas::VariableRequests::GetVariableDatum);
            if (m_variableDatum)
            {
                ScriptCanvas::VariableRequestBus::EventResult(m_variableName, m_variableId, &ScriptCanvas::VariableRequests::GetName);

                AZStd::string_view variableTypeName = TranslationHelper::GetSafeTypeName(m_variableDatum->GetData().GetType());
                m_variableDatum->GetData().SetLabel(variableTypeName);
                m_componentTitle = AZStd::string::format("%s Variable", variableTypeName.data());

                ScriptCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
            }
            else
            {
                AZ_Error("Script Canvas", false, "Failed to find VariableDatum for valid variable id %s."
                    " Most likely the variable was removed from the GraphVariableManagerComponent without sending the OnVariableRemoved Notification", variableId.ToString().data());
            }
        }
    }

    void VariablePropertiesComponent::SetScriptCanvasGraphId(AZ::EntityId scriptCanvasGraphId)
    {
        m_scriptCanvasGraphId = scriptCanvasGraphId;
    }

    void VariablePropertiesComponent::OnNameChanged()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();

        AZ::Outcome<void, AZStd::string> outcome = AZ::Failure(AZStd::string());
        AZStd::string_view oldVariableName;
        ScriptCanvas::VariableRequestBus::EventResult(oldVariableName, m_variableId, &ScriptCanvas::VariableRequests::GetName);
        if (oldVariableName != m_variableName)
        {
            ScriptCanvas::VariableRequestBus::EventResult(outcome, m_variableId, &ScriptCanvas::VariableRequests::RenameVariable, m_variableName);

            AZ_Warning("VariablePropertiesComponent", outcome.IsSuccess(), "Could not rename variable: %s", outcome.GetError().c_str());
            if (!outcome.IsSuccess())
            {
                // Revert the variable name if we couldn't rename it (e.g. not unique)
                m_variableName = oldVariableName;
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
            else
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
            }
        }

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
    }

    void VariablePropertiesComponent::OnVariableRemoved()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
        m_variableId = ScriptCanvas::VariableId();
        m_variableName = AZStd::string();
        m_variableDatum = nullptr;
    }

    void VariablePropertiesComponent::OnVariableValueChanged()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
    }

    void VariablePropertiesComponent::OnVariableRenamed(AZStd::string_view variableName)
    {
        m_variableName = variableName;
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
    }

    void VariablePropertiesComponent::OnVariableExposureChanged()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
    }

    /////////////////////////////
    // VariablePanelContextMenu
    /////////////////////////////

    VariablePanelContextMenu::VariablePanelContextMenu(VariableDockWidget* dockWidget, const AZ::EntityId& scriptCanvasGraphId, ScriptCanvas::VariableId varId)
        : QMenu()
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasGraphId);

        AZStd::string variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, varId);

        QAction* getAction = new QAction(QObject::tr("Get %1").arg(variableName.c_str()), this);
        getAction->setToolTip(QObject::tr("Adds a Get %1 variable node onto the active graph.").arg(variableName.c_str()));
        getAction->setStatusTip(QObject::tr("Adds a Get %1 variable node onto the active graph.").arg(variableName.c_str()));
        
        QObject::connect(getAction,
                &QAction::triggered,
            [graphCanvasGraphId, varId](bool)
        {
            CreateGetVariableNodeMimeEvent mimeEvent(varId);

            AZ::EntityId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 viewCenter;
            GraphCanvas::ViewRequestBus::EventResult(viewCenter, viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

            mimeEvent.ExecuteEvent(viewCenter, viewCenter, graphCanvasGraphId);
        });
        
        QAction* setAction = new QAction(QObject::tr("Set %1").arg(variableName.c_str()), this);
        setAction->setToolTip(QObject::tr("Adds a Set %1 variable node onto the active graph.").arg(variableName.c_str()));
        setAction->setStatusTip(QObject::tr("Adds a Set %1 variable node onto the active graph.").arg(variableName.c_str()));

        QObject::connect(setAction,
                &QAction::triggered,
            [graphCanvasGraphId, varId](bool)
        {
            CreateSetVariableNodeMimeEvent mimeEvent(varId);

            AZ::EntityId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 viewCenter;
            GraphCanvas::ViewRequestBus::EventResult(viewCenter, viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

            mimeEvent.ExecuteEvent(viewCenter, viewCenter, graphCanvasGraphId);
        });

        QAction* deleteAction = new QAction(QObject::tr("Delete %1").arg(variableName.c_str()), this);
        deleteAction->setToolTip(QObject::tr("Deletes the variable called - %1").arg(variableName.c_str()));
        deleteAction->setStatusTip(QObject::tr("Deletes the variable called - %1").arg(variableName.c_str()));

        QObject::connect(deleteAction,
            &QAction::triggered,
            [dockWidget, varId](bool)
        {
            AZStd::unordered_set< ScriptCanvas::VariableId > variableIds = { varId };
            dockWidget->OnDeleteVariables(variableIds);
        });

        addAction(getAction);
        addAction(setAction);
        addSeparator();
        addAction(deleteAction);
    }

    ///////////////////////
    // VariableDockWidget
    ///////////////////////

    VariableDockWidget::VariableDockWidget(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDockWidget(parent)
        , m_manipulatingSelection(false)
        , ui(new Ui::VariableDockWidget())
    {
        ui->setupUi(this);

        ui->graphVariables->setContentsMargins(0, 0, 0, 0);
        ui->graphVariables->setContextMenuPolicy(Qt::CustomContextMenu);

        QObject::connect(ui->graphVariables, &GraphVariablesTableView::SelectionChanged, this, &VariableDockWidget::OnSelectionChanged);
        QObject::connect(ui->graphVariables, &QWidget::customContextMenuRequested, this, &VariableDockWidget::OnContextMenuRequested);

        QObject::connect(ui->searchFilter, &QLineEdit::textChanged, this, &VariableDockWidget::OnQuickFilterChanged);
        QObject::connect(ui->searchFilter, &QLineEdit::returnPressed, this, &VariableDockWidget::OnReturnPressed);

        QAction* clearAction = ui->searchFilter->addAction(QIcon(":/ScriptCanvasEditorResources/Resources/lineedit_clear.png"), QLineEdit::TrailingPosition);
        QObject::connect(clearAction, &QAction::triggered, this, &VariableDockWidget::ClearFilter);

        // Tell the widget to auto create our context menu, for now
        setContextMenuPolicy(Qt::ActionsContextMenu);

        // Add button is disabled by default, since we don't want to switch panels until we have an active scene.
        connect(ui->addButton, &QPushButton::clicked, this, &VariableDockWidget::OnAddVariableButton);
        
        ui->addButton->setEnabled(false);
        ui->searchFilter->setEnabled(false);

        QObject::connect(ui->variablePalette, &VariablePaletteTableView::CreateVariable, this, &VariableDockWidget::OnCreateVariable);
        QObject::connect(ui->graphVariables, &GraphVariablesTableView::DeleteVariables, this, &VariableDockWidget::OnDeleteVariables);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &VariableDockWidget::UpdateFilter);

        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        ShowGraphVariables();
    }

    VariableDockWidget::~VariableDockWidget()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
    }

    void VariableDockWidget::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes)
    {
        ui->variablePalette->PopulateVariablePalette(objectTypes);
    }

    void VariableDockWidget::OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId)
    {
        ClearFilter();

        m_graphCanvasGraphId = graphCanvasGraphId;

        m_scriptCanvasGraphId.SetInvalid();
        GeneralRequestBus::BroadcastResult(m_scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        ui->graphVariables->SetActiveScene(m_scriptCanvasGraphId);

        ui->addButton->setEnabled(m_scriptCanvasGraphId.IsValid());
        ui->searchFilter->setEnabled(m_scriptCanvasGraphId.IsValid());

        ShowGraphVariables();
    }

    void VariableDockWidget::OnEscape()
    {
        ShowGraphVariables();
    }

    void VariableDockWidget::focusOutEvent(QFocusEvent* focusEvent)
    {
        AzQtComponents::StyledDockWidget::focusOutEvent(focusEvent);

        if (ui->stackedWidget->currentIndex() == ui->stackedWidget->indexOf(ui->variablePalettePage))
        {
            ShowGraphVariables();
        }
    }

    bool VariableDockWidget::IsShowingVariablePalette() const
    {
        return ui->stackedWidget->currentIndex() == ui->stackedWidget->indexOf(ui->variablePalettePage);
    }

    void VariableDockWidget::ShowVariablePalette()
    {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->variablePalettePage));
        ClearFilter();

        ui->searchFilter->setPlaceholderText("Variable Type...");
        FocusOnSearchFilter();

        ui->searchFilter->setCompleter(ui->variablePalette->GetVariableCompleter());

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    bool VariableDockWidget::IsShowingGraphVariables() const
    {
        return ui->stackedWidget->currentIndex() == ui->stackedWidget->indexOf(ui->graphVariablesPage);
    }

    void VariableDockWidget::ShowGraphVariables()
    {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->graphVariablesPage));
        ClearFilter();

        ui->variablePalette->clearSelection();

        ui->searchFilter->setPlaceholderText("Search...");

        ui->searchFilter->setCompleter(nullptr);

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void VariableDockWidget::FocusOnSearchFilter()
    {
        ui->searchFilter->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void VariableDockWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(ui->searchFilter);
            ui->searchFilter->setText("");
        }

        UpdateFilter();
    }

    void VariableDockWidget::UpdateFilter()
    {
        if (IsShowingGraphVariables())
        {
            ui->graphVariables->SetFilter(ui->searchFilter->text());
        }
        else if (IsShowingVariablePalette())
        {
            ui->variablePalette->SetFilter(ui->searchFilter->userInputText());
        }
    }

    void VariableDockWidget::OnReturnPressed()
    {
        if (IsShowingVariablePalette())
        {
            ui->variablePalette->TryCreateVariableByTypeName(ui->searchFilter->text().toStdString().c_str());
        }
        else if (IsShowingGraphVariables())
        {
            UpdateFilter();
        }
    }

    void VariableDockWidget::OnQuickFilterChanged()
    {
        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void VariableDockWidget::RefreshModel()
    {
        ui->graphVariables->SetActiveScene(m_scriptCanvasGraphId);
    }

    void VariableDockWidget::OnAddVariableButton()
    {
        int index = ui->stackedWidget->currentIndex();

        // Switch between pages
        if (index == ui->stackedWidget->indexOf(ui->graphVariablesPage))
        {
            ShowVariablePalette();
        }
        else if(index == ui->stackedWidget->indexOf(ui->variablePalettePage))
        {
            ShowGraphVariables();
        }
    }

    void VariableDockWidget::OnContextMenuRequested(const QPoint& pos)
    {
        QModelIndex index = ui->graphVariables->indexAt(pos);

        // Bring up the context menu if the item is valid
        if (index.row() > -1)
        {
            ScriptCanvas::VariableId varId = index.data(GraphVariablesModel::VarIdRole).value<ScriptCanvas::VariableId>();

            VariablePanelContextMenu menu(this, m_scriptCanvasGraphId, varId);
            menu.exec(ui->graphVariables->mapToGlobal(pos));
        }
    }

    void VariableDockWidget::OnSelectionChanged(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds)
    {
        if (!variableIds.empty())
        {
            GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        }

        AZStd::vector<AZ::EntityId> selection;

        for (const ScriptCanvas::VariableId& varId : variableIds)
        {
            if (m_propertyHelpersMap.count(varId) == 0)
            {
                m_propertyHelpersMap[varId] = AZStd::unique_ptr<AZ::Entity>(VariablePropertiesComponent::CreateVariablePropertiesEntity());
                m_propertyHelpersMap[varId]->Init();
                m_propertyHelpersMap[varId]->Activate();
            }

            VariablePropertiesComponent* propertiesComponent = AZ::EntityUtils::FindFirstDerivedComponent<VariablePropertiesComponent>(m_propertyHelpersMap[varId].get());

            if (propertiesComponent)
            {
                propertiesComponent->SetVariable(varId);
                propertiesComponent->SetScriptCanvasGraphId(m_scriptCanvasGraphId);
            }

            selection.push_back(m_propertyHelpersMap[varId]->GetId());
        }

        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::SetSelection, selection);
    }

    void VariableDockWidget::OnCreateVariable(ScriptCanvas::Data::Type varType)
    {
        ShowGraphVariables();

        AZ::u32 varCounter = 0;
        SceneCounterRequestBus::EventResult(varCounter, m_scriptCanvasGraphId, &SceneCounterRequests::GetNewVariableCounter);

        AZStd::string varName = AZStd::string::format("Variable %u", varCounter);
        ScriptCanvas::Datum datum(varType, ScriptCanvas::Datum::eOriginality::Original);

        AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> outcome = AZ::Failure(AZStd::string());
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(outcome, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::AddVariable, varName, datum);

        AZ_Warning("VariablePanel", outcome.IsSuccess(), "Could not create new variable: %s", outcome.GetError().c_str());
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
    }

    void VariableDockWidget::OnDeleteVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds)
    {
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::ClearSelection);

        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);

        bool result = false;
        for (const ScriptCanvas::VariableId& variableId : variableIds)
        {
            if (CanDeleteVariable(variableId))
            {
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(result, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::RemoveVariable, variableId);
                AZ_Warning("VariablePanel", result, "Could not delete Variable Id (%s).", variableId.ToString().data());

                if (result && m_propertyHelpersMap.count(variableId) > 0)
                {
                    m_propertyHelpersMap[variableId]->Deactivate();
                    m_propertyHelpersMap.erase(variableId);
                }
            }
        }
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);

        if (result)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
        }
    }

    bool VariableDockWidget::CanDeleteVariable(const ScriptCanvas::VariableId& variableId)
    {
        bool canDeleteVariable = false;

        if (VariableGraphMemberRefCountRequestBus::FindFirstHandler(variableId) != nullptr)
        {
            AZStd::string variableName;
            ScriptCanvas::VariableRequestBus::EventResult(variableName, variableId, &ScriptCanvas::VariableRequests::GetName);

            AZStd::unordered_set< AZ::EntityId > memberIds;
            VariableGraphMemberRefCountRequestBus::EnumerateHandlersId(variableId, [&memberIds](VariableGraphMemberRefCountRequests* requests)
            {
                memberIds.insert(requests->GetGraphMemberId());
                return true;
            });

            int result = QMessageBox::warning(this, QString("Delete %1 and References").arg(variableName.c_str()), QString("The variable \"%1\" has %2 active references.\nAre you sure you want to delete the variable and its references from the graph?").arg(variableName.c_str()).arg(memberIds.size()), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Cancel);

            if (result == QMessageBox::StandardButton::Yes)
            {
                canDeleteVariable = true;
                GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, memberIds);
            }
        }
        else
        {
            canDeleteVariable = true;
        }

        return canDeleteVariable;
    }
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.moc>
}

