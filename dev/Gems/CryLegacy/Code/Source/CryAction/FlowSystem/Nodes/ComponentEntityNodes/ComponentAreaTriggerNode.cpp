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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include "ComponentAreaTriggerNode.h"
#include <MathConversion.h>
#include <FlowSystem/FlowSystem.h>

void ComponentAreaTriggerNode::AreaTriggerEventHandler::ResetHandlerForEntityId(FlowEntityId triggerEntityId)
{
    if (triggerEntityId != FlowEntityId::s_invalidFlowEntityID)
    {
        LmbrCentral::TriggerAreaNotificationBus::Handler::BusDisconnect();

        m_triggeringEventBuffer.clear();
        m_lastCheckedEntityId = triggerEntityId;

        LmbrCentral::TriggerAreaNotificationBus::Handler::BusConnect(m_lastCheckedEntityId);
    }
    else
    {
        LmbrCentral::TriggerAreaNotificationBus::Handler::BusDisconnect();
    }
}

AZ::Outcome<ComponentAreaTriggerNode::AreaTriggerEventHandler::TriggeringEvent> ComponentAreaTriggerNode::AreaTriggerEventHandler::GetNextTriggeringEvent(FlowEntityId triggerComponentOwnerEntityId)
{
    if (triggerComponentOwnerEntityId == m_lastCheckedEntityId)
    {
        if (!m_triggeringEventBuffer.empty())
        {
            auto event = m_triggeringEventBuffer.front();
            m_triggeringEventBuffer.pop_front();
            return AZ::Success(event);
        }
    }
    else
    {
        ResetHandlerForEntityId(triggerComponentOwnerEntityId);
    }

    return AZ::Failure();
}


//////////////////////////////////////////////////////////////////////////

void ComponentAreaTriggerNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        OutputPortConfig<FlowEntityId>("Entered", _HELP("Entity Id of the entity that entered the Area trigger")),
        OutputPortConfig<FlowEntityId>("Exited", _HELP("Entity Id of the entity that exited the Area trigger")),
        { 0 }
    };

    config.sDescription = _HELP("Triggers when entities that are configured in the Trigger Component enter or exit the Trigger Area");
    config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

void ComponentAreaTriggerNode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(pActInfo->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    switch (event)
    {
    case eFE_Initialize:
    {
        if (flowEntityType == FlowEntityType::Component)
        {
            m_areaTriggerEvents.ResetHandlerForEntityId(pActInfo->entityId);
        }

        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
        break;
    }
    case eFE_Update:
    {
        if (flowEntityType == FlowEntityType::Component)
        {
            auto triggeringEvent = m_areaTriggerEvents.GetNextTriggeringEvent(pActInfo->entityId);

            while (triggeringEvent.IsSuccess())
            {
                if (triggeringEvent.GetValue().first == AreaTriggerEventHandler::TriggerEventType::Entry)
                {
                    ActivateOutput(pActInfo, OutputPorts::Entered, triggeringEvent.GetValue().second);
                }
                else if (triggeringEvent.GetValue().first == AreaTriggerEventHandler::TriggerEventType::Exit)
                {
                    ActivateOutput(pActInfo, OutputPorts::Exited, triggeringEvent.GetValue().second);
                }

                triggeringEvent = m_areaTriggerEvents.GetNextTriggeringEvent(pActInfo->entityId);
            }
        }
        break;
    }
    case eFE_Uninitialize:
    {
        m_areaTriggerEvents.ResetHandlerForEntityId(FlowEntityId(FlowEntityId::s_invalidFlowEntityID));
        pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
        break;
    }
    }
    ;
}

REGISTER_FLOW_NODE("ComponentEntity:TriggerComponent:EnterTrigger", ComponentAreaTriggerNode);
