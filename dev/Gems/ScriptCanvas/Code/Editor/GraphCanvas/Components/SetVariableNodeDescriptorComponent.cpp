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

#include "ScriptCanvas/Bus/RequestBus.h"
#include "ScriptCanvas/Core/GraphBus.h"
#include "ScriptCanvas/Core/PureData.h"
#include "ScriptCanvas/Core/Slot.h"

#include "Editor/Nodes/NodeUtils.h"

#include "Editor/GraphCanvas/Components/SetVariableNodeDescriptorComponent.h"
#include "Editor/GraphCanvas/PropertySlotIds.h"

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // SetVariableReferenceDataInterface
    //////////////////////////////////////

    SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::SetVariableReferenceDataInterface(const AZ::EntityId& busId)
        : m_busId(busId)
    {
        SetVariableNodeDescriptorNotificationBus::Handler::BusConnect(m_busId);
    }

    SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::~SetVariableReferenceDataInterface()
    {
        SetVariableNodeDescriptorNotificationBus::Handler::BusDisconnect();
    }

    AZ::EntityId SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::GetVariableReference() const
    {
        AZ::EntityId variableId;
        SetVariableNodeDescriptorRequestBus::EventResult(variableId, m_busId, &SetVariableNodeDescriptorRequests::GetVariableId);

        return variableId;
    }

    void SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::AssignVariableReference(const AZ::EntityId& variableId)
    {
        SetVariableNodeDescriptorRequestBus::Event(m_busId, &SetVariableNodeDescriptorRequests::SetVariableId, variableId);
    }

    AZ::Uuid SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::GetVariableDataType() const
    {
        return AZStd::any::TYPEINFO_Uuid();
    }

	void SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::OnVariableActivated()
	{
		SignalValueChanged();
	}

    void SetVariableNodeDescriptorComponent::SetVariableReferenceDataInterface::OnAssignVariableChanged()
    {
        SignalValueChanged();
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

    GraphCanvas::VariableReferenceDataInterface* SetVariableNodeDescriptorComponent::CreateVariableDataInterface()
    {
        return aznew SetVariableReferenceDataInterface(GetEntityId());
    }

    AZ::EntityId SetVariableNodeDescriptorComponent::GetVariableId() const
    {
        return m_variableId;
    }

    void SetVariableNodeDescriptorComponent::SetVariableId(const AZ::EntityId& variableId)
    {
        DisconnectFromEndpoint();

        AZ::Uuid preDataType;
        GraphCanvas::VariableRequestBus::EventResult(preDataType, m_variableId, &GraphCanvas::VariableRequests::GetVariableDataType);

        GraphCanvas::VariableNotificationBus::Handler::BusDisconnect();

        m_variableId = variableId;

        if (m_variableId.IsValid())
        {
            GraphCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
        }

        AZ::Uuid postDataType;
        GraphCanvas::VariableRequestBus::EventResult(postDataType, m_variableId, &GraphCanvas::VariableRequests::GetVariableDataType);

        UpdateTitle();

        if (preDataType == postDataType && m_variableId.IsValid())
        {
            ConnectToEndpoint();
        }
        else
        {
            PopulatePins();
        }

        SetVariableNodeDescriptorNotificationBus::Event(GetEntityId(), &SetVariableNodeDescriptorNotifications::OnAssignVariableChanged);

        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, sceneId);
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

    void SetVariableNodeDescriptorComponent::OnNameChanged()
    {
        UpdateTitle();
    }

    void SetVariableNodeDescriptorComponent::OnVariableActivated()
    {
        UpdateTitle();
		SetVariableNodeDescriptorNotificationBus::Event(GetEntityId(), &SetVariableNodeDescriptorNotifications::OnVariableActivated);
    }

    void SetVariableNodeDescriptorComponent::OnVariableDestroyed()
    {
        SetVariableId(AZ::EntityId());
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
            PopulatePins();
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

        if (m_variableId.IsValid())
        {
            GraphCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
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
                ScriptCanvas::GraphRequestBus::EventResult(outcome, sceneId, &ScriptCanvas::GraphRequests::CanConnectByEndpoint, m_getEndpoint, writeEndpoint);

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

    void SetVariableNodeDescriptorComponent::PopulatePins()
    {
        ClearPins();

        if (m_variableId.IsValid())
        {
            AZ::Uuid dataType;
            GraphCanvas::VariableRequestBus::EventResult(dataType, m_variableId, &GraphCanvas::VariableRequests::GetVariableDataType);

            ScriptCanvas::Slot* slot;
            ScriptCanvas::NodeRequestBus::EventResult(slot, m_setEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, m_setEndpoint.GetSlotId());

            if (slot)
            {
                m_dataConnectionSlot = Nodes::CreateForcedDataTypeGraphCanvasSlot(GetEntityId(), (*slot), dataType, SlotGroups::DynamicPropertiesGroup);
                
                AZStd::string subTitle;
                GraphCanvas::NodeTitleRequestBus::EventResult(subTitle, m_variableId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
                GraphCanvas::SlotRequestBus::Event(m_dataConnectionSlot, &GraphCanvas::SlotRequests::SetName, subTitle);

                if (m_dataConnectionSlot.IsValid())
                {
                    GraphCanvas::SlotNotificationBus::Handler::BusConnect(m_dataConnectionSlot);
                }
            }
        }
    }

    void SetVariableNodeDescriptorComponent::UpdateTitle()
    {
        if (!m_variableId.IsValid() || GraphCanvas::VariableRequestBus::FindFirstHandler(m_variableId) != nullptr)
        {
            AZStd::string variableName = "Variable";

            GraphCanvas::VariableRequestBus::EventResult(variableName, m_variableId, &GraphCanvas::VariableRequests::GetVariableName);

            AZStd::string title = AZStd::string::format("Set %s", variableName.c_str());
            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, title);

            if (m_variableId.IsValid())
            {
                AZStd::string subTitle;
                GraphCanvas::NodeTitleRequestBus::EventResult(subTitle, m_variableId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, subTitle);

                AZ::Uuid dataType;
                GraphCanvas::VariableRequestBus::EventResult(dataType, m_variableId, &GraphCanvas::VariableRequests::GetVariableDataType);

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