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

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuActions.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    //////////////////////////////
    // CreateNodeGroupMenuAction
    //////////////////////////////

    CreateNodeGroupMenuAction::CreateNodeGroupMenuAction(QObject* parent, bool collapseGroup)
        : NodeGroupContextMenuAction("", parent)
        , m_collapseGroup(collapseGroup)
    {
        if (!collapseGroup)
        {
            setText("Group");
            setToolTip("Will create a Node Group for the selected nodes.");
        }
        else
        {
            setText("Group [Collapsed]");
            setToolTip("Will create a Node Group for the selected nodes, and then collapse the group to a single node.");
        }
    }

    void CreateNodeGroupMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        bool hasSelection = false;
        SceneRequestBus::EventResult(hasSelection, graphId, &SceneRequests::HasSelectedItems);

        if (hasSelection)
        {
            hasSelection = !GraphUtils::IsNodeGroup(targetId);
        }

        setEnabled(hasSelection);
    }

    ContextMenuAction::SceneReaction CreateNodeGroupMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        bool hasSelection = false;
        SceneRequestBus::EventResult(hasSelection, graphId, &SceneRequests::HasSelectedItems);

        AZ::Entity* nodeGroupEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(nodeGroupEntity, &GraphCanvasRequests::CreateNodeGroupAndActivate);

        if (nodeGroupEntity)
        {
            SceneRequestBus::Event(graphId, &SceneRequests::AddNode, nodeGroupEntity->GetId(), scenePos);

            if (hasSelection)
            {
                AZStd::vector< AZ::EntityId > selectedNodes;
                SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

                QGraphicsItem* rootItem = nullptr;
                QRectF boundingArea;

                for (const AZ::EntityId& selectedNode : selectedNodes)
                {
                    SceneMemberUIRequestBus::EventResult(rootItem, selectedNode, &SceneMemberUIRequests::GetRootGraphicsItem);

                    if (rootItem)
                    {
                        if (boundingArea.isEmpty())
                        {
                            boundingArea = rootItem->sceneBoundingRect();
                        }
                        else
                        {
                            boundingArea = boundingArea.united(rootItem->sceneBoundingRect());
                        }
                    }
                }

                AZ::Vector2 gridStep;
                AZ::EntityId grid;
                SceneRequestBus::EventResult(grid, graphId, &SceneRequests::GetGrid);

                GridRequestBus::EventResult(gridStep, grid, &GridRequests::GetMinorPitch);

                boundingArea.adjust(-gridStep.GetX(), -gridStep.GetY(), gridStep.GetX(), gridStep.GetY());

                NodeGroupRequestBus::Event(nodeGroupEntity->GetId(), &NodeGroupRequests::SetGroupSize, boundingArea);
            }

            SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);

            if (!m_collapseGroup)
            {
                SceneMemberUIRequestBus::Event(nodeGroupEntity->GetId(), &SceneMemberUIRequests::SetSelected, true);
                CommentUIRequestBus::Event(nodeGroupEntity->GetId(), &CommentUIRequests::SetEditable, true);
            }
            else
            {
                NodeGroupRequestBus::Event(nodeGroupEntity->GetId(), &NodeGroupRequests::CollapseGroup);
            }
        }

        if (nodeGroupEntity)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }

    /////////////////////////////////
    // CreateNewNodeGroupMenuAction
    /////////////////////////////////

    CreateNewNodeGroupMenuAction::CreateNewNodeGroupMenuAction(QObject* parent)
        : CreateNodeGroupMenuAction(parent, false)
    {
        setText("New group");
        setToolTip("Creates an empty Node Group.");
    }

    void CreateNewNodeGroupMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        setEnabled(true);
    }

    ///////////////////////////////
    // UngroupNodeGroupMenuAction
    ///////////////////////////////

    UngroupNodeGroupMenuAction::UngroupNodeGroupMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Ungroup", parent)
    {

    }

    void UngroupNodeGroupMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        setEnabled((GraphUtils::IsNodeGroup(targetId) || GraphUtils::IsCollapsedNodeGroup(targetId)));

        if (isEnabled())
        {
            if (GraphUtils::IsCollapsedNodeGroup(targetId))
            {
                CollapsedNodeGroupRequestBus::EventResult(m_groupTarget, targetId, &CollapsedNodeGroupRequests::GetSourceGroup);
            }
            else
            {
                m_groupTarget = targetId;
            }
        }
        else
        {
            m_groupTarget.SetInvalid();
        }
    }

    ContextMenuAction::SceneReaction UngroupNodeGroupMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        SceneReaction reaction = SceneReaction::Nothing;

        if (m_groupTarget.IsValid())
        {
            NodeGroupRequestBus::Event(m_groupTarget, &NodeGroupRequests::UngroupGroup);
            reaction = SceneReaction::PostUndo;
        }

        return reaction;
    }

    ////////////////////////////////
    // CollapseNodeGroupMenuAction
    ////////////////////////////////

    CollapseNodeGroupMenuAction::CollapseNodeGroupMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Collapse", parent)
    {
        setToolTip("Collapses the selected group");
    }

    void CollapseNodeGroupMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);

        m_groupTarget = targetId;
        setEnabled(GraphUtils::IsNodeGroup(targetId));
    }

    ContextMenuAction::SceneReaction CollapseNodeGroupMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        NodeGroupRequestBus::Event(m_groupTarget, &NodeGroupRequests::CollapseGroup);

        return SceneReaction::PostUndo;
    }

    //////////////////////////////
    // ExpandNodeGroupMenuAction
    //////////////////////////////

    ExpandNodeGroupMenuAction::ExpandNodeGroupMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Expand", parent)
    {
        setToolTip("Expands the selected group");
    } 

    void ExpandNodeGroupMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);

        m_groupTarget = targetId;
        setEnabled(GraphUtils::IsCollapsedNodeGroup(targetId));
    }

    ContextMenuAction::SceneReaction ExpandNodeGroupMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        CollapsedNodeGroupRequestBus::Event(m_groupTarget, &CollapsedNodeGroupRequests::ExpandGroup);

        return SceneReaction::PostUndo;
    }

    /////////////////////////////
    // EditGroupTitleMenuAction
    /////////////////////////////

    EditGroupTitleMenuAction::EditGroupTitleMenuAction(QObject* parent)
        : NodeGroupContextMenuAction("Edit group title", parent)
    {
        setToolTip("Edits the selected group title");
    }

    void EditGroupTitleMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);

        m_groupTarget = targetId;
        setEnabled(GraphUtils::IsNodeGroup(targetId));
    }

    ContextMenuAction::SceneReaction EditGroupTitleMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
    {
        CommentUIRequestBus::Event(m_groupTarget, &CommentUIRequests::SetEditable, true);

        return SceneReaction::Nothing;
    }
}