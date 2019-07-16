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

#include <qmimedata.h>
#include <QFile>
#include <QScopedValueRollback>

#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/ConnectionBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/MappingBus.h>

#include <Core/PureData.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Nodes/NodeUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasBoolDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasEntityIdDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasNumericDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasColorDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasCRCDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasReadOnlyDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasStringDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVectorDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVariableDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasQuaternionDataInterface.h>

#include <Editor/Metrics.h>
#include <Editor/Nodes/ScriptCanvasAssetNode.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Core/EBusEventHandler.h>
#include <Libraries/Core/ReceiveScriptEvent.h>
#include <Libraries/Core/ScriptEventBase.h>
#include <Libraries/Core/SendScriptEvent.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <ScriptCanvas/Variable/VariableBus.h>

// Graph Version Conversion: Added Version 1.14
#include <AzCore/std/containers/map.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/DeprecatedVariableNodeDescriptorComponents.h>
#include <Editor/Nodes/NodeUtils.h>
#include <GraphCanvas/Components/GridBus.h>
////

namespace ScriptCanvasEditor
{
    namespace EditorGraph
    {
        static const char* GetMimeType()
        {
            return "application/x-lumberyard-scriptcanvas";
        }

        static const char* GetWrappedNodeGroupingMimeType()
        {
            return "application/x-lumberyard-scriptcanvas-wrappednodegrouping";
        }
    }

    Graph::~Graph()
    {
        for (auto& entry : m_graphCanvasSaveData)
        {
            delete entry.second;
        }

        m_graphCanvasSaveData.clear();

        delete m_graphCanvasSceneEntity;
        m_graphCanvasSceneEntity = nullptr;
    }

    static bool GraphVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootDataElementNode)
    {
        // Version 0 graph will have their PureData nodes to variable converted flag flipped off
        if (rootDataElementNode.GetVersion() < 1)
        {            
            rootDataElementNode.AddElementWithData(context, "m_pureDataNodesConvertedToVariables", false);
        }

        // Version 0/1 graph will have their SaveFormatConverted flag flipped off
        if (rootDataElementNode.GetVersion() < 2)
        {
            rootDataElementNode.AddElementWithData(context, "m_saveFormatConverted", false);
        }

        if (rootDataElementNode.GetVersion() < 6)
        {
            rootDataElementNode.AddElementWithData(context, "GraphCanvasSaveVersion", GraphCanvas::EntitySaveDataContainer::NoVersion);
        }

        return true;
    }

    void Graph::ConvertToGetVariableNode(Graph* graph, ScriptCanvas::VariableId variableId, const AZ::EntityId& nodeId, AZStd::unordered_map< AZ::EntityId, AZ::EntityId >& setVariableRemapping)
    {
        AZ::EntityId graphId = graph->GetGraphCanvasGraphId();

        AZ::EntityId gridId;
        GraphCanvas::SceneRequestBus::EventResult(gridId, graphId, &GraphCanvas::SceneRequests::GetGrid);

        AZ::Vector2 position;
        GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);

        int dataSlotIndex = 0;

        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > targetToNodeMapping;

        for (int i = 0; i < slotIds.size(); ++i)
        {
            AZ::EntityId slotId = slotIds[i];

            GraphCanvas::Endpoint endpoint(nodeId, slotId);

            AZStd::vector< AZ::EntityId > connectionIds;
            GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

            GraphCanvas::ConnectionType connectionType;
            GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

            GraphCanvas::SlotType slotType;
            GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                continue;
            }
            else if (slotType == GraphCanvas::SlotTypes::DataSlot)
            {
                ++dataSlotIndex;

                for (const AZ::EntityId& connectionId : connectionIds)
                {
                    GraphCanvas::Endpoint targetEndpoint;
                    GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                    AZ::EntityId targetNodeId = targetEndpoint.GetNodeId();

                    // Some nodes might have been converted
                    auto remappedNodeIter = setVariableRemapping.find(targetNodeId);
                    if (remappedNodeIter != setVariableRemapping.end())
                    {
                        targetNodeId = remappedNodeIter->second;

                        AZStd::vector< AZ::EntityId > originalSetDataSlots;
                        GraphCanvas::NodeRequestBus::EventResult(originalSetDataSlots, targetEndpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetSlotIds);

                        AZStd::vector< AZ::EntityId > newSetDataSlots;
                        GraphCanvas::NodeRequestBus::EventResult(newSetDataSlots, targetNodeId, &GraphCanvas::NodeRequests::GetSlotIds);

                        bool foundSlot = false;
                        int remappingDataSlotIndex = 0;

                        for (int i = 0; i < originalSetDataSlots.size(); ++i)
                        {
                            GraphCanvas::SlotType originalSlotType = GraphCanvas::SlotTypes::Invalid;
                            GraphCanvas::SlotRequestBus::EventResult(originalSlotType, originalSetDataSlots[i], &GraphCanvas::SlotRequests::GetSlotType);

                            if (originalSlotType == GraphCanvas::SlotTypes::DataSlot)
                            {
                                ++remappingDataSlotIndex;
                            }

                            if (originalSetDataSlots[i] == targetEndpoint.m_slotId)
                            {
                                foundSlot = true;
                                break;
                            }
                        }

                        if (foundSlot)
                        {
                            for (int i = 0; i < newSetDataSlots.size(); ++i)
                            {
                                GraphCanvas::SlotType remappedSlotType = GraphCanvas::SlotTypes::Invalid;
                                GraphCanvas::SlotRequestBus::EventResult(remappedSlotType, newSetDataSlots[i], &GraphCanvas::SlotRequests::GetSlotType);

                                if (remappedSlotType == GraphCanvas::SlotTypes::DataSlot)
                                {
                                    --remappingDataSlotIndex;

                                    if (remappingDataSlotIndex == 0)
                                    {
                                        targetEndpoint = GraphCanvas::Endpoint(targetNodeId, newSetDataSlots[i]);
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to convert a connection. Could not find equivalent connection pin on a converted Set Variable node.");
                            continue;
                        }
                    }

                    auto targetIter = targetToNodeMapping.find(targetNodeId);
                    AZStd::vector< AZ::EntityId > newSlotIds;
                    AZ::EntityId newNodeId;

                    if (targetIter == targetToNodeMapping.end())
                    {
                        NodeIdPair newVariablePair = Nodes::CreateGetVariableNode(variableId, graphId);
                        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, newVariablePair.m_graphCanvasId, position);                        

                        AZ::Vector2 minorStep;
                        GraphCanvas::GridRequestBus::EventResult(minorStep, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                        position += minorStep;

                        GraphCanvas::NodeRequestBus::EventResult(newSlotIds, newVariablePair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetSlotIds);
                        newNodeId = newVariablePair.m_graphCanvasId;
                        targetToNodeMapping[targetNodeId] = newNodeId;

                        GraphCanvas::Endpoint newExecutionInEndpoint;
                        newExecutionInEndpoint.m_nodeId = newNodeId;

                        GraphCanvas::Endpoint newExecutionOutEndpoint;
                        newExecutionOutEndpoint.m_nodeId = newNodeId;

                        for (AZ::EntityId newSlotId : newSlotIds)
                        {
                            GraphCanvas::SlotType slotType;
                            GraphCanvas::SlotRequestBus::EventResult(slotType, newSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
                            {
                                GraphCanvas::ConnectionType connectionType = GraphCanvas::CT_Invalid;
                                GraphCanvas::SlotRequestBus::EventResult(connectionType, newSlotId, &GraphCanvas::SlotRequests::GetConnectionType);

                                if (connectionType == GraphCanvas::CT_Input)
                                {
                                    newExecutionInEndpoint.m_slotId = newSlotId;
                                }
                                else if (connectionType == GraphCanvas::CT_Output)
                                {
                                    newExecutionOutEndpoint.m_slotId = newSlotId;
                                }
                            }
                        }

                        AZStd::vector< AZ::EntityId > targetSlotIds;
                        GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, targetNodeId, &GraphCanvas::NodeRequests::GetSlotIds);

                        bool spliceConnections = false;
                        AZ::EntityId targetExecutionInId;

                        for (AZ::EntityId testTargetSlotId : targetSlotIds)
                        {
                            GraphCanvas::SlotType slotType;
                            GraphCanvas::SlotRequestBus::EventResult(slotType, testTargetSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                            if (slotType != GraphCanvas::SlotTypes::ExecutionSlot)
                            {
                                continue;
                            }

                            GraphCanvas::ConnectionType connectionType = GraphCanvas::CT_Invalid;
                            GraphCanvas::SlotRequestBus::EventResult(connectionType, testTargetSlotId, &GraphCanvas::SlotRequests::GetConnectionType);

                            if (connectionType == GraphCanvas::CT_Input)
                            {
                                bool hasConnections = false;
                                GraphCanvas::SlotRequestBus::EventResult(hasConnections, testTargetSlotId, &GraphCanvas::SlotRequests::HasConnections);

                                if (hasConnections)
                                {
                                    // Gate the connection, so we only try to splice connections if we have a single execution slot
                                    spliceConnections = !targetExecutionInId.IsValid();
                                    targetExecutionInId = testTargetSlotId;
                                }
                            }
                        }

                        if (spliceConnections)
                        {
                            AZStd::vector< AZ::EntityId > connectionIds;
                            GraphCanvas::SlotRequestBus::EventResult(connectionIds, targetExecutionInId, &GraphCanvas::SlotRequests::GetConnections);

                            GraphCanvas::Endpoint connectionTargetEndpoint(targetNodeId, targetExecutionInId);

                            bool createConnection = false;

                            for (const AZ::EntityId& oldConnectionId : connectionIds)
                            {
                                GraphCanvas::Endpoint connectionSourceEndpoint;
                                GraphCanvas::ConnectionRequestBus::EventResult(connectionSourceEndpoint, oldConnectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                                if (graph->IsValidConnection(connectionSourceEndpoint, newExecutionInEndpoint))
                                {
                                    if (!createConnection)
                                    {
                                        createConnection = graph->IsValidConnection(newExecutionOutEndpoint, connectionTargetEndpoint);
                                    }

                                    AZStd::unordered_set<AZ::EntityId> deleteConnections = { oldConnectionId };
                                    GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deleteConnections);

                                    AZ::EntityId newConnectionId;
                                    GraphCanvas::SlotRequestBus::EventResult(newConnectionId, connectionSourceEndpoint.m_slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, newExecutionInEndpoint);

                                    if (newConnectionId.IsValid())
                                    {
                                        graph->CreateConnection(newConnectionId, connectionSourceEndpoint, newExecutionInEndpoint);
                                    }
                                }
                            }

                            if (createConnection)
                            {
                                AZ::EntityId newConnectionId;
                                GraphCanvas::SlotRequestBus::EventResult(newConnectionId, newExecutionOutEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, connectionTargetEndpoint);

                                if (newConnectionId.IsValid())
                                {
                                    graph->CreateConnection(newConnectionId, newExecutionOutEndpoint, connectionTargetEndpoint);
                                }
                            }
                        }
                    }
                    else
                    {
                        newNodeId = targetIter->second;
                        GraphCanvas::NodeRequestBus::EventResult(newSlotIds, newNodeId, &GraphCanvas::NodeRequests::GetSlotIds);
                    }

                    AZ::EntityId newSlotId;

                    // Going to just hope they're in the same ordering...since there really isn't much
                    // I can rely on to look this up.
                    int newDataSlotIndex = 0;

                    for (unsigned int newSlotIndex = 0; newSlotIndex < newSlotIds.size(); ++newSlotIndex)
                    {
                        AZ::EntityId testSlotId = newSlotIds[newSlotIndex];

                        GraphCanvas::SlotType slotType;
                        GraphCanvas::SlotRequestBus::EventResult(slotType, testSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                        if (slotType == GraphCanvas::SlotTypes::DataSlot)
                        {
                            ++newDataSlotIndex;

                            if (dataSlotIndex == newDataSlotIndex)
                            {
                                newSlotId = testSlotId;
                                break;
                            }
                        }
                    }

                    if (!newSlotId.IsValid() || !newNodeId.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", false, "Could not find appropriate Data Slot target when converting to a Get Variable node.");
                        continue;
                    }

                    // When stitching up the connections.
                    // We cannot add multiple data connections, so we need to remove the old connection before we attempt to make the
                    // new one, otherwise it might fail.
                    AZStd::unordered_set< AZ::EntityId > connectionClensing = { connectionId };
                    GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, connectionClensing);

                    GraphCanvas::Endpoint newEndpoint(newNodeId, newSlotId);

                    if (graph->IsValidConnection(newEndpoint, targetEndpoint))
                    {
                        AZ::EntityId newConnectionId;
                        GraphCanvas::SlotRequestBus::EventResult(newConnectionId, newEndpoint.m_slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

                        bool created = graph->CreateConnection(newConnectionId, newEndpoint, targetEndpoint);
                        AZ_Warning("ScriptCanvas", created, "Failed to created connection between migrated endpoints, despite valid connection check.");
                    }
                }
            }
        }
    }

    void Graph::Reflect(AZ::ReflectContext* context)
    {
        GraphStatisticsHelper::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CRCCache>()
                ->Version(1)
                ->Field("String", &CRCCache::m_cacheValue)
                ->Field("Count", &CRCCache::m_cacheCount)
            ;

            serializeContext->Class<Graph, ScriptCanvas::Graph>()
                ->Version(6, &GraphVersionConverter)
                ->Field("m_variableCounter", &Graph::m_variableCounter)
                ->Field("m_pureDataNodesConvertedToVariables", &Graph::m_pureDataNodesConverted)
                ->Field("m_saveFormatConverted", &Graph::m_saveFormatConverted)
                ->Field("GraphCanvasData", &Graph::m_graphCanvasSaveData)
                ->Field("CRCCacheMap", &Graph::m_crcCacheMap)
                ->Field("StatisticsHelper", &Graph::m_statisticsHelper)
                ->Field("GraphCanvasSaveVersion", &Graph::m_graphCanvasSaveVersion)
                ;
        }
    }

    void Graph::Activate()
    {
        // Overridden to prevent graph execution in the editor
        NodeCreationNotificationBus::Handler::BusConnect(GetEntityId());
        SceneCounterRequestBus::Handler::BusConnect(GetEntityId());
        EditorGraphRequestBus::Handler::BusConnect(GetEntityId());
        ScriptCanvas::GraphRequestBus::MultiHandler::BusConnect(GetEntityId());
        ScriptCanvas::StatusRequestBus::MultiHandler::BusConnect(GetEntityId());
        GraphItemCommandNotificationBus::Handler::BusConnect(GetEntityId());        
    }

    void Graph::Deactivate()
    {
        GraphItemCommandNotificationBus::Handler::BusDisconnect();
        ScriptCanvas::GraphRequestBus::MultiHandler::BusDisconnect(GetEntityId());        
        EditorGraphRequestBus::Handler::BusDisconnect();
        SceneCounterRequestBus::Handler::BusDisconnect();
        NodeCreationNotificationBus::Handler::BusDisconnect();

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect();

        delete m_graphCanvasSceneEntity;
        m_graphCanvasSceneEntity = nullptr;
    }

    void Graph::OnViewRegistered()
    {
        auto conversionOutcome = ConvertPureDataToVariables();
        AZ_Error("Script Canvas", conversionOutcome, "Graph Conversion: %s", conversionOutcome.GetError().data());

        if (!m_saveFormatConverted)
        {
            ConstructSaveData();
        }
    }

    AZ::Outcome<void, AZStd::string> Graph::ConvertPureDataToVariables()
    {
        if (m_pureDataNodesConverted)
        {
            // The PureData nodes have already been converted
            return AZ::Success();
        }

        m_pureDataNodesConverted = true;

        GraphCanvas::GraphData* sceneData = nullptr;
        GraphCanvas::SceneRequestBus::EventResult(sceneData, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetGraphData);

        if (!sceneData)
        {
            return AZ::Failure(AZStd::string("GraphCanvas Scene is missing scene data"));
        }

        auto variableComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::GraphVariableManagerComponent>(GetEntity());
        if (!variableComponent)
        {
            return AZ::Failure(AZStd::string("The Script Canvas variable component is missing"));
        }

        RequestPushPreventUndoStateUpdate();

        AZStd::vector<AZStd::string> pureDataToVariableErrors;

        // Want to keep track of a list of nodes we are destroying
        AZStd::unordered_set< AZ::EntityId > nodesToClear;

        // Need to keep track of the conversion between our GraphCanvas VariableId and the ScriptCanvas::VariableId
        AZStd::unordered_map< AZ::EntityId, ScriptCanvas::VariableId > variableIdMapping;
        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > setVariableRemapping;

        AZStd::unordered_set<AZ::Entity*> originalNodes = sceneData->m_nodes;

        for (AZ::Entity* sceneNodeEntity : originalNodes)
        {
            if (auto varDescriptorComp = AZ::EntityUtils::FindFirstDerivedComponent<Deprecated::VariableNodeDescriptorComponent>(sceneNodeEntity))
            {
                AZStd::string varName = varDescriptorComp->GetVariableName();

                AZStd::any* nodeUserData = nullptr;
                GraphCanvas::NodeRequestBus::EventResult(nodeUserData, varDescriptorComp->GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

                if (auto* scNodeId = AZStd::any_cast<AZ::EntityId>(nodeUserData))
                {
                    AZ::Entity* graphNodeEntity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(graphNodeEntity, &AZ::ComponentApplicationRequests::FindEntity, *scNodeId);
                    if (auto pureDataNode = graphNodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::PureData>(graphNodeEntity) : nullptr)
                    {
                        const auto& setSlotId = pureDataNode->GetSlotId(ScriptCanvas::PureData::k_setThis);
                        const auto& getSlotId = pureDataNode->GetSlotId(ScriptCanvas::PureData::k_getThis);

                        const ScriptCanvas::Datum* pureDatum = nullptr;
                        ScriptCanvas::EditorNodeRequestBus::EventResult(pureDatum, pureDataNode->GetEntityId(), &ScriptCanvas::EditorNodeRequests::GetInput, setSlotId);
                        
                        if (!pureDatum)
                        {
                            pureDataToVariableErrors.emplace_back(AZStd::string::format("Pure Data(%s) %s node is missing data value", varName.data(), pureDataNode->GetEntityId().ToString().data()));
                            continue;
                        }

                        // Math objects were converted from BehaviorContextObjects to raw datum types. Need to remap those using this.
                        ScriptCanvas::Datum newDatum(ScriptCanvas::Data::FromAZType(ScriptCanvas::Data::ToAZType(pureDatum->GetType())), ScriptCanvas::Datum::eOriginality::Original, pureDatum->GetAsDanger(), ScriptCanvas::Data::ToAZType(pureDatum->GetType()));

                        // Add the variable to the variable component
                        AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> createdVariable = variableComponent->AddVariable(varName, newDatum);

                        if (createdVariable.IsSuccess())
                        {
                            AZ::EntityId nodeId = varDescriptorComp->GetEntityId();

                            variableIdMapping[nodeId] = createdVariable.GetValue();
                            nodesToClear.insert(nodeId);

                            ConvertToGetVariableNode(this, createdVariable.GetValue(), nodeId, setVariableRemapping);
                        }
                    }
                }
            }
        }

        for (AZ::Entity* sceneNodeEntity : originalNodes)
        {
            AZ::EntityId nodeId = sceneNodeEntity->GetId();

            NodeDescriptorType nodeType;
            NodeDescriptorRequestBus::EventResult(nodeType, nodeId, &NodeDescriptorRequests::GetType);
            
            if (nodeType == NodeDescriptorType::GetVariable)
            {
                if (Deprecated::GetVariableNodeDescriptorRequestBus::FindFirstHandler(nodeId) == nullptr)
                {
                    continue;
                }

                nodesToClear.insert(nodeId);

                AZ::EntityId variableId;
                Deprecated::GetVariableNodeDescriptorRequestBus::EventResult(variableId, nodeId, &Deprecated::GetVariableNodeDescriptorRequests::GetVariableId);

                auto variableIter = variableIdMapping.find(variableId);

                AZ_Warning("ScriptCanvas", !variableId.IsValid() || variableIter != variableIdMapping.end(), "Could not convert a GetVariable node with variable Id(%s) no suitable conversion found.", variableId.ToString().c_str());

                if (variableId.IsValid() && variableIter != variableIdMapping.end())
                {
                    ConvertToGetVariableNode(this, variableIter->second, nodeId, setVariableRemapping);
                }
            }
            else if (nodeType == NodeDescriptorType::SetVariable)
            {
                if (Deprecated::SetVariableNodeDescriptorRequestBus::FindFirstHandler(nodeId) == nullptr)
                {
                    continue;
                }

                nodesToClear.insert(nodeId);

                AZ::EntityId variableId;
                Deprecated::SetVariableNodeDescriptorRequestBus::EventResult(variableId, nodeId, &Deprecated::SetVariableNodeDescriptorRequests::GetVariableId);

                auto variableIter = variableIdMapping.find(variableId);

                AZ_Warning("ScriptCanvas", !variableId.IsValid() || variableIter != variableIdMapping.end(), "Could not convert a GetVariable node with variable Id(%s) no suitable conversion found.", variableId.ToString().c_str());

                if (variableId.IsValid() && variableIter != variableIdMapping.end())
                {
                    NodeIdPair newVariablePair = Nodes::CreateSetVariableNode(variableIter->second, GetGraphCanvasGraphId());

                    setVariableRemapping[nodeId] = newVariablePair.m_graphCanvasId;

                    AZ::Vector2 position;
                    GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

                    GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::AddNode, newVariablePair.m_graphCanvasId, position);

                    AZStd::vector< AZ::EntityId > slotIds;
                    GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &GraphCanvas::NodeRequests::GetSlotIds);

                    AZStd::vector< AZ::EntityId > newSlotIds;
                    GraphCanvas::NodeRequestBus::EventResult(newSlotIds, newVariablePair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetSlotIds);

                    int lastExecutionIndex = 0;
                    int lastDataIndex = 0;

                    for (int i = 0; i < slotIds.size(); ++i)
                    {
                        AZ::EntityId slotId = slotIds[i];

                        GraphCanvas::Endpoint endpoint(nodeId, slotId);

                        GraphCanvas::Endpoint newEndpoint;
                        newEndpoint.m_nodeId = newVariablePair.m_graphCanvasId;

                        AZStd::vector< AZ::EntityId > connectionIds;
                        GraphCanvas::SlotRequestBus::EventResult(connectionIds, slotId, &GraphCanvas::SlotRequests::GetConnections);

                        GraphCanvas::ConnectionType connectionType;
                        GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

                        GraphCanvas::SlotType slotType;
                        GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

                        if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
                        {
                            AZ::EntityId newSlotId;

                            // Going to just hope they're in the same ordering...since there really isn't much
                            // I can rely on to look this up.
                            for (int k = lastExecutionIndex; k < newSlotIds.size(); ++k)
                            {
                                AZ::EntityId testSlotId = newSlotIds[k];

                                GraphCanvas::SlotType newNodeSlotType;
                                GraphCanvas::SlotRequestBus::EventResult(newNodeSlotType, testSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                                if (newNodeSlotType == GraphCanvas::SlotTypes::ExecutionSlot)
                                {
                                    lastExecutionIndex = (k+1);
                                    newSlotId = testSlotId;
                                    break;
                                }
                            }

                            newEndpoint.m_slotId = newSlotId;
                        }
                        else if (slotType == GraphCanvas::SlotTypes::DataSlot)
                        {
                            AZ::EntityId newSlotId;

                            // Going to just hope they're in the same ordering...since there really isn't much
                            // I can rely on to look this up.
                            for (int k = lastDataIndex; k < newSlotIds.size(); ++k)
                            {
                                AZ::EntityId testSlotId = newSlotIds[k];

                                GraphCanvas::SlotType newNodeSlotType;
                                GraphCanvas::SlotRequestBus::EventResult(newNodeSlotType, testSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                                if (newNodeSlotType == GraphCanvas::SlotTypes::DataSlot)
                                {
                                    lastDataIndex = (k+1);
                                    newSlotId = testSlotId;
                                    break;
                                }
                            }

                            newEndpoint.m_slotId = newSlotId;
                        }

                        for (const AZ::EntityId& connectionId : connectionIds)
                        {
                            if (connectionType == GraphCanvas::CT_Input)
                            {
                                GraphCanvas::Endpoint sourceEndpoint;
                                GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

                                if (IsValidConnection(sourceEndpoint, newEndpoint))
                                {
                                    AZ::EntityId connectionId;
                                    GraphCanvas::SlotRequestBus::EventResult(connectionId, newEndpoint.m_slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, sourceEndpoint);

                                    bool created = CreateConnection(connectionId, sourceEndpoint, newEndpoint);
                                    AZ_Warning("ScriptCanvas", created, "Failed to created connection between migrated endpoints, despite valid connection check.");
                                }
                            }
                            else if (connectionType == GraphCanvas::CT_Output)
                            {
                                GraphCanvas::Endpoint targetEndpoint;
                                GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                                if (IsValidConnection(newEndpoint, targetEndpoint))
                                {
                                    AZ::EntityId connectionId;
                                    GraphCanvas::SlotRequestBus::EventResult(connectionId, newEndpoint.m_slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, targetEndpoint);

                                    bool created = CreateConnection(connectionId, newEndpoint, targetEndpoint);
                                    AZ_Warning("ScriptCanvas", created, "Failed to created connection between migrated endpoints, despite valid connection check.");
                                }
                            }
                        }
                    }
                }
            }
        }

        GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::Delete, nodesToClear);

        RequestPopPreventUndoStateUpdate();
        SignalDirty();

        if (!pureDataToVariableErrors.empty())
        {
            AZStd::string errorMessage;
            AzFramework::StringFunc::Join(errorMessage, pureDataToVariableErrors.begin(), pureDataToVariableErrors.end(), "\n");
            return AZ::Failure(errorMessage);
        }

        return AZ::Success();
    }

    void Graph::OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget)
    {
        const GraphCanvas::GraphData& graphCanvasGraphData = serializationTarget.GetGraphData();

        AZStd::unordered_set< AZ::EntityId > forcedWrappedNodes;

        AZStd::unordered_set<AZ::Entity*> scriptCanvasEntities;
        for (const auto& node : graphCanvasGraphData.m_nodes)
        {
            // EBus Event nodes are purely visual, but require some user data manipulation in order to function correctly.
            // As such we don't want to copy over their script canvas user data, since it's not what was intended to be copied.
            if (EBusHandlerEventNodeDescriptorRequestBus::FindFirstHandler(node->GetId()) == nullptr)
            {
                AZStd::any* userData = nullptr;
                GraphCanvas::NodeRequestBus::EventResult(userData, node->GetId(), &GraphCanvas::NodeRequests::GetUserData);
                auto scriptCanvasNodeId = userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
                AZ::Entity* scriptCanvasEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasNodeId);
                if (scriptCanvasEntity)
                {
                    scriptCanvasEntities.emplace(scriptCanvasEntity);
                }

                if (GraphCanvas::ForcedWrappedNodeRequestBus::FindFirstHandler(node->GetId()) != nullptr)
                {
                    forcedWrappedNodes.insert(node->GetId());
                }
            }
            else 
            {
                forcedWrappedNodes.insert(node->GetId());
            }
        }

        for (const auto& connection : graphCanvasGraphData.m_connections)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::ConnectionRequestBus::EventResult(userData, connection->GetId(), &GraphCanvas::ConnectionRequests::GetUserData);

            auto scriptCanvasConnectionId = userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
            AZ::Entity* scriptCanvasEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasConnectionId);
            if (scriptCanvasEntity)
            {
                scriptCanvasEntities.emplace(scriptCanvasEntity);
            }
        }

        auto& userDataMap = serializationTarget.GetUserDataMapRef();

        AZStd::unordered_set<AZ::Entity*> graphData = CopyItems(scriptCanvasEntities);
        userDataMap.emplace(EditorGraph::GetMimeType(), graphData );

        if (!forcedWrappedNodes.empty())
        {
            // Keep track of which ebus methods were grouped together when we serialized them out.
            // This is so when we recreate them, we can create the appropriate number of
            // EBus wrappers and put the correct methods into each.
            WrappedNodeGroupingMap forcedWrappedNodeGroupings;

            for (const AZ::EntityId& wrappedNode : forcedWrappedNodes)
            {
                AZ::EntityId wrapperNode;
                GraphCanvas::NodeRequestBus::EventResult(wrapperNode, wrappedNode, &GraphCanvas::NodeRequests::GetWrappingNode);

                if (wrapperNode.IsValid())
                {
                    forcedWrappedNodeGroupings.emplace(wrappedNode, wrapperNode);
                }
            }

            userDataMap.emplace(EditorGraph::GetWrappedNodeGroupingMimeType(), forcedWrappedNodeGroupings);
        }
    }
    
    void Graph::OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource)
    {
        const auto& userDataMap = serializationSource.GetUserDataMapRef();

        auto userDataIt = userDataMap.find(EditorGraph::GetMimeType());
        if (userDataIt != userDataMap.end())
        {
            auto graphEntities(AZStd::any_cast<AZStd::unordered_set<AZ::Entity*>>(&userDataIt->second));
            if (graphEntities)
            {
                AddItems(*graphEntities);
            }
        }

        userDataIt = userDataMap.find(EditorGraph::GetWrappedNodeGroupingMimeType());

        if (userDataIt != userDataMap.end())
        {
            // Serialization system handled remapping this map data so we can just insert them into our map.
            const WrappedNodeGroupingMap* wrappedNodeGroupings = AZStd::any_cast<WrappedNodeGroupingMap>(&userDataIt->second);
            m_wrappedNodeGroupings.insert(wrappedNodeGroupings->begin(), wrappedNodeGroupings->end());
        }

        const GraphCanvas::GraphData& sceneData = serializationSource.GetGraphData();
        for (auto nodeEntity : sceneData.m_nodes)
        {
            NodeCreationNotificationBus::Event(GetEntityId(), &NodeCreationNotifications::OnGraphCanvasNodeCreated, nodeEntity->GetId());
        }
    }

    void Graph::DisconnectConnection(const GraphCanvas::ConnectionId& connectionId)
    {
        AZStd::any* connectionUserData = nullptr;
        GraphCanvas::ConnectionRequestBus::EventResult(connectionUserData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);
        auto scConnectionId = connectionUserData && connectionUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(connectionUserData) : AZ::EntityId();

        ScriptCanvas::Connection* connection = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(scConnectionId);

        if (connection)
        {
            ScriptCanvas::Endpoint scSourceEndpoint = connection->GetSourceEndpoint();
            GraphCanvas::Endpoint sourceEndpoint = ConvertToGraphCanvasEndpoint(scSourceEndpoint);

            ScriptCanvas::Endpoint scTargetEndpoint = connection->GetTargetEndpoint();
            GraphCanvas::Endpoint targetEndpoint = ConvertToGraphCanvasEndpoint(scTargetEndpoint);

            DisconnectById(scConnectionId);
        }
    }

    bool Graph::CreateConnection(const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint)
    {
        if (!sourcePoint.IsValid() || !targetPoint.IsValid())
        {
            return false;
        }

        DisconnectConnection(connectionId);
        bool scConnected = false;

        ScriptCanvas::Endpoint scSourceEndpoint = ConvertToScriptCanvasEndpoint(sourcePoint);
        ScriptCanvas::Endpoint scTargetEndpoint = ConvertToScriptCanvasEndpoint(targetPoint);

        scConnected = ConnectByEndpoint(scSourceEndpoint, scTargetEndpoint);

        if (scConnected)
        {
            scConnected = ConfigureConnectionUserData(scSourceEndpoint, scTargetEndpoint, connectionId);
        }

        return scConnected;
    }

    bool Graph::IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const
    {
        ScriptCanvas::Endpoint scSourceEndpoint = ConvertToScriptCanvasEndpoint(sourcePoint);
        ScriptCanvas::Endpoint scTargetEndpoint = ConvertToScriptCanvasEndpoint(targetPoint);

        return CanCreateConnectionBetween(scSourceEndpoint, scTargetEndpoint).IsSuccess();
    }

    bool Graph::IsValidVariableAssignment(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& targetPoint) const
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, variableId, &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId variableNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

        ScriptCanvas::SlotId variableSlotId;
        ScriptCanvas::NodeRequestBus::EventResult(variableSlotId, variableNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_getThis);

        ScriptCanvas::Endpoint variableSourceEndpoint(variableNodeId, variableSlotId);
        ScriptCanvas::Endpoint targetEndpoint = ConvertToScriptCanvasEndpoint(targetPoint);

        return CanCreateConnectionBetween(variableSourceEndpoint, targetEndpoint).IsSuccess();
    }

    AZStd::string Graph::GetDataTypeString(const AZ::Uuid& typeId)
    {
        return TranslationHelper::GetSafeTypeName(ScriptCanvas::Data::FromAZType(typeId));
    }

    void Graph::OnRemoveUnusedNodes()
    {
    }

    void Graph::OnRemoveUnusedElements()
    {
        RemoveUnusedVariables();
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const GraphCanvas::NodeId& nodeId, const GraphCanvas::SlotId& slotId) const
    {
        (void)dataType;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scriptCanvasNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

        AZStd::any* slotUserData = nullptr;
        GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotId, &GraphCanvas::SlotRequests::GetUserData);
        auto scriptCanvasSlotId = slotUserData && slotUserData->is<ScriptCanvas::SlotId>() ? *AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData) : ScriptCanvas::SlotId();

        return CreateDisplayPropertyForSlot(scriptCanvasNodeId, scriptCanvasSlotId);
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDataSlotVariablePropertyDisplay(const AZ::Uuid& dataType, const GraphCanvas::NodeId& nodeId, const GraphCanvas::SlotId& slotId) const
    {
        GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scriptCanvasNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

        AZStd::any* slotUserData = nullptr;
        GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotId, &GraphCanvas::SlotRequests::GetUserData);
        auto scriptCanvasSlotId = slotUserData && slotUserData->is<ScriptCanvas::SlotId>() ? *AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData) : ScriptCanvas::SlotId();

        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvasVariableDataInterface* variableInterface = aznew ScriptCanvasVariableDataInterface(sceneId, scriptCanvasNodeId, scriptCanvasSlotId);
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateItemModelNodePropertyDisplay, variableInterface);

        if (dataDisplay == nullptr)
        {
            delete variableInterface;
        }

        return dataDisplay;
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const GraphCanvas::NodeId& nodeId, const GraphCanvas::NodeId& slotId) const
    {
        (void)slotId;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scriptCanvasNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();
        
        if (propertyId == PropertySlotIds::DefaultValue)
        {
            ScriptCanvas::SlotId scriptCanvasSlotId;
            ScriptCanvas::NodeRequestBus::EventResult(scriptCanvasSlotId, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_setThis);

            return CreateDisplayPropertyForSlot(scriptCanvasNodeId, scriptCanvasSlotId);
        }
        else if (propertyId == PropertySlotIds::GetVariableReference)
        {
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

            AZ::EntityId scriptCanvasGraphId = GetEntityId();

            ScriptCanvasVariableDataInterface* dataInterface = aznew ScriptCanvasVariableDataInterface(scriptCanvasGraphId, scriptCanvasNodeId, ScriptCanvas::SlotId());
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateItemModelNodePropertyDisplay, dataInterface);

            if (dataDisplay == nullptr)
            {
                delete dataInterface;
            }

            return dataDisplay;
        }
        else if (propertyId == PropertySlotIds::SetVariableReference)
        {
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

            ScriptCanvasVariableDataInterface* dataInterface = aznew ScriptCanvasVariableDataInterface(sceneId, scriptCanvasNodeId, ScriptCanvas::SlotId());
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateItemModelNodePropertyDisplay, dataInterface);

            if (dataDisplay == nullptr)
            {
                delete dataInterface;
            }

            return dataDisplay;
        }

        return nullptr;
    }

    ScriptCanvas::Endpoint Graph::ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoint) const
    {
        AZStd::any* userData = nullptr;

        ScriptCanvas::Endpoint scriptCanvasEndpoint;
        AZ::EntityId scriptCanvasNodeId;

        GraphCanvas::SlotRequestBus::EventResult(userData, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetUserData);
        ScriptCanvas::SlotId scSourceSlotId = (userData && userData->is<ScriptCanvas::SlotId>()) ? *AZStd::any_cast<ScriptCanvas::SlotId>(userData) : ScriptCanvas::SlotId();
        userData = nullptr;

        GraphCanvas::NodeRequestBus::EventResult(userData, endpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scSourceNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        userData = nullptr;

        scriptCanvasEndpoint = ScriptCanvas::Endpoint(scSourceNodeId, scSourceSlotId);

        return scriptCanvasEndpoint;
    }

    GraphCanvas::Endpoint Graph::ConvertToGraphCanvasEndpoint(const ScriptCanvas::Endpoint& endpoint) const
    {
        GraphCanvas::Endpoint graphCanvasEndpoint;

        SlotMappingRequestBus::EventResult(graphCanvasEndpoint.m_slotId, endpoint.GetNodeId(), &SlotMappingRequests::MapToGraphCanvasId, endpoint.GetSlotId());
        GraphCanvas::SlotRequestBus::EventResult(graphCanvasEndpoint.m_nodeId, graphCanvasEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetNode);

        return graphCanvasEndpoint;
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const
    {
        // ScriptCanvas has access to better typing information regarding the slots than is exposed to GraphCanvas.
        // So let ScriptCanvas check the types based on it's own information rather than relying on the information passed back from GraphCanvas.
        ScriptCanvas::Data::Type slotType = ScriptCanvas::Data::Type::Invalid();
        ScriptCanvas::NodeRequestBus::EventResult(slotType, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetSlotDataType, scriptCanvasSlotId);

        {
            GraphCanvas::DataInterface* dataInterface = nullptr;
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

            if (slotType.IS_A(ScriptCanvas::Data::Type::Boolean()))
            {
                dataInterface = aznew ScriptCanvasBoolDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateBooleanNodePropertyDisplay, static_cast<ScriptCanvasBoolDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::Number()))
            {
                dataInterface = aznew ScriptCanvasNumericDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay, static_cast<ScriptCanvasNumericDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::String()))
            {
                dataInterface = aznew ScriptCanvasStringDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay, static_cast<ScriptCanvasStringDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::EntityID()))
            {
                dataInterface = aznew ScriptCanvasEntityIdDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateEntityIdNodePropertyDisplay, static_cast<ScriptCanvasEntityIdDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector3::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Vector3()))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector3, 3>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector2::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Vector2()))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector2, 2>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector4::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Vector4()))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector4, 4>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Quaternion::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Quaternion()))
            {
                dataInterface = aznew ScriptCanvasQuaternionDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Color::TYPEINFO_Uuid()))
                     || slotType.IS_A(ScriptCanvas::Data::Type::Color()))
            {
                dataInterface = aznew ScriptCanvasColorDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::CRC()))
            {
                dataInterface = aznew ScriptCanvasCRCDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay, static_cast<GraphCanvas::StringDataInterface*>(dataInterface));
            }

            if (dataDisplay != nullptr)
            {
                return dataDisplay;
            }

            delete dataInterface;
        }

        return nullptr;
    }

    void Graph::SignalDirty()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::SignalSceneDirty, GetEntityId());
    }

    void Graph::HighlightNodesByType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::Node* canvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(entity);

            if (canvasNode->GetNodeType() == nodeTypeIdentifier)
            {
                HighlightScriptCanvasEntity(entity->GetId());
            }
        }
    }

    void Graph::HighlightEBusNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId)
    {
        ScriptCanvas::NodeTypeIdentifier ebusIdentifier = ScriptCanvas::NodeUtils::ConstructEBusIdentifier(busId);

        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::Node* canvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(entity);

            if (canvasNode->GetNodeType() == ebusIdentifier)
            {
                AZ::EntityId graphCanvasNodeId;
                SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                bool hasEvent = false;
                EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventId);
                if (hasEvent)
                {
                    HighlightScriptCanvasEntity(entity->GetId());
                }
            }
        }
    }

    void Graph::HighlightScriptEventNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId)
    {
        ScriptCanvas::NodeTypeIdentifier sendScriptEventIdentifier = ScriptCanvas::NodeUtils::ConstructSendScriptEventIdentifier(busId, eventId);
        ScriptCanvas::NodeTypeIdentifier receiveScriptEventIdentifier = ScriptCanvas::NodeUtils::ConstructReceiveScriptIdentifier(busId);

        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::Node* canvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(entity);

            if (canvasNode->GetNodeType() == sendScriptEventIdentifier)
            {
                HighlightScriptCanvasEntity(entity->GetId());
            }
            else if (canvasNode->GetNodeType() == receiveScriptEventIdentifier)
            {
                AZ::EntityId graphCanvasNodeId;
                SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                bool hasEvent = false;
                EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventId);
                if (hasEvent)
                {
                    HighlightScriptCanvasEntity(entity->GetId());
                }
            }
        }
    }

    void Graph::HighlightScriptCanvasEntity(const AZ::EntityId& scriptCanvasId)
    {
        GraphCanvas::SceneMemberGlowOutlineConfiguration glowConfiguration;

        glowConfiguration.m_blurRadius = 5;

        glowConfiguration.m_pen = QPen();
        glowConfiguration.m_pen.setBrush(QColor(243,129,29));
        glowConfiguration.m_pen.setWidth(5);

        SceneMemberMappingRequestBus::EventResult(glowConfiguration.m_sceneMember, scriptCanvasId, &SceneMemberMappingRequests::GetGraphCanvasEntityId);

        glowConfiguration.m_pulseRate = AZStd::chrono::milliseconds(2500);
        glowConfiguration.m_zValue = 0;

        GraphCanvas::GraphicsEffectId graphicsEffectId;
        GraphCanvas::SceneRequestBus::EventResult(graphicsEffectId, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::CreateGlowOnSceneMember, glowConfiguration);

        if (graphicsEffectId.IsValid())
        {
            m_highlights.insert(graphicsEffectId);
        }
    }

    AZ::EntityId Graph::FindGraphCanvasSlotId(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::SlotId& slotId)
    {
        AZ::EntityId graphCanvasSlotId;
        SlotMappingRequestBus::EventResult(graphCanvasSlotId, graphCanvasNodeId, &SlotMappingRequests::MapToGraphCanvasId, slotId);

        if (!graphCanvasSlotId.IsValid())
        {
            // For the EBusHandler's I need to remap these to a different visual node.
            // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
            if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasNodeId) != nullptr)
            {
                GraphCanvas::Endpoint graphCanvasEventEndpoint;
                EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasEventEndpoint, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, slotId);

                graphCanvasSlotId = graphCanvasEventEndpoint.GetSlotId();
            }
        }

        return graphCanvasSlotId;
    }

    bool Graph::ConfigureConnectionUserData(const ScriptCanvas::Endpoint& sourceEndpoint, const ScriptCanvas::Endpoint& targetEndpoint, GraphCanvas::ConnectionId connectionId)
    {
        bool isConfigured = true;

        AZ::Entity* scConnectionEntity = nullptr;
        FindConnection(scConnectionEntity, sourceEndpoint, targetEndpoint);

        if (scConnectionEntity)
        {
            AZStd::any* connectionUserData = nullptr;
            GraphCanvas::ConnectionRequestBus::EventResult(connectionUserData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);

            if (connectionUserData)
            {
                *connectionUserData = scConnectionEntity->GetId();
                SceneMemberMappingConfigurationRequestBus::Event(connectionId, &SceneMemberMappingConfigurationRequests::ConfigureMapping, scConnectionEntity->GetId());
            }
        }
        else
        {
            isConfigured = false;
        }

        return isConfigured;
    }

    void Graph::HandleQueuedUpdates()
    {
        AZStd::unordered_set< AZ::EntityId > newUpdates;

        for (const AZ::EntityId& queuedUpdate : m_queuedConvertingNodes)
        {
            auto insertResult = m_convertingNodes.insert(queuedUpdate);

            if (insertResult.second)
            {
                newUpdates.insert(queuedUpdate);
            }
        }

        m_queuedConvertingNodes.clear();
        
        for (const AZ::EntityId& newNode : newUpdates)
        {
            VersionControlledNodeNotificationBus::MultiHandler::BusConnect(newNode);
            VersionControlledNodeRequestBus::Event(newNode, &VersionControlledNodeRequests::UpdateNodeVersion);
        }
    }

    bool Graph::IsNodeVersionConverting(const AZ::EntityId& graphCanvasNodeId) const
    {
        bool isConverting = false;

        if (!m_convertingNodes.empty())
        {
            if (GraphCanvas::GraphUtils::IsNodeWrapped(graphCanvasNodeId))
            {
                AZ::EntityId parentId;
                GraphCanvas::NodeRequestBus::EventResult(parentId, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetWrappingNode);

                if (m_convertingNodes.count(parentId) > 0)
                {
                    isConverting = true;
                }
            }
            else if (m_convertingNodes.count(graphCanvasNodeId) > 0)
            {
                isConverting = true;
            }
        }

        return isConverting;
    }

    void Graph::OnPreNodeDeleted(const AZ::EntityId& nodeId)
    {
        // If we are cdeleteing a HandlerEventNode we don't need to do anything since they are purely visual.
        // And the underlying ScriptCanvas nodes will persist and maintain all of their state.
        if (EBusHandlerEventNodeDescriptorRequestBus::FindFirstHandler(nodeId) == nullptr)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, nodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (userData && userData->is<AZ::EntityId>())
            {
                const AZ::EntityId* scriptCanvasNodeId = AZStd::any_cast<AZ::EntityId>(userData);

                auto iter = m_graphCanvasSaveData.find((*scriptCanvasNodeId));
                if (iter != m_graphCanvasSaveData.end())
                {
                    delete iter->second;
                    m_graphCanvasSaveData.erase(iter);
                }
            }

            AZStd::any* sourceUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(sourceUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
            auto scriptCanvasNodeId = sourceUserData && sourceUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(sourceUserData) : AZ::EntityId();

            if (RemoveNode(scriptCanvasNodeId))
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, scriptCanvasNodeId);
            }
        }
    }

    void Graph::OnPreConnectionDeleted(const AZ::EntityId& connectionId)
    {        
        AZStd::any* userData = nullptr;
        GraphCanvas::ConnectionRequestBus::EventResult(userData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            const AZ::EntityId* scriptCanvasConnectionId = AZStd::any_cast<AZ::EntityId>(userData);

            auto iter = m_graphCanvasSaveData.find((*scriptCanvasConnectionId));
            if (iter != m_graphCanvasSaveData.end())
            {
                delete iter->second;
                m_graphCanvasSaveData.erase(iter);
            }
        }

        GraphCanvas::Endpoint sourceEndpoint;
        GraphCanvas::ConnectionRequestBus::EventResult(sourceEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);

        // Don't disconnect any connections if we are version converting a node involved
        if (IsNodeVersionConverting(sourceEndpoint.GetNodeId()))
        {
            return;
        }           

        GraphCanvas::Endpoint targetEndpoint;
        GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

        if (IsNodeVersionConverting(targetEndpoint.GetNodeId()))
        {
            return;
        }

        DisconnectConnection(connectionId);
    }

    void Graph::OnUnknownPaste(const QPointF& scenePos)
    {
        GraphVariablesTableView::HandleVariablePaste(GetEntityId());
    }

    void Graph::OnSelectionChanged()
    {
        ClearHighlights();
    }

    AZ::u32 Graph::GetNewVariableCounter()
    {
        return ++m_variableCounter;
    }

    void Graph::ReleaseVariableCounter(AZ::u32 variableCounter)
    {
        if (m_variableCounter == variableCounter)
        {
            --m_variableCounter;
        }
    }

    void Graph::RequestUndoPoint()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
    }

    void Graph::RequestPushPreventUndoStateUpdate()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
    }

    void Graph::RequestPopPreventUndoStateUpdate()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
    }

    void Graph::TriggerUndo()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::TriggerUndo);
    }

    void Graph::TriggerRedo()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::TriggerRedo);
    }

    void Graph::PostDeletionEvent()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
    }

    void Graph::PostCreationEvent()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
        if (m_wrapperNodeDropTarget.IsValid())
        {
            for (const AZ::EntityId& nodeId : m_lastGraphCanvasCreationGroup)
            {
                GraphCanvas::WrappedNodeConfiguration configuration;
                GraphCanvas::WrapperNodeConfigurationRequestBus::EventResult(configuration, m_wrapperNodeDropTarget, &GraphCanvas::WrapperNodeConfigurationRequests::GetWrappedNodeConfiguration, nodeId);

                GraphCanvas::WrapperNodeRequestBus::Event(m_wrapperNodeDropTarget, &GraphCanvas::WrapperNodeRequests::WrapNode, nodeId, configuration);
            }
        }
        else
        {
            // Three maps here.
            // WrapperTypeMapping: Keeps track of which wrappers were created by wrapper type.
            AZStd::unordered_map< AZ::Crc32, AZ::EntityId > wrapperTypeMapping;

            // WrapperIdMapping: Keeps track of EntityId mappings for the Wrappers.
            AZStd::unordered_map< AZ::EntityId, AZ::EntityId > wrapperIdMapping;

            // RequiredWrappers: Keeps track of a map of all of the wrapper types required to be created, along with the nodes
            //                   that wanted to create the nodes.
            AZStd::unordered_multimap< AZ::Crc32, AZ::EntityId > requiredWrappersMapping;

            // In general, we will only ever use 2 at once(in the case of a drag/drop: busType + eventWrapper)
            // In the case of a paste: busIdWrappers + eventWrappers
            // Logic is merged here just to try to reduce the duplicated logic, and because I can't really
            // tell the difference between the two cases anyway.
            //
            // Idea here is to keep track of groupings so that when we paste, I can create the appropriate number
            // of nodes and groupings within these nodes to create a proper duplicate. And when we drag and drop
            // I want to merge as many wrapped elements onto a single node as I can.
            //
            // First step in this process is to sort our pasted nodes into EBus handlers and EBus events.
            for (const AZ::EntityId& nodeId : m_lastGraphCanvasCreationGroup)
            {
                if (GraphCanvas::WrapperNodeRequestBus::FindFirstHandler(nodeId) != nullptr)
                {
                    wrapperIdMapping[nodeId] = nodeId;

                    AZ::Crc32 wrapperType;
                    GraphCanvas::WrapperNodeRequestBus::EventResult(wrapperType, nodeId, &GraphCanvas::WrapperNodeRequests::GetWrapperType);

                    if (wrapperType != AZ::Crc32())
                    {
                        auto mapIter = wrapperTypeMapping.find(wrapperType);

                        if (mapIter == wrapperTypeMapping.end())
                        {
                            wrapperTypeMapping[wrapperType] = nodeId;
                        }
                    }
                }

                if (GraphCanvas::ForcedWrappedNodeRequestBus::FindFirstHandler(nodeId) != nullptr)
                {
                    bool isWrapped = false;

                    GraphCanvas::NodeRequestBus::EventResult(isWrapped, nodeId, &GraphCanvas::NodeRequests::IsWrapped);

                    if (!isWrapped)
                    {
                        AZ::Crc32 wrapperType;
                        GraphCanvas::ForcedWrappedNodeRequestBus::EventResult(wrapperType, nodeId, &GraphCanvas::ForcedWrappedNodeRequests::GetWrapperType);

                        if (wrapperType != AZ::Crc32())
                        {
                            requiredWrappersMapping.emplace(wrapperType,nodeId);
                        }
                    }
                }
            }

            // Second step is to go through, and determine which usage case is valid so we know how to filter down our events.
            // If we can't find a wrapper, or we can't create a handler for the wrapper. We need to delete it.
            AZStd::unordered_set<AZ::EntityId> invalidNodes;

            for (const auto& mapPair : requiredWrappersMapping)
            {
                AZ::EntityId wrapperNodeId;

                // Look up in our previous group mapping to see if it belonged to a node previously
                // (i.e. copy + pasted node).
                AZ::EntityId previousGroupWrapperNodeId;

                auto mapIter = m_wrappedNodeGroupings.find(mapPair.second);

                if (mapIter != m_wrappedNodeGroupings.end())
                {
                    previousGroupWrapperNodeId = mapIter->second;

                    auto busIter = wrapperIdMapping.find(previousGroupWrapperNodeId);

                    if (busIter != wrapperIdMapping.end())
                    {
                        wrapperNodeId = busIter->second;
                    }
                }

                // We may have already found our target node.
                // If we have, bypass the creation step.
                if (!wrapperNodeId.IsValid())
                {
                    // If we haven't check if we match a type, or if our previous group wrapper node is valid.
                    // If we had a previous group. I need to create a wrapper for that group.
                    // If we didn't have a previous group, I want to just use the Bus name to find an appropriate grouping.
                    auto busIter = wrapperTypeMapping.find(mapPair.first);
                    if (busIter == wrapperTypeMapping.end() || previousGroupWrapperNodeId.IsValid())
                    {
                        AZ::EntityId forcedWrappedNodeId = mapPair.second;

                        AZ::Vector2 position;
                        GraphCanvas::GeometryRequestBus::EventResult(position, forcedWrappedNodeId, &GraphCanvas::GeometryRequests::GetPosition);

                        GraphCanvas::ForcedWrappedNodeRequestBus::EventResult(wrapperNodeId, forcedWrappedNodeId, &GraphCanvas::ForcedWrappedNodeRequests::CreateWrapperNode, GetGraphCanvasGraphId(), position);

                        if (wrapperNodeId.IsValid())
                        {
                            m_lastGraphCanvasCreationGroup.emplace_back(wrapperNodeId);

                            if (!previousGroupWrapperNodeId.IsValid())
                            {
                                wrapperTypeMapping.emplace(mapPair.first, wrapperNodeId);
                            }
                            else
                            {
                                wrapperIdMapping.emplace(previousGroupWrapperNodeId, wrapperNodeId);
                            }
                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to instantiate an Wrapper node with type: (%d)", mapPair.first);
                            invalidNodes.insert(mapPair.second);
                            continue;
                        }
                    }
                    else
                    {
                        wrapperNodeId = busIter->second;
                    }
                }

                GraphCanvas::WrappedNodeConfiguration configuration;
                GraphCanvas::WrapperNodeConfigurationRequestBus::EventResult(configuration, wrapperNodeId, &GraphCanvas::WrapperNodeConfigurationRequests::GetWrappedNodeConfiguration, mapPair.second);

                GraphCanvas::WrapperNodeRequestBus::Event(wrapperNodeId, &GraphCanvas::WrapperNodeRequests::WrapNode, mapPair.second, configuration);
            }

            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::Delete, invalidNodes);
        }

        for (AZ::EntityId graphCanvasNodeId : m_lastGraphCanvasCreationGroup)
        {
            OnSaveDataDirtied(graphCanvasNodeId);
            Nodes::CopySlotTranslationKeyedNamesToDatums(graphCanvasNodeId);
        }

        m_wrappedNodeGroupings.clear();
        m_lastGraphCanvasCreationGroup.clear();
        m_wrapperNodeDropTarget.SetInvalid();

        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
    }

    void Graph::PostRestore(const UndoData&)
    {
        AZStd::vector<AZ::EntityId> graphCanvasNodeIds;
        GraphCanvas::SceneRequestBus::EventResult(graphCanvasNodeIds, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetNodes);

        for (AZ::EntityId graphCanvasNodeId : graphCanvasNodeIds)
        {
            Nodes::CopySlotTranslationKeyedNamesToDatums(graphCanvasNodeId);
        }
    }

    void Graph::OnPasteBegin()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
    }

    void Graph::OnPasteEnd()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
    }

    void Graph::OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId)
    {
        m_lastGraphCanvasCreationGroup.emplace_back(nodeId);
    }

    bool Graph::ShouldWrapperAcceptDrop(const AZ::EntityId& wrapperNode, const QMimeData* mimeData) const
    {
        if (!mimeData->hasFormat(Widget::NodePaletteDockWidget::GetMimeType()))
        {
            return false;
        }

        // Deep mime inspection
        QByteArray arrayData = mimeData->data(Widget::NodePaletteDockWidget::GetMimeType());

        GraphCanvas::GraphCanvasMimeContainer mimeContainer;

        if (!mimeContainer.FromBuffer(arrayData.constData(), arrayData.size()) || mimeContainer.m_mimeEvents.empty())
        {
            return false;
        }

        AZStd::string busName;
        EBusHandlerNodeDescriptorRequestBus::EventResult(busName, wrapperNode, &EBusHandlerNodeDescriptorRequests::GetBusName);

        for (GraphCanvas::GraphCanvasMimeEvent* mimeEvent : mimeContainer.m_mimeEvents)
        {
            CreateEBusHandlerEventMimeEvent* createEbusMethodEvent = azrtti_cast<CreateEBusHandlerEventMimeEvent*>(mimeEvent);

            if (createEbusMethodEvent)
            {
                if (createEbusMethodEvent->GetBusName().compare(busName) != 0)
                {
                    return false;
                }

                bool containsEvent = false;
                EBusHandlerNodeDescriptorRequestBus::EventResult(containsEvent, wrapperNode, &EBusHandlerNodeDescriptorRequests::ContainsEvent, createEbusMethodEvent->GetEventId());

                if (containsEvent)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    void Graph::AddWrapperDropTarget(const AZ::EntityId& wrapperNode)
    {
        if (!m_wrapperNodeDropTarget.IsValid())
        {
            m_wrapperNodeDropTarget = wrapperNode;
        }
    }

    void Graph::RemoveWrapperDropTarget(const AZ::EntityId& wrapperNode)
    {
        if (m_wrapperNodeDropTarget == wrapperNode)
        {
            m_wrapperNodeDropTarget.SetInvalid();
        }
    }

    GraphCanvas::GraphId Graph::GetGraphCanvasGraphId() const
    {
        if (m_saveFormatConverted)
        {
            if (m_graphCanvasSceneEntity)
            {
                return m_graphCanvasSceneEntity->GetId();
            }

            return AZ::EntityId();
        }
        else
        {
            return GetEntityId();
        }
    }

    NodeIdPair Graph::CreateCustomNode(const AZ::Uuid& typeId, const AZ::Vector2& position)
    {
        CreateCustomNodeMimeEvent mimeEvent(typeId);

        AZ::Vector2 dropPosition = position;
        
        if (mimeEvent.ExecuteEvent(position, dropPosition, GetGraphCanvasGraphId()))
        {
            return mimeEvent.GetCreatedPair();
        }

        return NodeIdPair();
    }

    void Graph::AddCrcCache(const AZ::Crc32& crcValue, const AZStd::string& cacheString)
    {
        auto mapIter = m_crcCacheMap.find(crcValue);

        if (mapIter == m_crcCacheMap.end())
        {
            m_crcCacheMap.emplace(crcValue, CRCCache(cacheString));
        }
        else
        {
            mapIter->second.m_cacheCount++;
        }
    }

    void Graph::RemoveCrcCache(const AZ::Crc32& crcValue)
    {
        auto mapIter = m_crcCacheMap.find(crcValue);

        if (mapIter != m_crcCacheMap.end())
        {
            mapIter->second.m_cacheCount--;

            if (mapIter->second.m_cacheCount <= 0)
            {
                m_crcCacheMap.erase(mapIter);
            }
        }
    }

    AZStd::string Graph::DecodeCrc(const AZ::Crc32& crcValue)
    {
        auto mapIter = m_crcCacheMap.find(crcValue);
        
        if (mapIter != m_crcCacheMap.end())
        {
            return mapIter->second.m_cacheValue;
        }

        return "";
    }

    void Graph::ClearHighlights()
    {
        for (const GraphCanvas::GraphicsEffectId& effectId : m_highlights)
        {
            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::CancelGraphicsEffect, effectId);
        }
        
        m_highlights.clear();
    }

    void Graph::HighlightMembersFromTreeItem(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        ClearHighlights();

        if (auto handleEbusEventTreeItem = azrtti_cast<const EBusHandleEventPaletteTreeItem*>(treeItem))
        {
            HighlightEBusNodes(handleEbusEventTreeItem->GetBusId(), handleEbusEventTreeItem->GetEventId());
        }
        else if (auto sendScriptEventTreeItem = azrtti_cast<const ScriptEventsEventNodePaletteTreeItem*>(treeItem))
        {
            HighlightScriptEventNodes(sendScriptEventTreeItem->GetBusIdentifier(), sendScriptEventTreeItem->GetEventIdentifier());
        }
        else
        {
            HighlightNodesByType(NodeIdentifierFactory::ConstructNodeIdentifier(treeItem));
        }
    }

    void Graph::HighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
    {
        ClearHighlights();

        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::VariableId lookupId;
            ScriptCanvas::VariableNodeRequestBus::EventResult(lookupId, entity->GetId(), &ScriptCanvas::VariableNodeRequests::GetId);

            if (lookupId.IsValid() && variableIds.count(lookupId) > 0)
            {
                HighlightScriptCanvasEntity(entity->GetId());
            }
        }
    }

    void Graph::HighlightNodes(const AZStd::vector<NodeIdPair>& nodes)
    {
        ClearHighlights();

        for (const NodeIdPair& nodeIdPair : nodes)
        {
            HighlightScriptCanvasEntity(nodeIdPair.m_scriptCanvasId);
        }
    }

    void Graph::RemoveUnusedVariables()
    {
        RequestPushPreventUndoStateUpdate();
        auto variableData = GetVariableData();

        auto variables = variableData->GetVariables();

        AZStd::unordered_set<ScriptCanvas::VariableId> usedVariableIds;

        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::VariableId lookupId;
            ScriptCanvas::VariableNodeRequestBus::EventResult(lookupId, entity->GetId(), &ScriptCanvas::VariableNodeRequests::GetId);

            if (lookupId.IsValid())
            {
                usedVariableIds.insert(lookupId);
            }
        }

        AZStd::unordered_set<ScriptCanvas::VariableId> unusedVariables;

        for (auto variableData : variables)
        {
            if (usedVariableIds.count(variableData.first) == 0)
            {
                unusedVariables.insert(variableData.first);
            }
        }

        bool removedVariable = false;

        for (ScriptCanvas::VariableId variableId : unusedVariables)
        {
            bool success = false;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(success, GetEntityId(), &ScriptCanvas::GraphVariableManagerRequests::RemoveVariable, variableId);

            if (success)
            {
                removedVariable = true;
            }
        }

        RequestPopPreventUndoStateUpdate();

        if (removedVariable)
        {
            RequestUndoPoint();
        }
    }

    void Graph::OnVersionConversionBegin()
    {        
    }

    void Graph::OnVersionConversionEnd()
    {
        const AZ::EntityId* busId = VersionControlledNodeNotificationBus::GetCurrentBusId();

        if (busId == nullptr)
        {
            return;
        }

        AZ::EntityId graphCanvasNodeId = (*busId);

        size_t removeCount = m_convertingNodes.erase(graphCanvasNodeId);

        if (removeCount >= 0)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (userData && userData->is<AZ::EntityId>())
            {
                AZ::EntityId scriptCanvasNodeId = (*AZStd::any_cast<AZ::EntityId>(userData));

                AZStd::vector< const ScriptCanvas::Slot* > nodeSlots;
                ScriptCanvas::NodeRequestBus::EventResult(nodeSlots, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetAllSlots);

                for (const ScriptCanvas::Slot* slot : nodeSlots)
                {
                    ScriptCanvas::Endpoint endpoint(scriptCanvasNodeId, slot->GetId());

                    AZStd::vector< ScriptCanvas::Endpoint > connectedEndpoints = GetConnectedEndpoints(endpoint);

                    for (const ScriptCanvas::Endpoint& connectedEndpoint : connectedEndpoints)
                    {
                        if (IsNodeVersionConverting(connectedEndpoint.GetNodeId()))
                        {
                            continue;
                        }

                        bool acceptConnection = false;

                        if (slot->IsInput())
                        {
                            if (CanConnectionExistBetween(connectedEndpoint, endpoint).IsSuccess())
                            {
                                AZ::EntityId sourceGraphCanvasId;
                                SceneMemberMappingRequestBus::EventResult(sourceGraphCanvasId, connectedEndpoint.GetNodeId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);                                

                                AZ::EntityId sourceSlotId = FindGraphCanvasSlotId(sourceGraphCanvasId, connectedEndpoint.GetSlotId());

                                if (sourceSlotId.IsValid())
                                {
                                    AZ::EntityId targetSlotId = FindGraphCanvasSlotId(graphCanvasNodeId, endpoint.GetSlotId());

                                    if (targetSlotId.IsValid())
                                    {
                                        acceptConnection = true;

                                        GraphCanvas::Endpoint sourceEndpoint(sourceGraphCanvasId, sourceSlotId);

                                        bool isConnectedTo = false;
                                        GraphCanvas::SlotRequestBus::EventResult(isConnectedTo, targetSlotId, &GraphCanvas::SlotRequests::IsConnectedTo, sourceEndpoint);

                                        if (!isConnectedTo)
                                        {
                                            AZ::EntityId connectionId;
                                            GraphCanvas::SlotRequestBus::EventResult(connectionId, targetSlotId, &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, sourceEndpoint);

                                            acceptConnection = ConfigureConnectionUserData(connectedEndpoint, endpoint, connectionId);
                                        }
                                    }
                                }
                            }
                        }
                        else if (slot->IsOutput())
                        {
                            if (CanConnectionExistBetween(endpoint, connectedEndpoint).IsSuccess())
                            {
                                AZ::EntityId targetGraphCanvasId;
                                SceneMemberMappingRequestBus::EventResult(targetGraphCanvasId, connectedEndpoint.GetNodeId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                                AZ::EntityId sourceSlotId = FindGraphCanvasSlotId(targetGraphCanvasId, connectedEndpoint.GetSlotId());

                                if (sourceSlotId.IsValid())
                                {
                                    AZ::EntityId targetSlotId = FindGraphCanvasSlotId(graphCanvasNodeId, endpoint.GetSlotId());

                                    if (targetSlotId.IsValid())
                                    {
                                        acceptConnection = true;
                                        GraphCanvas::Endpoint sourceEndpoint(targetGraphCanvasId, sourceSlotId);

                                        bool isConnectedTo = false;
                                        GraphCanvas::SlotRequestBus::EventResult(isConnectedTo, targetSlotId, &GraphCanvas::SlotRequests::IsConnectedTo, sourceEndpoint);

                                        if (!isConnectedTo)
                                        {
                                            AZ::EntityId connectionId;
                                            GraphCanvas::SlotRequestBus::EventResult(connectionId, targetSlotId, &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, sourceEndpoint);

                                            acceptConnection = ConfigureConnectionUserData(endpoint, connectedEndpoint, connectionId);
                                        }
                                    }
                                }
                            }
                        }

                        if (!acceptConnection)
                        {
                            AZ::Entity* connectionEntity = nullptr;

                            if (FindConnection(connectionEntity, endpoint, connectedEndpoint))
                            {
                                RemoveConnection(connectionEntity->GetId());
                            }
                        }
                    }
                }
            }
        }

        SignalDirty();
        VersionControlledNodeNotificationBus::MultiHandler::BusDisconnect(graphCanvasNodeId);
    }

    AZStd::vector<NodeIdPair> Graph::GetNodesOfType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        AZStd::vector<NodeIdPair> nodeIdPairs;

        for (AZ::Entity* entity : GetGraphData()->m_nodes)
        {
            ScriptCanvas::Node* canvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(entity);

            if (canvasNode->GetNodeType() == nodeTypeIdentifier)
            {
                NodeIdPair nodeIdPair;
                nodeIdPair.m_scriptCanvasId = entity->GetId();

                SceneMemberMappingRequestBus::EventResult(nodeIdPair.m_graphCanvasId, entity->GetId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                nodeIdPairs.emplace_back(nodeIdPair);
            }
            else
            {
                ScriptCanvas::Nodes::Core::EBusEventHandler* handlerNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

                if (handlerNode)
                {
                    ScriptCanvas::EBusBusId busId = handlerNode->GetEBusId();

                    for (auto eventPair : handlerNode->GetEvents())
                    {
                        ScriptCanvas::EBusEventId eventId = eventPair.second.m_eventId;
                        
                        if (ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(busId, eventId) == nodeTypeIdentifier)
                        {
                            AZ::EntityId graphCanvasNodeId;
                            SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, canvasNode->GetEntityId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);

                            bool hasEvent = false;
                            EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventId);

                            if (hasEvent)
                            {
                                NodeIdPair nodeIdPair;
                                nodeIdPair.m_scriptCanvasId = entity->GetId();
                                nodeIdPair.m_graphCanvasId = graphCanvasNodeId;

                                nodeIdPairs.emplace_back(nodeIdPair);
                            }
                        }
                    }
                }
            }
        }

        return nodeIdPairs;
    }

    AZStd::vector<NodeIdPair> Graph::GetVariableNodes(const ScriptCanvas::VariableId& variableId)
    {
        AZStd::vector<NodeIdPair> variableNodes;

        if (variableId.IsValid())
        {
            for (AZ::Entity* entity : GetGraphData()->m_nodes)
            {
                ScriptCanvas::VariableId lookupId;
                ScriptCanvas::VariableNodeRequestBus::EventResult(lookupId, entity->GetId(), &ScriptCanvas::VariableNodeRequests::GetId);

                if (variableId == lookupId)
                {
                    NodeIdPair nodeIdPair;
                    nodeIdPair.m_scriptCanvasId = entity->GetId();
                    SceneMemberMappingRequestBus::EventResult(nodeIdPair.m_graphCanvasId, entity->GetId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);
                    variableNodes.push_back(nodeIdPair);
                }
            }
        }

        return variableNodes;
    }

    void Graph::QueueVersionUpdate(const AZ::EntityId& graphCanvasNodeId)
    {
        bool queueUpdate = m_queuedConvertingNodes.empty();
        auto insertResult = m_queuedConvertingNodes.insert(graphCanvasNodeId);

        if (insertResult.second && queueUpdate)
        {
            QTimer::singleShot(0, [this]() { this->HandleQueuedUpdates(); });
        }
    }

    void Graph::OnSaveDataDirtied(const AZ::EntityId& savedElement)
    {
        // The EbusHandlerEvent's are a visual only representation of alternative data, and should not be saved.
        if (EBusHandlerEventNodeDescriptorRequestBus::FindFirstHandler(savedElement) != nullptr
            || m_ignoreSaveRequests)
        {
            return;
        }

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, savedElement, &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            const AZ::EntityId* scriptCanvasNodeId = AZStd::any_cast<AZ::EntityId>(userData);
            GraphCanvas::EntitySaveDataContainer* container = nullptr;

            auto mapIter = m_graphCanvasSaveData.find((*scriptCanvasNodeId));

            if (mapIter == m_graphCanvasSaveData.end())
            {
                container = aznew GraphCanvas::EntitySaveDataContainer();
                m_graphCanvasSaveData[(*scriptCanvasNodeId)] = container;
            }
            else
            {
                container = mapIter->second;
            }

            GraphCanvas::EntitySaveDataRequestBus::Event(savedElement, &GraphCanvas::EntitySaveDataRequests::WriteSaveData, (*container));
        }
        else if (savedElement == GetGraphCanvasGraphId())
        {
            GraphCanvas::EntitySaveDataContainer* container = nullptr;
            auto mapIter = m_graphCanvasSaveData.find(GetEntityId());

            if (mapIter == m_graphCanvasSaveData.end())
            {
                container = aznew GraphCanvas::EntitySaveDataContainer();
                m_graphCanvasSaveData[GetEntityId()] = container;
            }
            else
            {
                container = mapIter->second;
            }

            GraphCanvas::EntitySaveDataRequestBus::Event(savedElement, &GraphCanvas::EntitySaveDataRequests::WriteSaveData, (*container));

            m_statisticsHelper.PopulateStatisticData(this);
        }
    }

    bool Graph::NeedsSaveConversion() const
    {
        return !m_saveFormatConverted;
    }

    void Graph::ConvertSaveFormat()
    {
        if (!m_saveFormatConverted)
        {
            // Bit of a work around for not being able to clean this up in the actual save.
            m_saveFormatConverted = true;

            // SceneComponent
            for (const AZ::Uuid& componentType : {
                "{3F71486C-3D51-431F-B904-DA070C7A0238}", // GraphCanvas::SceneComponent
                "{486B009F-632B-44F6-81C2-3838746190AE}", // ColorPaletteManagerComponent
                "{A8F08DEA-0F42-4236-9E1E-B93C964B113F}", // BookmarkManagerComponent
                "{34B81206-2C69-4886-945B-4A9ECC0FDAEE}"  // StyleSheet
            }
                )
            {
                AZ::Component* component = GetEntity()->FindComponent(componentType);

                if (component)
                {
                    if (GetEntity()->RemoveComponent(component))
                    {
                        delete component;
                    }
                }
            }
        }
    }

    void Graph::ConstructSaveData()
    {
        // Save out the SceneData
        //
        // For this one all of the GraphCanvas information lives on the same entity.
        // So we need to use that key to look up everything
        {
            OnSaveDataDirtied(GetGraphCanvasGraphId());
        }
        
        AZStd::vector< AZ::EntityId > graphCanvasNodes;
        GraphCanvas::SceneRequestBus::EventResult(graphCanvasNodes, GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetNodes);

        for (const AZ::EntityId& graphCanvasNode : graphCanvasNodes)
        {
            OnSaveDataDirtied(graphCanvasNode);
        }
    }

    const GraphStatisticsHelper& Graph::GetNodeUsageStatistics() const
    {
        return m_statisticsHelper;
    }

    void Graph::CreateGraphCanvasScene()
    {
        if (!m_saveFormatConverted)
        {
            GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);
            GraphCanvas::GraphModelRequestBus::Handler::BusConnect(graphCanvasGraphId);

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SetEditorId, ScriptCanvasEditor::AssetEditorId);

            AZStd::any* userData = nullptr;
            GraphCanvas::SceneRequestBus::EventResult(userData, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetUserData);

            if (userData)
            {
                (*userData) = GetEntityId();
            }
        }
        else if (m_graphCanvasSceneEntity == nullptr)
        {
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(m_graphCanvasSceneEntity, &GraphCanvas::GraphCanvasRequests::CreateSceneAndActivate);

            if (m_graphCanvasSceneEntity == nullptr)
            {
                return;
            }

            AZ::EntityId graphCanvasGraphId = GetGraphCanvasGraphId();
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SetEditorId, ScriptCanvasEditor::AssetEditorId);

            DisplayGraphCanvasScene();

            AZStd::any* userData = nullptr;
            GraphCanvas::SceneRequestBus::EventResult(userData, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetUserData);

            if (userData)
            {
                (*userData) = GetEntityId();
            }
        }
    }

    void Graph::DisplayGraphCanvasScene()
    {
        RequestPushPreventUndoStateUpdate();

        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > scriptCanvasToGraphCanvasMapping;

        {
            QScopedValueRollback<bool> ignoreRequests(m_ignoreSaveRequests, true);

            GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

            GraphCanvas::GraphModelRequestBus::Handler::BusConnect(graphCanvasGraphId);
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphCanvasGraphId);

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SignalLoadStart);

            auto saveDataIter = m_graphCanvasSaveData.find(GetEntityId());

            if (saveDataIter != m_graphCanvasSaveData.end())
            {
                GraphCanvas::EntitySaveDataRequestBus::Event(graphCanvasGraphId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, (*saveDataIter->second));
            }

            ScriptCanvas::NodeIdList nodeList = GetNodes();

            bool graphNeedsDirtying = false;

            for (const AZ::EntityId& scriptCanvasNodeId : nodeList)
            {
                ScriptCanvas::Node* scriptCanvasNode = nullptr;

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasNodeId);

                if (entity)
                {
                    scriptCanvasNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(entity);

                    if (scriptCanvasNode)
                    {
                        AZ::EntityId graphCanvasNodeId = Nodes::DisplayScriptCanvasNode(graphCanvasGraphId, scriptCanvasNode);
                        scriptCanvasToGraphCanvasMapping[scriptCanvasNodeId] = graphCanvasNodeId;

                        auto saveDataIter = m_graphCanvasSaveData.find(scriptCanvasNodeId);

                        if (saveDataIter != m_graphCanvasSaveData.end())
                        {
                            GraphCanvas::EntitySaveDataRequestBus::Event(graphCanvasNodeId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, (*saveDataIter->second));
                        }

                        AZ::Vector2 position;
                        GraphCanvas::GeometryRequestBus::EventResult(position, graphCanvasNodeId, &GraphCanvas::GeometryRequests::GetPosition);

                        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, graphCanvasNodeId, position);
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Could not find ScriptCanvasNode Component on Entity - %s(%llu)", entity->GetName().c_str(), static_cast<AZ::u64>(scriptCanvasNodeId));
                    }
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false, "Could not find Entity with ID - %llu", static_cast<AZ::u64>(scriptCanvasNodeId));
                }
            }

            AZStd::vector< AZ::EntityId > connectionIds = GetConnections();

            for (const AZ::EntityId& connectionId : connectionIds)
            {
                ScriptCanvas::Endpoint scriptCanvasSourceEndpoint;
                ScriptCanvas::Endpoint scriptCanvasTargetEndpoint;

                ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasSourceEndpoint, connectionId, &ScriptCanvas::ConnectionRequests::GetSourceEndpoint);
                ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasTargetEndpoint, connectionId, &ScriptCanvas::ConnectionRequests::GetTargetEndpoint);

                AZ::EntityId graphCanvasSourceNode;

                auto scriptCanvasIter = scriptCanvasToGraphCanvasMapping.find(scriptCanvasSourceEndpoint.GetNodeId());

                if (scriptCanvasIter != scriptCanvasToGraphCanvasMapping.end())
                {
                    graphCanvasSourceNode = scriptCanvasIter->second;
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false, "Could not find ScriptCanvas Node with id %llu", static_cast<AZ::u64>(scriptCanvasSourceEndpoint.GetNodeId()));
                }

                AZ::EntityId graphCanvasSourceSlotId;
                SlotMappingRequestBus::EventResult(graphCanvasSourceSlotId, graphCanvasSourceNode, &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSourceEndpoint.GetSlotId());

                if (!graphCanvasSourceSlotId.IsValid())
                {
                    // For the EBusHandler's I need to remap these to a different visual node.
                    // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
                    if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasSourceNode) != nullptr)
                    {
                        GraphCanvas::Endpoint graphCanvasEventEndpoint;
                        EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasEventEndpoint, graphCanvasSourceNode, &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, scriptCanvasSourceEndpoint.GetSlotId());

                        graphCanvasSourceSlotId = graphCanvasEventEndpoint.GetSlotId();
                    }

                    if (!graphCanvasSourceSlotId.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", false, "Could not create connection(%s) for Node(%s).", connectionId.ToString().c_str(), scriptCanvasSourceEndpoint.GetNodeId().ToString().c_str());
                        DisconnectById(connectionId);
                        continue;
                    }
                }

                GraphCanvas::Endpoint graphCanvasTargetEndpoint;

                scriptCanvasIter = scriptCanvasToGraphCanvasMapping.find(scriptCanvasTargetEndpoint.GetNodeId());

                if (scriptCanvasIter != scriptCanvasToGraphCanvasMapping.end())
                {
                    graphCanvasTargetEndpoint.m_nodeId = scriptCanvasIter->second;
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false, "Could not find ScriptCanvas Node with id %llu", static_cast<AZ::u64>(scriptCanvasSourceEndpoint.GetNodeId()));
                }

                SlotMappingRequestBus::EventResult(graphCanvasTargetEndpoint.m_slotId, graphCanvasTargetEndpoint.GetNodeId(), &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasTargetEndpoint.GetSlotId());

                if (!graphCanvasTargetEndpoint.IsValid())
                {
                    // For the EBusHandler's I need to remap these to a different visual node.
                    // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
                    if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasTargetEndpoint.GetNodeId()) != nullptr)
                    {
                        EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasTargetEndpoint, graphCanvasTargetEndpoint.GetNodeId(), &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, scriptCanvasTargetEndpoint.GetSlotId());
                    }

                    if (!graphCanvasTargetEndpoint.IsValid())
                    {
                        AZ_Warning("ScriptCanvas", false, "Could not create connection(%s) for Node(%s).", connectionId.ToString().c_str(), scriptCanvasSourceEndpoint.GetNodeId().ToString().c_str());
                        DisconnectById(connectionId);
                        continue;
                    }
                }

                AZ::EntityId graphCanvasConnectionId;
                GraphCanvas::SlotRequestBus::EventResult(graphCanvasConnectionId, graphCanvasSourceSlotId, &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, graphCanvasTargetEndpoint);

                if (graphCanvasConnectionId.IsValid())
                {
                    AZStd::any* userData = nullptr;
                    GraphCanvas::ConnectionRequestBus::EventResult(userData, graphCanvasConnectionId, &GraphCanvas::ConnectionRequests::GetUserData);

                    if (userData)
                    {
                        (*userData) = connectionId;

                        SceneMemberMappingConfigurationRequestBus::Event(graphCanvasConnectionId, &SceneMemberMappingConfigurationRequests::ConfigureMapping, connectionId);
                    }
                }
            }

            // Fix up leaked data elements
            auto mapIter = m_graphCanvasSaveData.begin();

            while (mapIter != m_graphCanvasSaveData.end())
            {
                // Deleted using the wrong id, which orphaned the SaveData. For now we want to go through and sanitize our save data to avoid keeping around a bunch
                // of old save data for no reason.
                if (scriptCanvasToGraphCanvasMapping.find(mapIter->first) == scriptCanvasToGraphCanvasMapping.end())
                {
                    delete mapIter->second;
                    mapIter = m_graphCanvasSaveData.erase(mapIter);
                }
                else
                {
                    ++mapIter;
                }
            }

            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::SignalLoadEnd);
            EditorGraphNotificationBus::Event(GetEntityId(), &EditorGraphNotifications::OnGraphCanvasSceneDisplayed);

            AZStd::unordered_set<AZ::EntityId> outOfDateNodes;

            VersionControlledScrapperBus::EnumerateHandlers([&outOfDateNodes, this](VersionControlledScrapperRequests* versionControlledElement)
            {
                if (versionControlledElement->IsOutOfDate())
                {
                    AZ::EntityId nodeId = versionControlledElement->GetNodeId();

                    this->m_convertingNodes.emplace(nodeId);
                    outOfDateNodes.emplace(nodeId);
                }

                return true;
            });

            for (auto nodeId : outOfDateNodes)
            {
                VersionControlledNodeNotificationBus::MultiHandler::BusConnect(nodeId);
                VersionControlledNodeRequestBus::Event(nodeId, &VersionControlledNodeRequests::UpdateNodeVersion);
            }
        }

        if (m_graphCanvasSaveVersion != GraphCanvas::EntitySaveDataContainer::CurrentVersion)
        {
            for (auto saveDataPair : m_graphCanvasSaveData)
            {
                auto graphCanvasIter = scriptCanvasToGraphCanvasMapping.find(saveDataPair.first);
                OnSaveDataDirtied(graphCanvasIter->second);
            }

            m_graphCanvasSaveVersion = GraphCanvas::EntitySaveDataContainer::CurrentVersion;
            SignalDirty();
        }

        RequestPopPreventUndoStateUpdate();
    }

    AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > Graph::GetGraphCanvasSaveData()
    {
        return m_graphCanvasSaveData;
    }

    void Graph::UpdateGraphCanvasSaveData(const AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* >& saveData)
    {
        QScopedValueRollback<bool> ignoreRequests(m_ignoreSaveRequests, true);

        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect(graphCanvasGraphId);
        GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect(graphCanvasGraphId);

        for (auto& entry : m_graphCanvasSaveData)
        {
            delete entry.second;
        }

        m_graphCanvasSaveData = saveData;

        DisplayGraphCanvasScene();
    }

    void Graph::ClearGraphCanvasScene()
    {
        GraphCanvas::GraphId graphCanvasGraphId = GetGraphCanvasGraphId();

        RequestPushPreventUndoStateUpdate();

        // Wipe out all of the Graph Canvas Visuals
        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearScene);

        RequestPopPreventUndoStateUpdate();
    }    
} // namespace ScriptCanvasEditor