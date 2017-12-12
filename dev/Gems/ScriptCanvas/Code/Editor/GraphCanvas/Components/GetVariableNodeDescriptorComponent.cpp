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

#include "ScriptCanvas/Core/GraphBus.h"
#include "ScriptCanvas/Core/Slot.h"

#include "Editor/GraphCanvas/Components/GetVariableNodeDescriptorComponent.h"
#include <ScriptCanvas/Bus/RequestBus.h>
#include "ScriptCanvas/Core/PureData.h"

#include "Editor/Nodes/NodeUtils.h"
#include "Editor/GraphCanvas/PropertySlotIds.h"


namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // GetVariableReferenceDataInterface
    //////////////////////////////////////

    GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::GetVariableReferenceDataInterface(const AZ::EntityId& busId)
        : m_busId(busId)
    {
        GetVariableNodeDescriptorNotificationBus::Handler::BusConnect(m_busId);
    }

    GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::~GetVariableReferenceDataInterface()
    {
        GetVariableNodeDescriptorNotificationBus::Handler::BusDisconnect();
    }

    AZ::EntityId GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::GetVariableReference() const
    {
        AZ::EntityId variableId;
        GetVariableNodeDescriptorRequestBus::EventResult(variableId, m_busId, &GetVariableNodeDescriptorRequests::GetVariableId);

        return variableId;
    }

    void GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::AssignVariableReference(const AZ::EntityId& variableId)
    {
        GetVariableNodeDescriptorRequestBus::Event(m_busId, &GetVariableNodeDescriptorRequests::SetVariableId, variableId);
    }

    AZ::Uuid GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::GetVariableDataType() const
    {
        return AZStd::any::TYPEINFO_Uuid();
    }

	void GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::OnVariableActivated()
	{
		SignalValueChanged();
	}

    void GetVariableNodeDescriptorComponent::GetVariableReferenceDataInterface::OnAssignVariableChanged()
    {
        SignalValueChanged();
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
    }
    
    void GetVariableNodeDescriptorComponent::Deactivate()
    {
        NodeDescriptorComponent::Deactivate();

        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        GetVariableNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    GraphCanvas::VariableReferenceDataInterface* GetVariableNodeDescriptorComponent::CreateVariableDataInterface()
    {
        return aznew GetVariableReferenceDataInterface(GetEntityId());
    }

    ScriptCanvas::Endpoint GetVariableNodeDescriptorComponent::GetSourceEndpoint(ScriptCanvas::SlotId slotId) const
    {
        if (m_externalSlotIds.find(slotId) != m_externalSlotIds.end())
        {
            ScriptCanvas::Endpoint variableEndpoint;
            VariableNodeDescriptorRequestBus::EventResult(variableEndpoint, m_variableId, &VariableNodeDescriptorRequests::GetReadEndpoint);

            return ScriptCanvas::Endpoint(variableEndpoint.GetNodeId(), slotId);
        }
        else
        {
            AZStd::any* userData = nullptr;
            GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId targetNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

            return ScriptCanvas::Endpoint(targetNodeId, slotId);
        }
    }

    void GetVariableNodeDescriptorComponent::OnNameChanged()
    {
        UpdateTitle();
    }

    void GetVariableNodeDescriptorComponent::OnVariableActivated()
    {
        UpdateTitle();
		PopulateExternalSlotIds();
		GetVariableNodeDescriptorNotificationBus::Event(GetEntityId(), &GetVariableNodeDescriptorNotifications::OnVariableActivated);
    }

    void GetVariableNodeDescriptorComponent::OnVariableDestroyed()
    {
        SetVariableId(AZ::EntityId());
    }
    
    AZ::EntityId GetVariableNodeDescriptorComponent::GetVariableId() const
    {
        return m_variableId;
    }
    
    void GetVariableNodeDescriptorComponent::SetVariableId(const AZ::EntityId& variableId)
    {
        AZ::Uuid preDataType;
        GraphCanvas::VariableRequestBus::EventResult(preDataType, m_variableId, &GraphCanvas::VariableRequests::GetVariableDataType);

        GraphCanvas::VariableNotificationBus::Handler::BusDisconnect();

        AZ::EntityId oldVariableId = m_variableId;
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
            ConvertConnections(oldVariableId);
        }
        else
        {
            PopulatePins();
        }

        GetVariableNodeDescriptorNotificationBus::Event(GetEntityId(), &GetVariableNodeDescriptorNotifications::OnAssignVariableChanged);

        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, sceneId);
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
            PopulatePins();
        }
        else if (m_variableId.IsValid() && GraphCanvas::VariableRequestBus::FindFirstHandler(m_variableId) != nullptr)
        {
			PopulateExternalSlotIds();
        }

        if (m_variableId.IsValid())
        {
            GraphCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
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

    void GetVariableNodeDescriptorComponent::ConvertConnections(const AZ::EntityId& oldVariableId)
    {
        // In order to convert our connections over(allowing users to change a variable source on the fly).
        //
        // We want to create a mapping of slot names to the current slot id's that match to(the name will be the only constant
        // in this conversion).
        //
        // Once we have the slot name, we keep track of which endpoints were attached to that connection. Then we try to
        // re-create all of those connections.
        //
        // This is lossy because it's possible that the variable was already connected to the output, so some connections
        // might be lost in the transition.
        AZStd::unordered_multimap< AZStd::string, GraphCanvas::Endpoint > slotConnectionMapping;

        AZStd::vector< AZ::EntityId > slotIds;
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

        AZStd::any* userData = nullptr;

        AZ::EntityId scriptCanvasNodeId;
        GraphCanvas::NodeRequestBus::EventResult(userData, oldVariableId, &GraphCanvas::NodeRequests::GetUserData);
        scriptCanvasNodeId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

        for (const AZ::EntityId& slotId : slotIds)
        {
            GraphCanvas::SlotRequestBus::EventResult(userData, slotId, &GraphCanvas::SlotRequests::GetUserData);

            if (userData && userData->is<ScriptCanvas::SlotId>())
            {
                ScriptCanvas::SlotId scriptCanvasSlotId = (*AZStd::any_cast<ScriptCanvas::SlotId>(userData));
                ScriptCanvas::Endpoint scriptCanvasEndpoint(scriptCanvasNodeId, scriptCanvasSlotId);

                ScriptCanvas::Slot* slot = nullptr;
                ScriptCanvas::NodeRequestBus::EventResult(slot, scriptCanvasEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, scriptCanvasSlotId);

                if (slot)
                {
                    AZStd::vector< AZ::EntityId > connectionList;
                    GraphCanvas::SlotRequestBus::EventResult(connectionList, slotId, &GraphCanvas::SlotRequests::GetConnections);

                    for (const AZ::EntityId& connectionId : connectionList)
                    {
                        GraphCanvas::Endpoint targetEndpoint;
                        GraphCanvas::ConnectionRequestBus::EventResult(targetEndpoint, connectionId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);

                        if (targetEndpoint.IsValid())
                        {
                            slotConnectionMapping.emplace(slot->GetName(), targetEndpoint);
                        }
                    }
                }
            }

            userData = nullptr;
        }

        PopulatePins();

        slotIds.clear();
        GraphCanvas::NodeRequestBus::EventResult(slotIds, GetEntityId(), &GraphCanvas::NodeRequests::GetSlotIds);

        userData = nullptr;
        for (const AZ::EntityId& slotId : slotIds)
        {
            GraphCanvas::SlotRequestBus::EventResult(userData, slotId, &GraphCanvas::SlotRequests::GetUserData);

            if (userData && userData->is<ScriptCanvas::SlotId>())
            {
                ScriptCanvas::Endpoint endpoint = GetSourceEndpoint((*AZStd::any_cast<ScriptCanvas::SlotId>(userData)));

                ScriptCanvas::Slot* slot = nullptr;
                ScriptCanvas::NodeRequestBus::EventResult(slot, endpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, endpoint.GetSlotId());

                if (slot)
                {
                    auto rangeIter = slotConnectionMapping.equal_range(slot->GetName());

                    for (auto iter = rangeIter.first; iter != rangeIter.second; ++iter)
                    {
                        GraphCanvas::SlotRequestBus::Event(slotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, iter->second);
                    }
                }
            }

            userData = nullptr;
        }
    }

    void GetVariableNodeDescriptorComponent::ClearPins()
    {
        GraphCanvas::SlotLayoutRequestBus::Event(GetEntityId(), &GraphCanvas::SlotLayoutRequests::ClearSlotGroup, SlotGroups::DynamicPropertiesGroup);
        m_externalSlotIds.clear();
    }

    void GetVariableNodeDescriptorComponent::PopulatePins()
    {
        ClearPins();

        ScriptCanvas::Endpoint variableEndpoint;
        VariableNodeDescriptorRequestBus::EventResult(variableEndpoint, m_variableId, &VariableNodeDescriptorRequests::GetReadEndpoint);

        AZStd::vector< const ScriptCanvas::Slot* > slotList;
        ScriptCanvas::NodeRequestBus::EventResult(slotList, variableEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetAllSlots);

        ScriptCanvas::SlotId getSlotId;
        ScriptCanvas::NodeRequestBus::EventResult(getSlotId, variableEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_getThis);

        for (const ScriptCanvas::Slot* slot : slotList)
        {
            if (slot->GetType() == ScriptCanvas::SlotType::ExecutionIn
                || slot->GetType() == ScriptCanvas::SlotType::ExecutionOut
                || slot->GetType() == ScriptCanvas::SlotType::DataIn)
            {
                continue;
            }

            m_externalSlotIds.insert(slot->GetId());
            AZ::EntityId graphCanvasSlot = Nodes::CreateGraphCanvasSlot(GetEntityId(), (*slot), SlotGroups::DynamicPropertiesGroup);

            if (slot->GetId() == getSlotId)
            {
                AZStd::string subTitle;
                GraphCanvas::NodeTitleRequestBus::EventResult(subTitle, m_variableId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlot, &GraphCanvas::SlotRequests::SetName, subTitle);
            }
        }
    }

    void GetVariableNodeDescriptorComponent::UpdateTitle()
    {
        if (!m_variableId.IsValid() || GraphCanvas::VariableRequestBus::FindFirstHandler(m_variableId) != nullptr)
        {
            AZStd::string variableName = "Variable";

            GraphCanvas::VariableRequestBus::EventResult(variableName, m_variableId, &GraphCanvas::VariableRequests::GetVariableName);

            AZStd::string title = AZStd::string::format("Get %s", variableName.c_str());
            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, title);

            if (m_variableId.IsValid())
            {
                AZStd::string subTitle;
                GraphCanvas::NodeTitleRequestBus::EventResult(subTitle, m_variableId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
                GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetSubTitle, subTitle);

                AZ::Uuid dataType;
                GraphCanvas::VariableRequestBus::EventResult(dataType, m_variableId, &GraphCanvas::VariableRequests::GetVariableDataType);

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
}