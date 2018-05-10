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

#include "Editor/GraphCanvas/Components/NodeDescriptorComponent.h"

#include "ScriptCanvas/Core/NodeBus.h"
#include "ScriptCanvas/Core/Endpoint.h"

namespace ScriptCanvasEditor
{
    class VariableNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public VariableNodeDescriptorRequestBus::Handler
        , public GraphCanvas::SceneVariableRequestBus::Handler
        , public GraphCanvas::VariableRequestBus::Handler
        , public GraphCanvas::NodeNotificationBus::Handler
    {
    private:
        class VariableNodeDescriptorStringDataInterface
            : public GraphCanvas::StringDataInterface
            , public VariableNodeDescriptorNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(VariableNodeDescriptorStringDataInterface, AZ::SystemAllocator, 0);
            
            VariableNodeDescriptorStringDataInterface(const AZ::EntityId& busId);
            ~VariableNodeDescriptorStringDataInterface();

            // GraphCanvas::StringDataInterface
            AZStd::string GetString() const override;
            void SetString(const AZStd::string& newValue) override;
            ////

            // VariableNodeDescriptorNotificationBus
            void OnNameChanged() override;
            ////

        private:
            AZ::EntityId m_busId;
        };
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
        void SetVariableName(const AZStd::string& variableName) override;

        AZ::Uuid GetVariableDataType() const override;
        ////

        // VariableNodeDescriptorRequestBus::Handler
        GraphCanvas::StringDataInterface* CreateNameInterface() override;

        ScriptCanvas::Endpoint GetReadEndpoint() const override;
        ScriptCanvas::Endpoint GetWriteEndpoint() const override;

        void AddConnection(const GraphCanvas::Endpoint& endpoint, const AZ::EntityId& scConnectionId) override;
        void RemoveConnection(const GraphCanvas::Endpoint& endpoint) override;

        AZ::EntityId FindConnection(const GraphCanvas::Endpoint& endpoint) override;
        /////
        
        // GraphCanvas::NodeNotificationBus
        void OnAddedToScene(const AZ::EntityId& sceneId) override;
        void OnRemovedFromScene(const AZ::EntityId& sceneId) override;

        void OnNodeDeserialized(const GraphCanvas::SceneSerialization& sceneSerialization) override;
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
}