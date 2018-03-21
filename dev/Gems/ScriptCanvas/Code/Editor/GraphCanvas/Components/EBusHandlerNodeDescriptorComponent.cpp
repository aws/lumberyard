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
#include <AzCore/Serialization/EditContext.h>

#include <QString>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>

#include "Editor/GraphCanvas/Components/EBusHandlerNodeDescriptorComponent.h"

#include "ScriptCanvas/Libraries/Core/EBusEventHandler.h"
#include "Editor/GraphCanvas/Components/EBusHandlerEventNodeDescriptorComponent.h"
#include "Editor/GraphCanvas/PropertySlotIds.h"
#include "Editor/Translation/TranslationHelper.h"

#include <Editor/View/Widgets/PropertyGridBus.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////////////
    // EBusHandlerNodeDescriptorComponent
    ///////////////////////////////////////
    
    void EBusHandlerNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EBusHandlerNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ->Field("BusName", &EBusHandlerNodeDescriptorComponent::m_busName)
                ->Field("DisplayConnections", &EBusHandlerNodeDescriptorComponent::m_displayConnections)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EBusHandlerNodeDescriptorComponent>("Event Handler", "Configuration values for the EBus node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EBusHandlerNodeDescriptorComponent::m_displayConnections, "Display Connection Controls", "Controls whether or not manual connection controls are visible for this node.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EBusHandlerNodeDescriptorComponent::OnDisplayConnectionsChanged)
                ;
            }
        }
    }

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_displayConnections(false)
    {
    }
    
    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorComponent(const AZStd::string& busName)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_displayConnections(false)
        , m_busName(busName)
    {
    }

    void EBusHandlerNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        EBusNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::GraphCanvasPropertyBus::Handler::BusConnect(GetEntityId());
    }

    void EBusHandlerNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::GraphCanvasPropertyBus::Handler::BusDisconnect();
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusDisconnect();
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        EBusNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    void EBusHandlerNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            m_scriptCanvasId = (*AZStd::any_cast<AZ::EntityId>(userData));
        }

        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetActionString, "Add/Remove Events");
        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_displayConnections);

        if (m_scriptCanvasId.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_scriptCanvasId);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

                if (eventHandler && eventHandler->IsIDRequired())
                {
                    AZStd::vector< AZ::EntityId > slotIds;
                    GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

                    // Should be exactly one data slot on ourselves. And that is the BusId
                    for (const AZ::EntityId& testSlotId : slotIds)
                    {
                        GraphCanvas::SlotType slotType;
                        GraphCanvas::SlotRequestBus::EventResult(slotType, testSlotId, &GraphCanvas::SlotRequests::GetSlotType);

                        if (slotType == GraphCanvas::SlotTypes::DataSlot)
                        {
                            GraphCanvas::SlotRequestBus::Event(testSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerBusIdNameKey());
                            GraphCanvas::SlotRequestBus::Event(testSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerBusIdTooltipKey());
                            break;
                        }
                    }
                }
            }
        }
    }

    AZStd::string EBusHandlerNodeDescriptorComponent::GetBusName() const
    {
        return m_busName;
    }

    GraphCanvas::WrappedNodeConfiguration EBusHandlerNodeDescriptorComponent::GetEventConfiguration(const AZStd::string& eventName) const
    {
        AZ_Warning("ScriptCanvas", m_scriptCanvasId.IsValid(), "Trying to query event list before the node is added to the scene.");

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

        GraphCanvas::WrappedNodeConfiguration wrappedConfiguration;

        if (entity)
        {
            ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

            if (eventHandler)
            {
                const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = eventHandler->GetEvents();

                AZ::u32 i = 0;
                for (const auto& event : events)
                {
                    if (event.second.m_eventName.compare(eventName) == 0)
                    {
                        wrappedConfiguration.m_layoutOrder = i;
                        break;
                    }

                    ++i;
                }
            }
        }

        return wrappedConfiguration;
    }

    bool EBusHandlerNodeDescriptorComponent::ContainsEvent(const AZStd::string& eventName) const
    {
        return m_eventTypeToId.find(eventName) != m_eventTypeToId.end();
    }

    AZStd::vector< AZStd::string > EBusHandlerNodeDescriptorComponent::GetEventNames() const
    {
        AZ_Warning("ScriptCanvas", m_scriptCanvasId.IsValid(), "Trying to query event list before the node is added to the scene.");
        AZStd::vector< AZStd::string > eventNames;

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

        if (entity)
        {
            ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

            if (eventHandler)
            {
                const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = eventHandler->GetEvents();

                for (const auto& eventEntry : events)
                {
                    eventNames.push_back(eventEntry.second.m_eventName);
                }
            }
        }

        return eventNames;
    }

    AZ::EntityId EBusHandlerNodeDescriptorComponent::FindEventNodeId(const AZStd::string& eventName) const
    {
        AZ::EntityId retVal;

        auto iter = m_eventTypeToId.find(eventName);

        if (iter != m_eventTypeToId.end())
        {
            retVal = iter->second;
        }

        return retVal;
    }

    void EBusHandlerNodeDescriptorComponent::OnWrappedNode(const AZ::EntityId& wrappedNode)
    {
        AZStd::string eventName;
        EBusEventNodeDescriptorRequestBus::EventResult(eventName, wrappedNode, &EBusEventNodeDescriptorRequests::GetEventName);

        if (eventName.empty())
        {
            AZ_Warning("ScriptCanvas", false, "Trying to wrap an event node without an event name being specified.");
            return;
        }

        auto emplaceResult = m_eventTypeToId.emplace(eventName, wrappedNode);

        if (emplaceResult.second)
        {
            m_idToEventType.emplace(wrappedNode, eventName);

            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, wrappedNode, &GraphCanvas::NodeRequests::GetUserData);

            if (userData)
            {
                (*userData) = m_scriptCanvasId;

                GraphCanvas::NodeDataSlotRequestBus::Event(wrappedNode, &GraphCanvas::NodeDataSlotRequests::RecreatePropertyDisplay);
            }
        }
        // If we are wrapping the same node twice for just ignore it and log a message
        else if (emplaceResult.first->second != wrappedNode)
        {
            AZ_Error("ScriptCanvas", false, "Trying to wrap two identically named methods under the same EBus Handler. Deleting the second node.");

            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            AZStd::unordered_set<AZ::EntityId> deleteNodes;
            deleteNodes.insert(wrappedNode);
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Delete, deleteNodes);
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "Trying to wrap the same node twice.");
        }
    }

    void EBusHandlerNodeDescriptorComponent::OnUnwrappedNode(const AZ::EntityId& unwrappedNode)
    {
        auto iter = m_idToEventType.find(unwrappedNode);

        if (iter != m_idToEventType.end())
        {
            m_eventTypeToId.erase(iter->second);
            m_idToEventType.erase(iter);
        }
    }

    AZ::Component* EBusHandlerNodeDescriptorComponent::GetPropertyComponent()
    {
        return this;
    }

    void EBusHandlerNodeDescriptorComponent::OnDisplayConnectionsChanged()
    {
        // If we are hiding the connections, we need to confirm that
        // everything will be ok(i.e. no active connections)
        if (!m_displayConnections)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_scriptCanvasId);

            if (entity)
            {
                ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::EBusEventHandler>(entity);

                if (eventHandler)
                {
                    AZStd::vector< ScriptCanvas::SlotId > scriptCanvasSlots = eventHandler->GetNonEventSlotIds();

                    bool allowChange = true;

                    for (const auto& slotId : scriptCanvasSlots)
                    {
                        ScriptCanvas::Slot* slot = eventHandler->GetSlot(slotId);

                        if (eventHandler->IsConnected((*slot)))
                        {
                            allowChange = false;
                            break;
                        }
                    }

                    if (!allowChange)
                    {
                        AZ_Warning("Script Canvas", false, "Cannot hide EBus Connection Controls because one ore more slots are currently connected. Please disconnect all slots to hide.");
                        m_displayConnections = true;
                        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                    }
                }
            }
        }

        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_displayConnections);
    }
}