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

#include "precompiled.h"
#include "NodeUtils.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <Core/NodeBus.h>
#include <Core/ScriptCanvasBus.h>
#include <Core/Slot.h>
#include <Data/Data.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Components/IconComponent.h>
#include <Editor/GraphCanvas/Components/ClassMethodNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EBusHandlerNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EBusHandlerEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EBusSenderNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/EntityRefNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/GetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/SetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/VariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/UserDefinedNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/Translation/TranslationHelper.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/TranslationTypes.h>

#include <Libraries/Core/Assign.h>
#include <Libraries/Core/BehaviorContextObjectNode.h>
#include <Libraries/Core/EBusEventHandler.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Entity/EntityRef.h>

// Primitive data types we are hard coding for now
#include <Libraries/Logic/Boolean.h>
#include <Libraries/Math/Number.h>
#include <Libraries/Core/String.h>

namespace ScriptCanvasEditor
{
    namespace Nodes
    {
        static void RegisterAndActivateGraphCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::SlotId& slotId, AZ::Entity* slotEntity)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            if (slotEntity)
            {
                slotEntity->Init();
                slotEntity->Activate();

                GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());

                // Set the user data on the GraphCanvas slot to be the SlotId of the ScriptCanvas
                AZStd::any* slotUserData = nullptr;
                GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotEntity->GetId(), &GraphCanvas::SlotRequests::GetUserData);
                if (slotUserData)
                {
                    *slotUserData = slotId;
                }
            }
        }

        NodeIdPair CreateNode(const AZ::Uuid& classId, AZ::EntityId graphId, const AZStd::string& styleOverride)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Entity* graphCanvasEntity{};
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, styleOverride.c_str());
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();
            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(classId);
            graphCanvasEntity->CreateComponent<UserDefinedNodeDescriptorComponent>(classId);

            ScriptCanvas::Node* node{};
            AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, classId);

            // In the case of an EntityRef node, assign the graph owner's ID by default
            AZStd::string entityName;
            auto* entityNode = azrtti_cast<ScriptCanvas::Nodes::Entity::EntityRef*>(node);
            if (entityNode)
            {
                AZ::EntityId editorEntityId;
                EditorScriptCanvasRequestBus::EventResult(editorEntityId, graphId, &ScriptCanvasEditor::EditorScriptCanvasRequests::GetEditorEntityId);
                entityNode->SetEntityRef(editorEntityId);

                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, editorEntityId);
            }

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();
            
            // Create the GraphCanvas slots
            for (const auto& slot : node->GetSlots())
            {
                CreateGraphCanvasSlot(nodeIdPair.m_graphCanvasId, slot);
            }

            // Set the name
            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(classId);
            if (classData)
            {
                const char* name = entityName.empty() ? (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name : entityName.c_str();
                scriptCanvasEntity->SetName(AZStd::string::format("ScriptCanvas Node: %s", name));
                graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", name));
                GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, name);
                GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTitle, name);
                
                const char* descripton = (classData->m_editData && classData->m_editData->m_description) ? classData->m_editData->m_description : "";
                
                GraphCanvas::TranslationKeyedString nodeKeyedString(classData->m_editData->m_description);
                GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, nodeKeyedString);

            }

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData{};
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }

        NodeIdPair CreateEntityNode(const AZ::EntityId& sourceId, AZ::EntityId graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;
            
            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".entity");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();
            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Entity::EntityRef::RTTI_Type());
            graphCanvasEntity->CreateComponent<EntityRefNodeDescriptorComponent>();

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, ScriptCanvas::Nodes::Entity::EntityRef::RTTI_Type());

            auto* entityNode = azrtti_cast<ScriptCanvas::Nodes::Entity::EntityRef*>(node);
            entityNode->SetEntityRef(sourceId);

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();
            scriptCanvasEntity->Activate();
            
            // Create the GraphCanvas slots
            for (const auto& slot : node->GetSlots())
            {
                if (slot.GetType() == ScriptCanvas::SlotType::DataOut)
                {
                    CreateGraphCanvasSlot(nodeIdPair.m_graphCanvasId, slot);
                }
            }

            // Set the name
            AZ::Entity* sourceEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, sourceId);
            if (sourceEntity)
            {
                scriptCanvasEntity->SetName(AZStd::string::format("ScriptCanvas Node: %s", sourceEntity->GetName().data()));
                graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", sourceEntity->GetName().data()));
                GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, sourceEntity->GetName());
            }

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }
        
        NodeIdPair CreateEbusEventNode(const AZStd::string& busName, const AZStd::string& eventName, AZ::EntityId graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            NodeIdPair nodeIdPair;
            
            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".handler");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();

            graphCanvasEntity->CreateComponent<EBusHandlerEventNodeDescriptorComponent>(busName, eventName);

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            AZStd::string decoratedName = AZStd::string::format("%s::%s", busName.c_str(), eventName.c_str());

            GraphCanvas::TranslationKeyedString nodeKeyedString(eventName);
            nodeKeyedString.m_context = TranslationHelper::GetContextName(TranslationContextGroup::EbusHandler, busName);
            nodeKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, busName, eventName, TranslationItemType::Node, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString classKeyedString(busName, nodeKeyedString.m_context);
            classKeyedString.m_key = TranslationHelper::GetClassKey(TranslationContextGroup::EbusHandler, busName, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
            tooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, busName, eventName, TranslationItemType::Node, TranslationKeyId::Tooltip);

            // Set the name
            graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", decoratedName.c_str()));

            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, decoratedName.c_str());
            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, classKeyedString);

            return nodeIdPair;
        }
        
        NodeIdPair CreateEbusWrapperNode(const AZStd::string& busName, const AZ::EntityId& graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, "");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();

            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type());
            graphCanvasEntity->CreateComponent<EBusHandlerNodeDescriptorComponent>(busName);

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity( aznew AZ::Entity );
            scriptCanvasEntity->Init();
            
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type());
            auto* busNode = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(node);
            busNode->InitializeBus(busName);

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::EntityId graphCanvasNodeId = graphCanvasEntity->GetId();

            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::EBusConnectionSlotGroup, GraphCanvas::SlotGroupConfiguration(1));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(2));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::SetDividersEnabled, false);

            AZStd::vector< ScriptCanvas::SlotId > scriptCanvasSlots = busNode->GetNonEventSlotIds();

            for (const auto& slotId : scriptCanvasSlots)
            {
                ScriptCanvas::Slot* slot = busNode->GetSlot(slotId);

                GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid;

                if (slot->GetType() == ScriptCanvas::SlotType::ExecutionIn
                    || slot->GetType() == ScriptCanvas::SlotType::ExecutionOut)
                {
                    group = SlotGroups::EBusConnectionSlotGroup;
                }

                AZ::EntityId gcSlotId = CreateGraphCanvasSlot(graphCanvasNodeId, (*slot), group);

                if (busNode->IsIDRequired() && slot->GetType() == ScriptCanvas::SlotType::DataIn)
                {
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerBusIdNameKey());
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerBusIdTooltipKey());
                }
            }

            AZStd::string decoratedName = AZStd::string::format("%s", busName.c_str());

            GraphCanvas::TranslationKeyedString nodeKeyedString(busName);
            nodeKeyedString.m_context = TranslationHelper::GetContextName(TranslationContextGroup::EbusHandler, busName);
            nodeKeyedString.m_key = TranslationHelper::GetClassKey(TranslationContextGroup::EbusHandler, busName, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
            tooltipKeyedString.m_key = TranslationHelper::GetClassKey(TranslationContextGroup::EbusHandler, busName, TranslationKeyId::Tooltip);

            // Set the name
            //scriptCanvasEntity->SetName(AZStd::string::format("ScriptCanvas Node: %s", decoratedName.c_str()));
            graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", decoratedName.c_str()));

            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, decoratedName.c_str());
            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }

        NodeIdPair CreateGetVariableNode(const AZ::EntityId& variableId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".getVariable");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();

            graphCanvasEntity->CreateComponent<IconComponent>(AZ::Uuid());
            graphCanvasEntity->CreateComponent<GetVariableNodeDescriptorComponent>(variableId);

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::DynamicPropertiesGroup, GraphCanvas::SlotGroupConfiguration(1));

            {
                GraphCanvas::SlotConfiguration slotConfiguration;
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                slotConfiguration.m_name.m_fallback = "Variable";
                slotConfiguration.m_tooltip.m_fallback = "The variable whose properties will be collected.";

                CreateGraphCanvasPropertySlot(nodeIdPair.m_graphCanvasId, PropertySlotIds::GetVariableReference, slotConfiguration);
            }

            return nodeIdPair;
        }

        NodeIdPair CreateSetVariableNode(const AZ::EntityId& variableId, const AZ::EntityId& graphId)
        {
            const AZ::Uuid AssignNodeUuid = ScriptCanvas::Nodes::Core::Assign::RTTI_Type();

            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".setVariable");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();

            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(AssignNodeUuid);
            graphCanvasEntity->CreateComponent<SetVariableNodeDescriptorComponent>(variableId);

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, AssignNodeUuid);

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            // Create the GraphCanvas slots
            for (const auto& slot : node->GetSlots())
            {
                if (slot.GetType() == ScriptCanvas::SlotType::ExecutionIn
                    || slot.GetType() == ScriptCanvas::SlotType::ExecutionOut)
                {
                    CreateGraphCanvasSlot(nodeIdPair.m_graphCanvasId, slot);
                }
            }


            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(1));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::DynamicPropertiesGroup, GraphCanvas::SlotGroupConfiguration(2));

            {
                GraphCanvas::SlotConfiguration slotConfiguration;
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                slotConfiguration.m_name.m_fallback = "Variable";
                slotConfiguration.m_tooltip.m_fallback = "The variable whose properties will be collected.";

                CreateGraphCanvasPropertySlot(nodeIdPair.m_graphCanvasId, PropertySlotIds::SetVariableReference, slotConfiguration);
            }

            // Set the name
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(AssignNodeUuid);
            if (classData)
            {
                const char* name = (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name;
                scriptCanvasEntity->SetName(AZStd::string::format("ScriptCanvas Node: %s", name));
                graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", name));
                GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, name);
                GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTitle, name);
            }

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }

        NodeIdPair CreateVariablePrimitiveNode(const AZ::Uuid& primitiveId, AZ::EntityId graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;
            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".object");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();

            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
            graphCanvasEntity->CreateComponent<VariableNodeDescriptorComponent>();

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::Uuid nodeType;
            
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, primitiveId);            

            GraphCanvas::SlotLayoutRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::VariableSourceGroup, GraphCanvas::SlotGroupConfiguration(1));
            GraphCanvas::SlotLayoutRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::DynamicPropertiesGroup, GraphCanvas::SlotGroupConfiguration(2));

            {
                GraphCanvas::SlotConfiguration slotConfiguration;
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                slotConfiguration.m_name.m_fallback = "Display Name";
                slotConfiguration.m_tooltip.m_fallback = "The display name for the variable.";

                CreateGraphCanvasPropertySlot(nodeIdPair.m_graphCanvasId, PropertySlotIds::VariableName, slotConfiguration);
            }

            {
                GraphCanvas::SlotConfiguration slotConfiguration;
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                slotConfiguration.m_slotGroup = GraphCanvas::SlotGroups::VariableSourceGroup;
                slotConfiguration.m_name.m_fallback = "Default Value";
                slotConfiguration.m_tooltip.m_fallback = "The default value for the variable.";

                CreateGraphCanvasPropertySlot(nodeIdPair.m_graphCanvasId, PropertySlotIds::DefaultValue, slotConfiguration);
            }

            for (const auto& slot : node->GetSlots())
            {
                if (slot.GetType() == ScriptCanvas::SlotType::DataOut)
                {
                    CreateGraphCanvasSlot(nodeIdPair.m_graphCanvasId, slot, GraphCanvas::SlotGroups::VariableSourceGroup);
                }
            }


            // Set the name
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(primitiveId);
            if (classData)
            {
                const char* name = (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name;
                scriptCanvasEntity->SetName(AZStd::string::format("ScriptCanvas Node: %s", name));
                graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", name));
                GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, name);
                GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetSubTitle, name);

                const char* displayTooltip;

                // TODO: move to and consolidate these in the translation file
                if (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Logic::Boolean>())
                {
                    displayTooltip = "A variable that stores one of two values: True or False";
                }
                else if (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Math::Number>())
                {
                    displayTooltip = "A variable that stores a number";
                }
                else if (classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::String>())
                {
                    displayTooltip = "A variable that stores a text string";
                }

                GraphCanvas::NodeRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeRequests::SetTooltip, displayTooltip);
            }

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }

        NodeIdPair CreateVariableObjectNode(const AZStd::string& className, AZ::EntityId graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;
            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".object");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();

            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
            graphCanvasEntity->CreateComponent<VariableNodeDescriptorComponent>();

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            GraphCanvas::TranslationKeyedString nodeKeyedString(className);
            nodeKeyedString.m_context = TranslationHelper::GetContextName(TranslationContextGroup::ClassMethod, className);
            nodeKeyedString.m_key = TranslationHelper::GetClassKey(TranslationContextGroup::ClassMethod, className, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
            tooltipKeyedString.m_key = TranslationHelper::GetClassKey(TranslationContextGroup::ClassMethod, className, TranslationKeyId::Tooltip);

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::Uuid nodeType;

            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::RTTI_Type());
            auto* objectNode = azrtti_cast<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode*>(node);
            objectNode->InitializeObject(className);
            
            GraphCanvas::SlotLayoutRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::VariableSourceGroup, GraphCanvas::SlotGroupConfiguration(1));
            GraphCanvas::SlotLayoutRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::DynamicPropertiesGroup, GraphCanvas::SlotGroupConfiguration(2));

            {
                GraphCanvas::SlotConfiguration slotConfiguration;
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                slotConfiguration.m_name.m_fallback = "Display Name";
                slotConfiguration.m_tooltip.m_fallback = "The display name for the variable.";

                CreateGraphCanvasPropertySlot(nodeIdPair.m_graphCanvasId, PropertySlotIds::VariableName, slotConfiguration);
            }

            {
                GraphCanvas::SlotConfiguration slotConfiguration;
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                slotConfiguration.m_slotGroup = GraphCanvas::SlotGroups::VariableSourceGroup;
                slotConfiguration.m_name.m_fallback = "Default Value";
                slotConfiguration.m_tooltip.m_fallback = "The default value for the variable.";

                CreateGraphCanvasPropertySlot(nodeIdPair.m_graphCanvasId, PropertySlotIds::DefaultValue, slotConfiguration);
            }

            for (const auto& slot : objectNode->GetSlots())
            {
                if (slot.GetType() == ScriptCanvas::SlotType::DataOut)
                {
                    CreateGraphCanvasSlot(nodeIdPair.m_graphCanvasId, slot, GraphCanvas::SlotGroups::VariableSourceGroup);
                }
            }

            // Set the name
            AZStd::string titleName = ScriptCanvas::Data::GetBehaviorContextName(AZ::BehaviorContextHelper::GetClassType(className));

            graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", titleName.c_str()));

            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, titleName);
            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTitle, titleName);
            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, nodeKeyedString);

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }

        NodeIdPair CreateObjectMethodNode(const AZStd::string& className, const AZStd::string& methodName, AZ::EntityId graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Entity* graphCanvasEntity{};
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".method");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
            nodeIdPair.m_graphCanvasId = graphCanvasEntity->GetId();
            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::Method::RTTI_Type());

            ScriptCanvas::Node* node{};
            AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), graphId, ScriptCanvas::Nodes::Core::Method::RTTI_Type());
            auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node);
            ScriptCanvas::Namespaces emptyNamespaces;
            methodNode->InitializeClassOrBus(emptyNamespaces, className, methodName);

            TranslationContextGroup contextGroup = TranslationContextGroup::Invalid;

            switch (methodNode->GetMethodType())
            {
            case ScriptCanvas::Nodes::Core::MethodType::Event:
                graphCanvasEntity->CreateComponent<EBusSenderNodeDescriptorComponent>(className, methodName);
                contextGroup = TranslationContextGroup::EbusSender;
                break;
            case ScriptCanvas::Nodes::Core::MethodType::Member:
                graphCanvasEntity->CreateComponent<ClassMethodNodeDescriptorComponent>(className, methodName);
                contextGroup = TranslationContextGroup::ClassMethod;
                break;
            default:
                // Unsupported?
                AZ_Warning("NodeUtils", false, "Invalid node type?");
            }

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            GraphCanvas::TranslationKeyedString nodeKeyedString(methodName);
            nodeKeyedString.m_context = TranslationHelper::GetContextName(contextGroup, className);
            nodeKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, TranslationItemType::Node, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString classKeyedString(className, nodeKeyedString.m_context);
            classKeyedString.m_key = TranslationHelper::GetClassKey(contextGroup, className, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
            tooltipKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, TranslationItemType::Node, TranslationKeyId::Tooltip);

            int offset = methodNode->HasBusID() ? 1 : 0;
            int paramIndex = 0;
            int outputIndex = 0;

            for (const auto& slot : node->GetSlots())
            {
                AZ::EntityId graphCanvasSlotId = CreateGraphCanvasSlot(nodeIdPair.m_graphCanvasId, slot);

                GraphCanvas::TranslationKeyedString slotNameKeyedString(slot.GetName(), nodeKeyedString.m_context);
                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);

                const int index = methodNode->HasResult() ? 1 : 0;
                if (methodNode->HasBusID() && slot.GetIndex() == index && slot.GetType() == ScriptCanvas::SlotType::DataIn)
                {
                    slotNameKeyedString = TranslationHelper::GetEBusSenderBusIdNameKey();
                    slotTooltipKeyedString = TranslationHelper::GetEBusSenderBusIdTooltipKey();
                }
                else
                {
                    TranslationItemType itemType = TranslationHelper::GetItemType(slot.GetType());

                    int & index = (itemType == TranslationItemType::ParamDataSlot) ? paramIndex : outputIndex;

                    slotNameKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, itemType, TranslationKeyId::Name, index);
                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, itemType, TranslationKeyId::Tooltip, index);

                    if ( (itemType == TranslationItemType::ParamDataSlot) || (itemType == TranslationItemType::ReturnDataSlot))
                    {
                        index++;
                    }
                }

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
            }

            // Set the name
            AZStd::string displayName = methodNode->GetName();

            scriptCanvasEntity->SetName(AZStd::string::format("ScriptCanvas Node: %s", displayName.c_str()));
            graphCanvasEntity->SetName(AZStd::string::format("GraphCanvas Node: %s", displayName.c_str()));

            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetName, displayName);
            GraphCanvas::NodeRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, classKeyedString);

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData{};
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = nodeIdPair.m_scriptCanvasId;
            }

            return nodeIdPair;
        }

        NodeIdPair CreateObjectOrObjectMethodNode(const AZStd::string& className, const AZStd::string& methodName, AZ::EntityId graphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            return methodName == "class" ? CreateVariableObjectNode(className, graphId) : CreateObjectMethodNode(className, methodName, graphId);
        }

        AZ::EntityId CreateGraphCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::Slot& slot, GraphCanvas::SlotGroup slotGroup)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            AZ::Entity* slotEntity = nullptr;
            const AZ::Uuid typeId = ScriptCanvas::Data::ToAZType(slot.GetDataType());
            
            GraphCanvas::SlotConfiguration slotConfiguration;
            slotConfiguration.m_name = slot.GetName();
            slotConfiguration.m_tooltip = slot.GetToolTip();
            slotConfiguration.m_slotGroup = slotGroup;

            switch (slot.GetType())
            {
            case ScriptCanvas::SlotType::ExecutionIn:
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateExecutionSlot, graphCanvasNodeId, slotConfiguration);
                break;
            case ScriptCanvas::SlotType::ExecutionOut:
                slotConfiguration.m_connectionType = GraphCanvas::CT_Output;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateExecutionSlot, graphCanvasNodeId, slotConfiguration);
                break;
            case ScriptCanvas::SlotType::DataIn:
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateDataSlot, graphCanvasNodeId, typeId, slotConfiguration);
                break;
            case ScriptCanvas::SlotType::DataOut:
                slotConfiguration.m_connectionType = GraphCanvas::CT_Output;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateDataSlot, graphCanvasNodeId, typeId, slotConfiguration);
                break;
            }

            RegisterAndActivateGraphCanvasSlot(graphCanvasNodeId, slot.GetId(), slotEntity);

            return slotEntity ? slotEntity->GetId() : AZ::EntityId();
        }

        AZ::EntityId CreateForcedDataTypeGraphCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::Slot& slot, const AZ::Uuid& dataType, GraphCanvas::SlotGroup slotGroup)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            if (slot.GetType() == ScriptCanvas::SlotType::ExecutionIn
                || slot.GetType() == ScriptCanvas::SlotType::ExecutionOut)
            {
                AZ_Warning("Script Canvas", false, "Trying to force a data type onto an execution pin.");
                return CreateGraphCanvasSlot(graphCanvasNodeId, slot, slotGroup);
            }

            AZ::Entity* slotEntity = nullptr;

            GraphCanvas::SlotConfiguration slotConfiguration;
            slotConfiguration.m_name = slot.GetName();
            slotConfiguration.m_tooltip = slot.GetToolTip();
            slotConfiguration.m_slotGroup = slotGroup;

            switch (slot.GetType())
            {
            case ScriptCanvas::SlotType::DataIn:
            {
                slotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateDataSlot, graphCanvasNodeId, dataType, slotConfiguration);
                break;
            }
            case ScriptCanvas::SlotType::DataOut:
            {
                slotConfiguration.m_connectionType = GraphCanvas::CT_Output;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateDataSlot, graphCanvasNodeId, dataType, slotConfiguration);
                break;
            }
            default:
            {
                AZ_Warning("Script Canvas", false, "Trying to force a data type onto a non-data slot.");
                return CreateGraphCanvasSlot(graphCanvasNodeId, slot, slotGroup);
            }
            }

            RegisterAndActivateGraphCanvasSlot(graphCanvasNodeId, slot.GetId(), slotEntity);
            return slotEntity ? slotEntity->GetId() : AZ::EntityId();
        }

        AZ::EntityId CreateGraphCanvasPropertySlot(const AZ::EntityId& graphCanvasNodeId, const AZ::Crc32& propertyId, const GraphCanvas::SlotConfiguration& slotConfiguration)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            AZ::Entity* slotEntity = nullptr;

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreatePropertySlot, graphCanvasNodeId, propertyId, slotConfiguration);

            if (slotEntity)
            {
                slotEntity->Init();
                slotEntity->Activate();

                GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
            }

            return slotEntity ? slotEntity->GetId() : AZ::EntityId();
        }

        AZ::EntityId CreateGraphCanvasVariableSourceSlot(const AZ::EntityId& graphCanvasNodeId, const AZ::Uuid& typeId, const GraphCanvas::SlotConfiguration& slotConfiguration)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            AZ::Entity* slotEntity = nullptr;

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateVariableSourceSlot, graphCanvasNodeId, typeId, slotConfiguration);

            if (slotEntity)
            {
                slotEntity->Init();
                slotEntity->Activate();

                GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
            }

            return slotEntity ? slotEntity->GetId() : AZ::EntityId();
        }
    } // namespace Nodes
} // namespace ScriptCanvasEditor
