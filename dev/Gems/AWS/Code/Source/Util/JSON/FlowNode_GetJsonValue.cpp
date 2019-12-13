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
#include <Util/JSON/FlowNode_GetJsonValue.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
AZ_POP_DISABLE_WARNING

namespace LmbrAWS
{
    static const char* CLASS_TAG = "JSON:GetJsonValue";

    void FlowNode_GetJsonValue::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig<string>("JSON", _HELP("The JSON to parse")),
            InputPortConfig<string>("Attribute", _HELP("The attribute to get value of")),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<string>("Error", _HELP("Activated if JSON could not be parsed or attribute not found")),
            OutputPortConfig<string>("OutValue", "The attribute value"),
            { 0 }
        };

        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("Gets the value for the given attribute in JSON");
        configuration.SetCategory(EFLN_OBSOLETE);
    }

    void FlowNode_GetJsonValue::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Json))
        {
            string inputJson = GetPortString(pActInfo, EIP_Json);
            const string& attribute = GetPortString(pActInfo, EIP_Attribute);

            auto jsonMessage = Aws::Utils::Json::JsonValue(Aws::String(inputJson.c_str()));
            if (jsonMessage.View().ValueExists(attribute.c_str()))
            {
                SFlowAddress addr(pActInfo->myID, 1, true);

                Aws::Utils::Json::JsonView attributeValue = jsonMessage.View().GetObject(attribute.c_str());

                if (attributeValue.IsString() || attributeValue.IsIntegerType() || attributeValue.IsFloatingPointType())
                {
                    pActInfo->pGraph->ActivatePort(addr, string(attributeValue.AsString().c_str()));
                }
                else if (attributeValue.IsObject() || attributeValue.IsListType())
                {
                    Aws::String jsonString = attributeValue.WriteCompact();
                    pActInfo->pGraph->ActivatePort(addr, string(jsonString.c_str()));
                }
                else
                {
                    CRY_ASSERT_MESSAGE(false, "Unsupported type being accessed in GetJsonValue");
                }
            }
            else
            {
                SFlowAddress addr(pActInfo->myID, 0, true);
                pActInfo->pGraph->ActivatePortCString(addr, "Could not parse SNS Json.");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_GetJsonValue);
} // namespace LmbrAWS





