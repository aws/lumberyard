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
#pragma once
#include <Util/FlowSystem/BaseMaglevFlowNode.h>
#include <aws/sns/SNSClient.h>

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Check if an ARN is subscribed to an SNS topic
    ///
    //////////////////////////////////////////////////////////////////////////////////////

    class FlowNode_SnsCheckSubscribed
        : public BaseMaglevFlowNode<eNCT_Singleton>
    {
    public:

        FlowNode_SnsCheckSubscribed(IFlowNode::SActivationInfo* activationInfo);

        virtual ~FlowNode_SnsCheckSubscribed(){};

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Check if an ARN is subscribed to an SNS topic"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;

    private:
        void SendListSubscriptions(LmbrAWS::SNS::TopicClient& client, const std::shared_ptr<const FlowGraphContext> context, const string& topicArn, const string& nextToken = "");
        void ApplyResult(const Aws::SNS::Model::ListSubscriptionsByTopicRequest&, const Aws::SNS::Model::ListSubscriptionsByTopicOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);

        enum EInputs
        {
            EIP_Check = EIP_StartIndex,
            EIP_TopicClient,
            EIP_Endpoint
        };

        enum EOutputs
        {
            EOP_True = EOP_StartIndex,
            EOP_False
        };

        LmbrAWS::SNS::TopicClientInputPort m_topicClientPort {
            EIP_TopicClient
        };
    };
} // namespace AWS