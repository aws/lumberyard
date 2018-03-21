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

#include <functional>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QMimeData>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/sort.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Components/SceneComponent.h>

#include <Components/ColorPaletteManager/ColorPaletteManagerBus.h>
#include <Components/Connections/ConnectionComponent.h>
#include <Components/GridComponent.h>
#include <Components/Nodes/NodeComponent.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Types/SceneSerialization.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <Styling/Parser.h>

namespace GraphCanvas
{
    //////////////////////
    // SceneMimeDelegate
    //////////////////////

    void SceneMimeDelegate::Activate()
    {
        SceneMimeDelegateHandlerRequestBus::Handler::BusConnect(m_sceneId);
        SceneMimeDelegateRequestBus::Event(m_sceneId, &SceneMimeDelegateRequests::AddDelegate, m_sceneId);
    }

    void SceneMimeDelegate::Deactivate()
    {
        SceneMimeDelegateRequestBus::Event(m_sceneId, &SceneMimeDelegateRequests::RemoveDelegate, m_sceneId);
        SceneMimeDelegateHandlerRequestBus::Handler::BusDisconnect();
    }

    void SceneMimeDelegate::SetSceneId(const AZ::EntityId& sceneId)
    {
        m_sceneId = sceneId;
    }

    void SceneMimeDelegate::SetMimeType(const char* mimeType)
    {
        m_mimeType = mimeType;
    }

    bool SceneMimeDelegate::IsInterestedInMimeData(const AZ::EntityId&, const QMimeData* mimeData) const
    {
        return !m_mimeType.isEmpty() && mimeData->hasFormat(m_mimeType);
    }

    void SceneMimeDelegate::HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData)
    {
        if (!mimeData->hasFormat(m_mimeType))
        {
            AZ_Error("SceneMimeDelegate", false, "Handling an event that does not meet our Mime requirements");
            return;
        }

        QByteArray arrayData = mimeData->data(m_mimeType);

        GraphCanvasMimeContainer mimeContainer;

        if (!mimeContainer.FromBuffer(arrayData.constData(), arrayData.size()) || mimeContainer.m_mimeEvents.empty())
        {
            return;
        }

        bool success = false;

        AZ::Vector2 sceneMousePoint = AZ::Vector2(dropPoint.x(), dropPoint.y());
        AZ::Vector2 sceneDropPoint = sceneMousePoint;

        SceneRequestBus::Event(m_sceneId, &SceneRequests::ClearSelection);

        for (GraphCanvasMimeEvent* mimeEvent : mimeContainer.m_mimeEvents)
        {
            if (mimeEvent->ExecuteEvent(sceneMousePoint, sceneDropPoint, m_sceneId))
            {
                success = true;
            }
        }

        if (success)
        {
            SceneNotificationBus::Event(m_sceneId, &SceneNotifications::PostCreationEvent);
        }
    }

    ///////////////
    // Copy Utils
    ///////////////

    void SerializeToBuffer(const SceneSerialization& serializationTarget, AZStd::vector<char>& buffer)
    {
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();

        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
        AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &serializationTarget, serializeContext);
    }

    void SerializeToClipboard(const SceneSerialization& serializationTarget)
    {
        AZ_Error("Graph Canvas", !serializationTarget.GetSerializationKey().empty(), "Serialization Key not server for scene serialization. Cannot push to clipboard.");
        if (serializationTarget.GetSerializationKey().empty())
        {
            return;
        }

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();

        AZStd::vector<char> buffer;
        SerializeToBuffer(serializationTarget, buffer);

        QMimeData* mime = new QMimeData();
        mime->setData(serializationTarget.GetSerializationKey().c_str(), QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setMimeData(mime);
    }

    ///////////////////
    // SceneComponent
    ///////////////////

    void BuildEndpointMap(SceneData& sceneData)
    {
        sceneData.m_endpointMap.clear();
        for (auto& connectionEntity : sceneData.m_connections)
        {
            auto* connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(connectionEntity) : nullptr;
            if (connection)
            {
                sceneData.m_endpointMap.emplace(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
                sceneData.m_endpointMap.emplace(connection->GetTargetEndpoint(), connection->GetSourceEndpoint());
            }
        }
    }

    class SceneDataEventHandler : public AZ::SerializeContext::IEventHandler
    {
    public:
        /// Called to rebuild the Endpoint map
        void OnWriteEnd(void* classPtr) override
        {
            auto* sceneData = reinterpret_cast<SceneData*>(classPtr);

            BuildEndpointMap((*sceneData));
        }
    };

    static const char* k_copyPasteKey = "GraphCanvasScene";

    void SceneComponent::Reflect(AZ::ReflectContext* context)
    {
        SceneSerialization::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }
            
        serializeContext->Class<SceneData>()
            ->EventHandler<SceneDataEventHandler>()
            ->Field("m_nodes", &SceneData::m_nodes)
            ->Field("m_connections", &SceneData::m_connections)
            ->Field("m_userData", &SceneData::m_userData)
        ;

        serializeContext->Class<ViewParams>()
            ->Version(1)
            ->Field("Scale", &ViewParams::m_scale)
            ->Field("AnchorX", &ViewParams::m_anchorPointX)
            ->Field("AnchorY", &ViewParams::m_anchorPointY)
        ;

        serializeContext->Class<SceneComponent>()
            ->Version(3)
            ->Field("SceneData", &SceneComponent::m_sceneData)
            ->Field("ViewParams", &SceneComponent::m_viewParams)
        ;
    }

    SceneComponent::SceneComponent()
        : m_allowReset(false)
        , m_deleteCount(0)
        , m_dragSelectionType(DragSelectionType::OnRelease)
        , m_activateScene(true)
        , m_isDragSelecting(false)
        , m_ignoreSelectionChanges(false)
        , m_originalPosition(0,0)
        , m_isDraggingEntity(false)
    {
    }

    SceneComponent::~SceneComponent()
    {
        DestroyItems(m_sceneData.m_nodes);
        DestroyItems(m_sceneData.m_connections);
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, m_grid);
    }

    void SceneComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();

        AZ::Entity* gridEntity = GridComponent::CreateDefaultEntity();
        m_grid = gridEntity->GetId();

        InitItems(m_sceneData.m_nodes);
        InitConnections();
    }

    void SceneComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();

        // Need to register this before activating saved nodes. Otherwise data is not properly setup.
        SceneRequestBus::Handler::BusConnect(GetEntityId());
        SceneMimeDelegateRequestBus::Handler::BusConnect(GetEntityId());

        // Make the QGraphicsScene Ui element for managing Qt scene items
        m_graphicsSceneUi = AZStd::make_unique<GraphCanvasGraphicsScene>(*this);
        m_activateScene = true;
    }

    void SceneComponent::Deactivate()
    {
        m_mimeDelegate.Deactivate();

        GraphCanvasPropertyComponent::Deactivate();
            
        SceneMimeDelegateRequestBus::Handler::BusDisconnect();
        SceneRequestBus::Handler::BusDisconnect();

        m_activeDelegates.clear();

        DeactivateItems(m_sceneData.m_connections);
        DeactivateItems(m_sceneData.m_nodes);
        DeactivateItems(AZStd::initializer_list<AZ::Entity*>{ AzToolsFramework::GetEntity(m_grid) });
        SceneMemberRequestBus::Event(m_grid, &SceneMemberRequests::ClearScene, GetEntityId());

        // Reset GraphicsSceneUi
        m_graphicsSceneUi.reset();
    }

    AZStd::any* SceneComponent::GetUserData()
    {
        return &m_sceneData.m_userData;
    }

    const AZStd::any* SceneComponent::GetUserDataConst() const
    {
        return &m_sceneData.m_userData;
    }

    AZ::EntityId SceneComponent::GetGrid() const
    {
        return m_grid;
    }

    bool SceneComponent::AddNode(const AZ::EntityId& nodeId, const AZ::Vector2& position)
    {
        AZ_Assert(nodeId.IsValid(), "Node ID %s is not valid!", nodeId.ToString().data());

        AZ::Entity* nodeEntity(AzToolsFramework::GetEntity(nodeId));
        AZ_Assert(nodeEntity, "Node (ID: %s) Entity not found!", nodeId.ToString().data());
        AZ_Assert(nodeEntity->GetState() == AZ::Entity::ES_ACTIVE, "Only active node entities can be added to a scene");

        QGraphicsItem* item = nullptr;
        SceneMemberUIRequestBus::EventResult(item, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);
        AZ_Assert(item && !item->parentItem(), "Nodes must have a \"root\", unparented visual/QGraphicsItem");

        auto foundIt = AZStd::find_if(m_sceneData.m_nodes.begin(), m_sceneData.m_nodes.end(), [&nodeEntity](const AZ::Entity* node) { return node->GetId() == nodeEntity->GetId(); });
        if (foundIt == m_sceneData.m_nodes.end())
        {
            m_sceneData.m_nodes.emplace(nodeEntity);
            AddSceneMember(nodeId, true, position);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodeAdded, nodeId);

            return true;
        }

        return false;
    }

    void SceneComponent::AddNodes(const AZStd::vector<AZ::EntityId>& nodeIds)
    {
        for (const auto& nodeId : nodeIds)
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, nodeId, &GeometryRequests::GetPosition);
            AddNode(nodeId, position);
        }
    }

    bool SceneComponent::RemoveNode(const AZ::EntityId& nodeId)
    {
        AZ_Assert(nodeId.IsValid(), "Node ID %s is not valid!", nodeId.ToString().data());

        auto foundIt = AZStd::find_if(m_sceneData.m_nodes.begin(), m_sceneData.m_nodes.end(), [&nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
        if (foundIt != m_sceneData.m_nodes.end())
        {
            GeometryNotificationBus::MultiHandler::BusDisconnect(nodeId);
            m_sceneData.m_nodes.erase(foundIt);

            QGraphicsItem* item = nullptr;
            SceneMemberUIRequestBus::EventResult(item, nodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (item && item->scene() == m_graphicsSceneUi.get())
            {
                m_graphicsSceneUi->removeItem(item);
            }

            UnregisterSelectionItem(nodeId);            
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodeRemoved, nodeId);
            SceneMemberRequestBus::Event(nodeId, &SceneMemberRequests::ClearScene, GetEntityId());

            return true;
        }

        return false;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetNodes() const
    {
        AZStd::vector<AZ::EntityId> result;

        for (const auto& nodeRef : m_sceneData.m_nodes)
        {
            result.push_back(nodeRef->GetId());
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedNodes() const
    {
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity{};
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);
                    if (entity && AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(entity))
                    {
                        result.push_back(entity->GetId());
                    }
                }
            }
        }

        return result;
    }

    bool SceneComponent::AddConnection(const AZ::EntityId& connectionId)
    {
        AZ_Assert(connectionId.IsValid(), "Connection ID %s is not valid!", connectionId.ToString().data());

        AZ::Entity* connectionEntity(AzToolsFramework::GetEntity(connectionId));
        auto connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(connectionEntity) : nullptr;
        AZ_Warning("Graph Canvas", connection->GetEntity()->GetState() == AZ::Entity::ES_ACTIVE, "Only active connection entities can be added to a scene");
        AZ_Warning("Graph Canvas", connection, "Couldn't find the connection's component (ID: %s)!", connectionId.ToString().data());

        if (connection)
        {
            auto foundIt = AZStd::find_if(m_sceneData.m_connections.begin(), m_sceneData.m_connections.end(), [&connection](const AZ::Entity* connectionEntity) { return connectionEntity->GetId() == connection->GetEntityId(); });
            if (foundIt == m_sceneData.m_connections.end())
            {
                AddSceneMember(connectionId);

                m_sceneData.m_connections.emplace(connection->GetEntity());
                Endpoint sourceEndpoint;
                ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);
                Endpoint targetEndpoint;
                ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);
                m_sceneData.m_endpointMap.emplace(sourceEndpoint, targetEndpoint);
                m_sceneData.m_endpointMap.emplace(targetEndpoint, sourceEndpoint);

                SlotRequestBus::Event(sourceEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, connectionId, targetEndpoint);
                SlotRequestBus::Event(targetEndpoint.GetSlotId(), &SlotRequests::AddConnectionId, connectionId, sourceEndpoint);

                return true;
            }
        }

        return false;
    }

    void SceneComponent::AddConnections(const AZStd::vector<AZ::EntityId>& connectionIds)
    {
        for (const auto& connectionId : connectionIds)
        {
            AddConnection(connectionId);
        }
    }

    bool SceneComponent::RemoveConnection(const AZ::EntityId& connectionId)
    {
        AZ_Assert(connectionId.IsValid(), "Connection ID %s is not valid!", connectionId.ToString().data());

        auto foundIt = AZStd::find_if(m_sceneData.m_connections.begin(), m_sceneData.m_connections.end(), [&connectionId](const AZ::Entity* connection) { return connection->GetId() == connectionId; });
        if (foundIt != m_sceneData.m_connections.end())
        {
            GeometryNotificationBus::MultiHandler::BusDisconnect(connectionId);

            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &ConnectionRequests::GetTargetEndpoint);
            m_sceneData.m_endpointMap.erase(sourceEndpoint);
            m_sceneData.m_endpointMap.erase(targetEndpoint);
            m_sceneData.m_connections.erase(foundIt);

            QGraphicsItem* item{};
            SceneMemberUIRequestBus::EventResult(item, connectionId, &SceneMemberUIRequests::GetRootGraphicsItem);
            AZ_Assert(item, "Connections must have a visual/QGraphicsItem");
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->removeItem(item);
            }

            UnregisterSelectionItem(connectionId);

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnConnectionRemoved, connectionId);
            SlotRequestBus::Event(targetEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, connectionId, sourceEndpoint);
            SlotRequestBus::Event(sourceEndpoint.GetSlotId(), &SlotRequests::RemoveConnectionId, connectionId, targetEndpoint);

            return true;
        }

        return false;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetConnections() const
    {
        AZStd::vector<AZ::EntityId> result;

        for (const auto& connection : m_sceneData.m_connections)
        {
            result.push_back(connection->GetId());
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedConnections() const
    {
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    AZ::Entity* entity{};
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entry->second);
                    if (entity && AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(entity))
                    {
                        result.push_back(entity->GetId());
                    }
                }
            }
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetConnectionsForEndpoint(const Endpoint& firstEndpoint) const
    {
        AZStd::vector<AZ::EntityId> result;
        for (const auto& connection : m_sceneData.m_connections)
        {
            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, connection->GetId(), &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, connection->GetId(), &ConnectionRequests::GetTargetEndpoint);

            if (sourceEndpoint == firstEndpoint || targetEndpoint == firstEndpoint)
            {
                result.push_back(connection->GetId());
            }
        }

        return result;
    }

    bool SceneComponent::IsEndpointConnected(const Endpoint& endpoint) const
    {
        return m_sceneData.m_endpointMap.count(endpoint) > 0;
    }

    AZStd::vector<Endpoint> SceneComponent::GetConnectedEndpoints(const Endpoint& firstEndpoint) const
    {
        AZStd::vector<Endpoint> connectedEndpoints;
        auto otherEndpointsRange = m_sceneData.m_endpointMap.equal_range(firstEndpoint);
        for (auto otherIt = otherEndpointsRange.first; otherIt != otherEndpointsRange.second; ++otherIt)
        {
            connectedEndpoints.push_back(otherIt->second);
        }
        return connectedEndpoints;
    }

    bool SceneComponent::Connect(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        AZ::Entity* foundEntity = nullptr;
        if (FindConnection(foundEntity, sourceEndpoint, targetEndpoint))
        {
            AZ_Warning("Graph Canvas", false, "Attempting to create duplicate connection between source endpoint (%s, %s) and target endpoint(%s, %s)",
                sourceEndpoint.GetNodeId().ToString().data(), sourceEndpoint.GetSlotId().ToString().data(),
                targetEndpoint.GetNodeId().ToString().data(), targetEndpoint.GetSlotId().ToString().data());
            return false;
        }

        auto entry = AZStd::find_if(m_sceneData.m_nodes.begin(), m_sceneData.m_nodes.end(), [&sourceEndpoint](const AZ::Entity* node) { return node->GetId() == sourceEndpoint.GetNodeId(); });
        if (entry == m_sceneData.m_nodes.end())
        {
            AZ_Error("Scene", false, "The source node with id %s is not in this scene, a connection cannot be made", sourceEndpoint.GetNodeId().ToString().data());
            return false;
        }

        entry = AZStd::find_if(m_sceneData.m_nodes.begin(), m_sceneData.m_nodes.end(), [&targetEndpoint](const AZ::Entity* node) { return node->GetId() == targetEndpoint.GetNodeId(); });
        if (entry == m_sceneData.m_nodes.end())
        {
            AZ_Error("Scene", false, "The target node with id %s is not in this scene, a connection cannot be made", targetEndpoint.GetNodeId().ToString().data());
            return false;
        }

        AZ::EntityId connectionEntity;
        SlotRequestBus::EventResult(connectionEntity, sourceEndpoint.GetSlotId(), &SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

        return true;
    }

    bool SceneComponent::Disconnect(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        AZ::Entity* connectionEntity{};
        if (FindConnection(connectionEntity, sourceEndpoint, targetEndpoint) && RemoveConnection(connectionEntity->GetId()))
        {
            delete connectionEntity;
            return true;
        }
        return false;
    }

    bool SceneComponent::DisconnectById(const AZ::EntityId& connectionId)
    {
        if (RemoveConnection(connectionId))
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connectionId);
            return true;
        }

        return false;
    }

    bool SceneComponent::FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const
    {
        AZ::Entity* foundEntity = nullptr;
        for (auto connectionIt = m_sceneData.m_connections.begin(); connectionIt != m_sceneData.m_connections.end(); ++connectionIt)
        {
            Endpoint sourceEndpoint;
            ConnectionRequestBus::EventResult(sourceEndpoint, (*connectionIt)->GetId(), &ConnectionRequests::GetSourceEndpoint);
            Endpoint targetEndpoint;
            ConnectionRequestBus::EventResult(targetEndpoint, (*connectionIt)->GetId(), &ConnectionRequests::GetTargetEndpoint);

            if ((sourceEndpoint == firstEndpoint && targetEndpoint == otherEndpoint)
                || (sourceEndpoint == otherEndpoint && targetEndpoint == firstEndpoint))
            {
                foundEntity = *connectionIt;
                break;
            }
        }

        if (foundEntity)
        {
            connectionEntity = foundEntity;
            return true;
        }

        return false;
    }

    bool SceneComponent::Add(const AZ::EntityId& entityId)
    {
        AZ::Entity* actual = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!actual)
        {
            return false;
        }

        if (AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(actual))
        {
            AZ::Vector2 position;
            GeometryRequestBus::EventResult(position, entityId, &GeometryRequests::GetPosition);
            return AddNode(entityId, position);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(actual))
        {
            return AddConnection(entityId);
        }
        else
        {
            AZ_Error("Scene", false, "The entity does not appear to be a valid scene membership candidate (ID: %s)", entityId.ToString().data());
        }

        return false;
    }

    bool SceneComponent::Remove(const AZ::EntityId& entityId)
    {
        AZ::Entity* actual = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(actual, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (!actual)
        {
            return false;
        }

        if (AZ::EntityUtils::FindFirstDerivedComponent<NodeComponent>(actual))
        {
            return RemoveNode(entityId);
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(actual))
        {
            return RemoveConnection(entityId);
        }
        else
        {
            AZ_Error("Scene", false, "The entity does not appear to be a valid scene membership candidate (ID: %s)", entityId.ToString().data());
        }

        return false;
    }

    void SceneComponent::SetStyleSheet(const AZStd::string& json)
    {
        Styling::Parser parser;
        AZStd::unique_ptr<Styling::StyleSheet> fresh(parser.Parse(json));

        auto styleSheet = static_cast<Styling::StyleSheet*>(GetEntity()->FindComponent<Styling::StyleSheet>());
        if (styleSheet)
        {
            *styleSheet = std::move(*fresh);
            ColorPaletteManagerRequestBus::Event(GetEntityId(), &ColorPaletteManagerRequests::RefreshColorPalette);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStyleSheetChanged);
        }
    }

    void SceneComponent::SetStyleSheet(const rapidjson::Document& json)
    {
        Styling::Parser parser;
        AZStd::unique_ptr<Styling::StyleSheet> fresh(parser.Parse(json));

        auto styleSheet = static_cast<Styling::StyleSheet*>(GetEntity()->FindComponent<Styling::StyleSheet>());
        if (styleSheet)
        {
            *styleSheet = *fresh;
            ColorPaletteManagerRequestBus::Event(GetEntityId(), &ColorPaletteManagerRequests::RefreshColorPalette);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStyleSheetChanged);
        }
    }

    void SceneComponent::ClearSelection()
    {
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->clearSelection();
        }
    }

    void SceneComponent::SetSelectedArea(const AZ::Vector2& topLeft, const AZ::Vector2& topRight)
    {
        if (m_graphicsSceneUi)
        {
            QPainterPath path;
            path.addRect(QRectF{ QPointF {topLeft.GetX(), topLeft.GetY()}, QPointF { topRight.GetX(), topRight.GetY() } });
            m_graphicsSceneUi->setSelectionArea(path);
        }
    }

    bool SceneComponent::HasSelectedItems() const
    {
        return m_graphicsSceneUi ? !m_graphicsSceneUi->selectedItems().isEmpty() : false;
    }

    bool SceneComponent::HasEntitiesAt(const AZ::Vector2& scenePoint) const
    {
        bool retVal = false;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(QPointF(scenePoint.GetX(), scenePoint.GetY()));
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    retVal = true;
                    break;
                }
            }
        }

        return retVal;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetSelectedItems() const
    {
        AZStd::vector<AZ::EntityId> result;
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());
            result.reserve(selected.size());

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    result.push_back(entry->second);
                }
            }
        }
        return result;
    }

    QGraphicsScene* SceneComponent::AsQGraphicsScene()
    {
        return m_graphicsSceneUi.get();
    }

    void SceneComponent::CopySelection() const
    {
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();

        Copy(entities);
    }

    void SceneComponent::Copy(const AZStd::vector<AZ::EntityId>& selectedItems) const
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnCopyBegin);

        SceneSerialization serializationTarget(m_copyMimeType);
        SerializeEntities({ selectedItems.begin(), selectedItems.end() }, serializationTarget);
        SerializeToClipboard(serializationTarget);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnCopyEnd);
    }

    void SceneComponent::CutSelection()
    {
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        Cut(entities);
    }

    void SceneComponent::Cut(const AZStd::vector<AZ::EntityId>& selectedItems)
    {
        Copy(selectedItems);

        AZStd::unordered_set<AZ::EntityId> deletedItems(selectedItems.begin(), selectedItems.end());
        Delete(deletedItems);
    }

    void SceneComponent::Paste()
    {
        m_allowReset = false;
        QPointF pasteCenter = GetViewCenterScenePoint() + m_pasteOffset;
        PasteAt(pasteCenter);

        AZ::Vector2 minorPitch;
        GridRequestBus::EventResult(minorPitch, m_grid, &GridRequests::GetMinorPitch);

        // Don't want to shift it diagonally, because we also shift things diagonally when we drag/drop in stuff
        // So we'll just move it straight down.
        m_pasteOffset += QPointF(0, minorPitch.GetY() * 2);
        m_allowReset = true;
    }

    void SceneComponent::PasteAt(const QPointF& scenePos)
    {
        m_ignoreSelectionChanges = true;

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteBegin);

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->clearSelection();
        }

        AZ::Vector2 pastePos{ static_cast<float>(scenePos.x()), static_cast<float>(scenePos.y()) };
        QClipboard* clipboard = QApplication::clipboard();

        // Trying to paste unknown data into our scene.
        if (!clipboard->mimeData()->hasFormat(m_copyMimeType.c_str()))
        {
            return;
        }

        QByteArray byteArray = clipboard->mimeData()->data(m_copyMimeType.c_str());
        SceneSerialization serializationSource(byteArray);
        DeserializeEntities(scenePos, serializationSource);

        VariableReferenceSceneNotificationBus::Event(GetEntityId(), &VariableReferenceSceneNotifications::ResolvePastedReferences);
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostCreationEvent);
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPasteEnd);

        m_ignoreSelectionChanges = false;
        OnSelectionChanged();
    }

    void SceneComponent::SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneSerialization& serializationTarget) const
    {
        SceneData& serializedEntities = serializationTarget.GetSceneData();

        AZStd::unordered_set<AZ::EntityId> nodes;
        AZStd::unordered_set<AZ::EntityId> connections;
        FilterItems(itemIds, nodes, connections);

        // If we have nothing to copy. Don't copy.
        if (nodes.empty())
        {
            return;
        }

        // Calculate the position of selected items relative to the position from either the context menu mouse point or the scene center
        AZ::Vector2 aggregatePos = AZ::Vector2::CreateZero();
        for (const auto& nodeRef : nodes)
        {
            NodeNotificationBus::Event(nodeRef, &NodeNotifications::OnNodeAboutToSerialize, serializationTarget);
            serializedEntities.m_nodes.emplace(AzToolsFramework::GetEntity(nodeRef));
        }

        AZStd::unordered_set< AZ::EntityId > serializedNodes;

        // Can't do this with the above listing. Because when nodes get serialized, they may add other nodes to the list.
        // So once we are fully added in, we can figure out our positions.
        for (AZ::Entity* entity : serializedEntities.m_nodes)
        {
            serializedNodes.insert(entity->GetId());

            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);

            AZ::Vector2 itemPos = AZ::Vector2::CreateZero();

            if (graphicsItem)
            {
                QPointF scenePosition = graphicsItem->scenePos();
                itemPos.SetX(scenePosition.x());
                itemPos.SetY(scenePosition.y());
            }
            
            aggregatePos += itemPos;
        }



        // This copies only connections among nodes in the copied node set
        connections = FindConnections(serializedNodes, true);

        for (const auto& connection : connections)
        {
            serializedEntities.m_connections.emplace(AzToolsFramework::GetEntity(connection));
        }

        AZ::Vector2 averagePos = aggregatePos / serializedEntities.m_nodes.size();
        serializationTarget.SetAveragePosition(averagePos);

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesSerialized, serializationTarget);
    }

    void SceneComponent::DeserializeEntities(const QPointF& scenePoint, const SceneSerialization& serializationSource)
    {
        AZ::Vector2 deserializePoint = AZ::Vector2(scenePoint.x(), scenePoint.y());
        const AZ::Vector2& averagePos = serializationSource.GetAveragePosition();

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnEntitiesDeserialized, serializationSource);

        const SceneData& pasteSceneData = serializationSource.GetSceneData();

        for (auto& nodeRef : pasteSceneData.m_nodes)
        {
            AZStd::unique_ptr<AZ::Entity> entity(nodeRef);
            entity->Init();
            entity->Activate();

            AZ::Vector2 prevNodePos;
            GeometryRequestBus::EventResult(prevNodePos, entity->GetId(), &GeometryRequests::GetPosition);
            GeometryRequestBus::Event(entity->GetId(), &GeometryRequests::SetPosition, (prevNodePos - averagePos) + deserializePoint);
            NodeNotificationBus::Event(entity->GetId(), &NodeNotifications::OnNodeDeserialized, serializationSource);

            SceneMemberUIRequestBus::Event(entity->GetId(), &SceneMemberUIRequests::SetSelected, true);

            if (Add(entity->GetId()))
            {
                entity.release();
            }
        }

        for (auto& connection : pasteSceneData.m_connections)
        {
            AZStd::unique_ptr<AZ::Entity> entity(connection);
            entity->Init();
            entity->Activate();

            if (ValidateConnectionEndpoints(connection, m_sceneData.m_nodes) && Add(entity->GetId()))
            {
                entity.release();
            }
        }
    }

    void SceneComponent::DuplicateSelection()
    {
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        DuplicateAt(entities, GetViewCenterScenePoint());
    }

    void SceneComponent::DuplicateSelectionAt(const QPointF& scenePos)
    {
        AZStd::vector<AZ::EntityId> entities = GetSelectedItems();
        DuplicateAt(entities, scenePos);
    }

    void SceneComponent::Duplicate(const AZStd::vector<AZ::EntityId>& itemIds)
    {
        DuplicateAt(itemIds, GetViewCenterScenePoint());
    }

    void SceneComponent::DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos)
    {
        m_ignoreSelectionChanges = true;

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDuplicateBegin);

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->clearSelection();
        }

        SceneSerialization serializationTarget;
        SerializeEntities({ itemIds.begin(), itemIds.end() }, serializationTarget);

        AZStd::vector<char> buffer;
        SerializeToBuffer(serializationTarget, buffer);
        SceneSerialization deserializationTarget(QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));

        DeserializeEntities(scenePos, deserializationTarget);

        VariableReferenceSceneNotificationBus::Event(GetEntityId(), &VariableReferenceSceneNotifications::ResolvePastedReferences);
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostCreationEvent);
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDuplicateEnd);

        m_ignoreSelectionChanges = false;
        OnSelectionChanged();
    }

    void SceneComponent::DeleteSelection()
    {
        if (m_graphicsSceneUi)
        {
            const QList<QGraphicsItem*> selected(m_graphicsSceneUi->selectedItems());

            AZStd::unordered_set<AZ::EntityId> toDelete;

            for (QGraphicsItem* item : selected)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end())
                {
                    toDelete.insert(entry->second);
                }
            }

            Delete(toDelete);
        }
    }

    void SceneComponent::Delete(const AZStd::unordered_set<AZ::EntityId>& itemIds)
    {
        if (itemIds.empty())
        {
            return;
        }

        // Need to deal with recursive deleting because of Wrapper Nodes
        ++m_deleteCount;

        AZStd::unordered_set<AZ::EntityId> nodes;
        AZStd::unordered_set<AZ::EntityId> connections;

        FilterItems(itemIds, nodes, connections);

        auto nodeConnections = FindConnections(nodes);
        connections.insert(nodeConnections.begin(), nodeConnections.end());

        // Block the graphics scene from sending selection update events as we remove items.
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(true);
        }

        for (const auto& connection : connections)
        {
            if (Remove(connection))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPreConnectionDeleted, connection);
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connection);
            }
        }

        for (const auto& node : nodes)
        {
            if (Remove(node))
            {
                SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnPreNodeDeleted, node);
                NodeNotificationBus::Event(node, &NodeNotifications::OnRemovedFromScene, GetEntityId());

                bool isWrappedNode = false;
                NodeUIRequestBus::EventResult(isWrappedNode, node, &NodeUIRequests::IsWrapped);
                
                QGraphicsItem* item = nullptr;
                if (!isWrappedNode)
                {
                    SceneMemberUIRequestBus::EventResult(item, node, &SceneMemberUIRequests::GetRootGraphicsItem);
                }

                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, node);
            }
        }

        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->blockSignals(false);

            // Once complete, signal selection is changed
            emit m_graphicsSceneUi->selectionChanged();
        }

        --m_deleteCount;

        if (m_deleteCount == 0)
        {
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::PostDeletionEvent);
        }
    }

    void SceneComponent::DeleteSceneData(const SceneData& sceneData)
    {
        AZStd::unordered_set<AZ::EntityId> itemIds;
        for (auto&& itemContainer : { sceneData.m_nodes, sceneData.m_connections })
        {
            for (AZ::Entity* item : itemContainer)
            {
                itemIds.insert(item->GetId());
            }
        }

        Delete(itemIds);
    }

    void SceneComponent::SuppressNextContextMenu()
    {
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->SuppressNextContextMenu();
        }
    }

    const AZStd::string& SceneComponent::GetCopyMimeType() const
    {
        return m_copyMimeType;
    }

    void SceneComponent::SetMimeType(const char* mimeType)
    {
        m_mimeDelegate.SetMimeType(mimeType);

        m_copyMimeType = AZStd::string::format("%s::copy", mimeType);
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetEntitiesAt(const AZ::Vector2& position) const
    {
        AZStd::vector<AZ::EntityId> result;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(QPointF(position.GetX(), position.GetY()));
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    result.emplace_back(AZStd::move(entry->second));
                }
            }
        }

        return result;
    }

    AZStd::vector<AZ::EntityId> SceneComponent::GetEntitiesInRect(const QRectF& rect, Qt::ItemSelectionMode mode) const
    {
        AZStd::vector<AZ::EntityId> result;

        if (m_graphicsSceneUi)
        {
            QList<QGraphicsItem*> itemsThere = m_graphicsSceneUi->items(rect, mode);
            for (QGraphicsItem* item : itemsThere)
            {
                auto entry = m_itemLookup.find(item);
                if (entry != m_itemLookup.end() && entry->second != m_grid)
                {
                    result.emplace_back(AZStd::move(entry->second));
                }
            }
        }

        return result;
    }

    AZStd::vector<Endpoint> SceneComponent::GetEndpointsInRect(const QRectF& rect) const
    {
        AZStd::vector<Endpoint> result;

        AZStd::vector<AZ::EntityId> entitiesThere = GetEntitiesInRect(rect, Qt::ItemSelectionMode::IntersectsItemShape);
        for (AZ::EntityId nodeId : entitiesThere)
        {
            if (NodeRequestBus::FindFirstHandler(nodeId) != nullptr)
            {
                AZStd::vector<AZ::EntityId> slotIds;
                NodeRequestBus::EventResult(slotIds, nodeId, &NodeRequestBus::Events::GetSlotIds);
                for (AZ::EntityId slotId : slotIds)
                {
                    QPointF point;
                    SlotUIRequestBus::EventResult(point, slotId, &SlotUIRequestBus::Events::GetConnectionPoint);
                    if (rect.contains(point))
                    {
                        result.emplace_back(Endpoint(nodeId, slotId));
                    }
                }
            }
        }

        AZStd::sort(result.begin(), result.end(), [&rect](Endpoint a, Endpoint b) {
            QPointF pointA;
            SlotUIRequestBus::EventResult(pointA, a.GetSlotId(), &SlotUIRequestBus::Events::GetConnectionPoint);
            QPointF pointB;
            SlotUIRequestBus::EventResult(pointB, b.GetSlotId(), &SlotUIRequestBus::Events::GetConnectionPoint);

            return (rect.center() - pointA).manhattanLength() < (rect.center() - pointB).manhattanLength();
        });

        return result;
    }

    void SceneComponent::RegisterView(const AZ::EntityId& viewId)
    {
        if (m_activateScene)
        {
            m_activateScene = false;

            SceneMemberRequestBus::Event(m_grid, &SceneMemberRequests::SetScene, GetEntityId());

            ActivateItems(AZStd::initializer_list<AZ::Entity*>{ AzToolsFramework::GetEntity(m_grid) });
            ActivateItems(m_sceneData.m_nodes);
            ActivateItems(m_sceneData.m_connections);
            NotifyConnectedSlots();

            ColorPaletteManagerRequestBus::Event(GetEntityId(), &ColorPaletteManagerRequests::RefreshColorPalette);
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnStyleSheetChanged); // Forces activated elements to refresh their visual elements.

            m_mimeDelegate.SetSceneId(GetEntityId());
            m_mimeDelegate.Activate();
        }

        if (!m_viewId.IsValid() || m_viewId == viewId)
        {
            m_viewId = viewId;
            ViewNotificationBus::Handler::BusConnect(m_viewId);
            ViewRequestBus::Event(m_viewId, &ViewRequests::SetViewParams, m_viewParams);
        }
        else
        {
            AZ_Error("Scene", false, "Trying to register scene to two different views.");
        }
    }

    void SceneComponent::RemoveView(const AZ::EntityId& viewId)
    {
        if (m_viewId == viewId)
        {
            m_viewId.SetInvalid();
            ViewNotificationBus::Handler::BusDisconnect();
        }
        else
        {
            AZ_Error("Scene", !m_viewId.IsValid(), "Trying to unregister scene from the wrong view.");
        }
    }

    ViewId SceneComponent::GetViewId() const
    {
        return m_viewId;
    }

    void SceneComponent::DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData)
    {
        QPointF scenePoint(scenePos.GetX(), scenePos.GetY());

        for (const AZ::EntityId& delegateId : m_delegates)
        {
            bool isInterested;
            SceneMimeDelegateHandlerRequestBus::EventResult(isInterested, delegateId, &SceneMimeDelegateHandlerRequests::IsInterestedInMimeData, GetEntityId(), mimeData);

            if (isInterested)
            {
                SceneMimeDelegateHandlerRequestBus::Event(delegateId, &SceneMimeDelegateHandlerRequests::HandleDrop, GetEntityId(), scenePoint, mimeData);
            }
        }
    }

    bool SceneComponent::AddSceneData(const SceneData& sceneData)
    {
        bool success = true;
        for (auto&& itemContainer : { sceneData.m_nodes, sceneData.m_connections })
        {
            for (AZ::Entity* itemRef : itemContainer)
            {
                if (itemRef->GetState() == AZ::Entity::ES_INIT)
                {
                    itemRef->Activate();
                }

                success = Add(itemRef->GetId()) && success;
            }
        }

        return success;
    }

    void SceneComponent::RemoveSceneData(const SceneData& sceneData)
    {
        for (auto&& itemContainer : { sceneData.m_nodes, sceneData.m_connections })
        {
            for (AZ::Entity* itemRef : itemContainer)
            {
                Remove(itemRef->GetId());
            }
        }
    }

    void SceneComponent::SetDragSelectionType(DragSelectionType selectionType)
    {
        m_dragSelectionType = selectionType;
    }

    void SceneComponent::SignalDragSelectStart()
    {
        m_isDragSelecting = true;
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDragSelectStart);
    }

    void SceneComponent::SignalDragSelectEnd()
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnDragSelectEnd);
        m_isDragSelecting = false;
    }

    bool SceneComponent::OnMousePress(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && sourceId != m_grid)
        {
            m_pressedEntity = sourceId;
            GeometryRequestBus::EventResult(m_originalPosition, m_pressedEntity, &GeometryRequests::GetPosition);
        }

        return false;
    }

    bool SceneComponent::OnMouseRelease(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* event)
    {
        if (m_isDraggingEntity)
        {
            m_isDraggingEntity = false;

            AZ::Vector2 finalPosition;
            GeometryRequestBus::EventResult(finalPosition, m_pressedEntity, &GeometryRequests::GetPosition);
        
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberDragComplete, sourceId);

            if (!finalPosition.IsClose(m_originalPosition))
            {
                SceneUIRequestBus::Event(GetEntityId(), &SceneUIRequests::RequestUndoPoint);
            }
        }

        m_pressedEntity.SetInvalid();
        return false;
    }

    void SceneComponent::OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position)
    {
        if (m_pressedEntity.IsValid() && !m_isDraggingEntity)
        {
            m_isDraggingEntity = true;
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberDragBegin, m_pressedEntity);
        }

        if (NodeRequestBus::FindFirstHandler(itemId) != nullptr)
        {
            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnNodePositionChanged, itemId, position);

            if (m_allowReset)
            {
                m_pasteOffset.setX(0);
                m_pasteOffset.setY(0);
            }
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberPositionChanged, itemId, position);
        if (m_graphicsSceneUi)
        {
            m_graphicsSceneUi->update();
        }
    }

    void SceneComponent::OnViewParamsChanged(const ViewParams& viewParams)
    {
        m_pasteOffset.setX(0);
        m_pasteOffset.setY(0);
        
        m_viewParams = viewParams;
    }

    void SceneComponent::AddDelegate(const AZ::EntityId& delegateId)
    {
        m_delegates.insert(delegateId);
    }

    void SceneComponent::RemoveDelegate(const AZ::EntityId& delegateId)
    {
        m_delegates.erase(delegateId);
    }

    void SceneComponent::OnSceneDragEnter(const QMimeData* mimeData)
    {
        m_activeDelegates.clear();

        for (const AZ::EntityId& delegateId : m_delegates)
        {
            bool isInterested = false;
            SceneMimeDelegateHandlerRequestBus::EventResult(isInterested, delegateId, &SceneMimeDelegateHandlerRequests::IsInterestedInMimeData, GetEntityId(), mimeData);

            if (isInterested)
            {
                m_activeDelegates.insert(delegateId);
            }
        }
    }

    void SceneComponent::OnSceneDropEvent(const QPointF& scenePoint, const QMimeData* mimeData)
    {
        for (const AZ::EntityId& dropHandler : m_activeDelegates)
        {
            SceneMimeDelegateHandlerRequestBus::Event(dropHandler, &SceneMimeDelegateHandlerRequests::HandleDrop, GetEntityId(), scenePoint, mimeData);
        }
    }

    void SceneComponent::OnSceneDragExit()
    {
        m_activeDelegates.clear();
    }

    bool SceneComponent::HasActiveMimeDelegates() const
    {
        return !m_activeDelegates.empty();
    }

    template<typename Container>
    void SceneComponent::InitItems(const Container& entities) const
    {
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                AZ::Entity::State state = entity->GetState();
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }
            }
        }
    }

    template<typename Container>
    void SceneComponent::ActivateItems(const Container& entities)
    {
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::ES_INIT)
                {
                    entity->Activate();
                }

                AddSceneMember(entity->GetId());
            }
        }
    }

    template<typename Container>
    void SceneComponent::DeactivateItems(const Container& entities)
    {
        for (const auto& entityRef : entities)
        {
            AZ::Entity* entity = entityRef;
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    GeometryNotificationBus::MultiHandler::BusDisconnect(entity->GetId());
                    QGraphicsItem* item = nullptr;
                    SceneMemberUIRequestBus::EventResult(item, entity->GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);
                    SceneMemberRequestBus::Event(entity->GetId(), &SceneMemberRequests::ClearScene, GetEntityId());
                    if (m_graphicsSceneUi)
                    {
                        m_graphicsSceneUi->removeItem(item);
                    }
                    entity->Deactivate();
                }
            }
        }
    }

    template<typename Container>
    void SceneComponent::DestroyItems(const Container& entities) const
    {
        for (auto& entityRef : entities)
        {
            delete entityRef;
        }
    }

    void SceneComponent::InitConnections()
    {
        BuildEndpointMap(m_sceneData);
        InitItems(m_sceneData.m_connections);
    }

    void SceneComponent::NotifyConnectedSlots()
    {
        for (auto& connection : m_sceneData.m_connections)
        {
            AZ::Entity* entity = connection;
            auto* connectionEntity = entity ? AZ::EntityUtils::FindFirstDerivedComponent<ConnectionComponent>(entity) : nullptr;
            if (connectionEntity)
            {
                SlotRequestBus::Event(connectionEntity->GetSourceEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, connectionEntity->GetEntityId(), connectionEntity->GetTargetEndpoint());
                SlotRequestBus::Event(connectionEntity->GetTargetEndpoint().GetSlotId(), &SlotRequests::AddConnectionId, connectionEntity->GetEntityId(), connectionEntity->GetSourceEndpoint());
            }
        }
    }

    void SceneComponent::OnSelectionChanged()
    {

        if((m_isDragSelecting && (m_dragSelectionType != DragSelectionType::Realtime))
            || m_ignoreSelectionChanges)
        {
            // Nothing to do.
            return;
        }

        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSelectionChanged);
    }

    void SceneComponent::RegisterSelectionItem(const AZ::EntityId& itemId)
    {
        QGraphicsItem* selectionItem = nullptr;
        SceneMemberUIRequestBus::EventResult(selectionItem, itemId, &SceneMemberUIRequests::GetSelectionItem);

        m_itemLookup[selectionItem] = itemId;
    }

    void SceneComponent::UnregisterSelectionItem(const AZ::EntityId& itemId)
    {
        QGraphicsItem* selectionItem = nullptr;
        SceneMemberUIRequestBus::EventResult(selectionItem, itemId, &SceneMemberUIRequests::GetSelectionItem);

        m_itemLookup.erase(selectionItem);
    }

    void SceneComponent::AddSceneMember(const AZ::EntityId& sceneMemberId, bool positionItem, const AZ::Vector2& position)
    {
        QGraphicsItem* graphicsItem = nullptr;
        SceneMemberUIRequestBus::EventResult(graphicsItem, sceneMemberId, &SceneMemberUIRequests::GetRootGraphicsItem);

        if (graphicsItem)
        {
            if (m_graphicsSceneUi)
            {
                m_graphicsSceneUi->addItem(graphicsItem);
            }

            RegisterSelectionItem(sceneMemberId);
            
            SceneMemberRequestBus::Event(sceneMemberId, &SceneMemberRequests::SetScene, GetEntityId());

            if (positionItem)
            {
                GeometryRequestBus::Event(sceneMemberId, &GeometryRequests::SetPosition, position);
            }

            SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnSceneMemberAdded, sceneMemberId);
            GeometryNotificationBus::MultiHandler::BusConnect(sceneMemberId);
            VisualNotificationBus::MultiHandler::BusConnect(sceneMemberId);

            SceneMemberRequestBus::Event(sceneMemberId, &SceneMemberRequests::SignalMemberSetupComplete);
        }
    }

    AZStd::unordered_set<AZ::EntityId> SceneComponent::FindConnections(const AZStd::unordered_set<AZ::EntityId>& nodeIds, bool internalConnectionsOnly) const
    {
        AZStd::unordered_set<AZ::EntityId> foundConnections;
        for (auto& connection : m_sceneData.m_connections)
        {
            AZ::EntityId source;
            ConnectionRequestBus::EventResult(source, connection->GetId(), &ConnectionRequests::GetSourceNodeId);
            AZ::EntityId target;
            ConnectionRequestBus::EventResult(target, connection->GetId(), &ConnectionRequests::GetTargetNodeId);

            if (internalConnectionsOnly)
            {
                // Both sides of the connection must be in the nodeIds set for internal connections
                if (nodeIds.count(source) != 0 && nodeIds.count(target) != 0)
                {
                    foundConnections.emplace(connection->GetId());
                }
            }
            else
            {
                if (nodeIds.count(source) != 0 || nodeIds.count(target) != 0)
                {
                    foundConnections.emplace(connection->GetId());
                }
            }
        }

        return foundConnections;
    }

    bool SceneComponent::ValidateConnectionEndpoints(AZ::Entity* connection, const AZStd::unordered_set<AZ::Entity*>& nodeRefs)
    {
        Endpoint source;
        ConnectionRequestBus::EventResult(source, connection->GetId(), &ConnectionRequests::GetSourceEndpoint);
        Endpoint target;
        ConnectionRequestBus::EventResult(target, connection->GetId(), &ConnectionRequests::GetTargetEndpoint);

        auto sourceFound = AZStd::find_if(nodeRefs.begin(), nodeRefs.end(), [&source](const AZ::Entity* node) { return node->GetId() == source.GetNodeId(); }) != nodeRefs.end();
        auto targetFound = AZStd::find_if(nodeRefs.begin(), nodeRefs.end(), [&target](const AZ::Entity* node) { return node->GetId() == target.GetNodeId(); }) != nodeRefs.end();
        return sourceFound != 0 && targetFound != 0;
    }

    void SceneComponent::FilterItems(const AZStd::unordered_set<AZ::EntityId>& itemIds, AZStd::unordered_set<AZ::EntityId>& nodeIds, AZStd::unordered_set<AZ::EntityId>& connectionIds) const
    {
        AZStd::unordered_set< AZ::EntityId > wrapperNodes;

        for (const auto& node : m_sceneData.m_nodes)
        {
            if (itemIds.find(node->GetId()) != itemIds.end())
            {
                nodeIds.insert(node->GetId());

                if (WrapperNodeRequestBus::FindFirstHandler(node->GetId()) != nullptr)
                {
                    wrapperNodes.insert(node->GetId());
                }
            }
        }

        // Wrapper nodes handle copying/deleting everything internal to themselves. 
        // So we need to sanitize our filtering to avoid things that are wrapped when the wrapper
        // is also copied.
        for (const auto& wrapperNode : wrapperNodes)
        {
            AZStd::vector< AZ::EntityId > wrappedNodes;
            WrapperNodeRequestBus::EventResult(wrappedNodes, wrapperNode, &WrapperNodeRequests::GetWrappedNodeIds);

            for (const auto& wrappedNode : wrappedNodes)
            {
                nodeIds.erase(wrappedNode);
            }
        }

        for (const auto& connection : m_sceneData.m_connections)
        {
            if (itemIds.find(connection->GetId()) != itemIds.end())
            {
                connectionIds.insert(connection->GetId());
            }
        }

    }

    QPointF SceneComponent::GetViewCenterScenePoint() const
    {
        QPointF viewCenter;

        AZ_Error("Scene", m_graphicsSceneUi, "Cannot get the center point of a scene without a valid UI Scene.");
        if (m_graphicsSceneUi)
        {
            auto graphicsViews = m_graphicsSceneUi->views();
            if (!graphicsViews.empty())
            {
                auto* graphicsView = graphicsViews.front();
                viewCenter = graphicsView->mapToScene(graphicsView->rect()).boundingRect().center();
            }
        }

        return viewCenter;
    }

    //////////////////////////////
    // GraphCanvasGraphicsScenes
    //////////////////////////////

    GraphCanvasGraphicsScene::GraphCanvasGraphicsScene(SceneComponent& scene)
        : m_scene(scene)
        , m_suppressContextMenu(false)
    {
        connect(this, &QGraphicsScene::selectionChanged, this, [this]() { m_scene.OnSelectionChanged(); });
        setSceneRect(-100000, -100000, 200000, 200000);
    }

    AZ::EntityId GraphCanvasGraphicsScene::GetEntityId() const
    {
        return m_scene.GetEntityId();
    }

    void GraphCanvasGraphicsScene::SuppressNextContextMenu()
    {
        m_suppressContextMenu = true;
    }

    void GraphCanvasGraphicsScene::keyPressEvent(QKeyEvent* event)
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnKeyPressed, event);

        QGraphicsScene::keyPressEvent(event);
    }

    void GraphCanvasGraphicsScene::keyReleaseEvent(QKeyEvent* event)
    {
        SceneNotificationBus::Event(GetEntityId(), &SceneNotifications::OnKeyReleased, event);

        QGraphicsScene::keyPressEvent(event);
    }

    void GraphCanvasGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
    {
        if (!m_suppressContextMenu)
        {
            contextMenuEvent->ignore();

            // Send the event to all items at this position until one item accepts the event.
            for (QGraphicsItem* item : itemsAtPosition(contextMenuEvent->screenPos(), contextMenuEvent->scenePos(), contextMenuEvent->widget()))
            {
                contextMenuEvent->setPos(item->mapFromScene(contextMenuEvent->scenePos()));
                contextMenuEvent->accept();
                if (!sendEvent(item, contextMenuEvent))
                {
                    break;
                }

                if (contextMenuEvent->isAccepted())
                {
                    break;
                }
            }

            if (!contextMenuEvent->isAccepted())
            {
                SceneUIRequestBus::Event(GetEntityId(), &SceneUIRequests::OnSceneContextMenuEvent, GetEntityId(), contextMenuEvent);
            }
        }
        else
        {
            m_suppressContextMenu = false;
        }
    }

    QList<QGraphicsItem*> GraphCanvasGraphicsScene::itemsAtPosition(const QPoint& screenPos, const QPointF& scenePos, QWidget* widget) const
    {
        QGraphicsView* view = widget ? qobject_cast<QGraphicsView*>(widget->parentWidget()) : nullptr;
        if (!view)
        {
            return items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());
        }

        const QRectF pointRect(QPointF(widget->mapFromGlobal(screenPos)), QSizeF(1, 1));
        if (!view->isTransformed())
        {
            return items(pointRect, Qt::IntersectsItemShape, Qt::DescendingOrder);
        }

        const QTransform viewTransform = view->viewportTransform();
        if (viewTransform.type() <= QTransform::TxScale)
        {
            return items(viewTransform.inverted().mapRect(pointRect), Qt::IntersectsItemShape,
                Qt::DescendingOrder, viewTransform);
        }
        return items(viewTransform.inverted().map(pointRect), Qt::IntersectsItemShape,
            Qt::DescendingOrder, viewTransform);
    }

    void GraphCanvasGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            // IMPORTANT: When the user right-clicks on the scene,
            // and there are NO items at the click position, the
            // current selection is lost. See documentation:
            //
            // "If there is no item at the given position on the scene,
            // the selection area is reset, any focus item loses its
            // input focus, and the event is then ignored."
            // http://doc.qt.io/qt-5/qgraphicsscene.html#mousePressEvent
            //
            // This ISN'T the behavior we want. We want to preserve
            // the current selection to allow scene interactions. To get around
            // this behavior, we'll accept the event and by-pass its
            // default implementation.

            event->accept();
            return;
        }

        QGraphicsScene::mousePressEvent(event);
    }

    void GraphCanvasGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
    {
        m_scene.OnSelectionChanged();

        QGraphicsScene::mouseReleaseEvent(event);
    }

    void GraphCanvasGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
    {
        SceneUIRequestBus::Event(GetEntityId(), &SceneUIRequests::OnSceneDoubleClickEvent, GetEntityId(), event);

        QGraphicsScene::mouseDoubleClickEvent(event);
    }

    void GraphCanvasGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
    {
        QGraphicsScene::dragEnterEvent(event);

        m_scene.OnSceneDragEnter(event->mimeData());

        if (m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            event->acceptProposedAction();
        }
    }

    void GraphCanvasGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent* event)
    {
        QGraphicsScene::dragLeaveEvent(event);

        m_scene.OnSceneDragExit();
    }

    void GraphCanvasGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
    {
        QGraphicsScene::dragMoveEvent(event);

        if (m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            event->acceptProposedAction();
        }
    }

    void GraphCanvasGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent* event)
    {
        bool accepted = event->isAccepted();
        event->setAccepted(false);

        QGraphicsScene::dropEvent(event);

        if (!event->isAccepted() && m_scene.HasActiveMimeDelegates())
        {
            event->accept();
            m_scene.OnSceneDropEvent(event->scenePos(), event->mimeData());
        }
        else
        {
            event->setAccepted(accepted);
        }
    }
}
