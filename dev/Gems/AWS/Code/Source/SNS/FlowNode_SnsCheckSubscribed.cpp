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
#pragma warning(push)
#pragma warning(disable: 4819) // Invalid character not in default code page
#include <SNS/FlowNode_SnsCheckSubscribed.h>
#include <aws/sns/model/ListSubscriptionsByTopicRequest.h>
#pragma warning(pop)

#include <CloudCanvas/CloudCanvasMappingsBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

namespace LmbrAWS
{
    static const char* CLASS_TAG = "AWS:Primitive:SNS:SnsCheckSubscribed";

    FlowNode_SnsCheckSubscribed::FlowNode_SnsCheckSubscribed(IFlowNode::SActivationInfo* activationInfo)
        : BaseMaglevFlowNode < eNCT_Singleton >(activationInfo)
    {
    }

    Aws::Vector<SInputPortConfig> FlowNode_SnsCheckSubscribed::GetInputPorts() const
    {
        static const Aws::Vector<SInputPortConfig> inputPorts = {
            InputPortConfig_Void("Check", _HELP("Activate this to check if the ARN is subscribed to the SNS topic.")),
            InputPortConfig<string>("TopicARN", _HELP("SNS topic arn to check.")),
            InputPortConfig<string>("Endpoint", _HELP("Endpoint to check if it is subscribed to the specified topic. This may be an email address, a SQS queue or any other endpoint supported by SNS."), "Endpoint")
        };

        return inputPorts;
    }

    Aws::Vector<SOutputPortConfig> FlowNode_SnsCheckSubscribed::GetOutputPorts() const
    {
        static const Aws::Vector<SOutputPortConfig> outputPorts =
        {
            OutputPortConfig_Void("True", _HELP("Triggered if ARN is subscribed to the SNS topic.")),
            OutputPortConfig_Void("False", _HELP("Triggered if ARN is not subscribed to the SNS topic.")),
        };
        return outputPorts;
    }

    void FlowNode_SnsCheckSubscribed::SendListSubscriptions(std::shared_ptr<const FlowGraphContext> context, const string& topicArn, const string& nextToken)
    {
        Aws::SNS::Model::ListSubscriptionsByTopicRequest listSubscriptionsRequest;
        listSubscriptionsRequest.SetTopicArn(topicArn);
        if (!nextToken.empty())
        {
            listSubscriptionsRequest.SetNextToken(nextToken);
        }

        MARSHALL_AWS_BACKGROUND_REQUEST(SNS, client, ListSubscriptionsByTopic, FlowNode_SnsCheckSubscribed::ApplyResult, listSubscriptionsRequest, context)
    }

    void FlowNode_SnsCheckSubscribed::ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
    {
        if (event == eFE_Activate && IsPortActive(pActInfo, EIP_Check))
        {
            AZStd::string topicArn = GetPortString(pActInfo, EIP_TopicClient).c_str();
            EBUS_EVENT_RESULT(topicArn, CloudGemFramework::CloudCanvasMappingsBus, GetLogicalToPhysicalResourceMapping, topicArn);
            auto context =  std::make_shared<FlowGraphContext>(pActInfo->pGraph, pActInfo->myID);
            SendListSubscriptions(context, topicArn.c_str());
        }
    }

    void FlowNode_SnsCheckSubscribed::ApplyResult(const Aws::SNS::Model::ListSubscriptionsByTopicRequest& request, const Aws::SNS::Model::ListSubscriptionsByTopicOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
    {
        if (context)
        {
            auto fgContext = std::static_pointer_cast<const FlowGraphContext>(context);
            if (outcome.IsSuccess())
            {
                auto& subscriptions = outcome.GetResult().GetSubscriptions();
                auto checkArn = fgContext->GetFlowGraph()->GetInputValue(fgContext->GetFlowNodeId(), EIP_Endpoint)->GetPtr<string>();
                auto foundSubscription = std::find_if(subscriptions.begin(), subscriptions.end(), [&checkArn](const Aws::SNS::Model::Subscription& subscription)
                        {
                            return std::strcmp(subscription.GetEndpoint().c_str(), checkArn->c_str()) == 0;
                        });
                if (foundSubscription != subscriptions.end())
                {
                    SFlowAddress arnAddr(fgContext->GetFlowNodeId(), EOP_True, true);
                    fgContext->GetFlowGraph()->ActivatePort(arnAddr, true);
                }
                else
                {
                    auto nextToken = outcome.GetResult().GetNextToken();
                    if (!nextToken.empty())
                    {
                        SendListSubscriptions(fgContext, request.GetTopicArn().c_str(), nextToken.c_str());
                    }
                    else
                    {
                        SFlowAddress arnAddr(fgContext->GetFlowNodeId(), EOP_False, true);
                        fgContext->GetFlowGraph()->ActivatePort(arnAddr, true);
                    }
                }
            }
            else
            {
                ErrorNotify(fgContext->GetFlowGraph(), fgContext->GetFlowNodeId(), outcome.GetError().GetMessage().c_str());
            }
        }
    }

    REGISTER_CLASS_TAG_AND_FLOW_NODE(CLASS_TAG, FlowNode_SnsCheckSubscribed);
}