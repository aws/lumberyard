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

#include <stdarg.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <ScriptCanvas/AST/Node.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <ScriptCanvas/Data/BehaviorContextObject.h>
#include <ScriptCanvas/Libraries/Core/UnaryOperator.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ErrorHandler.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Profiler/Driller.h>
#include <ScriptCanvas/Translation/Translation.h>
#include <ScriptCanvas/Developer/StatusBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    Graph::Graph(const AZ::EntityId& uniqueId)
        : m_uniqueId(uniqueId)
    {
    }

    Graph::~Graph()
    {
        GraphRequestBus::MultiHandler::BusDisconnect(GetUniqueId());
        const bool deleteData{ true };
        m_graphData.Clear(deleteData);
    }

    void Graph::Reflect(AZ::ReflectContext* context)
    {
        Data::PropertyMetadata::Reflect(context);
        Data::Type::Reflect(context);
        Connection::Reflect(context);
        Node::Reflect(context);
        PureData::Reflect(context);
        Nodes::UnaryOperator::Reflect(context);
        Nodes::UnaryExpression::Reflect(context);
        Nodes::BinaryOperator::Reflect(context);
        Nodes::ArithmeticExpression::Reflect(context);
        Nodes::BooleanExpression::Reflect(context);
        Nodes::EqualityExpression::Reflect(context);
        Nodes::ComparisonExpression::Reflect(context);
        Datum::Reflect(context);
        BehaviorContextObjectPtrReflect(context);

        GraphData::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Graph, AZ::Component>()
                ->Version(11)
                ->Field("m_graphData", &Graph::m_graphData)
                ->Field("m_uniqueId", &Graph::m_uniqueId)
                ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Entity::MakeId)
                ;
        }
    }

    void Graph::Init()
    {
        AZ::Outcome<void, StatusErrors> statusOutcome = AZ::Success();
        StatusRequestBus::BroadcastResult(statusOutcome, &StatusRequests::ValidateGraph, *this);
        if (!statusOutcome)
        {
            for (const AZStd::string& graphError : statusOutcome.GetError().m_graphErrors)
            {
                AZ_Error("Script Canvas", false, graphError.data());
            }

            for (AZ::EntityId connectionId : statusOutcome.GetError().m_invalidConnectionIds)
            {
                DisconnectById(connectionId);
            }
        }

        for (auto& nodeEntity : m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                if (nodeEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    nodeEntity->Init();
                }

                if (auto* node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity))
                {
                    node->SetGraphUniqueId(m_uniqueId);
                }
            }
        }

        m_graphData.BuildEndpointMap();
        for (auto& connectionEntity : m_graphData.m_connections)
        {
            if (connectionEntity)
            {
                if (connectionEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    connectionEntity->Init();
                }
            }
        }

        GraphRequestBus::MultiHandler::BusConnect(GetUniqueId());
        RuntimeRequestBus::Handler::BusConnect(GetUniqueId());
    }

    void Graph::Activate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        if (!m_executionContext.Activate(GetUniqueId()))
        {
            return;
        }

        AZ::EntityBus::Handler::BusConnect(GetEntityId());

        RefreshDataFlowValidity(true);

        // If there are no nodes, there's nothing to do, deactivate the graph's entity.
        if (m_graphData.m_nodes.empty())
        {
            Deactivate();
            return;
        }

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        const bool replaceIdOnEntity{ true };
        const bool updateIdOnEntityId{ false };

        // Gather list of all the graph's node and connection entities
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> assetToRuntimeInternalMap;
        auto internalGraphEntityIdMapper = [&assetToRuntimeInternalMap](const AZ::EntityId& entityId, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator)
        {
            // Add entity AZ::Entity::m_id instances to the map
            assetToRuntimeInternalMap.emplace(entityId, entityId);
            return entityId;
        };

        AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&m_graphData, internalGraphEntityIdMapper, serializeContext, replaceIdOnEntity);

        // Looks up the EntityContext loaded game entity map
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> loadedGameEntityIdMap;
        AzFramework::EntityContextId owningContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(owningContextId, GetEntityId(), &AzFramework::EntityIdContextQueries::GetOwningContextId);
        if (!owningContextId.IsNull())
        {
            // Add a mapping for the SelfReferenceId to the execution component entity id
            AzFramework::EntityContextRequestBus::EventResult(loadedGameEntityIdMap, owningContextId, &AzFramework::EntityContextRequests::GetLoadedEntityIdMap);
        }


        // Added in mapping of SelfReferenceId Sentinel value to this component's EntityId
        // as well as an identity mapping for the EntityId and the UniqueId
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> editorToRuntimeEntityIdMap{
            { ScriptCanvas::SelfReferenceId, GetEntityId() },
            { ScriptCanvas::InvalidUniqueRuntimeId, GetUniqueId() },
            { GetEntityId(), GetEntityId() },
            { GetUniqueId(), GetUniqueId() },
            { AZ::EntityId(), AZ::EntityId() }
        };
        editorToRuntimeEntityIdMap.insert(assetToRuntimeInternalMap.begin(), assetToRuntimeInternalMap.end());
        editorToRuntimeEntityIdMap.insert(loadedGameEntityIdMap.begin(), loadedGameEntityIdMap.end());
        // Lambda function remaps any known world map entities to their correct id other wise it DOES NOT remap the entityId.
        // This works differently than the runtime component remapping which remaps unknown world entities to invalid entity id
        auto worldEntityRemapper = [&editorToRuntimeEntityIdMap](const AZ::EntityId& entityId, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator&) -> AZ::EntityId
        {
            auto foundEntityIdIt = editorToRuntimeEntityIdMap.find(entityId);
            if (foundEntityIdIt != editorToRuntimeEntityIdMap.end())
            {
                return foundEntityIdIt->second;
            }
            else
            {
                AZ_Warning("Script Canvas", false, "Entity Id %s is not part of the entity ids known by the graph. It will be not be remapped", entityId.ToString().data());
                return entityId;
            }
        };

        AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(&m_graphData, worldEntityRemapper, serializeContext);

        bool entryPointFound = false;

        for (auto& nodeEntity : m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                if (auto startNode = AZ::EntityUtils::FindFirstDerivedComponent<Nodes::Core::Start>(nodeEntity))
                {
                    m_executionContext.AddToExecutionStack(*startNode, SlotId());
                    entryPointFound = true;
                }

                auto nodes = AZ::EntityUtils::FindDerivedComponents<ScriptCanvas::Node>(nodeEntity);
                for (auto nodePtr : nodes)
                {
                    if (!azrtti_cast<PureData*>(nodeEntity))
                    {
                        entryPointFound = entryPointFound || nodePtr->IsEntryPoint();
                    }
                }
            }
        }

        // If we still can't find a start node, there's nothing to do.
        if (!entryPointFound)
        {
            AZ_Warning("Script Canvas", false, "Graph does not have any entry point nodes, it will not run.");
            Deactivate();
            return;
        }

        for (auto& nodeEntity : m_graphData.m_nodes)
        {
            if (auto errorHandlerNode = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Nodes::Core::ErrorHandler>(nodeEntity) : nullptr)
            {
                AZStd::vector<AZStd::pair<Node*, const SlotId>> errorSources = errorHandlerNode->GetSources();

                if (errorSources.empty())
                {
                    m_executionContext.AddErrorHandler(m_uniqueId, errorHandlerNode->GetEntityId());
                }
                else
                {
                    for (auto errorNodes : errorSources)
                    {
                        m_executionContext.AddErrorHandler(errorNodes.first->GetEntityId(), errorHandlerNode->GetEntityId());
                    }
                }
            }

            if (nodeEntity)
            {
                if (nodeEntity->GetState() == AZ::Entity::ES_INIT)
                {
                    nodeEntity->Activate();
                }
            }
        }

        for (auto& connectionEntity : m_graphData.m_connections)
        {
            if (connectionEntity)
            {
                if (connectionEntity->GetState() == AZ::Entity::ES_INIT)
                {
                    connectionEntity->Activate();
                }
            }
        }
    }

    void Graph::Deactivate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        m_executionContext.Deactivate();
        AZ::EntityBus::Handler::BusDisconnect(GetEntityId());

        for (auto& nodeEntity : m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                if (nodeEntity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    nodeEntity->Deactivate();
                }
            }
        }

        for (auto& connectionEntity : m_graphData.m_connections)
        {
            if (connectionEntity)
            {
                if (connectionEntity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    connectionEntity->Deactivate();
                }
            }
        }
    }

    bool Graph::AddItem(AZ::Entity* itemRef)
    {
        AZ::Entity* elementEntity = itemRef;
        if (!elementEntity)
        {
            return false;
        }

        if (elementEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
        {
            elementEntity->Init();
        }

        if (auto* node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(elementEntity))
        {
            return AddNode(elementEntity->GetId());
        }

        if (auto* connection = AZ::EntityUtils::FindFirstDerivedComponent<Connection>(elementEntity))
        {
            return AddConnection(elementEntity->GetId());
        }

        return false;
    }

    bool Graph::RemoveItem(AZ::Entity* itemRef)
    {
        if (AZ::EntityUtils::FindFirstDerivedComponent<Node>(itemRef))
        {
            return RemoveNode(itemRef->GetId());
        }
        else if (AZ::EntityUtils::FindFirstDerivedComponent<Connection>(itemRef))
        {
            return RemoveConnection(itemRef->GetId());
        }

        return false;
    }

    bool Graph::AddNode(const AZ::EntityId& nodeId)
    {
        if (nodeId.IsValid())
        {
            auto entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
            if (entry == m_graphData.m_nodes.end())
            {
                AZ::Entity* nodeEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
                AZ_Assert(nodeEntity, "Failed to add node to Graph, did you initialize the node entity?");
                if (nodeEntity)
                {
                    auto node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity);
                    if (node)
                    {
                        m_graphData.m_nodes.emplace(nodeEntity);
                        node->SetGraphUniqueId(m_uniqueId);
                        node->Configure();
                        GraphNotificationBus::Event(GetUniqueId(), &GraphNotifications::OnNodeAdded, nodeId);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool Graph::RemoveNode(const AZ::EntityId& nodeId)
    {
        if (nodeId.IsValid())
        {
            auto entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
            if (entry != m_graphData.m_nodes.end())
            {
                m_graphData.m_nodes.erase(entry);
                GraphNotificationBus::Event(GetUniqueId(), &GraphNotifications::OnNodeRemoved, nodeId);
                return true;
            }
        }
        return false;
    }

    Node* Graph::FindNode(AZ::EntityId nodeID) const
    {
        auto entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [nodeID](const AZ::Entity* node) { return node->GetId() == nodeID; });
        return entry != m_graphData.m_nodes.end() ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(*entry) : nullptr;
    }

    AZStd::vector<AZ::EntityId> Graph::GetNodes() const
    {
        AZStd::vector<AZ::EntityId> entityIds;
        for (auto& nodeRef : m_graphData.m_nodes)
        {
            entityIds.push_back(nodeRef->GetId());
        }

        return entityIds;
    }

    const AZStd::vector<AZ::EntityId> Graph::GetNodesConst() const
    {
        return GetNodes();
    }

    bool Graph::AddConnection(const AZ::EntityId& connectionId)
    {
        if (connectionId.IsValid())
        {
            auto entry = AZStd::find_if(m_graphData.m_connections.begin(), m_graphData.m_connections.end(), [connectionId](const AZ::Entity* connection) { return connection->GetId() == connectionId; });
            if (entry == m_graphData.m_connections.end())
            {
                AZ::Entity* connectionEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(connectionEntity, &AZ::ComponentApplicationRequests::FindEntity, connectionId);
                auto connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(connectionEntity) : nullptr;
                AZ_Warning("Script Canvas", connection, "Failed to add connection to Graph, did you initialize the connection entity?");
                if (connection)
                {
                    m_graphData.m_connections.emplace_back(connectionEntity);
                    m_graphData.m_endpointMap.emplace(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
                    m_graphData.m_endpointMap.emplace(connection->GetTargetEndpoint(), connection->GetSourceEndpoint());
                    GraphNotificationBus::Event(GetUniqueId(), &GraphNotifications::OnConnectionAdded, connectionId);

                    if (connection->GetSourceEndpoint().IsValid())
                    {
                        EndpointNotificationBus::Event(connection->GetSourceEndpoint(), &EndpointNotifications::OnEndpointConnected, connection->GetTargetEndpoint());
                    }
                    if (connection->GetTargetEndpoint().IsValid())
                    {
                        EndpointNotificationBus::Event(connection->GetTargetEndpoint(), &EndpointNotifications::OnEndpointConnected, connection->GetSourceEndpoint());
                    }

                    return true;
                }
            }
        }
        return false;
    }

    bool Graph::RemoveConnection(const AZ::EntityId& connectionId)
    {
        if (connectionId.IsValid())
        {
            auto entry = AZStd::find_if(m_graphData.m_connections.begin(), m_graphData.m_connections.end(), [connectionId](const AZ::Entity* connection) { return connection->GetId() == connectionId; });
            if (entry != m_graphData.m_connections.end())
            {
                auto connection = *entry ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(*entry) : nullptr;
                if (connection)
                {
                    m_graphData.m_endpointMap.erase(connection->GetSourceEndpoint());
                    m_graphData.m_endpointMap.erase(connection->GetTargetEndpoint());
                }
                m_graphData.m_connections.erase(entry);
                GraphNotificationBus::Event(GetUniqueId(), &GraphNotifications::OnConnectionRemoved, connectionId);

                if (connection->GetSourceEndpoint().IsValid())
                {
                    EndpointNotificationBus::Event(connection->GetSourceEndpoint(), &EndpointNotifications::OnEndpointDisconnected, connection->GetTargetEndpoint());
                }
                if (connection->GetTargetEndpoint().IsValid())
                {
                    EndpointNotificationBus::Event(connection->GetTargetEndpoint(), &EndpointNotifications::OnEndpointDisconnected, connection->GetSourceEndpoint());
                }

                return true;
            }
        }
        return false;
    }

    AZStd::vector<AZ::EntityId> Graph::GetConnections() const
    {
        AZStd::vector<AZ::EntityId> entityIds;
        for (auto& connectionRef : m_graphData.m_connections)
        {
            entityIds.push_back(connectionRef->GetId());
        }

        return entityIds;
    }

    AZStd::vector<Endpoint> Graph::GetConnectedEndpoints(const Endpoint& firstEndpoint) const
    {
        AZStd::vector<Endpoint> connectedEndpoints;
        auto otherEndpointsRange = m_graphData.m_endpointMap.equal_range(firstEndpoint);
        for (auto otherIt = otherEndpointsRange.first; otherIt != otherEndpointsRange.second; ++otherIt)
        {
            connectedEndpoints.push_back(otherIt->second);
        }
        return connectedEndpoints;
    }

    bool Graph::FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const
    {
        if (!firstEndpoint.IsValid() || !otherEndpoint.IsValid())
        {
            return false;
        }

        AZ::Entity* foundEntity{};
        for (auto connectionRefIt = m_graphData.m_connections.begin(); connectionRefIt != m_graphData.m_connections.end(); ++connectionRefIt)
        {
            auto* connection = *connectionRefIt ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(*connectionRefIt) : nullptr;
            if (connection)
            {
                if ((connection->GetSourceEndpoint() == firstEndpoint && connection->GetTargetEndpoint() == otherEndpoint)
                    || (connection->GetSourceEndpoint() == otherEndpoint && connection->GetTargetEndpoint() == firstEndpoint))
                {
                    foundEntity = connection->GetEntity();
                    break;
                }
            }
        }

        if (foundEntity)
        {
            connectionEntity = foundEntity;
            return true;
        }

        return false;
    }


    bool Graph::Connect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId)
    {
        return ConnectByEndpoint({ sourceNodeId, sourceSlotId }, { targetNodeId, targetSlotId });
    }

    bool Graph::ConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        AZ::Outcome<void, AZStd::string> outcome = CanConnectByEndpoint(sourceEndpoint, targetEndpoint);

        if (outcome.IsSuccess())
        {
            auto* connectionEntity = aznew AZ::Entity("Connection");
            connectionEntity->CreateComponent<Connection>(sourceEndpoint, targetEndpoint);


            AZ::Entity* nodeEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, sourceEndpoint.GetNodeId());
            auto node = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity) : nullptr;
            AZStd::string sourceNodeName = node ? node->GetNodeName() : "";
            AZStd::string sourceSlotName = node ? node->GetSlotName(sourceEndpoint.GetSlotId()) : "";

            nodeEntity = {};
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, targetEndpoint.GetNodeId());
            node = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity) : nullptr;
            AZStd::string targetNodeName = node ? node->GetNodeName() : "";
            AZStd::string targetSlotName = node ? node->GetSlotName(targetEndpoint.GetSlotId()) : "";
            connectionEntity->SetName(AZStd::string::format("srcEndpoint=(%s: %s), destEndpoint=(%s: %s)",
                sourceNodeName.data(),
                sourceSlotName.data(),
                targetNodeName.data(),
                targetSlotName.data()));

            connectionEntity->Init();
            connectionEntity->Activate();

            return AddConnection(connectionEntity->GetId());
        }
        else
        {
            AZ_Warning("Script Canvas", false, "Failed to create connection: %s", outcome.GetError().c_str());
        }

        return false;
    }

    bool Graph::IsInDataFlowPath(const Node* sourceNode, const Node* targetNode) const
    {
        return sourceNode && sourceNode->IsTargetInDataFlowPath(targetNode);
    }

    AZ::Outcome<void, AZStd::string> Graph::ValidateDataFlow(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const
    {
        auto sourceEntity = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&sourceEndpoint](const AZ::Entity* node) { return node->GetId() == sourceEndpoint.GetNodeId(); });
        if (sourceEntity == m_graphData.m_nodes.end())
        {
            return AZ::Failure(AZStd::string::format("The source node with id %s is not a part of this graph, a connection cannot be made", sourceEndpoint.GetNodeId().ToString().data()));
        }

        auto targetEntity = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&targetEndpoint](const AZ::Entity* node) { return node->GetId() == targetEndpoint.GetNodeId(); });
        if (targetEntity == m_graphData.m_nodes.end())
        {
            return AZ::Failure(AZStd::string::format("The target node with id %s is not a part of this graph, a connection cannot be made", targetEndpoint.GetNodeId().ToString().data()));
        }

        return ValidateDataFlow(**sourceEntity, **targetEntity, sourceEndpoint, targetEndpoint);
    }

    AZ::Outcome<void, AZStd::string> Graph::ValidateDataFlow(AZ::Entity& sourceNodeEntity, AZ::Entity& targetNodeEntity, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const
    {
        Node* sourceNode = AZ::EntityUtils::FindFirstDerivedComponent<Node>(&sourceNodeEntity);
        Node* targetNode = AZ::EntityUtils::FindFirstDerivedComponent<Node>(&targetNodeEntity);
        Slot* sourceSlot = sourceNode ? sourceNode->GetSlot(sourceEndpoint.GetSlotId()) : nullptr;
        Slot* targetSlot = targetNode ? targetNode->GetSlot(targetEndpoint.GetSlotId()) : nullptr;

        if (sourceSlot && targetSlot)
        {
            if (CanDataConnect(sourceSlot->GetType(), targetSlot->GetType()))
            {
                if (sourceSlot->GetType() == SlotType::DataIn)
                {
                    AZStd::swap(sourceNode, targetNode);
                    AZStd::swap(sourceSlot, targetSlot);
                }

                if (!IsInDataFlowPath(sourceNode, targetNode))
                {
                    return AZ::Failure(AZStd::string::format
                    ("There is an invalid data connection %s.%s --> %s.%s, the data is not in the execution path between nodes. Either route execution %s --> %s, or store the data in a variable if it is needed."
                        , sourceNode->GetNodeName().data()
                        , sourceSlot->GetName().data()
                        , targetNode->GetNodeName().data()
                        , targetSlot->GetName().data()
                        , sourceNode->GetNodeName().data()
                        , targetNode->GetNodeName().data()));
                }
            }
        }
        else
        {
            return AZ::Failure(AZStd::string("Source and Target slot must be present and valid data configurations."));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> Graph::CanConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const
    {
        AZ::Entity* foundEntity = nullptr;
        if (FindConnection(foundEntity, sourceEndpoint, targetEndpoint))
        {
            return AZ::Failure(AZStd::string::format("Attempting to create duplicate connection between source endpoint (%s, %s) and target endpoint(%s, %s)",
                sourceEndpoint.GetNodeId().ToString().data(), sourceEndpoint.GetSlotId().m_id.ToString<AZStd::string>().data(),
                targetEndpoint.GetNodeId().ToString().data(), targetEndpoint.GetSlotId().m_id.ToString<AZStd::string>().data()));
        }

        auto entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&sourceEndpoint](const AZ::Entity* node) { return node->GetId() == sourceEndpoint.GetNodeId(); });
        if (entry == m_graphData.m_nodes.end())
        {
            return AZ::Failure(AZStd::string::format("The source node with id %s is not a part of this graph, a connection cannot be made", sourceEndpoint.GetNodeId().ToString().data()));
        }

        entry = AZStd::find_if(m_graphData.m_nodes.begin(), m_graphData.m_nodes.end(), [&targetEndpoint](const AZ::Entity* node) { return node->GetId() == targetEndpoint.GetNodeId(); });
        if (entry == m_graphData.m_nodes.end())
        {
            return AZ::Failure(AZStd::string::format("The target node with id %s is not a part of this graph, a connection cannot be made", targetEndpoint.GetNodeId().ToString().data()));
        }

        auto outcome = Connection::ValidateEndpoints(sourceEndpoint, targetEndpoint);
        return outcome;
    }

    bool Graph::Disconnect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId)
    {
        return DisconnectByEndpoint({ sourceNodeId, sourceSlotId }, { targetNodeId, targetSlotId });
    }

    bool Graph::DisconnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        AZ::Entity* connectionEntity{};
        if (FindConnection(connectionEntity, sourceEndpoint, targetEndpoint) && RemoveConnection(connectionEntity->GetId()))
        {
            delete connectionEntity;
            return true;
        }
        return false;
    }

    bool Graph::DisconnectById(const AZ::EntityId& connectionId)
    {
        if (RemoveConnection(connectionId))
        {
            AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, connectionId);
            return true;
        }

        return false;
    }

    void Graph::RefreshDataFlowValidity(bool warnOnRemoval)
    {
        AZStd::vector<Connection*> removableConnections;

        for (auto& connectionEntity : m_graphData.m_connections)
        {
            if (auto connection = (connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(connectionEntity) : nullptr))
            {
                auto outcome = ValidateDataFlow(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());

                if (!outcome.IsSuccess())
                {
                    AZ_Warning("ScriptCanvas", !warnOnRemoval, outcome.GetError().data());
                    removableConnections.push_back(connection);
                }
            }
        }

        for (auto connection : removableConnections)
        {
            DisconnectByEndpoint(connection->GetSourceEndpoint(), connection->GetTargetEndpoint());
        }

        if (!removableConnections.empty())
        {
            RefreshDataFlowValidity(warnOnRemoval);
        }
    }

    void Graph::OnEntityActivated(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect(GetEntityId());
        m_executionContext.Execute();
    }

    bool Graph::AddGraphData(const GraphData& graphData)
    {
        bool success = true;
        for (auto&& nodeItem : graphData.m_nodes)
        {
            success = AddItem(nodeItem) && success;
        }

        for (auto&& nodeItem : graphData.m_connections)
        {
            success = AddItem(nodeItem) && success;
        }

        return success;
    }

    void Graph::RemoveGraphData(const GraphData& graphData)
    {
        RemoveItems(graphData.m_connections);
        RemoveItems(graphData.m_nodes);
    }

    AZStd::unordered_set<AZ::Entity*> Graph::CopyItems(const AZStd::unordered_set<AZ::Entity*>& entities)
    {
        AZStd::unordered_set<AZ::Entity*> elementsToCopy;
        for (const auto& nodeElement : m_graphData.m_nodes)
        {
            if (entities.find(nodeElement) != entities.end())
            {
                elementsToCopy.emplace(nodeElement);
            }
        }

        for (const auto& connectionElement : m_graphData.m_connections)
        {
            if (entities.find(connectionElement) != entities.end())
            {
                elementsToCopy.emplace(connectionElement);
            }
        }

        return elementsToCopy;
    }

    void Graph::AddItems(const AZStd::unordered_set<AZ::Entity*>& graphField)
    {
        for (auto& graphElementRef : graphField)
        {
            AddItem(graphElementRef);
        }
    }

    void Graph::RemoveItems(const AZStd::unordered_set<AZ::Entity*>& graphField)
    {
        for (auto& graphElementRef : graphField)
        {
            RemoveItem(graphElementRef);
        }
    }

    void Graph::RemoveItems(const AZStd::vector<AZ::Entity*>& graphField)
    {
        for (auto& graphElementRef : graphField)
        {
            RemoveItem(graphElementRef);
        }
    }

    bool Graph::ValidateConnectionEndpoints(const AZ::EntityId& connectionRef, const AZStd::unordered_set<AZ::EntityId>& nodeRefs)
    {
        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, connectionRef);
        auto* connection = entity ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(entity) : nullptr;
        if (connection)
        {
            auto sourceIt = nodeRefs.find(connection->GetSourceNode());
            auto targetIt = nodeRefs.find(connection->GetTargetNode());
            return sourceIt != nodeRefs.end() && targetIt != nodeRefs.end();
        }

        return false;
    }

    AZStd::unordered_set<AZ::Entity*> Graph::GetItems() const
    {
        AZStd::unordered_set<AZ::Entity*> result;

        for (auto& nodeEntity : m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                result.emplace(nodeEntity);
            }
        }

        for (auto& connectionEntity : m_graphData.m_connections)
        {
            if (connectionEntity)
            {
                result.emplace(connectionEntity);
            }
        }
        return result;
    }

    VariableData* Graph::GetVariableData()
    {
        VariableData* variableData{};
        GraphVariableManagerRequestBus::EventResult(variableData, GetUniqueId(), &GraphVariableManagerRequests::GetVariableData);
        return variableData;
    }

    const AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::VariableNameValuePair>* Graph::GetVariables() const
    {
        const GraphVariableMapping* variables{};
        GraphVariableManagerRequestBus::EventResult(variables, GetUniqueId(), &GraphVariableManagerRequests::GetVariables);
        return variables;
    }

    VariableDatum* Graph::FindVariable(AZStd::string_view propName)
    {
        VariableDatum* variableDatum{};
        GraphVariableManagerRequestBus::EventResult(variableDatum, GetUniqueId(), &GraphVariableManagerRequests::FindVariable, propName);
        return variableDatum;
    }

    VariableNameValuePair* Graph::FindVariableById(const VariableId& variableId)
    {
        VariableNameValuePair* variableNameValuePair{};
        GraphVariableManagerRequestBus::EventResult(variableNameValuePair, GetUniqueId(), &GraphVariableManagerRequests::FindVariableById, variableId);
        return variableNameValuePair;
    }

    Data::Type Graph::GetVariableType(const VariableId& variableId) const
    {
        Data::Type scType;
        GraphVariableManagerRequestBus::EventResult(scType, GetUniqueId(), &GraphVariableManagerRequests::GetVariableType, variableId);
        return scType;
    }

    AZStd::string_view Graph::GetVariableName(const VariableId& variableId) const
    {
        AZStd::string_view varName;
        GraphVariableManagerRequestBus::EventResult(varName, GetUniqueId(), &GraphVariableManagerRequests::GetVariableName, variableId);
        return varName;
    }
}
