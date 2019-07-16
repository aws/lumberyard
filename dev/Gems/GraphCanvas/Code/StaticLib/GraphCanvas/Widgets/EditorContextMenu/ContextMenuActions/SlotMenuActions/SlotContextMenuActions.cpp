
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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SlotMenuActions/SlotContextMenuActions.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
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
}