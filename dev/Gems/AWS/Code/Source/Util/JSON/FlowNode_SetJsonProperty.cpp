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
#include <Util/JSON/FlowNode_SetJsonProperty.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "JSON:SetJsonProperty";

    IFlowNodePtr FlowNode_SetJsonProperty::Clone(SActivationInfo* pActInfo)
    {
        return new FlowNode_SetJsonProperty(pActInfo);
    }

    void FlowNode_SetJsonProperty::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig_Void("In", _HELP("The activation in pin")),
            InputPortConfig<string>("JsonObject", _HELP("The JSON object to set the property on (can be empty)")),
            InputPortConfig<string>("Name", _HELP("Name of the property")),
            InputPortConfig<string>("Value", _HELP("Value of the property")),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<string>("Out", "The resulting JSON object"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("Sets a property on a JSON Object");
        configuration.SetCategory(EFLN_ADVANCED);
    }

    void FlowNode_SetJsonProperty::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_In))
        {
            const string& objectString = GetPortString(pActInfo, EIP_JsonObject);
            const string& nameString = GetPortString(pActInfo, EIP_Name);
            const string& valueString = GetPortString(pActInfo, EIP_Value);

            Aws::Utils::Json::JsonValue objectValue;
            Aws::Utils::Json::JsonValue value(Aws::String(valueString.c_str()));

            if (!objectString.empty())
            {
                objectValue = Aws::Utils::Json::JsonValue(Aws::String(objectString.c_str()));

                if (!objectValue.WasParseSuccessful() || !objectValue.IsObject())
                {
                    CRY_ASSERT_TRACE(false, ("Value of the Object field in JSONProperty node ('%s') does not parse to a valid JSON object", objectString.c_str()));

                    objectValue = Aws::Utils::Json::JsonValue();
                }
            }

            if (value.WasParseSuccessful())
            {
                if (value.IsObject())
                {
                    objectValue.WithObject(nameString.c_str(), value);
                }
                else if (value.IsListType())
                {
                    objectValue.WithArray(nameString.c_str(), value.AsArray());
                }
                else
                {
                    objectValue.WithString(nameString.c_str(), valueString.c_str());
                }
            }
            else
            {
                objectValue.WithString(nameString.c_str(), valueString.c_str());
            }

            Aws::StringStream sstream;
            objectValue.WriteCompact(sstream);

            SFlowAddress addr(pActInfo->myID, EOP_Out, true);
            pActInfo->pGraph->ActivatePort(addr, string(sstream.str().c_str()));
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SetJsonProperty);
} // namespace LmbrAWS





