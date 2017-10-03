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
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Bus/GraphBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>

namespace ScriptCanvasEditor
{
    //! EditorGraph is the editor version of the ScriptCanvas::Graph component that is activated when executing the script canvas engine
    class Graph
        : public ScriptCanvas::Graph
        , private NodeCreationNotificationBus::Handler
        , private VariableNodeSceneRequestBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
        , private GraphCanvas::NodePropertySourceRequestBus::Handler
        , private GraphCanvas::ConnectionSceneRequestBus::Handler
        , private GraphCanvas::WrapperNodeActionRequestBus::Handler
        , private GraphCanvas::VariableActionRequestBus::Handler
        , private GraphCanvas::DataSlotActionRequestBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
    private:
        typedef AZStd::unordered_map< AZ::EntityId, AZ::EntityId > EbusMethodGroupingMap;

    public:
        AZ_COMPONENT(Graph, "{4D755CA9-AB92-462C-B24F-0B3376F19967}", ScriptCanvas::Graph);

        static void Reflect(AZ::ReflectContext* context);

        Graph(const AZ::EntityId& uniqueId = AZ::Entity::MakeId())
            : ScriptCanvas::Graph(uniqueId)
            , m_variableCounter(0)
        {}
        ~Graph() override = default;

        void Activate() override;
        void Deactivate() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            ScriptCanvas::Graph::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("EditorScriptCanvasService", 0x975114ff));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_SceneService", 0x8ec010e7));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            ScriptCanvas::Graph::GetIncompatibleServices(incompatible);
            incompatible.push_back(AZ_CRC("EditorScriptCanvasService", 0x975114ff));
        }

        // TODO: Currently the Compile function flattens an ScriptCanvasAssetNodes, into one graph, without respect to scoping or namespacing
        // Support for graph scoping is needed, to prevent nodes accessing data that they shouldn't be allowed to.
        // Furthermore this currently causes all start nodes to be placed in one graph and to start executing immediately
        ScriptCanvas::Graph* Compile(const AZ::EntityId& graphOwnerId);

        // VariableNodeSceneRequestBus
        AZ::u32 GetNewVariableCounter() override;
        ////

        // SceneNotificationBus
        void OnItemMouseMoveComplete(const AZ::EntityId& nodeId, const QGraphicsSceneMouseEvent* event) override;
        void OnEntitiesSerialized(GraphCanvas::SceneSerialization& serializationTarget) override;
        void OnEntitiesDeserialized(const GraphCanvas::SceneSerialization& serializationSource) override;
        void OnPreNodeDeleted(const AZ::EntityId& nodeId) override;
        void OnPreConnectionDeleted(const AZ::EntityId& nodeId) override;

        void PostDeletionEvent() override;
        void PostCreationEvent() override;
        void OnPasteBegin() override;
        void OnPasteEnd() override;
        /////////////////////////////////////////////////////////////////////////////////////////////

        // ConnectionSceneRequestBus
        void DisconnectConnection(const AZ::EntityId& connectionId) override;
        bool CreateConnection(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) override;

        bool IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const override;
        bool IsValidVariableAssignment(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& targetPoint) const override;

        double GetSnapDistance() const override;
        ////

        //! NodeCreationNotifications
        void OnGraphCanvasNodeCreated(const AZ::EntityId& nodeId) override;
        ///////////////////////////

        // WrapperNodeActionRequests
        bool ShouldAcceptDrop(const AZ::EntityId& wrapperNode, const QMimeData* mimeData) const override;

        void AddWrapperDropTarget(const AZ::EntityId& wrapperNode) override;
        void RemoveWrapperDropTarget(const AZ::EntityId& wrapperNode) override;
        /////////////////////////////////////////////////////////////////////////////////////////////

        // VariableActionRequestBuss
        void AssignVariableValue(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& endpoint) override;
        void UnassignVariableValue(const AZ::EntityId& variableId, const GraphCanvas::Endpoint& endpoint) override;
        ////

        // DataSlotActionRequestBus
        AZStd::string GetTypeString(const AZ::Uuid& typeId) override;
        ////

        // NodePropertySourceRequestBus
        GraphCanvas::NodePropertyDisplay* CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreateDataSlotVariablePropertyDisplay(const AZ::Uuid& dataType, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const AZ::EntityId& nodeId, const AZ::EntityId& slotId) const override;
        /////////////////////////////////////////////////////////////////////////////////////////////

        // EditorEntityContextNotificationBus
        // This Notifications is handled to remap EntityId references in ScriptCanvas nodes when slices are created
        void OnEditorEntitiesReplacedBySlicedEntities(const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& replacedEntityMap) override;
        ////

    private:
        Graph(const Graph&) = delete;

        ScriptCanvas::Endpoint ConvertToScriptCanvasEndpoint(const GraphCanvas::Endpoint& endpoint) const;

        GraphCanvas::NodePropertyDisplay* CreateDisplayPropertyForSlot(const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId) const;

        AZ::u32 m_variableCounter;
        AZ::EntityId m_wrapperNodeDropTarget;
        
        EbusMethodGroupingMap m_ebusMethodGroupings;
        AZStd::vector< AZ::EntityId > m_lastGraphCanvasCreationGroup;
    };
}