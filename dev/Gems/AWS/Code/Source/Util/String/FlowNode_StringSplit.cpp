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
#include <Util/String/FlowNode_StringSplit.h>
#include <aws/core/utils/StringUtils.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "String:Split";

    void FlowNode_StringSplit::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig_Void("Activate", "String Split"),
            InputPortConfig<string>("Input", "String to split"),
            InputPortConfig<string>("separator", "character to separate on. If you pass a string, only the first characater will be used"),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<string>("Split0", "The first split"),
            OutputPortConfig<string>("Split2", "The second split"),
            OutputPortConfig<string>("Split3", "The third split"),
            OutputPortConfig<string>("Split4", "The fourth split"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("");
        configuration.SetCategory(EFLN_APPROVED);
    }

    void FlowNode_StringSplit::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && pActInfo->pInputPorts[0].IsUserFlagSet())
        {
            Aws::String inputString = GetPortString(pActInfo, 1).c_str();
            char splitter = GetPortString(pActInfo, 2)[0];

            auto parts = Aws::Utils::StringUtils::Split(inputString, splitter);

            if (parts.size() > 0)
            {
                SFlowAddress addr(pActInfo->myID, 0, true);
                string part = parts[0].c_str();
                pActInfo->pGraph->ActivatePort(addr, part);
            }
            if (parts.size() > 1)
            {
                SFlowAddress addr(pActInfo->myID, 1, true);
                string part = parts[1].c_str();
                pActInfo->pGraph->ActivatePort(addr, part);
            }
            if (parts.size() > 2)
            {
                SFlowAddress addr(pActInfo->myID, 2, true);
                string part = parts[2].c_str();
                pActInfo->pGraph->ActivatePort(addr, part);
            }
            if (parts.size() > 3)
            {
                SFlowAddress addr(pActInfo->myID, 3, true);
                string part = parts[3].c_str();
                pActInfo->pGraph->ActivatePort(addr, part);
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_StringSplit);
} // namespace AWS





