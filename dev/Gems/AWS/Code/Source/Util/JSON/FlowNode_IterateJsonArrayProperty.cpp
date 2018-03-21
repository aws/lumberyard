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
#include <Util/JSON/FlowNode_IterateJsonArrayProperty.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "JSON:IterateJsonArrayProperty";

    FlowNode_IterateJsonArrayProperty::FlowNode_IterateJsonArrayProperty(IFlowNode::SActivationInfo* activationInfo)
        : m_index(0)
    {
    }

    void FlowNode_IterateJsonArrayProperty::GetConfiguration(SFlowNodeConfig& configuration)
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
            OutputPortConfig<string>("Value", "Value of the current element"),
            OutputPortConfig<int>("Index", "Index of the current element"),
            OutputPortConfig<bool>("Done", "Fired when continue is hit and there are no more elements"),
            OutputPortConfig<bool>("IsEmpty", "Fired if the given array is actually empty"),
            OutputPortConfig<string>("Error", "Fired if an error occurs"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("Iterates over a JSON array, returning one element at a time");
        configuration.SetCategory(EFLN_ADVANCED);
    }

    void FlowNode_IterateJsonArrayProperty::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, EIP_Begin))
            {
                const string& jsonArrayStr = GetPortString(pActInfo, EIP_JsonArray);
                Aws::Utils::Json::JsonValue jsonValue(Aws::String(jsonArrayStr.c_str()));
                if (!jsonValue.WasParseSuccessful())
                {
                    SFlowAddress addr(pActInfo->myID, EOP_Error, true);
                    pActInfo->pGraph->ActivatePort(addr, string("Error parsing JSON array"));
                    return;
                }

                const Aws::Utils::Array<Aws::Utils::Json::JsonValue>& jsonArray = jsonValue.AsArray();
                if (!jsonArray.GetUnderlyingData())
                {
                    SFlowAddress addr(pActInfo->myID, EOP_IsEmpty, true);
                    pActInfo->pGraph->ActivatePort(addr, true);
                    return;
                }

                m_index = 0;
                m_array = jsonArray;
            }

            if (IsPortActive(pActInfo, EIP_Begin) || IsPortActive(pActInfo, EIP_Continue))
            {
                if (m_index >= m_array.GetLength())
                {
                    m_index = 0;
                    SFlowAddress addr(pActInfo->myID, EOP_Done, true);
                    pActInfo->pGraph->ActivatePort(addr, true);
                    return;
                }

                Aws::Utils::Json::JsonValue& jsonValue = m_array.GetItem(m_index);

                Aws::StringStream sstream;
                jsonValue.WriteCompact(sstream);

                SFlowAddress addrValue(pActInfo->myID, EOP_Value, true);
                pActInfo->pGraph->ActivatePort(addrValue, string(sstream.str().c_str()));

                SFlowAddress addrIndex(pActInfo->myID, EOP_Index, true);
                pActInfo->pGraph->ActivatePort(addrIndex, m_index);

                ++m_index;
            }
        }
    }

    IFlowNodePtr FlowNode_IterateJsonArrayProperty::Clone(SActivationInfo* pActInfo)
    {
        return new FlowNode_IterateJsonArrayProperty(pActInfo);
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_IterateJsonArrayProperty);
} // namespace LmbrAWS





