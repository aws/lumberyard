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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/Core/ErrorHandler.h>
#include <ScriptCanvas/Libraries/Core/Start.h>

namespace ScriptCanvas
{
    RuntimeComponent::RuntimeComponent(AZ::EntityId uniqueId)
        : RuntimeComponent({}, uniqueId)
    {
    }

    RuntimeComponent::RuntimeComponent(AZ::Data::Asset<RuntimeAsset> runtimeAsset, AZ::EntityId uniqueId)
        : m_runtimeAsset(runtimeAsset)
        , m_uniqueId(uniqueId)
    {
    }

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
                ->Version(1)
                ->Field("m_uniqueId", &RuntimeComponent::m_uniqueId)
                ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Entity::MakeId)
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
        RuntimeRequestBus::Handler::BusDisconnect();
        m_executionContext.Deactivate();
        DeactivateGraph();
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
        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::ActivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
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

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::ActivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
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
        AZ::TickBus::QueueFunction([oldRuntimeData]() mutable
        {
            oldRuntimeData.m_graphData.Clear(true);
            oldRuntimeData.m_variableData.Clear();
        });
    }

    void RuntimeComponent::CreateAssetInstance()
    {
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::CreateAssetInstance [Clone Graph Data] (%s)",  m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
            m_runtimeData.m_graphData.Clear(true);
            m_runtimeData.m_variableData.Clear();

            auto& assetRuntimeData = m_runtimeAsset.Get()->GetData();
            // Clone GraphData
            serializeContext->CloneObjectInplace(m_runtimeData.m_graphData, &assetRuntimeData.m_graphData);
            // Set all nodes unique Id references to Invalid, they will be remapped correctly to the runtime component
            // unique id later in this method
            for (AZ::Entity* nodeEntity : m_runtimeData.m_graphData.m_nodes)
            {
                if (auto* node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity))
                {
                    node->SetGraphUniqueId(InvalidUniqueRuntimeId);
                }
            }
        }

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::CreateAssetInstance [Copy Variable Data] (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
            // Copy Variable Data
            m_runtimeData.m_variableData = m_runtimeAsset.Get()->GetData().m_variableData;
        }

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> assetToRuntimeInternalMap;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> assetToRuntimeEntityIdMap{
            { ScriptCanvas::SelfReferenceId, GetEntityId() },
            { ScriptCanvas::InvalidUniqueRuntimeId, GetUniqueId() },
            { GetEntityId(), GetEntityId() },
            { GetUniqueId(), GetUniqueId() },
            { AZ::EntityId(), AZ::EntityId() } // Adds an Identity mapping for InvalidEntityId -> InvalidEntityId since it is valid to set an EntityId field to InvalidEntityId 
        };

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::CreateAssetInstance [Internal Graph Entity Mapper] (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
            const bool replaceIdOnEntity{ true };

            // Generates new EntityIds for all GraphData's internal node and connection entities, but does not apply the generated ids until later
            auto internalGraphEntityIdMapper = [&assetToRuntimeInternalMap](const AZ::EntityId& entityId, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator& idGenerator)
            {
                // Finds the Node/Connection AZ::Entity::m_id members and generates a new entity Id which is added to the map,
                // but not applied until list of game entities are gathered as well
                assetToRuntimeInternalMap.emplace(entityId, idGenerator());
                return entityId;
            };

            AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&m_runtimeData.m_graphData, internalGraphEntityIdMapper, serializeContext, replaceIdOnEntity);
        }

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::CreateAssetInstance [Editor to Runtime Asset Mapper] (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
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
            assetToRuntimeEntityIdMap.insert(assetToRuntimeInternalMap.begin(), assetToRuntimeInternalMap.end());
            assetToRuntimeEntityIdMap.insert(loadedGameEntityIdMap.begin(), loadedGameEntityIdMap.end());
            // Add only entities within the m_assetToRuntimeEntityIdMap that has been mapped to a new value
            for (const auto& assetToRuntimeEntityIdPair : m_variableEntityIdMap)
            {
                AZ::EntityId persistentAssetEntityId(assetToRuntimeEntityIdPair.first);
                AZ::EntityId remappedAssetEntityId(assetToRuntimeEntityIdPair.second);
                if (persistentAssetEntityId != remappedAssetEntityId)
                {
                    assetToRuntimeEntityIdMap.emplace(persistentAssetEntityId, remappedAssetEntityId);
                }
            }

            // Lambda function remaps any known entities to their correct id otherwise it remaps unknown entity Ids to invalid
            auto worldEntityRemapper = [&assetToRuntimeEntityIdMap](const AZ::EntityId& contextEntityId, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator&) -> AZ::EntityId
            {
                auto foundEntityIdIt = assetToRuntimeEntityIdMap.find(contextEntityId);
                if (foundEntityIdIt != assetToRuntimeEntityIdMap.end())
                {
                    return foundEntityIdIt->second;
                }
                else
                {
                    AZ_Warning("Script Canvas", false, "Entity Id %s is not part of the current entity context loaded entities and not an internal node/connection entity. It will be mapped to InvalidEntityId", contextEntityId.ToString().data());
                    return AZ::EntityId(AZ::EntityId::InvalidEntityId);
                }
            };

            AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(&m_runtimeData, worldEntityRemapper, serializeContext);
        }

        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::CreateAssetInstance [Variable Remapping] (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        {
            // Apply Variable Overrides and Remap VariableIds
            ApplyVariableOverrides();
        }
    }

    void RuntimeComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        auto runtimeAsset = asset.GetAs<RuntimeAsset>();
        if (runtimeAsset && runtimeAsset->GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
        {
            m_runtimeAsset = runtimeAsset;
            ActivateGraph();
        }
    }

    void RuntimeComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        DeactivateGraph();
        OnAssetReady(asset);
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

    Node* RuntimeComponent::FindNode(AZ::EntityId nodeId) const
    {
        auto entry = AZStd::find_if(m_runtimeData.m_graphData.m_nodes.begin(), m_runtimeData.m_graphData.m_nodes.end(), [nodeId](const AZ::Entity* node) { return node->GetId() == nodeId; });
        return entry != m_runtimeData.m_graphData.m_nodes.end() ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(*entry) : nullptr;
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
        m_editorToRuntimeVariableMap.clear();

        // Remap both the Variable overrides Variable Ids and the Runtime Data Variable Ids
        AZ::IdUtils::Remapper<VariableId>::GenerateNewIdsAndFixRefs(&m_variableOverrides, m_editorToRuntimeVariableMap);
        AZ::IdUtils::Remapper<VariableId>::GenerateNewIdsAndFixRefs(&m_runtimeData, m_editorToRuntimeVariableMap, serializeContext);

        // Apply Variable Overrides - Overrides were already remapped as part of the EditorToRuntimeVariableIdMap
        // stored on the SystemComponent
        auto& runtimeVariables = m_runtimeData.m_variableData.GetVariables();
        for (const auto& varIdInfoPair : m_variableOverrides.GetVariables())
        {
            runtimeVariables[varIdInfoPair.first] = varIdInfoPair.second;
        }

        // Connect all variables to the VariableRequestBus
        for (const auto& variablePair : m_runtimeData.m_variableData.GetVariables())
        {
            VariableRequestBus::MultiHandler::BusConnect(variablePair.first);
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
        return foundIt != variableMap.end() ? AZStd::string_view(foundIt->second.m_varName) : "";
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

        return variableNameValuePair ? AZStd::string_view(variableNameValuePair->m_varName) : "";
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
