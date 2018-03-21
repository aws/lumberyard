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
#include <Configuration/FlowNode_SetDefaultRegion.h>
#include <aws/core/Region.h>
#include <CloudGemFramework/AwsApiJob.h>
#include <CloudCanvas/CloudCanvasIdentityBus.h>
#include <CloudGemFramework/CloudGemFrameworkBus.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Configuration:SetDefaultRegion";

    Aws::Vector<SInputPortConfig> FlowNode_SetDefaultRegion::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPortConfiguration =
        {
            InputPortConfig_Void("Activate", "Set (override) region for all AWS clients in current project."),
            InputPortConfig<string>("Region", "The region name you wish to set as default region for all AWS clients", "Region",
                _UICONFIG("enum_string:ap-northeast-1=ap-northeast-1,ap-southeast-1=ap-southeast-1,ap-southeast-2=ap-southeast-2,eu-central-1=eu-central-1,eu-west-1=eu-west-1,sa-east-1=sa-east-1,us-east-1=us-east-1,us-west-1=us-west-1,us-west-2=us-west-2"))
        };

        return inputPortConfiguration;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SetDefaultRegion::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts = { };
        return outputPorts;
    }

    void FlowNode_SetDefaultRegion::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && activationInfo->pInputPorts[EIP_Activate].IsUserFlagSet())
        {
            Aws::String region = GetPortString(activationInfo, EIP_Region).c_str();
            CloudGemFramework::AwsApiJob::Config* clientSettings{ nullptr };
            EBUS_EVENT_RESULT(clientSettings, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultConfig);

            if (clientSettings)
            {
                clientSettings->region = region;
                bool applyResult{ false };
                EBUS_EVENT_RESULT(applyResult, CloudGemFramework::CloudCanvasPlayerIdentityBus, ApplyConfiguration);
            }

            SuccessNotify(activationInfo->pGraph, activationInfo->myID);
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SetDefaultRegion);
} // namespace LmbrAWS
