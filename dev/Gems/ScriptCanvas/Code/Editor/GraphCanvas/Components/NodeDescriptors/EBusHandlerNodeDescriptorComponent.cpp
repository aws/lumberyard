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

#include <qstring.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerNodeDescriptorComponent.h>

#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/EBusHandlerEventNodeDescriptorComponent.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/SlotMappingBus.h>

#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // EBusHandlerNodeDescriptorSaveData
    //////////////////////////////////////

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::EBusHandlerNodeDescriptorSaveData()
        : m_displayConnections(false)
        , m_callback(nullptr)
    {
    }

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::EBusHandlerNodeDescriptorSaveData(EBusHandlerNodeDescriptorComponent* component)
        : m_displayConnections(false)
        , m_callback(component)
    {
    }

    void EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::operator=(const EBusHandlerNodeDescriptorSaveData& other)
    {
        // Purposefully skipping over the callback
        m_displayConnections = other.m_displayConnections;
        m_enabledEvents = other.m_enabledEvents;
    }

    void EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData::OnDisplayConnectionsChanged()
    {
        if (m_callback)
        {
            m_callback->OnDisplayConnectionsChanged();
            SignalDirty();
        }
    }

    ///////////////////////////////////////
    // EBusHandlerNodeDescriptorComponent
    ///////////////////////////////////////

    bool EBusHandlerDescriptorVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 1)
        {
            AZ::Crc32 displayConnectionId = AZ_CRC("DisplayConnections", 0x710635cc);

            EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(displayConnectionId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_displayConnections);
            }

            classElement.RemoveElementByName(displayConnectionId);
            classElement.RemoveElementByName(AZ_CRC("BusName", 0x1bbf25c5));
            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }
    
    void EBusHandlerNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EBusHandlerNodeDescriptorSaveData, GraphCanvas::ComponentSaveData>()
                ->Version(1)
                ->Field("DisplayConnections", &EBusHandlerNodeDescriptorSaveData::m_displayConnections)
                ->Field("EventNames", &EBusHandlerNodeDescriptorSaveData::m_enabledEvents)
            ;
            
            serializeContext->Class<EBusHandlerNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(3, &EBusHandlerDescriptorVersionConverter)
                ->Field("SaveData", &EBusHandlerNodeDescriptorComponent::m_saveData)
                ->Field("BusName", &EBusHandlerNodeDescriptorComponent::m_busName)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EBusHandlerNodeDescriptorSaveData>("SaveData", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EBusHandlerNodeDescriptorSaveData::m_displayConnections, "Display Connection Controls", "Controls whether or not manual connection controls are visible for this node.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EBusHandlerNodeDescriptorSaveData::OnDisplayConnectionsChanged)
                    ;

                editContext->Class<EBusHandlerNodeDescriptorComponent>("Event Handler", "Configuration values for the EBus node.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EBusHandlerNodeDescriptorComponent::m_saveData, "SaveData", "The modifiable information about this comment.")
                ;
            }
        }
    }

    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_saveData(this)
        , m_loadingEvents(false)
    {
    }
    
    EBusHandlerNodeDescriptorComponent::EBusHandlerNodeDescriptorComponent(const AZStd::string& busName)
        : NodeDescriptorComponent(NodeDescriptorType::EBusHandler)
        , m_saveData(this)
        , m_busName(busName)
        , m_loadingEvents(false)
    {
    }

    void EBusHandlerNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();

        EBusHandlerNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::GraphCanvasPropertyBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::WrapperNodeConfigurationRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void EBusHandlerNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
        GraphCanvas::EntitySaveDataRequestBus::Handler::BusDisconnect();
        GraphCanvas::WrapperNodeConfigurationRequestBus::Handler::BusDisconnect();
        GraphCanvas::GraphCanvasPropertyBus::Handler::BusDisconnect();
        GraphCanvas::WrapperNodeNotificationBus::Handler::BusDisconnect();
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        EBusHandlerNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    void EBusHandlerNodeDescriptorComponent::OnNodeActivated()
    {
        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetWrapperType, AZ::Crc32(m_busName.c_str()));
    }

    void EBusHandlerNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& graphCanvasGraphId)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        if (userData && userData->is<AZ::EntityId>())
        {
            m_scriptCanvasId = (*AZStd::any_cast<AZ::EntityId>(userData));
        }

        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::SetActionString, "Add/Remove Events");
        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_saveData.m_displayConnections);

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

    void EBusHandlerNodeDescriptorComponent::OnNodeDeserialized(const AZ::EntityId&, const GraphCanvas::GraphSerialization&)
    {
        m_saveData.m_enabledEvents.clear();
    }

    void EBusHandlerNodeDescriptorComponent::OnMemberSetupComplete()
    {
        m_loadingEvents = true;
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        bool inUndoRedo = false;
        GeneralRequestBus::BroadcastResult(inUndoRedo, &GeneralRequests::IsInUndoRedo, graphCanvasGraphId);

        // If we are in undo/redo we don't want to repopulate ourselves
        // as all the visual information gets recreated through the undo/redo stack.
        if (!inUndoRedo)
        {
            for (const AZStd::string& eventName : m_saveData.m_enabledEvents)
            {
                if (m_eventTypeToId.find(eventName) == m_eventTypeToId.end())
                {
                    AZ::EntityId internalNode = Nodes::DisplayEbusEventNode(graphCanvasGraphId, m_busName, eventName);

                    if (internalNode.IsValid())
                    {
                        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Add, internalNode);

                        GraphCanvas::WrappedNodeConfiguration configuration = GetEventConfiguration(eventName);
                        GraphCanvas::WrapperNodeRequestBus::Event(GetEntityId(), &GraphCanvas::WrapperNodeRequests::WrapNode, internalNode, configuration);
                    }
                }
            }
        }
        m_loadingEvents = false;

        m_saveData.RegisterIds(GetEntityId(), graphCanvasGraphId);
    }

    void EBusHandlerNodeDescriptorComponent::WriteSaveData(GraphCanvas::EntitySaveDataContainer& saveDataContainer) const
    {
        EBusHandlerNodeDescriptorSaveData* saveData = saveDataContainer.FindCreateSaveData<EBusHandlerNodeDescriptorSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void EBusHandlerNodeDescriptorComponent::ReadSaveData(const GraphCanvas::EntitySaveDataContainer& saveDataContainer)
    {
        const EBusHandlerNodeDescriptorSaveData* saveData = saveDataContainer.FindSaveDataAs<EBusHandlerNodeDescriptorSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
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

    GraphCanvas::Endpoint EBusHandlerNodeDescriptorComponent::FindEventNodeEndpoint(const ScriptCanvas::SlotId& scriptCanvasSlotId) const
    {
        GraphCanvas::Endpoint endpoint;

        for (auto& mapPair : m_eventTypeToId)
        {
            AZ::EntityId graphCanvasSlotId;
            SlotMappingRequestBus::EventResult(graphCanvasSlotId, mapPair.second, &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSlotId);

            if (graphCanvasSlotId.IsValid())
            {
                endpoint = GraphCanvas::Endpoint(mapPair.second, graphCanvasSlotId);
                break;
            }
        }

        return endpoint;
    }

    void EBusHandlerNodeDescriptorComponent::OnWrappedNode(const AZ::EntityId& wrappedNode)
    {
        AZStd::string eventName;
        EBusHandlerEventNodeDescriptorRequestBus::EventResult(eventName, wrappedNode, &EBusHandlerEventNodeDescriptorRequests::GetEventName);

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
            
            if (!m_loadingEvents)
            {
                m_saveData.m_enabledEvents.emplace_back(eventName);
                m_saveData.SignalDirty();
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
            AZStd::string eventType = iter->second;

            m_eventTypeToId.erase(eventType);
            m_idToEventType.erase(iter);

            for (auto eventIter = m_saveData.m_enabledEvents.begin(); eventIter != m_saveData.m_enabledEvents.end(); ++eventIter)
            {
                if (eventIter->compare(eventType) == 0)
                {
                    m_saveData.m_enabledEvents.erase(eventIter);
                    m_saveData.SignalDirty();
                    break;
                }
            }
        }
    }

    GraphCanvas::WrappedNodeConfiguration EBusHandlerNodeDescriptorComponent::GetWrappedNodeConfiguration(const AZ::EntityId& wrappedNodeId) const
    {
        AZStd::string eventName;
        EBusHandlerEventNodeDescriptorRequestBus::EventResult(eventName, wrappedNodeId, &EBusHandlerEventNodeDescriptorRequests::GetEventName);

        if (eventName.empty())
        {
            return GraphCanvas::WrappedNodeConfiguration();
        }
        else
        {
            return GetEventConfiguration(eventName);
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
        if (!m_saveData.m_displayConnections)
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
                        m_saveData.m_displayConnections = true;
                        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                    }
                }
            }
        }

        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::SetSlotGroupVisible, SlotGroups::EBusConnectionSlotGroup, m_saveData.m_displayConnections);
    }
}
