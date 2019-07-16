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

#include <AzCore/Asset/AssetManager.h>
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
#include <Editor/GraphCanvas/Components/DynamicSlotComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ClassMethodNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/ScriptEventSenderNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusSenderNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EntityRefNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/GetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/SetVariableNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/UserDefinedNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/Components/MappingComponent.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/TranslationTypes.h>

#include <Libraries/Core/Assign.h>
#include <Libraries/Core/BehaviorContextObjectNode.h>
#include <Libraries/Core/EBusEventHandler.h>
#include <Libraries/Core/GetVariable.h>
#include <Libraries/Core/Method.h>
#include <Libraries/Core/ReceiveScriptEvent.h>
#include <Libraries/Core/SendScriptEvent.h>
#include <Libraries/Core/SetVariable.h>
#include <Libraries/Entity/EntityRef.h>

// Primitive data types we are hard coding for now
#include <Libraries/Core/String.h>
#include <Libraries/Logic/Boolean.h>
#include <Libraries/Math/Number.h>

#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/PureData.h>

namespace
{
    void CopyTranslationKeyedNameToDatumLabel(const AZ::EntityId& graphCanvasNodeId,
        ScriptCanvas::SlotId scSlotId,
        const AZ::EntityId& graphCanvasSlotId)
    {
        GraphCanvas::TranslationKeyedString name;
        GraphCanvas::SlotRequestBus::EventResult(name, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetTranslationKeyedName);
        if (name.GetDisplayString().empty())
        {
            return;
        }

        // GC node -> SC node.
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scNodeEntityId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        if (scNodeEntityId.IsValid())
        {
            ScriptCanvas::Datum* datum = nullptr;
            ScriptCanvas::EditorNodeRequestBus::EventResult(datum, scNodeEntityId, &ScriptCanvas::EditorNodeRequests::ModInput, scSlotId);
            if(datum)
            {
                // Only input data slots will get this far.
                datum->SetLabel(name.GetDisplayString());
            }
        }
    }

    void CopyTranslationKeyedNameToDatumLabel(ScriptCanvas::PureData* node,
                                                const GraphCanvas::TranslationKeyedString& name)
    {
        ScriptCanvas::SlotId slotId = node->GetSlotId(ScriptCanvas::PureData::k_setThis);
        if (!slotId.IsValid())
        {
            return;
        }

        ScriptCanvas::Datum* datum = nullptr;
        ScriptCanvas::EditorNodeRequestBus::EventResult(datum, node->GetEntityId(), &ScriptCanvas::EditorNodeRequests::ModInput, slotId);
        if (!datum)
        {
            return;
        }

        datum->SetLabel(name.GetDisplayString());
    }
}

namespace ScriptCanvasEditor
{
    namespace Nodes
    {

        void CopySlotTranslationKeyedNamesToDatums(AZ::EntityId graphCanvasNodeId)
        {
            AZStd::vector<AZ::EntityId> graphCanvasSlotIds;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetSlotIds);
            for (AZ::EntityId graphCanvasSlotId : graphCanvasSlotIds)
            {
                AZStd::any* slotUserData{};
                GraphCanvas::SlotRequestBus::EventResult(slotUserData, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetUserData);

                if (auto scriptCanvasSlotId = AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData))
                {
                    CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, *scriptCanvasSlotId, graphCanvasSlotId);
                }
            }
        }
        //////////////////////
        // NodeConfiguration
        //////////////////////
        static void RegisterAndActivateGraphCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::SlotId& slotId, AZ::Entity* slotEntity)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            if (slotEntity)
            {
                slotEntity->Init();
                slotEntity->Activate();

                // Set the user data on the GraphCanvas slot to be the SlotId of the ScriptCanvas
                AZStd::any* slotUserData = nullptr;
                GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotEntity->GetId(), &GraphCanvas::SlotRequests::GetUserData);

                if (slotUserData)
                {
                    *slotUserData = slotId;
                }

                GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
            }
        }

        AZStd::string GetCategoryName(const AZ::SerializeContext::ClassData& classData)
        {
            if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
                {
                    if (auto data = azrtti_cast<AZ::Edit::AttributeData<const char*>*>(attribute))
                    {
                        return data->Get(nullptr);
                    }
                }
            }

            return {};
        }

        AZStd::string GetContextName(const AZ::SerializeContext::ClassData& classData)
        {
            if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
                {
                    if (auto data = azrtti_cast<AZ::Edit::AttributeData<const char*>*>(attribute))
                    {
                        AZStd::string fullCategoryName = data->Get(nullptr);
                        AZStd::string delimiter = "/";
                        AZStd::vector<AZStd::string> results;
                        AZStd::tokenize(fullCategoryName, delimiter, results);
                        return results.back();
                    }
                }
            }

            return {};
        }

        // Handles the creation of a node through the node configurations for most nodes.
        AZ::EntityId DisplayGeneralScriptCanvasNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Node* node, const NodeConfiguration& nodeConfiguration)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            AZ::Entity* graphCanvasEntity = nullptr;

            if (nodeConfiguration.m_nodeType == NodeType::GeneralNode)
            {
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, nodeConfiguration.m_nodeSubStyle.c_str());
            }
            else if (nodeConfiguration.m_nodeType == NodeType::WrapperNode)
            {
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, nodeConfiguration.m_nodeSubStyle.c_str());
            }

            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

            if (graphCanvasEntity == nullptr)
            {
                return AZ::EntityId();
            }

            for (const AZ::Uuid& componentId : nodeConfiguration.m_customComponents)
            {
                graphCanvasEntity->CreateComponent(componentId);
            }

            // Apply SceneMember remapping if ScriptCanvasId is valid.
            if (nodeConfiguration.m_scriptCanvasId.IsValid())
            {
                graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(nodeConfiguration.m_scriptCanvasId);
                graphCanvasEntity->CreateComponent<SlotMappingComponent>(nodeConfiguration.m_scriptCanvasId);
            }

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasEntity->GetId(), &GraphCanvas::NodeRequests::GetUserData);

            if (graphCanvasUserData)
            {
                *graphCanvasUserData = node->GetEntityId();
            }

            GraphCanvas::TranslationKeyedString nodeKeyedString(nodeConfiguration.m_titleFallback, nodeConfiguration.m_translationContext);
            nodeKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, TranslationItemType::Node, TranslationKeyId::Name);

            AZStd::string nodeName = nodeKeyedString.GetDisplayString();

            int paramIndex = 0;
            int outputIndex = 0;

            // Create the GraphCanvas slots
            for (const auto& slot : node->GetSlots())
            {
                AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasEntity->GetId(), slot);                

                GraphCanvas::TranslationKeyedString slotNameKeyedString(slot.GetName(), nodeKeyedString.m_context);
                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(slot.GetToolTip(), nodeKeyedString.m_context);

                TranslationItemType itemType = TranslationHelper::GetItemType(slot.GetType());

                if (itemType == TranslationItemType::ParamDataSlot || itemType == TranslationItemType::ReturnDataSlot)
                {
                    int& index = (itemType == TranslationItemType::ParamDataSlot) ? paramIndex : outputIndex;

                    slotNameKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, itemType, TranslationKeyId::Name, index);
                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, itemType, TranslationKeyId::Tooltip, index);
                    index++;
                }

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
            }

            GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeConfiguration.m_subtitleFallback, nodeConfiguration.m_translationContext);
            subtitleKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, TranslationItemType::Node, TranslationKeyId::Category);

            graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", nodeKeyedString.GetDisplayString().c_str()));

            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, subtitleKeyedString);

            if (!nodeConfiguration.m_titlePalette.empty())
            {
                GraphCanvas::NodeTitleRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeTitleRequests::SetPaletteOverride, nodeConfiguration.m_titlePalette);
            }

            // Set the name
            GraphCanvas::TranslationKeyedString tooltipKeyedString(nodeConfiguration.m_tooltipFallback, nodeConfiguration.m_translationContext);
            tooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::ClassMethod, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, TranslationItemType::Node, TranslationKeyId::Tooltip);

            GraphCanvas::NodeRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            EditorNodeNotificationBus::Event(node->GetEntityId(), &EditorNodeNotifications::OnGraphCanvasNodeDisplayed, graphCanvasEntity->GetId());

            return graphCanvasEntity->GetId();
        }

        AZ::EntityId DisplayNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Node* node, StyleConfiguration styleConfiguration = StyleConfiguration())
        {
            NodeConfiguration nodeConfiguration;

            nodeConfiguration.PopulateComponentDescriptors<IconComponent, DynamicSlotComponent, UserDefinedNodeDescriptorComponent>();
            nodeConfiguration.m_nodeSubStyle = styleConfiguration.m_nodeSubStyle;
            nodeConfiguration.m_titlePalette = styleConfiguration.m_titlePalette;
            nodeConfiguration.m_scriptCanvasId = node->GetEntityId();

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(azrtti_typeid(node));

            if (classData)
            {
                AZStd::string nodeContext = GetContextName(*classData);
                nodeConfiguration.m_translationContext = TranslationHelper::GetUserDefinedContext(nodeContext);

                nodeConfiguration.m_titleFallback = (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name;
                nodeConfiguration.m_tooltipFallback = (classData->m_editData && classData->m_editData->m_description) ? classData->m_editData->m_description : "";

                GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeContext, nodeConfiguration.m_translationContext);
                subtitleKeyedString.m_key = TranslationHelper::GetUserDefinedNodeKey(nodeContext, nodeConfiguration.m_titleFallback, ScriptCanvasEditor::TranslationKeyId::Category);

                nodeConfiguration.m_subtitleFallback = subtitleKeyedString.GetDisplayString();

                nodeConfiguration.m_translationKeyName = nodeConfiguration.m_titleFallback;
                nodeConfiguration.m_translationKeyContext = nodeContext;

                nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;

                if (classData->m_editData)
                {
                    const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);

                    if (elementData)
                    {
                        if (auto nodeTypeAttribute = elementData->FindAttribute(ScriptCanvas::Attributes::Node::NodeType))
                        {
                            if (auto nodeTypeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<NodeType>*>(nodeTypeAttribute))
                            {
                                nodeConfiguration.m_nodeType = nodeTypeAttributeData->Get(nullptr);
                            }
                        }
                    }
                }
            }

            return DisplayGeneralScriptCanvasNode(graphCanvasGraphId, node, nodeConfiguration);
        }

        AZ::EntityId DisplayEntityNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Entity::EntityRef* entityNode)
        {
            AZ::EntityId graphCanvasNodeId;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".entity");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

            graphCanvasNodeId = graphCanvasEntity->GetId();

            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Entity::EntityRef::RTTI_Type());
            graphCanvasEntity->CreateComponent<EntityRefNodeDescriptorComponent>();
            graphCanvasEntity->CreateComponent<SlotMappingComponent>(entityNode->GetEntityId());
            graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(entityNode->GetEntityId());

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();
                
            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = entityNode->GetEntityId();
            }

            // Create the GraphCanvas slots
            for (const auto& slot : entityNode->GetSlots())
            {
                if (slot.GetType() == ScriptCanvas::SlotType::DataOut)
                {
                    DisplayScriptCanvasSlot(graphCanvasNodeId, slot);
                }
            }
             
            AZ::Entity* sourceEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, entityNode->GetEntityRef());

            if (sourceEntity)
            {
                graphCanvasEntity->SetName(AZStd::string::format("GC-EntityRef(%s)", sourceEntity->GetName().data()));
            }
            else
            {
                graphCanvasEntity->SetName(AZStd::string::format("GC-EntityRef(%s)", entityNode->GetEntityRef().ToString().c_str()));
            }
            
            return graphCanvasNodeId;
        }

        AZ::EntityId DisplayMethodNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::Method* methodNode)
        {
            AZ::EntityId graphCanvasNodeId;
            
            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".method");
            AZ_Error("GraphCanvas", graphCanvasEntity, "Unable to create GraphCanvas Node");

            if (graphCanvasEntity)
            {
                graphCanvasNodeId = graphCanvasEntity->GetId();
            }

            // Add the icon component
            graphCanvasEntity->CreateComponent<DynamicSlotComponent>();
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::Method::RTTI_Type());
            graphCanvasEntity->CreateComponent<SlotMappingComponent>(methodNode->GetEntityId());
            graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(methodNode->GetEntityId());

            TranslationContextGroup contextGroup = TranslationContextGroup::Invalid;

            switch (methodNode->GetMethodType())
            {
            case ScriptCanvas::Nodes::Core::MethodType::Event:
                graphCanvasEntity->CreateComponent<EBusSenderNodeDescriptorComponent>();
                contextGroup = TranslationContextGroup::EbusSender;
                break;
            case ScriptCanvas::Nodes::Core::MethodType::Member:
                graphCanvasEntity->CreateComponent<ClassMethodNodeDescriptorComponent>();
                contextGroup = TranslationContextGroup::ClassMethod;
                break;
            default:
                // Unsupported?
                AZ_Warning("NodeUtils", false, "Invalid node type?");
            }

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();
            
            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (graphCanvasUserData)
            {
                *graphCanvasUserData = methodNode->GetEntityId();
            }

            const AZStd::string& className = methodNode->GetMethodClassName();
            const AZStd::string& methodName = methodNode->GetName();

            AZStd::string translationContext = TranslationHelper::GetContextName(contextGroup, className);

            GraphCanvas::TranslationKeyedString nodeKeyedString(methodName, translationContext);
            nodeKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, TranslationItemType::Node, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString classKeyedString(className, translationContext);
            classKeyedString.m_key = TranslationHelper::GetClassKey(contextGroup, className, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), translationContext);
            tooltipKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, TranslationItemType::Node, TranslationKeyId::Tooltip);

            int offset = methodNode->HasBusID() ? 1 : 0;
            int paramIndex = 0;
            int outputIndex = 0;

            auto busId = methodNode->GetBusSlotId();
            for (const auto& slot : methodNode->GetSlots())
            {
                AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, slot);

                GraphCanvas::TranslationKeyedString slotNameKeyedString(slot.GetName(), translationContext);
                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(slot.GetToolTip(), translationContext);

                if (methodNode->HasBusID() && busId == slot.GetId() && slot.GetType() == ScriptCanvas::SlotType::DataIn)
                {
                    slotNameKeyedString = TranslationHelper::GetEBusSenderBusIdNameKey();
                    slotTooltipKeyedString = TranslationHelper::GetEBusSenderBusIdTooltipKey();
                }
                else
                {
                    TranslationItemType itemType = TranslationHelper::GetItemType(slot.GetType());

                    int& index = (itemType == TranslationItemType::ParamDataSlot) ? paramIndex : outputIndex;

                    slotNameKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, itemType, TranslationKeyId::Name, index);
                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, itemType, TranslationKeyId::Tooltip, index);

                    if ((itemType == TranslationItemType::ParamDataSlot) || (itemType == TranslationItemType::ReturnDataSlot))
                    {
                        index++;
                    }
                }

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);

                CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), graphCanvasSlotId);
            }

            // Set the name
            AZStd::string displayName = methodNode->GetName();
            graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", displayName.c_str()));

            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, classKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "MethodNodeTitlePalette");

            return graphCanvasNodeId;
        }
        
        AZ::EntityId DisplayEbusWrapperNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::EBusEventHandler* busNode)
        {
            AZ::Entity* graphCanvasEntity = nullptr;

            AZStd::string busName = busNode->GetEBusName();

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, "");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

            AZ::EntityId graphCanvasNodeId = graphCanvasEntity->GetId();

            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type());
            graphCanvasEntity->CreateComponent<EBusHandlerNodeDescriptorComponent>(busName);
            graphCanvasEntity->CreateComponent<SlotMappingComponent>(busNode->GetEntityId());
            graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(busNode->GetEntityId());
            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = busNode->GetEntityId();
            }

            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::EBusConnectionSlotGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(1));
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

                AZ::EntityId gcSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, (*slot), group);

                if (busNode->IsIDRequired() && slot->GetType() == ScriptCanvas::SlotType::DataIn)
                {
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerBusIdNameKey());
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerBusIdTooltipKey());
                }
            }

            GraphCanvas::TranslationKeyedString nodeKeyedString(busName);
            nodeKeyedString.m_context = TranslationHelper::GetEbusHandlerContext(busName);
            nodeKeyedString.m_key = TranslationHelper::GetEbusHandlerKey(busName, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
            tooltipKeyedString.m_key = TranslationHelper::GetEbusHandlerKey(busName, TranslationKeyId::Tooltip);

            // Set the name
            graphCanvasEntity->SetName(AZStd::string::format("GC-BusNode: %s", busName.data()));

            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetDefaultPalette, "HandlerWrapperNodeTitlePalette");

            return graphCanvasNodeId;
        }

        AZ::EntityId DisplayEbusEventNode(const AZ::EntityId& graphCanvasGraphId, const AZStd::string& busName, const AZStd::string& eventName, const ScriptCanvas::EBusEventId& eventId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            AZ::EntityId graphCanvasNodeId;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".handler");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
            graphCanvasNodeId = graphCanvasEntity->GetId();

            graphCanvasEntity->CreateComponent<EBusHandlerEventNodeDescriptorComponent>(busName, eventName, eventId);
            graphCanvasEntity->CreateComponent<SlotMappingComponent>();

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            AZStd::string decoratedName = AZStd::string::format("%s::%s", busName.c_str(), eventName.c_str());

            GraphCanvas::TranslationKeyedString nodeKeyedString(eventName);
            nodeKeyedString.m_context = TranslationHelper::GetEbusHandlerContext(busName);
            nodeKeyedString.m_key = TranslationHelper::GetEbusHandlerEventKey(busName, eventName, TranslationKeyId::Name);

            GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
            tooltipKeyedString.m_key = TranslationHelper::GetEbusHandlerEventKey(busName, eventName, TranslationKeyId::Tooltip);

            // Set the name
            graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", decoratedName.c_str()));

            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "HandlerNodeTitlePalette");

            return graphCanvasNodeId;
        }
        
        AZ::EntityId DisplayScriptEventWrapperNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::ReceiveScriptEvent* busNode)
        {
            AZ::Entity* graphCanvasEntity = nullptr;

            const AZ::Data::AssetId assetId = busNode->GetAssetId();

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, "");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

            AZ::EntityId graphCanvasNodeId = graphCanvasEntity->GetId();

            // Add the icon component
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::ReceiveScriptEvent::RTTI_Type());
            graphCanvasEntity->CreateComponent<ScriptEventReceiverNodeDescriptorComponent>(assetId);
            graphCanvasEntity->CreateComponent<SlotMappingComponent>(busNode->GetEntityId());
            graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(busNode->GetEntityId());
            graphCanvasEntity->CreateComponent<DynamicSlotComponent>();
            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId);
            if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
            {
                AZ_Error("Script Canvas", false, "Script Event asset (%s) is not loaded, unable to display event node.", assetId.ToString<AZStd::string>().c_str());

                GraphCanvas::TranslationKeyedString errorTitle("ERROR!");
                GraphCanvas::TranslationKeyedString errorSubstring("Missing Script Event Asset!");

                GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, errorTitle);
                GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, errorSubstring);

                return graphCanvasNodeId;
            }

            const ScriptEvents::ScriptEvent& definition = asset.Get()->m_definition;

            AZStd::string busName = definition.GetName();

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
            if (graphCanvasUserData)
            {
                *graphCanvasUserData = busNode->GetEntityId();
            }

            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::EBusConnectionSlotGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(1));
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

                AZ::EntityId gcSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, (*slot), group);

                if (busNode->IsIDRequired() && slot->GetType() == ScriptCanvas::SlotType::DataIn)
                {
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerBusIdNameKey());
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerBusIdTooltipKey());
                }
            }

            // Set the name
            graphCanvasEntity->SetName(AZStd::string::format("GC-BusNode: %s", busName.data()));
            
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetDefaultPalette, "HandlerWrapperNodeTitlePalette");

            return graphCanvasNodeId;
        }

        AZ::EntityId DisplayScriptEventNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            AZ::EntityId graphCanvasNodeId;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".handler");
            AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
            graphCanvasNodeId = graphCanvasEntity->GetId();

            graphCanvasEntity->CreateComponent<ScriptEventReceiverEventNodeDescriptorComponent>(assetId, methodDefinition);
            graphCanvasEntity->CreateComponent<SlotMappingComponent>();
            graphCanvasEntity->CreateComponent<DynamicSlotComponent>();

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            AZStd::string eventName = methodDefinition.GetName();

            const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId);

            const AZStd::string& busName = asset.Get()->m_definition.GetName();
            AZStd::string decoratedName = AZStd::string::format("%s::%s", busName.data(), eventName.c_str());

            // Set the name
            graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", decoratedName.c_str()));

            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "HandlerNodeTitlePalette");

            return graphCanvasNodeId;
        }

        AZ::EntityId DisplayScriptEventSenderNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::SendScriptEvent* senderNode)
        {
            AZ::EntityId graphCanvasNodeId;

            AZ::Entity* graphCanvasEntity = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".method");
            AZ_Error("GraphCanvas", graphCanvasEntity, "Unable to create GraphCanvas Node");

            if (graphCanvasEntity)
            {
                graphCanvasNodeId = graphCanvasEntity->GetId();
            }

            // Add the icon component
            graphCanvasEntity->CreateComponent<DynamicSlotComponent>();
            graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::Method::RTTI_Type());
            graphCanvasEntity->CreateComponent<SlotMappingComponent>(senderNode->GetEntityId());
            graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(senderNode->GetEntityId());

            TranslationContextGroup contextGroup = TranslationContextGroup::Invalid;

            graphCanvasEntity->CreateComponent<ScriptEventSenderNodeDescriptorComponent>(senderNode->GetAssetId(), senderNode->GetEventId());
            contextGroup = TranslationContextGroup::EbusSender;

            graphCanvasEntity->Init();
            graphCanvasEntity->Activate();

            const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(senderNode->GetAssetId());
            if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
            {
                AZ_Error("Script Canvas", false, "Script Event asset (%s) is not loaded, unable to display event node.", senderNode->GetAssetId().ToString<AZStd::string>().c_str());

                GraphCanvas::TranslationKeyedString errorTitle("ERROR!");
                GraphCanvas::TranslationKeyedString errorSubstring("Missing Script Event Asset!");

                GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, errorTitle);
                GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, errorSubstring);

                return graphCanvasNodeId;
            }

            // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
            AZStd::any* graphCanvasUserData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

            if (graphCanvasUserData)
            {
                *graphCanvasUserData = senderNode->GetEntityId();
            }

            const ScriptEvents::ScriptEvent& definition = senderNode->GetScriptEvent();

            const AZStd::string& className = definition.GetName();
            const AZStd::string& methodName = senderNode->GetEventName();

            int offset = senderNode->HasBusID() ? 1 : 0;
            int paramIndex = 0;
            int outputIndex = 0;

            auto busId = senderNode->GetBusSlotId();
            for (const auto& slot : senderNode->GetSlots())
            {
                AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, slot);

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetName, slot.GetName());
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTooltip, slot.GetToolTip());

                CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), graphCanvasSlotId);
            }

            // Set the name
            AZStd::string displayName = senderNode->GetEventName();
            graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", displayName.c_str()));

            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "MethodNodeTitlePalette");

            return graphCanvasNodeId;
        }

        AZ::EntityId DisplayGetVariableNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::GetVariableNode* variableNode)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            NodeConfiguration nodeConfiguration;
            nodeConfiguration.PopulateComponentDescriptors<IconComponent, DynamicSlotComponent, GetVariableNodeDescriptorComponent>();
            nodeConfiguration.m_nodeSubStyle = ".getVariable";
            nodeConfiguration.m_titlePalette = "GetVariableNodeTitlePalette";
            nodeConfiguration.m_scriptCanvasId = variableNode->GetEntityId();

            // <Translation>
            nodeConfiguration.m_translationContext = TranslationHelper::GetContextName(TranslationContextGroup::ClassMethod, "CORE");

            nodeConfiguration.m_translationKeyContext = "CORE";
            nodeConfiguration.m_translationKeyName = "GETVARIABLE";

            nodeConfiguration.m_titleFallback = "Get Variable";
            nodeConfiguration.m_subtitleFallback = "";
            nodeConfiguration.m_tooltipFallback = "Gets the specified Variable or one of it's properties.";

            nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;
            // </Translation>

            AZ::EntityId graphCanvasNodeId = DisplayGeneralScriptCanvasNode(graphCanvasGraphId, variableNode, nodeConfiguration);

            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(1));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(2));

            return graphCanvasNodeId;
        }

        AZ::EntityId DisplaySetVariableNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::SetVariableNode* variableNode)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);

            NodeConfiguration nodeConfiguration;
            nodeConfiguration.PopulateComponentDescriptors<IconComponent, DynamicSlotComponent, SetVariableNodeDescriptorComponent>();
            nodeConfiguration.m_nodeSubStyle = ".setVariable";
            nodeConfiguration.m_titlePalette = "SetVariableNodeTitlePalette";
            nodeConfiguration.m_scriptCanvasId = variableNode->GetEntityId();

            // <Translation>

            nodeConfiguration.m_translationContext = TranslationHelper::GetContextName(TranslationContextGroup::ClassMethod, "CORE");

            nodeConfiguration.m_translationKeyContext = "CORE";
            nodeConfiguration.m_translationKeyName = "SETVARIABLE";

            nodeConfiguration.m_titleFallback = "Set Variable";
            nodeConfiguration.m_subtitleFallback = "";
            nodeConfiguration.m_tooltipFallback = "Sets the specified Variable.";

            nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;
            // </Translation>

            AZ::EntityId graphCanvasId = DisplayGeneralScriptCanvasNode(graphCanvasGraphId, variableNode, nodeConfiguration);

            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(0));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(1));
            GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(2));

            return graphCanvasId;
        }

        ///////////////////
        // Header Methods
        ///////////////////
        AZ::EntityId DisplayScriptCanvasNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Node* node)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            AZ::EntityId graphCanvasNodeId;

            if (azrtti_istypeof<ScriptCanvas::Nodes::Core::SetVariableNode>(node))
            {
                graphCanvasNodeId = DisplaySetVariableNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::SetVariableNode*>(node));
            }
            else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::GetVariableNode>(node))
            {
                graphCanvasNodeId = DisplayGetVariableNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::GetVariableNode*>(node));
            }
            else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::Method>(node))
            {
                graphCanvasNodeId = DisplayMethodNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::Method*>(node));
            }
            else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::EBusEventHandler>(node))
            {
                graphCanvasNodeId = DisplayEbusWrapperNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::EBusEventHandler*>(node));
            }
            else if (azrtti_istypeof<ScriptCanvas::Nodes::Entity::EntityRef>(node))
            {
                graphCanvasNodeId = DisplayEntityNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Entity::EntityRef*>(node));
            }
            else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(node))
            {
                graphCanvasNodeId = DisplayScriptEventWrapperNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(node));
            }
            else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::SendScriptEvent>(node))
            {
                graphCanvasNodeId = DisplayScriptEventSenderNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::SendScriptEvent*>(node));
            }
            else if (node)
            {
                graphCanvasNodeId = DisplayNode(graphCanvasGraphId, node);
            }

            return graphCanvasNodeId;
        }

        NodeIdPair CreateNode(const AZ::Uuid& classId, AZ::EntityId scriptCanvasGraphId, const StyleConfiguration& styleConfiguration)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            ScriptCanvas::Node* node{};
            AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, classId);
            scriptCanvasEntity->SetName(AZStd::string::format("SC-Node(%s)", scriptCanvasEntity->GetName().data()));

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIdPair.m_graphCanvasId = DisplayNode(graphCanvasGraphId, node, styleConfiguration);

            return nodeIdPair;
        }

        NodeIdPair CreateEntityNode(const AZ::EntityId& sourceId, AZ::EntityId scriptCanvasGraphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
            scriptCanvasEntity->Init();
            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Entity::EntityRef::RTTI_Type());

            auto* entityNode = azrtti_cast<ScriptCanvas::Nodes::Entity::EntityRef*>(node);
            entityNode->SetEntityRef(sourceId);

            // Set the name
            AZ::Entity* sourceEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, sourceId);
            if (sourceEntity)
            {
                scriptCanvasEntity->SetName(AZStd::string::format("SC-EntityRef(%s)", sourceEntity->GetName().data()));                
            }

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIdPair.m_graphCanvasId = DisplayEntityNode(graphCanvasGraphId, entityNode);

            return nodeIdPair;
        }

        NodeIdPair CreateObjectMethodNode(const AZStd::string& className, const AZStd::string& methodName, AZ::EntityId scriptCanvasGraphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIds;

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
            scriptCanvasEntity->Init();
            nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Core::Method::RTTI_Type());
            auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node);

            ScriptCanvas::Namespaces emptyNamespaces;
            methodNode->InitializeClassOrBus(emptyNamespaces, className, methodName);

            AZStd::string displayName = methodNode->GetName();
            scriptCanvasEntity->SetName(AZStd::string::format("SC-Node(%s)", displayName.c_str()));

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIds.m_graphCanvasId = DisplayMethodNode(graphCanvasGraphId, methodNode);

            return nodeIds;
        }

        NodeIdPair CreateEbusWrapperNode(const AZStd::string& busName, const AZ::EntityId& scriptCanvasGraphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            ScriptCanvas::Node* node = nullptr;

            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Node(%s)", busName.c_str()).c_str());
            scriptCanvasEntity->Init();

            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type());
            auto* busNode = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(node);
            busNode->InitializeBus(busName);

            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIdPair.m_graphCanvasId = DisplayEbusWrapperNode(graphCanvasGraphId, busNode);

            return nodeIdPair;
        }

        NodeIdPair CreateScriptEventReceiverNode(const AZ::EntityId& scriptCanvasGraphId, const AZ::Data::AssetId& assetId)
        {
            AZ_Assert(assetId.IsValid(), "CreateScriptEventReceiverNode asset Id must be valid");

            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId);
            if (!asset)
            {
                AZ_Error("GraphCanvas", asset, "Unable to CreateScriptEventReceiverNode, asset %s not found.", assetId.ToString<AZStd::string>().c_str());
                return nodeIdPair;
            }

            ScriptCanvas::Node* node = nullptr;

            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Node(%s)", asset.Get()->m_definition.GetName().data()).c_str());
            scriptCanvasEntity->Init();

            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Core::ReceiveScriptEvent::RTTI_Type());
            auto* busNode = azrtti_cast<ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(node);
            busNode->Initialize(assetId);

            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIdPair.m_graphCanvasId = DisplayScriptEventWrapperNode(graphCanvasGraphId, busNode);

            return nodeIdPair;
        }

        NodeIdPair CreateScriptEventSenderNode(const AZ::EntityId& scriptCanvasGraphId, const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId)
        {
            AZ_Assert(assetId.IsValid(), "CreateScriptEventSenderNode asset Id must be valid");

            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            NodeIdPair nodeIdPair;

            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId);

            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Node(%s)", asset.Get()->m_definition.GetName().data()).c_str());
            scriptCanvasEntity->Init();

            ScriptCanvas::Node* node = nullptr;
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Core::SendScriptEvent::RTTI_Type());
            auto* senderNode = azrtti_cast<ScriptCanvas::Nodes::Core::SendScriptEvent*>(node);

            senderNode->ConfigureNode(assetId, eventId);

            nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIdPair.m_graphCanvasId = DisplayScriptEventSenderNode(graphCanvasGraphId, senderNode);

            return nodeIdPair;
        }

        NodeIdPair CreateGetVariableNode(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            const AZ::Uuid k_VariableNodeTypeId = azrtti_typeid<ScriptCanvas::Nodes::Core::GetVariableNode>();

            NodeIdPair nodeIds;

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
            scriptCanvasEntity->Init();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, k_VariableNodeTypeId);

            ScriptCanvas::Nodes::Core::GetVariableNode* variableNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::GetVariableNode>(scriptCanvasEntity);

            if (variableNode)
            {
                variableNode->SetId(variableId);
            }

            nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIds.m_graphCanvasId = DisplayGetVariableNode(graphCanvasGraphId, variableNode);            

            scriptCanvasEntity->SetName(AZStd::string::format("SC Node(GetVariable)"));

            return nodeIds;
        }

        NodeIdPair CreateSetVariableNode(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            const AZ::Uuid k_VariableNodeTypeId = azrtti_typeid<ScriptCanvas::Nodes::Core::SetVariableNode>();

            NodeIdPair nodeIds;

            ScriptCanvas::Node* node = nullptr;
            AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
            scriptCanvasEntity->Init();
            ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, k_VariableNodeTypeId);

            ScriptCanvas::Nodes::Core::SetVariableNode* variableNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::SetVariableNode>(scriptCanvasEntity);

            if (variableNode)
            {
                variableNode->SetId(variableId);
            }

            nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

            AZ::EntityId graphCanvasGraphId;
            EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

            nodeIds.m_graphCanvasId = DisplaySetVariableNode(graphCanvasGraphId, variableNode);

            scriptCanvasEntity->SetName(AZStd::string::format("SC Node(SetVariable)"));

            return nodeIds;
        }

        AZ::EntityId DisplayScriptCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::Slot& slot, GraphCanvas::SlotGroup slotGroup)
        {
            AZ_PROFILE_TIMER("ScriptCanvas", __FUNCTION__);
            AZ::Entity* slotEntity = nullptr;
            
            AZ::Uuid typeId = ScriptCanvas::Data::ToAZType(slot.GetDataType());

            if (slot.IsExecution())
            {
                GraphCanvas::ExecutionSlotConfiguration executionConfiguration;
                executionConfiguration.m_name = slot.GetName();
                executionConfiguration.m_tooltip = slot.GetToolTip();
                executionConfiguration.m_slotGroup = slotGroup;

                if (slotGroup == GraphCanvas::SlotGroups::Invalid && slot.GetDisplayGroup() != AZ::Crc32())
                {
                    executionConfiguration.m_slotGroup = slot.GetDisplayGroup();
                }

                if (slot.IsInput())
                {
                    executionConfiguration.m_connectionType = GraphCanvas::CT_Input;
                }
                else
                {
                    executionConfiguration.m_connectionType = GraphCanvas::CT_Output;
                }

                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, graphCanvasNodeId, executionConfiguration);
            }
            else if (slot.IsData())
            {
                GraphCanvas::DataSlotConfiguration dataSlotConfiguration;
                dataSlotConfiguration.m_typeId = typeId;
                dataSlotConfiguration.m_dataSlotType = GraphCanvas::DataSlotType::Value;

                dataSlotConfiguration.m_name = slot.GetName();
                dataSlotConfiguration.m_tooltip = slot.GetToolTip();
                dataSlotConfiguration.m_slotGroup = slotGroup;

                if (slotGroup == GraphCanvas::SlotGroups::Invalid && slot.GetDisplayGroup() != AZ::Crc32())
                {
                    dataSlotConfiguration.m_slotGroup = slot.GetDisplayGroup();
                }

                if (slot.IsInput())
                {
                    dataSlotConfiguration.m_connectionType = GraphCanvas::CT_Input;
                }
                else
                {
                    dataSlotConfiguration.m_connectionType = GraphCanvas::CT_Output;
                }

                if (ScriptCanvas::Data::IsContainerType(typeId))
                {
                    dataSlotConfiguration.m_dataSlotType = GraphCanvas::DataSlotType::Container;
                    dataSlotConfiguration.m_containerTypeIds = ScriptCanvas::Data::GetContainedTypes(typeId);
                }

                switch (slot.GetDynamicDataType())
                {
                case ScriptCanvas::DynamicDataType::Container:
                    dataSlotConfiguration.m_dataSlotType = GraphCanvas::DataSlotType::Container;
                    break;
                default:
                    break;
                }

                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, graphCanvasNodeId, dataSlotConfiguration);
            }

            if (slotEntity)
            {
                RegisterAndActivateGraphCanvasSlot(graphCanvasNodeId, slot.GetId(), slotEntity);
                CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), slotEntity->GetId());
                return slotEntity->GetId();
            }
            else
            {
                return AZ::EntityId();
            }
        }

        AZ::EntityId DisplayScriptCanvasPropertySlot(const AZ::EntityId& graphCanvasNodeId, const AZ::Crc32& propertyId, const GraphCanvas::SlotConfiguration& slotConfiguration)
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
    } // namespace Nodes
} // namespace ScriptCanvasEditor
