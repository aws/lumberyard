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
#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/sns/SNSClient.h>
#pragma warning(default: 4355)

namespace LmbrAWS
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Subscribe an endpoint to a SNS topic
    ///
    //////////////////////////////////////////////////////////////////////////////////////

    class FlowNode_SnsSubscribe
        : public BaseMaglevFlowNode<eNCT_Singleton>
    {
    public:

        FlowNode_SnsSubscribe(IFlowNode::SActivationInfo* activationInfo);

        virtual ~FlowNode_SnsSubscribe() {}

    protected:
        const char* GetFlowNodeDescription() const override { return _HELP("Push a message to an AWS Queue"); }
        void ProcessEvent_Internal(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* activationInfo) override;
        Aws::Vector<SInputPortConfig> GetInputPorts() const override;
        Aws::Vector<SOutputPortConfig> GetOutputPorts() const override;

        virtual const char* GetClassTag() const override;
    private:

        void ApplyResult(const Aws::SNS::Model::SubscribeRequest&, const Aws::SNS::Model::SubscribeOutcome& outcome, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);

        enum EInputs
        {
            EIP_Subscribe = EIP_StartIndex,
            EIP_Protocol,
            EIP_TopicClient,
            EIP_Endpoint
        };

        enum EOutputs
        {
            EOP_SubscriptionArn = EOP_StartIndex
        };

        bool m_isValid;
        LmbrAWS::SNS::TopicClientInputPort m_topicClientPort {
            EIP_TopicClient
        };
    };
} // namespace AWS