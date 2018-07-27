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

#include "CryLegacy_precompiled.h"
#include "GameplayEventHandlerNode.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>

template <class TypeToUse>
struct GameplayEventTypeConverter
{
    using FlowType = TypeToUse;
};

template <>
struct GameplayEventTypeConverter<const char*>
{
    using FlowType = string;
};

template <>
struct GameplayEventTypeConverter <AZ::EntityId>
{
    using FlowType = FlowEntityId;
};

template <>
struct GameplayEventTypeConverter <AZ::Vector3>
{
    using FlowType = Vec3;
};


void GameplayEventHandlerNode::ActivatePortWithAny(SActivationInfo* activationInformation)
{
    SFlowAddress addr(activationInformation->myID, m_cachedEventType, true);
    float number = 0.0f;
    if (AZStd::any_numeric_cast<float>(&m_storedValue, number))
    {
        activationInformation->pGraph->ActivatePortAny(addr, TFlowInputData(number));
    }
    else if (m_storedValue.is<AZ::EntityId>())
    {
        activationInformation->pGraph->ActivatePortAny(addr, TFlowInputData(FlowEntityId(AZStd::any_cast<AZ::EntityId>(m_storedValue))));
    }
    else if (m_storedValue.is<AZ::Vector3>())
    {
        activationInformation->pGraph->ActivatePortAny(addr, TFlowInputData(AZVec3ToLYVec3(AZStd::any_cast<AZ::Vector3>(m_storedValue))));
    }
    else if (m_storedValue.is<AZStd::string>())
    {
        activationInformation->pGraph->ActivatePortAny(addr, TFlowInputData(string(AZStd::any_cast<AZStd::string>(m_storedValue).c_str())));
    }
    else if (m_storedValue.is<const char*>())
    {
        activationInformation->pGraph->ActivatePortAny(addr, TFlowInputData(string(AZStd::any_cast<const char*>(m_storedValue))));
    }
    else if (m_storedValue.is<bool>())
    {
        activationInformation->pGraph->ActivatePortAny(addr, TFlowInputData(AZStd::any_cast<bool>(m_storedValue)));
    }
    else if (!m_storedValue.empty())
    {
        //This is going to get called a lot, but we still want to capture unique types
        AZStd::string uuidString = m_storedValue.type().template ToString<AZStd::string>();
        AZStd::string windowName = AZStd::string::format("GameplayEventHandler (%s)", uuidString.c_str());
        AZ_WarningOnce(windowName.c_str(), false, "Type (%s) unsupported by flowgraph", uuidString.c_str());
    }
}

void GameplayEventHandlerNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig<FlowEntityId>("ChannelId", _HELP("The Entity you want to use as a channel.  Leave empty to use the general channel")),
        InputPortConfig<string>("EventName", _HELP("The Name of the event that you are expecting")),
        InputPortConfig_AnyType("Enable", _HELP("Trigger to enable the event handler")),
        InputPortConfig_AnyType("Disable", _HELP("Trigger to disable the event handler")),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        OutputPortConfig_AnyType("OnEventBegin", _HELP("Fires when event begins")),
        OutputPortConfig_AnyType("OnEventUpdate", _HELP("Fires when event updates")),
        OutputPortConfig_AnyType("OnEventEnd", _HELP("Fires when event ends")),
        { 0 }
    };

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

void GameplayEventHandlerNode::ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation)
{
    switch (evt)
    {
    case eFE_Update:
    {
        if (m_cachedEventType != None)
        {
            ActivatePortWithAny(activationInformation);
            m_cachedEventType = None;
        }
    }
    case eFE_Initialize:
    {
        activationInformation->pGraph->SetRegularlyUpdated(activationInformation->myID, true);
        SetCurrentHandler(activationInformation);
        break;
    }
    case eFE_Uninitialize:
    {
        if (!m_eventName.empty())
        {
            DisconnectFromHandlerBus();
        }
        break;
    }
    case eFE_Activate:
    {
        if (IsPortActive(activationInformation, InputPorts::EventName) || IsPortActive(activationInformation, InputPorts::ChannelId))
        {
            SetCurrentHandler(activationInformation);
        }
        if (IsPortActive(activationInformation, InputPorts::Enable))
        {
            m_enabled = true;
        }
        if (IsPortActive(activationInformation, InputPorts::Disable))
        {
            m_enabled = false;
        }
        break;
    }
    default:
        break;
    }
}

void GameplayEventHandlerNode::SetCurrentHandler(SActivationInfo* activationInformation)
{
    string newEventName = GetPortString(activationInformation, InputPorts::EventName);
    AZ::EntityId newId;
    FlowEntityId flowId = GetPortFlowEntityId(activationInformation, InputPorts::ChannelId);
    if (flowId != FlowEntityId::s_invalidFlowEntityID)
    {
        newId = flowId;
    }
    if (newEventName != m_eventName || newId != m_channelId)
    {
        if (!m_eventName.empty())
        {
            DisconnectFromHandlerBus();
        }
        m_eventName = newEventName;
        m_channelId = newId;
        if (!m_eventName.empty())
        {
            ConnectToHandlerBus();
        }
    }
}

void GameplayEventHandlerNode::ConnectToHandlerBus()
{
    AZ::GameplayNotificationId actionBusId(m_channelId, m_eventName.c_str());
    AZ::GameplayNotificationBus::Handler::BusConnect(actionBusId);
}

void GameplayEventHandlerNode::DisconnectFromHandlerBus()
{
    AZ::GameplayNotificationId actionBusId(m_channelId, m_eventName.c_str());
    AZ::GameplayNotificationBus::Handler::BusDisconnect(actionBusId);
}

void GameplayEventHandlerNode::OnEventBegin(const AZStd::any& value)
{
    if (m_enabled)
    {
        m_storedValue = value;
        m_cachedEventType = Begin;
    }
}

void GameplayEventHandlerNode::OnEventUpdating(const AZStd::any& value)
{
    if (m_enabled)
    {
        m_storedValue = value;
        m_cachedEventType = Update;
    }
}

void GameplayEventHandlerNode::OnEventEnd(const AZStd::any& value)
{
    if (m_enabled)
    {
        m_storedValue = value;
        m_cachedEventType = End;
    }
}

void GameplayEventSenderNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig_Void("Activate", _HELP("Activation")),
        InputPortConfig<FlowEntityId>("ChannelId", _HELP("The Entity you want to use as a channel.  Leave empty to use the general channel")),
        InputPortConfig_AnyType("SendEventValue", _HELP("Make sure the type of the value matches what the listeners are expecting")),
        InputPortConfig<string>("EventName", _HELP("The Name of the event that you are expecting")),
        InputPortConfig<int>("EventType", Updating, _HELP("Sned a Begin, Updating or End event"), _HELP("EventType"), "enum_int:OnEventBegin=0,OnEventUpdating=1,OnEventEnd=2"),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        { 0 }
    };

    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

void GameplayEventSenderNode::SendGameplayEvent(const AZ::GameplayNotificationId& actionBusId, const AZStd::any& value, GameplayEventType eventType)
{
    switch (eventType)
    {
        case Begin:
            AZ::GameplayNotificationBus::Event(actionBusId, &AZ::GameplayNotifications::OnEventBegin, value);
            break;
        case Updating:
            AZ::GameplayNotificationBus::Event(actionBusId, &AZ::GameplayNotifications::OnEventUpdating, value);
            break;
        case End:
            AZ::GameplayNotificationBus::Event(actionBusId, &AZ::GameplayNotifications::OnEventEnd, value);
            break;
        default:
            AZ_Warning("GameplayEventSenderNode", false, "invalid event type");
            break;
    }
}

AZStd::any GameplayEventSenderNode::GetPortValueAsAny(SActivationInfo* activationInformation)
{
    const TFlowInputData& flowData = GetPortAny(activationInformation, InputPorts::Value);
    switch (flowData.GetType())
    {
        case eFDT_Int:
        {
            int value = 0;
            flowData.GetValueWithConversion(value);
            return AZStd::any(value);
        }
        case eFDT_Float:
        {
            float value = 0;
            flowData.GetValueWithConversion(value);
            return AZStd::any(value);
        }
        case eFDT_EntityId:
        {
            EntityId value = 0;
            flowData.GetValueWithConversion(value);
            return AZStd::any(AZ::EntityId(value));
        }
        case eFDT_Vec3:
        {
            Vec3 value;
            flowData.GetValueWithConversion(value);
            return AZStd::any(LYVec3ToAZVec3(value));
        }
        case eFDT_String:
        {
            string value;
            flowData.GetValueWithConversion(value);
            return AZStd::any(AZStd::string(value.c_str()));
        }
        case eFDT_Bool:
        {
            bool value;
            flowData.GetValueWithConversion(value);
            return AZStd::any(value);
        }
        case eFDT_Double:
        {
            double value;
            flowData.GetValueWithConversion(value);
            return AZStd::any(value);
        }
        case eFDT_FlowEntityId:
        {
            FlowEntityId value;
            flowData.GetValueWithConversion(value);
            return AZStd::any(static_cast<AZ::EntityId>(value.GetId()));
        }
        default:
            return AZStd::any();
    }
}

void GameplayEventSenderNode::ProcessEvent(EFlowEvent evt, SActivationInfo* activationInformation)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInformation->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    switch (evt)
    {
    case eFE_Activate:
    {
        if (IsPortActive(activationInformation, InputPorts::Activate))
        {
            string eventName = GetPortString(activationInformation, InputPorts::EventName);
            AZ::EntityId channelId;
            FlowEntityId flowId = GetPortFlowEntityId(activationInformation, InputPorts::ChannelId);
            if (flowId != FlowEntityId::s_invalidFlowEntityID)
            {
                channelId = flowId;
            }
            AZStd::any value = GetPortValueAsAny(activationInformation);
            if (!eventName.empty())
            {
                AZ::GameplayNotificationId actionBusId(channelId, eventName.c_str());
                SendGameplayEvent(actionBusId, value, static_cast<GameplayEventType>(GetPortInt(activationInformation, EventType)));
            }
        }
        break;
    }
    default:
        break;
    }
}

REGISTER_FLOW_NODE("ComponentEntity:GameplayEventHandler", GameplayEventHandlerNode);
REGISTER_FLOW_NODE("ComponentEntity:GameplayEventSender",  GameplayEventSenderNode);

