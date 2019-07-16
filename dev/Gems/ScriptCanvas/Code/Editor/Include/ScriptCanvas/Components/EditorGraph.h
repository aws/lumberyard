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
#include <ScriptCanvas/GraphCanvas/VersionControlledNodeBus.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/Components/EditorUtils.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>
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
        , private VersionControlledNodeNotificationBus::MultiHandler
    {
    private:
        typedef AZStd::unordered_map< AZ::EntityId, AZ::EntityId > WrappedNodeGroupingMap;

        static void ConvertToGetVariableNode(Graph* graph, ScriptCanvas::VariableId variableId, const AZ::EntityId& nodeId, AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& setVariableRemapping);

        struct CRCCache
        {
            AZ_TYPE_INFO(CRCCache, "{59798D92-94AD-4A08-8F38-D5975B0DC33B}");

            CRCCache()
                : m_cacheCount(0)
            {
            }

            CRCCache(const AZStd::string& cacheString)
                : m_cacheValue(cacheString)
                , m_cacheCount(1)
            {

            }

            AZStd::string m_cacheValue;
            int m_cacheCount;
        };

    public:
        AZ_COMPONENT(Graph, "{4D755CA9-AB92-462C-B24F-0B3376F19967}", ScriptCanvas::Graph);

        static void Reflect(AZ::ReflectContext* context);

        Graph(const AZ::EntityId& uniqueId = AZ::Entity::MakeId())
            : ScriptCanvas::Graph(uniqueId)
            , m_variableCounter(0)
            , m_graphCanvasSceneEntity(nullptr)
            , m_ignoreSaveRequests(false)
            , m_graphCanvasSaveVersion(GraphCanvas::EntitySaveDataContainer::CurrentVersion)
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
        void ReleaseVariableCounter(AZ::u32 variableCounter) override;
        ////

        // GraphCanvas::GraphModelRequestBus
        void RequestUndoPoint() override;

        void RequestPushPreventUndoStateUpdate() override;
        void RequestPopPreventUndoStateUpdate() override;

        void TriggerUndo() override;
        void TriggerRedo() override;

        GraphCanvas::NodePropertyDisplay* CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreateDataSlotVariablePropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;

        void DisconnectConnection(const GraphCanvas::ConnectionId& connectionId) override;
        bool CreateConnection(const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) override;

        bool IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const override;
        bool IsValidVariableAssignment(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& targetPoint) const override;       

        AZStd::string GetDataTypeString(const AZ::Uuid& typeId) override;

        void OnRemoveUnusedNodes() override;
        void OnRemoveUnusedElements() override;

        bool ShouldWrapperAcceptDrop(const GraphCanvas::NodeId& wrapperNode, const QMimeData* mimeData) const override;

        void AddWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;
        void RemoveWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;        
        ////

        // SceneNotificationBus
        void OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget) override;
        void OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource) override;
        void OnPreNodeDeleted(const AZ::EntityId& nodeId) override;
        void OnPreConnectionDeleted(const AZ::EntityId& nodeId) override;

        void OnUnknownPaste(const QPointF& scenePos) override;

        void OnSelectionChanged() override;

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
        void ClearGraphCanvasScene() override;
        void DisplayGraphCanvasScene() override;

        GraphCanvas::GraphId GetGraphCanvasGraphId() const override;

        AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > GetGraphCanvasSaveData();
        void UpdateGraphCanvasSaveData(const AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* >& saveData);

        NodeIdPair CreateCustomNode(const AZ::Uuid& typeId, const AZ::Vector2& position) override;

        void AddCrcCache(const AZ::Crc32& crcValue, const AZStd::string& cacheString) override;
        void RemoveCrcCache(const AZ::Crc32& crcValue) override;
        AZStd::string DecodeCrc(const AZ::Crc32& crcValue) override;

        void ClearHighlights() override;
        void HighlightMembersFromTreeItem(const GraphCanvas::GraphCanvasTreeItem* treeItem) override;
        void HighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds) override;
        void HighlightNodes(const AZStd::vector<NodeIdPair>& nodes) override;
        
        void RemoveUnusedVariables() override;

        AZStd::vector<NodeIdPair> GetNodesOfType(const ScriptCanvas::NodeTypeIdentifier&) override;
        AZStd::vector<NodeIdPair> GetVariableNodes(const ScriptCanvas::VariableId&) override;

        void QueueVersionUpdate(const AZ::EntityId& graphCanvasNodeId) override;
        ////

        // VersionControlledNodeNotificationBus
        void OnVersionConversionBegin() override;
        void OnVersionConversionEnd() override;
        ////

        // EntitySaveDataGraphActionBus
        void OnSaveDataDirtied(const AZ::EntityId& savedElement) override;
        ////

        // Save Information Conversion
        bool NeedsSaveConversion() const;
        void ConvertSaveFormat();

        void ConstructSaveData();
        ////

        const GraphStatisticsHelper& GetNodeUsageStatistics() const;

    protected:
        void PostRestore(const UndoData& restoredData);

    private:
        Graph(const Graph&) = delete;

        ScriptCanvas::Endpoint ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoint) const;
        GraphCanvas::Endpoint ConvertToGraphCanvasEndpoint(const ScriptCanvas::Endpoint& endpoint) const;

        GraphCanvas::NodePropertyDisplay* CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const;

        void SignalDirty();

        void HighlightNodesByType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier);
        void HighlightEBusNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId);
        void HighlightScriptEventNodes(const ScriptCanvas::EBusBusId& busId, const ScriptCanvas::EBusEventId& eventId);
        void HighlightScriptCanvasEntity(const AZ::EntityId& scriptCanvasId);

        AZ::EntityId FindGraphCanvasSlotId(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::SlotId& slotId);
        bool ConfigureConnectionUserData(const ScriptCanvas::Endpoint& sourceEndpoint, const ScriptCanvas::Endpoint& targetEndpoint, GraphCanvas::ConnectionId connectionId);

        void HandleQueuedUpdates();
        bool IsNodeVersionConverting(const AZ::EntityId& graphCanvasNodeId) const;

        //// Version Update code
        AZ::Outcome<void, AZStd::string> ConvertPureDataToVariables();

        AZStd::unordered_set< AZ::EntityId > m_queuedConvertingNodes;
        AZStd::unordered_set< AZ::EntityId > m_convertingNodes;

        AZ::u32 m_variableCounter;
        AZ::EntityId m_wrapperNodeDropTarget;
        
        WrappedNodeGroupingMap m_wrappedNodeGroupings;
        AZStd::vector< AZ::EntityId > m_lastGraphCanvasCreationGroup;
        
        AZ::Entity* m_graphCanvasSceneEntity;

        GraphCanvas::EntitySaveDataContainer::VersionInformation m_graphCanvasSaveVersion;
        AZStd::unordered_map< AZ::EntityId, GraphCanvas::EntitySaveDataContainer* > m_graphCanvasSaveData;

        AZStd::unordered_map< AZ::Crc32, CRCCache > m_crcCacheMap;

        AZStd::unordered_set< GraphCanvas::GraphicsEffectId > m_highlights;

        GraphStatisticsHelper m_statisticsHelper;

        bool m_ignoreSaveRequests;

        //! Defaults to true to signal that the all PureData nodes have been converted to variables
        bool m_pureDataNodesConverted = true;

        //! Defaults to true to signal that this graph does not have the GraphCanvas stuff intermingled
        bool m_saveFormatConverted = true;
    };
}
