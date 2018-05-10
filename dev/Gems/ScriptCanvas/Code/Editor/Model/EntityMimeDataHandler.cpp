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
#include "EntityMimeDataHandler.h"

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsView>

#include <Editor/Nodes/NodeUtils.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <Core/GraphBus.h>

namespace ScriptCanvasEditor
{
    namespace EntityMimeData
    {
        static QString GetMimeType()
        {
            return AzToolsFramework::EditorEntityIdContainer::GetMimeType();
        }
    }

    void EntityMimeDataHandler::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<EntityMimeDataHandler, AZ::Component>()
            ->Version(1)
            ;
    }

    EntityMimeDataHandler::EntityMimeDataHandler()
    {}

    void EntityMimeDataHandler::Activate()
    {
        GraphCanvas::SceneMimeDelegateHandlerRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EntityMimeDataHandler::Deactivate()
    {
        GraphCanvas::SceneMimeDelegateHandlerRequestBus::Handler::BusDisconnect();
    }
    
    bool EntityMimeDataHandler::IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* mimeData)
    {        
        (void)sceneId;

        return mimeData->hasFormat(EntityMimeData::GetMimeType());
    }    

    void EntityMimeDataHandler::HandleMove(const AZ::EntityId&, const QPointF&, const QMimeData*)
    {        
    }

    void EntityMimeDataHandler::HandleDrop(const AZ::EntityId& graphCanvasGraphId, const QPointF& dropPoint, const QMimeData* mimeData)
    {
        if (!mimeData->hasFormat(EntityMimeData::GetMimeType()))
        {
            return;
        }

        QByteArray arrayData = mimeData->data(EntityMimeData::GetMimeType());

        AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
        if (!entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()) || entityIdListContainer.m_entityIds.empty())
        {
            return;
        }

        bool areEntitiesEditable = true;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(areEntitiesEditable, &AzToolsFramework::ToolsApplicationRequests::AreEntitiesEditable, entityIdListContainer.m_entityIds);
        if (!areEntitiesEditable)
        {
            return;
        }
        
        AZ::Vector2 pos(dropPoint.x(), dropPoint.y());

        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        for (const AZ::EntityId& id : entityIdListContainer.m_entityIds)
        {
            NodeIdPair nodePair = Nodes::CreateEntityNode(id, scriptCanvasGraphId);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, pos);
            pos += AZ::Vector2(20, 20);
        }

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasGraphId);        
    }

    void EntityMimeDataHandler::HandleLeave(const AZ::EntityId&, const QMimeData*)
    {

    }
} // namespace GraphCanvas