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
#include <Configuration/FlowNode_ExpandConfigurationParameters.h>
#include <LmbrAWS/ILmbrAWS.h>
#include <LmbrAWS/IAWSClientManager.h>

namespace LmbrAWS
{
    FlowNode_ExpandConfigurationParameters::FlowNode_ExpandConfigurationParameters(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_ExpandConfigurationParameters::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Expand", "Expand parameter references."),
            InputPortConfig<string>("Value", "Value containing $param-name$ substrings.")
        };
        return inputPortConfiguration;
    }
    Aws::Vector<SOutputPortConfig> FlowNode_ExpandConfigurationParameters::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig>  outputPortConfiguration =
        {
            OutputPortConfig<string>("Value", "Value with $param-name$ substrings replaced by parameter values.")
        };

        return outputPortConfiguration;
    }

    void FlowNode_ExpandConfigurationParameters::ProcessEvent_Internal(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Expand))
        {
            string value = GetPortString(pActInfo, EIP_Value);
            gEnv->pLmbrAWS->GetClientManager()->GetConfigurationParameters().ExpandParameters(value);
            ActivateOutput(pActInfo, EOP_Value, value);
            ActivateOutput(pActInfo, EOP_Success, true);
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE("AWS:Configuration:ExpandConfigurationParameters", FlowNode_ExpandConfigurationParameters);
} // namespace Amazon
