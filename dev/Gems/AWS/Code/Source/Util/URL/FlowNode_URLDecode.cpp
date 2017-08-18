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
#include <Util/URL/FlowNode_URLDecode.h>
#include <aws/core/utils/StringUtils.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "String:URLDecode";

    void FlowNode_URLDecode::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig<string>("Input", "String to URL Decode"),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<string>("DecodedString", "The URL decoded string"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("");
        configuration.SetCategory(EFLN_APPROVED);
    }

    void FlowNode_URLDecode::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && pActInfo->pInputPorts[0].IsUserFlagSet())
        {
            string inputString = GetPortString(pActInfo, 0);
            string decodedString = Aws::Utils::StringUtils::URLDecode(inputString).c_str();

            SFlowAddress addr(pActInfo->myID, 0, true);

            pActInfo->pGraph->ActivatePort(addr, decodedString);
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_URLDecode);
} // namespace AWS





