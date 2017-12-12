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

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include "SpecializedNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeUtils.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"

#include <Core/Attributes.h>
#include <Editor/Metrics.h>
#include <Libraries/Entity/EntityRef.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////
    // CreateEntityRefNodeMimeEvent
    /////////////////////////////////

    void CreateEntityRefNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEntityRefNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("EntityId", &CreateEntityRefNodeMimeEvent::m_entityId)
                ;
        }
    }

    CreateEntityRefNodeMimeEvent::CreateEntityRefNodeMimeEvent(const AZ::EntityId& entityId)
        : m_entityId(entityId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateEntityRefNodeMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropNode, AZ::AzTypeInfo<ScriptCanvas::Nodes::Entity::EntityRef>::Uuid(), graphId);
        return Nodes::CreateEntityNode(m_entityId, graphId);
    }

    /////////////////////////////////
    // EntityRefNodePaletteTreeItem
    /////////////////////////////////

    EntityRefNodePaletteTreeItem::EntityRefNodePaletteTreeItem(const QString& nodeName, const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, iconPath)
    {
    }

    GraphCanvas::GraphCanvasMimeEvent* EntityRefNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateEntityRefNodeMimeEvent();
    }

    ///////////////////////////////
    // CreateCommentNodeMimeEvent
    ///////////////////////////////

    void CreateCommentNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateCommentNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    NodeIdPair CreateCommentNodeMimeEvent::ConstructNode(const AZ::EntityId& sceneId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair retVal;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateCommentNodeAndActivate);

        if (graphCanvasEntity)
        {
            retVal.m_graphCanvasId = graphCanvasEntity->GetId();
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePosition);
            GraphCanvas::NodeUIRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeUIRequests::SetSelected, true);
        }

        return retVal;
    }

    bool CreateCommentNodeMimeEvent::ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId)
    {
        AZ::EntityId graphId;
        GeneralRequestBus::BroadcastResult(graphId, &GeneralRequests::GetGraphId, sceneId);
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropHandler, AZ::Uuid("{CBA20A26-1ED7-4B3A-A491-F5FF2C47BC29}"), graphId);

        NodeIdPair nodeId = ConstructNode(sceneId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            sceneDropPosition += AZ::Vector2(20, 20);
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ///////////////////////////////
    // CommentNodePaletteTreeItem
    ///////////////////////////////

    CommentNodePaletteTreeItem::CommentNodePaletteTreeItem(const QString& nodeName, const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, iconPath)
    {
        SetToolTip("Comment box for notes. Does not affect script execution or data.");
    }

    GraphCanvas::GraphCanvasMimeEvent* CommentNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateCommentNodeMimeEvent();
    }
}