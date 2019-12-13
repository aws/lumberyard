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
#include <Util/JSON/FlowNode_IsValueInJsonArray.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

// The AWS gem does not use AzCore, therefore the AZ_PUSH_DISABLE_WARNING macro is not available
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/json/JsonSerializer.h>
AZ_POP_DISABLE_WARNING

namespace LmbrAWS
{
    static const char* CLASS_TAG = "JSON:IsValueInJsonArray";

    void FlowNode_IsValueInJsonArray::GetConfiguration(SFlowNodeConfig& configuration)
    {
        static const SInputPortConfig inputPortConfiguration[] =
        {
            InputPortConfig<string>("JsonArray", _HELP("The array to search")),
            InputPortConfig<string>("Value", _HELP("The value to search for")),
            { 0 }
        };

        static const SOutputPortConfig outputPortConfiguration[] =
        {
            OutputPortConfig<int>("Result", "Outputs the number of occurences found"),
            OutputPortConfig<bool>("True", "Fires if value was found"),
            OutputPortConfig<bool>("False", "Fires if value was not found"),
            { 0 }
        };

        configuration.nFlags |= EFLN_ACTIVATION_INPUT;
        configuration.pInputPorts = inputPortConfiguration;
        configuration.pOutputPorts = outputPortConfiguration;
        configuration.sDescription = _HELP("Looks through a Json Array for the specified value");
        configuration.SetCategory(EFLN_ADVANCED);
    }

    void FlowNode_IsValueInJsonArray::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate)
        {
            if (IsPortActive(pActInfo, (int)EInputs::Activate))
            {
                const string& jsonArrayStr = GetPortString(pActInfo, (int)EInputs::JsonArray);
                const string& value = GetPortString(pActInfo, (int)EInputs::Value);

                Aws::Utils::Json::JsonValue jsonValue(Aws::String(jsonArrayStr.c_str()));

                if (!jsonValue.View().IsListType())
                {
                    CRY_ASSERT_MESSAGE(false, "Json value is not a list");
                    return;
                }

                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonArray = jsonValue.View().AsArray();

                int numOccurrences = 0;

                for (unsigned int i = 0; i < jsonArray.GetLength(); ++i)
                {
                    Aws::Utils::Json::JsonView& item = jsonArray.GetItem(i);
                    Aws::String itemValue = item.AsString();

                    if (itemValue == value.c_str())
                    {
                        ++numOccurrences;
                    }
                }

                ActivateOutput(pActInfo, (int)EOutputs::Result, numOccurrences);
                ActivateOutput(pActInfo, numOccurrences > 0 ? (int)EOutputs::True : (int)EOutputs::False, true);
            }
        }
    }


    IFlowNodePtr FlowNode_IsValueInJsonArray::Clone(SActivationInfo* pActInfo)
    {
        return new FlowNode_IsValueInJsonArray(pActInfo);
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_IsValueInJsonArray);
} // namespace LmbrAWS





