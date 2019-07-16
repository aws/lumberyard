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

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>

#include <ScriptCanvas/Core/GraphBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/DeprecatedVariableNodeDescriptorComponents.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/PureData.h>

#include <Editor/Nodes/NodeUtils.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>

namespace ScriptCanvasEditor
{
    namespace Deprecated
    {
        ////////////////////////////////////
        // VariableNodeDescriptorComponent
        ////////////////////////////////////

        void VariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

            if (serializeContext)
            {
                serializeContext->Class<VariableNodeDescriptorComponent, NodeDescriptorComponent>()
                    ->Version(1)
                    ->Field("DisplayName", &VariableNodeDescriptorComponent::m_displayName)
                    ;
            }
        }

        VariableNodeDescriptorComponent::VariableNodeDescriptorComponent()
            : NodeDescriptorComponent(NodeDescriptorType::VariableNode)
        {
        }

        void VariableNodeDescriptorComponent::Activate()
        {
            VariableNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
            GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
            GraphCanvas::Deprecated::VariableRequestBus::Handler::BusConnect(GetEntityId());
            GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void VariableNodeDescriptorComponent::Deactivate()
        {
            GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
            GraphCanvas::Deprecated::SceneVariableRequestBus::Handler::BusDisconnect();
            GraphCanvas::Deprecated::VariableRequestBus::Handler::BusDisconnect();
            GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
            VariableNodeDescriptorRequestBus::Handler::BusDisconnect();
        }

        AZ::EntityId VariableNodeDescriptorComponent::GetVariableId() const
        {
            return GetEntityId();
        }

        AZStd::string VariableNodeDescriptorComponent::GetVariableName() const
        {
            return m_displayName;
        }

        AZ::Uuid VariableNodeDescriptorComponent::GetDataType() const
        {
            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::Type::Invalid();
            ScriptCanvas::NodeRequestBus::EventResult(dataType, m_variableSourceEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlotDataType, m_variableSourceEndpoint.GetSlotId());

            if (dataType.IsValid())
            {
                return ScriptCanvas::Data::ToAZType(dataType);
            }
            else
            {
                return AZ::Uuid();
            }
        }

        AZStd::string VariableNodeDescriptorComponent::GetDataTypeDisplayName() const
        {
            AZStd::string displayName;

            GraphCanvas::NodeTitleRequestBus::EventResult(displayName, GetEntityId(), &GraphCanvas::NodeTitleRequests::GetSubTitle);
            return displayName;
        }

        ScriptCanvas::Endpoint VariableNodeDescriptorComponent::GetReadEndpoint() const
        {
            return m_variableSourceEndpoint;
        }

        ScriptCanvas::Endpoint VariableNodeDescriptorComponent::GetWriteEndpoint() const
        {
            return m_variableWriteEndpoint;
        }

        void VariableNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
        {
            GraphCanvas::Deprecated::SceneVariableRequestBus::Handler::BusConnect(sceneId);

            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId variableNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

            ScriptCanvas::SlotId variableSlotId;
            ScriptCanvas::NodeRequestBus::EventResult(variableSlotId, variableNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_getThis);

            m_variableSourceEndpoint = ScriptCanvas::Endpoint(variableNodeId, variableSlotId);

            ScriptCanvas::SlotId variableWriteSlotId;
            ScriptCanvas::NodeRequestBus::EventResult(variableWriteSlotId, variableNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_setThis);

            m_variableWriteEndpoint = ScriptCanvas::Endpoint(variableNodeId, variableWriteSlotId);

            VariableNodeDescriptorNotificationBus::Event(GetEntityId(), &VariableNodeDescriptorNotifications::OnNameChanged);
            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, m_displayName);

            ActivateVariable();
        }

        void VariableNodeDescriptorComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
        {
            GraphCanvas::Deprecated::SceneVariableRequestBus::Handler::BusDisconnect(sceneId);
        }

        void VariableNodeDescriptorComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization&)
        {
            // When we are deserialized(from a paste)
            // We want to clear our display name to generate a new variable name.
            m_displayName.clear();
        }

        void VariableNodeDescriptorComponent::ActivateVariable()
        {
            GraphCanvas::Deprecated::VariableRequestBus::Handler::BusConnect(GetEntityId());

            UpdateTitleDisplay();
        }

        void VariableNodeDescriptorComponent::UpdateTitleDisplay()
        {
            AZ::Uuid dataType;
            GraphCanvas::Deprecated::VariableRequestBus::EventResult(dataType, GetEntityId(), &GraphCanvas::Deprecated::VariableRequests::GetDataType);

            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetDataPaletteOverride, dataType);
        }

        ///////////////////////////////////////
        // GetVariableNodeDescriptorComponent
        ///////////////////////////////////////
        
        void GetVariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
            
            if (serializeContext)
            {
                serializeContext->Class<GetVariableNodeDescriptorComponent, NodeDescriptorComponent>()
                    ->Version(1)
                    ->Field("VariableId", &GetVariableNodeDescriptorComponent::m_variableId)
                ;
            }
        }

        GetVariableNodeDescriptorComponent::GetVariableNodeDescriptorComponent()
            : NodeDescriptorComponent(NodeDescriptorType::GetVariable)
        {
        }

        GetVariableNodeDescriptorComponent::GetVariableNodeDescriptorComponent(const AZ::EntityId& variableId)
            : GetVariableNodeDescriptorComponent()
        {
            m_variableId = variableId;
        }
        
        void GetVariableNodeDescriptorComponent::Activate()
        {
            NodeDescriptorComponent::Activate();

            GetVariableNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
            GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
            GraphCanvas::SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        }
        
        void GetVariableNodeDescriptorComponent::Deactivate()
        {
            NodeDescriptorComponent::Deactivate();

            GraphCanvas::SceneMemberNotificationBus::Handler::BusDisconnect();
            GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
            GetVariableNodeDescriptorRequestBus::Handler::BusDisconnect();
        }
        
        AZ::EntityId GetVariableNodeDescriptorComponent::GetVariableId() const
        {
            return m_variableId;
        }

        void GetVariableNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
        {
            // Less then ideal way of detecting whether or not we are being loaded from a save file.
            AZStd::vector< AZ::EntityId > slotIds;
            GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

            // Checking against 1 here. Since we have a slot that is 
            // always added to us. So we need to check if we have anything else
            // added, and if we do; skip the population step(to avoid
            // recreating things on a load and breaking connection visuals)
            if (slotIds.size() == 1 && m_variableId.IsValid())
            {
                // Do Nothing
            }
            else if (m_variableId.IsValid() && GraphCanvas::Deprecated::VariableRequestBus::FindFirstHandler(m_variableId) != nullptr)
            {
                PopulateExternalSlotIds();
            }

            UpdateTitle();
        }

        void GetVariableNodeDescriptorComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
        {
            ClearPins();
            GetVariableNodeDescriptorRequestBus::Handler::BusDisconnect();
        }

        void GetVariableNodeDescriptorComponent::PopulateExternalSlotIds()
        {
            if (!m_variableId.IsValid())
            {
                return;
            }

            m_externalSlotIds.clear();

            ScriptCanvas::Endpoint variableEndpoint;
            VariableNodeDescriptorRequestBus::EventResult(variableEndpoint, m_variableId, &VariableNodeDescriptorRequests::GetReadEndpoint);

            AZStd::vector< const ScriptCanvas::Slot* > slotList;
            ScriptCanvas::NodeRequestBus::EventResult(slotList, variableEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetAllSlots);

            for (const ScriptCanvas::Slot* slot : slotList)
            {
                if (slot->GetType() == ScriptCanvas::SlotType::ExecutionIn
                    || slot->GetType() == ScriptCanvas::SlotType::ExecutionOut
                    || slot->GetType() == ScriptCanvas::SlotType::DataIn)
                {
                    continue;
                }

                m_externalSlotIds.insert(slot->GetId());
            }
        }

        void GetVariableNodeDescriptorComponent::ClearPins()
        {
            GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::ClearSlotGroup, SlotGroups::DynamicPropertiesGroup);
            m_externalSlotIds.clear();
        }

        void GetVariableNodeDescriptorComponent::UpdateTitle()
        {
            if (!m_variableId.IsValid() || GraphCanvas::Deprecated::VariableRequestBus::FindFirstHandler(m_variableId) != nullptr)
            {
                AZStd::string variableName = "Variable";

                GraphCanvas::Deprecated::VariableRequestBus::EventResult(variableName, m_variableId, &GraphCanvas::Deprecated::VariableRequests::GetVariableName);

                AZStd::string title = AZStd::string::format("Get %s", variableName.c_str());
                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, title);

                if (m_variableId.IsValid())
                {
                    AZStd::string subTitle;
                    GraphCanvas::NodeTitleRequestBus::EventResult(subTitle, m_variableId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, subTitle);

                    AZ::Uuid dataType;
                    GraphCanvas::Deprecated::VariableRequestBus::EventResult(dataType, m_variableId, &GraphCanvas::Deprecated::VariableRequests::GetDataType);

                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetDataPaletteOverride, dataType);

                    AZStd::string tooltip = AZStd::string::format("This node returns %s's values", variableName.c_str());
                    GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, tooltip);
                }
                else
                {
                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::ClearPaletteOverride);
                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, "");

                    GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, "This node returns a variable's values");
                }
            }
        }

        ///////////////////////////////////////
        // SetVariableNodeDescriptorComponent
        ///////////////////////////////////////
        
        void SetVariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
            
            if (serializeContext)
            {
                serializeContext->Class<SetVariableNodeDescriptorComponent, NodeDescriptorComponent>()
                    ->Version(1)
                    ->Field("VariableId", &SetVariableNodeDescriptorComponent::m_variableId)
                ;
            }
        }

        SetVariableNodeDescriptorComponent::SetVariableNodeDescriptorComponent()
            : NodeDescriptorComponent(NodeDescriptorType::SetVariable)
        {
        }

        SetVariableNodeDescriptorComponent::SetVariableNodeDescriptorComponent(const AZ::EntityId& variableId)
            : SetVariableNodeDescriptorComponent()
        {
            m_variableId = variableId;
        }
        
        void SetVariableNodeDescriptorComponent::Activate()
        {
            NodeDescriptorComponent::Activate();

            SetVariableNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
            GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());

        }
        
        void SetVariableNodeDescriptorComponent::Deactivate()
        {
            NodeDescriptorComponent::Deactivate();

            GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
            SetVariableNodeDescriptorRequestBus::Handler::BusDisconnect();
        }

        AZ::EntityId SetVariableNodeDescriptorComponent::GetVariableId() const
        {
            return m_variableId;
        }

        void SetVariableNodeDescriptorComponent::OnConnectedTo(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& targetEndpoint)
        {
            // We don't want to delete the connection if we are dragging off of our pin.
            if (targetEndpoint.IsValid() && !ConnectToEndpoint())
            {
                AZ::EntityId sceneId;
                GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::RemoveConnection, connectionId);
            }
        }

        void SetVariableNodeDescriptorComponent::OnDisconnectedFrom(const AZ::EntityId&, const GraphCanvas::Endpoint&)
        {
            bool isConnected = false;
            GraphCanvas::SlotRequestBus::EventResult(isConnected, m_dataConnectionSlot, &GraphCanvas::SlotRequests::HasConnections);

            if (!isConnected)
            {
                DisconnectFromEndpoint();
            }
        }

        void SetVariableNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

            if (userData && userData->is<AZ::EntityId>())
            {
                AZ::EntityId scriptCanvasNodeId = (*AZStd::any_cast<AZ::EntityId>(userData));

                AZStd::vector< const ScriptCanvas::Slot* > slotList;
                ScriptCanvas::NodeRequestBus::EventResult(slotList, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::GetAllSlots);

                for (const ScriptCanvas::Slot* slot : slotList)
                {
                    if (slot->GetType() == ScriptCanvas::SlotType::DataIn)
                    {
                        m_setEndpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, slot->GetId());
                    }
                    else if (slot->GetType() == ScriptCanvas::SlotType::DataOut)
                    {
                        m_getEndpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, slot->GetId());
                    }
                }
            }

            // Less then ideal way of detecting whether or not we are being loaded from a save file.
            AZStd::vector< AZ::EntityId > slotIds;
            GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

            // Checking against 3 here. Since we have 3 slots that are
            // always added to us(Property Display, Logic In, Logic Out). So we need to check if we have anything else
            // added, and if we do; skip the population step(to avoid
            // recreating things on a load and breaking connection visuals)
            if (slotIds.size() == 3 && m_variableId.IsValid())
            {
                // Do nothing
            }
            else
            {
                for (const AZ::EntityId& slotId : slotIds)
                {
                    GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::Invalid;
                    GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

                    if (slotType == GraphCanvas::SlotTypes::DataSlot)
                    {
                        m_dataConnectionSlot = slotId;
                        break;
                    }
                }

                if (m_dataConnectionSlot.IsValid())
                {
                    GraphCanvas::SlotNotificationBus::Handler::BusConnect(m_dataConnectionSlot);
                }
            }

            UpdateTitle();
        }

        void SetVariableNodeDescriptorComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
        {
            ClearPins();
            SetVariableNodeDescriptorRequestBus::Handler::BusDisconnect();
        }

        void SetVariableNodeDescriptorComponent::DisconnectFromEndpoint()
        {
            if (m_variableId.IsValid())
            {
                AZ::EntityId sceneId;
                GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

                ScriptCanvas::Endpoint writeEndpoint;
                VariableNodeDescriptorRequestBus::EventResult(writeEndpoint, m_variableId, &VariableNodeDescriptorRequests::GetWriteEndpoint);

                ScriptCanvas::GraphRequestBus::Event(sceneId, &ScriptCanvas::GraphRequests::DisconnectByEndpoint, m_getEndpoint, writeEndpoint);
            }
        }

        bool SetVariableNodeDescriptorComponent::ConnectToEndpoint()
        {
            bool connected = false;
            if (m_variableId.IsValid())
            {
                AZ::EntityId sceneId;
                GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

                ScriptCanvas::Endpoint writeEndpoint;
                VariableNodeDescriptorRequestBus::EventResult(writeEndpoint, m_variableId, &VariableNodeDescriptorRequests::GetWriteEndpoint);

                AZ::Entity* connectionEntity = nullptr;

                ScriptCanvas::GraphRequestBus::EventResult(connected, sceneId, &ScriptCanvas::GraphRequests::FindConnection, connectionEntity, m_getEndpoint, writeEndpoint);
                GraphCanvas::SlotRequestBus::EventResult(connected, m_dataConnectionSlot, &GraphCanvas::SlotRequests::HasConnections);

                if (!connected || connectionEntity == nullptr)
                {
                    AZ::Outcome<void, AZStd::string> outcome = AZ::Failure(AZStd::string(""));
                    ScriptCanvas::GraphRequestBus::EventResult(outcome, sceneId, &ScriptCanvas::GraphRequests::CanCreateConnectionBetween, m_getEndpoint, writeEndpoint);

                    if (outcome.IsSuccess())
                    {
                        ScriptCanvas::GraphRequestBus::EventResult(connected, sceneId, &ScriptCanvas::GraphRequests::ConnectByEndpoint, m_getEndpoint, writeEndpoint);
                    }
                    else
                    {
                        connected = false;
                    }
                }
            }

            return connected;
        }

        void SetVariableNodeDescriptorComponent::ClearPins()
        {
            GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::ClearSlotGroup, SlotGroups::DynamicPropertiesGroup);

            GraphCanvas::SlotNotificationBus::Handler::BusDisconnect();
            m_dataConnectionSlot.SetInvalid();
        }

        void SetVariableNodeDescriptorComponent::UpdateTitle()
        {
            if (!m_variableId.IsValid() || GraphCanvas::Deprecated::VariableRequestBus::FindFirstHandler(m_variableId) != nullptr)
            {
                AZStd::string variableName = "Variable";

                GraphCanvas::Deprecated::VariableRequestBus::EventResult(variableName, m_variableId, &GraphCanvas::Deprecated::VariableRequests::GetVariableName);

                AZStd::string title = AZStd::string::format("Set %s", variableName.c_str());
                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, title);

                if (m_variableId.IsValid())
                {
                    AZStd::string subTitle;
                    GraphCanvas::NodeTitleRequestBus::EventResult(subTitle, m_variableId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, subTitle);

                    AZ::Uuid dataType;
                    GraphCanvas::Deprecated::VariableRequestBus::EventResult(dataType, m_variableId, &GraphCanvas::Deprecated::VariableRequests::GetDataType);

                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetDataPaletteOverride, dataType);

                    AZStd::string tooltip = AZStd::string::format("This node changes %s's values according to the data connected to the input slots", variableName.c_str());
                    GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, tooltip);
                }
                else
                {
                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::ClearPaletteOverride);
                    GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, "");

                    GraphCanvas::NodeRequestBus::Event(GetEntityId(), &GraphCanvas::NodeRequests::SetTooltip, "This node changes a variable's values according to the data connected to the input slots");
                }
            }
        }
    }
}