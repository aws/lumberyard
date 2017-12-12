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
#include <AzCore/Debug/Trace.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Entity/EntityReference.h>
#include <ScriptCanvas/AST/Node.h>
#include <ScriptCanvas/Debugger/Bus.h>

#include "Core.h"
#include "GraphBus.h" 
#include "GraphAsset.h"

namespace ScriptCanvas
{
    class Node;
    
    namespace Execution
    {
        class ASTInterpreter;
    }

    bool UseVMZeroPointOnePipeline();
    
    //! Structure for maintaining GraphData
    struct GraphData
    {
        AZ_TYPE_INFO(GraphData, "{ADCB5EB5-8D3F-42ED-8F65-EAB58A82C381}");
        AZ_CLASS_ALLOCATOR(GraphData, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        void BuildEndpointMap();

        using NodeContainer = AZStd::unordered_set<AZ::Entity*>;
        using ConnectionContainer = AZStd::vector<AZ::Entity*>;

        NodeContainer m_nodes;
        ConnectionContainer m_connections;

        // An endpoint(NodeId, SlotId Pair) is represents one end of a potential connection
        // The endpoint map is lookup table for all endpoints connected on the opposite end of the key value endpoint
        AZStd::unordered_multimap<Endpoint, Endpoint> m_endpointMap; ///< Endpoint map built at edit time based on active connections

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    };

    //! Graph is the execution model of a ScriptCanvas graph.
    class Graph 
        : public AZ::Component
        , protected GraphRequestBus::MultiHandler
        , protected Debugger::RequestBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    public:

        AZ_COMPONENT(Graph, "{C3267D77-EEDC-490E-9E42-F1D1F473E184}");

        static void Reflect(AZ::ReflectContext* context);

        Graph(const AZ::EntityId& uniqueId = AZ::Entity::MakeId());

        ~Graph() override;

        void Activate() override;
        void Deactivate() override;
        AZStd::string GetLastErrorDescription() const;
        Node* GetNode(const ID& id) const;
        const AZStd::vector<AZ::EntityId> GetNodesConst() const;
        void HandleError(const Node& callStackTop);
        void Init() override;
        AZ_INLINE bool IsInErrorState() const { return m_isInErrorState; }
        AZ_INLINE bool IsInIrrecoverableErrorState() const { return m_isInErrorState && !m_isRecoverable; }
        void SetStartNode(AZ::EntityId nodeID);
        void ReportError(const Node& reporter, const char* format, ...);
        
        // GraphRequestBus::Handler
        bool AddNode(const AZ::EntityId&) override;
        bool RemoveNode(const AZ::EntityId& nodeId);
        AZStd::vector<AZ::EntityId> GetNodes() const override;
        AZStd::unordered_set<AZ::Entity*>& GetNodeEntities() { return m_graphData.m_nodes; }
        const AZStd::unordered_set<AZ::Entity*>& GetNodeEntities() const { return m_graphData.m_nodes; }

        bool AddConnection(const AZ::EntityId&) override;
        bool RemoveConnection(const AZ::EntityId& nodeId) override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;
        bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const override;

        bool Connect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId) override;
        bool Disconnect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId) override;

        bool ConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        AZ::Outcome<void, AZStd::string> CanConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const override;
        bool DisconnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        bool DisconnectById(const AZ::EntityId& connectionId) override;

        //! Retrieves the Entity this Graph component is currently located on
        //! NOTE: There can be multiple Graph components on the same entity so calling FindComponent may not not return this GraphComponent
        AZ::Entity* GetGraphEntity() const override { return GetEntity(); }

        Graph* GetGraph() { return this; }

        GraphData* GetGraphData() override{ return &m_graphData; }
        const GraphData* GetGraphDataConst() const override{ return &m_graphData; }

        bool AddGraphData(const GraphData&) override;
        void RemoveGraphData(const GraphData&) override;

        AZStd::unordered_set<AZ::Entity*> CopyItems(const AZStd::unordered_set<AZ::Entity*>& entities) override;
        void AddItems(const AZStd::unordered_set<AZ::Entity*>& graphField) override;
        void RemoveItems(const AZStd::unordered_set<AZ::Entity*>& graphField) override;
        void RemoveItems(const AZStd::vector<AZ::Entity*>& graphField);
        AZStd::unordered_set<AZ::Entity*> GetItems() const override;

        bool AddItem(AZ::Entity* itemRef) override;
        bool RemoveItem(AZ::Entity* itemRef) override;

        ///////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////////
        // Debugger::RequestBus
        void SetBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot) override;
        void RemoveBreakpoint(const AZ::EntityId& graphId, const AZ::EntityId& nodeId, const SlotId& slot) override;

        void StepOver() override;
        void StepIn() override;
        void StepOut() override;
        void Stop() override;
        void Continue() override;
        //////////////////////////////////////////////////////////////////////////

        // Debugger::ServiceBus
        //void Attach() override;
        ////////////////////////////////////////////////////////////////////////////////////

        // AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZ::EntityId GetUniqueId() const { return m_uniqueId; };

        void ResolveSelfReferences(const AZ::EntityId& graphOwnerId);

#if defined(SCRIPTCANVAS_ERRORS_ENABLED)
        int AddToExecutionStack(const Node& node);
        int RemoveFromExecutionStack(const Node& node);
#endif//defined(SCRIPTCANVAS_ERRORS_ENABLED)

    protected:
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }
        
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            (void)incompatible;
        }
        
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        void ErrorIrrecoverably();
        Node* GetErrorHandler() const;
        //! Searches for the node ids that from both endpoints of the connection inside of the supplied node set container
        bool ValidateConnectionEndpoints(const AzFramework::EntityReference& connectionRef, const AZStd::unordered_set<AzFramework::EntityReference>& nodeRefs);

    private:
        bool m_isInErrorState = false;
        
        GraphData m_graphData;

        AZStd::vector<AZ::EntityId> m_entryPointNodes;
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_errorHandlersBySource;

        AZ::EntityId m_uniqueId;

        bool m_isFinalErrorReported = false;
        bool m_isRecoverable = true;
        bool m_isInErrorHandler = false;
        const Node* m_errorReporter;
        Node* m_errorHandler = nullptr;
        AZStd::string m_errorDescription;

#if defined(SCRIPTCANVAS_ERRORS_ENABLED)
        AZStd::unordered_map<const Node*, int> m_executionStack;
#endif

//////////////////////////////////////////////////////////////////////////
/// VM 0.1 begin
    private:
        Graph(const Graph&) = delete;
        bool m_executeWithVM = false;        
        AST::NodePtrConst m_ast = nullptr;
        AZStd::unique_ptr<Execution::ASTInterpreter> m_astInterpreter;
/// VM 0.1 end
//////////////////////////////////////////////////////////////////////////
        class GraphEventHandler;
    };
    
}