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

#include <AzCore/std/containers/unordered_map.h>

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Bus/GraphBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

#include <GraphCanvas/Widgets/NodePropertyBus.h>

namespace ScriptCanvasEditor
{
    //! EditorGraph is the editor version of the ScriptCanvas::Graph component that is activated when executing the script canvas engine
    class Graph
        : public ScriptCanvas::Graph
        , private NodeCreationNotificationBus::Handler
        , private SceneCounterRequestBus::Handler
        , private EditorGraphRequestBus::Handler
        , private GraphCanvas::GraphModelRequestBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
        , private GraphItemCommandNotificationBus::Handler
    {
    private:
        typedef AZStd::unordered_map< AZ::EntityId, AZ::EntityId > WrappedNodeGroupingMap;

        static void ConvertToGetVariableNode(Graph* graph, ScriptCanvas::VariableId variableId, const AZ::EntityId& nodeId, AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& setVariableRemapping);

    public:
        AZ_COMPONENT(Graph, "{4D755CA9-AB92-462C-B24F-0B3376F19967}", ScriptCanvas::Graph);

        static void Reflect(AZ::ReflectContext* context);

        Graph(const AZ::EntityId& uniqueId = AZ::Entity::MakeId())
            : ScriptCanvas::Graph(uniqueId)
            , m_variableCounter(0)
            , m_graphCanvasSceneEntity(nullptr)
            , m_loading(false)
        {}
        ~Graph() override;

        void Activate() override;
        void Deactivate() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            ScriptCanvas::Graph::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("EditorScriptCanvasService", 0x975114ff));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            ScriptCanvas::Graph::GetIncompatibleServices(incompatible);
            incompatible.push_back(AZ_CRC("EditorScriptCanvasService", 0x975114ff));
        }

        // SceneCounterRequestBus
        AZ::u32 GetNewVariableCounter() override;
        ////

        // GraphCanvas::GraphModelRequestBus
        void RequestUndoPoint() override;

        void RequestPushPreventUndoStateUpdate() override;
        void RequestPopPreventUndoStateUpdate() override;

        GraphCanvas::NodePropertyDisplay* CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreateDataSlotVariablePropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;

        void DisconnectConnection(const GraphCanvas::ConnectionId& connectionId) override;
        bool CreateConnection(const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) override;

        bool IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const override;
        bool IsValidVariableAssignment(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& targetPoint) const override;

        bool ShouldWrapperAcceptDrop(const GraphCanvas::NodeId& wrapperNode, const QMimeData* mimeData) const override;

        void AddWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;
        void RemoveWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;

        AZStd::string GetDataTypeString(const AZ::Uuid& typeId) override;
        ////

        // SceneNotificationBus
        void OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget) override;
        void OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource) override;
        void OnPreNodeDeleted(const AZ::EntityId& nodeId) override;
        void OnPreConnectionDeleted(const AZ::EntityId& nodeId) override;

        void PostDeletionEvent() override;
        void PostCreationEvent() override;
        void OnPasteBegin() override;
        void OnPasteEnd() override;
        
        void OnViewRegistered() override;
        /////////////////////////////////////////////////////////////////////////////////////////////

        //! NodeCreationNotifications
        void OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId) override;
        ///////////////////////////

        // EditorGraphRequestBus
        void CreateGraphCanvasScene() override;

        GraphCanvas::GraphId GetGraphCanvasGraphId() const override;

        NodeIdPair CreateCustomNode(const AZ::Uuid& typeId, const AZ::Vector2& position) override;
        ////

        // EntitySaveDataGraphActionBus
        void OnSaveDataDirtied(const AZ::EntityId& savedElement) override;
        ////

        // Save Information Conversion
        bool NeedsSaveConversion() const;
        void ConvertSaveFormat();

        void ConstructSaveData();
        ////

    protected:
        void PostRestore(const UndoData& restoredData);

    private:
        Graph(const Graph&) = delete;

        ScriptCanvas::Endpoint ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoint) const;
        GraphCanvas::NodePropertyDisplay* CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const;

        void SignalDirty();

        //// Version Update code
        AZ::Outcome<void, AZStd::string> ConvertPureDataToVariables();

        AZ::u32 m_variableCounter;
        AZ::EntityId m_wrapperNodeDropTarget;
        
        WrappedNodeGroupingMap m_wrappedNodeGroupings;
        AZStd::vector< AZ::EntityId > m_lastGraphCanvasCreationGroup;
        
        AZ::Entity* m_graphCanvasSceneEntity;
        AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > m_graphCanvasSaveData;

        bool m_loading;

        //! Defaults to true to signal that the all PureData nodes have been converted to variables
        bool m_pureDataNodesConverted = true;

        //! Defaults to true to signal that this graph does not have the GraphCanvas stuff intermingled
        bool m_saveFormatConverted = true;
    };
}