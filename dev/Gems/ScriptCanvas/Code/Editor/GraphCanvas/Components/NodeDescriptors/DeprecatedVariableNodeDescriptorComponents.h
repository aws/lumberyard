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

#include <AzCore/Component/EntityBus.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/NodePropertyDisplay/StringDataInterface.h>
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Endpoint.h>

namespace ScriptCanvasEditor
{
    namespace Deprecated
    {
        // Deprecated in 1.12
        class VariableNodeDescriptorComponent
            : public NodeDescriptorComponent
            , public VariableNodeDescriptorRequestBus::Handler
            , public GraphCanvas::Deprecated::SceneVariableRequestBus::Handler
            , public GraphCanvas::Deprecated::VariableRequestBus::Handler
            , public GraphCanvas::NodeNotificationBus::Handler
            , public GraphCanvas::SceneMemberNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(VariableNodeDescriptorComponent, "{2E21B07C-B963-413A-B753-1BAFD0AEEA7D}", NodeDescriptorComponent);
            static void Reflect(AZ::ReflectContext* reflectContext);

            VariableNodeDescriptorComponent();
            ~VariableNodeDescriptorComponent() = default;

            // Component
            void Activate() override;
            void Deactivate() override;
            ////

            // SceneVariableRequestBus
            AZ::EntityId GetVariableId() const override;
            ////

            // VariableRequestBus
            AZStd::string GetVariableName() const override;
            AZ::Uuid GetDataType() const override;
            AZStd::string GetDataTypeDisplayName() const override;
            ////

            // VariableNodeDescriptorRequestBus::Handler
            ScriptCanvas::Endpoint GetReadEndpoint() const override;
            ScriptCanvas::Endpoint GetWriteEndpoint() const override;
            /////

            // GraphCanvas::NodeNotificationBus
            void OnAddedToScene(const AZ::EntityId& sceneId) override;
            void OnRemovedFromScene(const AZ::EntityId& sceneId) override;            
            ////

            // GraphCanvas::SceneMemberNotifications
            void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization& graphSerialization) override;
            ////

        private:

            void ActivateVariable();
            void UpdateTitleDisplay();

            AZ::Uuid m_savedDataType;
            AZStd::string m_displayName;

            ScriptCanvas::Endpoint m_variableSourceEndpoint;
            ScriptCanvas::Endpoint m_variableWriteEndpoint;

            // Variables work by just hiding the connections visually, while still having a connection active on the backend.
            // To facilitate this, we want to keep track of our connections so we can clean them up.
            AZStd::unordered_map< AZ::EntityId, AZ::EntityId > m_slotIdToConnectionId;
        };
    
        class SetVariableNodeDescriptorComponent
            : public NodeDescriptorComponent
            , public SetVariableNodeDescriptorRequestBus::Handler
            , public GraphCanvas::NodeNotificationBus::Handler
            , public GraphCanvas::SlotNotificationBus::Handler
            , public GraphCanvas::SceneMemberNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(SetVariableNodeDescriptorComponent, "{11747A7B-13F4-4CAE-9743-08995532FC0B}", NodeDescriptorComponent);
            static void Reflect(AZ::ReflectContext* reflectContext);
            
            SetVariableNodeDescriptorComponent();
            SetVariableNodeDescriptorComponent(const AZ::EntityId& variableId);
            ~SetVariableNodeDescriptorComponent() = default;

            // Component
            void Activate() override;
            void Deactivate() override;
            ////

            // SetVariableNodeDescriptorRequestBus
            AZ::EntityId GetVariableId() const override;
            ////

            // SlotNotificationBus
            using GraphCanvas::SlotNotificationBus::Handler::OnNameChanged;
            void OnConnectedTo(const AZ::EntityId& entityId, const GraphCanvas::Endpoint& endpoint) override;
            void OnDisconnectedFrom(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint) override;
            ////
            
            // GraphCanvas::NodeNotificationBus
            using GraphCanvas::NodeNotificationBus::Handler::OnNameChanged;
            void OnAddedToScene(const AZ::EntityId& sceneId) override;
            ////

            // GraphCanvas::SceneMemberNotificationBus
            void OnRemovedFromScene(const AZ::EntityId& sceneId) override;
            ////
            
        private:
        
            void DisconnectFromEndpoint();
            bool ConnectToEndpoint();
        
            void ClearPins();

            void UpdateTitle();

            AZ::EntityId m_variableId;
            AZ::EntityId m_dataConnectionSlot;

            ScriptCanvas::Endpoint m_setEndpoint;
            ScriptCanvas::Endpoint m_getEndpoint;
        };
        
        class GetVariableNodeDescriptorComponent
            : public NodeDescriptorComponent
            , public GetVariableNodeDescriptorRequestBus::Handler
            , public GraphCanvas::NodeNotificationBus::Handler
            , public GraphCanvas::SceneMemberNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(GetVariableNodeDescriptorComponent, "{4FE9C826-0CE6-40D0-8253-3A7524358819}", NodeDescriptorComponent);
            static void Reflect(AZ::ReflectContext* reflectContext);
            
            GetVariableNodeDescriptorComponent();
            GetVariableNodeDescriptorComponent(const AZ::EntityId& variableId);
            ~GetVariableNodeDescriptorComponent() = default;

            // Component
            void Activate() override;
            void Deactivate() override;
            ////

            // GetVariableNodeDescriptorRequests
            AZ::EntityId GetVariableId() const override;
            ////
            
            // GraphCanvas::NodeNotificationBus
            using GraphCanvas::NodeNotificationBus::Handler::OnNameChanged;
            void OnAddedToScene(const AZ::EntityId& sceneId) override;
            ////

            // GraphCanvas::SceneMemberNotificationBus
            void OnRemovedFromScene(const AZ::EntityId& sceneId) override;
            ////
            
        private:
        
            void PopulateExternalSlotIds();
        
            void ClearPins();

            void UpdateTitle();

            AZ::EntityId m_variableId;

            AZStd::unordered_set< ScriptCanvas::SlotId > m_externalSlotIds;
        };
    }
}