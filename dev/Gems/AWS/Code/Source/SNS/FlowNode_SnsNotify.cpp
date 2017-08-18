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
#pragma warning(push)
#pragma warning(disable: 4819) // Invalid character not in default code page
#include <SNS/FlowNode_SnsNotify.h>
#include <aws/sns/model/PublishRequest.h>
#pragma warning(pop)

using namespace Aws::SNS;
using namespace Aws::SNS::Model;

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:SNS:SNSNotify";

    FlowNode_SnsNotify::FlowNode_SnsNotify(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_SnsNotify::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Notify", _HELP("Activate this to send your notification.")),
            InputPortConfig<string>("Message", _HELP("Message to send.")),
            InputPortConfig<string>("Subject", _HELP("Subject of message.")),
            m_topicClientPort.GetConfiguration("TopicARN", _HELP("The ARN for your SNS topic."))
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SnsNotify::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts(0);
        return outputPorts;
    }

    void FlowNode_SnsNotify::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        auto startIndex = EIP_StartIndex;

        //publish to sns
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Notify))
        {
            auto client = m_topicClientPort.GetClient(pActInfo);
            if (!client.IsReady())
            {
                ErrorNotify(pActInfo->pGraph, pActInfo->myID, "Client configuration not ready.");
                return;
            }

            auto& arn = client.GetTopicARN();
            auto& message = GetPortString(pActInfo, EIP_Message);
            auto& subject = GetPortString(pActInfo, EIP_Subject);

            PublishRequest publishRequest;
            publishRequest.SetMessage(message.c_str());
            publishRequest.SetSubject(subject.c_str());
            publishRequest.SetTopicArn(arn.c_str());

            auto context = std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            MARSHALL_AWS_BACKGROUND_REQUEST(SNS, client, Publish, FlowNode_SnsNotify::ApplyResult, publishRequest, context)
        }
    }

    void FlowNode_SnsNotify::ApplyResult(const PublishRequest&, const PublishOutcome& outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
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

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SnsNotify);
}