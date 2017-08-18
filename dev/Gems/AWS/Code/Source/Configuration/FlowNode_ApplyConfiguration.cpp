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
#include <StdAfx.h>
#include <Configuration/FlowNode_ApplyConfiguration.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>

namespace LmbrAWS
{
    FlowNode_ApplyConfiguration::FlowNode_ApplyConfiguration(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode< eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_ApplyConfiguration::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Apply", "Apply the current AWS configuration to all managed clients.")
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_ApplyConfiguration::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
        };
        return outputPorts;
    }

    void FlowNode_ApplyConfiguration::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Apply))
        {
            if (gEnv->pLmbrAWS->GetClientManager()->ApplyConfiguration())
            {
                ActivateOutput(pActInfo, EOP_Success, true);
            }
            else
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, _LOC("Cloud Canvas Error: Unable to apply configuration. Please correct errors found earlier in the log."));
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE("AWS:Configuration:ApplyConfiguration", FlowNode_ApplyConfiguration);
} // namespace LmbrAWS
