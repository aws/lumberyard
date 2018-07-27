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
#include "AudioComponentOneShotNodes.h"
#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>

//////////////////////////////////////////////////////////////////////////
// AudioComponentExecuteOneShotNode
//////////////////////////////////////////////////////////////////////////

void AudioComponentExecuteOneShotNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig<string>("Trigger", _HELP("Name of the trigger to be executed")),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        { 0 }
    };

    config.sDescription = _HELP("Executes the indicated audio trigger as a one shot on the attached Entity's Audio Component");
    config.nFlags |= EFLN_ACTIVATION_INPUT | EFLN_TARGET_ENTITY;
    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

void AudioComponentExecuteOneShotNode::ProcessEvent(EFlowEvent event, SActivationInfo* activationInformation)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInformation->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    if (flowEntityType == FlowEntityType::Component)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(activationInformation, InputPorts::Activate))
            {
                AZStd::string triggerName(GetPortString(activationInformation, InputPorts::TriggerName).c_str());
                if (!triggerName.empty())
                {
                    EBUS_EVENT_ID(activationInformation->entityId, LmbrCentral::AudioTriggerComponentRequestBus, ExecuteTrigger, triggerName.c_str());
                }
                else
                {
                    AZ_Warning("FlowgraphNode", false, "Please specify a valid trigger name to be played");
                }
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// AudioComponentStopOneShotNode
//////////////////////////////////////////////////////////////////////////

void AudioComponentStopOneShotNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig<string>("Trigger", _HELP("Name of the trigger to be stopped")),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        { 0 }
    };

    config.sDescription = _HELP("Stops the indicated audio trigger as a one shot on the attached Entity's Audio Component");
    config.nFlags |= EFLN_ACTIVATION_INPUT | EFLN_TARGET_ENTITY;
    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

void AudioComponentStopOneShotNode::ProcessEvent(EFlowEvent event, SActivationInfo* activationInformation)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInformation->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    if (flowEntityType == FlowEntityType::Component)
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(activationInformation, InputPorts::Activate))
            {
                AZStd::string triggerName(GetPortString(activationInformation, InputPorts::TriggerName).c_str());
                if (!triggerName.empty())
                {
                    EBUS_EVENT_ID(activationInformation->entityId, LmbrCentral::AudioTriggerComponentRequestBus, KillTrigger, triggerName.c_str());
                }
                else
                {
                    AZ_Warning("FlowgraphNode", false, "Please specify a valid trigger name to be stopped");
                }
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("ComponentEntity:Audio:StopOneShot", AudioComponentStopOneShotNode);
REGISTER_FLOW_NODE("ComponentEntity:Audio:ExecuteOneShot", AudioComponentExecuteOneShotNode);


