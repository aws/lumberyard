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
#include <Util/String/FlowNode_ReplaceString.h>
#include <aws/core/utils/StringUtils.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "String:ReplaceString";

    void FlowNode_ReplaceString::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig<string>("Input", _HELP("The string which a substring will be replaced")),
            InputPortConfig<string>("Replace", _HELP("The string to replace")),
            InputPortConfig<string>("ReplaceWith", _HELP("The string to replace with")),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<string>("OutString", "The modified string"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("Replace occurances of a string with another string");
        configuration.SetCategory(EFLN_APPROVED);
    }

    void FlowNode_ReplaceString::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Input))
        {
            string inputString = GetPortString(pActInfo, EIP_Input);
            const string& stringToReplace = GetPortString(pActInfo, EIP_Replace);
            const string& replacementString = GetPortString(pActInfo, EIP_ReplaceWith);

            size_t start_pos = 0;
            while ((start_pos = inputString.find(stringToReplace, start_pos)) != std::string::npos)
            {
                inputString.replace(start_pos, stringToReplace.length(), replacementString);
                start_pos += replacementString.length();
            }

            SFlowAddress addr(pActInfo->myID, 0, true);

            pActInfo->pGraph->ActivatePort(addr, inputString);
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_ReplaceString);
} // namespace Amazon





