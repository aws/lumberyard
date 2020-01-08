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
#include <SNS/FlowNode_SNSMessageDeserialize.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/json/JsonSerializer.h>
AZ_POP_DISABLE_WARNING

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:SNS:SNSMessageDeserialize";

    FlowNode_SNSMessageDeserialize::FlowNode_SNSMessageDeserialize(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_SNSMessageDeserialize::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Parse", _HELP("Extract the subject and body text from an SNS message (as JSON).")),
            InputPortConfig<string>("Message", "JSON message to deserialize.")
        };
        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SNSMessageDeserialize::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = {
            OutputPortConfig<string>("Body", _HELP("The message body")),
            OutputPortConfig<string>("Subject", _HELP("The message subject"))
        };
        return outputPorts;
    }


    void FlowNode_SNSMessageDeserialize::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Parse))
        {
            string inputString = GetPortString(pActInfo, EIP_Message);
            auto jsonMessage = Aws::Utils::Json::JsonValue(Aws::String(inputString.c_str()));
            if (jsonMessage.View().ValueExists("Message") && jsonMessage.View().ValueExists("Subject"))
            {
                SFlowAddress messageAddr(pActInfo->myID, EOP_MessageBody, true);
                pActInfo->pGraph->ActivatePort(messageAddr, string(jsonMessage.View().GetString("Message").c_str()));

                SFlowAddress subjectAddr(pActInfo->myID, EOP_MessageSubject, true);
                pActInfo->pGraph->ActivatePort(subjectAddr, string(jsonMessage.View().GetString("Subject").c_str()));

                SuccessNotify(pActInfo->pGraph, pActInfo->myID);
            }
            else
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Could not parse SNS Json.");
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SNSMessageDeserialize);
} // namespace AWS





