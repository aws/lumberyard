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
#include <AWS_precompiled.h>
#include <Configuration/FlowNode_ExpandConfigurationParameters.h>
#include <CloudCanvas/CloudCanvasMappingsBus.h>

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
            AZStd::string value = GetPortString(pActInfo, EIP_Value).c_str();
            AZStd::unordered_set<AZStd::string> seenSet;
            ExpandParameterMapping(value, seenSet);
            string valueStr{ value.c_str() };
            ActivateOutput(pActInfo, EOP_Value, valueStr);
            ActivateOutput(pActInfo, EOP_Success, true);
        }
    }

    void FlowNode_ExpandConfigurationParameters::ExpandParameterMapping(AZStd::string& value, AZStd::unordered_set<AZStd::string>& seen)
    {
        int start, end, pos = 0;
        while ((start = value.find_first_of('$', pos)) != -1)
        {
            end = value.find_first_of('$', start + 1);
            if (end == -1)
            {
                // unmatched $ at end of string, leave it there
                pos = value.size();
            }
            else if (end == start + 1)
            {
                // $$, remove the second $
                value.replace(start + 1, 1, "");
                pos = start + 1;
            }
            else
            {
                // $foo$
                AZStd::string name = value.substr(start + 1, (end - start) - 1);
                if (seen.count(name) == 0) // protect against recursive parameter values
                {
                    AZStd::string temp;
                    EBUS_EVENT_RESULT(temp, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, name);
                    seen.insert(name);
                    ExpandParameterMapping(temp, seen);
                    seen.erase(name);
                    value.replace(start, (end - start) + 1, temp);
                    pos = start + temp.length();
                }
                else
                {
                    pos = end + 1;
                }
            }
        }
    }
    REGISTER_CLASS_TAG_AND_FLOW_NODE("AWS:Configuration:ExpandConfigurationParameters", FlowNode_ExpandConfigurationParameters);
} // namespace Amazon
