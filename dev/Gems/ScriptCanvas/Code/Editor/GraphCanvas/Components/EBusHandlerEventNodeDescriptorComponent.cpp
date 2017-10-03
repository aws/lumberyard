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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/Components/SceneBus.h>

#include "ScriptCanvas/Core/NodeBus.h"
#include "ScriptCanvas/Libraries/Core/EBusEventHandler.h"
#include "Editor/GraphCanvas/Components/EBusHandlerEventNodeDescriptorComponent.h"
#include "Editor/Nodes/NodeUtils.h"
#include "Editor/Translation/TranslationHelper.h"

namespace ScriptCanvasEditor
{
    ////////////////////////////////////////////
    // EBusHandlerEventNodeDescriptorComponent
    ////////////////////////////////////////////
    
    void EBusHandlerEventNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EBusHandlerEventNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ->Field("BusName", &EBusHandlerEventNodeDescriptorComponent::m_busName)
                ->Field("EventName", &EBusHandlerEventNodeDescriptorComponent::m_eventName)
            ;
        }
    }

    EBusHandlerEventNodeDescriptorComponent::EBusHandlerEventNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandlerEvent)
    {
    }
    
    EBusHandlerEventNodeDescriptorComponent::EBusHandlerEventNodeDescriptorComponent(const AZStd::string& busName, const AZStd::string& eventName)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandlerEvent)
        , m_busName(busName)
        , m_eventName(eventName)
    {
    }

    void EBusHandlerEventNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        EBusEventNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EBusHandlerEventNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        EBusEventNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    bool EBusHandlerEventNodeDescriptorComponent::IsWrapped() const
    {
        return m_ebusWrapper.m_graphCanvasId.IsValid() && m_ebusWrapper.m_scriptCanvasId.IsValid();
    }

    NodeIdPair EBusHandlerEventNodeDescriptorComponent::GetEbusWrapperNodeId() const
    {
        return m_ebusWrapper;
    }

    AZStd::string EBusHandlerEventNodeDescriptorComponent::GetBusName() const
    {
        return m_busName;
    }

    AZStd::string EBusHandlerEventNodeDescriptorComponent::GetEventName() const
    {
        return m_eventName;
    }

    void EBusHandlerEventNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId&)
    {
        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

        for (const auto& slotId : slotIds)
        {
            GraphCanvas::SlotType slotType;
            GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_None;
                GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

                if (connectionType == GraphCanvas::ConnectionType::CT_Output)
                {
                    GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerOnEventTriggeredNameKey());
                    GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerOnEventTriggeredTooltipKey());

                    break;
                }
            }
        }
    }

    void EBusHandlerEventNodeDescriptorComponent::OnNodeAboutToSerialize(GraphCanvas::SceneSerialization& sceneSerialization)
    {
        AZ::Entity* wrapperEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(wrapperEntity, &AZ::ComponentApplicationRequests::FindEntity, m_ebusWrapper.m_graphCanvasId);

        if (wrapperEntity)
        {
            sceneSerialization.GetSceneData().m_nodes.insert(wrapperEntity);
        }
    }

    void EBusHandlerEventNodeDescriptorComponent::OnNodeWrapped(const AZ::EntityId& wrappingNode)
    {
        bool isValid = false;

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, wrappingNode, &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            AZ_Warning("ScriptCanvas", !m_ebusWrapper.m_graphCanvasId.IsValid() && !m_ebusWrapper.m_scriptCanvasId.IsValid(), "Wrapping the same ebus event node twice without unwrapping it.");
            AZ::EntityId scriptCanvasId = (*AZStd::any_cast<AZ::EntityId>(userData));

            m_ebusWrapper.m_graphCanvasId = wrappingNode;
            m_ebusWrapper.m_scriptCanvasId = scriptCanvasId;

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, scriptCanvasId);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

                if (eventHandler)
                {
                    AZStd::vector< AZ::EntityId > graphCanvasSlotIds;
                    GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                    const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = eventHandler->GetEvents();
                    ScriptCanvas::Nodes::Core::EBusEventEntry myEvent;

                    for (const auto& event : events)
                    {
                        if (event.second.m_eventName.compare(m_eventName) == 0)
                        {
                            myEvent = event.second;
                            break;
                        }
                    }

                    const int numEventSlots = 1;
                    const int numResultSlots = myEvent.m_resultSlotId.IsValid() ?  1 : 0;

                    const int totalSlots = (myEvent.m_numExpectedArguments + numEventSlots + numResultSlots);

                    // Potentially overly simplistic way of detecting if we need to refresh our Slot information.
                    if (totalSlots != graphCanvasSlotIds.size())
                    {
                        // Remove the previous slots
                        for (const auto& graphCanvasSlotId : graphCanvasSlotIds)
                        {
                            GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::RemoveSlot, graphCanvasSlotId);
                        }

                        // Then from a clean slate, fully regenerate all of our slots.
                        ScriptCanvas::Slot* scriptCanvasSlot = eventHandler->GetSlot(myEvent.m_eventSlotId);

                        if (scriptCanvasSlot)
                        {
                            AZ::EntityId slotId = Nodes::CreateGraphCanvasSlot(GetEntityId(), (*scriptCanvasSlot));

                            GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerOnEventTriggeredNameKey());
                            GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerOnEventTriggeredTooltipKey());
                        }

                        //
                        // inputCount and outputCount work because the order of the slots is maintained from the BehaviorContext, if this changes
                        // in the future then we should consider storing the actual offset or key name at that time.
                        //
                        int inputCount = 0;
                        int outputCount = 0;
                        for (const auto& slotId : myEvent.m_parameterSlotIds)
                        {
                            scriptCanvasSlot = eventHandler->GetSlot(slotId);

                            if (scriptCanvasSlot)
                            {
                                AZ::EntityId graphCanvasSlotId = Nodes::CreateGraphCanvasSlot(GetEntityId(), (*scriptCanvasSlot));

                                TranslationItemType itemType = TranslationHelper::GetItemType(scriptCanvasSlot->GetType());
                                AZStd::string context = TranslationHelper::GetContextName(TranslationContextGroup::EbusHandler, m_busName);

                                GraphCanvas::TranslationKeyedString slotNameKeyedString;
                                slotNameKeyedString.m_context = context;
                                GraphCanvas::TranslationKeyedString slotTooltipKeyedString;
                                slotTooltipKeyedString.m_context = context;

                                if (scriptCanvasSlot->GetType() == ScriptCanvas::SlotType::DataOut)
                                {
                                    slotNameKeyedString.m_fallback = scriptCanvasSlot->GetName();
                                    slotNameKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Name, outputCount);

                                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Tooltip, outputCount);
                                    ++outputCount;
                                }
                                else
                                {
                                    slotNameKeyedString.m_fallback = scriptCanvasSlot->GetName();
                                    slotNameKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Name, inputCount);

                                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Tooltip, inputCount);
                                    ++inputCount;
                                }

                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
                            }
                        }

                        if (myEvent.m_resultSlotId.IsValid())
                        {
                            scriptCanvasSlot = eventHandler->GetSlot(myEvent.m_resultSlotId);

                            if (scriptCanvasSlot)
                            {
                                AZ::EntityId graphCanvasSlotId = Nodes::CreateGraphCanvasSlot(GetEntityId(), (*scriptCanvasSlot));

                                TranslationItemType itemType = TranslationHelper::GetItemType(scriptCanvasSlot->GetType());

                                GraphCanvas::TranslationKeyedString slotNameKeyedString(scriptCanvasSlot->GetName());
                                slotNameKeyedString.m_context = TranslationHelper::GetContextName(TranslationContextGroup::EbusHandler, m_busName);
                                slotNameKeyedString.m_key = slotNameKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Name);

                                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(TranslationHelper::GetSafeTypeName(scriptCanvasSlot->GetDataType()), slotNameKeyedString.m_context);
                                slotTooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::EbusHandler, m_busName, m_eventName, itemType, TranslationKeyId::Tooltip);

                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
                            }
                        }
                        
                        GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);
                    }

                    isValid = (totalSlots == graphCanvasSlotIds.size());
                }
            }
        }

        if (!isValid)
        {
            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            if (sceneId.IsValid())
            {
                AZStd::unordered_set<AZ::EntityId> deleteNodes;
                deleteNodes.insert(GetEntityId());

                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, deleteNodes);
            }
        }
    }
}