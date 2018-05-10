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

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include "CreateNodeMimeEvent.h"

#include "ScriptCanvas/Bus/RequestBus.h"

namespace ScriptCanvasEditor
{
    ////////////////////////
    // CreateNodeMimeEvent
    ////////////////////////

    void CreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateNodeMimeEvent, GraphCanvas::CreateSplicingNodeMimeEvent>()
                ->Version(0)
                ;
        }
    }

    const ScriptCanvasEditor::NodeIdPair& CreateNodeMimeEvent::GetCreatedPair() const
    {
        return m_nodeIdPair;
    }
        
    bool CreateNodeMimeEvent::ExecuteEvent(const AZ::Vector2&, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        if (!scriptCanvasGraphId.IsValid() || !graphCanvasGraphId.IsValid())
        {
            return false;
        }

        m_nodeIdPair = CreateNode(scriptCanvasGraphId);

        if (m_nodeIdPair.m_graphCanvasId.IsValid() && m_nodeIdPair.m_scriptCanvasId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, m_nodeIdPair.m_graphCanvasId, sceneDropPosition);
            GraphCanvas::SceneMemberUIRequestBus::Event(m_nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

            ScriptCanvasEditor::NodeCreationNotificationBus::Event(scriptCanvasGraphId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, m_nodeIdPair.m_graphCanvasId);

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
            return true;
        }
        else
        {
            if (m_nodeIdPair.m_graphCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_graphCanvasId);
            }

            if (m_nodeIdPair.m_scriptCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_scriptCanvasId);
            }
        }

        return false;
    }

    AZ::EntityId CreateNodeMimeEvent::CreateSplicingNode(const AZ::EntityId& graphCanvasGraphId)
    {
        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        ScriptCanvasEditor::NodeIdPair idPair = CreateNode(scriptCanvasGraphId);

        if (idPair.m_graphCanvasId.IsValid() && idPair.m_scriptCanvasId.IsValid())
        {
            return idPair.m_graphCanvasId;
        }

        return AZ::EntityId();
    }

    ///////////////////////////////////
    // SpecializedCreateNodeMimeEvent
    ///////////////////////////////////

    void SpecializedCreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<SpecializedCreateNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }
}