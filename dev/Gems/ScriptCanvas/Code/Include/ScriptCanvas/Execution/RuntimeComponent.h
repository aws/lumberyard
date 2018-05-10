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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/VariableData.h>

namespace ScriptCanvas
{
    using VariableIdMap = AZStd::unordered_map<VariableId, VariableId>;

    //! Runtime Component responsible for loading a ScriptCanvas Graph from a runtime
    //! asset.
    //! It contains none of the Graph functionality of Validating Connections, as well as adding
    //! and removing nodes. Furthermore none of the functionality to remove and add variables
    //! exist in this component.
    //! It is assumed that the graph has runtime graph has already been validated and compiled
    //! at this point.
    //! This component should only be used at runtime 
    class RuntimeComponent
        : public AZ::Component
        , protected RuntimeRequestBus::Handler
        , protected VariableRequestBus::MultiHandler
        , private AZ::Data::AssetBus::Handler
		, public AZ::EntityBus::Handler
    {
    public:
        friend class Node;

        AZ_COMPONENT(RuntimeComponent, "{95BFD916-E832-4956-837D-525DE8384282}");

        static void Reflect(AZ::ReflectContext* context);

        RuntimeComponent(AZ::EntityId uniqueId = AZ::Entity::MakeId());
        RuntimeComponent(AZ::Data::Asset<RuntimeAsset> runtimeAsset, AZ::EntityId uniqueId = AZ::Entity::MakeId());
        ~RuntimeComponent() override;

        //// AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        AZ::EntityId GetUniqueId() const { return m_uniqueId; };

        void SetRuntimeAsset(const AZ::Data::Asset<RuntimeAsset>& runtimeAsset);

        const AZStd::vector<AZ::EntityId> GetNodesConst() const;

        //// RuntimeRequestBus::Handler
        AZ::EntityId GetRuntimeEntityId() const override { return GetEntity() ? GetEntityId() : AZ::EntityId(); }
        Node* FindNode(AZ::EntityId nodeId) const override;
        AZStd::vector<AZ::EntityId> GetNodes() const override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;
        GraphData* GetGraphData() override;
        const GraphData* GetGraphDataConst() const override { return const_cast<RuntimeComponent*>(this)->GetGraphData(); }

        VariableData* GetVariableData() override;
        const VariableData* GetVariableDataConst() const { return const_cast<RuntimeComponent*>(this)->GetVariableData(); }
        const AZStd::unordered_map<VariableId, VariableNameValuePair>* GetVariables() const override;
        VariableDatum* FindVariable(AZStd::string_view propName) override;
        VariableNameValuePair* FindVariableById(const VariableId& variableId) override;
        Data::Type GetVariableType(const VariableId& variableId) const override;
        AZStd::string_view GetVariableName(const VariableId& variableId) const override;
        ////

        void SetVariableOverrides(const VariableData& overrideData);
        void ApplyVariableOverrides();
        void SetVariableEntityIdMap(const AZStd::unordered_map<AZ::u64, AZ::EntityId> variableEntityIdMap);

        //// VariableRequestBus::Handler
        VariableDatum* GetVariableDatum() override;
        const VariableDatum* GetVariableDatumConst() const override { return const_cast<RuntimeComponent*>(this)->GetVariableDatum(); }
        Data::Type GetType() const override;
        AZStd::string_view GetName() const override;
        AZ::Outcome<void, AZStd::string> RenameVariable(AZStd::string_view newVarName) override;
        ////

        //// AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        // AZ::EntityBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        ////

        bool IsInErrorState() const { return m_executionContext.IsInErrorState(); }

    protected:
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // Cannot be used with either the GraphComponent or the VariableManager Component
            incompatible.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
            incompatible.push_back(AZ_CRC("ScriptCanvasVariableService", 0x819c8460));
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasRuntimeService", 0x776e1e3a));
        }

        //// Execution
        void ActivateNodes();
        void ActivateGraph();
        void DeactivateGraph();
        void CreateAssetInstance();
        ////

    private:
        AZ::EntityId m_uniqueId;
        RuntimeData m_runtimeData;
        AZ::Data::Asset<RuntimeAsset> m_runtimeAsset;
        //<! Per instance variable data overrides for the runtime asset
        //<! This is serialized when building this component from the EditorScriptCanvasComponent
        VariableData m_variableOverrides;

        ExecutionContext m_executionContext;
        
        // Script Canvas VariableId populated when the RuntimeAsset loads
        VariableIdMap m_editorToRuntimeVariableMap;

		AZStd::unordered_set< Node* > m_entryNodes;

        // Script Canvas VariableId map populated by the EditorScriptCanvasComponent in build game Entity
        AZStd::unordered_map<AZ::u64, AZ::EntityId> m_variableEntityIdMap;
    };
}
