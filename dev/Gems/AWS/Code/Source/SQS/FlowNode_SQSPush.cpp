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
#include <SQS/FlowNode_SQSPush.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/GetQueueUrlResult.h>

#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:SQS:SQSPush";

    FlowNode_SQSPush::FlowNode_SQSPush(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_SQSPush::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Push"),
            InputPortConfig<string>("QueueName", _HELP("Name of the SQS queue that has already been created")),
            InputPortConfig<string>("Message", _HELP("Message to send"))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SQSPush::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts(0);
        return outputPorts;
    }


    void FlowNode_SQSPush::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo)
    {
        if (event == eFE_Activate && IsPortActive(activationInfo, EIP_Push))
        {
            AZStd::string queueName = GetPortString(activationInfo, EIP_QueueClient).c_str();
            EBUS_EVENT_RESULT(queueName, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, queueName);
            auto messageBody = GetPortString(activationInfo, EIP_Message);


            Aws::SQS::Model::SendMessageRequest sendMessageRequest;
            sendMessageRequest.SetQueueUrl(queueName.c_str());
            sendMessageRequest.SetMessageBody(messageBody);
            auto context = std::make_shared<FlowGraphContext>(activationInfo->pGraph, activationInfo->myID);

            MARSHALL_AWS_BACKGROUND_REQUEST(SQS, client, SendMessage, FlowNode_SQSPush::ApplyResult, sendMessageRequest, context)
        }
    }


    void FlowNode_SQSPush::ApplyResult(const Aws::SQS::Model::SendMessageRequest& request,
        const Aws::SQS::Model::SendMessageOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        if (context)
        {
            auto fgContext = std::static_pointer_cast<const FlowGraphContext>(context);

            if (outcome.IsSuccess())
            {
                SuccessNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId());
            }
            else
            {
                ErrorNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId(), outcome.GetError().GetMessage().c_str());
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SQSPush);
}
