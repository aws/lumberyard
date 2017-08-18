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
#include <Util/JSON/FlowNode_IterateJsonArray.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "JSON:IterateJsonArray";

    void FlowNode_IterateJsonArray::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig_Void("Begin", _HELP("Starts iterating over the supplied JSON array")),
            InputPortConfig_Void("Continue", _HELP("Continues iterating over the supplied JSON array")),
            InputPortConfig<string>("JsonArray", _HELP("The JSON array you want to iterate over")),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<string>("NextValue", "The next value in the array"),
            OutputPortConfig<bool>("IsNotEmpty", "If there are more values in the array"),
            OutputPortConfig<bool>("IsEmpty", "If there are no more values in the array"),
            OutputPortConfig<string>("Error", "Error while iterating"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("Iterates over a JSON array, returning one element at a time");
        configuration.SetCategory(EFLN_OBSOLETE);
    }

    void FlowNode_IterateJsonArray::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, EIP_Begin))
            {
                std::queue<Aws::Utils::Json::JsonValue>().swap(m_jsonArray);

                const string& jsonArrayStr = GetPortString(pActInfo, EIP_JsonArray);
                Aws::Utils::Json::JsonValue jsonValue(Aws::String(jsonArrayStr.c_str()));
                if (!jsonValue.WasParseSuccessful())
                {
                    SFlowAddress addr(pActInfo->myID, EOP_Error, true);
                    pActInfo->pGraph->ActivatePort(addr, string("Error parsing JSON array"));
                    return;
                }

                const auto& jsonArray = jsonValue.AsArray();
                if (!jsonArray.GetUnderlyingData())
                {
                    SFlowAddress addr(pActInfo->myID, EOP_Error, true);
                    pActInfo->pGraph->ActivatePort(addr, string("Error parsing JSON array or the array was empty"));
                    return;
                }

                for (int x = 0; x < jsonArray.GetLength(); x++)
                {
                    m_jsonArray.push(jsonArray.GetItem(x));
                }
            }

            if (IsPortActive(pActInfo, EIP_Begin) || IsPortActive(pActInfo, EIP_Continue))
            {
                if (m_jsonArray.empty())
                {
                    SFlowAddress addr(pActInfo->myID, EOP_Error, true);
                    pActInfo->pGraph->ActivatePort(addr, string("Can not iterate over an empty array"));
                    return;
                }

                auto& jsonValue = m_jsonArray.front();

                Aws::StringStream sstream;
                jsonValue.WriteCompact(sstream);

                SFlowAddress addr(pActInfo->myID, EOP_NextValue, true);
                pActInfo->pGraph->ActivatePort(addr, string(sstream.str().c_str()));

                m_jsonArray.pop();

                if (m_jsonArray.empty())
                {
                    SFlowAddress addr(pActInfo->myID, EOP_IsEmpty, true);
                    pActInfo->pGraph->ActivatePort(addr, true);
                }
                else
                {
                    SFlowAddress addr(pActInfo->myID, EOP_IsNotEmpty, true);
                    pActInfo->pGraph->ActivatePort(addr, true);
                }
            }
        }
    }

    IFlowNodePtr FlowNode_IterateJsonArray::Clone(SActivationInfo* pActInfo)
    {
        return new FlowNode_IterateJsonArray(pActInfo);
    }
    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_IterateJsonArray);
} // namespace LmbrAWS





