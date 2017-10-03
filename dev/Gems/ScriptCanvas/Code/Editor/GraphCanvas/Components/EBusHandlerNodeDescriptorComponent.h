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

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptorComponent.h>

namespace ScriptCanvasEditor
{
    class EBusHandlerNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public EBusNodeDescriptorRequestBus::Handler
        , public GraphCanvas::NodeNotificationBus::Handler
        , public GraphCanvas::WrapperNodeNotificationBus::Handler
        , public GraphCanvas::GraphCanvasPropertyBus::Handler
    {
    public:
        AZ_COMPONENT(EBusHandlerNodeDescriptorComponent, "{A93B4B22-DBB8-4F18-B741-EB041BFEA4F6}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        EBusHandlerNodeDescriptorComponent();
        EBusHandlerNodeDescriptorComponent(const AZStd::string& busName);
        ~EBusHandlerNodeDescriptorComponent() = default;

        void Activate() override;
        void Deactivate() override;

        // NodeNotifications
        void OnAddedToScene(const AZ::EntityId& sceneId) override;
        ////

        // EBusNodeDescriptorRequestBus
        AZStd::string GetBusName() const override;

        GraphCanvas::WrappedNodeConfiguration GetEventConfiguration(const AZStd::string& eventName) const override;
        bool ContainsEvent(const AZStd::string& eventName) const override;

        AZStd::vector< AZStd::string > GetEventNames() const override;

        AZ::EntityId FindEventNodeId(const AZStd::string& eventName) const override;
        ////

        // WrapperNodeNotificationBus
        void OnWrappedNode(const AZ::EntityId& wrappedNode) override;
        void OnUnwrappedNode(const AZ::EntityId& unwrappedNode) override;
        ////

        // GraphCanvasPropertyBus
        AZ::Component* GetPropertyComponent() override;
        ////
        
    private:

        void OnDisplayConnectionsChanged();
        
        bool         m_displayConnections;
        AZ::EntityId m_scriptCanvasId;
    
        AZStd::string m_busName;
        AZStd::unordered_map< AZStd::string, AZ::EntityId > m_eventTypeToId;
        AZStd::unordered_map< AZ::EntityId, AZStd::string > m_idToEventType;
    };
}