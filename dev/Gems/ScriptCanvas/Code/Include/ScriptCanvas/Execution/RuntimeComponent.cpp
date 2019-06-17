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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/Core/ErrorHandler.h>
#include <ScriptCanvas/Libraries/Core/Start.h>

namespace ScriptCanvas
{
    GraphInfo CreateGraphInfo(AZ::EntityId uniqueExecutionId, const GraphIdentifier& graphIdentifier)
    {
        AZ::EntityId runtimeEntity{};
        RuntimeRequestBus::EventResult(runtimeEntity, uniqueExecutionId, &RuntimeRequests::GetRuntimeEntityId);
        return GraphInfo(runtimeEntity, graphIdentifier);
    }

    DatumValue CreateDatumValue(AZ::EntityId uniqueExecutionId, const VariableDatumBase& variable)
    {
        VariableId assetVariableId{};
        RuntimeRequestBus::EventResult(assetVariableId, uniqueExecutionId, &RuntimeRequests::FindAssetVariableIdByRuntimeVariableId, variable.GetId());
        return DatumValue::Create(VariableDatumBase(variable.GetData(), assetVariableId));
    }

    bool RuntimeComponent::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            classElement.RemoveElementByName(AZ_CRC("m_uniqueId", 0x52157a7a));
        }

        return true;
    }

    RuntimeComponent::RuntimeComponent(AZ::EntityId uniqueId)
        : RuntimeComponent({}, uniqueId)
    {}

    RuntimeComponent::RuntimeComponent(AZ::Data::Asset<RuntimeAsset> runtimeAsset, AZ::EntityId uniqueId)
        : m_runtimeAsset(runtimeAsset)
        , m_uniqueId(uniqueId)
    {}

    RuntimeComponent::~RuntimeComponent()
    {
        const bool deleteData{ true };
        m_runtimeData.m_graphData.Clear(deleteData);
    }

    void RuntimeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeComponent, AZ::Component>()
                ->Version(3, &RuntimeComponent::VersionConverter)
                ->Field("m_runtimeAsset", &RuntimeComponent::m_runtimeAsset)
                ->Field("m_variableOverrides", &RuntimeComponent::m_variableOverrides)
                ->Field("m_variableEntityIdMap", &RuntimeComponent::m_variableEntityIdMap)
                ;
        }
    }

    void RuntimeComponent::Init()
    {
    }

    void RuntimeComponent::Activate()
    {
        RuntimeRequestBus::Handler::BusConnect(GetUniqueId());

        if (m_runtimeAsset.GetId().IsValid())
        {
            auto& assetManager = AZ::Data::AssetManager::Instance();
            m_runtimeAsset = assetManager.GetAsset(m_runtimeAsset.GetId(), azrtti_typeid<RuntimeAsset>(), true, nullptr, false);
            AZ::Data::AssetBus::Handler::BusConnect(m_runtimeAsset.GetId());
        }
    }

    void RuntimeComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_executionContext.Deactivate();
        DeactivateGraph();
        RuntimeRequestBus::Handler::BusDisconnect();
    }

    void RuntimeComponent::ActivateNodes()
    {
        m_runtimeData.m_graphData.BuildEndpointMap();
        for (auto& nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            if (nodeEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                nodeEntity->Init();
            }
            if (nodeEntity->GetState() == AZ::Entity::ES_INIT)
            {
                nodeEntity->Activate();
            }
        }

        for (auto& connectionEntity : m_runtimeData.m_graphData.m_connections)
        {
            if (connectionEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                connectionEntity->Init();
            }

            if (connectionEntity->GetState() == AZ::Entity::ES_INIT)
            {
                connectionEntity->Activate();
            }
        }
    }

    void RuntimeComponent::ActivateGraph()
    {
        {
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::ActivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());

            if (!m_executionContext.Activate(GetUniqueId()))
            {
                return;
            }

            auto& runtimeAssetData = m_runtimeAsset.Get()->GetData();
            // If there are no nodes, there's nothing to do, deactivate the graph's entity.
            if (runtimeAssetData.m_graphData.m_nodes.empty())
            {
                DeactivateGraph();
                return;
            }
        }

        CreateAssetInstance();

        {
            AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::ActivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());

            bool entryPointFound = false;
            for (auto& nodeEntity : m_runtimeData.m_graphData.m_nodes)
            {
                auto node = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity);
                if (!node)
                {
                    AZ_Error("Script Canvas", false, "Entity %s is missing node component", nodeEntity->GetName().data());
                    continue;
                }

                entryPointFound = entryPointFound || node->IsEntryPoint();

                if (auto startNode = azrtti_cast<Nodes::Core::Start*>(node))
                {
                    m_entryNodes.insert(startNode);
                }
                else if (auto errorHandlerNode = azrtti_cast<Nodes::Core::ErrorHandler*>(node))
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
            }

            // If we still can't find a start node, there's nothing to do.
            if (!entryPointFound)
            {
                AZ_Warning("Script Canvas", false, "Graph does not have any entry point nodes, it will not run.");
                DeactivateGraph();
                return;
            }

            ActivateNodes();
            
            SC_EXECUTION_TRACE_GRAPH_ACTIVATED(CreateActivationInfo());
            
            if (RuntimeRequestBus::Handler::BusIsConnected())
            {
                m_executionContext.Execute();
            }

            AZ::EntityBus::Handler::BusConnect(GetEntityId());
        }
    }

    void RuntimeComponent::DeactivateGraph()
    {
        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::DeactivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        VariableRequestBus::MultiHandler::BusDisconnect();

        for (auto& nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                if (nodeEntity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    nodeEntity->Deactivate();
                }
            }
        }

        for (auto& connectionEntity : m_runtimeData.m_graphData.m_connections)
        {
            if (connectionEntity)
            {
                if (connectionEntity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    connectionEntity->Deactivate();
                }
            }
        }

        // Defer graph deletion to next frame as an executing graph in a dynamic slice can be deleted from a node
        auto oldRuntimeData(AZStd::move(m_runtimeData));
        // Use AZ::SystemTickBus, not AZ::TickBus otherwise the deferred delete will not happen if timed just before a level unload
        AZ::SystemTickBus::QueueFunction([oldRuntimeData]() mutable
        {
            oldRuntimeData.m_graphData.Clear(true);
            oldRuntimeData.m_variableData.Clear();
        });

        SC_EXECUTION_TRACE_GRAPH_DEACTIVATED(CreateActivationInfo());
    }

    void RuntimeComponent::CreateAssetInstance()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        m_runtimeIdToAssetId.clear();
        m_assetIdToRuntimeId.clear();

        m_runtimeData.m_graphData.Clear(true);
        m_runtimeData.m_variableData.Clear();

        auto& assetRuntimeData = m_runtimeAsset.Get()->GetData();
        // Clone GraphData
        serializeContext->CloneObjectInplace(m_runtimeData.m_graphData, &assetRuntimeData.m_graphData);

        for (AZ::Entity* nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            AZ::EntityId runtimeNodeId = AZ::Entity::MakeId();
            m_assetIdToRuntimeId.emplace(nodeEntity->GetId(), runtimeNodeId);
            m_runtimeIdToAssetId.emplace(runtimeNodeId, nodeEntity->GetId());
            if (auto* node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity))
            {
                // Set all nodes Graph unique Id references directly to the Runtime Unique Id
                node->SetGraphUniqueId(GetUniqueId());
            }
        }

        for (AZ::Entity* connectionEntity : m_runtimeData.m_graphData.m_connections)
        {
            AZ::EntityId runtimeConnectionId = AZ::Entity::MakeId();
            m_assetIdToRuntimeId.emplace(connectionEntity->GetId(), runtimeConnectionId);
            m_runtimeIdToAssetId.emplace(runtimeConnectionId, connectionEntity->GetId());
        }

        m_runtimeData.m_graphData.LoadDependentAssets();

        // Clone Variable Data
        serializeContext->CloneObjectInplace(m_runtimeData.m_variableData, &assetRuntimeData.m_variableData);

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> loadedGameEntityIdMap;

        // Looks up the EntityContext loaded game entity map
        AzFramework::EntityContextId owningContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(owningContextId, GetEntityId(), &AzFramework::EntityIdContextQueries::GetOwningContextId);
        if (!owningContextId.IsNull())
        {
            AzFramework::EntityContextRequestBus::EventResult(loadedGameEntityIdMap, owningContextId, &AzFramework::EntityContextRequests::GetLoadedEntityIdMap);
        }

            // Added an entity mapping of the SelfReferenceId Entity to this component's EntityId
            // Also add a mapping of the InvalidUniqueRuntimeId to the graph uniqueId
            // as well as an identity mapping for the EntityId and the UniqueId
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> assetToRuntimeEntityIdMap
        {
            { ScriptCanvas::SelfReferenceId, GetEntityId() },
            { ScriptCanvas::InvalidUniqueRuntimeId, GetUniqueId() },
            { GetEntityId(), GetEntityId() },
            { GetUniqueId(), GetUniqueId() },
            { AZ::EntityId(), AZ::EntityId() } // Adds an Identity mapping for InvalidEntityId -> InvalidEntityId since it is valid to set an EntityId field to InvalidEntityId 
        };
        assetToRuntimeEntityIdMap.insert(m_assetIdToRuntimeId.begin(), m_assetIdToRuntimeId.end());
            assetToRuntimeEntityIdMap.insert(loadedGameEntityIdMap.begin(), loadedGameEntityIdMap.end());

            // Lambda function remaps any known entities to their correct id otherwise it remaps unknown entity Ids to invalid
        auto assetToRuntimeEnd = assetToRuntimeEntityIdMap.end();
        auto worldEntityRemapper = [&assetToRuntimeEntityIdMap, &assetToRuntimeEnd, this](const AZ::EntityId& contextEntityId) -> AZ::EntityId
            {
                auto foundEntityIdIt = assetToRuntimeEntityIdMap.find(contextEntityId);
                if (foundEntityIdIt != assetToRuntimeEnd)
                {
                    return foundEntityIdIt->second;
                }
                else
                {
                    AZ_Warning("Script Canvas", false, "(%s) Entity Id %s is not part of the current entity context loaded entities and not an internal node/connection entity. It will be mapped to InvalidEntityId", GetAssetName().c_str(), contextEntityId.ToString().data());
                    return AZ::EntityId(AZ::EntityId::InvalidEntityId);
                }
            };

        AZ::IdUtils::Remapper<AZ::EntityId>::RemapIdsAndIdRefs(&m_runtimeData, worldEntityRemapper, serializeContext);

        // Apply Variable Overrides and Remap VariableIds
        ApplyVariableOverrides();
    }

    ActivationInfo RuntimeComponent::CreateActivationInfo() const
    {
        return ActivationInfo(CreateGraphInfo(GetUniqueId(), GetGraphIdentifier()), CreateVariableValues());
    }

    ActivationInfo RuntimeComponent::CreateDeactivationInfo() const
    {
        return ActivationInfo(CreateGraphInfo(GetUniqueId(), GetGraphIdentifier()));
    }
    
    VariableValues RuntimeComponent::CreateVariableValues() const
    {
        VariableValues variableValues;

        auto& runtimeVariables = m_runtimeData.m_variableData.GetVariables();
        for (const auto& variablePair : runtimeVariables)
        {
            variableValues.emplace(variablePair.first, AZStd::make_pair(variablePair.second.GetVariableName(), CreateDatumValue(GetUniqueId(), variablePair.second.m_varDatum)));
        }

        return variableValues;
    }

    void RuntimeComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (auto runtimeAsset = asset.GetAs<RuntimeAsset>())
        {
            m_runtimeAsset = runtimeAsset;
            ActivateGraph();
        }
    }

    void RuntimeComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        for (auto startNode : m_entryNodes)
        {
            m_executionContext.AddToExecutionStack(*startNode, SlotId());
        }

        m_entryNodes.clear();

        if (RuntimeRequestBus::Handler::BusIsConnected())
        {
            m_executionContext.Execute();
        }

        AZ::EntityBus::Handler::BusDisconnect();
    }

    void RuntimeComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // TODO: deleting this potentially helps with a rare crash when "live editing"
        DeactivateGraph();
        OnAssetReady(asset);
    }

    void RuntimeComponent::SetRuntimeAsset(const AZ::Data::Asset<RuntimeAsset>& runtimeAsset)
    {   
        AZ::Entity* entity = GetEntity();
        if (entity && entity->GetState() == AZ::Entity::ES_ACTIVE)
        {
            DeactivateGraph();
            AZ::Data::AssetBus::Handler::BusDisconnect();
            if (runtimeAsset.GetId().IsValid())
            {
                auto& assetManager = AZ::Data::AssetManager::Instance();
                m_runtimeAsset = assetManager.GetAsset(runtimeAsset.GetId(), azrtti_typeid<RuntimeAsset>(), true, nullptr, false);
                AZ::Data::AssetBus::Handler::BusConnect(m_runtimeAsset.GetId());
            }
        }
        else
        {
            m_runtimeAsset = runtimeAsset;
        }
    }

    AZStd::vector<RuntimeComponent*> RuntimeComponent::FindAllByEditorAssetId(AZ::Data::AssetId editorScriptCanvasAssetId)
    {
        AZStd::vector<RuntimeComponent*> result;
        AZ::Data::AssetId runtimeAssetId(editorScriptCanvasAssetId.m_guid, AZ_CRC("RuntimeData", 0x163310ae));
        AZ::Data::AssetBus::EnumerateHandlersId(runtimeAssetId,
            [&result](AZ::Data::AssetEvents* assetEvent) -> bool
            {
                if (auto runtimeComponent = azrtti_cast<ScriptCanvas::RuntimeComponent*>(assetEvent))
                {
                    result.push_back(runtimeComponent);
                }

                // Continue enumeration.
                return true;
            }
        );

        return result;
    }
    
    VariableId RuntimeComponent::FindAssetVariableIdByRuntimeVariableId(VariableId runtimeId) const
    {
        auto iter = m_runtimeToAssetVariableMap.find(runtimeId);
        if (iter != m_runtimeToAssetVariableMap.end())
        {
            return iter->second;
        }

        return VariableId{};
    }

    VariableId RuntimeComponent::FindRuntimeVariableIdByAssetVariableId(VariableId assetId) const
    {
        auto iter = m_assetToRuntimeVariableMap.find(assetId);
        if (iter != m_assetToRuntimeVariableMap.end())
        {
            return iter->second;
        }

        return VariableId{};
    }

    AZ::EntityId RuntimeComponent::FindAssetNodeIdByRuntimeNodeId(AZ::EntityId runtimeNodeId) const
    {
        auto iter = m_runtimeIdToAssetId.find(runtimeNodeId);

        if (iter != m_runtimeIdToAssetId.end())
        {
            return iter->second;
        }

        return AZ::EntityId{};
    }
    
    Node* RuntimeComponent::FindNode(AZ::EntityId nodeId) const
    {
        auto entry = AZStd::find_if(m_runtimeData.m_graphData.m_nodes.begin(), m_runtimeData.m_graphData.m_nodes.end(), [nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
        return entry != m_runtimeData.m_graphData.m_nodes.end() ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(*entry) : nullptr;
    }

    AZ::EntityId RuntimeComponent::FindRuntimeNodeIdByAssetNodeId(AZ::EntityId editorNodeId) const
    {
        auto iter = m_assetIdToRuntimeId.find(editorNodeId);

        if (iter != m_assetIdToRuntimeId.end())
        {
            return iter->second;
        }

        return AZ::EntityId{};
    }

    GraphIdentifier RuntimeComponent::GetGraphIdentifier() const
    {
        GraphIdentifier identifier;
        identifier.m_assetId = AZ::Data::AssetId(GetAssetId().m_guid, 0);

        // For now we can gloss over multiple instances of the same graph running on the same entity.
        // Once this becomes a supported case, we can toggle back on the component Id, or whatever it is we used
        // to identify it.
        //identifier.m_componentId = GetId();
        identifier.m_componentId = 0;

        return identifier;
    }

    AZStd::string RuntimeComponent::GetAssetName() const
    {
        AZStd::string graphName;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(graphName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_runtimeAsset.GetId());
        return graphName;
    }
    
    AZStd::vector<AZ::EntityId> RuntimeComponent::GetNodes() const
    {
        AZStd::vector<AZ::EntityId> entityIds;
        for (auto& nodeRef : m_runtimeData.m_graphData.m_nodes)
        {
            entityIds.push_back(nodeRef->GetId());
        }

        return entityIds;
    }

    const AZStd::vector<AZ::EntityId> RuntimeComponent::GetNodesConst() const
    {
        return GetNodes();
    }

    AZStd::vector<AZ::EntityId> RuntimeComponent::GetConnections() const
    {
        AZStd::vector<AZ::EntityId> entityIds;
        for (auto& connectionRef : m_runtimeData.m_graphData.m_connections)
        {
            entityIds.push_back(connectionRef->GetId());
        }

        return entityIds;
    }

    AZStd::vector<Endpoint> RuntimeComponent::GetConnectedEndpoints(const Endpoint& endpoint) const
    {
        AZStd::vector<Endpoint> connectedEndpoints;
        auto otherEndpointsRange = m_runtimeData.m_graphData.m_endpointMap.equal_range(endpoint);
        for (auto otherIt = otherEndpointsRange.first; otherIt != otherEndpointsRange.second; ++otherIt)
        {
            connectedEndpoints.push_back(otherIt->second);
        }
        return connectedEndpoints;
    }

    bool RuntimeComponent::IsEndpointConnected(const Endpoint& endpoint) const
    {
        size_t connectionCount = m_runtimeData.m_graphData.m_endpointMap.count(endpoint);
        return connectionCount > 0;
    }

    GraphData* RuntimeComponent::GetGraphData()
    {
        return &m_runtimeData.m_graphData;
    }

    VariableData* RuntimeComponent::GetVariableData()
    {
        return &m_runtimeData.m_variableData;
    }

    void RuntimeComponent::SetVariableOverrides(const VariableData& overrideData)
    {
        m_variableOverrides = overrideData;
    }

    void RuntimeComponent::ApplyVariableOverrides()
    {
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        m_assetToRuntimeVariableMap.clear();
        m_runtimeToAssetVariableMap.clear();

        // Apply Variable Overrides - Overrides were already remapped as part of the EditorToRuntimeVariableIdMap
        // stored on the SystemComponent
        auto& runtimeVariables = m_runtimeData.m_variableData.GetVariables();
        for (const auto& varIdInfoPair : m_variableOverrides.GetVariables())
        {
            runtimeVariables[varIdInfoPair.first] = varIdInfoPair.second;
        }

        AZStd::vector< VariableId > originalIds;
        originalIds.reserve(runtimeVariables.size());

        for (auto& mapPair : runtimeVariables)
        {
            originalIds.emplace_back(mapPair.first);
        }

        for (const VariableId& originalId : originalIds)
        {
            auto mapIter = runtimeVariables.find(originalId);
            mapIter->second.m_varDatum.GenerateNewId();
            VariableId remappedVariableId = mapIter->second.m_varDatum.GetId();

            VariableNameValuePair variableNameValue = AZStd::move(mapIter->second);
            runtimeVariables.erase(mapIter);

            // Move the VariableDatum to be keyed with the remapped VaraibleId within the VariableData map
            m_assetToRuntimeVariableMap[originalId] = remappedVariableId;
            VariableRequestBus::MultiHandler::BusConnect(remappedVariableId);
            runtimeVariables[remappedVariableId] = AZStd::move(variableNameValue);
        }

        VariableIdMap variableMap(m_assetToRuntimeVariableMap.begin(), m_assetToRuntimeVariableMap.end());
        for (AZ::Entity* nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            if (ScriptCanvas::Node* nodeComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity))
            {
                for (auto& datum : nodeComponent->m_varDatums)
                {
                    // VariableDatumBase instances stored on a Node are internal to that node and does
                    // not need to be remapped. This adds an entry to a set to prevent remapping from
                    // occurring in that case.
                    variableMap.emplace(datum.GetId(), datum.GetId());
                }
            }
        }

        auto variableIdRemapper = [this, &variableMap](const VariableId& variableId) -> VariableId
        {
            auto foundVariableIdIt = variableMap.find(variableId);
            if (foundVariableIdIt != variableMap.end())
            {
                return foundVariableIdIt->second;
            }
            else
            {
                return VariableId{};
            }
        };

        // Remap both the Variable overrides Variable Ids and the Runtime Data Variable Ids
        AZ::IdUtils::Remapper<VariableId>::RemapIdsAndIdRefs(&m_runtimeData.m_graphData.m_nodes, variableIdRemapper, serializeContext);
        
        m_runtimeToAssetVariableMap.reserve(m_assetToRuntimeVariableMap.size());
        for (auto& assetToRuntime : m_assetToRuntimeVariableMap)
        {
            m_runtimeToAssetVariableMap.emplace(assetToRuntime.second, assetToRuntime.first);
        }
    }

    void RuntimeComponent::SetVariableEntityIdMap(const AZStd::unordered_map<AZ::u64, AZ::EntityId> variableEntityIdMap)
    {
        m_variableEntityIdMap = variableEntityIdMap;
    }

    const AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::VariableNameValuePair>* RuntimeComponent::GetVariables() const
    {
        return &m_runtimeData.m_variableData.GetVariables();
    }

    VariableDatum* RuntimeComponent::FindVariable(AZStd::string_view propName)
    {
        return m_runtimeData.m_variableData.FindVariable(propName);
    }

    VariableNameValuePair* RuntimeComponent::FindVariableById(const VariableId& variableId)
    {
        auto& variableMap = m_runtimeData.m_variableData.GetVariables();
        auto foundIt = variableMap.find(variableId);
        return foundIt != variableMap.end() ? &foundIt->second : nullptr;
    }

    Data::Type RuntimeComponent::GetVariableType(const VariableId& variableId) const
    {
        auto& variableMap = m_runtimeData.m_variableData.GetVariables();
        auto foundIt = variableMap.find(variableId);
        return foundIt != variableMap.end() ? foundIt->second.m_varDatum.GetData().GetType() : Data::Type::Invalid();
    }

    AZStd::string_view RuntimeComponent::GetVariableName(const VariableId& variableId) const
    {
        auto& variableMap = m_runtimeData.m_variableData.GetVariables();
        auto foundIt = variableMap.find(variableId);
        return foundIt != variableMap.end() ? AZStd::string_view(foundIt->second.GetVariableName()) : "";
    }

    VariableDatum* RuntimeComponent::GetVariableDatum()
    {
        VariableNameValuePair* variableNameValuePair{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            variableNameValuePair = m_runtimeData.m_variableData.FindVariable(*variableId);
        }

        return variableNameValuePair ? &variableNameValuePair->m_varDatum : nullptr;
    }

    Data::Type RuntimeComponent::GetType() const
    {
        const VariableNameValuePair* variableNameValuePair{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            variableNameValuePair = m_runtimeData.m_variableData.FindVariable(*variableId);
        }

        return variableNameValuePair ? variableNameValuePair->m_varDatum.GetData().GetType() : Data::Type::Invalid();
    }

    AZStd::string_view RuntimeComponent::GetName() const
    {
        const VariableNameValuePair* variableNameValuePair{};
        if (auto variableId = VariableRequestBus::GetCurrentBusId())
        {
            variableNameValuePair = m_runtimeData.m_variableData.FindVariable(*variableId);
        }

        return variableNameValuePair ? AZStd::string_view(variableNameValuePair->GetVariableName()) : "";
    }

    AZ::Outcome<void, AZStd::string> RuntimeComponent::RenameVariable(AZStd::string_view newVarName)
    {
        const VariableId* variableId = VariableRequestBus::GetCurrentBusId();
        if (!variableId)
        {
            return AZ::Failure(AZStd::string::format("No variable id was found, cannot rename variable to %s", newVarName.data()));
        }

        auto varDatumPair = FindVariableById(*variableId);

        if (!varDatumPair)
        {
            return AZ::Failure(AZStd::string::format("Unable to find variable with Id %s on Entity %s. Cannot rename",
                variableId->ToString().data(), GetEntityId().ToString().data()));
        }

        VariableDatum* newVariableDatum = FindVariable(newVarName);
        if (newVariableDatum && newVariableDatum->GetId() != *variableId)
        {
            return AZ::Failure(AZStd::string::format("A variable with name %s already exist on Entity %s. Cannot rename",
                newVarName.data(), GetEntityId().ToString().data()));
        }

        if (!m_runtimeData.m_variableData.RenameVariable(*variableId, newVarName))
        {
            return AZ::Failure(AZStd::string::format("Unable to rename variable with id %s to %s.",
                variableId->ToString().data(), newVarName.data()));
        }

        VariableNotificationBus::Event(*variableId, &VariableNotifications::OnVariableRenamed, newVarName);

        return AZ::Success();
    }
}
