
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
#include <QMenu>
#include <QMessageBox>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    /////////////////////////
    // RemoveSlotMenuAction
    /////////////////////////

    RemoveSlotMenuAction::RemoveSlotMenuAction(QObject* parent)
        : SlotContextMenuAction("Remove slot", parent)
    {
    }

    void RemoveSlotMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;

        if (GraphUtils::IsSlot(m_targetId))
        {
            NodeId nodeId;
            SlotRequestBus::EventResult(nodeId, m_targetId, &SlotRequests::GetNode);

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            GraphModelRequestBus::EventResult(enableAction, graphId, &GraphModelRequests::IsSlotRemovable, nodeId, m_targetId);
        }

        setEnabled(enableAction);
    }

    ContextMenuAction::SceneReaction RemoveSlotMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        if (GraphUtils::IsSlot(m_targetId))
        {
            bool hasConnections = false;
            SlotRequestBus::EventResult(hasConnections, m_targetId, &SlotRequests::HasConnections);

            NodeId nodeId;
            SlotRequestBus::EventResult(nodeId, m_targetId, &SlotRequests::GetNode);

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

            if (hasConnections)
            {

                GraphCanvasGraphicsView* graphicsView = nullptr;

                ViewId viewId;
                SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

                ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

                QMessageBox::StandardButton result = QMessageBox::question(graphicsView, "Slot has active connections", "The selected slot has active connections, as you sure you wish to remove it?");

                if (result == QMessageBox::StandardButton::Cancel
                    || result == QMessageBox::StandardButton::No)
                {
                    return SceneReaction::Nothing;
                }
            }

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RemoveSlot, nodeId, m_targetId);

            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    ///////////////////////////////
    // ClearConnectionsMenuAction
    ///////////////////////////////
    
    ClearConnectionsMenuAction::ClearConnectionsMenuAction(QObject* parent)
        : SlotContextMenuAction("Clear connections", parent)
    {
    }
    
    void ClearConnectionsMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;
        
        if (GraphUtils::IsSlot(m_targetId))
        {
            SlotRequestBus::EventResult(enableAction, m_targetId, &SlotRequests::HasConnections);
        }
        
        setEnabled(enableAction);
    }

    ContextMenuAction::SceneReaction ClearConnectionsMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        if (GraphUtils::IsSlot(m_targetId))
        {
            SlotRequestBus::Event(m_targetId, &SlotRequests::ClearConnections);
            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    //////////////////////////////////
    // ResetToDefaultValueMenuAction
    //////////////////////////////////

    ResetToDefaultValueMenuAction::ResetToDefaultValueMenuAction(QObject* parent)
        : SlotContextMenuAction("Reset Value", parent)
    {

    }

    void ResetToDefaultValueMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;

        if (GraphUtils::IsSlot(m_targetId))
        {
            bool isDataSlot = false;
            SlotType slotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(slotType, m_targetId, &SlotRequests::GetSlotType);

            if (slotType == SlotTypes::PropertySlot
                || slotType == SlotTypes::DataSlot)
            {
                enableAction = true;                
            }
        }

        setEnabled(enableAction);
    }

    ContextMenuAction::SceneReaction ResetToDefaultValueMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        NodeId nodeId;
        SlotRequestBus::EventResult(nodeId, m_targetId, &SlotRequests::GetNode);

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::ResetSlotToDefaultValue, nodeId, m_targetId);

        return SceneReaction::PostUndo;
    }
}