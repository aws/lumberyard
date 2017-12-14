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
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/AST/Node.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/GraphAsset.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <ScriptCanvas/Data/BehaviorContextObject.h>
#include <ScriptCanvas/Execution/ASTInterpreter.h>
#include <ScriptCanvas/Libraries/Core/UnaryOperator.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ErrorHandler.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Profiler/Driller.h>
#include <ScriptCanvas/Translation/Translation.h>

namespace ScriptCanvas
{
    Graph::Graph(const AZ::EntityId& uniqueId)
        : m_uniqueId(uniqueId)
    {
    }

    Graph::~Graph()
    {
        GraphRequestBus::MultiHandler::BusDisconnect(GetUniqueId());
        for (auto& nodeRef : m_graphData.m_nodes)
        {
            delete nodeRef;
        }

        for (auto& connectionRef : m_graphData.m_connections)
        {
            delete connectionRef;
        }
    }
    
    void Graph::AddToExecutionStack(Node& node, SlotId slot)
    {
        m_executionStack.push(AZStd::make_pair(&node, slot));
    }

    void Graph::ErrorIrrecoverably()
    {
        if (!m_isFinalErrorReported)
        {
            m_isInErrorState = true;
            m_isRecoverable = false;
            m_isFinalErrorReported = true;
            AZ_Warning("ScriptCanvas", false, "ERROR! Node: %s, Description: %s\n\n", m_errorReporter ? m_errorReporter->GetDebugName().c_str() : "unknown", m_errorDescription.c_str());
            ExecutionStack().swap(m_executionStack);
            // dump error report(callStackTop, m_errorReporter, m_error, graph name, entity ID)
            Deactivate();
        }
    }

    Node* Graph::GetErrorHandler() const
    {
        if (m_errorReporter)
        {
            auto iter = m_errorHandlersBySource.find(m_errorReporter->GetEntityId());
            if (iter != m_errorHandlersBySource.end())
            {
                return Node::FindNode(iter->second);
            }
        }

        return m_errorHandler;
    }

    void Graph::HandleError(const Node& callStackTop)
    {
        if (!m_isInErrorHandler)
        {
            if (Node* errorHandler = GetErrorHandler())
            {
                m_isInErrorState = false;
                UnwindStack(callStackTop);
                errorHandler->SignalOutput(errorHandler->GetSlotId("Out"));
                m_errorReporter = nullptr;
                m_errorDescription.clear();
            }
            else
            {
                ErrorIrrecoverably();
            }
        }
        else
        {
            m_errorDescription += "\nMultiple error handling attempted without resolving previous error handling. Last node: ";
            m_errorDescription += callStackTop.GetDebugName();
            ErrorIrrecoverably();
        }
    }

    void Graph::ReportError(const Node& reporter, const char* format, ...)
    {
        const size_t k_maxErrorMessageLength = 4096;
        char message[k_maxErrorMessageLength];
        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, k_maxErrorMessageLength, format, mark);
        va_end(mark);
        
        if (m_isInErrorState)
        {
            m_errorDescription += "\nMultiple errors reported without allowing for handling. Last node: ";
            m_errorDescription += reporter.GetDebugName();
            m_errorDescription += "\nDescription: ";
            m_errorDescription += message;
            ErrorIrrecoverably();
        }
        else if (m_isInErrorHandler)
        {
            m_errorDescription += "\nFurther error encountered during error handling. Last node: ";
            m_errorDescription += reporter.GetDebugName();
            m_errorDescription += "\nDescription: ";
            m_errorDescription += message;
            ErrorIrrecoverably();
        }
        else
        {
            m_errorReporter = &reporter;
            m_errorDescription = message;
            m_isInErrorState = true;
        }
    }

    void Graph::Reflect(AZ::ReflectContext* context)
    {
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
        BehaviorContextObject::Reflect(context);
        BehaviorContextObjectPtrReflect(context);

        GraphData::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Graph>()
                ->Version(11)
                ->Field("m_graphData", &Graph::m_graphData)
                ->Field("m_uniqueId", &Graph::m_uniqueId)
                    ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Entity::MakeId)
                ;
        }
    }

    void Graph::Init()
    {
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
                    node->SetGraph(this);
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
    }

    void Graph::Activate()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        SCRIPTCANVAS_RETURN_IF_NOT_GRAPH_RECOVERABLE((*this));

        AZ::EntityBus::Handler::BusConnect(GetEntityId());
        
        RefreshDataFlowValidity(true);

        // If there are no nodes, there's nothing to do, deactivate the graph's entity.
        if (m_graphData.m_nodes.empty())
        {
            Deactivate();
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        bool entryPointFound = false;

        for (auto& nodeEntity : m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                if (auto startNode = AZ::EntityUtils::FindFirstDerivedComponent<Nodes::Core::Start>(nodeEntity))
                {
                    AddToExecutionStack(*startNode, SlotId());
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
                    AZ_Warning("ScriptCanvas", !m_errorHandler, "Multiple Graph Scope Error handlers specified");
                    m_errorHandler = errorHandlerNode;
                }
                else
                {
                    for (auto errorNodes : errorSources)
                    {
                        m_errorHandlersBySource.insert(AZStd::make_pair(errorNodes.first->GetEntityId(), errorHandlerNode->GetEntityId()));
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

        m_errorHandler = nullptr;
        m_errorHandlersBySource.clear();
        ExecutionStack().swap(m_executionStack);
    }

    bool Graph::IsExecuting() const
    {
        return m_isExecuting;
    }

    void Graph::Execute()
    {
        if (!m_isExecuting && !IsInErrorState())
        {
            m_isExecuting = true;
#if defined(SCRIPTCANVAS_ERRORS_ENABLED)
            AZ::u32 executionCount(0);
#endif//defined(SCRIPTCANVAS_ERRORS_ENABLED)

            while (!m_executionStack.empty())
            {

                auto nodeAndSlot = m_executionStack.back();
                m_executionStack.pop();
                m_preExecutedStackSize = m_executionStack.size();
                nodeAndSlot.first->SignalInput(nodeAndSlot.second);

#if defined(SCRIPTCANVAS_ERRORS_ENABLED)
                ++executionCount;

                if (executionCount == SCRIPT_CANVAS_INFINITE_LOOP_DETECTION_COUNT)
                {
                    ReportError(*nodeAndSlot.first, "Infinite loop detected");
                    ErrorIrrecoverably();
                }
#endif//defined(SCRIPTCANVAS_ERRORS_ENABLED)        
            }

            m_isExecuting = false;
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

    void Graph::ResolveSelfReferences(const AZ::EntityId& graphOwnerId)
    {
        for (auto& nodeEntry : m_graphData.m_nodes)
        {
            auto nodes = AZ::EntityUtils::FindDerivedComponents<Node>(nodeEntry);
            for (auto node : nodes)
            {
                node->ResolveSelfEntityReferences(graphOwnerId);
            }
        }
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
                        node->SetGraph(this);
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

    AZStd::string Graph::GetLastErrorDescription() const
    {
        return m_errorDescription;
    }

    Node* Graph::GetNode(const ID& nodeID) const
    {
        return Node::FindNode(nodeID);
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

                    if(connection->GetSourceEndpoint().IsValid())
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

                if(connection->GetSourceEndpoint().IsValid())
                {
                    EndpointNotificationBus::Event(connection->GetSourceEndpoint(), &EndpointNotifications::OnEndpointDisconnected, connection->GetTargetEndpoint());
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
    
    AZ::Outcome<void, AZStd::string> Graph::ValidateDataFlow(const AZ::Entity& sourceNodeEntity, const AZ::Entity& targetNodeEntity, const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const
    {
        Node* sourceNode = AZ::EntityUtils::FindFirstDerivedComponent<Node>(const_cast<AZ::Entity*>(&sourceNodeEntity));
        Node* targetNode = AZ::EntityUtils::FindFirstDerivedComponent<Node>(const_cast<AZ::Entity*>(&targetNodeEntity));
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
                        ( "There is an invalid data connection %s.%s --> %s.%s, the data is not in the execution path between nodes. Either route execution %s --> %s, or store the data in a variable if it is needed."
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

    //////////////////////////////////////////////////////////////////////////
    // Debugger::RequestBus
    void Graph::SetBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot)
    {

    }

    void Graph::RemoveBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot)
    {

    }

    void Graph::StepOver()
    {

    }

    void Graph::StepIn()
    {

    }

    void Graph::StepOut()
    {

    }

    void Graph::Stop()
    {

    }

    void Graph::Continue()
    {

    }

    void Graph::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        GraphAsset* graphAsset = asset.GetAs<GraphAsset>();
        if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
        {

        }
    }

    void Graph::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void Graph::OnEntityActivated(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect(GetEntityId());
        Execute();
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

    void Graph::UnwindStack(const Node& callStackTop)
    {
        while (m_executionStack.size() > m_preExecutedStackSize)
        {
            m_executionStack.pop();
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
}
