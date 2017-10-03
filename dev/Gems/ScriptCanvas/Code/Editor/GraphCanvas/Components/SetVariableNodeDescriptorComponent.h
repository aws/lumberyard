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
#include <GraphCanvas/Components/NodePropertyDisplay/VariableDataInterface.h>
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

#include "Editor/GraphCanvas/Components/NodeDescriptorComponent.h"

#include "ScriptCanvas/Core/NodeBus.h"
#include "ScriptCanvas/Core/Endpoint.h"

namespace ScriptCanvasEditor
{
    class SetVariableNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public SetVariableNodeDescriptorRequestBus::Handler
        , public GraphCanvas::NodeNotificationBus::Handler
        , public GraphCanvas::VariableNotificationBus::Handler
        , public GraphCanvas::SlotNotificationBus::Handler        
    {
    private:

        class SetVariableReferenceDataInterface
            : public GraphCanvas::VariableReferenceDataInterface
            , public SetVariableNodeDescriptorNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(SetVariableReferenceDataInterface, AZ::SystemAllocator, 0);

            SetVariableReferenceDataInterface(const AZ::EntityId& busId);
            ~SetVariableReferenceDataInterface();

            // VariableReferenceDataInterface
            AZ::EntityId GetVariableReference() const override;
            void AssignVariableReference(const AZ::EntityId& variableId) override;

            AZ::Uuid GetVariableDataType() const override;
            /////

            // SetVariableNodeDescriptNotificationBus
			void OnVariableActivated() override;
            void OnAssignVariableChanged() override;
            ////

        private:

            AZ::EntityId m_busId;
        };

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
        GraphCanvas::VariableReferenceDataInterface* CreateVariableDataInterface() override;

        AZ::EntityId GetVariableId() const override;
        void SetVariableId(const AZ::EntityId& variableId) override;
        ////

        // SlotNotificationBus
        using GraphCanvas::SlotNotificationBus::Handler::OnNameChanged;
        void OnConnectedTo(const AZ::EntityId& entityId, const GraphCanvas::Endpoint& endpoint) override;
        void OnDisconnectedFrom(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint) override;
        ////

        // VariableNotificationBus
        using GraphCanvas::VariableNotificationBus::Handler::OnNameChanged;
        void OnNameChanged() override;
        void OnVariableActivated() override;
        void OnVariableDestroyed() override;
        ////
        
        // GraphCanvas::NodeNotificationBus
        using GraphCanvas::NodeNotificationBus::Handler::OnNameChanged;
        void OnAddedToScene(const AZ::EntityId& sceneId) override;
        void OnRemovedFromScene(const AZ::EntityId& sceneId) override;
        ////
        
    private:
    
        void DisconnectFromEndpoint();
        bool ConnectToEndpoint();
    
        void ClearPins();
        void PopulatePins();

        void UpdateTitle();

        AZ::EntityId m_variableId;
        AZ::EntityId m_dataConnectionSlot;

        ScriptCanvas::Endpoint m_setEndpoint;
        ScriptCanvas::Endpoint m_getEndpoint;
    };
}