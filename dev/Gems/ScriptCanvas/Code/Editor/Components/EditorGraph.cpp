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

#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Types/SceneSerialization.h>

#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <Core/PureData.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Nodes/NodeUtils.h>

#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasBoolDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasEntityIdDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasDoubleDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasColorDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasReadOnlyDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasStringDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVectorDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasVariableDataInterface.h>
#include <Editor/GraphCanvas/DataInterfaces/ScriptCanvasQuaternionDataInterface.h>


#include <Editor/Nodes/ScriptCanvasAssetNode.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/View/Widgets/NodePalette.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>


namespace ScriptCanvasEditor
{
    namespace EditorGraph
    {
        static const char* GetMimeType()
        {
            return "application/x-lumberyard-scriptcanvas";
        }

        static const char* GetEbusGroupingMimeType()
        {
            return "application/x-lumberyard-scriptcanvas-ebusgrouping";
        }
    }

    struct NodeEntityContainer
    {
        AZ_TYPE_INFO(NodeEntityContainer, "{E873868D-7445-466E-B483-09F9EB026CD2}");
        AZ_CLASS_ALLOCATOR(NodeEntityContainer, AZ::SystemAllocator, 0);
        NodeEntityContainer() = default;

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeEntityContainer>()
                    ->Field("m_entityRefs", &NodeEntityContainer::m_entities)
                    ;
            }
        }

        AZStd::unordered_set<AZ::Entity*> m_entities;
    };

    void Graph::Reflect(AZ::ReflectContext* context)
    {
        NodeEntityContainer::Reflect(context);
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Graph, ScriptCanvas::Graph>()
                ->Field("m_variableCounter", &Graph::m_variableCounter)
                ;
        }
    }

    void Graph::Activate()
    {
        // Overridden to prevent graph execution in the editor
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::NodePropertySourceRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::ConnectionSceneRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::WrapperNodeActionRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::VariableActionRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::DataSlotActionRequestBus::Handler::BusConnect(GetEntityId());
        NodeCreationNotificationBus::Handler::BusConnect(GetEntityId());
        VariableNodeSceneRequestBus::Handler::BusConnect(GetEntityId());
        ScriptCanvas::GraphRequestBus::MultiHandler::BusConnect(GetEntityId());
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void Graph::Deactivate()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ScriptCanvas::GraphRequestBus::MultiHandler::BusDisconnect(GetEntityId());
        VariableNodeSceneRequestBus::Handler::BusDisconnect();
        NodeCreationNotificationBus::Handler::BusDisconnect();
        GraphCanvas::DataSlotActionRequestBus::Handler::BusDisconnect();
        GraphCanvas::VariableActionRequestBus::Handler::BusDisconnect();
        GraphCanvas::WrapperNodeActionRequestBus::Handler::BusDisconnect();
        GraphCanvas::ConnectionSceneRequestBus::Handler::BusDisconnect();
        GraphCanvas::NodePropertySourceRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    void Graph::OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntityMap)
    {
        // Remap Editor EntityId references on ScriptCanvas nodes after a set of Editor entities has been replaced with a slice
        bool entitiesRemapped = false;
        auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        using EntityIdRemapper = AZ::IdUtils::Remapper<AZ::EntityId>;
        EntityIdRemapper::RemapIds(this, [&entitiesRemapped, &replacedEntityMap](const AZ::EntityId& editorId, bool, const EntityIdRemapper::IdGenerator&) -> AZ::EntityId
        {
            auto findIt = replacedEntityMap.find(editorId);
            if (findIt == replacedEntityMap.end())
            {
                return editorId;
            }
            entitiesRemapped = true;
            return findIt->second;
        }, serializeContext, false);

        // When the ScriptCanvas graph has references to Editor Entities that have been added to a slice,
        // the EntityId remapping will be recorded to the UndoStack and the graph is saved to it's associated file name.
        if (entitiesRemapped)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
            
            AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset;
            EditorScriptCanvasRequestBus::EventResult(scriptCanvasAsset, GetUniqueId(), &EditorScriptCanvasRequests::GetAsset);
            AZ::Data::AssetInfo suppliedAssetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(suppliedAssetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, scriptCanvasAsset.GetId());
            DocumentContextRequestBus::Broadcast(&DocumentContextRequests::SaveScriptCanvasAsset, suppliedAssetInfo, scriptCanvasAsset, nullptr);
        }
    }

    static void CompileVisitor(NodeEntityContainer& graphItems, ScriptCanvasAssetNode& scriptCanvasAssetNode)
    {
        scriptCanvasAssetNode.Visit([&graphItems](ScriptCanvasAssetNode& scriptCanvasAssetNode)
        {
            auto subGraphEntity = scriptCanvasAssetNode.GetScriptCanvasEntity();
            auto subGraph = subGraphEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(subGraphEntity) : nullptr;

            if (subGraph)
            {
                const AZStd::unordered_set<AZ::Entity*>& itemEntities = subGraph->GetItems();
                for (AZ::Entity* itemEntity : itemEntities)
                {
                    if (!AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasAssetNode>(itemEntity))
                    {
                        graphItems.m_entities.insert(itemEntity);
                    }
                }
            }
            return true;
        }, {});
    }

    ScriptCanvas::Graph* Graph::Compile(const AZ::EntityId& graphOwnerId)
    {
        ScriptCanvas::Graph* newGraph{};

        NodeEntityContainer graphItems;
        const AZStd::unordered_set<AZ::Entity*>& itemEntities = GetItems();
        for (AZ::Entity* itemEntity : itemEntities)
        {
            if (auto scriptCanvasAssetNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasAssetNode>(itemEntity))
            {
                CompileVisitor(graphItems, *scriptCanvasAssetNode);
            }
            else
            {
                graphItems.m_entities.insert(itemEntity);
            }
        }

        NodeEntityContainer clonedItems;
        auto serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        serializeContext->CloneObjectInplace(clonedItems, &graphItems);

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> editorToRuntimeMap;
        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&clonedItems, editorToRuntimeMap);

        newGraph = aznew ScriptCanvas::Graph();
        newGraph->AddItems(clonedItems.m_entities);
        newGraph->ResolveSelfReferences(graphOwnerId);

        return newGraph;
    }

    void Graph::OnEntitiesSerialized(GraphCanvas::SceneSerialization& serializationTarget)
    {
        const GraphCanvas::SceneData& sceneData = serializationTarget.GetSceneData();

        AZStd::unordered_set< AZ::EntityId > ebusEventNodes;

        AZStd::unordered_set<AZ::Entity*> scriptCanvasEntities;
        for (const auto& node : sceneData.m_nodes)
        {
            // EBus Event nodes are purely visual, but require some user data manipulation in order to function correctly.
            // As such we don't want to copy over their script canvas user data, since it's not what was intended to be copied.
            if (EBusEventNodeDescriptorRequestBus::FindFirstHandler(node->GetId()) == nullptr)
            {
                AZStd::any* userData = nullptr;
                GraphCanvas::NodeRequestBus::EventResult(userData, node->GetId(), &GraphCanvas::NodeRequests::GetUserData);
                auto scriptCanvasNodeId = userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
                AZ::Entity* scriptCanvasEntity = AzFramework::EntityReference(scriptCanvasNodeId).GetEntity();
                if (scriptCanvasEntity)
                {
                    scriptCanvasEntities.emplace(scriptCanvasEntity);
                }
            }
            else
            {
                ebusEventNodes.insert(node->GetId());
            }
        }

        for (const auto& connection : sceneData.m_connections)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::ConnectionRequestBus::EventResult(userData, connection->GetId(), &GraphCanvas::ConnectionRequests::GetUserData);
            auto scriptCanvasConnectionId = userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
            AZ::Entity* scriptCanvasEntity = AzFramework::EntityReference(scriptCanvasConnectionId).GetEntity();
            if (scriptCanvasEntity)
            {
                scriptCanvasEntities.emplace(scriptCanvasEntity);
            }
        }

        auto& userDataMap = serializationTarget.GetUserDataMapRef();

        AZStd::unordered_set<AZ::Entity*> graphData = CopyItems(scriptCanvasEntities);
        userDataMap.emplace(EditorGraph::GetMimeType(), graphData );

        if (!ebusEventNodes.empty())
        {
            // Keep track of which ebus methods were grouped together when we serialized them out.
            // This is so when we recreate them, we can create the appropriate number of
            // EBus wrappers and put the correct methods into each.
            EbusMethodGroupingMap ebusMethodGroupings;

            for (const AZ::EntityId& ebusEventNode : ebusEventNodes)
            {
                NodeIdPair ebusNodeId;
                EBusEventNodeDescriptorRequestBus::EventResult(ebusNodeId, ebusEventNode, &EBusEventNodeDescriptorRequests::GetEbusWrapperNodeId);

                if (ebusNodeId.m_graphCanvasId.IsValid())
                {
                    ebusMethodGroupings.emplace(ebusEventNode, ebusNodeId.m_graphCanvasId);
                }
            }

            userDataMap.emplace(EditorGraph::GetEbusGroupingMimeType(), ebusMethodGroupings);
        }
    }
    
    void Graph::OnEntitiesDeserialized(const GraphCanvas::SceneSerialization& serializationSource)
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

        userDataIt = userDataMap.find(EditorGraph::GetEbusGroupingMimeType());

        if (userDataIt != userDataMap.end())
        {
            // Serialization system handled remapping this map data so we can just insert them into our map.
            const EbusMethodGroupingMap* ebusMethodGroupings = AZStd::any_cast<EbusMethodGroupingMap>(&userDataIt->second);
            m_ebusMethodGroupings.insert(ebusMethodGroupings->begin(), ebusMethodGroupings->end());
        }

        const GraphCanvas::SceneData& sceneData = serializationSource.GetSceneData();
        for (auto nodeEntity : sceneData.m_nodes)
        {
            NodeCreationNotificationBus::Event(GetEntityId(), &NodeCreationNotifications::OnGraphCanvasNodeCreated, nodeEntity->GetId());
        }
    }

    void Graph::DisconnectConnection(const AZ::EntityId& connectionId)
    {
        AZStd::any* connectionUserData = nullptr;
        GraphCanvas::ConnectionRequestBus::EventResult(connectionUserData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);
        auto scConnectionId = connectionUserData && connectionUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(connectionUserData) : AZ::EntityId();
        DisconnectById(scConnectionId);
    }

    bool Graph::CreateConnection(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint)
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
            AZ::Entity* scConnectionEntity = nullptr;
            FindConnection(scConnectionEntity, scSourceEndpoint, scTargetEndpoint);

            if (scConnectionEntity)
            {
                AZStd::any* connectionUserData = nullptr;
                GraphCanvas::ConnectionRequestBus::EventResult(connectionUserData, connectionId, &GraphCanvas::ConnectionRequests::GetUserData);

                if (connectionUserData)
                {
                    *connectionUserData = scConnectionEntity->GetId();
                }
            }
            else
            {
                scConnected = false;
            }
        }

        return scConnected;
    }

    bool Graph::IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const
    {
        AZStd::any* userData = nullptr;

        ScriptCanvas::Endpoint scSourceEndpoint = ConvertToScriptCanvasEndpoint(sourcePoint);
        ScriptCanvas::Endpoint scTargetEndpoint = ConvertToScriptCanvasEndpoint(targetPoint);

        // Need to confirm with the set variable that we can actually set the correct type.
        // Quick work around until the underlying system can be queried correctly.
        //
        // (Caused by the fact that a set variable connection is actually 2 connections
        //  one of which might fail. So we just need to confirm both connections in order to signal]
        //  that it is valid).
        bool isSetVariable = false;
        NodeDescriptorRequestBus::EventResult(isSetVariable, targetPoint.GetNodeId(), &NodeDescriptorRequests::IsType, NodeDescriptorType::SetVariable);
        if (isSetVariable)
        {
            AZ::EntityId variableId;
            SetVariableNodeDescriptorRequestBus::EventResult(variableId, targetPoint.GetNodeId(), &SetVariableNodeDescriptorRequests::GetVariableId);

            if (variableId.IsValid())
            {
                if (GraphCanvas::DataSlotRequestBus::FindFirstHandler(targetPoint.GetSlotId()) != nullptr)
                {
                    ScriptCanvas::Endpoint variableSetEndpoint;
                    VariableNodeDescriptorRequestBus::EventResult(variableSetEndpoint, variableId, &VariableNodeDescriptorRequests::GetWriteEndpoint);

                    if (!CanConnectByEndpoint(scSourceEndpoint, variableSetEndpoint).IsSuccess())
                    {
                        return false;
                    }
                }
            }
            else
            {
                return false;
            }
        }

        return CanConnectByEndpoint(scSourceEndpoint, scTargetEndpoint).IsSuccess();
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

        return CanConnectByEndpoint(variableSourceEndpoint, targetEndpoint).IsSuccess();
    }

    double Graph::GetSnapDistance() const
    {
        AZStd::intrusive_ptr<EditorSettings::PreviewSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::PreviewSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (settings)
        {
            return settings->m_snapDistance;
        }

        return 10.0;
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const
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

    GraphCanvas::NodePropertyDisplay* Graph::CreateDataSlotVariablePropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const
    {
        GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;

        AZStd::any* nodeUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scriptCanvasNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

        AZStd::any* slotUserData = nullptr;
        GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotId, &GraphCanvas::SlotRequests::GetUserData);
        auto scriptCanvasSlotId = slotUserData && slotUserData->is<ScriptCanvas::SlotId>() ? *AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData) : ScriptCanvas::SlotId();

        ScriptCanvasVariableDataInterface* variableInterface = aznew ScriptCanvasVariableDataInterface(dataType, nodeId, slotId, scriptCanvasNodeId, scriptCanvasSlotId);
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVariableReferenceNodePropertyDisplay, variableInterface);

        if (dataDisplay == nullptr)
        {
            delete variableInterface;
        }

        return dataDisplay;
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const
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
        else if (propertyId == PropertySlotIds::VariableName)
        {
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;
            GraphCanvas::StringDataInterface* dataInterface = nullptr;

            VariableNodeDescriptorRequestBus::EventResult(dataInterface, nodeId, &VariableNodeDescriptorRequests::CreateNameInterface);

            if (dataInterface != nullptr)
            {
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay, dataInterface);
            }

            if (dataDisplay == nullptr)
            {
                delete dataInterface;
            }

            return dataDisplay;
        }
        else if (propertyId == PropertySlotIds::GetVariableReference)
        {
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;
            GraphCanvas::VariableReferenceDataInterface* dataInterface = nullptr;

            GetVariableNodeDescriptorRequestBus::EventResult(dataInterface, nodeId, &GetVariableNodeDescriptorRequests::CreateVariableDataInterface);

            if (dataInterface != nullptr)
            {
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVariableReferenceNodePropertyDisplay, dataInterface);
            }

            if (dataDisplay == nullptr)
            {
                delete dataInterface;
            }

            return dataDisplay;
        }
        else if (propertyId == PropertySlotIds::SetVariableReference)
        {
            GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;
            GraphCanvas::VariableReferenceDataInterface* dataInterface = nullptr;

            SetVariableNodeDescriptorRequestBus::EventResult(dataInterface, nodeId, &SetVariableNodeDescriptorRequests::CreateVariableDataInterface);

            if (dataInterface != nullptr)
            {
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVariableReferenceNodePropertyDisplay, dataInterface);
            }

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

        bool isGetVariableNode = false;
        NodeDescriptorRequestBus::EventResult(isGetVariableNode, endpoint.GetNodeId(), &NodeDescriptorRequests::IsType, NodeDescriptorType::GetVariable);

        ScriptCanvas::Endpoint scriptCanvasEndpoint;
        AZ::EntityId scriptCanvasNodeId;

        GraphCanvas::SlotRequestBus::EventResult(userData, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetUserData);
        ScriptCanvas::SlotId scSourceSlotId = (userData && userData->is<ScriptCanvas::SlotId>()) ? *AZStd::any_cast<ScriptCanvas::SlotId>(userData) : ScriptCanvas::SlotId();
        userData = nullptr;

        if (isGetVariableNode)
        {
            GetVariableNodeDescriptorRequestBus::EventResult(scriptCanvasEndpoint, endpoint.GetNodeId(), &GetVariableNodeDescriptorRequests::GetSourceEndpoint, scSourceSlotId);
        }
        else
        {
            GraphCanvas::NodeRequestBus::EventResult(userData, endpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId scSourceNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
            userData = nullptr;

            scriptCanvasEndpoint = ScriptCanvas::Endpoint(scSourceNodeId, scSourceSlotId);
        }

        return scriptCanvasEndpoint;
    }

    GraphCanvas::NodePropertyDisplay* Graph::CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const
    {
        // ScriptCanvas has access to better typing information regarding the slots than is exposed to GraphCanvas.
        // So let ScriptCanvas check the types based on it's own information rather than relying on the information passed back from GraphCanvas.
        ScriptCanvas::Data::Type slotType = ScriptCanvas::Data::Type::Invalid();
        ScriptCanvas::NodeRequestBus::EventResult(slotType, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetSlotDataType, scriptCanvasSlotId);

        bool isSlotValidStorage = false;
        ScriptCanvas::NodeRequestBus::EventResult(isSlotValidStorage, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::IsSlotValidStorage, scriptCanvasSlotId);
        if (isSlotValidStorage)
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
                dataInterface = aznew ScriptCanvasDoubleDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateDoubleNodePropertyDisplay, static_cast<ScriptCanvasDoubleDataInterface*>(dataInterface));
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
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector3::TYPEINFO_Uuid())))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector3, 3>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector2::TYPEINFO_Uuid())))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector2, 2>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Vector4::TYPEINFO_Uuid())))
            {
                dataInterface = aznew ScriptCanvasVectorDataInterface<AZ::Vector4, 4>(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Quaternion::TYPEINFO_Uuid())))
            {
                dataInterface = aznew ScriptCanvasQuaternionDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else if (slotType.IS_A(ScriptCanvas::Data::Type::BehaviorContextObject(AZ::Color::TYPEINFO_Uuid())))
            {
                dataInterface = aznew ScriptCanvasColorDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay, static_cast<GraphCanvas::VectorDataInterface*>(dataInterface));
            }
            else
            {
                dataInterface = aznew ScriptCanvasReadOnlyDataInterface(scriptCanvasNodeId, scriptCanvasSlotId);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(dataDisplay, &GraphCanvas::GraphCanvasRequests::CreateReadOnlyNodePropertyDisplay, static_cast<ScriptCanvasReadOnlyDataInterface*>(dataInterface));
            }

            if (dataDisplay != nullptr)
            {
                return dataDisplay;
            }

            delete dataInterface;
        }

        return nullptr;
    }

    void Graph::OnPreNodeDeleted(const AZ::EntityId& nodeId)
    {
        // If the node is an EBusEvent Node, we don't want to delete the script canvas
        // stuff since that belongs to the wrapper.
        if (EBusEventNodeDescriptorRequestBus::FindFirstHandler(nodeId) == nullptr)
        {
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
        DisconnectConnection(connectionId);
    }

    AZ::u32 Graph::GetNewVariableCounter()
    {
        return ++m_variableCounter;
    }

    void Graph::OnItemMouseMoveComplete(const AZ::EntityId&, const QGraphicsSceneMouseEvent*)
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
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
                AZStd::string eventName;
                EBusEventNodeDescriptorRequestBus::EventResult(eventName, nodeId, &EBusEventNodeDescriptorRequests::GetEventName);

                GraphCanvas::WrappedNodeConfiguration configuration;
                EBusNodeDescriptorRequestBus::EventResult(configuration, m_wrapperNodeDropTarget, &EBusNodeDescriptorRequests::GetEventConfiguration, eventName);

                GraphCanvas::WrapperNodeRequestBus::Event(m_wrapperNodeDropTarget, &GraphCanvas::WrapperNodeRequests::WrapNode, nodeId, configuration);
            }
        }
        else
        {
            // Three maps here.
            // BusTypeWrappers: Keeps track of which busses were created by bus name.
            AZStd::unordered_map< AZStd::string, AZ::EntityId > busTypeWrappers;
            // BusIdWrapper: Keeps track of which busses were created by previous Wrapper name.
            AZStd::unordered_map< AZ::EntityId, AZ::EntityId > busIdWrappers;
            // EventWrapeprs: Keeps track of which event wrappers are being created by Bus name.
            AZStd::unordered_multimap< AZStd::string, AZ::EntityId > eventWrappers;

            // In general, we will only ever use 2 at once(in the case of a drag/drop: busType + eventWrapper)
            // In the case of a paste: busIdWrappers + eventWrappers
            // Logic is merged here just to try to reduce the duplicated logic, and because I can't really
            // tell the difference between the two cases anyway.
            //
            // Idea here is to keep track of groupings so that when we paste, I can create the appropriate number
            // of nodes and groupings within these nodes to create a proper dupilcate. And when we drag and drop
            // I want to merge as many events onto a single node as I can.
            //
            // First step in this process is to sort our pasted nodes into EBus handlers and EBus events.
            for (const AZ::EntityId& nodeId : m_lastGraphCanvasCreationGroup)
            {
                NodeDescriptorType descriptorType = NodeDescriptorType::Unknown;
                NodeDescriptorRequestBus::EventResult(descriptorType, nodeId, &NodeDescriptorRequests::GetType);

                switch (descriptorType)
                {
                    case NodeDescriptorType::EBusHandler:
                    {
                        busIdWrappers[nodeId] = nodeId;

                        AZStd::string busName;
                        EBusNodeDescriptorRequestBus::EventResult(busName, nodeId, &EBusNodeDescriptorRequests::GetBusName);

                        if (!busName.empty())
                        {
                            auto mapIter = busTypeWrappers.find(busName);

                            if (mapIter == busTypeWrappers.end())
                            {
                                busTypeWrappers[busName] = nodeId;
                            }
                        }
                        break;
                    }
                    case NodeDescriptorType::EBusHandlerEvent:
                    {
                        bool isWrapped = false;
                        EBusEventNodeDescriptorRequestBus::EventResult(isWrapped, nodeId, &EBusEventNodeDescriptorRequests::IsWrapped);

                        if (!isWrapped)
                        {
                            AZStd::string busName;
                            EBusEventNodeDescriptorRequestBus::EventResult(busName, nodeId, &EBusEventNodeDescriptorRequests::GetBusName);

                            if (!busName.empty())
                            {
                                eventWrappers.emplace(busName, nodeId);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            // Second step is to go through, and determine which usage case is valid so we know how to filter down our events.
            // If we can't find a wrapper, or we can't create a handler for the wrapper. We need to delete it.
            AZStd::unordered_set<AZ::EntityId> invalidNodes;

            for (const auto& mapPair : eventWrappers)
            {
                AZ::EntityId wrapperNodeId;

                // Look up in our previous group mapping to see if it belonged to a node previously
                // (i.e. copy + pasted node).
                AZ::EntityId previousGroupWrapperNodeId;

                auto mapIter = m_ebusMethodGroupings.find(mapPair.second);

                if (mapIter != m_ebusMethodGroupings.end())
                {
                    previousGroupWrapperNodeId = mapIter->second;

                    auto busIter = busIdWrappers.find(previousGroupWrapperNodeId);

                    if (busIter != busIdWrappers.end())
                    {
                        wrapperNodeId = busIter->second;
                    }
                }

                // We may have already found our target node.
                // If we have, bypass the creation step.
                if (!wrapperNodeId.IsValid())
                {
                    // If we haven't check if we match a type, or if our previous group wrapper node is is valid.
                    // If we had a previous group. I need to create a ebus wrapper for that group.
                    // If we didn't have a previous group, I want to just use the Bus name to find an appropriate grouping.
                    auto busIter = busTypeWrappers.find(mapPair.first);
                    if (busIter == busTypeWrappers.end() || previousGroupWrapperNodeId.IsValid())
                    {
                        CreateEBusHandlerMimeEvent createEbusMimeEvent(mapPair.first);

                        AZ::Vector2 position;
                        GraphCanvas::GeometryRequestBus::EventResult(position, mapPair.second, &GraphCanvas::GeometryRequests::GetPosition);

                        if (createEbusMimeEvent.ExecuteEvent(position, position, GetEntityId()))
                        {
                            const NodeIdPair& nodePair = createEbusMimeEvent.GetCreatedPair();

                            if (!previousGroupWrapperNodeId.IsValid())
                            {
                                busTypeWrappers.emplace(mapPair.first, nodePair.m_graphCanvasId);
                            }
                            else
                            {
                                busIdWrappers.emplace(previousGroupWrapperNodeId, nodePair.m_graphCanvasId);
                            }

                            wrapperNodeId = nodePair.m_graphCanvasId;

                        }
                        else
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to instantiate an EBus wrapper node for EBus(%s)", mapPair.first.c_str());
                            invalidNodes.insert(mapPair.second);
                            continue;
                        }
                    }
                    else
                    {
                        wrapperNodeId = busIter->second;
                    }
                }

                AZStd::string eventName;
                EBusEventNodeDescriptorRequestBus::EventResult(eventName, mapPair.second, &EBusEventNodeDescriptorRequests::GetEventName);

                GraphCanvas::WrappedNodeConfiguration configuration;
                EBusNodeDescriptorRequestBus::EventResult(configuration, wrapperNodeId, &EBusNodeDescriptorRequests::GetEventConfiguration, eventName);

                GraphCanvas::WrapperNodeRequestBus::Event(wrapperNodeId, &GraphCanvas::WrapperNodeRequests::WrapNode, mapPair.second, configuration);
            }

            GraphCanvas::SceneRequestBus::Event(GetEntityId(), &GraphCanvas::SceneRequests::Delete, invalidNodes);
        }

        m_ebusMethodGroupings.clear();
        m_lastGraphCanvasCreationGroup.clear();
        m_wrapperNodeDropTarget.SetInvalid();

        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, GetEntityId());
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

    bool Graph::ShouldAcceptDrop(const AZ::EntityId& wrapperNode, const QMimeData* mimeData) const
    {
        if (!mimeData->hasFormat(Widget::NodePalette::GetMimeType()))
        {
            return false;
        }

        // Deep mime inspection
        QByteArray arrayData = mimeData->data(Widget::NodePalette::GetMimeType());

        GraphCanvas::GraphCanvasMimeContainer mimeContainer;

        if (!mimeContainer.FromBuffer(arrayData.constData(), arrayData.size()) || mimeContainer.m_mimeEvents.empty())
        {
            return false;
        }

        AZStd::string busName;
        EBusNodeDescriptorRequestBus::EventResult(busName, wrapperNode, &EBusNodeDescriptorRequests::GetBusName);

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
                EBusNodeDescriptorRequestBus::EventResult(containsEvent, wrapperNode, &EBusNodeDescriptorRequests::ContainsEvent, createEbusMethodEvent->GetEventName());

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

    void Graph::AssignVariableValue(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& targetEndpoint)
    {
        ScriptCanvas::Endpoint variableEndpoint;
        VariableNodeDescriptorRequestBus::EventResult(variableEndpoint, variableId, &VariableNodeDescriptorRequests::GetReadEndpoint);

        ScriptCanvas::Endpoint scTargetEndpoint = ConvertToScriptCanvasEndpoint(targetEndpoint);

        if (ConnectByEndpoint(variableEndpoint, scTargetEndpoint))
        {
            bool foundConnection = false;
            AZ::Entity* connectionEntity = nullptr;
            if (FindConnection(connectionEntity, variableEndpoint, scTargetEndpoint))
            {
                VariableNodeDescriptorRequestBus::Event(variableId, &VariableNodeDescriptorRequests::AddConnection, targetEndpoint, connectionEntity->GetId());
            }
        }
    }

    void Graph::UnassignVariableValue(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& endpoint)
    {
        AZ::EntityId connectionId;
        VariableNodeDescriptorRequestBus::EventResult(connectionId, variableId, &VariableNodeDescriptorRequests::FindConnection, endpoint);

        if (connectionId.IsValid())
        {
            if (DisconnectById(connectionId))
            {
                VariableNodeDescriptorRequestBus::Event(variableId, &VariableNodeDescriptorRequests::RemoveConnection, endpoint);
            }
        }
    }

    AZStd::string Graph::GetTypeString(const AZ::Uuid& typeId)
    {
        return TranslationHelper::GetSafeTypeName(ScriptCanvas::Data::FromAZType(typeId));
    }

} // namespace ScriptCanvasEditor
