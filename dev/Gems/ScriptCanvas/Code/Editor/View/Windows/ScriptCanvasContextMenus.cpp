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
#include "precompiled.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>

#include "ScriptCanvasContextMenus.h"

namespace ScriptCanvasEditor
{
    //////////////////////////////
    // AddSelectedEntitiesAction
    //////////////////////////////

    AddSelectedEntitiesAction::AddSelectedEntitiesAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("", parent)
    {
    }

    GraphCanvas::ActionGroupId AddSelectedEntitiesAction::GetActionGroupId() const
    {
        return AZ_CRC("EntityActionGroup", 0x17e16dfe);
    }

    void AddSelectedEntitiesAction::RefreshAction(const GraphCanvas::GraphId&, const AZ::EntityId&)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        setEnabled(!selectedEntities.empty());

        if (selectedEntities.size() <= 1)
        {
            setText("Reference selected entity");
        }
        else
        {
            setText("Reference selected entities");
        }
    }

    GraphCanvas::ContextMenuAction::SceneReaction AddSelectedEntitiesAction::TriggerAction(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePos)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Vector2 addPosition = scenePos;

        for (const AZ::EntityId& id : selectedEntities)
        {
            NodeIdPair nodePair = Nodes::CreateEntityNode(id, scriptCanvasId);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, addPosition);
            addPosition += AZ::Vector2(20, 20);
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
    }

    ////////////////////////////
    // EndpointSelectionAction
    ////////////////////////////

    EndpointSelectionAction::EndpointSelectionAction(const GraphCanvas::Endpoint& proposedEndpoint)
        : QAction(nullptr)
        , m_endpoint(proposedEndpoint)
    {
        AZStd::string name;
        GraphCanvas::SlotRequestBus::EventResult(name, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetName);

        AZStd::string tooltip;
        GraphCanvas::SlotRequestBus::EventResult(tooltip, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetTooltip);

        setText(name.c_str());
        setToolTip(tooltip.c_str());
    }

    const GraphCanvas::Endpoint& EndpointSelectionAction::GetEndpoint() const
    {
        return m_endpoint;
    }

    ////////////////////////////////////
    // RemoveUnusedVariablesMenuAction
    ////////////////////////////////////

    RemoveUnusedVariablesMenuAction::RemoveUnusedVariablesMenuAction(QObject* parent)
        : SceneContextMenuAction("Variables", parent)
    {
        setToolTip("Removes all of the unused variables from the active graph");
    }

    void RemoveUnusedVariablesMenuAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        setEnabled(true);
    }

    bool RemoveUnusedVariablesMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string RemoveUnusedVariablesMenuAction::GetSubMenuPath() const
    {
        return "Remove Unused";
    }

    GraphCanvas::ContextMenuAction::SceneReaction RemoveUnusedVariablesMenuAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
        return SceneReaction::PostUndo;
    }

    ///////////////////////////////
    // SlotManipulationMenuAction
    ///////////////////////////////

    SlotManipulationMenuAction::SlotManipulationMenuAction(AZStd::string_view actionName, QObject* parent)
        : GraphCanvas::ContextMenuAction(actionName, parent)
    {
    }


    ScriptCanvas::Slot* SlotManipulationMenuAction::GetScriptCanvasSlot(const GraphCanvas::Endpoint& endpoint) const
    {
        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, endpoint.GetNodeId(), &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Endpoint scriptCanvasEndpoint;

        {
            AZStd::any* userData = nullptr;

            GraphCanvas::SlotRequestBus::EventResult(userData, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetUserData);
            ScriptCanvas::SlotId scriptCanvasSlotId = (userData && userData->is<ScriptCanvas::SlotId>()) ? *AZStd::any_cast<ScriptCanvas::SlotId>(userData) : ScriptCanvas::SlotId();

            userData = nullptr;

            GraphCanvas::NodeRequestBus::EventResult(userData, endpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId scriptCanvasNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

            scriptCanvasEndpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, scriptCanvasSlotId);
        }

        ScriptCanvas::Slot* scriptCanvasSlot = nullptr;

        ScriptCanvas::GraphRequestBus::EventResult(scriptCanvasSlot, scriptCanvasId, &ScriptCanvas::GraphRequests::FindSlot, scriptCanvasEndpoint);

        return scriptCanvasSlot;
    }

    /////////////////////////////////////////
    // ConvertVariableNodeToReferenceAction
    /////////////////////////////////////////

    ConvertVariableNodeToReferenceAction::ConvertVariableNodeToReferenceAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("Convert to References", parent)
    {
    }

    GraphCanvas::ActionGroupId ConvertVariableNodeToReferenceAction::GetActionGroupId() const
    {
        return AZ_CRC("VariableConversion", 0x157beab0);
    }

    void ConvertVariableNodeToReferenceAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        bool hasMultipleSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasMultipleSelection, graphId, &GraphCanvas::SceneRequests::HasMultipleSelection);

        // This item is added only when it's valid.
        setEnabled(!hasMultipleSelection);

        m_targetId = targetId;
    }

    GraphCanvas::ContextMenuAction::SceneReaction ConvertVariableNodeToReferenceAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool convertedNode = false;
        EditorGraphRequestBus::EventResult(convertedNode, scriptCanvasId, &EditorGraphRequests::ConvertVariableNodeToReference, m_targetId);

        if (convertedNode)
        {
            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    /////////////////////////////////////////
    // ConvertReferenceToVariableNodeAction
    /////////////////////////////////////////

    ConvertReferenceToVariableNodeAction::ConvertReferenceToVariableNodeAction(QObject* parent)
        : SlotManipulationMenuAction("Convert to Variable Node", parent)
    {
    }

    GraphCanvas::ActionGroupId ConvertReferenceToVariableNodeAction::GetActionGroupId() const
    {
        return AZ_CRC("VariableConversion", 0x157beab0);
    }


    void ConvertReferenceToVariableNodeAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;

        if (GraphCanvas::GraphUtils::IsSlot(m_targetId))
        {
            bool isDataSlot = false;
            GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::Invalid;
            GraphCanvas::SlotRequestBus::EventResult(slotType, m_targetId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::DataSlot)
            {
                GraphCanvas::DataSlotRequestBus::EventResult(enableAction, m_targetId, &GraphCanvas::DataSlotRequests::CanConvertToValue);

                if (enableAction)
                {
                    GraphCanvas::DataSlotType valueType = GraphCanvas::DataSlotType::Unknown;
                    GraphCanvas::DataSlotRequestBus::EventResult(valueType, m_targetId, &GraphCanvas::DataSlotRequests::GetDataSlotType);

                    enableAction = (valueType == GraphCanvas::DataSlotType::Reference);

                    if (enableAction)
                    {
                        GraphCanvas::Endpoint endpoint;
                        GraphCanvas::SlotRequestBus::EventResult(endpoint, m_targetId, &GraphCanvas::SlotRequests::GetEndpoint);

                        ScriptCanvas::Slot* slot = GetScriptCanvasSlot(endpoint);

                        enableAction = slot->GetVariableReference().IsValid();
                    }
                }
            }
        }

        setEnabled(enableAction);
    }

    GraphCanvas::ContextMenuAction::SceneReaction ConvertReferenceToVariableNodeAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::Endpoint endpoint;
        GraphCanvas::SlotRequestBus::EventResult(endpoint, m_targetId, &GraphCanvas::SlotRequests::GetEndpoint);

        GraphCanvas::ConnectionType connectionType = GraphCanvas::CT_Invalid;
        GraphCanvas::SlotRequestBus::EventResult(connectionType, m_targetId, &GraphCanvas::SlotRequests::GetConnectionType);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Slot* scriptCanvasSlot = GetScriptCanvasSlot(endpoint);

        if (scriptCanvasSlot == nullptr
            || !scriptCanvasSlot->IsVariableReference())
        {
            return SceneReaction::Nothing;
        }

        // Store the variable then convert the slot to a value for the next step.
        ScriptCanvas::VariableId variableId = scriptCanvasSlot->GetVariableReference();
        GraphCanvas::DataSlotRequestBus::Event(m_targetId, &GraphCanvas::DataSlotRequests::ConvertToValue);

        AZ::EntityId createdNodeId;

        if (connectionType == GraphCanvas::CT_Input)
        {
            CreateGetVariableNodeMimeEvent createMimeEvent(variableId);

            createdNodeId = createMimeEvent.CreateSplicingNode(graphId);
        }
        else if (connectionType == GraphCanvas::CT_Output)
        {
            CreateSetVariableNodeMimeEvent createMimeEvent(variableId);

            createdNodeId = createMimeEvent.CreateSplicingNode(graphId);
        }

        if (!createdNodeId.IsValid())
        {
            return SceneReaction::Nothing;
        }

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, createdNodeId, scenePos);

        GraphCanvas::CreateConnectionsBetweenConfig createConnectionBetweenConfig;

        createConnectionBetweenConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SingleConnection;
        createConnectionBetweenConfig.m_createModelConnections = true;

        GraphCanvas::GraphUtils::CreateConnectionsBetween({ endpoint }, createdNodeId, createConnectionBetweenConfig);

        if (!createConnectionBetweenConfig.m_createdConnections.empty())
        {
            GraphCanvas::Endpoint otherEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, (*createConnectionBetweenConfig.m_createdConnections.begin()), &GraphCanvas::ConnectionRequests::FindOtherEndpoint, endpoint);

            if (otherEndpoint.IsValid())
            {
                QPointF jutDirection;
                GraphCanvas::SlotUIRequestBus::EventResult(jutDirection, endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetJutDirection);

                AZ::EntityId gridId;
                GraphCanvas::SceneRequestBus::EventResult(gridId, graphId, &GraphCanvas::SceneRequests::GetGrid);

                AZ::Vector2 minorStep(0,0);
                GraphCanvas::GridRequestBus::EventResult(minorStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                jutDirection.setX(jutDirection.x() * minorStep.GetX() * 3.0f);
                jutDirection.setY(jutDirection.y() * minorStep.GetY() * 3.0f);

                QPointF finalPosition;
                GraphCanvas::SlotUIRequestBus::EventResult(finalPosition, endpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                finalPosition += jutDirection;

                // To deal with resizing. Move it back an extra half step so it'll grow into dead space rather then node space.
                if (jutDirection.x() < 0)
                {
                    finalPosition.setX(finalPosition.x() - minorStep.GetX() * 0.5f);
                }

                QPointF originalPosition;
                GraphCanvas::SlotUIRequestBus::EventResult(originalPosition, otherEndpoint.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                AZ::Vector2 difference = GraphCanvas::ConversionUtils::QPointToVector(finalPosition - originalPosition);

                AZ::Vector2 originalCorner;
                GraphCanvas::GeometryRequestBus::EventResult(originalCorner, otherEndpoint.GetNodeId(), &GraphCanvas::GeometryRequests::GetPosition);

                AZ::Vector2 finalCorner = originalCorner + difference;
                GraphCanvas::GeometryRequestBus::Event(otherEndpoint.GetNodeId(), &GraphCanvas::GeometryRequests::SetPosition, finalCorner);
            }
        }

        AZStd::vector<AZ::EntityId> slotIds;

        GraphCanvas::NodeRequestBus::EventResult(slotIds, endpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetSlotIds);        

        GraphCanvas::ConnectionSpliceConfig spliceConfig;
        spliceConfig.m_allowOpportunisticConnections = false;

        bool connectedExecution = false;
        AZStd::vector< GraphCanvas::Endpoint > validInputSlots;

        for (auto slotId : slotIds)
        {
            GraphCanvas::SlotRequests* slotRequests = GraphCanvas::SlotRequestBus::FindFirstHandler(slotId);

            if (slotRequests == nullptr)
            {
                continue;
            }

            GraphCanvas::SlotType slotType = slotRequests->GetSlotType();

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                GraphCanvas::ConnectionType testConnectionType = slotRequests->GetConnectionType();

                // We only want to connect to things going in the same direction as we are.
                if (testConnectionType == connectionType)
                {
                    validInputSlots.emplace_back(endpoint.GetNodeId(), slotId);

                    AZStd::vector< AZ::EntityId > connectionIds = slotRequests->GetConnections();

                    for (const AZ::EntityId& connectionId : connectionIds)
                    {
                        if (GraphCanvas::GraphUtils::SpliceNodeOntoConnection(createdNodeId, connectionId, spliceConfig))
                        {
                            connectedExecution = true;
                        }
                    }
                }
            }

            if (!connectedExecution)
            {
                GraphCanvas::CreateConnectionsBetweenConfig fallbackConnectionConfig;

                fallbackConnectionConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SinglePass;
                fallbackConnectionConfig.m_createModelConnections = true;

                GraphCanvas::GraphUtils::CreateConnectionsBetween(validInputSlots, createdNodeId, fallbackConnectionConfig);
            }
        }

        GraphCanvas::NodeNudgingController nudgingController;

        nudgingController.SetGraphId(graphId);
        nudgingController.StartNudging({ createdNodeId });
        nudgingController.FinalizeNudging();

        GraphCanvas::AnimatedPulseConfiguration animatedPulseConfig;

        animatedPulseConfig.m_enableGradient = true;
        animatedPulseConfig.m_drawColor = QColor(255, 255, 255);
        animatedPulseConfig.m_durationSec = 0.25f;

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::CreatePulseAroundSceneMember, createdNodeId, 4, animatedPulseConfig);

        return SceneReaction::PostUndo;
    }
    
    /////////////////////
    // SceneContextMenu
    /////////////////////

    SceneContextMenu::SceneContextMenu(const NodePaletteModel& paletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : GraphCanvas::SceneContextMenu(ScriptCanvasEditor::AssetEditorId)
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);

        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(paletteModel, assetModel, inContextMenu);

        m_palette = aznew Widget::NodePaletteDockWidget(tr("Node Palette"), this, paletteConfig);

        actionWidget->setDefaultWidget(m_palette);

        GraphCanvas::ContextMenuAction* menuAction = aznew AddSelectedEntitiesAction(this);
        
        AddActionGroup(menuAction->GetActionGroupId());
        AddMenuAction(menuAction);

        AddMenuAction(actionWidget);

        connect(this, &QMenu::aboutToShow, this, &SceneContextMenu::SetupDisplay);
        connect(m_palette, &Widget::NodePaletteDockWidget::OnContextMenuSelection, this, &SceneContextMenu::HandleContextMenuSelection);
    }

    void SceneContextMenu::ResetSourceSlotFilter()
    {
        m_palette->ResetSourceSlotFilter();
    }

    void SceneContextMenu::FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId)
    {
        m_palette->FilterForSourceSlot(scriptCanvasGraphId, sourceSlotId);
    }

    const Widget::NodePaletteDockWidget* SceneContextMenu::GetNodePalette() const
    {
        return m_palette;
    }

    void SceneContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::SceneContextMenu::OnRefreshActions(graphId, targetMemberId);

        // Don't want to overly manipulate the state. So we only modify this when we know we want to turn it on.
        if (GraphVariablesTableView::HasCopyVariableData())
        {
            m_editorActionsGroup.SetPasteEnabled(true);
        }
    }

    void SceneContextMenu::HandleContextMenuSelection()
    {
        close();
    }

    void SceneContextMenu::SetupDisplay()
    {
        m_palette->ResetDisplay();
        m_palette->FocusOnSearchFilter();
    }

    void SceneContextMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        if (!m_palette->hasFocus())
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }

    //////////////////////////
    // ConnectionContextMenu
    //////////////////////////

    ConnectionContextMenu::ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : GraphCanvas::ConnectionContextMenu(ScriptCanvasEditor::AssetEditorId)
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);

        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(nodePaletteModel, assetModel, inContextMenu);
        m_palette = aznew Widget::NodePaletteDockWidget(tr("Node Palette"), this, paletteConfig);

        actionWidget->setDefaultWidget(m_palette);

        AddMenuAction(actionWidget);

        connect(this, &QMenu::aboutToShow, this, &ConnectionContextMenu::SetupDisplay);
        connect(m_palette, &Widget::NodePaletteDockWidget::OnContextMenuSelection, this, &ConnectionContextMenu::HandleContextMenuSelection);
    }

    const Widget::NodePaletteDockWidget* ConnectionContextMenu::GetNodePalette() const
    {
        return m_palette;
    }

    void ConnectionContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::ConnectionContextMenu::OnRefreshActions(graphId, targetMemberId);

        m_palette->ResetSourceSlotFilter();

        m_connectionId = targetMemberId;
        
        // TODO: Filter nodes.
    }

    void ConnectionContextMenu::HandleContextMenuSelection()
    {
        close();
    }

    void ConnectionContextMenu::SetupDisplay()
    {
        m_palette->ResetDisplay();
        m_palette->FocusOnSearchFilter();
    }

    void ConnectionContextMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        if (!m_palette->hasFocus())
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }

    #include "Editor/View/Windows/ScriptCanvasContextMenus.moc"
}

