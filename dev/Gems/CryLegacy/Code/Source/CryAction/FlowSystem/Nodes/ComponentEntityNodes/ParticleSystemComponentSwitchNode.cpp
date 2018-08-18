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
#include "ParticleSystemComponentSwitchNode.h"
#include <LmbrCentral/Rendering/ParticleComponentBus.h>


void ParticleComponentSwitchNode::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_config[] =
    {
        InputPortConfig_Void("Show", _HELP("Shows the particle system")),
        InputPortConfig_Void("Hide", _HELP("Hides the particle system")),
        { 0 }
    };

    static const SOutputPortConfig out_config[] =
    {
        { 0 }
    };

    config.sDescription = _HELP("Shows or Hides the particle component(s) on the attached entity");
    config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = in_config;
    config.pOutputPorts = out_config;
    config.SetCategory(EFLN_APPROVED);
}

void ParticleComponentSwitchNode::ProcessEvent(EFlowEvent event, SActivationInfo* activationInformation)
{
    FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(activationInformation->entityId);

    AZ_Assert((flowEntityType == FlowEntityType::Component) || (flowEntityType == FlowEntityType::Invalid)
        , "This node is incompatible with Legacy Entities");

    if (flowEntityType == FlowEntityType::Component)
    {
        switch (event)
        {
        case eFE_Initialize:
        case eFE_Activate:
        {
            if (IsPortActive(activationInformation, InputPorts::Show))
            {
                EBUS_EVENT_ID(activationInformation->entityId, LmbrCentral::ParticleComponentRequestBus, Show);
            }
            else if (IsPortActive(activationInformation, InputPorts::Hide))
            {
                EBUS_EVENT_ID(activationInformation->entityId, LmbrCentral::ParticleComponentRequestBus, Hide);
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

REGISTER_FLOW_NODE("ComponentEntity:Particles:Switch", ParticleComponentSwitchNode);
