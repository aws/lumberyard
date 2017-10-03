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

#include "Editor/GraphCanvas/Components/NodeDescriptorComponent.h"

#include "ScriptCanvas/Core/NodeBus.h"
#include "ScriptCanvas/Core/Endpoint.h"

namespace ScriptCanvasEditor
{
    class GetVariableNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public GetVariableNodeDescriptorRequestBus::Handler
        , public GraphCanvas::NodeNotificationBus::Handler        
        , public GraphCanvas::VariableNotificationBus::Handler
    {
    private:
        class GetVariableReferenceDataInterface
            : public GraphCanvas::VariableReferenceDataInterface
            , public GetVariableNodeDescriptorNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(GetVariableReferenceDataInterface, AZ::SystemAllocator, 0);

            GetVariableReferenceDataInterface(const AZ::EntityId& busId);
            ~GetVariableReferenceDataInterface();

            // VariableReferenceDataInterface
            AZ::EntityId GetVariableReference() const override;
            void AssignVariableReference(const AZ::EntityId& variableId) override;

            AZ::Uuid GetVariableDataType() const override;
            /////

            // GetVariableNodeDescriptNotificationBus
			void OnVariableActivated() override;
            void OnAssignVariableChanged() override;
            ////

        private:

            AZ::EntityId m_busId;
        };

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
        GraphCanvas::VariableReferenceDataInterface* CreateVariableDataInterface() override;
        ScriptCanvas::Endpoint GetSourceEndpoint(ScriptCanvas::SlotId slotId) const override;

        AZ::EntityId GetVariableId() const override;
        void SetVariableId(const AZ::EntityId& variableId) override;
        ////

        // VariableNotificationBus::Handler
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
    
		void PopulateExternalSlotIds();
        void ConvertConnections(const AZ::EntityId& oldVariableId);
    
        void ClearPins();
        void PopulatePins();

        void UpdateTitle();

        AZ::EntityId m_variableId;

        AZStd::unordered_set< ScriptCanvas::SlotId > m_externalSlotIds;
    };
}